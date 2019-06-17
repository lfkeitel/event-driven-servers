/*
   Copyright (C) 1999-2016 Marc Huber (Marc.Huber@web.de)
   All rights reserved.

   Redistribution and use in source and binary  forms,  with or without
   modification, are permitted provided  that  the following conditions
   are met:

   1. Redistributions of source code  must  retain  the above copyright
      notice, this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions  and  the following disclaimer in
      the  documentation  and/or  other  materials  provided  with  the
      distribution.

   3. The end-user documentation  included with the redistribution,  if
      any, must include the following acknowledgment:

          This product includes software developed by Marc Huber
	  (Marc.Huber@web.de).

      Alternately,  this  acknowledgment  may  appear  in  the software
      itself, if and wherever such third-party acknowledgments normally
      appear.

   THIS SOFTWARE IS  PROVIDED  ``AS IS''  AND  ANY EXPRESSED OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
   IN NO EVENT SHALL  ITS  AUTHOR  BE  LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
   BUT NOT LIMITED  TO,  PROCUREMENT OF  SUBSTITUTE  GOODS OR SERVICES;
   LOSS OF USE,  DATA,  OR PROFITS;  OR  BUSINESS INTERRUPTION) HOWEVER
   CAUSED AND ON ANY THEORY OF LIABILITY,  WHETHER IN CONTRACT,  STRICT
   LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN
   ANY WAY OUT OF THE  USE  OF  THIS  SOFTWARE,  EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
 */

#include "headers.h"
#include "misc/version.h"
#include <sys/resource.h>
#include <grp.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <netinet/tcp.h>
#include "misc/buffer.h"
#include "misc/strops.h"
#include "mavis/log.h"

static const char rcsid[] __attribute__ ((used)) = "$Id: main.c,v 1.277 2019/05/29 10:09:33 marc Exp marc $";

struct config config;		/* configuration data */

static void die(int signum)
{
    report(NULL, LOG_INFO, ~0, "Received signal %d, shutting down.", signum);
    tac_exit(EX_OK);
}

int die_when_idle = 0;
static struct context *ctx_spawnd = NULL;

static void cleanup_spawnd(struct context *ctx __attribute__ ((unused)), int cur __attribute__ ((unused)))
{
    if (ctx_spawnd) {
	io_close(ctx->io, ctx_spawnd->sock);
	ctx_spawnd = NULL;
    }

    if (common_data.users_cur == 0 /*&& logs_flushed() FIXME */ ) {
	drop_mcx();
	report(NULL, LOG_INFO, ~0, "Terminating, no longer needed.");
	exit(EX_OK);
    }

    die_when_idle = -1;
    set_proctitle(ACCEPT_NEVER);
}

static int compare_session(const void *a, const void *b)
{
    return ((tac_session *) a)->session_id - ((tac_session *) b)->session_id;
}

static u_int context_id = 0;

struct context *new_context(struct io_context *io, tac_realm * r)
{
    struct context *c = calloc(1, sizeof(struct context));
    c->io = io;
    c->pool = mempool_create();
    RB_insert(c->pool, c);
    if (r) {
	c->sessions = RB_tree_new(compare_session, NULL);
	c->id = context_id++;
	c->realm = r;
	c->debug = r->debug;
	c->timeout = r->timeout;
	c->dns_timeout = r->dns_timeout;
	c->anon_enable = r->anon_enable;
	c->augmented_enable = r->augmented_enable;
	c->key = r->key;
	c->authfallback = r->authfallback;
	c->single_connection = r->single_connection;
	c->lookup_revmap = r->lookup_revmap;
	c->authen_max_attempts = r->authen_max_attempts;
	c->authfail_delay = r->authfail_delay;
	c->nac_realm = r->nac_realm;
	c->aaa_realm = r->aaa_realm;
	c->debug = r->debug;
	c->map_pap_to_login = r->map_pap_to_login;
    } else {
	c->debug = common_data.debug;
    }
    return c;
}

static sigset_t master_set;
static void process_signals(void)
{
    sigprocmask(SIG_UNBLOCK, &master_set, NULL);
    sigprocmask(SIG_SETMASK, &master_set, NULL);
}

#ifdef WITH_LWRES
struct io_dns_ctx *idc = NULL;
radixtree_t *dns_tree_ptr_dynamic[2];
static time_t dnspurge_last = 0;
#endif
radixtree_t *dns_tree_ptr_static = NULL;

static void periodics(struct context *ctx, int cur __attribute__ ((unused)))
{
    struct scm_data sd;

    io_sched_renew_proc(ctx->io, ctx, (void *) periodics);
    process_signals();
    io_child_reap();

    sd.type = SCM_DYING;

    if (!die_when_idle && config.suicide && (config.suicide < io_now.tv_sec)) {
	report(NULL, LOG_INFO, ~0, "Retire timeout is up. Told parent about this.");
	common_data.scm_send_msg(ctx_spawnd->sock, &sd, -1);
	die_when_idle = -1;
    }

    sd.type = SCM_KEEPALIVE;
    if (ctx_spawnd && !die_when_idle && common_data.scm_send_msg(ctx_spawnd->sock, &sd, -1))
	die_when_idle = -1;

    if (common_data.users_cur == 0 && die_when_idle)
	cleanup_spawnd(ctx, -1 /* unused */);

    expire_dynamic_users();

#ifdef WITH_LWRES
    /* purge old DNS cache */
    if (dnspurge_last + config.dns_caching_period < io_now.tv_sec) {
	dnspurge_last = io_now.tv_sec;
	radix_drop(&dns_tree_ptr_dynamic[1], NULL);
	dns_tree_ptr_dynamic[1] = dns_tree_ptr_dynamic[0];
	dns_tree_ptr_dynamic[0] = NULL;
    }
#endif
}

static void periodics_ctx(struct context *ctx, int cur __attribute__ ((unused)))
{
    rb_node_t *rbn, *rbnext;

    if (!ctx->out && !ctx->delayed && (ctx->timeout || ctx->dying) && (ctx->last_io + ctx->timeout < io_now.tv_sec)) {
	cleanup(ctx, ctx->sock);
	return;
    }

    for (rbn = RB_first(ctx->sessions); rbn; rbn = rbnext) {
	tac_session *s = RB_payload(rbn, tac_session *);
	rbnext = RB_next(rbn);
	if (s->timeout < io_now.tv_sec)
	    cleanup_session(s);

    }

    tac_script_expire_exec_context(ctx);

    if (ctx->cleanup_when_idle && !ctx->out && !ctx->delayed && !RB_first(ctx->sessions) && !RB_first(ctx->shellctxcache))
	cleanup(ctx, ctx->sock);
    else
	io_sched_renew_proc(ctx->io, ctx, (void *) periodics_ctx);
}

static void accept_control(struct context *, int __attribute__ ((unused)));
static void accept_control_raw(int, struct scm_data_accept *);
static void setup_signals(void);

static struct pwdat pwdat_unknown;

int main(int argc, char **argv, char **envp)
{
    int nfds_max;
    struct rlimit rlim;
    struct scm_data_max sd;
    rb_node_t *rbn;

    scm_main(argc, argv, envp);

    cfg_init();

    memset(&pwdat_unknown, 0, sizeof(struct pwdat));
    pwdat_unknown.type = S_unknown;

    buffer_setsize(0x8000, 0x10);

    dns_tree_ptr_static = radix_new(NULL /* never freed */ , NULL);

    if (!common_data.conffile) {
	common_data.conffile = argv[optind];
	common_data.id = argv[optind + 1];
    }
    cfg_read_config(common_data.conffile, parse_decls, common_data.id ? common_data.id : common_data.progname);

    if (common_data.parse_only)
	tac_exit(EX_OK);

    if (config.userid)
	setgroups(0, NULL);

    if (config.groupid && setgid(config.groupid))
	report(NULL, LOG_ERR, ~0, "Can't set group id to %d: %s", (int) config.groupid, strerror(errno));

    if (config.userid && setuid(config.userid))
	report(NULL, LOG_ERR, ~0, "Can't set user id to %d: %s", (int) config.userid, strerror(errno));

    for (rbn = RB_first(config.realms); rbn; rbn = RB_next(rbn)) {
	tac_realm *r = RB_payload(rbn, tac_realm *);
	if (!r->mavis_userdb)
	    r->mavis_pap_prefetch = r->mavis_login_prefetch = 0;
	if (r->caching_period < 11)
	    r->caching_period = 0;
    }

    signal(SIGTERM, die);
    signal(SIGPIPE, SIG_IGN);

    report(NULL, LOG_INFO, ~0, "Version " VERSION " initialized");

    umask(022);

    mavis_detach();

    setup_sig_segv(common_data.coredumpdir, common_data.gcorepath, common_data.debug_cmd);

    if (common_data.singleprocess) {
	common_data.scm_accept = accept_control_raw;
    } else {
	common_data.io = io_init();
	setproctitle_init(argv, envp);
	setup_signals();
	ctx_spawnd = new_context(common_data.io, NULL);
	ctx_spawnd->sock = dup(0);
	dup2(2, 0);
	io_register(common_data.io, ctx_spawnd->sock, ctx_spawnd);
	io_set_cb_i(common_data.io, ctx_spawnd->sock, (void *) accept_control);
	io_clr_cb_o(common_data.io, ctx_spawnd->sock);
	io_set_cb_h(common_data.io, ctx_spawnd->sock, (void *) cleanup_spawnd);
	io_set_cb_e(common_data.io, ctx_spawnd->sock, (void *) cleanup_spawnd);
	io_set_i(common_data.io, ctx_spawnd->sock);
    }

    if (getrlimit(RLIMIT_NOFILE, &rlim)) {
	report(NULL, LOG_ERR, ~0, "rlimit: %s", strerror(errno));
	exit(EX_SOFTWARE);
    }

    nfds_max = (int) rlim.rlim_cur;
    sd.type = SCM_MAX;
    sd.max = nfds_max / 4;
    if (ctx_spawnd)
	common_data.scm_send_msg(ctx_spawnd->sock, (struct scm_data *) &sd, -1);

    io_sched_add(common_data.io, new_context(common_data.io, NULL), (void *) periodics, 60, 0);

#ifdef WITH_LWRES
    idc = io_dns_init(common_data.io);
    dns_tree_ptr_dynamic[0] = NULL;
    dns_tree_ptr_dynamic[1] = NULL;
    dnspurge_last = io_now.tv_sec;
#endif				/* WITH_LWRES */

    init_mcx();

    set_proctitle(ACCEPT_YES);
    io_main(common_data.io);
}

void cleanup(struct context *ctx, int cur)
{
    rb_node_t *t, *u;
    struct scm_data sd;

    if (ctx == ctx_spawnd) {
	cleanup_spawnd(ctx, cur);
	return;
    }

    while (io_sched_pop(ctx->io, ctx));
    io_close(ctx->io, ctx->sock);

    for (t = RB_first(ctx->sessions); t; t = u) {
	u = RB_next(t);
	cleanup_session(RB_payload(t, tac_session *));
    }

    if (ctx->sessions)
	RB_tree_delete(ctx->sessions);

    if (ctx->shellctxcache)
	RB_tree_delete(ctx->shellctxcache);

#ifdef WITH_LWRES
    if (ctx->revmap_pending) {
	io_dns_cancel(idc, ctx);
	if (ctx->revmap_timedout)
	    add_revmap(&ctx->nas_address, NULL);
    }
#endif

    mempool_destroy(ctx->pool);

    common_data.users_cur--;

    sd.type = SCM_DONE;
    if (ctx_spawnd)
	common_data.scm_send_msg(ctx_spawnd->sock, &sd, -1);
    set_proctitle(die_when_idle ? ACCEPT_NEVER : ACCEPT_YES);

    if (ctx_spawnd && common_data.users_cur == 0 && die_when_idle)
	cleanup(ctx_spawnd, 0);
}

void catchhup(int i __attribute__ ((unused)))
{
    signal(SIGHUP, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    if (ctx_spawnd)
	cleanup(ctx_spawnd, 0);
    die_when_idle = -1;
    report(NULL, LOG_INFO, ~0, "SIGHUP: No longer accepting new connections.");
}

static void setup_signals()
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, catchhup);
    signal(SIGTERM, catchhup);
    sigfillset(&master_set);
    sigdelset(&master_set, SIGSEGV);
    sigprocmask(SIG_SETMASK, &master_set, NULL);
}

static void accept_control_raw(int s, struct scm_data_accept *sd)
{
    static int id = 0;
    int one = 1;
    char realm[40];
    char afrom[256];
    char *hint = "", *f;

    *realm = 0;

    tac_host *arr[129], *h = NULL;
    sockaddr_union from;
    int i;
    int arr_min = 0, arr_max = 0;
    struct in6_addr addr;
    tac_realm *r;

    socklen_t from_len = (socklen_t) sizeof(from);
    memset(&from, 0, sizeof(from));

    if (getpeername(s, &from.sa, &from_len)) {
	struct scm_data d;
	close(s);
	report(NULL, LOG_DEBUG, DEBUG_PACKET_FLAG, "getpeername: %s", strerror(errno));
	d.type = SCM_DONE;
	common_data.scm_send_msg(ctx_spawnd->sock, &d, -1);
	return;
    }
    fcntl(s, F_SETFD, FD_CLOEXEC);
    fcntl(s, F_SETFL, O_NONBLOCK);

    su_convert(&from, AF_INET);
    su_ptoh(&from, &addr);

    setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char *) &one, (socklen_t) sizeof(one));
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char *) &one, (socklen_t) sizeof(one));

    memset(arr, 0, sizeof(arr));

    if (*sd->realm)
	r = get_realm(sd->realm);
    else
	r = config.default_realm;

    if (radix_lookup(r->hosttree, &addr, (void *) arr)) {

	for (arr_max = 0; arr_max < 129 && arr[arr_max]; arr_max++);
	arr_max--;

	for (i = arr_max; i > -1 && !arr[i]->orphan; i--);
	arr_min = i;

	for (i = arr_max; i > arr_min && arr[i]->valid_for_nas == TRISTATE_DUNNO; i--);

	if ((i > arr_min && arr[i]->valid_for_nas != TRISTATE_NO) || (i == arr_min)) {
	    for (i = arr_max; i > arr_min; i--)
		if (arr[i]->key) {
		    h = arr[i];
		    break;
		}

	    if (!r->key && !h)
		hint = ": no encryption key found";
	}
    } else
	hint = ": host unknown";

    f = su_ntop(&from, afrom, sizeof(afrom)) ? afrom : "<unknown>";

    if (h || r->key) {
	int priv_lvl, enable_implied[TAC_PLUS_PRIV_LVL_MAX + 1];
	struct context *ctx = new_context(common_data.io, r);
	int hc = 1;
	char *k;
	tac_session session;

	memset(&session, 0, sizeof(tac_session));
	session.ctx = ctx;

	for (i = arr_max; i > arr_min; i--)
	    report(&session, LOG_DEBUG, DEBUG_PACKET_FLAG | DEBUG_CONFIG_FLAG, "cidr match level %d = %s", i, arr[i]->name ? arr[i]->name : "(unnamed)");

	if (h && h->key)
	    ctx->key = h->key;

	k = ctx->key ? ctx->key->key : "<NULL>";

	if (*(sd->realm))
	    report(&session, LOG_DEBUG, DEBUG_PACKET_FLAG, "connection request from %s (realm: %s, key: %s)", f, sd->realm, k);
	else
	    report(&session, LOG_DEBUG, DEBUG_PACKET_FLAG, "connection request from %s (key: %s)", f, k);

	ctx->sock = s;
	common_data.users_cur++;

	set_proctitle(die_when_idle ? ACCEPT_NEVER : ACCEPT_YES);

	/* copy various settings from host hierarchy to context */

	for (i = arr_max; i > arr_min; i--)
	    if (arr[i]->anon_enable != TRISTATE_DUNNO) {
		ctx->anon_enable = arr[i]->anon_enable;
		break;
	    }

	for (i = arr_max; i > arr_min; i--)
	    if (arr[i]->augmented_enable != TRISTATE_DUNNO) {
		ctx->augmented_enable = arr[i]->augmented_enable;
		break;
	    }

	for (i = arr_max; i > arr_min; i--)
	    if (arr[i]->single_connection != TRISTATE_DUNNO) {
		ctx->single_connection = arr[i]->single_connection;
		break;
	    }

	for (i = arr_max; i > arr_min; i--)
	    if (arr[i]->authfallback != TRISTATE_DUNNO) {
		ctx->authfallback = arr[i]->authfallback;
		break;
	    }

	for (i = arr_max; i > arr_min; i--)
	    if (arr[i]->cleanup_when_idle != TRISTATE_DUNNO) {
		ctx->cleanup_when_idle = arr[i]->cleanup_when_idle;
		break;
	    }

	for (i = arr_max; i > arr_min; i--)
	    if (arr[i]->lookup_revmap != TRISTATE_DUNNO) {
		ctx->lookup_revmap = arr[i]->lookup_revmap;
		break;
	    }

	for (i = arr_max; i > arr_min; i--)
	    if (arr[i]->welcome_banner) {
		ctx->welcome_banner = arr[i]->welcome_banner;
		break;
	    }

	for (i = arr_max; i > arr_min; i--)
	    if (arr[i]->welcome_banner_fallback) {
		ctx->welcome_banner_fallback = arr[i]->welcome_banner_fallback;
		break;
	    }

	for (i = arr_max; i > arr_min; i--)
	    if (arr[i]->reject_banner) {
		ctx->reject_banner = arr[i]->reject_banner;
		break;
	    }

	for (i = arr_max; i > arr_min; i--)
	    if (arr[i]->authfail_banner) {
		ctx->authfail_banner = arr[i]->authfail_banner;
		break;
	    }

	for (i = arr_max; i > arr_min; i--)
	    if (arr[i]->motd) {
		ctx->motd = arr[i]->motd;
		break;
	    }

	for (i = arr_max; i > arr_min; i--) {
	    ctx->debug |= arr[i]->debug;
	    if (arr[i]->debug & DEBUG_NONE_FLAG)
		break;
	}

	for (i = arr_max; i > arr_min; i--)
	    if (arr[i]->authen_max_attempts > -1) {
		ctx->authen_max_attempts = arr[i]->authen_max_attempts;
		break;
	    }

	for (i = arr_max; i > arr_min; i--)
	    if (arr[i]->authfail_delay > -1) {
		ctx->authfail_delay = arr[i]->authfail_delay;
		break;
	    }

	for (i = arr_max; i > arr_min; i--)
	    if (arr[i]->timeout > -1) {
		ctx->timeout = arr[i]->timeout;
		break;
	    }

	for (i = arr_max; i > arr_min; i--)
	    if (arr[i]->dns_timeout > -1) {
		ctx->dns_timeout = arr[i]->dns_timeout;
		break;
	    }

	for (i = arr_max; i > arr_min; i--)
	    if (arr[i]->nac_realm) {
		ctx->nac_realm = arr[i]->nac_realm;
		break;
	    }

	for (i = arr_max; i > arr_min; i--)
	    if (arr[i]->aaa_realm) {
		ctx->aaa_realm = arr[i]->aaa_realm;
		break;
	    }

	for (i = arr_max; i > arr_min; i--)
	    if (arr[i]->realm) {
		ctx->realm = arr[i]->realm;
		break;
	    }

	for (i = arr_max; i > arr_min; i--)
	    if (arr[i]->map_pap_to_login) {
		ctx->map_pap_to_login = arr[i]->map_pap_to_login;
		break;
	    }

	/* make note of host objects */
	hc = arr_max - arr_min + 1;

	ctx->hostchain = mempool_malloc(ctx->pool, hc * sizeof(tac_host *));

	for (hc = 0, i = arr_max; i > arr_min; i--, hc++)
	    ctx->hostchain[hc] = arr[i];

	/* copy enable passwords to context */

	for (priv_lvl = TAC_PLUS_PRIV_LVL_MIN; priv_lvl <= TAC_PLUS_PRIV_LVL_MAX; priv_lvl++)
	    enable_implied[priv_lvl] = TAC_PLUS_PRIV_LVL_MAX + 1;

	for (priv_lvl = TAC_PLUS_PRIV_LVL_MIN; priv_lvl <= TAC_PLUS_PRIV_LVL_MAX; priv_lvl++)
	    for (i = arr_max; i > arr_min; i--) {
		if (arr[i]->enable[priv_lvl]) {
		    if (!arr[i]->enable_implied[priv_lvl]) {
			ctx->enable[priv_lvl] = arr[i]->enable[priv_lvl];
			enable_implied[priv_lvl] = arr[i]->enable_implied[priv_lvl];
			break;
		    }
		    if (arr[i]->enable_implied[priv_lvl] < enable_implied[priv_lvl]) {
			ctx->enable[priv_lvl] = arr[i]->enable[priv_lvl];
			enable_implied[priv_lvl] = arr[i]->enable_implied[priv_lvl];
		    }
		}
	    }

	/* make sure all enable password pointers are set */
	for (priv_lvl = TAC_PLUS_PRIV_LVL_MIN; priv_lvl <= TAC_PLUS_PRIV_LVL_MAX; priv_lvl++)
	    if (!ctx->enable[priv_lvl])
		ctx->enable[priv_lvl] = &pwdat_unknown;

	ctx->nas_address = addr;
	ctx->nas_address_ascii = mempool_strdup(ctx->pool, afrom);

	io_register(ctx->io, ctx->sock, ctx);
	io_set_cb_i(ctx->io, ctx->sock, (void *) tac_read);
	io_set_cb_o(ctx->io, ctx->sock, (void *) tac_write);
	io_set_cb_h(ctx->io, ctx->sock, (void *) cleanup);
	io_set_cb_e(ctx->io, ctx->sock, (void *) cleanup);
	io_set_i(ctx->io, ctx->sock);
	io_sched_add(ctx->io, ctx, (void *) periodics_ctx, 60, 0);

	get_revmap_nas(ctx);

	if (config.retire && (++id == config.retire) && !common_data.singleprocess) {
	    struct scm_data d;
	    report(&session, LOG_INFO, ~0, "Retire limit reached. Told parent about this.");
	    d.type = SCM_DYING;
	    common_data.scm_send_msg(ctx_spawnd->sock, &d, -1);
	}
    } else {
	struct scm_data d;
	char rs[sizeof(realm) + 20];

	close(s);

	if (*realm)
	    snprintf(rs, sizeof(rs), "(realm: %s) ", realm);
	else
	    rs[0] = 0;

	report(NULL, LOG_INFO, ~0, "session request from %s %srejected%s", f, rs, hint);

	d.type = SCM_DONE;
	common_data.scm_send_msg(ctx_spawnd->sock, &d, -1);
    }
}

static void accept_control(struct context *ctx, int cur)
{
    int s;
    struct scm_data_accept sd;

    if (common_data.scm_recv_msg(cur, &sd, sizeof(sd), &s)) {
	cleanup_spawnd(ctx, cur);
	return;
    }
    switch (sd.type) {
    case SCM_MAY_DIE:
	cleanup_spawnd(ctx, cur);
	return;
    case SCM_ACCEPT:
	accept_control_raw(s, &sd);
	break;
    default:
	if (s > -1)
	    close(s);
    }
}

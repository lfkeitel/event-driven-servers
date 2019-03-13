/*
 * signals.c (C)1999-2011 by Marc Huber <Marc.Huber@web.de>
 *
 * $Id: spawnd_signals.c,v 1.20 2015/03/14 06:11:28 marc Exp $
 *
 */

#include "spawnd_headers.h"
#include "misc/sysconf.h"
#include <sysexits.h>

static const char rcsid[] __attribute__ ((used)) = "$Id: spawnd_signals.c,v 1.20 2015/03/14 06:11:28 marc Exp $";

static sigset_t master_set;

static void catchhup(int i __attribute__ ((unused)))
{
    int j;
    struct scm_data sd;
    sd.type = SCM_MAY_DIE;

    for (j = 0; j < common_data.servers_cur; j++)
	common_data.scm_send_msg(spawnd_data.server_arr[j]->fn, &sd, -1);

    logmsg("SIGHUP: restarting");
    pid_unlink(&common_data.pidfile);
    execve(common_data.progpath, common_data.argv, common_data.envp);
    exit(EX_OSERR);
}

static void catchterm(int i __attribute__ ((unused)))
{
    int j;
    struct scm_data sd;
    sd.type = SCM_MAY_DIE;

    for (j = 0; j < common_data.servers_cur; j++)
	common_data.scm_send_msg(spawnd_data.server_arr[j]->fn, &sd, -1);

    logmsg("SIGTERM: exiting");
    pid_unlink(&common_data.pidfile);
    exit(EX_OK);
}

void spawnd_setup_signals()
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    if (spawnd_data.inetd)
	signal(SIGHUP, catchterm);
    else
	signal(SIGHUP, catchhup);
    signal(SIGINT, catchterm);
    signal(SIGTERM, catchterm);

    sigfillset(&master_set);
    sigdelset(&master_set, SIGSEGV);
    sigdelset(&master_set, SIGTERM);
    sigdelset(&master_set, SIGINT);
    if (spawnd_data.inetd)
	sigdelset(&master_set, SIGHUP);
    sigprocmask(SIG_SETMASK, &master_set, NULL);
}

void spawnd_process_signals()
{
    sigprocmask(SIG_UNBLOCK, &master_set, NULL);
    sigprocmask(SIG_SETMASK, &master_set, NULL);
}

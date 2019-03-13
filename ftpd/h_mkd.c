/*
 * h_mkd.c
 * (C)1998-2011 by Marc Huber <Marc.Huber@web.de>
 * All rights reserved.
 *
 * $Id: h_mkd.c,v 1.10 2015/03/14 06:11:25 marc Exp marc $
 *
 */

#include "headers.h"

static const char rcsid[] __attribute__ ((used)) = "$Id: h_mkd.c,v 1.10 2015/03/14 06:11:25 marc Exp marc $";

void h_mkd(struct context *ctx, char *arg)
{
    char *t;
    struct stat st;

    DebugIn(DEBUG_COMMAND);

    if ((t = buildpath(ctx, arg)) && !pickystat_path(ctx, &st, t) &&
	(acl_set_umask(ctx, arg, t), (!ctx->anonymous || check_incoming(ctx, t, 0))) && !mkdir(t, ctx->chmod_dirmask | (0755 & ~ctx->umask)))
	reply(ctx, MSG_250_Directory_created);
    else
	reply(ctx, MSG_550_Permission_denied);

    DebugOut(DEBUG_COMMAND);
}

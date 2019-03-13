/*
 * h_site_umask.c
 *
 * (C)1998-2011 by Marc Huber <Marc.Huber@web.de>
 * All rights reserved.
 *
 * $Id: h_site_umask.c,v 1.10 2015/03/14 06:11:26 marc Exp marc $
 *
 */

#include "headers.h"

static const char rcsid[] __attribute__ ((used)) = "$Id: h_site_umask.c,v 1.10 2015/03/14 06:11:26 marc Exp marc $";

void h_site_umask(struct context *ctx, char *arg)
{
    DebugIn(DEBUG_COMMAND);

    if (arg && (1 == sscanf(arg, "%o", &ctx->umask)))
	ctx->umask_set = 1;

    replyf(ctx, MSG_200_umask, ctx->umask);

    DebugOut(DEBUG_COMMAND);
}

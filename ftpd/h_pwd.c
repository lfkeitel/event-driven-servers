/*
 * h_pwd.c
 *
 * (C)1998-2011 by Marc Huber <Marc.Huber@web.de>
 * All rights reserved
 *
 * $Id: h_pwd.c,v 1.10 2015/03/14 06:11:26 marc Exp marc $
 *
 */

#include "headers.h"

static const char rcsid[] __attribute__ ((used)) = "$Id: h_pwd.c,v 1.10 2015/03/14 06:11:26 marc Exp marc $";

void h_pwd(struct context *ctx, char *arg __attribute__ ((unused)))
{
    DebugIn(DEBUG_COMMAND);
    replyf(ctx, MSG_257_current_dir, ctx->cwdlen == ctx->rootlen ? "/" : ctx->cwd + ctx->rootlen);
    DebugOut(DEBUG_COMMAND);
}

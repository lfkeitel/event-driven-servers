/*
 * h_pass.c
 * (C)1998-2011 by Marc Huber <Marc.Huber@web.de>
 *
 * $Id: h_pass.c,v 1.11 2015/03/14 06:11:25 marc Exp marc $
 *
 */

#include "headers.h"

static const char rcsid[] __attribute__ ((used)) = "$Id: h_pass.c,v 1.11 2015/03/14 06:11:25 marc Exp marc $";

void h_pass(struct context *ctx, char *arg)
{
    DebugIn(DEBUG_COMMAND);

    if (ctx->state == ST_user) {
	if (arg[0] == '-')
	    ctx->multiline_banners = 0;
	auth_mavis(ctx, arg);
    } else
	reply(ctx, MSG_503_USER_before_PASS);

    DebugOut(DEBUG_COMMAND);
}

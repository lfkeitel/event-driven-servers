/*
 * connected.c
 * (C)1999-2011 by Marc Huber <Marc.Huber@web.de>
 * All rights reserved.
 *
 * $Id: connected.c,v 1.10 2015/03/14 06:11:31 marc Exp marc $
 *
 */

#include "headers.h"

static const char rcsid[] __attribute__ ((used)) = "$Id: connected.c,v 1.10 2015/03/14 06:11:31 marc Exp marc $";

void connected(struct context *ctx, int cur)
{
    DebugIn(DEBUG_NET);

    io_set_cb_o(ctx->io, cur, (void *) buffer2socket);
    io_set_cb_i(ctx->io, cur, (void *) socket2buffer);
    io_set_cb_e(ctx->io, cur, (void *) cleanup_error);
    io_set_cb_h(ctx->io, cur, (void *) cleanup_error);

    io_clr_i(ctx->io, cur);
    io_clr_o(ctx->io, cur);

    if (ctx->ifn > -1) {
	io_set_i(ctx->io, ctx->ifn);
	io_set_i(ctx->io, cur);
    } else
	cleanup(ctx, cur);

    DebugOut(DEBUG_NET);
}

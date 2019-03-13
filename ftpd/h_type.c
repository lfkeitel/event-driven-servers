/*
 * h_type.c
 *
 * (C)1998-2011 by Marc Huber <Marc.Huber@web.de>
 * All rights reserved.
 *
 * $Id: h_type.c,v 1.12 2015/03/14 06:11:26 marc Exp marc $
 *
 */

#include "headers.h"
#include <ctype.h>

static const char rcsid[] __attribute__ ((used)) = "$Id: h_type.c,v 1.12 2015/03/14 06:11:26 marc Exp marc $";

void h_type(struct context *ctx, char *arg)
{
    DebugIn(DEBUG_COMMAND);

    ctx->use_ascii = tolower((int) *arg) == 'a';
    replyf(ctx, MSG_200_transfer_type, ctx->use_ascii ? "ASCII" : "BINARY");

    if (ctx->use_ascii) {
	ctx->io_offset = ctx->io_offset_start = 0;
	ctx->io_offset_end = -1;
    }

    DebugOut(DEBUG_COMMAND);
}

/*
 * h_dele.c
 * (C)1998-2011 by Marc Huber <Marc.Huber@web.de>
 *
 * $Id: h_dele.c,v 1.10 2015/03/14 06:11:25 marc Exp marc $
 *
 */

#include "headers.h"

static const char rcsid[] __attribute__ ((used)) = "$Id: h_dele.c,v 1.10 2015/03/14 06:11:25 marc Exp marc $";

void h_dele(struct context *ctx, char *arg)
{
    char *t;
    struct stat st;

    DebugIn(DEBUG_COMMAND);

    if ((t = buildpath(ctx, arg)) && (!pickystat(ctx, &st, t)) && S_ISREG(st.st_mode) && !unlink(t)) {
	quota_add(ctx, -st.st_size);
	reply(ctx, MSG_250_File_removed);
    } else
	reply(ctx, MSG_550_Permission_denied);

    DebugOut(DEBUG_COMMAND);
}

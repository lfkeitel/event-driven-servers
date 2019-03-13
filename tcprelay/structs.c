/*
 * structs.c
 * (C)1999-2011 by Marc Huber <Marc.Huber@web.de>
 * All rights reserved.
 *
 * $Id: structs.c,v 1.10 2015/03/14 06:11:31 marc Exp marc $
 *
 */

#include "headers.h"

static const char rcsid[] __attribute__ ((used)) = "$Id: structs.c,v 1.10 2015/03/14 06:11:31 marc Exp marc $";

struct context *new_context(struct io_context *io)
{
    struct context *c = Xcalloc(1, sizeof(struct context));
    c->io = io;
    c->ifn = c->ofn = -1;
    c->con_arr_idx = -1;

    return c;
}

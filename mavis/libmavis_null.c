/*
 * libmavis_null.c
 * (C)1999-2011 by Marc Huber <Marc.Huber@web.de>
 * All rights reserved.
 *
 * $Id: libmavis_null.c,v 1.9 2015/03/14 06:11:28 marc Exp $
 *
 */

#define __MAVIS_null__
#define MAVIS_name "null"

#include <sys/types.h>

#include "misc/strops.h"

static const char rcsid[] __attribute__ ((used)) = "$Id: libmavis_null.c,v 1.9 2015/03/14 06:11:28 marc Exp $";

#include "mavis.h"

#define HAVE_mavis_parse_in
static int mavis_parse_in(mavis_ctx * mcx, struct sym *sym)
{
    while (1) {
	switch (sym->code) {
	case S_script:
	    mavis_script_parse(mcx, sym);
	    continue;
	case S_eof:
	case S_closebra:
	    return MAVIS_CONF_OK;
	default:
	    parse_error_expect(sym, S_script, S_closebra, S_unknown);
	}
    }
}

#include "mavis_glue.c"

/*
 * h_syst.c
 * (C)1998-2011 by Marc Huber <Marc.Huber@web.de>
 * All rights reserved.
 *
 * Could obviously use utsname below, but the data is pretty static anyway.
 * Keep it that way.
 *
 * $Id: h_syst.c,v 1.12 2015/03/14 06:11:26 marc Exp marc $
 *
 */

#include "headers.h"
#include <ctype.h>
#include <sys/utsname.h>

static const char rcsid[] __attribute__ ((used)) = "$Id: h_syst.c,v 1.12 2015/03/14 06:11:26 marc Exp marc $";

void h_syst(struct context *ctx, char *arg __attribute__ ((unused)))
{
    DebugIn(DEBUG_COMMAND);

    reply(ctx, "215 UNIX Type: L8"
#if defined (__linux__)
	  " Version: LINUX"
#endif
#if defined (__sun__)
	  " Version: SUNOS"
#endif
#if defined (__FreeBSD__)
	  " Version: FREEBSD"
#endif
#if defined (__NetBSD__)
	  " Version: NETBSD"
#endif
#if defined (__OpenBSD__)
	  " Version: OPENBSD"
#endif
#if defined (__APPLE__)
	  " Version: DARWIN"
#endif
	  "\r\n");

    DebugOut(DEBUG_COMMAND);
}

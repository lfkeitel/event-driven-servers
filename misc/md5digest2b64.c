/*
 * md5digest2b64.c
 * (C) 2000 by Marc Huber <Marc.Huber@web.de>
 * All rights reserved.
 *
 * $Id: md5digest2b64.c,v 1.6 2015/03/14 06:11:29 marc Exp marc $
 *
 */

#ifndef __GNUC__
#define __attribute__(A)
#endif				/* __GNUC__ */

static const char rcsid[] __attribute__ ((used)) = "$Id: md5digest2b64.c,v 1.6 2015/03/14 06:11:29 marc Exp marc $";

#include "misc/md5digest2b64.h"

char *md5digest2b64(u_char * md5, char *digest)
{
    int len = 30;
    base64enc((char *) md5, 16, digest, &len);
    return digest;
}

/*
 * parse_digest_md5.c
 * (C) 2000 by Marc Huber <Marc.Huber@web.de>
 * All rights reserved.
 *
 * $Id: parse_digest_md5.c,v 1.6 2015/03/14 06:11:29 marc Exp marc $
 *
 */

#ifndef __GNUC__
#define __attribute__(A)
#endif				/* __GNUC__ */

static const char rcsid[] __attribute__ ((used)) = "$Id: parse_digest_md5.c,v 1.6 2015/03/14 06:11:29 marc Exp marc $";

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "mavis/parse_digest_md5.h"
#include "misc/memops.h"

/* Take care -- this must be consistent with the DIGMD5_* definitions
 * in memops.h
 */
static char *digmd5_key[] = {
    "nonce",
    "cnonce",
    "user",
    "realm",
    "digest-uri",
    "response",
    "charset",
    "qop",
    "algorithm",
    "nc",
    NULL
};

static char *get_key(char *string, char **remainder)
{
    char *start = NULL;
    *remainder = NULL;
    if (string) {
	while (*string && isspace((int) *string))
	    string++;
	while (*string) {
	    switch (*string) {
	    case '=':
		*string++ = 0;
		*remainder = string;
		return start;
	    default:
		if (!start)
		    start = string;
	    }
	    if (*string)
		string++;
	}
    }
    *remainder = NULL;
    return start;
}

static char *get_token(char *string, char **remainder)
{
    int quoted = 0;
    char *start = NULL;
    *remainder = NULL;
    if (string) {
	while (*string && isspace((int) *string))
	    string++;
	while (*string) {
	    switch (*string) {
	    case ',':
		if (quoted)
		    break;
	    case '"':
		if (start) {
		    *string++ = 0;
		    if (*string == ',')
			string++;
		    *remainder = string;
		    return start;
		} else
		    start = string + 1, quoted = -1;
		break;
	    case '\\':
		if (!quoted)
		    memmove(string, string + 1, strlen(string + 1));
		break;
	    default:
		if (!start)
		    start = string;
	    }
	    if (*string)
		string++;
	}
    }
    *remainder = NULL;
    return start;
}

int parse_digest_md5(char *string, char **arr)
{
    char *token = NULL, *key, *remainder = string;
    int i;
    for (i = 0; i < DIGMD5_ARRSIZE; i++)
	arr[i] = NULL;

    do {
	key = get_key(remainder, &remainder);
	if (key && (token = get_token(remainder, &remainder))) {
	    char **k = digmd5_key;
	    for (i = 0; *k && strcasecmp(key, *k); k++, i++);
	    if (*k)
		arr[i] = token;
	}
    }
    while (key && remainder && *remainder);

    if (arr[DIGMD5_qop] && strcasecmp(arr[DIGMD5_qop], "auth"))
	return -1;
    if (arr[DIGMD5_algorithm]
	&& strcasecmp(arr[DIGMD5_algorithm], "md5-sess"))
	return -1;
    if (arr[DIGMD5_nonce] && arr[DIGMD5_cnonce] && arr[DIGMD5_user]
	&& arr[DIGMD5_realm] && arr[DIGMD5_uri] && arr[DIGMD5_response])
	return 0;
    return -1;
}

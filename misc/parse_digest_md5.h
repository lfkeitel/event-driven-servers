/*
 * parse_digest_md5.h
 * (C)2000 by Marc Huber <Marc.Huber@web.de>
 * All rights reserved.
 *
 * $Id: parse_digest_md5.h,v 1.3 2005/12/31 12:14:23 huber Exp $
 *
 */

#ifndef __PARSE_DIGEST_MD5_H__
#define __PARSE_DIGEST_MD5_H__

#include <sys/types.h>

int parse_digest_md5(char *, char **);

#define DIGMD5_nonce	0
#define DIGMD5_cnonce	1
#define DIGMD5_user	2
#define DIGMD5_realm	3
#define DIGMD5_uri	4
#define DIGMD5_response	5
#define DIGMD5_charset	6
#define DIGMD5_qop	7
#define DIGMD5_algorithm	8
#define DIGMD5_nc	9
#define DIGMD5_ARRSIZE	10

#endif				/* __PARSE_DIGEST_MD5_H__ */

/*
 * pcre_rewrite.c
 * (C)1999-2011 Marc Huber <Marc.Huber@web.de>
 * All rights reserved.
 *
 * $Id: pcre_rewrite.c,v 1.14 2015/03/14 06:11:27 marc Exp marc $
 *
 */

/*
 * This code was tested with version 2.08 of the PCRE library,
 * available from:
 * ftp://ftp.cus.cam.ac.uk/pub/software/programs/pcre/
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcre.h>
#include "headers.h"

static const char rcsid[] __attribute__ ((used)) = "$Id: pcre_rewrite.c,v 1.14 2015/03/14 06:11:27 marc Exp marc $";

struct pcre_rule {
    pcre *p;
    pcre_extra *pe;
    struct pcre_rule *next;
    char *flags;
    char replacement[1];
};

static struct pcre_rule *pcre_start = NULL, *pcre_last = NULL;

int PCRE_add(char *regex, char *replacement, char *flags)
{
    struct pcre_rule *pr;
    const char *errptr;
    int erroffset;

    if (!flags || !*flags)
	flags = "L";

    pr = Xcalloc(1, sizeof(struct pcre_rule)
		 + strlen(replacement) + strlen(flags) + 1);

    pr->p = pcre_compile(regex, 0, &errptr, &erroffset, NULL);
    if (!pr->p) {
	logmsg("pcre_compile: %s: %s", regex, errptr ? errptr : "(NULL)");
	free(pr);
	return -1;
    }

    pr->pe = pcre_study(pr->p, 0, &errptr);

    strcpy(pr->replacement, replacement);
    pr->flags = pr->replacement + strlen(pr->replacement) + 2;
    strcpy(pr->flags, flags);
    pr->next = NULL;

    if (!pcre_start)
	pcre_start = pcre_last = pr;
    else {
	pcre_last->next = pr;
	pcre_last = pr;
    }

    Debug((DEBUG_PROC, " PCRE_add(%s, %s, %s)\n", regex, replacement, flags));
    return 0;
}

int PCRE_exec(const char *inbuf, char *outbuf, size_t outlen)
{
    int loopmax = 20;
#define OVECSIZE 100
    int ovector[3 * (OVECSIZE + 1)];
    struct pcre_rule *pr = pcre_start;
    size_t tbuflen = 2 * outlen;
    char *tbuf = alloca(tbuflen);

    if (!pcre_start)
	return 0;		/* no match */

    strncpy(tbuf, inbuf, tbuflen);
    tbuf[tbuflen - 1] = 0;

    while (loopmax--) {
	int m;
	if (pr == NULL)
	    pr = pcre_start;
	m = pcre_exec(pr->p, pr->pe, tbuf, (int) strlen(tbuf), 0, 0, ovector, OVECSIZE);
	if (m > -1) {
	    char *o, *t;

	    for (o = outbuf, t = pr->replacement; *t && o < outbuf + tbuflen; t++)
		switch (*t) {
		case '\\':	/* escape next character */
		    if (*(t + 1))
			*o++ = *t++;
		    break;
		case '$':
		    if (*(t + 1)) {
			int num = 0;

			if (isdigit((int) *++t))
			    num = *t - '0';
			else if (*t == '{')
			    for (t++; *t && *t != '}'; t++) {
				if (isdigit((int) *t))
				    num = num * 10 + *t - '0';
			} else
			    continue;

			pcre_copy_substring(tbuf, ovector, m, num, o, (int) (outbuf + outlen - o - 1));
			while (*o)
			    o++;
		    }
		    break;
		default:
		    *o++ = *t;
		}		/* switch */

	    *o = 0;
	    if (!strcmp(pr->flags, "N")) {	/* Next */
		pr = pcre_start;
		strncpy(tbuf, outbuf, tbuflen);
		tbuf[tbuflen - 1] = 0;
		continue;
	    }
	    if (!strcmp(pr->flags, "R"))	/* Reject */
		*outbuf = 0;
	    Debug((DEBUG_PROC, " PCRE_exec(%s) = %s\n", inbuf, outbuf));
	    return -1;
	}			/* if */
	pr = pr->next;
    }				/* while */
    return 0;
}

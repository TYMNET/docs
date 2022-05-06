/************************************************************************
 * getword.c - Input Word 
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    getword.c contains the source modules which are called to input
 *    a word of data.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KS    Initial Draft
 *
 ************************************************************************/
#include "stddef.h"

IMPORT INT gettext();			/* returns input length */
IMPORT INT printf();			/* returns output count */
IMPORT INT sscanf();			/* returns conversion count */

/* _getword - static word input routine
 */
LOCAL BOOL _getword(pp, pc, ps, pw)
    TEXT *pp;				/* pointer to prompt */
    TEXT pc;				/* prompt end character */
    TEXT *ps;				/* pointer to scan string */
    WORD *pw;				/* pointer to destination word */
    {
    TEXT buf[20];			/* scratch input buffer */
    
    (VOID)printf("    %s %c ", pp, pc);	/* output prompt */
    if (gettext(buf, NO) < 0)		/* if ESC hit */
	return (NO);
    *pw = 0;				/* clear word at pw */
    (VOID)sscanf(buf, ps, pw);		/* convert text to word */
    return (YES);
    }

/* getword - input word (decimal)
 */
BOOL getword(pp, pw)
    TEXT *pp;				/* pointer to prompt */
    WORD *pw;				/* pointer to destination word */
    {

    return (_getword(pp, '?', "%d", pw));
    }

/* gethword - input word (hex)
 */
BOOL gethword(pp, pw)
    TEXT *pp;				/* pointer to prompt */
    WORD *pw;
    {

    return (_getword(pp, '$', "%x", pw));
    }

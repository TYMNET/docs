/************************************************************************
 * aierror.c - Output Error For Application Test Program
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    aierror.c contains the source modules which output errors for the
 *    AI program.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KS    Initial Draft
 *
 ************************************************************************/
#include "stddef.h"

IMPORT INT printf();			/* returns output count */
IMPORT VOID exit();			/* program exit */

IMPORT TEXT *pname;			/* program name (from main) */

/* error - output error mesage and exit
 */
VOID error(p)
    TEXT *p;				/* pointer to error message */
    {
    
    (VOID)printf("%s: %s\n", pname, p);
    exit();
    }

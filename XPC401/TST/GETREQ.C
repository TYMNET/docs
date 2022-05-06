/************************************************************************
 * getreq.c - Allocate and Free Request Blocks
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    getreq.c contains the source modules which are called to allocate 
 *    and free request blocks.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KS    Initial Draft
 *
 ************************************************************************/
#include "stddef.h"
#include "ai.h"

IMPORT UBYTE *malloc();			/* memory alloc */
IMPORT VOID error();			/* fatal error routine */
IMPORT VOID free();			/* memory free */

/* ralloc - allocate a request block
 */
LOCAL UBYTE *ralloc(n)
    BYTES n;				/* request block size */
    {
    UBYTE *p;				/* pointer to alloc'ed memory */
    
    if ((p = malloc(n)) == 0)		/* if alloc fails */
	error("out of heap");		/* output message and exit */
    return (p);
    }

/* getreq - allocate and initialize a request block
 */
REQ *getreq()
    {
    REQ *pr;				/* request block pointer (returned) */
    WORD *pw;				/* scratch word pointer */

    /* allocate memory for request block and NPARAMS parameters
     */
    pr = (REQ *)ralloc((BYTES)(sizeof(REQ) + (NPARAMS * PARSIZ)));
    
    /* initialize each pointer in the request block strcuture to point to
     * a word in the newly alloc'ed area
     */
    pw = (WORD *)(pr + 1);
    pr->fid = pw++;
    pr->dev = pw++;
    pr->chnl = pw++;
    pr->scode = pw++;
    pr->par1 = pw++;
    pr->par2 = pw++;
    pr->par3 = pw++;
    pr->par4 = pw;
    return (pr);			/* return address of request block */
    }

/* freereq - free request block
 */
VOID freereq(pr)
    REQ *pr;				/* pointer to request block */
    {
    
    free((UBYTE *)pr);
    }

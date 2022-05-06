/************************************************************************
 * status.c - Call XPC Driver for Input Status and Output Status
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    status.c contains the source modules which are called to get
 *    the input status and output status from the XPC driver.
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

IMPORT REQ *call();			/* returns pointer to resuest block */

LOCAL REQ *pr = NULLREQ;		/* request block pointer */

/* instat - request input status
 */
WORD instat(chnl)
    INT chnl;				/* channel number */
    {
    
    pr = call(15, pr, chnl);		/* request io status */
    return (*pr->par1);			/* return input status */
    }

/* outstat - request output status
 */
WORD outstat(chnl)
    INT chnl;				/* channel number */
    {
    
    pr = call(15, pr, chnl);		/* request io status */
    return (*pr->par3);			/* return output status */
    }

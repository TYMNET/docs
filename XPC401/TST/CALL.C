/************************************************************************
 * call.c - Call XPC Driver
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    call.c contains the source modules which are called to call
 *    functions which set up application parameters for an XPC
 *    application function, call the xpc driver to execute application
 *    function and call function to process output from application
 *    function.
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
#include "appl.h"
#include "dos.h"
#define NERRS		(sizeof(err) / sizeof(err[0]))

IMPORT INT inchar();			/* input a character */
IMPORT INT int86x();			/* generates software interrupt */
IMPORT INT printf();			/* returns output count */
IMPORT REQ *getreq();			/* allocates request block */

IMPORT BOOL ttymode;			/* tty mode flag */
IMPORT INT vector;			/* interrupt vector */
IMPORT FUNC func[];			/* application function table */

/* error message text
 */
LOCAL TEXT *err[] = {"success",
		     "unrecognized function code",
		     "illegal device number",
		     "illegal channel number",
		     "illegal function in wait reset mode",
		     "illegal function in character mode",
		     "illegal function in packet mode",
		     "illegal port address",
		     "illegal baud rate",
		     "illegal parity parameter",
		     "illegal number of data bits",
		     "illegal number of stop bits",
		     "illegal DTE/DCE specification",
		     "port active, for reset with non-zero port",	
		     "write buffer bigger than 128 characters",
		     "write attempted while flow controlled",
		     "illegal write data type",
		     "illegal call request format",
		     "illegal call clearing cause code",
		     "illegal function in packet channel state",
		     "no channels available",
		     "illegal PAD parameter or value",
		     "packets channels open",
		     "illegal event code",
		     "timer already running",
		     "insufficient resources to start timer",
		     "checkpoint already running"};

/* _call - call xpc driver
 */
LOCAL VOID _call(pr)
    REQ *pr;				/* pointer to request block */
    {
    struct SREGS sregs;			/* see dos.h */
    union REGS regs;			/* see dos.h */

    /* initialize "registers" and call X.PX Driver through vector
     */
    segread(&sregs);
    pr->ds = sregs.es = sregs.ds;
    regs.x.bx = (UWORD)pr;
    (VOID)int86x(vector, &regs, &regs, &sregs);
    }

/* call - call application function
 */
REQ *call(fid, pr, chnl)
    WORD fid;				/* function index */
    REQ *pr;				/* pointer to request block */
    INT chnl;				/* channel number */
    {
    FUNC *pf;				/* scratch function pointer */


    if (!pr)				/* if no request block provided, */
	pr = getreq();			/* allocate one */
    FOREVER
	{
	/* initialize the request block
	*/
	*pr->par1 = -1;
	*pr->par2 = -1;
	*pr->par3 = -1;
	*pr->par4 = -1;
	*pr->fid = fid;
	*pr->dev = 1;
	*pr->chnl = chnl;
	*pr->scode = -1;
    
	/* set pf to the function indicated by fid (or 0 if invalid). note fid
	 * is assumed positive
	 */
	pf = &func[fid <= MAX_APPL_FUNC + 4 ? fid + 1 : 0];
    
	/* if no function is provided, or function input/initialization is
	* complete, call the driver
	*/
	if (!pf->func || (*pf->func)(pr, YES))
	    {
	    _call(pr);			/* call driver */
    
	    /* if an error occurred, or if the program is running in interactive
	     * (non-tty) mode, output function call status
	     */
	    if (*pr->scode || !ttymode)
		(VOID)printf("%s (%d): returned %d (%s)\n", pf->name, chnl, 
		    *pr->scode, *pr->scode < 0 || NERRS <= *pr->scode ?
			"unrecognized return value!" : err[*pr->scode]);
	
	    /* if a function was provided, call it again for output processing
	     */
	    if (pf->func)
		(*pf->func)(pr, NO);
	    if (fid == 13)
		if (inchar() == 0)
		    continue;
	    break;
	    }
	}
    return (pr);			/* guaranteed non-null */
    }

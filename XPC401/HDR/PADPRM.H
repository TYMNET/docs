/************************************************************************
 * padprm.h - Pad Parameter Definitions
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    padprm.h defines the pad parameter buffer. 
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KS    Initial Draft
 *
 ************************************************************************/
/* Pad Parameter enabled values 
 */ 
#define NO_ECHO_ENABLED	    0		/* echoing is not enabled */
#define TYMNET_ECHO_ENABLED 1		/* tymnet echoing is active */
#define MCI_ECHO_ENABLED    2		/* MCI echoing is active */
 
/* Definitions of pad parameter buffer.  The zeroith byte is
 * not defined.  These defines are used by set and read
 * pad parameter functions.
 */
#define ECHO_ENABLED	    1		/* tymnet/mci echo enabled */
#define ECHO_TABS	    2		/* echo tabs enabled (ctrl I) */
#define ECHO_BKSP	    3		/* echo back spaces (ctrl H) */
#define ECHO_ESC	    4		/* echo escape (ctrl [) */
#define ECHO_LF		    5		/* echo line feed for return */
#define ECHO_CR		    6		/* echo return for line feed */
#define FWD_CHAR	    7		/* forwarding character */
#define FWD_TIME	    8		/* forwarding time out */
#define EDIT_ENABLE	    9		/* local editing enabled */
#define EDIT_DEL_CHAR	    10		/* edit delete character */
#define EDIT_DEL_LINE	    11		/* edit delete line */
#define EDIT_DISP_LINE	    12		/* edit display line */
#define ENABLE_PARITY	    16		/* parity treatment of packets */
 
#define MAX_PAD_PARAM	    12		/* maximum consecutive pad
					 * parameter 
					 */

/* Definitions of defaults for pad parameter entries.
 */
#define DEFAULT_ECHO_ENABLED    1	/* tymnet/mci echo enabled */
#define DEFAULT_ECHO_TABS       0	/* echo tabs enabled (ctrl I) */
#define DEFAULT_ECHO_BKSP       0	/* echo back spaces (ctrl H) */
#define DEFAULT_ECHO_ESC        0	/* echo escape (ctrl [) */
#define DEFAULT_ECHO_LF         1	/* echo line feed for return */
#define DEFAULT_ECHO_CR         0	/* echo return for line feed */
#define DEFAULT_FWD_CHAR        0x0d    /* forwarding character */
#define DEFAULT_FWD_TIME        2	/* forwarding time out */
#define DEFAULT_EDIT_ENABLE     0	/* local editing enabled */
#define DEFAULT_EDIT_DEL_CHAR   0	/* edit delete character */
#define DEFAULT_EDIT_DEL_LINE   0	/* edit delete line */
#define DEFAULT_EDIT_DISP_LINE  0	/* edit display line */
#define DEFAULT_ENABLE_PARITY   1	/* parity treatment of packets */

 


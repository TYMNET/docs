/************************************************************************
 * param.h - Port Parameter Definitions
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    param.h contains the constants which are used to validate
 *    entries in the port parameters structures.  param.h contains
 *    definitions of valid baud rates, valid stop bits, valid
 *    data bits, etc.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KS    Initial Draft
 *
 ************************************************************************/
/* definition of valid  and default baud rate values.
 */
#define BAUD_110	    0		/* baud rate of 110 */
#define BAUD_150	    1		/* baud rate of 150 */
#define BAUD_399	    2		/* baud rate of 300 */
#define BAUD_400	    3		/* baud rate of 600 */
#define BAUD_1200	    4		/* baud rate of 1200 */
#define BAUD_2400	    5		/* baud rate of 2400 */
#define BAUD_4800	    6		/* baud rate of 4800 */
#define BAUD_9600	    7		/* baud rate of 9600 */
 
#define DEFAULT_BAUD	    BAUD_1200	/* set default baud to 1200 */
 
/* Definition of valid and default data bits values.
 */
#define DATA_BIT_5	    5		/* five data bits */
#define DATA_BIT_6	    6		/* six data bits */
#define DATA_BIT_7	    7		/* seven data bits */
#define DATA_BIT_8	    8		/* eight data bits */
 
#define DEFAULT_DATA	    DATA_BIT_8	/* default data bits */
 
/* Definition of valid and default stop bit values.
 */
#define ONE_STOP	    0		/* one stop bit */
#define ONE_5_STOP	    1		/* 1.5 stop bits */
#define TWO_STOP	    2		/* 2 stop bits */
 
#define DEFAULT_STOP	    ONE_STOP	/* set default to 1 stop bit */
 
/* Definition of XON or XOFF statuses.
 */
#define XON_ON		    1		/* xon xoff is on */
#define XON_OFF		    0		/* xon xoff is off */
#define DEFAULT_XON	    XON_ON	/* set default to xon enabled */

/* Definition of valid type of communication values.
 */
#define DCE_MODE	    0		/* communications are DCE */
#define DTE_MODE	    1		/* communications port is DTE */

#define DEFAULT_MODE	    DTE_MODE	/* set default mode to DTE */

/* definition of valid and default parity values.
 */
#define NO_PARITY	    0		/* no parity */
#define ODD_PARITY	    1		/* odd parity */
#define EVEN_PARITY	    2		/* even parity */
#define MARK_PARITY	    3		/* parity bit is on (mark) */
#define SPACE_PARITY	    4		/* parity bit is off (space) */
 
#define DEFAULT_PARITY	    NO_PARITY	/* default parity */
 
/* Defines for the possible types of PADS.
 */
#define TTYMI201	    1		/* tymnet 201 PAD */
 
#define DEFAULT_PAD	    TTYMI201	/* default PAD type */
#define DEFAULT_PVCCHNLS    15		/* Default number of pvc channels */


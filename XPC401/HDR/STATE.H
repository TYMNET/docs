/************************************************************************
 * state.h - XPC Application State, Channel State, Link State Defines
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    state.h contains the definitions which are used when
 *    processing the current application state (reset, character, packet),
 *    the current channel state (CS0 thru CS9), the current device
 *    state, the current restart state (R1 - R2), the current
 *    reset state (D1 - D1), the current DXE flow control state
 *    (F1 - F2), the current DTE flow control state (G1 - G2), etc.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KS    Initial Draft
 *
 ************************************************************************/
/* defines for device states 
 */
#define RESET_STATE	    1		/* device is in wait/reset mode */
#define CHAR_STATE	    2		/* device is in character state */
#define PKT_STATE	    3		/* device is in packet state */

#define MAX_DEV_STATE	    PKT_STATE	/* maximum state value */
 
/* defines for the channel states
 */
#define CHNL_DISCONNECTED   0		/* channel disconnected */
#define CHNL_PEND_CALL_ACP  1		/* pendding call accept */
#define CHNL_CALL_ACP	    2		/* call has been accepted */
#define CHNL_CONNECTED	    3		/* channel has been connected */
#define CHNL_PEND_CLEAR	    4		/* pending confirmation of clear */
#define CHNL_PEND_RECV_CALL 5		/* pending incomming call */
#define CHNL_RECV_CALL	    6		/* incoming call accepted */
#define CHNL_PEND_ACP	    7		/* pending acceptance */
#define CHNL_CALL_CLEARED   8		/* call cleared */
#define CHNL_RESET	    9		/* received or send reset */
 
/* defines for the packet states
 */
#define S1		    1		/* pending call */
#define S2		    2		/* DCE sent session request */
#define S3		    3		/* DTE sent session request */
#define S4		    4		/* transferring data */
#define S5		    5		/* DTE/DCE have collided */
#define S6		    6		/* DCE cleared session */
#define S7		    7		/* DTC cleared session */
 
/* defines for the types of tymnet echoing
 */
#define T0		    0		/* tymnet echo off */
#define T1		    1		/* no balls are out */
#define T2		    2		/* green ball is out */
#define T3		    3		/* red ball is out */
#define T4		    4		/* local echo is off */
#define T5		    5		/* local echo is on */
 
/* defines for the types of MCI echoing
 */
#define M0		    0		/* MCI echo off */
#define M1		    1		/* local echo is on */
#define M2		    2		/* local edit is on */


/* Reset states
 */
#define D1		    1		/* Flow Control Ready */
#define D2		    2 		/* DTE reset request */

/* defines for RESTART states in link
 */
#define R0		    0		/* DCE waiting for restart */
#define R1		    1		/*  packet level ready */
#define R2		    2		/* DTE Restart Request */

/* defines for DXE states in link
 */
#define F1		    1		/* DXE Receive ready */
#define F2		    2		/* DXE Receive not ready */

/*defines for DTE states in link
 */
#define G1		    1		/* DTE Receive Ready */
#define G2		    2		/* DTE Receivce Not Ready */
#define G3		    3		/* DTE Receive Ready was detected,
					 * next time plink is called, state
					 * goes to G1
					 */



/* defines of states for I/O status
 */
#define INPUT_NORMAL 	    (WORD)0	/* input of data has not
					 * break or error.
					 */
#define ERROR_BIT	    (WORD)1     /* error was encountered */
#define BREAK_BIT	    (WORD)2	/* break was received */
#define NO_OUTPUT	    (WORD)0	/* No output in queues */
#define NO_FLOWCTRL_OUTPUT  (WORD)1	/* Output is not flow controlled */
#define FLOWCTRL_OUTPUT     (WORD)2	/* Output is flow controlled */


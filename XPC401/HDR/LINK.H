/************************************************************************
 * link.h - Link Definitions
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    link.h contains the defines for the functions which
 *    are processed by link and the counters which are used
 *    for the link timers.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KS    Initial Draft
 *
 ************************************************************************/
/* Defines of control packet functions.  A control packet has bit 80 set
 * in the GFI_LCI byte of the packet.  These functions are processed
 * by link.
 */
#define RESTART		(UBYTE)0xFB	/* restart packet */
#define RESTART_CONFIRM (UBYTE)0xFF	/* restart confirm packet */
#define RESET		(UBYTE)0x1B	/* reset system */
#define RESET_CONFIRM 	(UBYTE)0x1F	/* confirmation of reset */
#define RR		(UBYTE)0x01	/* RR packet */
#define RNR		(UBYTE)0x05	/* RNR packet */
#define REJECT 		(UBYTE)0x09	/* REJECT packet */
#define DIAG		(UBYTE)0xF1	/* diagnostic packet */

/* Defines which are used to extract the P(R), P(S) and the channel
 * number out of the incoming packet.
 */

#define EXTRACT_SENDSEQ (UBYTE)0x0f	/* and to get P(s) */
#define EXTRACT_ACKSEQ  (UBYTE)0xf0	/* and to get P(r) */
#define EXTRACT_CHNL 	(UBYTE)0x0f	/* extract channel number */


/* link diagnostic codes.
 */
#define DIAG0		(UBYTE)0	/* diagnostic 0 */
#define DIAG1		(UBYTE)1	/* invalid P(S) */
#define DIAG2		(UBYTE)2	/* RR or REJECT with channel
					 * number 0 and invalid
					 * P(R)
					 */
#define DIAG17 		(UBYTE)17	/* Restart confirmation */
#define DIAG27		(UBYTE)27	/* confirmation of reset
					 * confirm.
					 */
#define DIAG33  	(UBYTE)33	/* Invalid packet type */
#define DIAG36  	(UBYTE)36	/* Invalid packet type for
					 * channel 0
					 */
#define DIAG38		(UBYTE)38	/* Cause missing for restart
					 * reset packet.
					 */
#define DIAG39		(UBYTE)39	/* length or diagnostic
					 * for restart/restart confirm
					 * reset/reset confirm is not valid
					 */
#define DIAG40  	(UBYTE)40	/* M or D bit is set */
#define DIAG41  	(UBYTE)41	/* restart with non-zero
					 * channel number
					 */
#define DIAG52  	(UBYTE)52	/* Restart being sent because
					 * of timeout
					 */
#define DIAG81 		(UBYTE)81	/* invalid cause for reset
					 * or restart
					 */


/* Defines of window parameters.
 */
#define WINDOW_SIZE 8			/* size of the window */
#define WINDOW_DATA (WINDOW_SIZE / 2)	/* number of data packets allowed in
					 * the window.
					 */

/* Return codes which are used by link.
 */

#define QUEUE_NOT_FULL		0	/* queue is not full */
#define QUEUE_IS_FULL		1	/* queue is full -- can't add any more
					 * packets to iocomm.
					 */
#define SEQNUM_IN_ERROR		2	/* the output packet seqnuence number
					 * is in error.
					 */


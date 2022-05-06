/************************************************************************
 * error.h - Error Definitions
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    error.h contains the defines for all of the errors detected by
 *    the XPC Driver.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KS    Initial Draft
 *
 ************************************************************************/
/* xpc drive interface status code summary
 */
#define ILLEGAL_FUNC		1	/* invalid function */
#define ILLEGAL_DEVICE		2	/* illegal device number */
#define ILLEGAL_CHNL		3	/* illegal channel number */
#define ILLEGAL_WS_FUNC		4	/* illegal wait reset function */
#define ILLEGAL_CHAR_FUNC	5	/* illegal character function */
#define ILLEGAL_PKT_FUNC	6	/* illegal packet function */
#define ILLEGAL_PORT		7	/* illegal port address */
#define ILLEGAL_BAUD_RATE	8	/* illegal baud rate */
#define ILLEGAL_PARITY		9	/* illegal parity parameter */
#define ILLEGAL_NO_DATA_BITS	10	/* illegal number data bits */
#define ILLEGAL_NO_STOP_BITS	11	/* illegal number stop bits */
#define ILLEGAL_DTE_SPEC	12	/* illegal DTE,DCE specification */
#define PORT_ALREADY_ACTIVE	13	/* port already active for reset
					 * command with non-zero port
					 */
#define BUFFER_OVERFLOW		14	/* write buffer too large */
#define ILLEGAL_FLOW_STATE	15	/* illegal flow state */
#define ILLEGAL_DATA_TYPE	16	/* illegal write data type */
#define ILLEGAL_CALL_REQ	17	/* illegal call request format */
#define ILLEGAL_CLEAR_CODE	18	/* illegal clear cause code */
#define ILLEGAL_CHNL_FUNC	19	/* function illegal for chnl state */
#define NO_CHNLS_AVAILABLE      20	/* all channels in use */
#define ILLEGAL_PAD_PARAM       21	/* illegal packet parameter */
#define PACKET_CHNL_OPEN        22	/* unable to set character mode while
					 * packet channels are open
					 */
#define ILLEGAL_EVENT_CODE	23	/* illegal event code */
#define TIMER_REQ_IGNORED	24	/* timer is already running */
#define UNABLE_START_TIMER	25	/* unable to start timer */
#define CHECKPOINT_ACTIVE       26	/* checkpoint is already running */


/* The following defines are for internal programming errors.
 */
#define PRGM_ERROR		1000	/* programming error */
#define NO_BUFFER_AVAIL		1001	/* no buffers available */
#define QUEUE_FULL		1002	/* queue full */
#define C_READ_ERROR		1003	/* character count does not match
					 * queue data count
					 */
#define P_READ_ERROR		1004	/* character count does not match
					 * queue data count
					 */
#define RDCHNL_ERROR		1005	/* read channel error */
#define WRONG_DEVICE_STATE	1006	/* device is in invalid state */
#define FREE_COUNT_ERROR	1007	/* no buflets available when free
					 * count greater than zero.
					 */
#define LINK_CHNL_ERROR		1008	/* number of pending incoming calls
					 * is greater than zero, but no
					 * channels available.
					 */
#define PS_SEQNUM_ERROR		1009	/* p(s) not correct */
#define XMIT_LINK_ERROR		1010	/* ran past the end of link
					 * assembly packet in link xmit
					 * packet
					 */
#define PS_DUPLICATE		1020	/* duplicate packet */
#define STOP_WRITE		1021	/* stop writing data (no buffer or
					 * assembly packet is full).
					 */
#define INPUT_QUEUE_FULL	8000	/* appplication read queue is full */

#define INVALID_PARAMETER	9000	/* invalid parameter entry */

/************************************************************************
 * appl.h - Driver Interface Defines
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    appl.h contains the defines which are used to process
 *    driver interface packets and the definitions of 
 *    driver interface functions.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KS    Initial Draft
 *
 ************************************************************************/
/* defines used to move data from the parameter packet in move_param.
 */
#define TO_APPL		   0		/* into application parameter */
#define FROM_APPL	   1		/* from application parameter */

#define PARAM_1		   1		/* first parameter in packet */
#define PARAM_2		   2		/* second parameter in packet */
#define PARAM_3		   3		/* third parameter in packet */
#define PARAM_4		   4		/* fourth parameter in packet */
 
#define TYMNET_STRING	   0		/* format of accept/request data */
 
/* The following defines the functions provided for the application process.
 */
#define CLEAR_DEVICE	   (UBYTE)0	/* clear device */
#define READ_DEVICE	   (UBYTE)1	/* read device status */
#define DEVICE_RESET	   (UBYTE)2	/* reset the current device */
#define SET_CHAR_STATE	   (UBYTE)3	/* set state to character */
#define SET_PKT_STATE	   (UBYTE)4	/* set state to packet state */
#define SET_PORT_PARAMS	   (UBYTE)5	/* set port parameters */
#define READ_PORT_PARAMS   (UBYTE)6	/* read port parameters */
#define SET_DTR		   (UBYTE)7	/* set data terminal ready */
#define CLEAR_DTR	   (UBYTE)8	/* clear data terminal ready */
#define SET_RTS		   (UBYTE)9	/* set request to send */
#define CLEAR_RTS	   (UBYTE)10	/* clear request to send */
#define TEST_DSR	   (UBYTE)11	/* test for dataset ready */
#define TEST_DCD	   (UBYTE)12	/* test static carrier detect */
#define TEST_RI		   (UBYTE)13	/* test ring indicator */
#define SEND_BREAK	   (UBYTE)14	/* send break */
#define IO_STATUS	   (UBYTE)15	/* report input/output status */
#define INPUT_DATA	   (UBYTE)16	/* input data */
#define OUTPUT_DATA	   (UBYTE)17	/* output data */

/* The following functions (18-23) are only available in packet mode; the
 * processing of the function is dependent upon the state of the packet
 * virtual channel.
 */
#define READ_CHNL_STATUS   (UBYTE)18	/* read session channel status */
#define SEND_SSN_REQ	   (UBYTE)19	/* intiate start of session */
#define SEND_SSN_CLEAR	   (UBYTE)20	/* terminate  session */
#define SSN_ANSWER_REQ	   (UBYTE)21	/* wait for session request */
#define SEND_SSN_ACCEPT	   (UBYTE)22	/* send acknowledge for session req */
#define READ_SSN_DATA	   (UBYTE)23	/* read data from received session
					 * request or session accept packet
					 */
#define SET_INTERRUPT	   (UBYTE)24	/* set interrupt */
#define CLEAR_INTERRUPT	   (UBYTE)25	/* clear interrupt */
#define SET_UPDATE	   (UBYTE)26	/* set update */
#define CLEAR_UDPATE	   (UBYTE)27	/* clear udpate */
#define REPORT_PAD_PARAMS  (UBYTE)28	/* report PAD parameters */
#define SET_PAD_PARAMS	   (UBYTE)29	/* set PAD parameters */
#define FLUSH_INPUT	   (UBYTE)30	/* flush the input buffer */
#define FLUSH_OUTPUT	   (UBYTE)31	/* flush the output buffer */
#define LINK_STATS	   (UBYTE)32	/* link statistics */

#define MAX_APPL_FUNC	   LINK_STATS	/* maximum application function */
#define START_SSN_FUNC	   READ_CHNL_STATUS /* starting virtual session */
#define NUM_SSN_FUNC	   (READ_SSN_DATA - START_SSN_FUNC) /* # of functions
							     */

/* define miscellaneous sizes, tuning parameters, etc.
 */ 
#define ARQ_HIWATER	128		/* application read queue hiwater */
#define BUF_LOWATER	CWD_BUFLETS	/* buflet free list count lowater */
#define CWD_BUFLETS	(BYTES)bufreq(CWD_MAX_DATA) /* buflets required by
						     * c_write_data
						     */
#define CWD_HIWATER	127		/* write data hiwater */
#define CWD_MAX_DATA	256		/* character write data maximum */
#define CRB_HIWATER	384		/* circular read buffer hiwater */
#define CRB_LOWATER	128		/* circular read buffer lowater */
#define CRB_MAX_READ	255		/* maximum read from circular buffer */
#define MAX_RDQUEUE_CHARS   512		/* maximum read_queue characters */
#define MAX_ECHO_CHARS  (MAX_RDQUEUE_CHARS + 128)
					/* maximum number of characters in
					 * read queue when echoing is being
					 * done.
					 */


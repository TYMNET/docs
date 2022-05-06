/************************************************************************
 * pkt.h - PAD Packet Definitions
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    pkt.h contains the defines used to process and 
 *    build pad packets and the definitions of PAD Packet functions.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KS    Initial Draft
 *
 ************************************************************************/


/* Definition of a packet frame.  The fourth byte (PKT_TYP_ID or
 * FIRST_DATA_BYTE) has a duplication meaning.  If the packet is
 * not a data packet, then  the fourth byte contains the first byte
 * of data.  Otherwise, the fourth byte is the type of packet.
 * When packets are read the first byte of data is moved to
 * byte 6 (previously used for the second byte of the CRC).
 * This makes the moving of input data easier.  If there is
 * more than one byte of input data or the packet type has
 * extra data, the extra data starts at position EXTRA_DATA
 * and there is a second CRC.
 */

#define STX		    0		/* STX (0x02 or control B) */
#define FRAME_LEN	    1		/* length of data after CRC1 */
#define GFI_LCI		    2		/* Gen. Format ID, Logical Chnl ID */
#define SEQ_NUM		    3		/* P(R), P(S) sequence numbers */
#define PKT_TYP_ID	    4		/* packet type identifier */
#define FIRST_DATA_BYTE	    4		/* first data byte */
#define CRC1		    5		/* CRC for bytes 0 thru 4 */
#define MOVED_FIRST_BYTE    6		/* position where first data byte is
					 * stored by read 
					 */
#define EXTRA_DATA	    7		/* start of extra data (followed by
					 * two bytes CRC)
					 */

#define QPKT_DATA_SIZ	    (DATA_BUF_SIZ - EXTRA_DATA) /* packet data size */
#define PKT_DATA_SIZ	    (QPKT_DATA_SIZ + 1) /* number of bytes of data */
 
#define IO_DATA_LEN	    63		/* When more than 63 characters are
					 * in the output packet, link 
					 * the output packet.
					 */
#define MAX_DATA_PKT	    128		/* maximum data for transmit packet */

#define QBIT		    0x20	/* Q bit, if this bit is set
					 * then packet is not a data
					 * packet and the type of packet
					 * is a pad function.
					 */
#define MBIT		    0x10	/* M BIT, more data before
					 * acknowledge. (unused) 
					 */
#define DBIT 		    0x40	/* D BIT, destination bit 
					 * (unused)
					 */ 

#define CBIT 		    0x80	/* C bit, if this bit is set
					 * then packet is not a data
					 * packet and the type of packet
					 * is a link function.
					 */

#define STX_CHAR	    0x02	/* STX character */
 
/* Definitions which are used to define a buflet when there is 
 * character data.
 */
#define CH_LEN		    0		/* length of character data */
#define CH_DATA		    1		/* start of data in first buflet */
 
#define MAX_IQUEUE_CHAR	    100		/* maximum number of characters in
					 * input queue
					 */
#define MAX_OUTPUT_PKTS	    4		/* maximum number of control packets */
#define SSN_DATA_LEN	    127		/* maximum number of session data
					 * bytes
					 */
 
/* Definitions of packets which are processed by the PAD.  
 */
#define ENABLE_ECHO	    (UBYTE)1	/* Enable Echo Control */
#define DISABLE_ECHO	    (UBYTE)2	/* Disable Echo Control */
#define ENTER_DEFER_ECHO    (UBYTE)3	/* Enter Deferred Echo Mode */
#define EXIT_DEFER_ECHO	    (UBYTE)4	/* leave Deferred Echo Mode */
#define GREEN_BALL	    (UBYTE)5	/* Receive Green Ball */
#define RED_BALL	    (UBYTE)6	/* Receive Red Ball */
#define BEGIN_BRK	    (UBYTE)7	/* Begin Break */
#define END_BRK		    (UBYTE)8	/* End Break */
#define SESSION_REQ	    (UBYTE)9	/* Session Request */
#define SESSION_ACP	    (UBYTE)10	/* Session Accept */
#define SESSION_CLR	    (UBYTE)11	/* Session Clear */
#define SSN_CLR_ACP	    (UBYTE)12	/* Session Clear Accept */
#define YELLOW_BALL	    (UBYTE)13	/* Yellow Ball */
#define ORANGE_BALL	    (UBYTE)14	/* Orange Ball */
#define ENABLE_PERM_ECHO    (UBYTE)15	/* Enable Permanent Echo */
#define DISABLE_PERM_ECHO   (UBYTE)16	/* Disable Permanent Echo */
#define SET_FWD_CHAR	    (UBYTE)17	/* Set Forward Character */
#define SET_FWD_TIMEOUT	    (UBYTE)18	/* Set Forward Time out */
#define ENABLE_LOCAL_EDIT   (UBYTE)19	/* Enable Local Edit */
#define DISABLE_LOCAL_EDIT  (UBYTE)20	/* Disable Local Edit */

#define MAX_PKT_FUNC	    DISABLE_LOCAL_EDIT /* Maximum # of Functions */
 
#define START_PSSN_FUNC	    SESSION_REQ
#define NUM_PSSN_FUNC	    (SSN_CLR_ACP - SESSION_REQ)

/* Definitions for valid clear codes.
 */
#define NORMAL_CALL_END	    	(BYTE)0		/* normal end of call */
#define NO_PORTS	    	(BYTE)1		/* no ports available */
#define HOST_UNAVAIL	    	(BYTE)2		/* host is not available */
#define ACCESS_DENIED	    	(BYTE)3		/* */
#define NETWORK_UNAVAIL	    	(BYTE)4		/* network not available */
#define FORMAT_ERROR	    	(BYTE)5		/* */
#define USERNAME_ERROR	    	(BYTE)6		/* */
#define ADDRESS_ERROR	    	(BYTE)7		/* */
#define PASSWORD_ERROR	    	(BYTE)8		/* */
#define CONFIRM_TIMEOUT	    	(BYTE)9		/* pending clear confirm
				         	 * timed out.
					         */
#define RESET_PACKET	    	(BYTE)10	/* channel being reset */
#define PACKET_LEVEL_RESTART	(BYTE)11	/* communications being
				 		 * restarted.
						 */
#define RESTART_END	    	(BYTE)12	/* restart ending */
#define REMOTE_XPC_LOST	    	(BYTE)13	/* */
#define MODEM_STATUS_LOST  	(BYTE)14	/* */
#define INVALID_PAD_SIGNAL	(BYTE)15	/* Invalid pad signal */



/* define macros
 */
#define calc_num_buflets(c) \
    (((c) + DATA_BUF_SIZ - 1 + EXTRA_DATA + 2) / DATA_BUF_SIZ)



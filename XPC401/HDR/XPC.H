/************************************************************************
 * xpc.h - XPC Driver Definitions and Strucutures
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    xpc.h contains the definitions and structures which are used
 *    throughout the XPC Driver.  The buflet structure definitions,
 *    and queue structure definitions are contained in xpc.h.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KS    Initial Draft
 *
 ************************************************************************/
/* define channel parameters
 */
#define CM_CHNL		    0		/* character mode channel */
#define NUM_CHNL	    16		/* number of channels supported */
#define MAX_CHNL	    (NUM_CHNL - 1) /* maximum channel number */

/* Defines of XPC devstat entries.
 */
#define XPC_VERSION	    4		/* XPC version */
#define XPC_REVISION	    0		/* XPC revision number */

/* Defines of types of links.
 */
#define NO_LINK		    (UWORD)0	/* link type undefined */
#define CHAR_LINK	    (UWORD)1	/* character mode link */
#define PKT_LINK	    (UWORD)2	/* packet mode link */

/* define miscellaneous (sizes, etc.)
 */
#define BUFLET_SIZ	    sizeof(BUFLET) /* data buffer size (bytes) */
#define DATA_BUF_SIZ	    50		/* data buffer size (bytes) */
#define MAX_WRITE_BYTES	    512		/* maximum number of bytes in output
					 * queue when in character state
					 */
					
#define MIN_WRTBUFLETS       12		/* minimum number of buflets when
				   	 * writing data.
					 */					
#define MIN_REQUIRED_BUFLETS 25		/* minimum number of buflets to
					 * create assembly and certain
					 * other packets.
					 */
#define RNR_CHNL_BUFLETS     18	        /* number of buflets needed for
					 * a window of link data packets.
					 */
#define BUFLET_HIWATER	     (2 * RNR_CHNL_BUFLETS)  
					/* number of buflet high water */

/*
 * The following define is added to RNR_CHNL_BUFLETS * number of attached
 * channels to determine the low water mark to send RNR.  It is our
 * safety factor.  If it changes any higher, you might want to also
 * change MIN_REQUIRED_BUFLETS up a similar amount.
 */
#define BUFLET_LOWATER	     10		/* number of buflets too low */

#define NULLBUF		    (BUFLET *)0	/* null buflet pointer */
#define NULLBYTE	    (BYTE)0	/* null byte */
#define NULLFUNC	    (VOID (*)())0 /* null pointer to void function */
#define NULLQUEUE	    (QUEUE *)0	/* null queue pointer */
#define NULLUBYTE	    (UBYTE)0	/* null unsigned byte */
#define NULLUWORD	    (UWORD)0	/* null unsigned word */
#define NULLWORD	    (WORD)0	/* null word */

#define NUM_PAD_PARAM	    17		/* number of pad parameters */

/* The following is the X.PC timer stack length.  If it must change, then
 * XPCSTACKLEN in timeint.asm must also change to match it.
 */
#define XPC_STACK_LEN	    1000	/* xpc internal stack, in bytes */

/* define the BUFLET structure
 */
typedef struct buflet 
    {
    struct buflet *bufnext;		/* address of next buflet */
    struct buflet *chainnext;		/* address of next chain */
    UBYTE bufdata[DATA_BUF_SIZ];	/* data buffer */
    } BUFLET;

/* define the QUEUE structure
 */
typedef struct
    {
    BUFLET *begqueue;			/* beginning buflet chain in queue */
    BUFLET *endqueue;			/* last buflet chain in queue */
    UWORD nchains;			/* number of buflet chains in queue */
    } QUEUE;
 
/* defines used in queue management
 */
#define MUST_LINK     (BOOL)0		/* must link to queue */
#define CHECK_LINK    (BOOL)1		/* check if room in queue before
					 * linking packet
					 */


/* Logical Channel information structure (Link level)
 */
typedef struct		/* LCIINFO logical channel information structure */
    {
    BYTE appchnl;			/* Application/PAD channel number */
    UBYTE ssnstate;			/* PAD Session State (s1-s7) */
    QUEUE inpktqueue;			/* Incoming packet queue */
    QUEUE outpktqueue;			/* Outgoing packet queue */
    QUEUE waitackqueue;			/* Packets waiting to be acknowledged
					 * queue 
					 */
    QUEUE linkoutqueue;			/* link output queue */
    UBYTE resetstate;			/* Reset state (d1-d3) */
    UWORD resetcode;			/* code for reset */
    BOOL resetreceived;			/* Set when Reset received or sent,
					 * cleared by the PAD when read
					 */
    WORD ssncleartime;			/* Session Clear Time */
    BYTE ssnclearcode;			/* Session Clear Code */
    BYTE dxeflowstate;			/* dxe flow state */
    BYTE dteflowstate;			/* dte flow state */
    BYTE restartstate;			/* restart state */
    BYTE flowstate;			/* Flow control states
					 * (to be defined?) 
					 */
    UBYTE r20trans;			/* number of times to retry the 
					 * restart timer - for channel 0
					 * only.
					 */
    UBYTE r22trans;			/* Number of times to retry
					 * the reset timer - for channels
					 * 1-15 only.
					 */
    UBYTE r25trans;			/* Number of times to retry the
					 * window rotation timer.
					 */
    UBYTE r27trans;			/* Number of times to retry the
					 * reject packet timer.
					 */
    
    BOOL pktrejected;			/* packet rejected */
    BOOL rnrtransmitted;		/* RNR transmitted */
    BOOL retransmitdata;		/* retransmit data */
    
    UBYTE inwindlow;			/* low bound for input window */
    UBYTE inwindhigh;			/* high bound for input window */
    UBYTE inpktrecvseq;			/* last P(R) received  */
    UBYTE inpktsendseq;			/* last P(S) received */
    UBYTE indcount;			/* number of data packets allowed 
					 * for link input.
					 */
    
    UBYTE outwindlow;			/* low bound for output window */
    UBYTE outwindhigh;			/* high bound for output window */
    UBYTE outpktrecvseq;		/* next P(R) to send */
    UBYTE outpktsendseq;		/* next P(S) to send */
    UBYTE outdcount;			/* number of data packets allowed
					 * for link output.
					 */
    } LCIINFO;
    
/* Channel information structure (Application/PAD level)
 */
typedef struct		/* CHNLINFO Channel Information Structure */
    {
    UBYTE chnlstate;			/* Application Channel State */
    UBYTE logicalchnl;			/* Corresponding Logical Channel 
					 * Indicator (LCI) 
					 */
    LCIINFO *lcistruct;			/* Pointer to the corresponding 
					 * LCI info struct 
					 */
    UBYTE tymechostate;			/* Tymnet Echo Mode State (t1-t5) */
    UBYTE mciechostate;			/* MCI Echo Mode State (m1-m2) */
    
    QUEUE readqueue;			/* Application Read Queue */
    UWORD nreadbytes;			/* # of data bytes in Application
					 * Read Queue 
					 */
    UWORD idxreadqueue;			/* Index to the first unread data
					 * byte in the buflet pointed to
					 * by readqueue
					 */
    BUFLET *ssndatapkt;			/* Already processed Session Request/
					 * Accept packet for Read Session Data
					 * requests
					 */
    UWORD idxssndata;			/* index into session data */

    BYTE clearcode;			/* Session cleared code */
    
    BUFLET *assemblypkt;		/* Packet being assembled for transmit,
					 * if any
					 */
    UWORD nwritebytes;  		/* In Character state - number of
					 * bytes in output data queue.
					 * In packet state - number of
					 * data bytes in assembly packet.
					 */
    BOOL holdassembly;			/* can't link transmit assembly packet
					 * (flow control)
					 */
    BUFLET *echopkt;			/* Pointer to echo packet */
    UWORD idxechodata;			/* Number of data bytes in echopkt */

    BYTE timerevent;			/* timer event status */
    UWORD timereventseg;		/* timer event segment */
    UWORD timereventoff;		/* timer event offset */
    BYTE checkevent;			/* checkpoint event status */
    UWORD checkeventseg;		/* checkpoint event segment */
    UWORD checkeventoff;		/* checkpoint event offset */
    
    BYTE errorstatus;			/* Status of input */
    BYTE breakstatus;			/* Begin/End Break status */
    WORD padparams[NUM_PAD_PARAM];	/* PAD Parameters */
    } CHNLINFO;

/* bufreq - determine number of buflets required to hold n bytes
 */
#define bufreq(n)	    ((((n) - 1) / DATA_BUF_SIZ) + 1)

/* miscellaneous ascii character definitions
 */
#define BACKSPACE	    '\b'	/* backspace */
#define CR		    '\r'	/* carriage return */
#define ESCAPE		    '\033'	/* escape */
#define LF		    '\n'	/* line feed */
#define TAB		    '\t'	/* tab */
#define XOFF		    '\023'	/* xoff character */
#define XON		    '\021'	/* xon character */


/************************************************************************
 * iocomm.h - I/O Communications defines and structures
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    iocomm.h contains the defines which are used communications.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    SAB   Initial Draft
 *
 ************************************************************************/

#define XMT_BUF_CHAINS	    3		/* # of chains avail for xmit
					 * WARNING! - If this changes, then
					 * commasm.h must also change.  See
					 * the COMMINFO structure below for
					 * details.
					 */
#define SIZ_RECV_BUF	    512		/* Size of circular receive buffer.
					 * At 9600 baud, this is about 1/2
					 * second.  Hopefully, Link will never
					 * fall quite that far behind.
					 */

#define MAX_XMT_INTCNT      10		/* Maximum number of timer ticks
					 * which can pass without processing
					 * a transmit interrupt before the
					 * regular call to enable_xmt() MUST
					 * "kick" the interrupt mechanism.
					 */

/* Indices into the link statistics array
 * (NOTE - by "last read" it is referring to the last time these statistics
 * were read by the Application.  This read statistics clears out the
 * entries.)
 */
#define NBR_CRC_ERRS	    0		/* # of CRC errors since last read */
#define NBR_FRAMING	    1		/* # framing errs since last read */
#define NBR_OVERRUN	    2		/* # overrun errs since last read */
#define NBR_PARITY	    3		/* # parity errs since last read */
#define PKTS_SENT	    4		/* # packets sent since last read */
#define PKTS_RECVD	    5		/* # packets recvd since last read */
#define NBR_BREAK	    6		/* # breaks --- ditto --- */

#define NBR_LINKSTATS	    7		/* # of link statistics total
					 * WARNING! - If this changes, then
					 * commasm.h must also change.  See
					 * the COMMINFO structure below for
					 * details.
					 */

/* Errors returnable by the IOCOMM level
 */
#define COMM_ALREADY_ENABLED	-1500
#define XMT_ALREADY_ACTIVE	-1501
#define COMM_NOT_ENABLED	-1502

/* IO port base addresses & vectors
 */
#define PIC0		    0x20	/* Programmable Interrupt Controller
					 * (8259) Register 0 port address
					 */
#define PIC1		    0x21	/* Programmable Interrupt Controller
					 * (8259) Register 1 port address
					 */
#define COM1_BASEADDR	    0x3f8	/* Comm port #1 base address */
#define COM1_INTVEC	    0x0c	/* Comm port #1 interrupt vector */
#define COM1_INTDISABLE	    0x10	/* Bit (IRQ4) in PIC1 for enable &
					 * disable of COM1 interrupts
					 */
#define COM2_BASEADDR	    0x2f8	/* Comm port #2 base address */
#define COM2_INTVEC	    0x0b	/* Comm port #2 interrupt vector */
#define COM2_INTDISABLE	    0x08	/* Bit (IRQ3) in PIC1 for enable &
					 * disable of COM2 interrupts
					 */

/* Port offsets into the COM1/COM2 8250 chips.
 */
#define DATAIO_PORT	    0		/* Port offset for I/O data reg. */
#define IER_PORT	    1		/* Port offset for the Interrupt
					 * Enable Register (IER)
					 */
#define IIR_PORT	    2		/* Port offset for the Interrupt
					 * Identification Register (IIR)
					 */
#define LCR_PORT	    3		/* Port offset for Line Ctrl Reg */
#define MCR_PORT	    4		/* Modem Control Reg. */
#define LSR_PORT	    5		/* Line Status Reg. */
#define MSR_PORT	    6		/* Modem Status Reg. */

#define DIV_PORT_LSB	    0		/* Divisor Latch LSB  (when DLAB=1) */
#define DIV_PORT_MSB	    1		/* Divisor Latch MSB  (when DLAB=1) */

/* Bit masks for control bits in Interrupt Enable Register (IER)
 */
#define IER_RECVDATA	    1		/* enable receive data avail int. */
#define IER_THRE	    2		/* Enable transmit holding reg.
					 * empty (THRE) interrupt.
					 */
#define IER_LINESTATUS	    4		/* enable line status changed int. */
#define IER_MDMSTATUS	    8		/* enable modem status changed int. */

/* Bit masks for bits in Interrupt Identification Register (IIR)
 */
#define IIR_NO_INT	    1		/* No interrupts pending if set */
#define IIR_INT_ID	    6		/* Mask to get interrupt ID */

/* Interrupt ID's
 */
#define IIR_MDMSTATUS	    0		/* Modem status changed interrupt */
#define IIR_THRE	    1		/* Transmit Holding Register empty */
#define IIR_RECVDATA	    2		/* Received data available */
#define IIR_LINESTATUS	    3		/* Line Status Error occurred */

/* Line Control Register bit masks
 */
#define LCR_WORDLEN	    3		/* Word length Select bits */
#define LCR_STOPBITS	    4		/* Number of stop bits */
#define LCR_PARITY	    8		/* Parity enable */
#define LCR_EVENPAR	    16		/* Even Parity Select */
#define LCR_STICKPAR	    32		/* Stick Parity */
#define LCR_BREAK	    64		/* Set Break */
#define LCR_DLAB	    128		/* Divisor Latch Access Bit */

/* Modem Control Register bit masks
 */
#define MCR_DTR		    1		/* Data Terminal Ready */
#define MCR_RTS		    2		/* Request to Send */
#define MCR_OUT1	    4		/* OUT1 Control bit */
#define MCR_OUT2	    8		/* OUT2 Control bit */
#define MCR_LOOPBACK	    16		/* Loopback bit */

/* Line Status Register bit masks
 */
#define LSR_DATARDY	    1		/* Receiver Data Ready */
#define LSR_OVERRUN	    2		/* Overrun error received */
#define LSR_PARITY	    4		/* Parity error received */
#define LSR_FRAMING	    8		/* Framing error received */
#define LSR_BREAK	    16		/* Break received */
#define LSR_THRE	    32		/* Transmitter Holding Reg. Empty */
#define LSR_TSRE	    64		/* Transmitter Shift Reg. Empty */

/* Modem Status Register bit masks
 */
#define MSR_DCTS	    1		/* Delta Clear to Send */
#define MSR_DDSR	    2		/* Delta Data Set Ready */
#define MSR_TERI	    4		/* Trailing Edge Ring Indicator */
#define MSR_DRLSD	    8		/* Delta Rx Line Signal Detect */
#define MSR_CTS		    16		/* Clear to Send */
#define MSR_DSR		    32		/* Data Set Ready */
#define MSR_RI		    64		/* Ring Indicator */
#define MSR_RSLD	    128		/* Receive Line Signal Detect
					 * (aka Data Carrier Detect (DCD))
					 */

/* Host pacing states
 */
#define NO_PACE_OUT	    0
#define SEND_STARTPACE	    1
#define SENT_STARTPACE	    2
#define SEND_ENDPACE	    3


/* Comm information structure
 * (The numbers in brackets are the byte offsets from the beginning of
 * the structure.  If these change, then the corresponding defines in
 * commasm.h must also be changed.  This means that if XMT_BUF_CHAINS
 * or NBR_LINKSTATS changes, commasm.h must also change.  If anything
 * new is added, same thing applies.)
 */
typedef struct		/* COMMINFO  Comm Information Structure */
    {
    BUFLET *xmtptr[XMT_BUF_CHAINS + 1];	/* Pointers to chains to be
					 * transmitted. [0]
					 */
    UWORD xmtlen[XMT_BUF_CHAINS + 1];	/* Lengths of above chains [8] */
    BOOL xmtflg[XMT_BUF_CHAINS + 1];	/* Pkt Mode flag to free chain? [16] */
    BOOL xmtactive;			/* is transmit transmitting? [24] */
    BOOL paceactive;			/* xon/xoff pacing active? [26] */
    BOOL xoffrecvd;			/* State of PC pacing [28] */
    WORD xmtxoff;			/* State of host pacing [30] */
    
    UBYTE *begrecvbuf;			/* Start of circular rec buffer [32] */
    UBYTE *endrecvbuf;			/* End of circ recv buf (CRB) [34] */
    UBYTE *wrtrecvbuf;			/* Write ptr into the CRB [36] */
    UBYTE *readrecvbuf;			/* Read pointer into the CRB [38] */
    BOOL commenabled;			/* Receive/Status int enabled [40] */
    
    UBYTE mdmstatus;			/* Modem Status Register [42] */
    UBYTE linestatus;			/* Line Status Register [43] */
    
    UWORD linkstats[NBR_LINKSTATS];	/* Link statistics array [44] */
    
    BOOL breakrecvd;			/* Physical break received [58] */
    BOOL errorrecvd;			/* Physical line error received [60] */
    
    BOOL ctschecking;			/* CTS checking enabled? [62] */

    UBYTE startpace;			/* The start pacing character [64] */
    UBYTE endpace;			/* The end pacing character [65] */

    UWORD oldcommcs;			/* Old comm vector, CS [66] */
    UWORD oldcommip;			/* Old comm vector, IP [68] */

    UBYTE saveier;			/* [70] */
    UBYTE savemcr;			/* [71] */
    UBYTE savepic;			/* [72] */
    
					/* 1 byte automatic gap... */
    UWORD commbase;			/* Comm port base address [74] */
    UBYTE commintvec;			/* Comm port interrupt vector # [76] */
    UBYTE commintdisable;		/* Interrupt disable bit mask for 
					 * this commport within the PIC1 (8289)
					 * [77] */
    UBYTE xmtidx;			/* Index into the first buflet in
    					 * each buflet chain to beginning of
    					 * transmittable data. [78]
    					 */
    } COMMINFO;


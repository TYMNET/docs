/************************************************************************
 * IOUTIL - IOCOMM utilities
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    The routines in this module are all the C language functions which
 * directly access the I/O chips plus related support.  Other functions
 * accessing the Comm chip are located in IOCOMM.ASM.
 *
 *    Several references are made to INS-8250 bugs in this module.  This
 * refers to a document by National Semiconductor entitled "National's
 * INS8250 Series of Asynchronous Communications Elements (ACE)."  (A
 * number on the document is B01361 -- This may be an ordering number.)
 * This document includes a lengthy list of various Hardware Faults and
 * incompatibilities between versions of the 8250.  
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    SAB    Initial Draft
 ************************************************************************/

#include "stddef.h"
#include "xpc.h"
#include "iocomm.h"
#include "device.h"
#include "dos.h"

/* Local definitions
 */
#define BAUD_MSB	0		/* Index into MSB of baud_tbl */
#define BAUD_LSB	1		/* Index into LSB of baud_tbl */

/* External functions from IOCOMM.ASM
 */
IMPORT VOID comm_interrupt();		/* Comm interrupt handler */
IMPORT VOID clear_xmt();		/* Clear transmitter flags */
IMPORT VOID xmt_one_char();		/* Bottom level xmt single char */

/* External functions from ASMUTL.ASM
 */
IMPORT VOID get_intvec();		/* Get interrupt vector via DOS */
IMPORT VOID set_intvec();		/* Set interrupt vector via DOS */
IMPORT VOID int_enable();		/* Enable interrupts */
IMPORT VOID int_disable();		/* Disable interrupts */
IMPORT VOID out_port();			/* Output byte to port */
IMPORT UBYTE in_port();			/* Input byte from port */

/* Other external functions
 */
IMPORT UBYTE *xpc_alloc();		/* Allocate memory from DOS */

/* External references
 */
IMPORT DEVSTAT dev_stat;		/* Device status struct */
IMPORT COMMINFO comm_info;		/* Communications information */
IMPORT UWORD xpc_cs;			/* Code segment for driver */
IMPORT UWORD xmt_intcnt;		/* Timer ticks since last transmit
					 * interrupt
					 */
    
/* Local references
 */

LOCAL UWORD port_base[] = { 0, COM1_BASEADDR, COM2_BASEADDR };
LOCAL UBYTE port_vec[] = { 0, COM1_INTVEC, COM2_INTVEC };
LOCAL UBYTE port_intdisable[] = { 0, COM1_INTDISABLE, COM2_INTDISABLE };
LOCAL UBYTE databit_tbl[] = { 0, 1, 2, 3 };
LOCAL UBYTE stopbit_tbl[] = { 0, LCR_STOPBITS, LCR_STOPBITS };
LOCAL UBYTE parity_tbl[] = { 0, LCR_PARITY, LCR_PARITY | LCR_EVENPAR,
		 LCR_PARITY | LCR_STICKPAR,
		 LCR_PARITY | LCR_EVENPAR | LCR_STICKPAR };
LOCAL UBYTE baud_tbl[8][2] =
    {
    0x04, 0x17,
    0x03, 0x00,
    0x01, 0x80,
    0x00, 0xc0,
    0x00, 0x60,
    0x00, 0x30,
    0x00, 0x18,
    0x00, 0x0c
    };

/* enable_comm()
 *
 * - Is called to set up all comm functions save transmit, including the
 * interrupt vector.  Should be called by Device Reset when changing from
 * a port address of zero to any non-zero port address.
 *
 * Returns: SUCCESS if successful
 *          COMM_ALREADY_ENABLED if communications is already enabled
 */
INT enable_comm()
    {
    FAST i;
    
    if (comm_info.commenabled)
	return(COMM_ALREADY_ENABLED);

    /* Initialize the circular receive buffer read and write pointers
     * (When they are the same, the buffer is empty.)
     */
    comm_info.wrtrecvbuf = comm_info.begrecvbuf;
    comm_info.readrecvbuf = comm_info.begrecvbuf;
    
    /* Initialize the transmit list and set transmit inactive
     */
    for (i = 0; i < XMT_BUF_CHAINS; ++i)
	{
	comm_info.xmtptr[i] = NULL;
	comm_info.xmtlen[i] = 0;
	comm_info.xmtflg[i] = 0;
	}
    clear_xmt();
    comm_info.xmtactive = NO;

    /* Clear various status flags and the link statistics array.
     */
    comm_info.breakrecvd = NO;
    comm_info.errorrecvd = NO;
    comm_info.xmtxoff = NO_PACE_OUT;
    comm_info.xoffrecvd = NO;
    for (i = 0; i < NBR_LINKSTATS; ++i)
	comm_info.linkstats[i] = 0;
    
    /* Read the current 8250 status registers
     */
    comm_info.mdmstatus = in_port(comm_info.commbase + MSR_PORT);
    comm_info.linestatus = in_port(comm_info.commbase + LSR_PORT);
    
    /* Read (and discard any junk in it) the Receive data register
     */
    (VOID)in_port(comm_info.commbase + DATAIO_PORT);
    
    /* Save the old interrupt vector, and set the new one.
     */
    get_intvec(comm_info.commintvec, &comm_info.oldcommcs,
	&comm_info.oldcommip);
    set_intvec(comm_info.commintvec, xpc_cs, (UWORD)comm_interrupt);
    
    /* Save the old IER, MCR, and PIC1 values, and set the new ones.
     * (Note that the out_port() commands for the the two 8250 registers
     * [IER, MCR] must be done twice because of a bug in some 8250 chips.
     * See INS-8250 bugs...)  At the end of this, Receive, Line status,
     * and Modem Status interrupts will be alive and kicking.
     */
    comm_info.saveier = in_port(comm_info.commbase + IER_PORT);
    comm_info.savemcr = in_port(comm_info.commbase + MCR_PORT);
    comm_info.savepic = in_port(PIC1);
    out_port(comm_info.commbase + MCR_PORT, MCR_OUT2);
    out_port(comm_info.commbase + MCR_PORT, MCR_OUT2);
    out_port(comm_info.commbase + IER_PORT,
	IER_RECVDATA | IER_LINESTATUS | IER_MDMSTATUS);
    out_port(comm_info.commbase + IER_PORT,
	IER_RECVDATA | IER_LINESTATUS | IER_MDMSTATUS);
    out_port(PIC1, (comm_info.savepic & ~comm_info.commintdisable));

    /* Finished enable of comm
     */
    comm_info.commenabled = YES;
    return(SUCCESS);
    }

/* clear_comm()
 *
 * - Is called to reset the xmit & receive comm buffers upon switching
 * from character to packet mode and vice versa.  It is assumed that any
 * chains in the transmit queue have already been released to the free
 * list, if necessary.
 *
 * Returns: none
 */
VOID clear_comm()
    {
    FAST i;
    
    if (!comm_info.commenabled)
	return;

    int_disable();
    /* Initialize the circular receive buffer read and write pointers
     * (When they are the same, the buffer is empty.)
     */
    comm_info.wrtrecvbuf = comm_info.begrecvbuf;
    comm_info.readrecvbuf = comm_info.begrecvbuf;
    
    /* Initialize the transmit list and set transmit inactive
     */
    for (i = 0; i < XMT_BUF_CHAINS; ++i)
	{
	comm_info.xmtptr[i] = NULL;
	comm_info.xmtlen[i] = 0;
	comm_info.xmtflg[i] = 0;
	}
    clear_xmt();
    int_enable();
    }

/* disable_comm()
 *
 * - Is called to disable all comm functions (including transmit) and
 * to restore the original comm interrupt vector.  Should be called by
 * Device Reset when reset with a port address of zero.  Note the duplicate
 * calls to out_port().  These are necessary to counteract a bug in some
 * versions of the 8250 chip.  (See INS-8250 bugs...)
 */
VOID disable_comm()
    {
    if (!comm_info.commenabled)
	return;
    /* Disable comm interrupts
     */
    out_port(PIC1, comm_info.savepic);
    out_port(comm_info.commbase + IER_PORT, 0);
    out_port(comm_info.commbase + IER_PORT, 0);
    
    /* Restore the old comm interrupt vector
     */
    set_intvec(comm_info.commintvec, comm_info.oldcommcs, comm_info.oldcommip);
    
    /* Restore the old IER and MCR.  This is done to graciously allow for
     * other comm programs which make unfair assumptions concerning their
     * use or lack thereof.
     */
    out_port(comm_info.commbase + IER_PORT, comm_info.saveier);
    out_port(comm_info.commbase + IER_PORT, comm_info.saveier);
    out_port(comm_info.commbase + MCR_PORT, comm_info.savemcr);
    out_port(comm_info.commbase + MCR_PORT, comm_info.savemcr);
    
    comm_info.commenabled = comm_info.xmtactive = NO;
    }
    
/* enable_xmt()
 *
 * - Is used for two purposes:  1) To turn on transmitter interrupts if
 * they are not already on.  2) To send a single character, thereby starting
 * a series of transmit interrupts to be established.
 *
 * Returns: SUCCESS if successful
 *          XMT_ALREADY_ACTIVE if transmit interrupts are active
 *              and a series of these is already being processed.
 *          COMM_NOT_ENABLED if comm interrupts as a group have not
 *              been enabled.
 */
VOID enable_xmt()
    {
    UBYTE tmp;
    
    if (comm_info.xmtactive && xmt_intcnt < MAX_XMT_INTCNT)
	return;
    if (!comm_info.commenabled)
	return;
    comm_info.xmtactive = YES;
    
    /* Turn on the THRE bit in the Interrupt Enable Register
     * Interrupts are disabled and the port is written to twice
     * in accordance with the INS-8250 bugs document.
     */
    tmp = in_port(comm_info.commbase + IER_PORT);
    int_disable();
    
    /* Transmit may require turning off before turning on...
     *
    out_port(comm_info.commbase + IER_PORT, (tmp & ~IER_THRE));
    out_port(comm_info.commbase + IER_PORT, (tmp & ~IER_THRE));
     *
     */
    out_port(comm_info.commbase + IER_PORT, (tmp | IER_THRE));
    out_port(comm_info.commbase + IER_PORT, (tmp | IER_THRE));
    int_enable();
    
    /* Transmit a character to start the ball rolling.  (There is a
     * possibility that this is unnecessary...  I will have to do some
     * probe experiments to check it out properly.)
    xmt_one_char();
     *
     */
    }

/* stop_xmt()
 *
 * - Disables the transmitter only.  There was a good reason for writing
 * this code, but it eludes me at this time of writing.
 */
VOID stop_xmt()
    {
    UBYTE tmp;
    
    tmp = in_port(comm_info.commbase + IER_PORT);
    tmp &= (~IER_THRE);
    
    /* Once again, we disable interrupts (see enable_xmt()).  It is
     * uncertain if this is really necessary here.  Better safe than
     * sorry.  (And *again* the duplicate out_port().  See INS-8250 bugs.)
     */
    int_disable();
    out_port(comm_info.commbase + IER_PORT, tmp);
    out_port(comm_info.commbase + IER_PORT, tmp);
    int_enable();
    comm_info.xmtactive = NO;
    }

/* comm_status(mask)
 *    UBYTE mask;
 *
 * - Is called to return a bit from the modem status register.
 * The bit is referred to by it's mask for that register.  (See
 * IOCOMM.H for all the MSR_xxxx masks.)  mdmstatus is updated by
 * interrupt.
 */
UWORD comm_status(mask)
    UBYTE mask;				/* Mask to get required bit */
    {
    comm_info.mdmstatus = in_port(comm_info.commbase + MSR_PORT);
    return(comm_info.mdmstatus & mask);
    }

    
/* set_break()
 *
 * - Sets the break bit in the line control register, thus starting a
 * break sequence to be transmitted.
 */
VOID set_break()
    {
    UBYTE tmp;
    
    tmp = in_port(comm_info.commbase + LCR_PORT);
    out_port(comm_info.commbase + LCR_PORT, tmp | LCR_BREAK);
    out_port(comm_info.commbase + LCR_PORT, tmp | LCR_BREAK);
    }
    

/* reset_break()
 *
 * - Clears the break bit in the line control register, thus ending a
 * break sequence.
 */
VOID reset_break()
    {
    UBYTE tmp;
    
    tmp = in_port(comm_info.commbase + LCR_PORT);
    out_port(comm_info.commbase + LCR_PORT, tmp & ~(LCR_BREAK));
    out_port(comm_info.commbase + LCR_PORT, tmp & ~(LCR_BREAK));
    }
    

/* set_modem_ctrl(bits)
 *    UBYTE bits;
 *
 * - Sets the specified bit(s) in the modem control register.
 */
VOID set_modem_ctrl(bits)
    UBYTE bits;
    {
    UBYTE tmp;
    
    tmp = in_port(comm_info.commbase + MCR_PORT);
    out_port(comm_info.commbase + MCR_PORT, tmp | bits);
    out_port(comm_info.commbase + MCR_PORT, tmp | bits);
    }
    

/* reset_modem_ctrl(bits)
 *    UBYTE bits;
 *
 * - resets the specified bit(s) in the modem control register.
 */
VOID reset_modem_ctrl(bits)
    UBYTE bits;
    {
    UBYTE tmp;
    
    tmp = in_port(comm_info.commbase + MCR_PORT);
    out_port(comm_info.commbase + MCR_PORT, tmp & ~bits);
    out_port(comm_info.commbase + MCR_PORT, tmp & ~bits);
    }
    

/* set_port(pps)
 *    PORTPARAM *pps;
 *
 * - Sets the port parameters listed in the given pps structure.  It
 * gets the correct port address from the global device status structure.
 * It ignores the xonxoff and dtedce flags.  This must be called with
 * a non zero port address in the dev_stat structure before calling
 * enable_comm()
 */
VOID set_port(pps)
    PORTPARAMS *pps;
    {
    UBYTE lcr;
    
    comm_info.commbase = port_base[dev_stat.portaddr];
    comm_info.commintvec = port_vec[dev_stat.portaddr];
    comm_info.commintdisable = port_intdisable[dev_stat.portaddr];
    comm_info.paceactive = pps->xonxoff;
    if (!comm_info.paceactive)
	comm_info.xoffrecvd = NO;

    /* Set up the Line Control Register bits for word length, parity,
     * and stopbits.  Word length (databits) is in the range of 5-8,
     * hence the subtraction to get the index into the databit table.
     * It is assumed that all port parameters are valid.
     */    
    lcr = databit_tbl[pps->databits - 5];
    lcr |= stopbit_tbl[pps->stopbits];
    lcr |= parity_tbl[pps->parity];
    
    /* Set DLAB (Divisol Latch Access Bit) so we can send the baud rate.
     * Interrupts are disabled while we are doing this so that things
     * don't access any registers while DLAB is high.  Send it out twice
     * to cover bugs in the 8250.  (See INS-8250 bugs...)
     */
    int_disable();
    out_port(comm_info.commbase + LCR_PORT, (lcr | LCR_DLAB));
    out_port(comm_info.commbase + LCR_PORT, (lcr | LCR_DLAB));
    
    /* set baud rate. Reset DLAB and restart interrupts when done.
     * (Again note the double writes for the same reason)
     */
    out_port(comm_info.commbase + DIV_PORT_MSB,
	baud_tbl[pps->baudrate][BAUD_MSB]);
    out_port(comm_info.commbase + DIV_PORT_MSB,
	baud_tbl[pps->baudrate][BAUD_MSB]);
    out_port(comm_info.commbase + DIV_PORT_LSB,
	baud_tbl[pps->baudrate][BAUD_LSB]);
    out_port(comm_info.commbase + DIV_PORT_LSB,
	baud_tbl[pps->baudrate][BAUD_LSB]);
    out_port(comm_info.commbase + LCR_PORT, lcr);
    out_port(comm_info.commbase + LCR_PORT, lcr);
    int_enable();
    }

/* init_comm()
 *
 * - This function will be called once only, when the driver is first
 * loaded.  It initializes the few things which must be initialized at
 * that time.
 */    
UBYTE *init_comm()
    {
    /* Allocate the receive buffer
     */
    comm_info.begrecvbuf = xpc_alloc(SIZ_RECV_BUF);
    comm_info.endrecvbuf = comm_info.begrecvbuf + SIZ_RECV_BUF - 1;
    
    comm_info.commenabled = NO;
    return (comm_info.begrecvbuf);
    }


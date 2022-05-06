/************************************************************************
 * IOU2 - IOCOMM utilities
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
 * This module is functionally identical to IOUTIL.C, save for different
 * function names the use of comm_info2 instead of comm_info, and the use
 * of dev_stat2 instead of dev_stat.  This is to allow the line monitor
 * to use two comm ports at one time.
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
IMPORT VOID com2_interrupt();		/* Comm interrupt handler */
IMPORT VOID xmt2_one_char();		/* Bottom level xmt single char */

/* External functions from ASMUTL.ASM
 */
IMPORT VOID get_intvec();		/* Get interrupt vector via DOS */
IMPORT VOID set_intvec();		/* Set interrupt vector via DOS */
IMPORT VOID int_enable();		/* Enable interrupts */
IMPORT VOID int_disable();		/* Disable interrupts */
IMPORT VOID out_port();			/* Output byte to port */
IMPORT UBYTE in_port();			/* Input byte from port */

/* External functions from DOSUTL.C
 */
IMPORT UBYTE *dos_alloc();		/* Allocate memory from DOS */

/* External references
 */
IMPORT DEVSTAT dev_stat2;		/* Device status struct */
IMPORT COMMINFO comm_info2;		/* Communications information */
IMPORT UWORD xpc_cs;			/* Code segment for driver */
    
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
INT enable_com2()
    {
    FAST i;
    
    if (comm_info2.commenabled)
	return(COMM_ALREADY_ENABLED);

    /* Initialize the circular receive buffer read and write pointers
     * (When they are the same, the buffer is empty.)
     */
    comm_info2.wrtrecvbuf = comm_info2.begrecvbuf;
    comm_info2.readrecvbuf = comm_info2.begrecvbuf;
    
    /* Initialize the transmit list and set transmit inactive
     */
    for (i = 0; i < XMT_BUF_CHAINS; ++i)
	{
	comm_info2.xmtptr[i] = NULL;
        comm_info2.xmtflg[i] = 0;
	comm_info2.xmtlen[i] = 0;
	}
    comm_info2.xmtactive = NO;

    /* Clear various status flags and the link statistics array.
     */
    comm_info2.breakrecvd = NO;
    comm_info2.errorrecvd = NO;
    comm_info2.xmtxoff = NO_PACE_OUT;
    for (i = 0; i < NBR_LINKSTATS; ++i)
	comm_info2.linkstats[i] = 0;
    
    /* Read the current 8250 status registers
     */
    comm_info2.mdmstatus = in_port(comm_info2.commbase + MSR_PORT);
    comm_info2.linestatus = in_port(comm_info2.commbase + LSR_PORT);

    /* Save the old interrupt vector, and set the new one.
     */
    get_intvec(comm_info2.commintvec, &comm_info2.oldcommcs,
	&comm_info2.oldcommip);
    set_intvec(comm_info2.commintvec, xpc_cs, (UWORD)com2_interrupt);
    
    /* Save the old IER, MCR, and PIC1 values, and set the new ones.
     * (Note that the out_port() commands for the the two 8250 registers
     * [IER, MCR] must be done twice because of a bug in some 8250 chips.
     * See INS-8250 bugs...)  At the end of this, Receive, Line status,
     * and Modem Status interrupts will be alive and kicking.
     */
    comm_info2.saveier = in_port(comm_info2.commbase + IER_PORT);
    comm_info2.savemcr = in_port(comm_info2.commbase + MCR_PORT);
    comm_info2.savepic = in_port(PIC1);
    out_port(comm_info2.commbase + MCR_PORT, MCR_OUT2);
    out_port(comm_info2.commbase + MCR_PORT, MCR_OUT2);
    out_port(comm_info2.commbase + IER_PORT,
	IER_RECVDATA | IER_LINESTATUS | IER_MDMSTATUS);
    out_port(comm_info2.commbase + IER_PORT,
	IER_RECVDATA | IER_LINESTATUS | IER_MDMSTATUS);
    out_port(PIC1, (comm_info2.savepic & ~comm_info2.commintdisable));

    /* Finished enable of comm
     */
    comm_info2.commenabled = YES;
    return(SUCCESS);
    }

/* disable_comm()
 *
 * - Is called to disable all comm functions (including transmit) and
 * to restore the original comm interrupt vector.  Should be called by
 * Device Reset when reset with a port address of zero.  Note the duplicate
 * calls to out_port().  These are necessary to counteract a bug in some
 * versions of the 8250 chip.  (See INS-8250 bugs...)
 */
VOID disable_com2()
    {
    if (!comm_info2.commenabled)
	return;
    /* Disable comm interrupts
     */
    out_port(PIC1, comm_info2.savepic);
    out_port(comm_info2.commbase + IER_PORT, 0);
    out_port(comm_info2.commbase + IER_PORT, 0);
    
    /* Restore the old comm interrupt vector
     */
    set_intvec(comm_info2.commintvec, comm_info2.oldcommcs, comm_info2.oldcommip);
    
    /* Restore the old IER and MCR.  This is done to graciously allow for
     * other comm programs which make unfair assumptions concerning their
     * use or lack thereof.
     */
    out_port(comm_info2.commbase + IER_PORT, comm_info2.saveier);
    out_port(comm_info2.commbase + IER_PORT, comm_info2.saveier);
    out_port(comm_info2.commbase + MCR_PORT, comm_info2.savemcr);
    out_port(comm_info2.commbase + MCR_PORT, comm_info2.savemcr);
    
    comm_info2.commenabled = comm_info2.xmtactive = NO;
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
INT enable_xmt2()
    {
    UBYTE tmp;
    
    if (comm_info2.xmtactive)
	return(XMT_ALREADY_ACTIVE);
    if (!comm_info2.commenabled)
	return(COMM_NOT_ENABLED);
    comm_info2.xmtactive = YES;
    
    /* Turn on the THRE bit in the Interrupt Enable Register
     * Interrupts are disabled and the port is written to twice
     * in accordance with the INS-8250 bugs document.
     */
    tmp = in_port(comm_info2.commbase + IER_PORT);
    int_disable();
    
    /* Transmit may require turning off before turning on...
     *
    out_port(comm_info2.commbase + IER_PORT, (tmp & ~IER_THRE));
    out_port(comm_info2.commbase + IER_PORT, (tmp & ~IER_THRE));
     *
     */
    out_port(comm_info2.commbase + IER_PORT, (tmp | IER_THRE));
    out_port(comm_info2.commbase + IER_PORT, (tmp | IER_THRE));
    int_enable();
    
    /* Transmit a character to start the ball rolling.  (There is a
     * possibility that this is unnecessary...  I will have to do some
     * probe experiments to check it out properly.)
    xmt2_one_char();
     *
     */
    return(SUCCESS);
    }

/* stop_xmt()
 *
 * - Disables the transmitter only.  There was a good reason for writing
 * this code, but it eludes me at this time of writing.
 */
VOID stop_xmt2()
    {
    UBYTE tmp;
    
    tmp = in_port(comm_info2.commbase + IER_PORT);
    tmp &= (~IER_THRE);
    
    /* Once again, we disable interrupts (see enable_xmt()).  It is
     * uncertain if this is really necessary here.  Better safe than
     * sorry.  (And *again* the duplicate out_port().  See INS-8250 bugs.)
     */
    int_disable();
    out_port(comm_info2.commbase + IER_PORT, tmp);
    out_port(comm_info2.commbase + IER_PORT, tmp);
    int_enable();
    comm_info2.xmtactive = NO;
    }

/* comm_status(mask)
 *    UBYTE mask;
 *
 * - Is called to return a bit from the modem status register.
 * The bit is referred to by it's mask for that register.  (See
 * IOCOMM.H for all the MSR_xxxx masks.)  mdmstatus is updated by
 * interrupt.
 */
UWORD com2_status(mask)
    UBYTE mask;				/* Mask to get required bit */
    {
    return(comm_info2.mdmstatus & mask);
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
VOID set_port2(pps)
    PORTPARAMS *pps;
    {
    UBYTE lcr;
    
    comm_info2.commbase = port_base[dev_stat2.portaddr];
    comm_info2.commintvec = port_vec[dev_stat2.portaddr];
    comm_info2.commintdisable = port_intdisable[dev_stat2.portaddr];

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
    out_port(comm_info2.commbase + LCR_PORT, (lcr | LCR_DLAB));
    out_port(comm_info2.commbase + LCR_PORT, (lcr | LCR_DLAB));
    
    /* set baud rate. Reset DLAB and restart interrupts when done.
     * (Again note the double writes for the same reason)
     */
    out_port(comm_info2.commbase + DIV_PORT_MSB,
	baud_tbl[pps->baudrate][BAUD_MSB]);
    out_port(comm_info2.commbase + DIV_PORT_MSB,
	baud_tbl[pps->baudrate][BAUD_MSB]);
    out_port(comm_info2.commbase + DIV_PORT_LSB,
	baud_tbl[pps->baudrate][BAUD_LSB]);
    out_port(comm_info2.commbase + DIV_PORT_LSB,
	baud_tbl[pps->baudrate][BAUD_LSB]);
    out_port(comm_info2.commbase + LCR_PORT, lcr);
    out_port(comm_info2.commbase + LCR_PORT, lcr);
    int_enable();
    }

/* init_comm()
 *
 * - This function will be called once only, when the driver is first
 * loaded.  It initializes the few things which must be initialized at
 * that time.
 */    
INT init_com2()
    {
    /* Allocate the receive buffer
     */
    comm_info2.begrecvbuf = dos_alloc(SIZ_RECV_BUF);
    comm_info2.endrecvbuf = comm_info2.begrecvbuf + SIZ_RECV_BUF - 1;
    
    comm_info2.commenabled = NO;
    return(SUCCESS);
    }

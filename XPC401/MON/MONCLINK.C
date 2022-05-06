/************************************************************************
 * monclink.c - X.PC line monitor link interface (character mode)
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    This module contains routines responsible for link level input to
 * the X.PC line monitor.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KJB   Initial Draft
 *
 ************************************************************************/
#include "stddef.h"
#include "xpc.h"
#include "appl.h"
#include "device.h"
#include "error.h"
#include "iocomm.h"
#include "mon.h"
#include "state.h"
#include "timer.h"

IMPORT DATA *alloc_data();		/* allocates a data entry */
IMPORT VOID put_data();			/* adds data entry to queue */
IMPORT VOID p_link();			/* packet mode link input (port 1) */
IMPORT VOID p_link2();			/* packet mode link input (port 2) */

IMPORT COMMINFO comm_info;		/* comm info structure */
IMPORT COMMINFO comm_info2;		/* comm info structure 2 */
IMPORT DEVSTAT dev_stat;		/* device status structure */
IMPORT UWORD tick_count;		/* timer interrupt count */

/************************************************************************
 * LOCAL VOID read_crb(rdlen, pb)
 *     INT rdlen;		number of bytes to copy
 *     BUFLET *pb;		pointer to buflet chain
 *
 *     read_crb is used by the c_link function, below, to read rdlen
 * bytes from the circular read buffer in the comm_info structure to
 * the buflet chain at pb.
 *
 * Notes: read_crb is an independent function for historical reasons
 *     only.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
LOCAL VOID read_crb(rdlen, pb)
    INT rdlen;				/* number of bytes to copy */
    BUFLET *pb;				/* pointer to buflet chain */
    {
    INT bdoff;				/* buflet data offset */
    FAST UBYTE *pd;			/* pointer to (buflet) data */
    FAST count;				/* copy count */

    bdoff = 1;				/* first position saves data length */
    while (rdlen)			/* for number of bytes to read */
	{

	/* copy (up to) a buflet's worth of data from the circular read
	 * buffer to the buflet chain
	 */
	pd = &pb->bufdata[bdoff];	/* set pd to start of buflet data */
    
	/* determine the number of bytes available in the current buflet,
	 * and decrement the circular buffer read length by this count (note
	 * min is a macro (stddef.h); redundant calculations may be done here)
	 */
	count = min(DATA_BUF_SIZ - bdoff, rdlen);
    	rdlen -= count;
	
	/* to avoid unnecessary tests, a check is made to see if the data
	 * being read wraps around the end of the circular read buffer (note
	 * pointer comparison may not work with 8088 large data models)
	 */
	if (comm_info.readrecvbuf + count < comm_info.endrecvbuf)
	
	    /* copy count bytes without wrap check
	     */
	    do  {
		*pd++ = *comm_info.readrecvbuf++;
		} while (--count);
	else
	
	    /* copy count bytes with wrap check
	     */
    	    do  {
		*pd++ = *comm_info.readrecvbuf++;
		if (comm_info.endrecvbuf < comm_info.readrecvbuf)
		    comm_info.readrecvbuf = comm_info.begrecvbuf;
		} while (--count);
	pb = pb->bufnext;		/* set pb to next data buflet */
	bdoff = 0;			/* offset used in first buflet only */
	}
    }

/************************************************************************
 * LOCAL VOID read_crb2(rdlen, pb)
 *     INT rdlen;		number of bytes to copy
 *     BUFLET *pb;		pointer to buflet chain
 *
 *     read_crb2 is used by the c_link2 function, below, to read rdlen
 * bytes from the circular read buffer in the comm_info2 structure to
 * the buflet chain at pb.
 *
 * Notes: read_crb2 is an independent function for historical reasons
 *     only.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
LOCAL VOID read_crb2(rdlen, pb)
    INT rdlen;				/* number of bytes to copy */
    BUFLET *pb;				/* pointer to buflet chain */
    {
    INT bdoff;				/* buflet data offset */
    FAST UBYTE *pd;			/* pointer to (buflet) data */
    FAST count;				/* copy count */

    bdoff = 1;				/* first position saves data length */
    while (rdlen)			/* for number of bytes to read */
	{

	/* copy (up to) a buflet's worth of data from the circular read
	 * buffer to the buflet chain
	 */
	pd = &pb->bufdata[bdoff];	/* set pd to start of buflet data */
    
	/* determine the number of bytes available in the current buflet,
	 * and decrement the circular buffer read length by this count (note
	 * min is a macro (stddef.h); redundant calculations may be done here)
	 */
	count = min(DATA_BUF_SIZ - bdoff, rdlen);
    	rdlen -= count;
	
	/* to avoid unnecessary tests, a check is made to see if the data
	 * being read wraps around the end of the circular read buffer (note
	 * pointer comparison may not work with 8088 large data models)
	 */
	if (comm_info2.readrecvbuf + count < comm_info2.endrecvbuf)
	
	    /* copy count bytes without wrap check
	     */
	    do  {
		*pd++ = *comm_info2.readrecvbuf++;
		} while (--count);
	else
	
	    /* copy count bytes with wrap check
	     */
    	    do  {
		*pd++ = *comm_info2.readrecvbuf++;
		if (comm_info2.endrecvbuf < comm_info2.readrecvbuf)
		    comm_info2.readrecvbuf = comm_info2.begrecvbuf;
		} while (--count);
	pb = pb->bufnext;		/* set pb to next data buflet */
	bdoff = 0;			/* offset used in first buflet only */
	}
    }

/************************************************************************
 * VOID c_link()
 *
 *     c_link is the X.PC line monitor link level input routine for
 * character mode. This routine is called by the timer interrupt code
 * each time the interrupt is received.
 *
 * Notes: c_link runs with interrupts enabled.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID c_link()
    {
    DATA *pd;				/* pointer to alloc'ed data entry */
    INT rdlen;				/* calculated read length */

    /* calculate the length of data to be read from the circular read
     * buffer (note pointer arithmetic will not work with 8088 large
     * data models)
     */
    rdlen = comm_info.wrtrecvbuf - comm_info.readrecvbuf;
    if (rdlen)
	{
    
	/* adjust read length, if negative (read buffer is circular)
	 */
	if (rdlen < 0)
	    rdlen += SIZ_RECV_BUF;

	/* at most CRB_MAX_READ bytes may be read into 1 buflet chain
	 */
	if (CRB_MAX_READ < rdlen)
	    rdlen = CRB_MAX_READ;
	
	/* if data entry is successfully allocated...
	 */
	if ((pd = alloc_data((BYTES)bufreq(rdlen + 1))) != NULLDATA)
	    {
	
	    /* copy rdlen bytes to the newly allocated buflet chain, install
	     * the data length in the first data location of the buflet chain,
	     * and add the data to the received data queue
	     */
	    read_crb(rdlen, pd->pbuf);
	    pd->pbuf->bufdata[0] = (UBYTE)rdlen;
	    put_data(pd, RPORT1, RCMODE);
	    }
	}
    }
	
/************************************************************************
 * VOID c_link2()
 *
 *     c_link2 is the X.PC line monitor link level input routine for
 * character mode. This routine is called by the timer interrupt code
 * each time the interrupt is received.
 *
 * Notes: c_link2 runs with interrupts enabled.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID c_link2()
    {
    DATA *pd;				/* pointer to alloc'ed data entry */
    INT rdlen;				/* calculated read length */

    /* calculate the length of data to be read from the circular read
     * buffer (note pointer arithmetic will not work with 8088 large
     * data models)
     */
    rdlen = comm_info2.wrtrecvbuf - comm_info2.readrecvbuf;
    if (rdlen)
	{
    
	/* adjust read length, if negative (read buffer is circular)
	 */
	if (rdlen < 0)
	    rdlen += SIZ_RECV_BUF;

	/* at most CRB_MAX_READ bytes may be read into 1 buflet chain
	 */
	if (CRB_MAX_READ < rdlen)
	    rdlen = CRB_MAX_READ;
	
	/* if data entry is successfully allocated...
	 */
	if ((pd = alloc_data((BYTES)bufreq(rdlen + 1))) != NULLDATA)
	    {
	
	    /* copy rdlen bytes to the newly allocated buflet chain, install
	     * the data length in the first data location of the buflet chain,
	     * and add the data to the received data queue
	     */
	    read_crb2(rdlen, pd->pbuf);
	    pd->pbuf->bufdata[0] = (UBYTE)rdlen;
	    put_data(pd, RPORT2, RCMODE);
	    }
	}
    }
	    
/************************************************************************
 * VOID do_link()
 *
 *     do_link is called by the X.PC line monitor each time a timer 
 * interrupt is received. The appropriate link input routines will be
 * called based on the current monitor state (CHAR_STATE or PKT_STATE).
 *
 * Notes: Only the device status structure for device 1 is checked to
 *     determine the current mode.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID do_link()
    {

    ++tick_count;			/* bump timer interrupt count */

    /* if we're running in character mode, perform character mode input
     */
    if (dev_stat.devstate == CHAR_STATE)
	{
	c_link();
	c_link2();
	}
    
    /* else perform packet mode input
     */
    else
	{
	p_link();
	p_link2();
	}
    }

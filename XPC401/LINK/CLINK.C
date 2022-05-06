/************************************************************************
 * clink.c - character mode link level interface
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    This module contains link interface functions to handle character
 *    mode I/O.
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
#include "timer.h"

IMPORT BUFLET *alloc_buf();		/* returns pointer to buflet chain */
IMPORT BUFLET *get_queue();		/* returns pointer to buflet chain */
IMPORT VOID add_queue();		/* installs buflet chain in queue */
IMPORT VOID enable_xmt();		/* kicks iocomm transmitter */
IMPORT VOID free_buf();			/* frees allocated buflets */
IMPORT VOID int_disable();		/* disables interrupts */
IMPORT VOID int_enable();		/* enables interrupts */

IMPORT CHNLINFO cis[];			/* channel info structure */
IMPORT COMMINFO comm_info;		/* comm info structure */
IMPORT PORTPARAMS port_params;		/* port parameters structure */
IMPORT UWORD free_count;		/* buflet free list count */

LOCAL CHNLINFO *pcis;			/* pointer to channel information
					 * strucuture.
					 */
/************************************************************************
 * LOCAL VOID read_crb(rdlen, pb)
 *     INT rdlen;		number of bytes to read
 *     BUFLET *pb;		pointer to first buflet in chain
 *
 *     read_crb is used by the c_link function, below, to read rdlen
 *     bytes from the circular read buffer to the buflet chain at pb.
 *
 * Notes: read_crb is a separate function for historical reasons only.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
LOCAL VOID read_crb(rdlen, pb)
    INT rdlen;				/* number of bytes to copy */
    BUFLET *pb;				/* pointer to buflet (chain) */
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
 * VOID c_link()
 *
 *     c_link is responsible for link level character mode I/O. On the
 *     transmit side, c_link will maintain the IOCOMM transmit buffer:
 *     transmitted buflet chains are freed, and chains pending in the
 *     outgoing packet queue are placed in the transmit buffer. On the
 *     receive side, data is copied from the circular read buffer and
 *     placed in the application read queue.
 *
 * Notes: None.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID c_link()
    {
    BUFLET *pc;				/* pointer to buflet chain */
    BYTES reqbuf;			/* required # of buflets for read */
    INT nbytes;				/* number of bytes in read buffer */
    INT rdlen;				/* number of bytes to read per pass */
    FAST x;				/* scratch int */


    pcis = &cis[0];			/* pointer to channel 0's channel
    					 * information structure.
    					 */
    
    /* scan the iocomm transmit buffer, high to low, until an untransmitted
     * entry is found (indicated by a non-zero transmit length). free any
     * transmitted buflet chains encountered
     */
    x = XMT_BUF_CHAINS;
    while (x-- && !comm_info.xmtlen[x])
	if (comm_info.xmtptr[x])
	    {
	    free_buf(comm_info.xmtptr[x]);
	    comm_info.xmtptr[x] = NULLBUF;
	    }

    /* while the iocomm transmit buffer is not full, and an entry exists
     * in the outgoing packet queue for the current channel, retrieve the
     * queue entry, subtract the entry length from the write data count,
     * and install its address and length in the iocomm transmit buffer(s)
     */
    while (++x < XMT_BUF_CHAINS &&
	(pc = get_queue(&pcis->lcistruct->outpktqueue)) != NULLBUF)
	    {
	    pcis->nwritebytes -= pc->bufdata[0];
	    int_disable();		/* disable interrupts */
	    comm_info.xmtptr[x] = pc;
	    comm_info.xmtlen[x] = pc->bufdata[0];
	    int_enable();		/* enable interrupts */
	    }

    /* if an XOFF has not been received from the host, enable (kick)
     * the transmitter
     */
    if (!comm_info.xoffrecvd)
	enable_xmt();
    
    /* read will be performed only if 1 or more bytes exist in the circular
     * read buffer. note pointer calculation will not work with 8088 large
     * data models
     */
    nbytes = comm_info.wrtrecvbuf - comm_info.readrecvbuf;
    if (nbytes)
	{
	
	/* adjust nbytes, if negative (read buffer is circular)
	 */
	if (nbytes < 0)
	    nbytes += SIZ_RECV_BUF;

	/* at most CRB_MAX_READ data bytes will be moved from the circular
	 * read buffer
	 */
	rdlen = min(nbytes, CRB_MAX_READ);

	/* calculate the required number of buflets to hold rdlen + 1
	 * (length byte) bytes (note bufreq is a macro (xpc.h) used to 
	 * calculate the number of buflets required)
	 */
	reqbuf = bufreq(rdlen + 1);

	/* if more than ARQ_HIWATER bytes exist in the application read
	 * queue, or allocating the required number of buflets will cause
	 * the buflet free count to drop below BUF_LOWATER, or the buflet
	 * allocation fails (should "never happen"), the application will
	 * be given a chance to catch up; the read will not be performed
	 */
	if (ARQ_HIWATER < pcis->nreadbytes ||
	    free_count < reqbuf + BUF_LOWATER ||
		(pc = alloc_buf(reqbuf)) == NULLBUF)
		    {

		    /* if more than CRB_HIWATER bytes exist in the circular 
		     * read buffer, the host should be alerted. if XON/XOFF
		     * handshaking is being performed, and an XOFF is not
		     * pending, have the transmitter send an XOFF (startpace)
		     * character
		     */
		    if (CRB_HIWATER < nbytes && port_params.xonxoff &&
			comm_info.xmtxoff == NO_PACE_OUT)
			    {
			    comm_info.xmtxoff = SEND_STARTPACE;
			    enable_xmt();
			    }
		    return;		/* read is not performed */
		    }
	
	/* read rdlen bytes from the circular read buffer into the buflet
	 * chain at pc (read_crb is a static function, above), install the
	 * byte count in the first data position of the buflet chain, add
	 * the buflet chain to the application read queue, and increment
	 * the number of bytes in the queue
	 */
	read_crb(rdlen, pc);
	pc->bufdata[0] = rdlen;
	add_queue(&pcis->readqueue, pc);
	pcis->nreadbytes += rdlen;

	/* if the host has been XOFF'ed (startpace character sent), and the
	 * number of bytes left in the circular read buffer has fallen below
	 * CRB_LOWATER, have the transmitter send an XON (endpace) character
	 */
	if (comm_info.xmtxoff == SENT_STARTPACE &&
	    (nbytes - rdlen) < CRB_LOWATER)
		{
		comm_info.xmtxoff = SEND_ENDPACE;
		enable_xmt();
		}
	}
    }

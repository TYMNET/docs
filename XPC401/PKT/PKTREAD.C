/************************************************************************
 * pktread.c - Application Read of Data Packets
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    pktread.c contains the module p_read_data(), which is 
 *    called from the application to read data from the
 *    application read queue.  The application must first perform
 *    the report I/O status function.  The report I/O status function
 *    reads data packets from the pad input queue and adds the data
 *    packets to the application read queue.  It also  informs the
 *    application of how many bytes of data are in the application
 *    read queue.   p_read_data() is called to read  data when the 
 *    device is in packet mode.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KS    Initial Draft
 * 06/23/87   4.01    KS    Bug, when there was a yellow packet in the
 *			    queue and there is no data to read, a
 *			    orange ball is never sent.  Change was made
 *			    to remove yellow balls from the  beginning
 *			    of the queue.
 *
 ************************************************************************/
#include "stddef.h"
#include "xpc.h"
#include "pkt.h"
#include "appl.h"
#include "state.h"
#include "timer.h"
#include "error.h"

IMPORT BUFLET *get_queue();		/* get next packet in queue */
IMPORT INT read_buflet_data();		/* function to read packet data */
IMPORT INT send_cpkt();			/* resend control packet */
IMPORT VOID free_buf();			/* free buflet chain */
IMPORT VOID int_disable();		/* disable interrupts */
IMPORT VOID int_enable();		/* enable interrupts */
IMPORT VOID mov_param();		/* move data to/from application
					 * parameter buffer.
					 */
IMPORT VOID stop_timer();		/* stop timer */

IMPORT CHNLINFO *ap_pcis;		/* channel information pointer which
					 * was set up by application interface
					 */
#ifdef DEBUG
IMPORT INT num_readbytes;		/* number of bytes read */
#endif

IMPORT UBYTE app_chnl;			/* channel specified by application */
IMPORT UWORD program_error;		/* program error */
/************************************************************************
 * INT p_read_data()
 *
 *    p_read_data() reads data from the application read queue.  
 *    The data read is moved into the application parameter buffer.
 *    p_read_data() processes until the number of bytes specified
 *    by the application has been read or there is no more data
 *    in the application read queue.  If p_read_data reads a yellow
 *    ball packet from the read queue, it acknowledges the yellow
 *    ball with an orange ball packet.
 *
 * Returns:   
 *     SUCCESS - Processing was successful.
 *     ILLEGAL_CHNL_FUNC - Application specified channel 0 or
 *         channel is not connected.
 *
 ************************************************************************/
INT p_read_data()
    {
    BUFLET *pb;			        /* beginning buflet pointer */
    BUFLET *ppkt;		        /* current buflet pointer */
    UWORD format = TYMNET_STRING;	/* buffer format */
    UWORD idx;		                /* index into buflet */
    UWORD nreq;			        /* maximum number of bytes to read */
    UWORD num = 0;		        /* number of bytes read from
				         * buflet chain.
				         */
    UWORD opos;			        /* output buffer position */
    UWORD totleft;			/* number of bytes left to
					 * read.
					 */

    /* If channel is not connected or the application specified channel
     * 0, then return an error.
     */
    if (ap_pcis->chnlstate != CHNL_CONNECTED  || app_chnl == 0)
	return (ILLEGAL_CHNL_FUNC);
    mov_param((UBYTE *)&nreq, sizeof(UWORD), PARAM_2, 0, FROM_APPL);

    
    pb = ppkt = ap_pcis->readqueue.begqueue;

    /* Calculate application read queue's current buflet pointer
     * and the index into the buflet.  ppkt will point to the
     * current buflet and idx is the index into the bufet data.
     */
    for (idx = ap_pcis->idxreadqueue; ppkt && (idx >= DATA_BUF_SIZ);
	ppkt = ppkt->bufnext, idx -= DATA_BUF_SIZ)
	;

    /* Read characters from the application read queue until the
     * specified number of bytes have been read or there is no
     * more input data.
     */
    for (opos = 0, totleft = nreq; ppkt && totleft && pb;
	totleft -= num, pb = ppkt = ap_pcis->readqueue.begqueue)
	{

	/*  If the incoming packet is a YELLOW BALL, then acknowledge
	 *  yellow ball with an orange ball.
	 */
	if (pb->bufdata[GFI_LCI] & QBIT)
	    {
	    if ((pb = get_queue(&ap_pcis->readqueue)) == NULLBUF)
		{
		program_error = P_READ_ERROR;
		break;
		}

	    /* Add orange ball packet to pad output queue.
	     */
	    if (pb->bufdata[PKT_TYP_ID] == YELLOW_BALL)
		(VOID)send_cpkt(ap_pcis->lcistruct,  ORANGE_BALL, 
		    (UBYTE *)0, MUST_LINK, pb);
	    else
		{
		free_buf(pb);
		program_error = P_READ_ERROR;
		}
	    num = 0;
	    }
	
	/* Move data from the application read queue into the application
	 * parameter buffer.
	 */
	else
	    {
	    num = (UWORD)read_buflet_data(ppkt, totleft,
		(UWORD)pb->bufdata[FRAME_LEN] + 1, (WORD)idx, &opos);
	    int_disable();
	    
	    /* Update the number of bytes in the application read queue.
	     */
	    ap_pcis->nreadbytes -= num;
	    int_enable();
	    
	    /* If the buflet chain is an echo packet, clear the
	     * echo packet pointer.
	     */
	    if (pb == ap_pcis->echopkt)
		{
		ap_pcis->echopkt = NULLBUF;
		ap_pcis->idxechodata = MOVED_FIRST_BYTE;
		}
	    
	    /* If finished with the current buflet chain remove the
	     * buflet from the read queue and free it.
	     */
	    if (num == (UWORD)(pb->bufdata[FRAME_LEN] + 1))
		{
	    
		/* Packet is read from the application read queue after
		 * it has been processed.  The reason this is done is
		 * to avoid another global pointer.
		 */
		if ((pb = get_queue(&ap_pcis->readqueue)) == NULLBUF)
		    {
		    program_error = P_READ_ERROR;
		    break;
		    }
		free_buf(pb);
		idx = ap_pcis->idxreadqueue = MOVED_FIRST_BYTE;
		}
	    
	    /* Increment the index into the current buflet chain and update
	     * the number of bytes of extra data in data packet.
	     */
	    else
		{
		ap_pcis->idxreadqueue += num;
		pb->bufdata[FRAME_LEN] -= num;
		}
	    if (num == totleft)
		break;
	    }
	}
    pb = ap_pcis->readqueue.begqueue;
    
    /*  If the incoming packet is a YELLOW BALL, then acknowledge
     *  yellow ball with an orange ball.
     */
    if (pb && pb->bufdata[GFI_LCI] & QBIT)
	{
	if ((pb = get_queue(&ap_pcis->readqueue)) == NULLBUF)
	    program_error = P_READ_ERROR;

	/* Add orange ball packet to pad output queue.
	 */
	else if (pb->bufdata[PKT_TYP_ID] == YELLOW_BALL)
	    (VOID)send_cpkt(ap_pcis->lcistruct,  ORANGE_BALL, 
		(UBYTE *)0, MUST_LINK, pb);
	else
	    {
	    free_buf(pb);
	    program_error = P_READ_ERROR;
	    }
	}
#ifdef DEBUG
    num_readbytes = num_readbytes + opos;
#endif     

    /* Move the number of bytes read and a hard-coded format into
     * the application read queue.
     */
    mov_param((UBYTE *)&opos, sizeof(UWORD), PARAM_3, 0, TO_APPL);
    mov_param((UBYTE *)&format, sizeof(UWORD), PARAM_4, 0, TO_APPL);
    return (SUCCESS);
    }
 

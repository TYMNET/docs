/************************************************************************
 * pktflush.c - Flush Channel Input and Output Queues
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    pktflush.c contains the source modules which are used to
 *    flush the channel input and output queues.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KS    Initial Draft
 *
 ************************************************************************/
#include "stddef.h"
#include "xpc.h"
#include "pkt.h"
#include "state.h"
#include "device.h"
#include "timer.h"
#include "error.h"

IMPORT BOOL ok_to_free();			/* ensure that it is ok to
						 * to free packet.  packet can
						 * not be freed if it is 
						 * currently being transmitted.
						 */
IMPORT BUFLET *get_queue();                     /* get first packet in queue */
IMPORT BUFLET *remove_queue();                  /* remove buflet from queue */
IMPORT INT send_cpkt();                         /* reuse control pkt */
IMPORT VOID free_buf();                         /* free buflet chain */
IMPORT VOID int_disable();			/* disable interrupts */
IMPORT VOID int_enable();			/* enable interrupts */

IMPORT CHNLINFO *ap_pcis;	                /* current channel information
					         * structure pointer.
					         */
IMPORT UBYTE app_chnl;				/* application channel */
IMPORT UWORD trans_idx;				/* index into assembly 
						 * transmit buffer.
						 */
#ifdef DEBUG
IMPORT WORD diags[];				/* used for debugging.
						 */
#endif

/************************************************************************
 * INT flush_pkt_input()
 *
 *    flush_pkt_input() performs the flush input command when
 *    the device is in packet state.   The application read queue is 
 *    flushed. Any yellow ball packets in the application read queue
 *    are acknowledged by an oragne ball packet.  The pad input queue 
 *    is flushed until the first q-bit packet is found.  A q-bit packet
 *    is a control packet with the q-bit set.
 *
 * Returns:  If the application channel is not connected or is
 *    not a packet channel, ILLEGAL_CHNL_FUNC is returned.   Otherwise,
 *    flush_pkt_input returns SUCCESS.
 *
 ************************************************************************/
INT flush_pkt_input()
    {
    BUFLET *pb;                                 /* pointer buflet in queue */
     

    /* If the channel is not connected then return an error.
     */
    if (app_chnl == 0 || ap_pcis->chnlstate != CHNL_CONNECTED)
	return(ILLEGAL_CHNL_FUNC);
	
    /* Flush all packets in the applications read queue.  If the
     * applications read queue contains a yellow ball, it
     * acknowledges the yellow ball packet with an orange ball
     * packet.
     */
    ap_pcis->echopkt = NULLBUF;
    ap_pcis->idxechodata = MOVED_FIRST_BYTE;
    while((pb = get_queue(&ap_pcis->readqueue)) != NULLBUF)
        {
        if ((pb->bufdata[GFI_LCI] & QBIT) && (pb->bufdata[PKT_TYP_ID] == 
	    YELLOW_BALL))
	    (VOID)send_cpkt(ap_pcis->lcistruct, ORANGE_BALL, (UBYTE *)0, 
		MUST_LINK, pb);
	else
	    free_buf(pb);
        }

    /*  Flush all data packets from the beginning of the pad input 
     *  queue.  Processing stops when the first q-bit packet is
     *  found.
     */
    FOREVER
	{
	int_disable();
	if ((pb = ap_pcis->lcistruct->inpktqueue.begqueue) != NULLBUF)
	    if ((pb->bufdata[GFI_LCI] & QBIT) == 0)
		{
		int_enable();
		
		/* Flush data packet from the pad input queue.
		 */
		pb = get_queue(&ap_pcis->lcistruct->inpktqueue);
		free_buf(pb);
		continue;
		}
	int_enable();
	break;
	}
    return(SUCCESS);
    }
    

/************************************************************************
 * INT flush_pkt_output()
 *
 *    flush_pkt_output() frees the transmit assembly packet and
 *    all data packets at the end of the pad output queue.  
 *
 * Returns:  If the application channel is not connected or is
 *    not a packet channel, ILLEGAL_CHNL_FUNC is returned.   Otherwise,
 *    flush_pkt_output returns SUCCESS.
 *
 ************************************************************************/
INT flush_pkt_output()
    {
    BUFLET *pb;                                 /* pointer buflet in queue */

    /* If the channel is not connected, then return an error.
     */
    if (app_chnl == 0 || ap_pcis->chnlstate != CHNL_CONNECTED)
	return(ILLEGAL_CHNL_FUNC);

    /* Free the transmit assembly packet and clear the number of bytes in
     * the transmit assembly packet.
     */
    if (ap_pcis->assemblypkt)
        {
	int_disable();
        ap_pcis->nwritebytes = 0;
        free_buf(ap_pcis->assemblypkt);
        ap_pcis->assemblypkt = NULLBUF;
	int_enable();
        }

    /* Search the pad output queue from the back until a q-bit
     * packet is found.  Interrupts are turned off to ensure that
     * link does not attempt to output the packet which is being
     * flushed.  After the data packet is removed from the queue,
     * a check is performed to ensure that the packet is not
     * currently being transmitted.  If the packet is being
     * transmitted it can not be freed.  The routine ok_to_free()
     * will set a flag in the iocomm table which will allow the
     * packet to be freed after it has been transmitted.
     */
    FOREVER
	{
	int_disable();
	if ((pb = ap_pcis->lcistruct->outpktqueue.endqueue) != NULLBUF)
	    if (!(pb->bufdata[GFI_LCI] & QBIT))
		{
		
		/* Remove the data packet from the queue.
		 */
		(VOID)remove_queue(&ap_pcis->lcistruct->outpktqueue, pb);
		int_enable();
		
		/* If the data packet is not being transmitted, free it.
		 */
		if (ok_to_free(pb))
		    free_buf(pb);
		continue;
		}
	int_enable();
	break;
        }
    return (SUCCESS);
    }



		

		


/************************************************************************
 * seqnum.c - Validate and Maintain Sequence Numbers
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    seqnum.c contains the modules which are called to validate
 *    and update sequence numbers.  
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
#include "link.h"
#include "state.h"
#include "timer.h"
#include "error.h"

IMPORT BOOL ok_to_free();		/* It's ok to free buflet */
IMPORT BUFLET *get_queue();		/* get buflet chain from queue */
IMPORT VOID free_buf();			/* free buflet chain */
IMPORT VOID start_timer();		/* start the timer */
IMPORT VOID stop_timer();		/* stop timer */
IMPORT VOID tim_t25();			/* time out for t25 timer */

IMPORT LCIINFO lcis[];			/* logical channel information
					 * structure.
					 */
IMPORT UBYTE linkchnl;			/* linkchnl number */
IMPORT WORD len_mult;			/* multiplier for T25 and T27 timer
					 * interrupts.
					 */
IMPORT UWORD program_error;		/* program error */
/************************************************************************
 * INT valid_recvseq(plcis, recv_send_seqnum)
 *    LCIINFO *plcis;	- pointer to logical channel information structure
 *    UBYTE recv_send_seqnum - received send sequence number 
 *
 *    valid_recvseq() ensures that the packet sequence number P(S) received
 *    matches the expected P(S).
 *
 * Returns:  
 *    PS_DUPLICATE - Input P(S) is a duplicate
 *    PS_SEQNUM_ERROR - Invalid input P(S)
 *    SUCCESS - Input P(S) is equal to expected input P(S)
 *
 ************************************************************************/
INT valid_recvseq(plcis, recv_send_seqnum)
    LCIINFO *plcis;			/* logical channel information
					 * structure.
					 */
    UBYTE recv_send_seqnum;		/* received send sequence number */
    {
    
    /* If the P(S) which was received is a duplicate, return an error.
     * Otherwise, check if the P(S) is valid.
     */
    if (recv_send_seqnum == plcis->inpktsendseq)
	return(PS_DUPLICATE);

    /* make sure that the P(S) received (recv_send_seqnum) is
     * equal to the expected P(S).
     */
    else if (recv_send_seqnum != ((plcis->inpktsendseq + 1) % 16))
	return(PS_SEQNUM_ERROR);
    return(SUCCESS);
    }

/************************************************************************
 * BOOL valid_recvackseq(plcis, ppkt)
 *    LCIINFO *plcis;	- pointer to logical channel information structure
 *    BUFLET *ppkt;	- pointer to buflet packet
 *
 *    valid_recvackseq ensures that the incoming packet acknowledge sequence 
 *    number P(R) is within the window.
 *
 * Returns:  valid_recvackseq returns YES if the P(R) received is
 *    within the window.  Otherwise, valid_recvackseq returns NO.
 *
 ************************************************************************/
BOOL valid_recvackseq(plcis, ppkt)
    FAST LCIINFO *plcis;		/* logical channel information
					 * structure.
					 */
    BUFLET *ppkt;			/* pointer to buflet chain */
    {
    UBYTE ackseq;			/* acknowledge sequence number */
    
    /* Extract the P(R) from the incomming packet.
     */
    ackseq = (ppkt->bufdata[SEQ_NUM] & EXTRACT_ACKSEQ) >> 4;
    
    /* The acknowledge sequence number must be greater than or equal 
     * to the last P(R) received (inpktrecvseq) and less than or equal 
     * to the P(S) of the next packet to transmit (outpktsendseq).
     */
    if (plcis->inpktrecvseq < plcis->outpktsendseq)
	{
	if (ackseq >= plcis->inpktrecvseq && ackseq <= plcis->outpktsendseq)
	    return (YES);
	}
    else if (ackseq >= plcis->inpktrecvseq || ackseq <= plcis->outpktsendseq)
	return (YES);
    return (NO);
    }
    
/************************************************************************
 * VOID update_recvseq(plcis)
 *    LCIINFO *plcis;   - pointer to logical channel information structure
 *
 *    update_recvseq() updates the P(S) counter which is stored in
 *    the logical channel information structure.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID update_recvseq(plcis)
    LCIINFO *plcis;			/* logical channel information
					 * structure.
					 */
    {
    
    /* Update to the next input send sequence number.
     */    
    plcis->inpktsendseq = ++plcis->inpktsendseq % 16;
    }

/************************************************************************
 * VOID process_recvackseq(plcis, ppkt)
 *    LCIINFO *plcis;	- pointer to logical channel information structrue
 *    BUFLET *ppkt;	- pointer to incoming packet
 *
 *    process_recvackseq processes the incoming packet's P(R).
 *    The incoming packet's P(R) is used to determine which 
 *    packets have been acknowledged by the host.  All packets
 *    in the waiting to be acknowledged queue which have
 *    a P(S) less than the incoming P(R) are acknowledged ( they
 *    are removed from the waiting to be acknowledged queue).
 *    The incoming packets's P(R) has been validated before
 *    process_recvackseq is called.
 *   
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID process_recvackseq(plcis, ppkt)
    FAST LCIINFO *plcis;		/* logical channel information
					 * structure.
					 */
    BUFLET *ppkt;			/* pointer to buflet packet */
    {
    BUFLET *pb;				/* pointer to buflet chain */
    UBYTE ackseq;			/* acknowledge sequence number */

    
    /* Extract the P(R) from the input packet.
     */
    ackseq = (ppkt->bufdata[SEQ_NUM] & EXTRACT_ACKSEQ) >> 4;
    
    /* Acknowledge any packets in the waiting to be acknowledged queue
     * which have a P(S) which is less than the P(R) in the input
     * packet.
     */
    while (plcis->inpktrecvseq != ackseq && 
       plcis->waitackqueue.begqueue != NULLBUF)
	{
	if ((pb = get_queue(&plcis->waitackqueue)) != NULLBUF)
	    {
	    if (plcis->inpktrecvseq != 
		(pb->bufdata[SEQ_NUM] & EXTRACT_SENDSEQ))
		program_error = PS_SEQNUM_ERROR;
	
	    /* If this is a data packet then increment the number
	     * of data slots which are available for output.
	     */
	    else if ((pb->bufdata[GFI_LCI] & CBIT) == 0)
		++plcis->outdcount;
	    
	    /* If it is ok to free the packet, free the packet.  The
	     * packet which was acknowledged may not be able to be
	     * freed at this time because it was retransmitted and
	     * the retransmission may not have completed.  If this is
	     * the case, then a flag in the iocomm structure is set
	     * so that it will be freed when iocomm has finished
	     * transmitting the packet.
	     */
	    if (ok_to_free(pb))
		free_buf(pb);
	    
	    /* Increment the expected the P(R).
	     */
	    plcis->inpktrecvseq = ++plcis->inpktrecvseq % 16;
	    }
	}

    /* Reset the bounds for the output window.
     */
    if (ackseq != plcis->outwindlow)
	{
	plcis->outwindlow = ackseq;
	plcis->outwindhigh = (ackseq + WINDOW_SIZE - 1) % 16;
	
	/* If this is not channel zero and there are no packets in the
	 * waiting to be acknowledged queue, stop the timer.  Otherwise,
	 * start the timer.
	 */
	if (linkchnl != 0)
	    {
	    if (plcis->waitackqueue.begqueue == NULLBUF)
		stop_timer(ONE_SEC_TIMER, TIM_T25, linkchnl);
	    else
		start_timer(ONE_SEC_TIMER, TIM_T25, linkchnl,
		    (len_mult * LEN_T25),  tim_t25);
	    }
	}
    }

/************************************************************************
 * BOOL is_in_window(plcis, ppkt)
 *    LCIINFO *plcis;	- pointer to logical channel information structure
 *    BUFLET *ppkt;	- pointer to packet
 *
 *    is_in_window() checks if receive sequence number (P(S)) for the
 *    incoming packet is in the window.  
 *
 * Returns:  If the incoming packet is in the queue, is_in_window()
 *    returns YES.  Otherwise, is_in_window() returns NO.
 *
 ************************************************************************/
BOOL is_in_window(plcis, ppkt)
    LCIINFO *plcis;			/* logical channel information
					 * structure.
					 */
    BUFLET *ppkt;			/* pointer to packet */
    {
    UBYTE recvseq;			/* receive sequence number */

    recvseq = ppkt->bufdata[SEQ_NUM] & EXTRACT_SENDSEQ;
    
    /* Check for packet in window if lower window bound is less than
     * the upper window bound.
     */
    if (plcis->inwindlow < plcis->inwindhigh)
	{
	if (recvseq < plcis->inwindlow || recvseq > plcis->inwindhigh)
	    return (NO);
	}
    else if (recvseq < plcis->inwindlow && recvseq > plcis->inwindhigh)
	return (NO);
	
    /* If this is a data packet then decrement the number of
     */
    if ((ppkt->bufdata[GFI_LCI] & QBIT) == 0)
	{
	if (plcis->indcount == 0)
	    return (NO);
	else
	    --plcis->indcount;
	}
    return (YES);
    }
    

/************************************************************************
 * VOID update_sendseq(plcis)
 *    LCIINFO *plcis;	- pointer to logical channel information structure
 *
 *    update_sendseq updates the send sequence number.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID  update_sendseq(plcis)
    LCIINFO *plcis;			/* logical channel information
					 * structure.
					 */
    {
    
    plcis->outpktsendseq = ++plcis->outpktsendseq % 16;
    }

/************************************************************************
 * BOOL need_out_rr(plcis)
 *    LCIINFO *plcis;	- pointer to logical channel information structure
 *
 *    need_out_rr() checks if it is necessary to output the RR
 *    control packet.  If there are data  packets in the queue which
 *    will acknowledge packets (they have to be in the window), then
 *    the RR packet does not have to be outputted.  If the other side 
 *    is not accepting data packets (state is F2), then the RR packet 
 *    must be outputted.
 *
 * Returns:  If the packet must be outputted, need_out_pkt returns
 *    YES.  Otherwise, need_out_rr() returns NO.
 *
 ************************************************************************/

BOOL need_out_rr(plcis)
    LCIINFO *plcis;			/* pointer to logical channel 
    					 * information structure.
    					 */
    {
    
    /* If the current output sequence number is in the window then
     * the RR packet does not have to be outputted.
     */
    if (plcis->outwindlow < plcis->outwindhigh)
	{
	if (plcis->outpktsendseq < plcis->outwindlow ||
	    plcis->outpktsendseq > plcis->outwindhigh)
	    return (YES);
	}
    else if (plcis->outpktsendseq < plcis->outwindlow && 
	plcis->outpktsendseq > plcis->outwindhigh)
	return (YES);
    if (plcis->outdcount == 0)
	return(YES);
    
    /* If no data packets are being sent, then the RR packet must be
     * outputted.
     */
    if (plcis->dxeflowstate == F2)
       return(YES);
    return(NO);
    }


/************************************************************************
 * INT get_sendseq(plcis, gfi)
 *    LCIINFO *plcis;   - pointer to logical channel information structure
 *    UBYTE gfi;	- GFI_LCI byte of output packet
 *
 *    get_sendseq() checks that the P(S) which is going to be used for
 *    the transmit packet is within the current output window.
 *    
 *
 * Returns:  If the output send sequence number is in the window, 
 *   it is returned.  Otherwise, get_sendseq returns a -1.
 *
 ************************************************************************/
INT get_sendseq(plcis, gfi)
    LCIINFO *plcis;			/* logical channel information
				 	 * structure.
				 	 */
    UBYTE gfi;				/* logical channel number */
    {

    if (linkchnl != 0)
	{
	
	/* Check if the output send sequence number is within the output 
	 * window.   If the send sequence number is not within the
         * output window, return -1.
	 */ 
	if (plcis->outwindlow < plcis->outwindhigh)
	    {
	    if (plcis->outpktsendseq < plcis->outwindlow ||
		plcis->outpktsendseq > plcis->outwindhigh)
		return (-1);
	    }
	else if (plcis->outpktsendseq < plcis->outwindlow && 
	    plcis->outpktsendseq > plcis->outwindhigh)
	    return(-1);
    
	/* If this is a data packet, and there already are a pre-determined
	 * number of data packets in the queue, return -1.
	 */
	if ((gfi & CBIT) == 0)
	    if (plcis->outdcount == 0)
		return (-1);
	    else
		--plcis->outdcount;
	}
    
    /* Return send sequence number.
     */
    return ((INT)plcis->outpktsendseq);
    }



/************************************************************************
 * INT get_sendackseq(plcis)
 *    LCIINFO *plcis;	- pointer to logical channel information structure
 *
 *    get_sendackseq() gets the send acknowledgment sequence number P(R)
 *    for a packet to be transmitted. The output packet
 *    transmit P(R) sequence number is extracted from the 
 *    logical channel information structure and the input window is
 *    rotated.
 *
 * Returns:  get_recvackseq() returns the send acknowledgement sequence
 *    number.
 *
 ************************************************************************/
INT get_sendackseq(plcis)
    FAST LCIINFO *plcis;		/* logical channel structure */
    {
    
    plcis->outpktrecvseq = (plcis->inpktsendseq + 1) % 16;
    plcis->inwindlow = plcis->outpktrecvseq;
    plcis->inwindhigh = (plcis->inwindlow + WINDOW_SIZE - 1) % 16;
    plcis->indcount = WINDOW_DATA;
    return ((INT)plcis->outpktrecvseq);
    }


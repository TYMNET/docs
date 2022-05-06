/************************************************************************
 * flowctrl.c - Process Incoming Flow Control and Data Packets
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    flowctrl.c contains the source modules flowctrl_pkt() and 
 *    data_pkt().   flowctrl_pkt() process flow control packets for
 *    channels 1-15.  data_pkt() processes incoming data packets.
 *    data packets include data and q-bit packets.
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

IMPORT BOOL is_in_window();		/* packet is in window */
IMPORT BOOL valid_recvackseq();		/* valid P(R) sequence number */
IMPORT BUFLET *alloc_buf();		/* allocate buflet chain */
IMPORT BUFLET *get_queue();		/* get buflet chain from queue */
IMPORT VOID add_queue();		/* add buflet chain to queue */
IMPORT VOID free_buf();			/* free buflet chain */
IMPORT VOID process_recvackseq();	/* process P(R) sequence */
IMPORT VOID reset_chnl();		/* reset channel */
IMPORT VOID reset_diag();		/* reset diagnostic */
IMPORT VOID send_link_cpkt();		/* send link control packet */
IMPORT VOID stop_timer();		/* stop timer */
IMPORT VOID update_recvseq();		/* update P(S) sequence */

IMPORT UBYTE linkchnl;			/* link channel number */
IMPORT UBYTE pkttype;			/* packet type */
IMPORT UWORD free_count;		/* number of free buflets */
IMPORT UWORD rnr_buflets_req;		/* number of buflets required before
					 * RNRing.
					 */
#ifdef DEBUG
IMPORT WORD diags[];			/* array of words used for debugging 
					 */
LOCAL WORD cnt  = 441;
#endif 
/************************************************************************
 * VOID flowctrl_pkt(plcis, ppkt)
 *    LCIINFO *plcis;	- pointer to logical channel information structure
 *    BUFLET *ppkt;	- pointer to incoming packet
 *
 *    flowctrl_pkt() processes flow control packets.
 *    flowctrl_pkt() is called to process RR, RNR and REJECT
 *    packets for channels 1-15.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID flowctrl_pkt(plcis, ppkt)
    FAST LCIINFO *plcis;	        /* logical channel information
				 	 * structure.
				 	 */
    BUFLET *ppkt;			/* incoming packet */
    {
    BUFLET *pb;				/* pointer to buflet chain */
    BUFLET *endctrl;			/* pointer to last control packet 
		                         * in waiting to be acknowledged
					 * queue.
    					 */
    UWORD num_link;			/* number of new links for 
    					 * link output queue.
    					 */

    /* If the channel is being reset, the free the packet.
     */
    if (plcis->resetstate == D2)
	free_buf(ppkt);
	
    /* If the input packet P(R) is not valid then reset the channel.
     */
    else if (!valid_recvackseq(plcis, ppkt))
	reset_diag(ppkt, DIAG2);

    else
	{
	
	/* Free all of the packets in the waiting to be acknowledged
	 * queue which have a P(S) which is less than the P(R) of the
	 * incoming packet.
	 */
	process_recvackseq(plcis, ppkt);

	/* If packet is an RR, update the DXE flow state to receive ready (F1). 
	 * The DXE flow state of F1 indicates that the other side is
	 * currently able to receive packets.
	 */
	if (pkttype == RR)
	    {
	    plcis->dxeflowstate = F1;
	    free_buf(ppkt);
	    }
	
	/* If packet is RNR, update the DXE flow state to receive not ready 
	 * (F2). No packets are sent until DXE flow state goes back 
	 * to receive ready (F1). The state is changed back to F1 when 
	 * an RR or a REJECT packet is received.
	 */	
	else if (pkttype == RNR)
	    {
	    plcis->dxeflowstate = F2;
	    free_buf(ppkt);
	    }
	    
	/* If control packet is REJECT then set DXE flow state to receive 
         * ready.  Move all of the data packets in the waiting to be 
         * acknowledged queue to  the beginning of the  pad output queue 
         * and move all control packets to the beginning of the link 
         * output queue.
	 */
	else if (pkttype == REJECT)
	    {
	    plcis->dxeflowstate = F1;
	    
	    /* If there are packets in the waiting to be acknowledged queue,
	     * then move the control packets to the beginning of the link
	     * output queue and the data packets to the beginning of the
	     * pad output queue.  The only control packets which
	     * are put in the waiting to be acknowledged queue are
	     * reset packets, and reset confirmation packets.  When 
	     * a channel is reset all of the data is deleted from
	     * the waiting to be acknowledged queue.  Therefore,
	     * when the first data packet is found, all of the rest
	     * of the packets in the queue must be data packets.
	     */
	    if (plcis->waitackqueue.begqueue != NULLBUF)
		{
		num_link = 0;
		endctrl = NULLBUF;
		pb = plcis->waitackqueue.begqueue;
		
		/* Process until there are no more packets in waiting
		 * to be acknowledged queue.
		 */
		while (pb != NULLBUF)
		    {
		
		    /* If the current packet is a control packet,
		     * save it's pointer as the end of control packets
		     * and increment the number of control packets.
		     */
		    if (pb->bufdata[GFI_LCI] & CBIT)
			{
			endctrl = pb;
			pb = pb->chainnext;
			++num_link;
			}
		
		    /* This is the beginning of the data packets. Insert the
		     * data packets into beginning of pad output queue.
		     */
		    else
			{
			plcis->waitackqueue.endqueue->chainnext =
			    plcis->outpktqueue.begqueue;
			plcis->outpktqueue.begqueue = pb;
			if (plcis->outpktqueue.endqueue == NULLBUF)
			    plcis->outpktqueue.endqueue = 
			    plcis->waitackqueue.endqueue;
			plcis->outpktqueue.nchains += 
			    (plcis->waitackqueue.nchains - num_link);
			break;
			}
		    }
	    
		/* If there were control packets, then move them
		 * to the beginning of the link output queue.
		 */
		if (endctrl)
		    {
		    endctrl->chainnext = plcis->linkoutqueue.begqueue;
		    plcis->linkoutqueue.begqueue = 
			plcis->waitackqueue.begqueue;
		    if (plcis->linkoutqueue.endqueue == NULLBUF)
			plcis->linkoutqueue.endqueue = endctrl;
		    plcis->linkoutqueue.nchains += num_link;
		    }
			
		/* clear out the pointers to the waiting to
		 * be acknowledged queue.
		 */
		plcis->waitackqueue.begqueue = 
		    plcis->waitackqueue.endqueue = NULLBUF;
		plcis->waitackqueue.nchains = 0;
	
		/* Update the output P(S) and the number of data packets
		 * which can be outputted in current window.  Since, there
	         * are no data packets waiting to be acknowledged,
	         * outdcount is set to a maximum value.
		 */
		plcis->outpktsendseq = ((ppkt->bufdata[SEQ_NUM] 
		    & EXTRACT_ACKSEQ) >> 4);
		plcis->outdcount = WINDOW_DATA;
		free_buf(ppkt);
		}
	    else
		send_link_cpkt(ppkt, RR, (UBYTE *)0, (UBYTE *)0, 0);
	    
	    }
	}
    }

/************************************************************************
 * VOID data_pkt(plcis, ppkt)
 *    LCIINFO *plcis;   - pointer to logical channel information structure
 *    BUFLET *ppkt;	- pointer to incoming packet
 *
 *    data_pkt() processes incoming data packets. All packets which do
 *    not have the CBIT set (0x80) in the GFI_LCI byte of the packet
 *    header, are considered data packets.  If the incoming packets
 *    P(R) is valid, then any acknowledged packets are removed from
 *    the waiting to be acknowledged queue.  If the incoming packet
 *    is within the window, it is added to the pad input queue.
 *    If the incoming packet is not within the window, the channel is
 *    reset.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID data_pkt(plcis, ppkt)
    FAST LCIINFO *plcis;	        /* logical channel information
				 	 * structure.
				 	 */
    BUFLET *ppkt;			/* incoming packet */
    {
    BUFLET *pb;				/* pointer to buflet chain */
    

    /* If the input packets P(R) is not valid, then reset the channel
     */
    if (!valid_recvackseq(plcis, ppkt ))
	reset_diag(ppkt, DIAG2);
    else
	{

	/* Update the P(S) which is expected in the next incoming packet.
	 */
	update_recvseq(plcis);
	
	/* Acknowledge all packets in the waiting to be acknowledged 
	 * queue which have a P(S) less than the incoming
	 * packets P(R).
	 */
	process_recvackseq(plcis, ppkt);
	
	/* If the P(S) in the incoming packet is within the current
	 * window then add the packet to the link input queue.
	 */
	if (is_in_window(plcis, ppkt))
	    {
	    
	    /* Since a valid data packet has been received, clear the
	     * clear rnr timer.
	     */
	    stop_timer(ONE_SEC_TIMER, TIM_RRRNR, linkchnl);
#ifdef DEBUG
	    if ((ppkt->bufdata[GFI_LCI] & QBIT) == 0)
		{
		if (diags[cnt] > 30000)
		    cnt++;
		diags[cnt] = diags[cnt] + ppkt->bufdata[FRAME_LEN] + 1;
		}
#endif
	    
	    /* Add data packet to the pad input queue.
	     */
	    add_queue(&plcis->inpktqueue, ppkt);

   
	    if ((plcis->dteflowstate == G2 && 
                free_count < (rnr_buflets_req + BUFLET_HIWATER)) ||
                (plcis->dteflowstate == G1 && 
		free_count < rnr_buflets_req))
		{
		if (plcis->dteflowstate == G1  && plcis->appchnl > 0)
		    rnr_buflets_req -= RNR_CHNL_BUFLETS;
		pb = alloc_buf(1);
		send_link_cpkt(pb, RNR, (UBYTE *)0, (UBYTE *)0, 0);
		plcis->dteflowstate = G2;
		return;
		}
	    
	    
	    /* If the link output queue is empty, then put an RR
	     * acknowledgement packet into the link output queue.
	     * Whether or not this packet is outputted is determined
	     * by link_output().
	     */
	    else if (plcis->linkoutqueue.begqueue == NULLBUF  ||
		plcis->dteflowstate == G2) 
		{
		if (plcis->dteflowstate == G2)
		    {
		    plcis->dteflowstate = G1;
		    if (plcis->appchnl > 0)
			rnr_buflets_req += RNR_CHNL_BUFLETS;
		    }
		pb = alloc_buf(1);
		send_link_cpkt(pb, RR, (UBYTE *)0, (UBYTE *)0, 0);
		}
	    }
    
	/* The input packet is not within the window.
	 * Reset the channel.
	 */
	else
	    reset_diag(ppkt, DIAG1);
	}
    }
	    


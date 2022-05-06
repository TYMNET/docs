/************************************************************************
 * linkout.c - Process Outgoing Link Packets
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    linkout.c contains the source module link_output() which
 *    is called to output link control and link data packets.
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
#include "iocomm.h"
#include "link.h"
#include "state.h"
#include "timer.h"
#include "pkt.h"

IMPORT BOOL need_out_rr();		/* have to output rr */
IMPORT BUFLET *get_queue();		/* get buflet chain off queue */
IMPORT COMMINFO comm_info;		/* communications information
					 * structure.
					 */
IMPORT CRC get_crc();			/* get the crc */
IMPORT INT get_sendackseq();		/* get acknowledge sequence 
					 * number.
					 */
IMPORT INT get_sendseq();		/* get send sequeunce number */
IMPORT INT retransmit_pkt();		/* transmit packet again */
IMPORT VOID add_queue();		/* add to queue */
IMPORT VOID enable_xmt();		/* kicks iocomm transmitter */
IMPORT VOID free_buf();			/* free buflet chain */
IMPORT VOID int_disable();		/* diable interrupts */
IMPORT VOID int_enable();		/* enable interrupts */
IMPORT VOID start_timer();		/* start timer */
IMPORT VOID tim_rrrnr();		/* clear RNR timer */
IMPORT VOID tim_t25();			/* timer for t25 */
IMPORT VOID update_sendseq();		/* update send sequence number */

IMPORT BOOL clear_pad;			/* flag indicating that restart 
					 * is to be done and the pad
					 * queues have not been cleared
					 * yet.
					 */
#ifdef DEBUG	    
LOCAL WORD cnt = 461;			/* number of characters written */
#endif	    
IMPORT LCIINFO lcis[];			/* logical channel information
					 * structure.
					 */
IMPORT UBYTE linkchnl;			/* link channel number */
IMPORT WORD len_mult;			/* multiplier for T25 and T27 timers */
#ifdef DEBUG
IMPORT UWORD free_count;		/* number of free buflets */
IMPORT WORD diags[];			/* diagnostics buffer */
#endif

IMPORT UWORD xmt_intcnt;		/* Count of ticks since last transmit
					 * interrupt was processed
					 */

IMPORT WORD pcnt;			/* plink called counter */

LOCAL BOOL control_flag = NO;		/* packet is a control packet
					 * (RR, RNR, or REJECT)
					 */
LOCAL UBYTE gotpkt = 0;			/* flag which is set when packet
					 * is outputted.
					 */
/************************************************************************
 * INT release_pkt(plcis, pq, ppkt, retrans)
 *    LCIINFO *plcis    - pointer to logical channel information structure
 *    QUEUE *pq;	- pointer to queue which holds the packet to be
 *                        output.
 *    BUFLET *ppkt;	- pointer to incoming link packet
 *    BOOL retrans;	- flag which is used to determine whether packet
 *			  should be read from the queue, or is a
 *			  packet to retransmit.
 *
 *    release_pkt() releases the packet to iocomm's transmit queue.  All 
 *    slots in the iocomm transmit queue, which point to packets which have 
 *    been transmitted, are cleared and if the iocomm flag is set the packet
 *    is freed.  If there is a free iocomm transmit slot, the send and 
 *    acknowledge sequence numbers are set up and the packet is added to 
 *    the iocomm transmit queue. The iocomm flag is set for packets which 
 *    are not going to be acknowledged (packets are not  added to the 
 *    waiting to be acknowledged queue).
 *
 * Returns:  
 *    SEQNUM_IN_ERROR - Send Sequence Number is in Error
 *    QUEUE_IS_FULL - IOCOMM queue is full, packet was not added to
 *        queue.
 *    QUEUE_NOT_FULL - IOCOMM queue is not full, send sequence number is
 *        good, and packet was added to the IOCOMM queue.
 *
 ************************************************************************/
INT release_pkt(plcis, pq, ppkt, retrans)
    LCIINFO *plcis;			/* pointer to logical channel
    					 * information structure.
    					 */
    QUEUE *pq;				/* pointer to queue from which
					 * packet may be extracted.
					 */
    BUFLET *ppkt;			/* pointer to packet that is
					 * to be added to the iocomm queue when
					 * the transmit queue is
					 * not full.
					 */
    BOOL retrans;			/* Type of packet being processed.
		   			 * If the flag is on, the packet 
		    			 * is being retransmitted. 
					 */
    {
    CRC crc;				/* crc for packet header */
    UBYTE ackseq;			/* acknowledge sequence no */
    UWORD len;				/* length of extra data in packet */
    UWORD x;				/* maximum number of buflets
					 * in transmit queue.
					 */
    WORD sendseq;			/* send sequence number */
    
    
#ifdef DEBUG
    diags[304] = pcnt;
    diags[305] = free_count;
#endif
   
    /* Scan the iocomm transmit queue, high to low, until an untransmitted
     * entry is found.  If there are no available entries in the 
     * iocomm transmit queue, return QUEUE_IS_FULL.
     */
    x = XMT_BUF_CHAINS;
    while (x-- && !comm_info.xmtlen[x])
	;
    
    /* If there is a free iocomm transmit queue entry, then 
     * set up the send and acknowledge sequence numbers and
     * add packet to the iocomm transmit queue.
     */
    if (++x < XMT_BUF_CHAINS)
	{
#ifdef DEBUG
        diags[306] = pcnt;
        diags[307] = free_count;
#endif
        
	/* If the packet is not being retransmitted, get the send sequence
	 * number and read the packet from the specified queue.
	 * When a packet is retransmitted it is left in the waiting
	 * to be acknowledged queue and it's send sequence number is
	 * extracted from the packet.
	 */
	if (!retrans)
	    {
	    if (!control_flag)
		{
		/* get the P(S) sequence number for an output packet.
		 */
		if ((sendseq = get_sendseq(plcis, ppkt->bufdata[GFI_LCI])) < 0)
		    return (SEQNUM_IN_ERROR);
#ifdef DEBUG
		if ((ppkt->bufdata[GFI_LCI] & (CBIT | QBIT)) == 0)
		    {
		    if (diags[cnt] > 30000)
			++cnt;
		    diags[cnt] = diags[cnt] + ppkt->bufdata[FRAME_LEN] + 1;
		    }
#endif
		}
	    
	    
	    /* RR, RNR and REJECT packets do not have a send sequeuence number,
	     * they are never acknowledged.
	     */
	    else
		sendseq = 0;

	    /* Read packet from the specified queue. pq points to the link 
	     * output queue, or the pad output queue.
	     */
	    (VOID)get_queue(pq);
 	    }

	/* Get the send sequence number for packets which are being 
	 * retransmitted.
	 */
	else
	    sendseq = (WORD)(ppkt->bufdata[SEQ_NUM] & EXTRACT_SENDSEQ);
	
	/* Set up the acknowledge sequence number for the packet.
	 */
	ackseq = (UBYTE)get_sendackseq(plcis);
	
	/* Set a flag which indicates that at least one packet has been
	 * outputted.
	 */    
	gotpkt = 1;
	ppkt->bufdata[SEQ_NUM] = (UBYTE)sendseq | (ackseq << 4);

	/* Build the crc for the outgoing packet.  The CRC needs two
	 * bytes.  The first 8 bits of the CRC are stored in the first
	 * byte.  The second 8 bits of the CRC are stored in the second
	 * byte.
	 */
	crc = get_crc(ppkt, FRAME_LEN, (CRC1 - FRAME_LEN));
	ppkt->bufdata[CRC1] = (UBYTE)(crc >> 8);
	ppkt->bufdata[CRC1 + 1] = (UBYTE)(crc & 0xff);
	len = (UWORD)EXTRA_DATA;
	
	/* Calculate the length of the output packet.  The length includes
	 * the header length, the number of bytest of extra data and
	 * the length of the extra data's crc.
	 */
	if (ppkt->bufdata[FRAME_LEN])
	    len += (ppkt->bufdata[FRAME_LEN] + 2);

	/* Increment the number of packets outputted.  This count is
	 * used for link statistics.
	 */
	++comm_info.linkstats[PKTS_SENT];
	
	/* Scan the iocomm transmit queue, high to low, until an untransmitted
	 * entry is found (indicated by a non-zero transmit length). If a
	 * transmitted buflet chain contained an RR, RNR or REJECT packet
	 * free the transmitted buflet chain.
	 */
	x = XMT_BUF_CHAINS;
	int_disable();
	while (x-- && !comm_info.xmtlen[x])
	
	    /* The packet has been sent.  If it's packet pointer is not
	     * cleared, clear the pointer and if the packet is not going
	     * to be acknowledged (packet was not added to the waiting 
	     * to be acknowledged queue), then free the buflet. 
	     * xmtflg is set to yes for all packets which are not added 
	     * to the waiting to be acknowledged queue.  
	     */
	    if (comm_info.xmtptr[x])
		{
		if (comm_info.xmtflg[x])
		    free_buf(comm_info.xmtptr[x]);
		comm_info.xmtflg[x] = NO;
		comm_info.xmtptr[x] = NULLBUF;
		}

	/* Add the packet into the iocomm transmit queue.  If the packet is
	 * not going to be acknowledged then set the xmtflg entry to YES.
	 */		
	comm_info.xmtptr[++x] = ppkt;
	comm_info.xmtlen[x] = len;
	
	if (control_flag)
	    comm_info.xmtflg[x] = YES;
	int_enable();		/* enable interrupts */
	
	/* enable transmission if not already going...
	 */
	enable_xmt();
        
	/* If the packet has to be added to the waiting to be acknowledged
	 * queue, add the packet to the waiting to be acknowledged queue
	 * and update the send sequence number.
	 */
	if (!retrans)
	    if (!control_flag)
		{
		add_queue(&plcis->waitackqueue, ppkt);
		update_sendseq(plcis);
		}
	return(QUEUE_NOT_FULL);
	}
    return (QUEUE_IS_FULL);
    }

/************************************************************************
 * VOID link_output()
 *
 *    link_output() moves packets from the link output queue and
 *    the pad output queue to iocomm's transmit queue.  Packets
 *    are added to the iocomm queue until the queue is full.
 *    The packets are transmitted in the following order: channel
 *    0's link output packets, any packets which have to be retransmitted
 *    for channel zero, channel 1-15 link output packets, any packets
 *    which have to be retransmitted for channels 1-15, and pad
 *    data packets for channels 1-15.  The output of packets for
 *    channels 1 thru 15 is handled like a round robin ( one packet
 *    for channel 1 then 1 packet for channel 2 etc.).
 *    Processing stops when the queues are empty or the iocomm queue
 *    is full.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID link_output()
    {
    BOOL first_packet;			/* waiting to be acknowledged queue
    					 * is empty.
    					 */
    BUFLET *pb;				/* pointer to buflet */
    INT ret;				/* return value */
    UBYTE type;				/* type of packet */
    FAST BUFLET *ppkt;			/* pointer to buflet chain */
    LOCAL UBYTE ctrl_index = 1;		/* index for control index */
    LOCAL UBYTE data_index = 1;		/* index for data packets */
	
    
#ifdef DEBUG
    diags[302] = pcnt;
    diags[303] = free_count;
#endif
    
    /* Increment ticks since last transmit interrupt, then
     * enable transmission if not already active
     */
    ++xmt_intcnt;
    enable_xmt();
        
    /* process until there are no more packets to output or
     * iocomm cannot accept more packets.
     */
    linkchnl = 0;

    /* Output all of the link output packets for channel 0.
     */
     FOREVER
        {
	if ((ppkt = lcis[linkchnl].linkoutqueue.begqueue) != NULLBUF)
	    {

	    type = lcis[linkchnl].linkoutqueue.begqueue->bufdata[PKT_TYP_ID];
	    control_flag = (type == RR || type == RNR  || type == REJECT);
	    
	    /* If the packet doesn't need to be outputted, then 
	     * remove packet from queue and free packet.  The
	     * only time this would occur would be when an
	     * RR packet is being transmitted and
	     * there are more packets to output which can
	     * be used as an acknowledgement.  The RR packet will
             * always be outputted if an RNR state is being 
             * exitted.
	     */
	    if (type == RR && !lcis[linkchnl].rnrtransmitted)
		
		if (lcis[linkchnl].linkoutqueue.nchains > 1 ||
		 (lcis[linkchnl].outpktqueue.begqueue != NULLBUF  &&
		  !(need_out_rr(&lcis[linkchnl]))))
		    {
		    pb = get_queue(&lcis[linkchnl].linkoutqueue);
		    free_buf(pb);
		    continue;
		    }
	    
	    /* Add packet to iocomm queue.
	     */
	    if ((ret = release_pkt(&lcis[linkchnl], 
		&lcis[linkchnl].linkoutqueue, ppkt, NO)) == QUEUE_IS_FULL)
		return;
	    
	    /* If the packet was added to the IOCOMM queue, then set
	     * or clear RNR transmitted flag.
	     */
	    else if (!ret)
		{

		/* The packet was moved to the iocomm transmit queue.
		 * If the packet was an RNR, set RNR transmitted flag.
		 */
		if (type == RNR)
		    lcis[linkchnl].rnrtransmitted = YES;

		/* If the last packet sent was an RNR, then clear
		 * the RNR transmitted flag and start a timer.  The
                 * timer sends RR packets until a valid data packet is
                 * received.
		 */
		else if (type == RR && lcis[linkchnl].rnrtransmitted)
		    {
		    lcis[linkchnl].rnrtransmitted = NO;
		    start_timer(ONE_SEC_TIMER, TIM_RRRNR, linkchnl,
			LEN_RRRNR, tim_rrrnr);
		    }
		continue;
		}
	    }
	break;
	}
    
    control_flag = NO;
	    
    /* If iocomm transmit queue is not full, and there is a packet to be
     * retransmitted, retransmit the last packet in the waiting to
     * be acknowledged queue.  The packet is not removed from the
     * queue, it is just resent.
     */
    if (lcis[linkchnl].retransmitdata)
	{
	if (lcis[linkchnl].waitackqueue.begqueue)
	    if ((ret = release_pkt(&lcis[linkchnl], &lcis[linkchnl].waitackqueue,
		lcis[linkchnl].waitackqueue.endqueue,  YES)) 
		== QUEUE_IS_FULL)
		return;
	lcis[linkchnl].retransmitdata = NO;
	}
    
    /* Output packets from the link output queue for channels 1 thru 15.
     */
    FOREVER
	{
	linkchnl = ctrl_index;
	gotpkt = 0;
	    
	/* Starting at the last channel processed, output one packet
	 * per channel until all of the control packets for every
	 * channel have been output or IOCOMM'S transmit queue is full.
	 */
	do
	    {
	
	    /* If there is a packet to retransmit, retransmit the
	     * last packet from the waiting to be acknowledged queue.
	     */
	    if (lcis[linkchnl].retransmitdata)
		{
		if (lcis[linkchnl].waitackqueue.begqueue)
		    {
		    
		    /* No control packets are put into the waiting to
		     * be acknowledged queue, therefore control_flag
		     * must be turned off.
		     */
		    control_flag = NO;
		    
		    /* Release packet to iocomm transmit queue.
		     */
		    if ((ret = release_pkt(&lcis[linkchnl], 
			&lcis[linkchnl].waitackqueue, 
			lcis[linkchnl].waitackqueue.endqueue,  
			YES)) == QUEUE_IS_FULL)
			{
			ctrl_index = linkchnl;
			return;
			}
		    }
		lcis[linkchnl].retransmitdata = NO;
		}

	    /* Transmit packets for the link output queue.
	     */
	    if ((ppkt = lcis[linkchnl].linkoutqueue.begqueue) != NULLBUF)
		{
		type = lcis[linkchnl].linkoutqueue.begqueue->bufdata[PKT_TYP_ID];
		control_flag = (type == RR || type == RNR  || type == REJECT);
	
		/* If the packet doesn't need to be outputted, then 
		 * remove packet from queue and free packet.  The
		 * only time this would occur would be when an
		 * RR packet is being transmitted and an RNR 
		 * has not previously been transmitted and
		 * there are more packets to output which will
		 * be used as an acknowledgement.
		 */
		if (type == RR && !lcis[linkchnl].rnrtransmitted &&
		    (lcis[linkchnl].linkoutqueue.nchains > 1 ||
		    (lcis[linkchnl].outpktqueue.begqueue != NULLBUF  &&
		    !(need_out_rr(&lcis[linkchnl])))))
		    {
		    pb = get_queue(&lcis[linkchnl].linkoutqueue);
		    free_buf(pb);
		    }
		
		/* Add packet to IOCOMM transmit queue.
		 */
		else
		    {
		    if ((ret = release_pkt(&lcis[linkchnl], 
			&lcis[linkchnl].linkoutqueue,  ppkt, NO)) 
			== QUEUE_IS_FULL)
			{
			ctrl_index = linkchnl;
			return;
			}
		    
		    /* The packet was moved to the iocomm transmit queue.
		     * If the packet was an RNR, set RNR transmitted flag.
		     */
		    if (type == RNR)
			lcis[linkchnl].rnrtransmitted = YES;

		    /* If the last packet transmitted was an RNR, then clear
		     * the RNR transmitted flag and start a timer.  The timer
                     * sends RR packets until a data packet is received.
		     */
		    else if (type == RR && lcis[linkchnl].rnrtransmitted)
			{
			lcis[linkchnl].rnrtransmitted = NO;
			start_timer(ONE_SEC_TIMER, TIM_RRRNR, linkchnl,
			    LEN_RRRNR, tim_rrrnr);
			}    
		    }
		}
		
	    /* Process packet for next link output channel.
	     */
	    if (++linkchnl > MAX_CHNL)
		linkchnl = 1;
	    } while (linkchnl != ctrl_index);
	
	/* No packets were processed for any of the channels. So there
	 * are no link output packets to process.
	 */
	if (!gotpkt)
	    break;
	}
    
    /* Save current control channel number for next call to link_output.
     */

    ctrl_index = linkchnl;

    /* If the pad is not being cleared (done during a restart), then
     * output packets from the pad output queue.
     */
    if (!clear_pad || !lcis[0].resetreceived )
	{
	control_flag = NO;
	
	FOREVER
	    {
	    gotpkt = 0;
	    linkchnl = data_index;
	    
        
	    /* If the host is accepting packets, then transmit
	     * data packets for every channel.  
	     */
	    do
		{
		if (!lcis[linkchnl].resetreceived && 
		    lcis[linkchnl].dxeflowstate == F1 
		    && lcis[linkchnl].resetstate == D1 && 
		    lcis[0].restartstate == R1)
		    {

		    /* If the pad output queue is not empty and the packet
		     * is in the window then release the packet to iocomm
		     * and add packet to the waiting to be acknowledged queue.
		     */
		    if ((ppkt = lcis[linkchnl].outpktqueue.begqueue) 
			!= NULLBUF)
			{
			
			/* First packet is set if the waiting to be 
			 * queue is empty before the packet is sent.  
			 */
			first_packet = (lcis[linkchnl].waitackqueue.begqueue
			    == NULLBUF);

			/* If the iocomm queue is not full, then
			 * put the packet in the iocomm transmit queue.
			 */
			if ((ret = release_pkt(&lcis[linkchnl], 
			    &lcis[linkchnl].outpktqueue, ppkt, NO)) 
			    == QUEUE_IS_FULL)
			    {
			    data_index = linkchnl;
			    return;
			    }
			
			/* The pad packet was added to IOCOMM'S transmit 
			 * queue.
			 */
			else if (!ret)
			    {

			    /* If this is the first packet in the
			     * waiting to be acknowledged queue, start
			     * the window rotation timer.
			     */
			    if (first_packet)
				{
				lcis[linkchnl].r25trans = INIT_R25_COUNT;
				start_timer(ONE_SEC_TIMER, TIM_T25, linkchnl, 
				    (len_mult * LEN_T25), tim_t25);
				}
			    }
			}
		    }
		
		/* Update to next data channel number.
		 */
		if (++linkchnl > MAX_CHNL)
		    linkchnl = 1;
		} while (linkchnl != data_index);
	    
	    /* No packets were processed for any channel. Therefore 
	     * stop processing.
	     */
	    if (!gotpkt)
		break;
	    }
	    
	/* Save current data channel number for next call to link_output.
	 */
	data_index = linkchnl;
	}
    }

    
    


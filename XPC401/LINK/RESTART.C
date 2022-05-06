/************************************************************************
 * restart.c - Process Packets For Channel 0
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    restart.c contains the source module restart_state.  
 *    restart_state processes incoming packets which are 
 *    for channel 0. 
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
#include "link.h"
#include "timer.h"
#include "device.h"
#include "param.h"
#include "state.h"
#include "pkt.h"

#define ORIGINATED_FROM_DTE	0	/* restart originated from DTE */
#define ORIGINATED_FROM_DCE 	1	/* restart originated from DCE */

IMPORT BOOL valid_recvackseq();		/* valid P(R) sequence number */
IMPORT BUFLET *get_queue();		/* get buflet chain from queue */
IMPORT VOID add_queue();		/* add buflet chain to queue */
IMPORT VOID flowctrl_pkt();		/* process flow control packet */
IMPORT VOID free_buf();			/* free buffer */
IMPORT VOID reset_diag();		/* reset the channel and send
					 * RESET diagnostic packet.
					 */
IMPORT VOID reset_pkt();		/* process reset and reset confirm
					 * packets.
					 */
IMPORT VOID process_recvackseq();	/* process acknowledge sequence
					 * number (P(R).
					 */
IMPORT VOID reset_chnl();		/* reset channel */
IMPORT VOID do_restart();		/* restart device */
IMPORT VOID send_link_cpkt();		/* send control packet during
					 * link processing.
					 */
IMPORT VOID start_timer();		/* start a timer */
IMPORT VOID stop_timer();		/* stop a timer */
IMPORT VOID tim_rrchnl0();		/* timer interrupt for RR on 
					 * channel 0.
					 */
IMPORT VOID tim_t20();			/* timer interrrupt routine */
IMPORT VOID update_recvseq();		/* update P(S) sequence number */

IMPORT BOOL app_active;			/* application is active */
IMPORT BYTE restart_clear_code;		/* restart clear code */
IMPORT LCIINFO lcis[];			/* logical channel information 
					 * structure.
					 */
IMPORT PORTPARAMS port_params;		/* port parameter structure */
IMPORT UBYTE linkchnl;			/* link channel number */
IMPORT UBYTE pkttype;			/* packet type */
IMPORT UBYTE restart_devmode;		/* type of device for restart 
					 * (DTE or DCE)
					 */
#ifdef DEBUG
IMPORT UWORD free_count;		/* number of free buflets */
IMPORT WORD diags[];			/* used for debugging */
#endif

	   
/************************************************************************
 * LOCAL VOID not_chnl0(plcis, ppkt)
 *    LCIINFO *plcis;			pointer to logical channel
 *					 information structure.
 *					
 *    BUFLET *ppkt;			Pointer to Input Packet
 *
 *    Restart or Restart confirm was sent for channel other than 0.
 *    incoming packet.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID not_chnl0(plcis, ppkt)
    LCIINFO *plcis;			/* pointer to logical channel
    					 * information structure.
    					 */
    BUFLET *ppkt;			/* pointer to input packet */
    {
    
   /* If Channel is not being RESET, then RESET the channel and send
    * RESET packet back.
    */ 
    if (plcis->resetstate == D1)
	
	/* restart with non-zero channel number */
	reset_diag(ppkt, DIAG41);
    else
	free_buf(ppkt);	
    }

/************************************************************************
 * LOCAL BOOL valid_restart_cause()
 *
 *    valid_restart_cause() validates the restart cause from the
 *    incoming packet.
 *
 * Returns: valid_restart_cause always returns YES.
 *
 ************************************************************************/
LOCAL BOOL valid_restart_cause()
    {
    return(YES);
    }


/************************************************************************
 * LOCAL VOID restart_error(plcis, ppkt, d)
 *    LCIINFO *plcis;	- pointer to logical channel information structure
 *    BUFLET *ppkt;	- pointer to packet
 *    UBYTE d;		- diagnostic to be send.
 *
 *    restart_error() processes an erroneous restart packet.  If
 *    the current link state is packet ready, send a diagnostic
 *    packet and exit.  Otherwise, send a restart packet, and
 *    initialize the channel.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
LOCAL VOID restart_error(plcis, ppkt,  d)
    LCIINFO *plcis;			/* pointer to logical channel
    					 * information structure 
					 */
    BUFLET *ppkt;			/* pointer to packet */
    UBYTE d;				/* pointer to diagnostic code */
    {
    UBYTE diag = d;
    
    /* If link is in packet level ready and receives an erroneous
     * restart packet, a diagnostic packet is sent.
     */
    if (lcis[0].restartstate == R1)
	{
	send_link_cpkt(ppkt, DIAG, &diag, (UBYTE *)0, 0);
	return;
	}
    
    /* If link is in DTE restart request state, and an erroneous
     * restart packet is received, a restart diagnostic packet is
     * sent, and channel zero is initialized.
     */
    reset_chnl(plcis, linkchnl);
    send_link_cpkt(ppkt, RESTART, &restart_devmode, &diag, 1);
    plcis->r20trans = 0;
    stop_timer(ONE_SEC_TIMER, TIM_T20, linkchnl);
    start_timer(ONE_SEC_TIMER, TIM_T20, linkchnl, LEN_T20, tim_t20);
    }


/************************************************************************
 * LOCAL VOID restart_confirm_error(plcis, ppkt, d)
 *    LCIINFO *plcis;	- pointer to logical channel information structure
 *    BUFLET *ppkt;	- pointer to incoming packet 
 *    UBYTE d;		- diagnostic code for packet
 *
 *    restart_confirm_errors() sends a control packet when an
 *    erroneous restart_confirm packet is received.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
LOCAL VOID restart_confirm_error(ppkt, d)
    BUFLET *ppkt;			/* pointer to packet */
    UBYTE d;				/* pointer to diagnostic code */
    {
    UBYTE diag = d;
    
    /* If link is in packet ready state, then diagnostic packet
     * is response to invalid restart_comfirm packet.
     */
    if (lcis[0].restartstate == R1)
	send_link_cpkt(ppkt, DIAG, &diag, (UBYTE *)0, 0);
    
    /* If link is in restart state, then send another restart in
     * response to invalid restart_confirm packet.
     */
    else
	send_link_cpkt(ppkt, RESTART, &restart_devmode, &diag, 1);
    }


/************************************************************************
 * VOID restart_state(plcis, ppkt)
 *    LCIINFO *plcis;	- pointer to logical channel information
 *			  structure.
 *    BUFLET *ppkt;	- pointer to incoming packet.
 *    
 *    restart_state() processes incoming packets for channel
 *    0.  Packets are verified depending on the packet type.
 *    Packet processing is dependent upon the current state of
 *    the link and the incoming packet type.  
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID restart_state(plcis, ppkt)
    LCIINFO *plcis;			/* pointer to logical channel
    					 * information structure.
    					 */
    BUFLET *ppkt;			/* current packet pointer */
    {
    UBYTE restart_cause;		/* reason for restart */
    UBYTE diag2 = DIAG2;		/* Invalid P(R) */
    UBYTE diag17 = DIAG17;		/* Restart confirmation */
    UBYTE diag33 = DIAG33;		/* Packet type invalid
					 */
    UBYTE diag36 = DIAG36;		/* Packet type invalid for
					 * channel 0
					 */
    
    
    /* If the restart state of channel 0 is R0, then ignore all packets
     * until a RESTART packet is received.  The state R0 is set when
     * going to packet mode and the device is a DCE.  A RESTART packet
     * is not sent. However, all other packets are ignored until a 
     * RESTART packet is received.
     */
    if (lcis[0].restartstate == R0)
	if (pkttype != RESTART)
	    {
	    free_buf(ppkt);
	    return;
	    }
	    
    switch(pkttype)
	{
	
	/* packet received was a restart packet.
	 * Restart is valid in any mode, however, a restart can
	 * only be performed on channel zero.
	 */
	case RESTART:
	
	    /* Restart was received for a channel other than 0.
	     */
	    if (linkchnl != 0)
		not_chnl0(&lcis[linkchnl], ppkt);
	
	    /* validate restart packet.  The restart packet must have
	     * a restart cause which is valid. If there is a diagnostic
	     * with the restart cause, then the diagnostic must be valid.
	     */
	    else if (ppkt->bufdata[FRAME_LEN] < 1)
		restart_error(plcis, ppkt, DIAG38);
	    else if (!valid_restart_cause())
		restart_error(plcis, ppkt, DIAG81);
	    else if (ppkt->bufdata[FRAME_LEN] > 2)
		restart_error(plcis, ppkt, DIAG39);
	    
	    /* Restart packet is valid.
	     */
	    else
		{

		/* If the restart state is not packet level not ready, then
		 * restart the device and send a restart confirm packet.
		 */
		if (lcis[0].restartstate != R2)
		    {
	    
		    /* Restart the device.  If the application is
		     * not active, initialize the pad.  Otherwise,
		     * set a flag for xpc_interface to clear the
		     * pad before it exits.
		     */
		    restart_clear_code = PACKET_LEVEL_RESTART;
		    if (ppkt->bufdata[EXTRA_DATA + 1] == 0xf1)
		    	restart_clear_code = RESTART_END;
		    do_restart();
		
		    /* Update the expected input P(S).
		     */
		    update_recvseq(plcis);

		    /* Send a RESTART_CONFIRM packet.  Adjust the
		     * type of device depending on the input RESTART 
		     * cause.  Start the timer which will send an
		     * RR packet on channel 0 periodically.
		     */
		    send_link_cpkt(ppkt, RESTART_CONFIRM, (UBYTE *)0, 
			(UBYTE *)0, 0);
		    
		    /* If restart cause is originated from dte, then
		     * update port parameters mode.
		     */
		    if (ppkt->bufdata[EXTRA_DATA] == ORIGINATED_FROM_DTE)
			port_params.dxemode = DCE_MODE;
		    
		    /* Set restart state to packet level ready.
		     */
		    lcis[0].restartstate = R1;
		    
		    /* Start the keep alive timer.
		     */
		    start_timer(ONE_SEC_TIMER, TIM_RRCHANZERO, 0, LEN_RRCHANZERO,
			tim_rrchnl0);
		    }
	    
		/* Link is in DTE restart state (R2), free the restart packet.
		 * If the restart packet was sent from a DTE, then start
		 * the restart timer.  If the restart was received from a
		 * DCE then stop the restart timer (restart is treated as
		 * if it were a restart confirm ??? )
		 */
		else
		    {
	
		    /* Update the expected incoming P(S).
		    */
		    update_recvseq(plcis);
		    restart_cause = ppkt->bufdata[EXTRA_DATA];
		    free_buf(ppkt);
		    if (restart_cause == ORIGINATED_FROM_DTE)
			{
			plcis->r20trans = INIT_R20_COUNT + 1;
			start_timer(ONE_SEC_TIMER, TIM_T20, linkchnl, 
			    LEN_T20, tim_t20);
			}
		
		    /* Treat the RESTART packet as if it was a RESTART
		     * CONFIRM.  Stop the RESTART timer.  Start the
		     * keep alive timer and set restart state to 
		     * packet level ready.
		     */
		    else
			{
			stop_timer(ONE_SEC_TIMER, TIM_T20, linkchnl);
			start_timer(ONE_SEC_TIMER, TIM_RRCHANZERO, 0, LEN_RRCHANZERO,
			    tim_rrchnl0);
			lcis[0].restartstate = R1;
			}
		    }
		}
	    break;
	
	/* packet received was a restart confirm packet.  A restart
	 * confirm packet can only be received for channel zero.
	 */
	case RESTART_CONFIRM:
	    if (linkchnl != 0)
		not_chnl0(&lcis[linkchnl], ppkt);
	    
	    /* Validate the input packet P(R).
	     */
	    else if (!valid_recvackseq(plcis, ppkt))
		restart_confirm_error(ppkt, DIAG2);
	    
	    /* If the restart confirm packet has extra data, it is
	     * in error.
	     */
	    else if (ppkt->bufdata[FRAME_LEN] > 0)
		restart_confirm_error(ppkt, DIAG39);
	    else
		{
		
		/* Update the expected P(S).
		 */
		update_recvseq(plcis);
		
		/* make sure that if acknowledge sequence number is 0, 
		 * change it to 1 so that the restart will be removed
		 * from the  waiting to be acknowledged queue.
		 */
		
		if ((ppkt->bufdata[SEQ_NUM] & EXTRACT_ACKSEQ) == 0)
		    ppkt->bufdata[SEQ_NUM] |= 0x10;
		
		/* Acknowledge packets in the waiting to be acknowledged
		 * queue.
		 */
		process_recvackseq(plcis, ppkt);

		/* If the restart state is packet level ready, then
		 * restart the device and send a RESTART packet.
		 */
		if (lcis[0].restartstate == R1)
		    {
		    restart_clear_code = PACKET_LEVEL_RESTART;
		    do_restart();
		    
		    send_link_cpkt(ppkt, RESTART, &restart_devmode, 
			&diag17, 1);
		    plcis->r20trans = INIT_R20_COUNT;
		    start_timer(ONE_SEC_TIMER, TIM_T20, linkchnl, LEN_T20,
			tim_t20);
		    
		    /* DTE restart state */
		    lcis[0].restartstate = R2;	
		    }

		/* Link is in DTE restart state, stop the restart timer,
		 * send an acknowledge packet back, start the keep
		 * alive timer and change the state to packet level ready.
		 */
		else
		    {
		    stop_timer(ONE_SEC_TIMER, TIM_T20, linkchnl);
		    send_link_cpkt(ppkt, RR, (UBYTE *)0, (UBYTE *)0, 
			0);
		    start_timer(ONE_SEC_TIMER, TIM_RRCHANZERO, 0, LEN_RRCHANZERO,
		        tim_rrchnl0);
		    lcis[0].restartstate = R1;
		    }
		}
	    break;
	
	/* packet received was a diagnostic packet.  All diagnostic
	 * packets for channels 1-15 are discarded.
	 */
	case DIAG:
	    if (linkchnl != 0)
		free_buf(ppkt);
	    else
		{
		
		/* Validate the incoming P(R).
		 */
		if (!valid_recvackseq(plcis, ppkt))
		    send_link_cpkt(ppkt, DIAG, (UBYTE *)0, (UBYTE *)0, 0);
		else
		    {
		
		    /* Update the expected incoming P(S).
		     */
		    update_recvseq(plcis);
		    
		    /* Acknowledge packets in the waiting to be acknowledged
		     * queue.
		     */
		    process_recvackseq(plcis, ppkt);
		    
		    /* If there are no packets in the link output queue,
		     * put RR packet into the queue.  This packet will
		     * acknowledge the DIAGNOSTIC packet.
		     */
		    if (plcis->linkoutqueue.begqueue == NULLBUF &&
			lcis[0].restartstate == R1)
			send_link_cpkt(ppkt, RR, (UBYTE *)0, (UBYTE *)0,
			    0);
		    else
			free_buf(ppkt);
		    }
		}
	    break;
	    
	/* packet received was a RR or REJECT.  If packet channel
	 * is 1-15 then call flowctrl_pkt to process the
	 * flow control packet.
	 */
	case RR:
	case REJECT:
		    
	    /* If the RR or REJECT packet is not for channel 0, then
	     * call flowctrl_pkt().
	     */
	    if (linkchnl != 0)
		flowctrl_pkt(plcis, ppkt);
	    
	    /* If the current restart state is R2, then discard the
	     * flow control packet.
	     */
	    else if (lcis[0].restartstate == R2)
		free_buf(ppkt);
	    
	    /* Validate the input packet P(R).
	     */
	    else if (!valid_recvackseq(plcis, ppkt))
		send_link_cpkt(ppkt, DIAG, &diag2, (UBYTE *)0, 0);
	    else
		{
	    
		/* Acknowledge the packets in the waiting to be acknowledged
		 * queue.
		 */
		process_recvackseq(plcis, ppkt);
		free_buf(ppkt);
		
		/* If packet is a reject packet and there are packets
		 * in the waiting to be acknowledged queue, the
		 * packets are moved to the beginning of the link
		 * output queue.
		 */
		if (pkttype == REJECT)
		    {
		    if (plcis->waitackqueue.begqueue != NULLBUF)
			{
			plcis->waitackqueue.endqueue->chainnext 
			    = plcis->linkoutqueue.begqueue;
			plcis->linkoutqueue.begqueue = 
			    plcis->waitackqueue.begqueue;
			if (plcis->linkoutqueue.endqueue == NULLBUF)
			    plcis->linkoutqueue.endqueue = 
				plcis->waitackqueue.endqueue;
			plcis->linkoutqueue.nchains += 
			    plcis->waitackqueue.nchains;
			plcis->waitackqueue.begqueue = 
			    plcis->waitackqueue.endqueue = NULLBUF;
			plcis->waitackqueue.nchains = 0;
			}
		    }
		}
	    break;

	default:
		
	    /* Packet type is invalid for channel 0.
	     */
	    if (linkchnl == 0)
		send_link_cpkt(ppkt, DIAG, &diag36, (UBYTE *)0, 0);
	    
	    /* If not currently restarting and packet is RESET, or
	     * RESET CONFIRM, call reset_pkt.
	     * Flow control packets are processed by flowctrl_pkt().
	     */
	    else if (lcis[0].restartstate == R1)	
		{
		if (pkttype != RNR && pkttype != RESET && 
		    pkttype != RESET_CONFIRM)
		    send_link_cpkt(ppkt, DIAG, &diag33, (UBYTE *)0, 0);
		else
		    if (pkttype == RNR)
			flowctrl_pkt(plcis, ppkt);
		    else
			reset_pkt(plcis, ppkt);
		}
	    
	    /* If system is being restarted, then discard the packet.
	     */
	    else
		free_buf(ppkt);
	    break;
	}
    }

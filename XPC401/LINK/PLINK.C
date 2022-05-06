/************************************************************************
 * plink.c - Packet Mode Link Interface
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    plink.c contains the link interface module for packet mode (plink).  
 *    plink calls link_input() to move characters from IOCOMM'S 
 *    circular read buffer into packets and validate the input
 *    packets.  link_output() is called to output the
 *    link control packets and  pad data packets to IOCOMM's transmit
 *    queue.
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
#include "pkt.h"
#include "timer.h"
#include "state.h"

IMPORT BUFLET *alloc_buf();		/* allocate buflet */
IMPORT BUFLET *get_queue();		/* get packet from queue */
IMPORT VOID link_input();		/* process input packets to link */
IMPORT VOID link_output();		/* link output packet */
IMPORT VOID restart_device();		/* restart device */
IMPORT VOID send_link_cpkt();		/* send link control packet */
IMPORT VOID start_timer();		/* start the timer */
IMPORT VOID stop_timer();		/* stop the timer */
IMPORT VOID tim_t20();			/* timer interrupt */

IMPORT BOOL restart_flag;		/* restart flag */
IMPORT BYTE restart_clear_code;		/* restart clear code */
IMPORT LCIINFO lcis[];			/* logical channel information
					 * structure.
					 */
IMPORT UBYTE linkchnl;			/* link channel number */
IMPORT UBYTE restart_devmode;		/* type of device doing restart
					 * (DTE or DCE).
					 */
IMPORT UWORD free_count;		/* number of free buflets */
IMPORT UWORD rnr_buflets_req;		/* number of buflets required before
					 * RNRing.
					 */
#ifdef DEBUG
WORD pcnt = 0;				/* number of times plink was called */
IMPORT WORD diags[];			/* used for diagnostic purposes */
#endif

/************************************************************************
 * VOID p_link()
 *
 *    p_link is the entry point for the link process. Link is
 *    called via the link interrupt to build input
 *    packets and to output packets to the link.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID p_link()
    {
    BUFLET *ppkt;			/* pointer to packet */
    UBYTE diag = 0;			/* diagnostic for restart */
    
#ifdef DEBUG
    ++pcnt;
    diags[300] = pcnt;
    diags[301] = free_count;
#endif

    
    /* If a restart is being done by pad via the clear device or 
     * set packet state, then clear the restart flag and restart the
     * device before processing packets.
     */
    if (restart_flag)
	{
	restart_flag = NO;
	restart_clear_code = PACKET_LEVEL_RESTART;
	restart_device();
	
	/* Try to start the t20 timer.  If there are no packets available
	 * for the RESTART packet, then set the timeout value to a small
 	 * number.  When the timer times out, a RESTART packet will
         * be sent.
	 */
	if ((ppkt = alloc_buf(1)) == NULLBUF)
	    {
	    lcis[0].r20trans = 1;
	    start_timer(ONE_SEC_TIMER, TIM_T20, 0, LEN_INITT20, tim_t20);
	    }
	
	/* Buflet is available for the restart device packet.  Start the
	 * timer and send the packet.
	 */
	else
	    {
	    linkchnl = 0;
	    send_link_cpkt(ppkt, RESTART, &restart_devmode, &diag, 1);
	    lcis[linkchnl].r20trans = INIT_R20_COUNT;
	    start_timer(ONE_SEC_TIMER, TIM_T20, linkchnl, LEN_INITT20, tim_t20);
	    lcis[linkchnl].restartstate = R2;
	    }
	}
    
    /* If any of the channels which have changed from an RNR state
     * to an RR state, then update the dte flow state.  If during
     * a report I/O status function it is determined that a channel which is
     * currently not accepting input (dte flow state is G2), can now accept
     * packets, then the dte flow state is changed to G3.  plink adjusts
     * the flow state to G1 if there are enough free buflets available.
     *
     * As another way of getting out of flow control situations, if the
     * channel has NO packets in the incoming packet queue, for any reason,
     * and we are in G2 state, send an RR and go into G1 state.  This covers
     * all possible bets, allowing at least SOME packets to make it through
     * when we have multiple RNRs out.
     */
    for (linkchnl = 0; linkchnl <= MAX_CHNL; ++linkchnl)
	{
	if (lcis[linkchnl].dteflowstate == G3)
	    {
	    if (free_count > (rnr_buflets_req + RNR_CHNL_BUFLETS))
		{
	        ppkt = alloc_buf(1);
		send_link_cpkt(ppkt, RR, (UBYTE *)0, (UBYTE *)0, 0);
		lcis[linkchnl].dteflowstate = G1;  /* DTE receive ready */
		if (lcis[linkchnl].appchnl > 0)
		    rnr_buflets_req += RNR_CHNL_BUFLETS;
		}
	    else 
		lcis[linkchnl].dteflowstate = G2;
	    }
	if (lcis[linkchnl].dteflowstate == G2 &&
	    lcis[linkchnl].appchnl >= 0 &&
	    lcis[linkchnl].inpktqueue.begqueue == NULLBUF &&
	    (ppkt = alloc_buf(1)) != NULLBUF)
	    {
	    send_link_cpkt(ppkt, RR, (UBYTE *)0, (UBYTE *)0, 0);
	    lcis[linkchnl].dteflowstate = G1;  /* DTE receive ready */
	    if (lcis[linkchnl].appchnl > 0)
		rnr_buflets_req += RNR_CHNL_BUFLETS;
	    }
	}
    linkchnl = 0;

    /* Move packets from the link output queue and the pad output queue
     * to IOCOMM'S transmit queue.  Processing stops when the queue is
     * full or there are no more packets for output.
     */
    link_output();			/* process link output */
    
    /* Build and validate input packets.  The input packets are added
     * to the pad input queue.
     */
    link_input();			/* process link input */
    }



/************************************************************************
 * linktime.c - Link Timeout Functions
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    linktime.c contains the modules which are called to process
 *    timer interrupts for link.
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
#include "timer.h"
#include "state.h"
#include "pkt.h"
#include "link.h"
#include "iocomm.h"

IMPORT BUFLET *alloc_buf();		/* allocate new buffer */
IMPORT VOID do_restart();		/* performe restart when
				 	 * application interface may be
					 * active.
					 */
IMPORT VOID reset_chnl();		/* reset channel */
IMPORT VOID reset_diag();		/* reset diagnostice */
IMPORT VOID send_link_cpkt();		/* send link packet */
IMPORT VOID start_timer();		/* start the specified timer */
IMPORT VOID stop_timer();		/* stop the timer */

IMPORT BYTE restart_clear_code;		/* restart clear code */
IMPORT CHNLINFO cis[];			/* channel information structure */
IMPORT COMMINFO comm_info;		/* communications information 
					 * structure.
					 */

IMPORT LCIINFO lcis[];			/* logical channel information
					 * structure.
					 */
IMPORT UBYTE linkchnl;			/* channel number */
IMPORT UBYTE restart_devmode;		/* type of restart - now
					 * only DTE or DCE.
					 */
IMPORT WORD len_mult;			/* multiplication factor for
					 * timer interrupt.
					 */
#ifdef DEBUG
IMPORT UWORD free_count;		/* number for free buflets */
IMPORT WORD diags[];			/* used for debugging */
#endif

/************************************************************************
 * VOID tim_t20(ptim)
 *    TIMER *ptim;	- pointer to timer structure
 *
 *    tim_t20 is the restart request timer.  The T20 timer
 *    is started when a RESTART packet is added to the link output queue.
 *    An additional restart packet is sent each time the T20 timer
 *    times out.  The T20 timer is stopped when the restart is confirmed.
 *    The T20 timer is only used for channel 0.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID tim_t20(ptim)
    TIMER *ptim;			/* pointer to timer structure */
    {
    BUFLET *pb;				/* pointer to buflet chain */
    UBYTE diag52 = DIAG52;		/* diagnostic code */
    UBYTE diag = 0;

    linkchnl = idtochnl(ptim->timid);
#ifdef DEBUG
    diags[10] += 1;
#endif

    /* If the T20 retry count is not 0, then send another reset channel 0
     * and send another restart packet.
     */
    if (lcis[linkchnl].r20trans != 0)
	{
	
	/* If there are no buflets available for the restart packet,
	 * restart the timer.  Hopefully, a RESTART packet will be
	 * sent the next time the T20 timer times out.
	 */
	if ((pb = alloc_buf(1)) == NULLBUF)
	    ptim->timlength = LEN_T20;
	
	/* There is a buflet available for the RESTART packet.  Reset
	 * channel zero, send a RESTART packet and restart the T20 
	 * timer.
	 */
	else
	    {
	    reset_chnl(&lcis[linkchnl], linkchnl);
	    
	    if (lcis[linkchnl].r20trans >= INIT_R20_COUNT)
		send_link_cpkt(pb, RESTART, &restart_devmode, &diag, 1);
	    else
		send_link_cpkt(pb, RESTART, &restart_devmode, &diag52, 1);
	    --lcis[linkchnl].r20trans;
	    ptim->timlength = LEN_T20;
	    }
	}
    else
	stop_timer(ptim->timtype, idtoclass(ptim->timid), linkchnl);
    }
	    
/************************************************************************
 * VOID tim_t22(ptim)
 *    TIMER *ptim;	- pointer to timer structure
 *
 *    tim_t22 is the reset request timer interrupt.  The T22 timer is started
 *    when a channel is reset.  When a RESET_CONFIRM packet is received
 *    the timer is stopped.  When the T22 timer times out, another RESET 
 *    packet is sent for the specified channel.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID tim_t22(ptim)
    TIMER *ptim;
    {
    BUFLET *pb;				/* pointer to buflet chain */
    UBYTE reset_code = 0;
    
    linkchnl = idtochnl(ptim->timid);
#ifdef DEBUG
    diags[11] += 1;
#endif

    /* If there is no buflet available for the RESET or RESTART packet,
     * then restart the timer.  Hopefully, the next time the T22 timer
     * times out, there will be a buflet available.
     */
    if ((pb = alloc_buf(1)) == 0)
	ptim->timlength = LEN_T22;
    
    /* If the T22 retry count is not zero, then resend a RESET packet
     * to the specified channel, restart the timer, and decrement the
     * T22 retry count.
     */
    else if (lcis[linkchnl].r22trans != 0)
	{
	reset_chnl(&lcis[linkchnl], linkchnl);
	lcis[linkchnl].resetstate = D2;
	send_link_cpkt(pb, RESET, &reset_code, 0, 0);
	ptim->timlength = LEN_T22;
	--lcis[linkchnl].r22trans;
	}
    
    /* If the T22 retry count is zero, then RESTART the device.  The
     * T22 timer is stopped, the device is restarted, and the
     * T20 (device restart) timer is started.
     */
    else
	{
	stop_timer(ptim->timtype, idtoclass(ptim->timid), linkchnl);
	restart_clear_code = PACKET_LEVEL_RESTART;
	do_restart();
	lcis[0].r20trans = INIT_R20_COUNT;
	start_timer(ONE_SEC_TIMER, TIM_T20, 0, LEN_T20, tim_t20);
	lcis[0].restartstate = R2;
	}
    }
    
/************************************************************************
 * VOID tim_t25(ptim)
 *    TIMER *ptim;	- pointer to timer structure
 *
 *    tim_t25 is the window rotation timer interrupt.  It is started
 *    when the first packet is put into the waiting to be acknowledged
 *    queue.  After packets have been acknowledged, if the waiting to
 *    be acknowledged queue is empty, the t25 timer is stopped. 
 *    Otherwise, it is restarted.  tim_t25() sets a flag which will
 *    cause the retransmission of the last packet in the waiting to
 *    be acknowledged queue.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID tim_t25(ptim)
    TIMER *ptim;			/* pointer to timer structure */
    {
    BUFLET *pb;				/* pointer to buflet */
    
    linkchnl = idtochnl(ptim->timid);
#ifdef DEBUG
    diags[12] += 1;
#endif

    /* If the dxe flow state is Receive not ready then don't send
     * the packet. 
     */
    if (lcis[linkchnl].dxeflowstate == F2)
	ptim->timlength = (len_mult * LEN_T25);
    
    /* If the retry count is not zero, set the flag which will
     * tell link_output() to retransmit the packet, decrement
     * the retry count and restart the timer.
     */
    else if (lcis[linkchnl].r25trans != 0)
	{
	lcis[linkchnl].retransmitdata = YES;
	--lcis[linkchnl].r25trans;
	ptim->timlength = len_mult * LEN_T25;
	}
    
    /* The retry count is zero, so try to reset the channel.  If
     * there are no buflets available for the reset, then restart
     * the timer.  The reset will be done when buflets become
     * available.
     */
    else
	{
	if ((pb = alloc_buf(1)) == NULLBUF)
	    ptim->timlength = len_mult * LEN_T25;
	else
	    {
	    stop_timer(ptim->timtype, linkchnl, idtoclass(ptim->timid));
	    reset_diag(pb, DIAG0);
	    }
	}
    }

/************************************************************************
 * VOID tim_t27(ptim)
 *    TIMER *ptim;	- pointer to timer structure
 *
 *    tim_t27 is the reject response timer interrupt.  The t27 timer
 *    is started when a packet is rejected.  The timer is stopped
 *    when a valid packet is received. The reject response timer
 *    sends another reject packet.  When the t27 retry count becomes
 *    zero, the channel is reset.    
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID tim_t27(ptim)
    TIMER *ptim;			/* pointer to timer structure */
    {
    BUFLET *pb;				/* pointer to buflet */
    
    linkchnl= idtochnl(ptim->timid);
#ifdef DEBUG
    diags[13] += 1;
#endif

    /* If dte flow state is Receive Not Ready, then don't send
     * a REJECT packet.
     */
    if (lcis[linkchnl].dteflowstate == G2)
	ptim->timlength = (len_mult * LEN_T27);
    
    /* If the reject response retry count is not zero, then send another
     * reject packet and restart the timer.
     */
    if (lcis[linkchnl].r27trans != 0)
	{
	if ((pb = alloc_buf(1)) != NULLBUF)
	    {
   	    send_link_cpkt(pb, REJECT, (UBYTE *)0, (UBYTE *)0, 0);
	    --lcis[linkchnl].r27trans;
	    lcis[linkchnl].pktrejected = YES;
	    }
	ptim->timlength = len_mult * LEN_T27;
	}
    
    /* The reject response retry count is zero, then try to
     * reset the channel.  If there are no buflets available for
     * the channel reset, then restart the timer.  The channel reset 
     * will be done when buflets are available.
     */
    else
	if ((pb = alloc_buf(1)) == NULLBUF)
	    ptim->timlength = len_mult * LEN_T27;
	else
	    {
	    stop_timer(ptim->timtype, idtoclass(ptim->timid), linkchnl);
	    reset_diag(pb, DIAG0);
	    }
    }

LOCAL UBYTE num_times_called = 0;    		/* number of times tim_rrchnl0
						 * called.
						 */
LOCAL UWORD num_pkts_recvd = 0;			/* number of packets received 
						 */
/************************************************************************
 * VOID tim_rrchnl0(ptim)
 *    TIMER *ptim;	- pointer to timer structure
 *
 *    tim_rrchnl0 is the keep alive timer.  This timer is started when
 *    the device is restarted.  It sends an RR packet, at a specified
 *    time increment, for channel 0.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID tim_rrchnl0(ptim)
    TIMER *ptim;			/* pointer to timer structure */
    {
    BUFLET *pb;				/* pointer to buflet */

    if (num_pkts_recvd == comm_info.linkstats[PKTS_RECVD])
	{
	if (++num_times_called == 4)
	    {
	    cis[0].chnlstate = CHNL_CALL_CLEARED;
	    cis[0].clearcode = REMOTE_XPC_LOST;
	    }
	}
    else
	num_times_called = 0;
    num_pkts_recvd = comm_info.linkstats[PKTS_RECVD];

    linkchnl = idtochnl(ptim->timid);
#ifdef DEBUG
    diags[14] += 1;
#endif
    if ((pb = alloc_buf(1)) != NULLBUF)
	send_link_cpkt(pb, RR, (UBYTE *)0, (UBYTE *)0, 0);
    ptim->timlength = LEN_RRCHANZERO;
    }
    
/************************************************************************
 * VOID tim_rrnr(ptim)
 *    TIMER *ptim;	- pointer to timer structure
 *
 *    tim_rrrnr processes clear RNR timer.   The clear RNR timer is started
 *    when an RNR state has been cleared.  The clear RNR timer sends an
 *    RR packet.  The timer is stopped when a valid data packet is 
 *    received.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID tim_rrrnr(ptim)
    TIMER *ptim;			/* pointer to timer structure */
    {
    BUFLET *pb;				/* pointer to buflet */

    linkchnl = idtochnl(ptim->timid);
#ifdef DEBUG
    diags[15] += 1;
#endif
    
    /* Send an RR packet and restart the clear RNR timer.
     */
    if ((pb = alloc_buf(1)) != NULLBUF)
	send_link_cpkt(pb, RR, (UBYTE *)0, (UBYTE *)0, 0);
    ptim->timlength = LEN_RRRNR;
    }
    


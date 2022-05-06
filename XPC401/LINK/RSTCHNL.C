/************************************************************************
 * rstchnl.c - Reset Device, Reset Channel and Reset Diagnostics
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    rstchnl.c contains the source modules which are called to
 *    reset the device, to reset a particular channel or to
 *    perform a reset because of an error in packet transmission.
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

IMPORT BOOL ok_to_free();		/* check if ok to free the buflet
					 * chain.
					 */
IMPORT BUFLET *get_queue();		/* get buflet chain from queue */
IMPORT VOID free_buf();			/* free buflet chain */
IMPORT VOID init_pad();			/* initialize the pad */
IMPORT VOID init_link_chnl();		/* initialize lcis entry */
IMPORT VOID init_chnl_data();		/* initialize channel data */
IMPORT VOID init_window();		/* initialize window */
IMPORT VOID start_timer();		/* start timer */
IMPORT VOID stop_timer();		/* stop timer */
IMPORT VOID send_link_cpkt();		/* send link control packet */
IMPORT VOID tim_t22();			/* time out for t22 timer */

IMPORT BOOL app_active;			/* flag which is set true when the
					 * application interface is active.
					 */
IMPORT BOOL clear_pad;			/* flag which is set for the
				 	 * application interface when
					 * the pad queues have to be
					 * cleared.
					 */
IMPORT BOOL in_a_frame;			/* flag which when set indicates
					 * that a packet is currently being
					 * inputted.
					 */
IMPORT BUFLET *frame_pntr;		/* pointer to frame */
IMPORT LCIINFO lcis[];			/* logical channel information
					 * structure.
					 */
IMPORT UBYTE linkchnl;			/* current channel number being
					 * processed.
					 */
IMPORT UBYTE restart_devmode;		/* 0 or 1 depending on device 
					 * mode (dce or dte).
					 */
#ifdef DEBUG
IMPORT WORD diags[];			/* XXX */
#endif
LOCAL UBYTE do_initpad = YES;		/* Call to init_pad should be
					 * done by reset channel.
					 */
/************************************************************************
 * VOID reset_chnl(plcis, chnl)
 *    LCIINFO *plcis; 	- pointer to logical channel information structure
 *    UBYTE chnl;	- channel to be reset
 *
 *    reset_chnl() resets a channel for link.  All of the flags, queues
 *    counters and timers which are used by link are cleared.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID reset_chnl(plcis, chnl)
    FAST LCIINFO *plcis;		/* logical channel information
					 * structure.
					 */
    UBYTE chnl;				/* channel to reset */
    {
    FAST BUFLET *pb;			/* pointer to packet */


    /* clear the link output queue and the waiting to be acknowledged queue.
     */
    while ((pb = get_queue(&plcis->waitackqueue)) != NULLBUF)
	{
	if (ok_to_free(pb))
	    free_buf(pb);
	}
    while ((pb = get_queue(&plcis->linkoutqueue)) != NULLBUF)
	{
	if (ok_to_free(pb))
	    free_buf(pb);
	}
    
   /* initialize the window  and the logical channel
    * information for the specified channel.
    */
    init_window(plcis);
    
    /* clear link packet rejected, packet retransmitted and rnr
     * transmitted flags for channel.
     */
    plcis->pktrejected = NO;
    plcis->rnrtransmitted = NO;
    plcis->retransmitdata = NO;
    
    /* stop the timers for link.
     */
    stop_timer(ONE_SEC_TIMER, TIM_RRRNR, chnl);
    stop_timer(TEN_SEC_TIMER, TIM_T22, chnl);
    stop_timer(ONE_SEC_TIMER, TIM_T25, chnl);
    stop_timer(ONE_SEC_TIMER, TIM_T27, chnl);
    
    /* reset link states.
     */
    plcis->dxeflowstate = F1;	/* DXE receive ready */
    plcis->dteflowstate = G1;	/* DTE receive ready */
    plcis->resetstate = D1;
    
    /* If do_initpad is on then reset_chnl is not being called
     * by do_restart.  If reset_chnl is being called by do_restart,
     * the pad is initialized by do_restart.
     */
    if (do_initpad)
	if (plcis->appchnl >= 0)
	    {
	    if (app_active)
		{
		clear_pad = YES;
		plcis->resetreceived = YES;
		}
	    else
		init_pad(plcis->appchnl, YES, YES);
	    }
    }


/************************************************************************
 * VOID reset_diag(pb, diag)
 *    BUFLET *ppkt;	- pointer buflet for packet
 *    UBYTE diag;	- diagnostic for packet
 *
 *    reset_diag() resets the specified logical channel and sends
 *    a diagnostic control packet.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID reset_diag(ppkt, diag)
    BUFLET *ppkt;			/* pointer to packet */
    UWORD diag;				/* diagnostic for reset */
    {
    FAST LCIINFO *plcis;		/* pointer to logical channel 
					 * information structure.
					 */
    UBYTE reset_diag = (UBYTE)diag;	/* diagnostic moved to packet */
     
    
    /* reset the specified channel
     */
    plcis = &lcis[linkchnl];
    reset_chnl(plcis, linkchnl);
    
    /* send diagnostic control packet
     */
    send_link_cpkt(ppkt, RESET, &restart_devmode, &reset_diag, 1);

    /* Set reset state to Not Ready.
     */
    plcis->resetstate = (UBYTE)D2;
    
    /* initialize retransmission count 
     */
    plcis->r22trans = INIT_R22_COUNT;
    start_timer(TEN_SEC_TIMER, TIM_T22, linkchnl, LEN_T22, tim_t22);
    }

/************************************************************************
 * VOID restart_device()
 *
 *    restart_device() performs a restart on all of the channels.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID restart_device()
    {
    UBYTE chnl;
    
    /* Reset all of the channels.  Clear the in a frame flag.
     */
    for (chnl = 0; chnl <= MAX_CHNL; ++chnl)
	reset_chnl(&lcis[chnl], chnl);

    in_a_frame = NO;
    }

/************************************************************************
 * VOID do_restart()
 *    UBYTE cause;	- cause sent for restart
 *
 *    do_restart() is called to restart a device when application may
 *    be active.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID do_restart()
    {
    
    /* Restart the device.  If the application is not currently active,
     * clear the pad.  Otherwise, set a flag so that the application
     * will clear the queue.
     */
    do_initpad = NO;
    restart_device();
    if (app_active)
	
	/* Set the clear pad flag so that application will clear the
	 * pad when finished processing.
	 */
	{
	clear_pad = YES;
	lcis[0].resetreceived = YES;
	}
    else
	init_pad(-1, YES, YES);
    do_initpad = YES;
    }

		

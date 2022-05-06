/************************************************************************
 * initpad.c - Initialize Pad Queues and Counts
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    pktflush.c contains the source modules which are used to
 *    clear a session and flush the pad input and output queues.
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

IMPORT BUFLET *get_queue();                     /* get first packet in queue */
IMPORT VOID int_disable();			/* disable interrupts */
IMPORT VOID int_enable();			/* enable interrupts */
IMPORT VOID clear_ssn();			/* clear session during 
						 * restart.
						 */
IMPORT VOID adjust_time_len();			/* adjust length of time
    						 * between T25 and T27 
    						 * timer interrupts.
    						 */    

IMPORT VOID free_buf();                         /* free buflet chain */
IMPORT VOID stop_timer();			/* stop the timer */

IMPORT BYTE restart_clear_code;			/* clear code during restart */
IMPORT CHNLINFO cis[];			        /* channel information array */
IMPORT UWORD rnr_buflets_req;			/* number of buflets required
						 * before RNRing.
						 */
IMPORT WORD pend_inc_calls;			/* number of pending incoming
						 * calls.
						 */
/************************************************************************
 * VOID clear_ssn(chnl)
 *    UBYTE  chnl          - index of channel to initialize.  
 *
 *    clear_ssn() is called to clear a session for the
 *    specified channel.  This code applies only to channels 
 *    which have established a session.  This routine is called when
 *    a session is being cleared.  It clears everything for the
 *    session except for the pad input and output queues.  These
 *    queues are needed to send and receive session clear packets.
 *    They are cleared by init_pad when the channel goes from a 
 *    call cleared state to a disconnnected state.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID clear_ssn(chnl)
    UBYTE chnl;				/* logical channel number */
    {
    BUFLET *pb;                         /* pointer to buflet chain */
    
    /* Stop the timers which may have been started for session.
     */
    if (cis[chnl].chnlstate == CHNL_PEND_CALL_ACP)
	stop_timer(TEN_SEC_TIMER, TIM_T21, cis[chnl].logicalchnl);
    else if (cis[chnl].chnlstate == CHNL_PEND_CLEAR)
	stop_timer(SIXTH_SEC_TIMER, TIM_CLRCONFIRM, cis[chnl].logicalchnl);
    else if (cis[chnl].chnlstate == CHNL_CONNECTED)
	{
	stop_timer(SIXTH_SEC_TIMER, TIM_TYMNOACTIV, cis[chnl].logicalchnl);
	stop_timer(ONE_SEC_TIMER, TIM_BALLOUT, cis[chnl].logicalchnl);
	stop_timer(SIXTH_SEC_TIMER, TIM_FORWARDING, cis[chnl].logicalchnl);
	}

    /* Clear the application read queue,  the number of bytes in the 
     * application read queue and the index into the application
     * read queue.
     */
    while((pb = get_queue(&cis[chnl].readqueue)) != NULLBUF)
	free_buf(pb);
    cis[chnl].nreadbytes = 0;
    cis[chnl].idxreadqueue = MOVED_FIRST_BYTE;

    /* Clear the pointers to the echo packet.  The echo packet
     * is part of the application read queue, therefore, it's buflet
     * has already been freed.
     */
    cis[chnl].echopkt = 0;
    cis[chnl].idxechodata = MOVED_FIRST_BYTE;

    /* Clear pointer to session data and the index into
     * the session data packet.
     */
    if (cis[chnl].ssndatapkt)
	free_buf(cis[chnl].ssndatapkt);
    cis[chnl].idxssndata = EXTRA_DATA;
    cis[chnl].ssndatapkt = NULLBUF;

    /* Clear pointer to the transmit assembly packet and the number of
     * bytes in the transmit assembly packet.  
     */
    if (cis[chnl].assemblypkt)
	free_buf(cis[chnl].assemblypkt);
    cis[chnl].nwritebytes = 0;

    /* Clear the output flow controlled flag.
     */
    cis[chnl].holdassembly = NO;
    cis[chnl].assemblypkt = NULLBUF;

    /* Clear break status.
     */
    cis[chnl].breakstatus = 0;
    cis[chnl].tymechostate = T0;	/* tymnet echo off */
    cis[chnl].mciechostate = M0;	/* MCI echo off */
    
    /* Clear the check point event.
     */
    cis[chnl].checkevent = 0;
    cis[chnl].checkeventoff = cis[chnl].checkeventseg = 0;
    }


/************************************************************************
 * VOID init_pad(chnl, clear_padqueue, in_restart)
 *    INT chnl       - index of channel to initialize.  
 *    BOOL clear_padqueue - clear outgoing pad queue
 *    BOOL in_restart - pad being initialized because there has been a
 *                      a restart.

 *
 *    init_pad() is called to disconnect an application channel
 *    from a logical channel.  All of the pad queues are flushed,
 *    and the channel is disconnected.  If init_pad is called with
 *    a negative number, all of the channels are disconnected.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID init_pad(chnl, clear_padqueue, in_restart)
    INT chnl;				        /* channel number */
    BOOL clear_padqueue;			/* clear outgoing pad queue */
    BOOL in_restart;				/* doing a restart */
    {
    INT i, j, k;				/* counters */
    BUFLET *pb;                                 /* pointer to buflet chain */
    
    /* Establish the start and end channel indexes.  If chnl is less than
     * zero then channels 0 thru MAX_CHNL are processed.
     */
    j = 0;
    k = MAX_CHNL;
    if (chnl >= 0)
        j = k = chnl;
    
    /* Process all of the channels specified by the calling routine.
     */
    for (i = j; i <= k; ++i)
	{

	    
	/* If in restart state then clear the session and set up
	 * the channel cleared codes.
	 */
	if (in_restart && cis[i].lcistruct)
	    {
	    clear_ssn((UBYTE)i);
	    if (cis[i].chnlstate == CHNL_PEND_RECV_CALL)
	    	--pend_inc_calls;
	    cis[i].chnlstate = CHNL_CALL_CLEARED;
	    if (i == 0)
		cis[i].clearcode = restart_clear_code;
	    else
		cis[i].clearcode = RESET_PACKET;
	    }
	
	/* Set the channel state to disconnected.
	 */
	else if (i > 0)
	    cis[i].chnlstate = CHNL_DISCONNECTED;
    	
	/* Clear the timeout values for a session clear which has
	 * been started, and flush the pad queues.
	 */
	if (cis[i].lcistruct)
	    {
	    cis[i].lcistruct->ssncleartime = 0;
	    cis[i].lcistruct->ssnclearcode = 0;
	    cis[i].lcistruct->ssnstate = S1;
	    cis[i].lcistruct->resetreceived = NO;
	    
	    /* Clear the pad input queue.
	     */
	    while((pb = get_queue(&cis[i].lcistruct->inpktqueue)) != NULLBUF) 
		free_buf(pb);
	
	    /* Clear the pad output queue.
	     */
	    if (clear_padqueue)
		{
		while((pb = get_queue(&cis[i].lcistruct->outpktqueue)) 
		    != NULLBUF)
		    free_buf(pb);
		}
	    
	    /* Disconnect channel.
	     */
	    if (i > 0)
		{
		int_disable();
		if (cis[i].lcistruct->dteflowstate == G1)
		    rnr_buflets_req -= RNR_CHNL_BUFLETS;
		cis[i].lcistruct->appchnl = -1;	
		int_enable();
		cis[i].lcistruct = (LCIINFO *)0;
		}
	    }
	/* Disconnect the current channel.
	 */
	cis[i].logicalchnl = 0;
	}
    
    /* If clearing all channels, then clear number of channels waiting
     * for call.
     */
    if (chnl < 0)
	pend_inc_calls = 0;

    /* adjust length of time in between T25 and T27 interrupts.
     */
    adjust_time_len();
    }


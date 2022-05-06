/************************************************************************
 * timeout.c - Timeout Subroutines
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    timeout.c contains the subroutines which are called when
 *    timers timeout.  tim_noact() is called when the no activity
 *    timeout occurs.  tim_ballout() is called when colored balls out
 *    timer expires; tim_forwarding() is called when the forwarding
 *    character times out. tim_clrconf() is called when the
 *    pending clear confirm timer times out. 
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
#include "padprm.h"
#include "timer.h"
#include "error.h"
 
IMPORT INT link_xmit_pkt();		/* link xmit packet to output queue,
					 * error if queue is full.
					 */ 
IMPORT INT send_ctrl_pkt();		/* send control packet */
IMPORT VOID exec_interrupt();		/* execute interrupt */
IMPORT VOID start_timer();		/* start timer */
IMPORT VOID stop_timer();		/* stop timer */
IMPORT VOID store_word();		/* store word in specified
					 * offset and segment
					 */
IMPORT VOID tim_noact();		/* no activity time out */
IMPORT VOID try_ssn_clear();		/* try to output a session clear
					 * packet.
					 */
IMPORT CHNLINFO cis[];			/* channel information struct*/
IMPORT LCIINFO lcis[];			/* logical channel information
    					 * structure.
    					 */
#ifdef DEBUG
IMPORT WORD diags[];
#endif
IMPORT UWORD free_count;

/************************************************************************
 * VOID tim_clrconf(ptim)
 *    TIMER *ptim;	- pointer to timer packet
 *
 *    tim_clrconf processes time out of pending clear confirm.  The
 *    timer is started when a session clear packet is outputted.
 *    tim_clrconf() sends another session clear packet.  The timer
 *    is stopped when the session clear confirm is received or the
 *    clear confirm timer times out.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID tim_clrconf(ptim)
    TIMER *ptim;			/* pointer to timer structure */
    {
    FAST CHNLINFO *pcis;		/* pointer to channel information
    					 * structure.
    					 */
    UBYTE chnl;				/* channel number */

#ifdef DEBUG    
    diags[16] += 1;
#endif
    
    /* Stop the clear confirm timer.
     */
    stop_timer(SIXTH_SEC_TIMER, TIM_CLRCONFIRM, 
	(chnl = idtochnl(ptim->timid)));
 
    /* If the pending clear confirm timer timed out because driver
     * is waiting for buflet to become available for session clear,
     * then try again to send session clear.
     */
    if (lcis[chnl].appchnl >= 0 && lcis[chnl].ssncleartime)
	try_ssn_clear(&lcis[chnl], chnl, lcis[chnl].ssncleartime ,
	    lcis[chnl].ssnclearcode);
    else
	{
	
	/* If a session has been established then save clear code and
	 * channel state.
	 */
	if (lcis[chnl].appchnl >= 0)
	    {
	    pcis = &cis[lcis[chnl].appchnl];
	    pcis->clearcode = (BYTE)CONFIRM_TIMEOUT;
	    pcis->chnlstate = CHNL_CALL_CLEARED;
	    lcis[chnl].ssnstate = S7;
	    }
	/* Update the session state.
	 */
	else
	    lcis[chnl].ssnstate = S1;	/* session xxx */
	}
    }


/************************************************************************
 * VOID tim_forwarding(ptim)
 *    TIMER *ptim;	- pointer to timer structure
 *
 *    tim_forwarding() turns off the forwarding character timer,
 *    and attempts to add the current assembly packet to the output
 *    queue.   
 *
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID tim_forwarding(ptim)
    TIMER *ptim;			/* pointer to time structure */
    {
    BOOL prev_holdassembly;		/* previous hold assembly */
    CHNLINFO *pcis;			/* pointer to channel information
    					 * structure.
    					 */
    UBYTE chnl;				/* channel number */
    

#ifdef DEBUG
    diags[17] += 1;
#endif
  
    /* Stop the forwarding character time out.
     */
    stop_timer(SIXTH_SEC_TIMER, TIM_FORWARDING, 
	(chnl = idtochnl(ptim->timid)));
    pcis = &cis[lcis[chnl].appchnl];
    
    /* If there is a transmit assembly packet to link, try to link
     * the transmit assembly packet.  If no attempt was made to link
     * the transmit assembly packet before the forwarding character
     * timer was started and we are unable to link the transmit
     * assembly packet, clear the hold assembly packet and restart
     * the forwarding character timer.  This allows for the case where
     * the transmit assembly packet is not full but the forwarding
     * character timer was started.
     */
    if (pcis->nwritebytes > 0)
	{
	prev_holdassembly = pcis->holdassembly;
	(VOID)link_xmit_pkt(pcis, NO);
	if (pcis->holdassembly && !prev_holdassembly)
	    {
	    pcis->holdassembly = NO;
	    start_timer(SIXTH_SEC_TIMER, TIM_FORWARDING, chnl, 
		pcis->padparams[FWD_TIME], tim_forwarding);
	    }
	}
    }
 
/************************************************************************
 * VOID tim_ballout(ptim)
 *    TIMER *ptim;	- pointer to timer structure
 *
 *    tim_ballout() is called when the colored ball out timer expires.
 *    The colored ball timer is turned off and a red ball packet is
 *    sent to the host.  Set the tymnet echo state to red ball out and
 *    restart the ball out timer. 
 *
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID tim_ballout(ptim)
    TIMER *ptim;			/* pointer to time structure */
    {
    FAST CHNLINFO *pcis;	 	/* pointer to channel information
    					 * structure.
    					 */
    UBYTE chnl;				/* channel number */
    
#ifdef DEBUG
    diags[18] += 1;
#endif
    
    /* Stop the ball out timer and send a red ball packet.
     */
    stop_timer(ONE_SEC_TIMER, TIM_BALLOUT, 
	(chnl = idtochnl(ptim->timid)));
    (VOID)send_ctrl_pkt(&lcis[chnl], chnl, RED_BALL, (UBYTE *)0, MUST_LINK);
    pcis = &cis[lcis[chnl].appchnl];
/*

    Originally no activity timer started when in RED Ball out state.
    Now it is not started.
    
    if (pcis->tymechostate == T2)	
	{
	start_timer(SIXTH_SEC_TIMER, TIM_TYMNOACTIV, chnl,
	    LEN_TYMNOACTIV, tim_noact);
	pcis->tymechostate = T3;	
	}
 */
    
    pcis->tymechostate = T3;	/* RBO status */
    start_timer(ONE_SEC_TIMER, TIM_BALLOUT, chnl, LEN_BALLOUT, tim_ballout);
    }
 
/************************************************************************
 * VOID tim_noact(ptim)
 *    TIMER *ptim;	- pointer to timer structure
 *
 *    tim_noact is called when the no activity timer times out.
 *    The no activity timer is started when deferred echo mode
 *    is initiated.  
 *    If a red ball has been sent to the host the ball out timer
 *    is turned off.  tim_noact() sends a green ball to the host,
 *    sets the state to green ball out, and starts the colored ball
 *    out timer.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID tim_noact(ptim)
    TIMER *ptim;			/* pointer to time structure */
    {
    FAST CHNLINFO *pcis;		/* pointer to channel information
    					 * structure.
    					 */
    UBYTE chnl;				/* channel number */
    
#ifdef DEBUG
    diags[19] += 1;
#endif

    /* Stop the no activity timer.
     */
    stop_timer(SIXTH_SEC_TIMER, TIM_TYMNOACTIV, 
	(chnl = idtochnl(ptim->timid)));
    pcis = &cis[lcis[chnl].appchnl];
	
    /* If the tymnet echo state is red ball out, then stop the ball out
     * timer.
     */
    if (pcis->tymechostate == T3)	/* RBO status */
	stop_timer(ONE_SEC_TIMER, TIM_BALLOUT, chnl);
    
    /* Send a GREEN ball packet and set state to green ball out.
     */
    (VOID)send_ctrl_pkt(&lcis[chnl], chnl,  GREEN_BALL, (UBYTE *)0, 
	CHECK_LINK);
    pcis->tymechostate = T2;	        /* GBO status */
    
    /* Start the ball out timer.
     */
    start_timer(ONE_SEC_TIMER, TIM_BALLOUT, chnl, LEN_BALLOUT,
	tim_ballout);
    }


/************************************************************************
 * VOID tim_t21(ptim)
 *    TIMER *ptim;	- pointer to timer structure
 *
 *    tim_t21 processes call request timeouts.  The T21 timer is started
 *    when a session request is sent.  It is stopped when the t21 timer
 *    timers out, a session accept is received or a session is cleared.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID tim_t21(ptim)
    TIMER *ptim;			/* pointer to time structure */
    {
    BYTE clear_code = (BYTE)INVALID_PAD_SIGNAL;   /* clear code */
    FAST CHNLINFO *pcis;		/* pointer to channel information
    					 * structure.
					 */
    UBYTE chnl;				/* channel number */
    WORD pend_clrtime = 0;		/* pending clear confirm ?? */
    
#ifdef DEBUG
    diags[20] += 1;
#endif
    
    /* Stop the T21 timer and clear the channel.
     */
    stop_timer(TEN_SEC_TIMER, TIM_T21, (chnl = idtochnl(ptim->timid)));
    pcis = &cis[lcis[chnl].appchnl];
    pcis->chnlstate = CHNL_PEND_CLEAR;
    pcis->lcistruct->ssnstate = S6;
    try_ssn_clear(pcis->lcistruct, chnl, pend_clrtime, clear_code);
    }
    
/************************************************************************
 * VOID tim_event(ptim)
 *    TIMER *ptim;	- pointer to timer structure
 *
 *    tim_event() processes the event timer.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID tim_event(ptim)
    TIMER *ptim;			    /* pointer to timer
    					     * structure.
    					     */
    {
    CHNLINFO *pcis;			    /* pointer to channel information
    					     * structure.
    					     */
    
#ifdef DEBUG
    diags[21] += 1;
#endif
    
    pcis = &cis[lcis[idtochnl(ptim->timid)].appchnl];
    if (pcis->timerevent != 0)
	{
	if (pcis->timerevent == UPDATE_ON)
	    store_word(pcis->timereventseg, pcis->timereventoff, 0);
	else
	    exec_interrupt(pcis->timereventseg, pcis->timereventoff);
	pcis->timerevent = 0;
	pcis->timereventseg = pcis->timereventoff = 0;
	}
    }
    

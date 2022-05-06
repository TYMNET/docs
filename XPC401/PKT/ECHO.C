/************************************************************************
 * echo.c - Turn Echo On and Off
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    echo.c contains the modules which are called to process incoming
 *    echo control packets.  echo_on() turns on tymnet echoing.  
 *    echo_off() turns tymnet echoing off.  perm_echo_on() turns on 
 *    MCI echoing,   and perm_echo_off() turns MCI echoing off. 
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

IMPORT INT link_xmit_pkt();	                /* link xmit packet to output 
					         * queue,  error if queue full.
				                 */
IMPORT INT send_cpkt();                         /* send control pkt */
IMPORT VOID free_buf();    		        /* free buflet chain */
IMPORT VOID int_disable();		        /* disable interrupts */
IMPORT VOID int_enable();                       /* enable interrupts */
IMPORT VOID pkt_in_error();                     /*  packet in error */
IMPORT VOID start_timer();                      /* start the timer */
IMPORT VOID stop_timer();                       /* stop the timer */
IMPORT VOID tim_ballout();			/* colored ballout timeout */
IMPORT VOID tim_noact();			/* no activity timeout */

IMPORT UBYTE log_chnl;				/* logical channel number */
#ifdef DEBUG
IMPORT WORD diags[];				/* used for debugging */
IMPORT UWORD free_count;			/* used for debugging */
#endif
    
/************************************************************************
 * VOID echo_on(pcis, ppkt)
 *    CHNLINFO *pcis    - pointer to channel information structure
 *    BUFLET *ppkt;	- pointer to incoming echo packet
 *
 *    echo_on() processes incoming echo enable packets.  The
 *    current tymnet echo state is updated.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID echo_on(pcis,  ppkt)
    FAST CHNLINFO *pcis;			/* pointer to channel 
						 * information structure.
						 */
    BUFLET *ppkt;                               /* pointer to packet */
    {

    /* Adjust tymet echo state upon receiving an echo on 
     * packet.
     */
    if (pcis->tymechostate != T0)               /* tymnet echo active */
        {
        if (pcis->tymechostate == T4)           /* tymet echo off */
	    {
	    pcis->tymechostate = T3;            /* RBO */
	    /*

	    Originally, the no activity timer was started  when echo is
	    off.  Now it is only started in T1 state.  We are waiting
	    for an answer to a question before code is deleted.
	    
	    start_timer(SIXTH_SEC_TIMER, TIM_TYMNOACTIV,
		log_chnl, LEN_TYMNOACTIV, tim_noact);
	     */
	    }
	}
    
    /* It is an error when enable tymnet echoing is received when
     * MCI echoing is active.  The session is cleared.
     */
    else if (pcis->mciechostate != M0)          /* MCI echo active */
	{
	pkt_in_error(pcis, ppkt);
	return;
	}
    
    /* If tymnet echoing is not active, activate tymnet echoing.
     */
    else
        {
        pcis->tymechostate = T1;                /* tymnet echo NBO */
        start_timer(SIXTH_SEC_TIMER, TIM_TYMNOACTIV, 
	    log_chnl, LEN_TYMNOACTIV, tim_noact);
        }
    free_buf(ppkt);
    }



/************************************************************************
 * VOID echo_off(pcis, ppkt)
 *    CHNLINFO *pcis    - pointer to channel information structure
 *    BUFLET *ppkt;	- pointer to incoming echo disable packet
 *
 *    echo_off() processed incoming disable tymnet echo packets.
 *    The tymnet echo state is updated depending on the
 *    current tymmet echo state.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID echo_off(pcis, ppkt)
    FAST CHNLINFO *pcis;			/* pointer to channel 
						 * information structure.
						 */
    BUFLET *ppkt;                               /* pointer to packet */
    {
    
    if (pcis->tymechostate != T0)               /* tymnet echo active */    
        {
        
	/* If there are no balls out, then disable tymnet echoing.
	 */
	int_disable();
        if (pcis->tymechostate == T1)           /* NBO state */
            {
            stop_timer(SIXTH_SEC_TIMER, TIM_TYMNOACTIV,
		log_chnl);
            int_enable();
	    pcis->tymechostate = T0;		/* inactivate tymnet echo */
	    free_buf(ppkt);
            }
	
	/* If tymnet echo state is green ball out, then 
         * send a red ball packet and restart ball out timer.
	 */
        else if (pcis->tymechostate == T2)      /* GBO mode */
            {
            stop_timer(ONE_SEC_TIMER, TIM_BALLOUT, log_chnl);
            int_enable();
	    (VOID)send_cpkt(pcis->lcistruct,RED_BALL, (UBYTE *)0,
		MUST_LINK, ppkt);
	    pcis->tymechostate = T4;		/* tymnet echo off */
            start_timer(ONE_SEC_TIMER, TIM_BALLOUT, log_chnl, 
                LEN_BALLOUT, tim_ballout);
            }
	
	/* If there is a red ball out, then set local echoing off.
	 */
	else if (pcis->tymechostate == T3)	/* RBO state */
	    {
	    /*

	    Originally, no activity timer was started when tymnet echoing
	    state was red ball out.  Now the no activity timer is only
	    started with no balls out.  We are waiting for answer to
	    question before code is deleted.
	    
	    stop_timer(SIXTH_SEC_TIMER, TIM_TYMNOACTIV, 
		log_chnl);
	     */
	    pcis->tymechostate = T4;
	    int_enable();
            free_buf(ppkt);
	    }
	
        else
            {
            int_enable();
	    if (pcis->tymechostate == T5)  /* tymnet echo on */
		{
		
		/* If tymnet echoing is currently active, then
		 * link the current transmit assembly packet and
		 * disable tymnet echoing.
		 */
		if (pcis->nwritebytes)
		    {
		    stop_timer(SIXTH_SEC_TIMER, TIM_FORWARDING,
			log_chnl);
		    (VOID)link_xmit_pkt(pcis, YES);
		    }
		(VOID)send_cpkt(pcis->lcistruct, ENTER_DEFER_ECHO, 
		    (UBYTE *)0, MUST_LINK, ppkt);
		pcis->tymechostate = T0;	/* inactivate tymnet echo */
                }
            else
		free_buf(ppkt);
            }
        }
    else
        free_buf(ppkt);
    }

/************************************************************************
 * VOID perm_echo_on(pcis, ppkt)
 *    CHNLINFO *pcis    - pointer to channel information structure
 *    BUFLET *ppkt;	- pointer to incoming permanent echo enable packet
 *
 *    perm_echo_on() processes an enable permanent echo packet.  MCI
 *    echoing is turned on.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID perm_echo_on(pcis, ppkt)
    FAST CHNLINFO *pcis;			/* pointer to channel 
						 * information structure.
						 */
    BUFLET *ppkt;                               /* pointer to packet */
    {
 
    /*  If MCI echoing is already active, then clear the
     *  session.
     */
    if (pcis->mciechostate != M0)               /* MCI echo active */
        pkt_in_error(pcis, ppkt);
    
    /* Start MCI echoing.
     */
    else
	{
        pcis->mciechostate = M1;                /* MCI echo on state */
    	free_buf(ppkt);
        }
    }

/************************************************************************
 * VOID perm_echo_off(pcis, ppkt)
 *    FAST CHNLINFO *pcis - pointer to channel information structure
 *    BUFLET *ppkt;	- pointer to incoming permanent echo off packet
 *
 *    perm_echo_off() disables permanent echoing.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID perm_echo_off(pcis, ppkt)
    FAST CHNLINFO *pcis;			/* pointer to channel 
						 * information structure.
						 */
    BUFLET *ppkt;                               /* pointer to packet */
    {
 
    /* set MCI echo state to off
      */
    if (pcis->mciechostate != M0)               /* MCI echo on  state */
        {
        pcis->mciechostate = M0;                /* MCI echo off state */
        free_buf(ppkt);
    	}
    
    /* Clear session if MCI echoing is already off.
     */
    else
        pkt_in_error(pcis, ppkt);
    }
	    


	    
	    
	    
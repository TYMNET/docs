/************************************************************************
 * ball.c  - Driver Ball Handling Routines
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    ball.c contains the modules needed to process incoming red, green, 
 *    yellow and orange ball packets.
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
#include "timer.h"
#include "error.h"

IMPORT INT link_xmit_pkt();                 /* link xmit packet */
IMPORT INT send_cpkt();                     /* send control packet */
IMPORT VOID add_queue();                    /* add buflet to queue */
IMPORT VOID exec_interrupt();		    /* execute interrupt */
IMPORT VOID free_buf();			    /* free buflet chain */
IMPORT VOID int_disable();		    /* disable interrupts */
IMPORT VOID int_enable();		    /* enable interrupts */
IMPORT VOID pkt_in_error();  	            /* packet received when 
                                             * channel connected
                                             */
IMPORT VOID start_timer();                  /* start the timer */
IMPORT VOID stop_timer();                   /* stop timer */
IMPORT VOID store_word();		    /* store word */ 
IMPORT VOID tim_noact();		    /* no activity time out */

IMPORT UBYTE log_chnl;			    /* logical channel number */

#ifdef DEBUG
IMPORT WORD diags[];			    /* used during debugging */
IMPORT UWORD free_count;		    /* number of free buflets */
#endif
/************************************************************************
 * VOID red_ball(pcis, ppkt)
 *    CHNLINFO *pcis;   - pointer to channel information structure
 *    BUFLET *ppkt;	- pointer to incoming red ball packet
 *
 *    red_ball() processes incoming red ball packets.  RED balls are
 *    sent when a GREEN ball has been outputted and the application
 *    is attempted to output data.  When a RED ball is sent the
 *    ball out timer is started.  When the ball out timer interrupt
 *    is called another RED ball packet is sent.
 *    
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID red_ball(pcis, ppkt)
    FAST CHNLINFO *pcis;		    /* pointer to channel information 
    					     * structure.
    					     */
    BUFLET *ppkt;                           /*  packet */
    {
    
    if (pcis->tymechostate != T0)           /* tymnet echo active */
        {
	
	/* Stop the ball out timer.
	 */
        stop_timer(ONE_SEC_TIMER, TIM_BALLOUT, pcis->logicalchnl);

        /* A RED ball was previously sent.  The incoming RED ball
	 * packet acknowledges the RBO state.  The state is changed
	 * to NBO and the no activity timer is set.
	 * When the no activity timer times out, a green ball is sent.
	 */

        int_disable();
	if (pcis->tymechostate == T3)       /* tymnet RBO state */    
            {
/*

	    Originally no activity timer was started when in red ball
            out state.  Now only started when in no balls out state.
            Therefore, it does not have to be stopped when in red ball
            out.  We are waiting for answer to question before code is
            deleted.
	    
            stop_timer(SIXTH_SEC_TIMER, TIM_TYMNOACTIV, pcis->logicalchnl);
 */
	    int_enable();
	    pcis->tymechostate = T1;        /* tymnet NBO state */
            start_timer(SIXTH_SEC_TIMER, TIM_TYMNOACTIV,
		 pcis->logicalchnl, LEN_TYMNOACTIV, tim_noact );
            }

        /* A RED ball is received, and a RED ball was not previously
	 * sent.
	 */ 
        else
	    {
	    int_enable();
	    if (pcis->tymechostate == T4)   /* tymnet echo off */
		pcis->tymechostate = T0;    /* tymnet inactive */
            }
	free_buf(ppkt);
       }
	
    /* RED ball was received when tymnet echoing is not active.
     */
    else
        pkt_in_error(pcis, ppkt);
    }



/************************************************************************
 * VOID green_ball(pcis, ppkt)
 *    CHNLINFO *pcis;   - pointer to channel information structure
 *    BUFLET *ppkt;	- pointer to incoming green ball packet
 *
 *    green_ball() processes incoming green ball packets.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID green_ball(pcis, ppkt)
    FAST CHNLINFO *pcis;		    /* pointer to channel information 
    					     * structure.
    					     */
    BUFLET *ppkt;                           /* pointer to packet */
    {
    
    if (pcis->tymechostate != T0)           /* tymnet echo active */
        {
        int_disable();
        if (pcis->tymechostate == T2)       /* tymnet GBO */
            {
        
	    /* Tymnet echo state is green ball out and a GREEN ball was 
             * received, stop the no activity timer.
	     */
	    stop_timer(ONE_SEC_TIMER, TIM_BALLOUT, pcis->logicalchnl);
            int_enable();
            
	    /* If there is data which is ready for output, then
	     * stop the forwarding character timer and attempt
	     * to link the transmit assembly packet.
	     */
	    if (pcis->nwritebytes > 0)
                {
                stop_timer(SIXTH_SEC_TIMER, TIM_FORWARDING, pcis->logicalchnl);
                (VOID)link_xmit_pkt(pcis, YES);
                }
	    
	    /* Send an exit DEFERRED echo packet and set tymnet echoing
	     * to local echoing.
	     */            
            (VOID)send_cpkt(pcis->lcistruct, EXIT_DEFER_ECHO, (UBYTE *)0, 
		MUST_LINK, ppkt);    
            pcis->tymechostate = T5;        /* tymet local echo on */
            }
	    
	/* Received a GREEN ball when state is not GREEN ball out, then
	 * ignore packet.
	 */
        else 
            {
            int_enable();
            free_buf(ppkt);
	    }
        }
    
    /* green ball packet was received when tymnet echoing is not
     * active.
     */
    else
        pkt_in_error(pcis, ppkt);
    }

/************************************************************************
 * VOID yellow_ball(pcis, ppkt)
 *    CHNLINFO *pcis;   - pointer to channel information structure
 *    BUFLET *ppkt;     - pointer to yellow ball packet
 *
 *    yellow_ball() processes incoming yellow ball packets.  When a
 *    yellow ball is received, an orange ball is returned.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID yellow_ball(pcis, ppkt)
    FAST CHNLINFO *pcis;		    /* pointer to channel information 
    					     * structure.
    					     */
    BUFLET *ppkt;                           /* pointer to packet */
    {
    
    /* Stop the forwarding character timeout and try to link the transmit
     * assembly packet.
     */
    stop_timer(SIXTH_SEC_TIMER, TIM_FORWARDING,
	pcis->logicalchnl);
    (VOID)link_xmit_pkt(pcis, YES);
    
    /* If the application read queue is not empty, then add the yellow
     * ball packet to the read queue.  The current echo packet is closed.
     * The ORANGE ball packet will be sent when the yellow ball
     * is read by the application.
     */
    if (pcis->readqueue.nchains != 0)
	{
        add_queue(&pcis->readqueue, ppkt);
	pcis->echopkt = NULLBUF;	    /* clear echo buffers */
	pcis->idxechodata = MOVED_FIRST_BYTE;
	}
    
    /* Since there is no data in the application read queue, send an
     * Orange ball now.
     */
    else
        (VOID)send_cpkt(pcis->lcistruct, ORANGE_BALL, (UBYTE *)0, 
	    MUST_LINK, ppkt);
    }
 
/************************************************************************
 * VOID orange_ball(pcis, ppkt)
 *    CHNLINFO *pcis;   - pointer to channel information structure
 *    BUFLET *ppkt;	- pointer to buflet packet
 *
 *    orange_ball() processes incoming orange ball packets.  The orange
 *    ball triggers checkpointing.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID orange_ball(pcis, ppkt)
    FAST CHNLINFO *pcis;		    /* pointer to channel information 
    					     * structure.
    					     */
    BUFLET *ppkt;			    /* pointer incoming packet */
    {
    
    /* If the checkpoint event is active then process check point
     * event.  
     */
    if (pcis->checkevent != 0)
	{
	    
	/* If event is update, store 0 at address which was specified
	 * by the application.
	 */
	if (pcis->checkevent == UPDATE_ON)
	    store_word(pcis->checkeventseg, pcis->checkeventoff, 0);
	
	/* Otherwise, execute interrupt which was specified by the
	 * application.
	 */
	else
	    exec_interrupt(pcis->checkeventseg, pcis->checkeventoff);
	
	/* Clear the check point event.
	 */
	pcis->checkevent = 0;
	pcis->checkeventseg = pcis->checkeventoff = 0;
	}
    free_buf(ppkt);
    }



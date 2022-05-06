/************************************************************************
 * perror.c - Build Error Packet or Discard Input Packet
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    perror.c contains the module pkt_error() which processes
 *    an invalid packet.  discard_pkt() discards a packet
 *    which was unnecessary.
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
#include "appl.h"
#include "pkt.h"
#include "state.h"
#include "timer.h"
 
IMPORT INT send_cpkt();                 /* build and send control packet */
IMPORT VOID clear_ssn();		/* clear session */
IMPORT VOID free_buf();			/* free buflet chain packet */
IMPORT VOID int_disable();		/* disable interrupts */
IMPORT VOID int_enable();		/* enable interrupts */
IMPORT VOID start_timer();              /* start timer */
IMPORT VOID stop_timer();		/* stop timer */
IMPORT VOID tim_clrconf();		/* clear confirm timer timeout */

IMPORT CHNLINFO cis[];			/* channel information structure */
IMPORT UBYTE log_chnl;			/* logical channel number */

/************************************************************************
 * VOID pkt_error(plcis, ppkt)
 *    LCIINFO *plcis    - pointer to logical channel information 
 *  			  structure
 *    BUFLET *ppkt;	- pointer to invalid packet
 *
 *    pkt_error() builds a session clear packet.  pkt_error() is called
 *    when an packet is received and  the channel is in an invalid 
 *    session state.
 *
 * Usage notes: Interrupts are disabled in process_pkt() and
 *    must be enabled after the T21 timer is stopped. Otherwise
 *    the T21 timer might change the channel state.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID pkt_error(plcis, ppkt)
    FAST LCIINFO *plcis;		/* pointer to logical channel 
    					 * information structure.
    					 */
    BUFLET *ppkt;                       /* pointer to packet */
    {
    UBYTE data = INVALID_PAD_SIGNAL;
 
    /* Packet received when channel in incorrect state. Send a session
     * clear request and update the channel states.
     */
    if (plcis->ssnstate == S2 || plcis->ssnstate == S5)
	stop_timer(TEN_SEC_TIMER, TIM_T21, log_chnl);
    int_enable(); 

    /* If the channel is mapped, clear the session.
     */
    if (plcis->appchnl >= 0)
	clear_ssn((UBYTE)plcis->appchnl);
    
    /* Send a session clear packet and start the clear confirm timer.
     */
    (VOID)send_cpkt(plcis, SESSION_CLR, &data, MUST_LINK, ppkt);
    start_timer(SIXTH_SEC_TIMER, TIM_CLRCONFIRM, log_chnl,
         LEN_CLRCONFIRM, tim_clrconf);
    
    /* If the channel is mapped, the update the channel state to 
     * pending clear confirm.
     */
    if (plcis->appchnl >= 0)
    	cis[plcis->appchnl].chnlstate = CHNL_PEND_CLEAR;
    plcis->ssnstate = S6;               /* session clear sent */
    }

/************************************************************************
 * VOID pkt_in_error(pcis, ppkt)
 *    CHNLINFO *pcis;   - pointer to application channel information
 *                        structure.
 *    BUFLET *ppkt;	- pointer to incoming packet.
 *
 *    pkt_in_error() is called when processing determines that
 *    the packet received is invalid i.e. an enable permanent echo
 *    is received when MCI echoing is already active.  It differs
 *    from pkt_error() in that the channel must be connected and
 *    interrupts are enabled when it is called.   
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID pkt_in_error(pcis, ppkt)
    FAST CHNLINFO *pcis;		/* channel information structure */
    BUFLET *ppkt;			/* pointer to packet */
    {
    UBYTE clearcode = INVALID_PAD_SIGNAL;
 
    /* packet received when driver in incorrect state. Send a session
     * clear request and update the channel states.
     */
    clear_ssn((UBYTE)pcis->lcistruct->appchnl);
    
    (VOID)send_cpkt(pcis->lcistruct, SESSION_CLR, &clearcode, MUST_LINK,
	ppkt);
    start_timer(SIXTH_SEC_TIMER, TIM_CLRCONFIRM, pcis->logicalchnl,
         LEN_CLRCONFIRM, tim_clrconf);
    pcis->chnlstate = CHNL_PEND_CLEAR;
    pcis->lcistruct->ssnstate = S6;     /* session clear sent */
    }
    
/************************************************************************
 * VOID discard_pkt(plcis, ppkt);
 *    LCIINFO *plcis    - pointer to logical channel information 
 *  			  structure
 *    BUFLET *ppkt;	- pointer to unnecessary packet
 *
 *    discard_pkt() frees an unneed packet.
 *
 * Usage notes: Interrupts are disabled by process_pkt() before
 *    discard_pkt() is called.  discard_pkt must enable the interrupts.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID discard_pkt(plcis, ppkt)
    LCIINFO *plcis;	       		/* pointer to logical channel 
    					 * information structure (not used).
    					 */
    BUFLET *ppkt;			/* pointer to packet */
    {

    int_enable();
    free_buf(ppkt);
    }

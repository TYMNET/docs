/************************************************************************
 * ssnclr.c - Send or Receive Session Clear Packets
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    ssnclr.c contains the source modules send_ssnclr() and
 *    recv_ssnclr().   send_ssnclr() builds and outputs a
 *    session clear control packet to the pad output queue.  recv_ssnclr
 *    processes a session clear control packet from the link
 *    input queue. try_ssn_clear() is called to attempt to send a
 *    session clear packet.  If no packets are available, try_ssn_clear
 *    starts the pending clear confirm timer which will call 
 *    try_ssn_clear to try again when it times out.
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
#include "appl.h"
#include "state.h"
#include "timer.h"
#include "error.h"

IMPORT BUFLET *alloc_buf();		/* allocate buflet chain */
IMPORT INT link_xmit_pkt();		/* link xmit packet */
IMPORT INT send_cpkt();                 /* resend q-bit control packet */ 
IMPORT INT send_ctrl_pkt();             /* send q_bit control packet */
IMPORT VOID clear_ssn();		/* clear session */
IMPORT VOID free_buf();			/* free buflet chain */
IMPORT VOID int_disable();              /* disable interrupts */
IMPORT VOID int_enable();               /* enable interrupts */
IMPORT VOID mov_param();                /* move data to/from application
                                         * parameter buffer.
                                         */
IMPORT VOID start_timer();              /* start timer */
IMPORT VOID stop_timer();               /* stop the timer */
IMPORT VOID tim_clrconf();              /* function called by timer */

IMPORT CHNLINFO *ap_pcis;	        /* pointer to channel information
				 	 * structure.  Set when called
					 * from application interface.
					 */
IMPORT CHNLINFO cis[];			/* array of channel information
					 * structures.
					 */
IMPORT UBYTE app_chnl;		        /* channel number received form
				 	 * application.
					 */
IMPORT UBYTE log_chnl;			/* logical channel number */
IMPORT WORD pend_inc_calls;		/* number of pending incoming
					 * calls
					 */
#ifdef DEBUG	
IMPORT WORD diags[];			/* used for debugging. */
#endif

/************************************************************************
 * VOID try_ssn_clear(plcis, chnl, cleartime, clearcode)
 *    LCIINFO *plcis;	- pointer to logical channel information structure
 *    UBYTE chnl;	- logical channel number
 *    WORD cleartime;	- clear time for start timer
 *    BYTE clearcode;	- clear code for session clear packet
 *
 *    try_ssn_clear attempts to get a buflet.  If the buflet is
 *    available, the session clear packet is sent  the clear time 
 *    is cleared and the pending clear confirm timer is started.
 *    Otherwise, the clear code and clear time are saved and the 
 *    pending clear confirm timer is started.  When the pending clear timer
 *    expires it's processing is dependent on whether the
 *    clear time is set.  If the clear time is set, the try_ssn_clear
 *    is called again to try to send a session clear packet.
 *
 * Usage notes: If no buflets are ever freed, try_ssn_clear will 
 *    loop forever.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID try_ssn_clear(plcis, chnl, cleartime, clearcode)
    LCIINFO *plcis;			/* pointer to logical channel 
					 * structure.
					 */
    UBYTE chnl;				/* logical channel number */
    WORD cleartime;			/* clear time */
    BYTE clearcode;			/* clear code */
    {
    BUFLET *ppkt;			/* pointer to packet */
    UBYTE code = (UBYTE)clearcode;	/* clear code */
	    
    /* Try to allocate a free buflet.  If buflets are available,
     * clear  the clear time and clear code, send the session clear
     * packet and start the pending clear confirm timer.
     */
    if (cleartime == 0)
        cleartime = 1;

    /* There was a buflet available, so built the session clear packet.
     */
    if ((ppkt = alloc_buf((BYTES)1)) != NULLBUF)
    	{
	plcis->ssncleartime = 0;	/* clear saved clear time */
	plcis->ssnclearcode = 0;	/* clear saved clear code */
	ppkt->bufdata[STX] = STX_CHAR;
	ppkt->bufdata[GFI_LCI] = QBIT | chnl;
	
	/* Send session clear packet and start the session clear confirm
	 * timer.  
	 */
	(VOID)send_cpkt(plcis, SESSION_CLR, &code, MUST_LINK, ppkt);
	start_timer(SIXTH_SEC_TIMER, TIM_CLRCONFIRM, chnl, cleartime,
	    tim_clrconf);
	}
	
    /* If there are no buflets available, save the clear code and
     * the clear time and start the pending clear confirm timer.
     * When the pending clear confirm timer times out, it calls
     * try_ssn_clear() again to send a session clear packet.
     */
    else
	{
	plcis->ssncleartime = cleartime;
	plcis->ssnclearcode = clearcode;
	start_timer(SIXTH_SEC_TIMER, TIM_CLRCONFIRM, chnl, 
	    LEN_CLRCONFIRM, tim_clrconf);
	}
    }

/************************************************************************
 * VOID recv_ssnclr(plcis, ppkt)
 *    LCIINFO *plcis    - pointer to logical channel information
 *                        structure.
 *    BUFLET *ppkt	- pointer to session clear packet
 *      
 *    recv_ssnclr processes session clear packets from the application
 *    read queue. 
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID recv_ssnclr(plcis, ppkt)
    FAST LCIINFO *plcis;		/* pointer to logical channel 
    					 * information structure.
    					 */
    BUFLET *ppkt;                       /* pointer to packet */
    {
    FAST CHNLINFO *pcis;		/* pointer to channel information
    					 * structure.
    					 */

    /* If the channel is mapped, the extract the clear code from the
     * packet.
     */
    if (plcis->appchnl >= 0)
	{
	pcis = &cis[plcis->appchnl];
	pcis->clearcode = (BYTE)ppkt->bufdata[EXTRA_DATA];
	}
    
    /* If the channel has already sent a call cleared packet or attempted
     * to send a call cleared packet but no buflet was available, then a 
     * call cleared accept message is sent.
     */
    if (plcis->ssnstate == S6)          /* session clear sent */
        {
        stop_timer(SIXTH_SEC_TIMER, TIM_CLRCONFIRM, log_chnl);
        int_enable();
    
	/* If the ssncleartime is not zero, then the sending of a session
	 * clear packet was attempted but was not successful.  Confirm
	 * the session clear with a session accept.  The session accept
	 * packet is added to the pad output queue.
	 */
	if (plcis->ssncleartime)
	    {
	    plcis->ssncleartime = plcis->ssnclearcode = 0;
	    (VOID)send_cpkt(plcis, SSN_CLR_ACP, (UBYTE *)0, MUST_LINK, ppkt);
	    }
	else
	    free_buf(ppkt);
        }
    
    /* Channel has not previously attempted to send a call clear
     * packet.
     */
    else
        {
	
	/* If a session is being established ( A session request has
	 * been sent or channel is waiting for a session request),
	 * stop timer which is used for establishing sessions.
	 */
	if (plcis->ssnstate == S2 || plcis->ssnstate == S5)
	    stop_timer(TEN_SEC_TIMER, TIM_T21, log_chnl);
	int_enable();
	
	/* If channel is mapped then clear the session.
	 */
	if (plcis->appchnl >= 0)
	    clear_ssn((UBYTE)plcis->appchnl);
        
	/* Confirm the session clear packet with a session accept. The
	 * session accept packet is added to the pad output queue.
	 */
	(VOID)send_cpkt(plcis, SSN_CLR_ACP, (UBYTE *)0, MUST_LINK, ppkt);
        }
    
    /* If the channel is mapped, the channel and session states are updated
     * to call clear confirmed.
     */
    if (plcis->appchnl >= 0)
	{
	pcis->chnlstate = CHNL_CALL_CLEARED;
	plcis->ssnstate = S7;           /* session clear confirmed/recvd */
	}
    
    /* Unmapped channel is set to session ready.
     */
    else
	plcis->ssnstate = S1;		/* session ready */
    }
 

/************************************************************************
 * INT send_ssnclr()
 *
 *    send_ssnclr() sends session clear requests.
 *
 * Returns:  
 *    SUCCESS - Processing Successful
 *    ILLEGAL_CLEAR_CODE - Clear Code specified by application is 
 *        not valid
 *    ILLEGAL_CHNL_FUNC - sending session clear is invalid in
 *        current channel state.
 *
 ************************************************************************/
INT send_ssnclr()
    {
    WORD clearcode;                    	/* clearing cause code */
    WORD cleartime;                     /* clear time_out */
 
    /* If channel state is not valid, return error.
     */
    if (app_chnl == 0)
	return (ILLEGAL_CHNL_FUNC);
    if (ap_pcis->chnlstate == CHNL_DISCONNECTED || 
       ap_pcis->chnlstate == CHNL_CALL_CLEARED)
        return (ILLEGAL_CHNL_FUNC);

    /* Before sending a session clear, attempt to link the last data
     * packet to the output queue.
     */
    stop_timer(SIXTH_SEC_TIMER, TIM_FORWARDING, ap_pcis->logicalchnl);
    (VOID)link_xmit_pkt(ap_pcis, YES);

    mov_param((UBYTE *)&clearcode, sizeof(WORD), PARAM_1, 0, FROM_APPL);
    
    /* Validate the application clear code.
     */
    if (clearcode < NORMAL_CALL_END || clearcode > INVALID_PAD_SIGNAL)
	return (ILLEGAL_CLEAR_CODE);
    
    /* Move the clear confirm timeout value from the application parameter
     * buffer.  Make sure the time is greater than zero.
     */
    mov_param((UBYTE *)&cleartime, sizeof(WORD), PARAM_2, 0, FROM_APPL);
    if (cleartime == 0)
	cleartime = 1;
		    
    ap_pcis->clearcode = clearcode;
    
    /* If channel is waiting for a session request, then decrement
     * the number of channels waiting for a session request and
     * update the channel state.  Since the session has never been
     * established, the session does not have to be cleared.
     */
    if (ap_pcis->chnlstate == CHNL_PEND_RECV_CALL)
	{
	--pend_inc_calls;
	ap_pcis->chnlstate = CHNL_CALL_CLEARED;
	}
    else
	{
	int_disable();
	
	/* If channel previously sent or attempted to send a call clear 
	 * packet then update the the clear code and clear time.  
	 */
	if (ap_pcis->lcistruct->ssnstate == S6)
	    {
	
	    /* if attempted to send session clear then update the
	     * clear time and the clear code.  
	     */
	    if (ap_pcis->lcistruct->ssncleartime)
		{
		ap_pcis->lcistruct->ssncleartime = cleartime;
		ap_pcis->lcistruct->ssnclearcode = clearcode;
		int_enable();
		}
	    
	    /* A session clear has already been sent, so stop the clear
             * confirm timer.  The pending clear confirm timer will
             * restart the timer when the clear session packet is sent. 
	     */
	    else
		{
		stop_timer(SIXTH_SEC_TIMER, TIM_CLRCONFIRM,
		    ap_pcis->logicalchnl);
		int_enable();
		}
	    }
	    
	/* Stop the timer which is waiting for a session to be established,
	 * clear the session and update the states.
	 */
	else
	    {
	    if (ap_pcis->lcistruct->ssnstate == S2 || 
		ap_pcis->lcistruct->ssnstate == S5)
		stop_timer(TEN_SEC_TIMER, TIM_T21, 
		    ap_pcis->logicalchnl);
	    int_enable();
	    
	    /* Clear the session.
	     */
	    clear_ssn(app_chnl);
	    
	    /* Update channel state to pending clear confirm.
	     */
	    ap_pcis->chnlstate = CHNL_PEND_CLEAR;
	    ap_pcis->lcistruct->ssnstate = S6;        /* session clear sent */
	    }
	
	/* Attempt to send a session clear packet.  If the session clear 
	 * cannot be sent then the pending clear confirm timer is
	 * started.  The timer will continue calling try_ssn_clear()
	 * until the clear packet is sent.
	 */
	try_ssn_clear(ap_pcis->lcistruct, ap_pcis->logicalchnl, cleartime,
	    clearcode);
	}
    return (SUCCESS);
    }

/************************************************************************
 * VOID recv_clracp(plcis, ppkt)
 *    LCIINFO *plcis;	- pointer to logical channnel information structure
 *    BUFLET *ppkt;	- pointer to buflet packet
 *
 *    recv_clracp() processes clear accept packets received from the
 *    link.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID recv_clracp(plcis, ppkt)
    LCIINFO *plcis;			/* pointer to logical channel
    					 * information structure.
    					 */
    BUFLET *ppkt;                       /* pointer to packet */
    {
    CHNLINFO *pcis;			/* pointer to channel information
    					 * structure.
    					 */
    /* Stop pending clear confirm timer and update session and channel
     * states when a clear accept is received.  The packet is freed.
     */
    stop_timer(SIXTH_SEC_TIMER, TIM_CLRCONFIRM, log_chnl);
    int_enable();
    if (!plcis->ssncleartime)
	{
	
	/* If the channel is mapped, the update channel status to call 
	 * cleared.
	 */
	if (plcis->appchnl >= 0)
	    {
	    pcis = &cis[plcis->appchnl];
	    pcis->chnlstate = CHNL_CALL_CLEARED;
	    plcis->ssnstate = S7;       /* session clear confirmed/received */
	    }
	else
	    plcis->ssnstate = S1;
	}
    free_buf(ppkt);
    }



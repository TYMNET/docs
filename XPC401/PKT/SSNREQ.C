/************************************************************************
 * ssnreq.c - Receive Send Session Request Packets
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    ssnreq.c contains the modules called to send a session request
 *    packet and to process received session request packets.
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
#include "state.h"
#include "pkt.h"
#include "appl.h"
#include "timer.h"
#include "param.h"
#include "device.h"
#include "error.h"

IMPORT INT send_cpkt();			  /* send control packet -
					   * use incoming packet
					   */
IMPORT INT send_ctrl_data_pkt();          /* send control packet */
IMPORT VOID adjust_time_len();		  /* adjust the time length */
IMPORT VOID free_buf();			  /* free buflet chain */
IMPORT VOID int_disable();		  /* disable interrupts */
IMPORT VOID int_enable();		  /* enable interrupts */ 
IMPORT VOID mov_param();                  /* move data to/from 
					   * application parameter buffer
					   */
IMPORT VOID start_timer();	          /* start timer */
IMPORT VOID stop_timer();		  /* stop timer */
IMPORT VOID tim_clrconf();		  /* process time out for 
					   * pending clear confirm.
					   */
IMPORT VOID tim_t21();			  /* timer time out function */

IMPORT CHNLINFO *ap_pcis;		  /* pointer to channel 
					   * information structure
					   * set up by application
					   * interface.
					   */
IMPORT CHNLINFO cis[];			  /* channel information 
					   * structure.
					   */
IMPORT LCIINFO lcis[];			  /* logical channel
					   * information struct.
			                   */
IMPORT PORTPARAMS port_params;            /* port param structure */ 
IMPORT UBYTE app_chnl;			  /* application channel number
                                           */
IMPORT UBYTE log_chnl;			  /* logical channel number */
IMPORT UWORD free_count;		  /* number of buflets free */
IMPORT UWORD program_error;		  /* program error code */
IMPORT UWORD rnr_buflets_req;		  /* number of buflets required before
					   * RNRing.
					   */
IMPORT WORD pend_inc_calls;               /* number of channels waiting
					   * for session requests.
					   */
#ifdef DEBUG
IMPORT WORD diags[];			  /* used for debugging */
#endif

/************************************************************************
 * VOID recv_ssnreq(plcis, ppkt)
 *    LCIINFO *plcis    - pointer to logical channel information
 *     			  structure.
 *    BUFLET *ppkt;	- pointer to incoming session request packet
 *
 *    recv_ssnreq processes incoming session request packets.  
 *    the session state is updated and a logical channel number
 *    is assigned.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID recv_ssnreq(plcis, ppkt)
    FAST LCIINFO *plcis;		    /* pointer to logical channel
					     * information structure.
					     */
    BUFLET *ppkt;                           /* pointer to packet */
    {
    WORD i;				    /* loop counter */
    FAST CHNLINFO *pcis;		    /* pointer to channel information
					     * structure.
					     */
    
    /* If DTE terminal already sent session request, set state
     * to session collision. Otherwise, update channel and
     * session states and save packet for read session data function.
     */ 
    if (plcis->ssnstate == S2)
	{
	
	/* If session request has been sent and port is a DTE, 
	 * then session collision has occurred.
	 */
	if (port_params.dxemode == DTE_MODE)
	    {
	    plcis->ssnstate = S5;           /* session collision */
	    int_enable();
	    free_buf(ppkt);
	    return;
	    }
	
	/* Stop the session request timer.  The session request which
         * was received will be treated as a session accept.
	 */
	else 			            /* pending session req */
	    {
	    stop_timer(TEN_SEC_TIMER, TIM_T21, log_chnl);
	    int_enable();
	    pcis = &cis[plcis->appchnl];
	    }
	}
	
    /* Channel is waiting for the session request.
     */
    else
	{
	int_enable();
	
	/* If no channels are waiting for session request packet or
	 * resources are low clear the session.
	 */
	if (!pend_inc_calls || free_count < 
	    (MIN_REQUIRED_BUFLETS + rnr_buflets_req))
	    {

	    
	    /* There are not enough buflets to activate the channel,
	     * send a session clear and start the pending clear confirm
	     * timer.
	     */
	    (VOID)send_cpkt(plcis, SESSION_CLR, (UBYTE *)0, MUST_LINK, ppkt);
	    start_timer(SIXTH_SEC_TIMER, TIM_CLRCONFIRM, log_chnl,
	    	LEN_CLRCONFIRM, tim_clrconf);
	    plcis->ssnstate = S6;
	    return;
	    }
	
	/* Link the application channel with a channel which is waiting for
	 * a session request.
         */
	else
	    {
	    for (i = 1; i <= MAX_CHNL; ++i)
		if (cis[i].chnlstate == CHNL_PEND_RECV_CALL)
		    break;
	    if (i > MAX_CHNL)
		{
		program_error = LINK_CHNL_ERROR;
		free_buf(ppkt);
		return;
		}

	    /* link the application channel to the logical
	     * channel.
	     */
	    pcis = &cis[i];
	    
	    /* Link the structures.
	     */
	    pcis->lcistruct = plcis;
	    
	    int_disable();
	    if (plcis->dteflowstate == G1)
		rnr_buflets_req += RNR_CHNL_BUFLETS;
	    
	    /* Link the channel numbers (application and logical).  The
	     * application channel number is stored in the logical 
	     * channel structure and the logical channel number is
	     * stored in the application channel structure.
	     */
	    plcis->appchnl = i;
	    int_enable();
	    pcis->logicalchnl = log_chnl;

	    /* Depending on the current number of channels open and
	     * the baud rate, adjust the time out lengths for the
	     * reject and window rotations timers.  These timers
	     * are contained in link.  Their durations are adjusted
	     * whenever a channel is opened or closed or the port
	     * parameters change.
	     */
	    adjust_time_len();
	
	    /* Decrement number of channels waiting for session request.
	     */
	    --pend_inc_calls;
	    }
	}
	
    /* Save pointer to session request packet in the channel information
     * structure.  The data will be processed by a read session data
     * application function.
     */
    pcis->ssndatapkt = ppkt;
    pcis->idxssndata = EXTRA_DATA;
    pcis->chnlstate = CHNL_RECV_CALL;
    plcis->ssnstate = S3;               /* session request recvd */
    }
 
/************************************************************************
 * INT send_ssnreq()
 *
 *    send_ssnreq builds and sends a session request packet.   If the
 *    channel is not disconnected, an error is returned.   send_ssnreq
 *    is called directly from the application packet jump table.
 *
 * Returns:  
 *    SUCCESS - processing was successful.
 *    ILLEGAL_CHNL_FUNC - Function is invalid in current channel state
 *        or application attempted session request on channel 0. 
 *    ILLEGAL_CALL_REQ - Session requeust format or length is invalid.
 *    NO_CHANNELS_AVAILABLE - No channels are available for send session
 *        request or the driver is running out of buflets.
 *
 ************************************************************************/
INT send_ssnreq()
    {
    WORD format;                            /* format of transfer */
    WORD num;                               /* number of bytes to transfer */
    SHORT i;			            /* for loop counter */
  

    /* If the current channel state is disconnected, 
     * send session request packet and update session/channel states.
     */ 
    
    
    if (app_chnl == 0 || ap_pcis->chnlstate != CHNL_DISCONNECTED)
	return (ILLEGAL_CHNL_FUNC);

    /* If the resources are low then return error.
     */
    if (free_count < (rnr_buflets_req + MIN_REQUIRED_BUFLETS))
        return(NO_CHNLS_AVAILABLE);
    
    /* If attempted to send a session request when restart is being
     * done, set the channel state to call cleared.
     */
    if (lcis[0].restartstate == R2 && lcis[0].r20trans == 0)
	{
	ap_pcis->chnlstate = CHNL_CALL_CLEARED;
	ap_pcis->clearcode = REMOTE_XPC_LOST;
	}
	
    /* If this device is a DTE, assign application channels starting at the
     * first logical channel structure.  For DCE devices, assign channels 
     * from last to first in logical channel structure.
     */
    else
	{
	if (port_params.dxemode == DTE_MODE)
	    {
	    for (i = 1; i <= MAX_CHNL; ++i)
		if (lcis[i].appchnl < 0)
		    break;
	    if (i > MAX_CHNL)
		return(NO_CHNLS_AVAILABLE);
	    }
	else
	    {
	    for (i = MAX_CHNL; i > 0; --i)
		if (lcis[i].appchnl < 0)
		    break;
	    if (i == 0)
		return(NO_CHNLS_AVAILABLE);
	    }
    
	/* Map application channel to logical channel.
	 */
	ap_pcis->lcistruct = &lcis[i];
	int_disable();
	lcis[i].appchnl = (BYTE)app_chnl;
	if (lcis[i].dteflowstate == G1)
	    rnr_buflets_req += RNR_CHNL_BUFLETS;
	int_enable();
	ap_pcis->logicalchnl = i;
	
	/* Depending on the current number of channels open and
	 * the baud rate, adjust the time out lengths for the
	 * reject and window rotations timers.  These timers
	 * are contained in link.  Their durations are adjusted
	 * whenever a channel is opened or closed or the port
	 * parameters change.
	 */
	adjust_time_len();
    
    
	/* Extract parameters from application buffer.
	 */
	mov_param((UBYTE *)&num, sizeof(WORD), PARAM_2, 0, FROM_APPL);
	mov_param((UBYTE *)&format, sizeof(WORD), PARAM_3, 0, FROM_APPL);
    
	/* if the session request parameters are invalid, return error.
	 */

	if (num > SSN_DATA_LEN || format != TYMNET_STRING)
	    return (ILLEGAL_CALL_REQ);
    
	/* Send session request packet to host.
	 */
	if (send_ctrl_data_pkt(ap_pcis->lcistruct, ap_pcis->logicalchnl, 
	    SESSION_REQ, num, CHECK_LINK) != SUCCESS)
	    return (NO_CHNLS_AVAILABLE);	    /* !!!!NEED ANOTHER ERROR
						    * CODE FOR THIS RETURN
					            */

	/* Update channel status to pending call accept and session
	 * state to S2 DTE session request.
	 */
	ap_pcis->chnlstate = CHNL_PEND_CALL_ACP;
	ap_pcis->lcistruct->ssnstate = S2;      /* session request sent */

	/* Start the session request timer.
	 */
	start_timer(TEN_SEC_TIMER, TIM_T21, ap_pcis->logicalchnl,
	    LEN_T21, tim_t21);
	}
    return (SUCCESS);
    }
 
/************************************************************************
 * INT ssn_answerset()
 *
 *    ssn_answerset() sets the channel state to waiting for incoming
 *    call.  The number of channels waiting for an incoming call is
 *    incremented and the channel state updated.
 *
 *
 * Usage notes: If there are not at least a minimum number of
 *    buflets available, the channel state is not updated.
 *
 * Returns:  If the channel is not in the correct state, ssn_answerset
 *    returns ILLEGAL_CHNL_FUNC.  Otherwise, ssn_answerset returns 
 *    SUCCESS.
 *
 ************************************************************************/
INT ssn_answerset()
    {
    
    /* If channel is in a disconnected state, update channel
     * and session states.
     */ 
    
    
    if (app_chnl == 0 || ap_pcis->chnlstate != CHNL_DISCONNECTED)
        return (ILLEGAL_CHNL_FUNC);
    
    /* Set the channel state to pending incoming call.
     */
    ap_pcis->chnlstate = CHNL_PEND_RECV_CALL;
    
    /* Increment the number of channels which are waiting for an incoming
     * call.
     */
    ++pend_inc_calls;        
    return (SUCCESS);
    }

    


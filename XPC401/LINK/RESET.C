/************************************************************************
 * reset.c - Process Reset and Reset Confirm Packets
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    reset.c contains the source module reset_state() which
 *    processes reset packets for channels 1 thru 15.  
 *    It is invalid to perform a RESET of channel 0.
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
#include "link.h"
#include "state.h"
#include "timer.h"

IMPORT BOOL valid_recvackseq();		/* valid P(R) sequence number */
IMPORT VOID free_buf();			/* free buflet chain */
IMPORT VOID process_recvackseq();	/* process P(R) sequence no */
IMPORT VOID reset_chnl();		/* reset channel */
IMPORT VOID reset_diag();		/* reset diagnostic */
IMPORT VOID send_link_cpkt();		/* send control packet */
IMPORT VOID stop_timer();		/* stop timer */
IMPORT VOID update_recvseq();		/* update P(S) sequence no */

IMPORT UBYTE linkchnl;			/* link channel */
IMPORT UBYTE pkttype;			/* packet type */
#ifdef DEBUG
IMPORT WORD diags[];			/* XXX */
#endif

/************************************************************************
 * LOCAL BOOL valid_reset_cause()
 *
 *    valid_reset_cause() validates the cause in a reset packet.
 *
 * Returns:  valid_reset_cause() always  returns YES.
 *
 ************************************************************************/
LOCAL BOOL valid_reset_cause()
    {

    return(YES);
    }
    
/************************************************************************
 * LOCAL BOOL valid_reset_diag()
 *
 *    valid_reset_diag() validates the diagnostic which was passed in the
 *    reset packet.  As of now all diagnostics are valid.
 *
 * Returns:  valid_reset_diag() always returns YES 
 *
 ************************************************************************/
LOCAL BOOL valid_reset_diag()
    {
    return(YES);
    }
    


/************************************************************************
 * VOID reset_pkt(plcis, ppkt)
 *    LCIINFO *plcis;	- pointer to logical channel information structure
 *    BUFLET *ppkt;	- pointer to incoming packet
 *
 *    reset_state() processes Reset and Reset Confirm packets for
 *    channels 1 thru 15.  
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID reset_pkt(plcis, ppkt)
    LCIINFO *plcis;			/* pointer to logical channel
    					 * information structure.
    					 */
    BUFLET *ppkt;			/* pointer to current buflet packet.
    					 */
    {

    /* Process RESET packets.
     */
    if (pkttype == RESET)

	{
	
	/* Validate the reset cause code, the reset diagnostic,
	* and the reset packet length.
	*/
	if (ppkt->bufdata[FRAME_LEN] < 1)
	    {
	    reset_diag(ppkt, DIAG38);
	    return;
	    }
	else if (!valid_reset_cause())	
	    {
	    reset_diag(ppkt, DIAG81);
	    return;
	    }

	/* If there is a reset diagnostic, then validate it.
	 */
	else if (ppkt->bufdata[FRAME_LEN] > 1)
	    {
	    if (!valid_reset_diag())
		{
		reset_diag(ppkt, DIAG39);
		return;
		}
	    }
	
	/* The length of the extra data in a RESET packet must be
	 * less than 2.
	 */
	if (ppkt->bufdata[FRAME_LEN] > 2)
	    {
	    reset_diag(ppkt, DIAG39);
	    return;
	    }
	    
	/* The reset packet is valid. 
	 */
	else
	    {
	
	    /* If a RESET control packet has not been output, then
	     * reset the channel and send a reset confirmation
	     * packet.  Otherwise, stop the timer which was set
	     * when the RESET was output, and initialize the reset state.
	     */
	    if (plcis->resetstate == D1)
		{
		reset_chnl(plcis, linkchnl);
		    
		/* Update the expect input packet's P(S).
		*/
		update_recvseq(plcis);
		send_link_cpkt(ppkt, RESET_CONFIRM, (UBYTE *)0, 
		    (UBYTE *)0, 0);
		}
	    else
		{
	    
		/* Update the incoming packets P(S).
		*/
		update_recvseq(plcis);
		stop_timer(TEN_SEC_TIMER, TIM_T22, linkchnl);
		free_buf(ppkt);
		plcis->resetstate = D1;		/* flow control ready */
		}
	    }

	}
    
    /* The input packet is a RESET CONFIRM.
     */
    else if (pkttype == RESET_CONFIRM)
	{
	    
	/* Validate the input packet's P(R).
	 */

	if (!valid_recvackseq(plcis, ppkt))
	    reset_diag(ppkt, DIAG2);
	else if (ppkt->bufdata[FRAME_LEN] > 0)
	    reset_diag(ppkt, DIAG39);
	    
	/* The reset confirm packet is valid.
         */
	else 
	    {
		
	    /* Update the expected input packet's P(S).
	     */
	    update_recvseq(plcis);
		
	    /* Acknowledge packets using the RESET CONFIRMATION packet's
	     * P(R).  All packets in the waiting to
	     * be acknowledged queue, which have a  P(S) which is less
	     * than the incoming P(R) are removed from the
	     * waiting to be acknowledged queue.
	     */
	    process_recvackseq(plcis, ppkt);
		
	    /* If the reset state is flow control ready then set
	     * the state to DTE Reset request and send a diagnostic
	     * message to the host.  Otherwise, acknowledge the
	     * reset confirmation packet and update the state to
	     * flow control ready.
	     */
	    if (plcis->resetstate == D1)
		{
		reset_diag(ppkt, DIAG27);
		plcis->resetstate = D2;
		}
	    else
		{
		stop_timer(TEN_SEC_TIMER, TIM_T22, linkchnl);
		send_link_cpkt(ppkt, RR, (UBYTE *)0, (UBYTE *)0, 0);
		plcis->resetstate = D1;
		}
	    }
	}
		
    }



    


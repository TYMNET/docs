/************************************************************************
 * pktstat.c - Report Channel I/O Status
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    pktstat.c contains the module p_rpt_iostat().  p_rpt_iostat()
 *    processes all of the packets in the pad input queue.  The current
 *    I/O status is then moved into the application parameter buffer.
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

IMPORT INT get_xmit_pkt();		/* get a transmit packet */
IMPORT INT link_xmit_pkt();		/* link xmit packet to output queue,
					 * error if queue is full.
					 */
IMPORT VOID int_disable();		/* disable interrupts */
IMPORT VOID int_enable();		/* enable interrupts */
IMPORT VOID mov_param();		/* move data to/from application
					 * parameter buffer.
					 */
IMPORT VOID process_queue();	        /* process queue */
IMPORT VOID stop_timer();		/* stop the timer */

IMPORT CHNLINFO *ap_pcis;		/* current channel information
					 * structure pointer.
					 */
IMPORT UBYTE app_chnl;			/* application channel number */
IMPORT UWORD free_count;		/* count of number of free buflets */
IMPORT UWORD program_error;		/* error was encountered */
IMPORT UWORD rnr_buflets_req;		/* number of buflets required before
					 * RNRing.
					 */
#ifdef DEBUG
IMPORT WORD diags[];			/* diagnostics used for debugging */
#endif
	
/************************************************************************
 * INT p_rpt_iostat()
 *
 *    p_rpt_iostat() reports the current status of I/O for
 *    the channel specified by the application.  
 *  
 * Returns:  If the channel is not connected, p_rpt_iostat returns
 *    an ILLEGAL_CHNL_FUNC.  Otherwise, p_rpt_iostat returns
 *    SUCCESS.
 *
 ************************************************************************/
INT p_rpt_iostat()
    {
    WORD inp_status = INPUT_NORMAL;	/* input status */
    WORD out_status = NO_OUTPUT;	/* output status */ 

    /* If the channel is not connected, return an error.
     */
    if (app_chnl == 0)
	return (ILLEGAL_CHNL_FUNC);
    
    /* Read and process packets from the pad input queue. Session request
     * packet pointers are stored in the channel structure,  data packets
     * are moved into the application read queue.
     */
    process_queue(ap_pcis->lcistruct);

    /* If previously we were in a flow controlled state, and now we aren't
     * in a flow controlled state, change the state so that an RR control
     * packet will be sent by plink.  The RR packet allows the receiving
     * of more data packets.
     */
     
     int_disable();
     if (ap_pcis->lcistruct->dteflowstate == G2 && free_count >= 
	(rnr_buflets_req + BUFLET_HIWATER))
         ap_pcis->lcistruct->dteflowstate = G3;
     int_enable();
    
    /* Move the number of bytes in the application read queue into the
     * application parameter buffer.
     */
    mov_param((UBYTE *)&ap_pcis->nreadbytes, sizeof(WORD), PARAM_1, 0, TO_APPL);
    
    /* If the channel is not connected it is assumed to be in a flow controlled
     * state.
     */
    if (ap_pcis->chnlstate != CHNL_CONNECTED)
	out_status = FLOWCTRL_OUTPUT;
    else
        {
	
	/* If output is currently flow controlled, attempt to link the 
	 * assembly packet one more time.
	 */
	int_disable();
	if (ap_pcis->holdassembly && ap_pcis->nwritebytes > 0)
	    {
	    stop_timer(SIXTH_SEC_TIMER, TIM_FORWARDING, ap_pcis->logicalchnl);
	    int_enable();
	    (VOID)link_xmit_pkt(ap_pcis, NO);
	    }
	else
	    int_enable();
	
	/* If output is currently flow controlled because tranmit assembly
	 * packet cannot be linked or there isn't a transmit assembly
	 * packet, then set output status to flow controlled.
	 */
	if (ap_pcis->holdassembly ||
	   (!ap_pcis->assemblypkt && free_count < MIN_REQUIRED_BUFLETS))
	    out_status = FLOWCTRL_OUTPUT;
	else 
	    {
	    
	    /* If there is no packet, then allocate transmit buffer.  There
	     * has to be a buflet available.
	     */
	    if (!ap_pcis->assemblypkt)
		if (get_xmit_pkt(ap_pcis) != SUCCESS)
		    program_error = FREE_COUNT_ERROR;
	
	    /* The are no packets to output, therefore set status to no output.
	     */
	    if (ap_pcis->lcistruct->outpktqueue.begqueue != NULLBUF
		|| ap_pcis->nwritebytes > 0)
		out_status = NO_FLOWCTRL_OUTPUT;
	    }
	}
   	   
    /* Set up the input status.
     */
    if (ap_pcis->breakstatus)
	inp_status = BREAK_BIT;
    
    /* Move input and output statuses into the application parameter
     * buffers.
     */
    mov_param((UBYTE *)&inp_status, sizeof(WORD), PARAM_2, 0, TO_APPL);
    mov_param((UBYTE *)&out_status, sizeof(WORD), PARAM_3, 0, TO_APPL);
    ap_pcis->errorstatus = ap_pcis->breakstatus = 0;
    return (SUCCESS);
    }



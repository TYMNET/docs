/************************************************************************
 * pktchnl.c - Read Channel Status When in Packet State
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    pktchnl.c contains the module chnl_status() which reads the
 *    current channel status.
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
#include "error.h"

IMPORT VOID init_pad();		        /* initialize packet channel */
IMPORT VOID int_disable();		/* disable interrupts */
IMPORT VOID int_enable();		/* enable interrupts */
IMPORT VOID mov_param();	        /* move data to/from application 
				         * parameter buffer.
				         */
IMPORT VOID process_queue();            /* process queue */
IMPORT VOID recv_ssnreq();	        /* process session request */

IMPORT CHNLINFO *ap_pcis;	        /* pointer to channel information
				         * structure.
				         */
IMPORT LCIINFO lcis[];		        /* logical channel information
				         * structure.
				         */
IMPORT UBYTE app_chnl;		        /* application channel number */
IMPORT UWORD free_count;		/* number of buflets free */
IMPORT UWORD rnr_buflets_req;		/* number of buflets needed before
					 * RNRing.
					 */
#ifdef DEBUG
IMPORT WORD diags[];			/* debugging diagnostics */
#endif
 
/************************************************************************
 * INT chnl_stat()
 *
 *    chnl_status() reads the current channel status.  If a channel
 *    has been reset, the reset state and cause are moved into the
 *    application parameter buffers.  Otherwise, the channel state
 *    and call cleared code are moved into the application parameter
 *    buffers.  When the session has been cleared the pad is initialized.
 *    
 *
 * Usage notes: chnl_status() assumes that the pointer to the 
 *     application channel structure has been set up before it
 *     is called.  chnl_status() also assumes that the channel
 *     state is not updated by init_pad().
 *
 * Returns:  chnl_stat() always returns SUCCESS.  
 *
 ************************************************************************/
INT chnl_status()
    {
    UBYTE i;			        /* array indexes */
    WORD code;				/* code */
    WORD state = -2;			/* state */

    /* If channel status is requested for channel 0, then 
     * move channel status into application parameter buffer.
     */
    if (app_chnl == 0)      
    	{
	state = (WORD)ap_pcis->chnlstate;
	code = (WORD)ap_pcis->clearcode;
	if (ap_pcis->chnlstate == CHNL_CALL_CLEARED)
	    ap_pcis->chnlstate = CHNL_CONNECTED;
	}

    /* Process pad input  queue for specified channel and determine
     * the current channel status.
     */
    else
	{
	
	/* Read and process packets from the pad input queue for
         * the channel specified by the application.
	 */
	if (ap_pcis->lcistruct)
	    process_queue(ap_pcis->lcistruct);

	/* Attempt to read and process session requests for any
         * unmapped channel.
	 */
	for (i = 1; i <= MAX_CHNL; ++i)
	    {
	    if (lcis[i].appchnl != -1)
		continue;
	    process_queue(&lcis[i]);
	    }
	
	/* If the application is in a flow controlled state, and there
	 * are buflets available now, set the state so that when plink
	 * is called, the inputting of packets will resume.
	 */
	int_disable();
	if (ap_pcis->lcistruct->dteflowstate == G2 &&
	    free_count >= (rnr_buflets_req + BUFLET_HIWATER))
	    ap_pcis->lcistruct->dteflowstate = G3;

	    
	/* Set up the channel state and call cleared code.
	 */
	state = (WORD)ap_pcis->chnlstate;
	code = (WORD)ap_pcis->clearcode;
	int_enable();
	if (state == CHNL_CALL_CLEARED)
	    init_pad(app_chnl, NO, NO);
	}

    /* Move the channel state and call cleared code into
     * the application parameter buffers.
     */
    mov_param((UBYTE *)&state, sizeof(WORD), PARAM_1, 0, TO_APPL);
    mov_param((UBYTE *)&code, sizeof (WORD), PARAM_2, 0, TO_APPL);
    return (SUCCESS);
    }

	    

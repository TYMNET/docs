/************************************************************************
 * ssnacp.c - Receive or Send Session Accept Packet
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    ssnacp.c contains the module recv_clracp() which processes 
 *    received session clear accept packets and send_clracp()
 *    which builds and sends session clear accept packets.  
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
#include "error.h"

IMPORT INT send_ctrl_data_pkt();        /* send q-bit control packet */
IMPORT VOID int_enable();		/* enable interrupts */
IMPORT VOID mov_param();                /* move data to/from application
                                         * parameter packet.
                                         */
IMPORT VOID stop_timer();		/* stop timer */

IMPORT CHNLINFO *ap_pcis;		/* pointer to channel information
					 * structure. Set when called from
    					 * application.
					 */ 
IMPORT CHNLINFO cis[];			/* channel information structure
    					 * array.
    					 */
IMPORT UBYTE app_chnl;			/* application channel number */
IMPORT UBYTE log_chnl;			/* logical channel number */
IMPORT UWORD free_count;		/* number of free buflets */
#ifdef DEBUG
IMPORT WORD diags[];			/* used for debugging.*/
#endif

/************************************************************************
 * VOID recv_ssnacp(plcis, ppkt)
 *    LCIINFO *plcis    - pointer to logical channel information structure
 *    BUFLET *ppkt      - pointer to incoming session accept packet
 *
 *    recv_ssnacp processes an incoming session accept packet.  
 *    The pointer to the session accept packet is stored in the 
 *    channel information structure.  
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID recv_ssnacp(plcis, ppkt)
    LCIINFO *plcis;			/* pointer to logical information
    					 * structure.
    					 */
    BUFLET *ppkt;                       /* pointer to packet */
    {
    FAST CHNLINFO *pcis;		/* pointer to channel information
    					 * structure.
    					 */
    

    
    pcis = &cis[plcis->appchnl];
    
    /* Stop the session request timer.  This timer is set when a
     * session request packet is sent.
     */
    stop_timer(TEN_SEC_TIMER, TIM_T21, log_chnl);
    
    /* Interrupts were disabled by process_pkt(), now enable them.
     */
    int_enable();

    /* Save session accept packet pointer for read session data 
     * function and set index to data. 
     */
    pcis->ssndatapkt = ppkt;
    pcis->idxssndata = EXTRA_DATA;
    
    /* Update channel state to call accepted.
     */
    pcis->chnlstate = CHNL_CALL_ACP;
    pcis->lcistruct->ssnstate = S4;     /* data transfer state */
    }
 

/************************************************************************
 * INT send_ssnacp()
 *
 *    send_ssnacp builds a session accept packet.  A q-bit control 
 *    packet is built, data is moved from the application parameter
 *    buffer into the  packet, and the packet is added to the pad
 *    output queue.  
 *
 * Usage notes: The subroutine send_ctrl_data_pkt assumes that the
 *    session accept data is in parameter 1.  If this changes, 
 *    then the subroutine send_ctrl_data_pkt must be updated.
 *
 * Returns:  
 *    SUCCESS  - Processing was successful.    
 *    ILLEGAL_CHANNEL FUNCTION - Channel is in incorrect state, or the
 *        application specified channel 0.
 *    ILLEGAL_CALL_REQ - Too many  characters in the session accept packet.
 *         or the output format is invalid.
 *
 ************************************************************************/
INT send_ssnacp()
    {
    WORD num;                           /* number of bytes in buffer */
    WORD format;                        /* buffer format */
 


    /* If channel state is invalid, return error. Otherwise, move
     * data from application parameter buffer into packet and
     * add packet to pad output queue.
     */
    if (ap_pcis->chnlstate != CHNL_PEND_ACP  || app_chnl == 0)
        return (ILLEGAL_CHNL_FUNC);
    mov_param((UBYTE *)&num, sizeof(UWORD), PARAM_2, 0, FROM_APPL);
    
    /* Running out of buflets, so the session accept is not sent. 
     */
    if (free_count < MIN_REQUIRED_BUFLETS)
	return (ILLEGAL_CALL_REQ);
    mov_param((UBYTE *)&format, sizeof(UWORD), PARAM_3, 0, FROM_APPL);
    
    /* The packet length or format is invalid.
     */
    if (num > SSN_DATA_LEN || format != TYMNET_STRING)
        return (ILLEGAL_CALL_REQ);
    
    /* Send the session accept packet.
     */
    (VOID)send_ctrl_data_pkt(ap_pcis->lcistruct, ap_pcis->logicalchnl, 
	SESSION_ACP, num, CHECK_LINK);
    
    /* Update the channel state to connected.
     */
    ap_pcis->chnlstate = CHNL_CONNECTED;
    ap_pcis->lcistruct->ssnstate = S4;  /* data transfer state */

    return (SUCCESS);
    }


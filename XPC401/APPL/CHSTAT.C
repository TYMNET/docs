/************************************************************************
 * chstat.c - Return Status of Device When In Character State
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    chstat.c contains the subroutines which are used to 
 *    get the status of the device when device is in 
 *    character state.   
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
#include "device.h"
#include "error.h"

IMPORT VOID mov_param();		/* move data to/from application
					 * parameter buffer.
					 */

IMPORT CHNLINFO *ap_pcis;		/* current pointer to channel 
					 * information structure.
					 */
IMPORT DEVSTAT dev_stat;                /* device status structure */
IMPORT UWORD free_count;		/* number of free buflets */ 
/************************************************************************
 * INT rpt_char_iostat()
 *
 *    rpt_char_iostat() returns the current I/O status for 
 *    channel 0.  rpt_char_iostat() is called to report status
 *    when device is in character state.
 *
 * Usage notes: rpt_char_iostat assumes that ap_pcis is pointing to
 *    channel 0's channel information structure and that
 *    the channel information structure is linked to the logical
 *    channel information structure.
 *
 * Returns:  If no  port is  active, rpt_char_iostat returns
 *    ILLEGAL_PORT.  Otherwise, rpt_char_iostat returns SUCCESS.
 *
 ************************************************************************/
INT rpt_char_iostat()
    {
    WORD inp_status = INPUT_NORMAL;	/* input status */
    WORD out_status = NO_OUTPUT;	/* output status */ 

    /* Check if the port is active.  If not return an error.
     */
    if (dev_stat.portaddr == 0)
	return(ILLEGAL_PORT);
	
    /* Get the number of bytes in the application read queue.
     */
    mov_param((UBYTE *)&ap_pcis->nreadbytes, sizeof(WORD), PARAM_1, 0, 
	TO_APPL);
    
    /* Set up the output status for channel zero.  Channel 0 is in a flow
     * control state when the number of buflets available for output or the
     * output queue is full.
     */ 
    if (free_count < CWD_BUFLETS)
	out_status = FLOWCTRL_OUTPUT;
    else if (ap_pcis->lcistruct->outpktqueue.begqueue)
	if (ap_pcis->nwritebytes > MAX_WRITE_BYTES)
	    out_status = FLOWCTRL_OUTPUT;
	else
	    out_status = NO_FLOWCTRL_OUTPUT;
    
    /* Establish input status.
     */
    if (ap_pcis->breakstatus)
	inp_status = BREAK_BIT;
    if (ap_pcis->errorstatus)
	inp_status |= ERROR_BIT;
    
    /* Move the statuses into the application parameter buffers.  Clear
     * the input statuses.
     */
    mov_param((UBYTE *)&inp_status, sizeof(WORD), PARAM_2, 0, TO_APPL);
    mov_param((UBYTE *)&out_status, sizeof(WORD), PARAM_3, 0, TO_APPL);
    ap_pcis->errorstatus = ap_pcis->breakstatus = 0;
    return (SUCCESS);
    }

/************************************************************************
 * INT c_chnl_status()
 *
 *    c_chnl_status() moves the  channel status of channel 0 into the 
 *    application parameter buffer.
 *
 * Usage notes: c_chnl_status assumes that ap_pcis is pointing to
 *    channel 0's channel information structure and that
 *    the channel information structure is linked to the logical
 *    channel information structure.
 *
 * Returns:  c_chnl_status() always returns SUCCESS.
 *
 ************************************************************************/
INT c_chnl_status()
    {
    WORD state, code;
    
    /* If channel status is requested for channel 0, then 
     * move channel status into application parameter buffer.
     */
    state = (WORD)ap_pcis->chnlstate;
    code = 0;
    if (ap_pcis->chnlstate == CHNL_CALL_CLEARED)
	{
	code = (WORD)ap_pcis->clearcode;
	ap_pcis->chnlstate = CHNL_CONNECTED;
	}

    /* move the status of channel zero from the channel structure
     * into the application parameter packet.
     */      
    mov_param((UBYTE *)&state, sizeof(WORD), PARAM_1, 0, TO_APPL);
    mov_param((UBYTE *)&code, sizeof(WORD), PARAM_2, 0, TO_APPL);
    return (SUCCESS);
    }
 

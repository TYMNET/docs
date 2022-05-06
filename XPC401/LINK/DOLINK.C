/************************************************************************
 * dolink.c - link processing
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *     This module contains functions to process link I/O via the timer
 *     interrupt.
 *     
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KJB   Initial Draft
 *
 ************************************************************************/
#include "stddef.h"
#include "xpc.h"
#include "pkt.h"
#include "device.h"
#include "iocomm.h"
#include "state.h"
#include "timer.h"

IMPORT VOID c_link();			/* character mode I/O processor */
IMPORT VOID p_link();			/* packet mode I/O processor */
IMPORT VOID exec_interrupt();		/* execute an interrupt */
IMPORT VOID store_word();		/* store the word */

IMPORT CHNLINFO cis[];			/* channel information structure */
IMPORT COMMINFO comm_info;		/* communications I/O structure */
IMPORT DEVSTAT dev_stat;		/* device status structure */
IMPORT UBYTE prev_mdmstatus;		/* previous modem status */
/************************************************************************
 * VOID do_link()
 *
 *     do_link is called by the timer interrupt service routine to
 *     process link I/O based on the current link type.
 *
 * Notes: do_link runs with interrupts enabled.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID do_link()
    {

    /* If modem status has changed and check point is active,
     * perform interrupt.
     */
    if (comm_info.mdmstatus != prev_mdmstatus)
	{
	if ((comm_info.mdmstatus & MSR_RSLD) != (prev_mdmstatus & MSR_RSLD))
	    if ((comm_info.mdmstatus & MSR_RSLD) == 0)
		{
		cis[0].clearcode = MODEM_STATUS_LOST;
		cis[0].chnlstate = CHNL_CALL_CLEARED;
		}
	if (cis[0].checkevent != 0)
	    {
	    if (cis[0].checkevent == UPDATE_ON)
		store_word(cis[0].checkeventseg, cis[0].checkeventoff, 
		    comm_info.mdmstatus);
	    else
		exec_interrupt(cis[0].checkeventseg, cis[0].checkeventoff);
	    }
	}
    prev_mdmstatus = comm_info.mdmstatus;
	    
    /* if the device is in character mode, perform character mode link I/O
     */
    if (dev_stat.devstate == CHAR_STATE)
	c_link();

    /* else if the device is in packet mode, perform packet mode link I/O
     */
    else if (dev_stat.devstate == PKT_STATE)
	p_link();
    }

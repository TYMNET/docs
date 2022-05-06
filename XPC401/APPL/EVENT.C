/************************************************************************
 * event.c - Set or Reset Timer/Checkpoint  Events
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    event.c contains the modules which are called to set or
 *    reset a timer or checkpoint event.  The set/reset update
 *    functions update the specified event and the event offset
 *    address.  The segment of the event is assumed to be that
 *    of the application program.  The set/reset interrupt updates
 *    the specified event code, the event segment address and the
 *    event offset address.
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
#include "device.h"
#include "appl.h"
#include "state.h"
#include "timer.h"
#include "pkt.h"
#include "error.h"
 
IMPORT INT send_ctrl_pkt();		/* send control packet */
IMPORT VOID tim_event();		/* function called for timer */
IMPORT VOID mov_param();		/* move data to/from application
					 * parameter buffer.
					 */
IMPORT VOID start_timer();		/* start timer */

IMPORT CHNLINFO *ap_pcis;		/* pointer to channel information
					 * structure.
					 */
IMPORT CHNLINFO cis[];			/* channel information structure */
IMPORT DEVSTAT dev_stat;                /* device status structure */
IMPORT UBYTE app_chnl;			/* application channel number */
IMPORT UWORD free_count;		/* number of free buflets */
IMPORT UWORD param_block[];		/* Array of parameter pointers */
 

/************************************************************************
 * LOCAL INT set_event(addr_off, addr_seg, event_type)
 *    WORD addr_off;	- address offset
 *    WORD addr_seg;    - address segment
 *    WORD event_type;  - event type
 *
 *    set_event() sets up an update or an interrupt for the event specified by
 *    the application. The event code, offset address and segment address
 *    are extracted from the application parameter structure.  If the
 *    event is a timer event, the number of timer ticks is extracted
 *    from the application parameter structure and the timer is started.
 *    The event code, segment address and offset address are moved into
 *    the channel information structure.
 *
 * Returns:  
 *    SUCCESS - Processing Successful
 *    ILLEGAL_PORT - No port is active
 *    ILLEGAL_CHAR_FUNC - set_event of checkpoint is invalid when in
 *        character state.
 *    ILLEGAL_CHNL_FUNC - set_event of checkpoint is invalid when channel
 *        is not connected or application specified channel 0.
 *    ILLEGAL_EVENT_CODE - The event code is not a timer event or checkpoint
 *        event.
 *    TIMER_REQ_IGNORED - The timer is already running i.e. the offset
 *        and segment addresses are non-zero.
 *    CHECKPOINT_ACTIVE - A checkpoint is already active.
 *
 ************************************************************************/
LOCAL INT set_event(addr_off, addr_seg, timelen, event_type)
    WORD addr_off;			/* Address offset */
    WORD addr_seg;			/* Segment address */
    WORD timelen;			/* Timer length */
    WORD event_type;			/* type of event, update or 
    					 * interrupt.
    					 */
    {
    WORD event_code = 0;		/* event code */

    /* If no port is active, return an error.
     */
    if (dev_stat.portaddr == 0)
        return (ILLEGAL_PORT);

    /* Extract the event code and  event offset address 
     * from the application parameter buffer.
     */
    mov_param((UBYTE *)&event_code, sizeof(WORD), PARAM_1, 0, FROM_APPL);
    
    /* The event specified by application is a timer event. Get the
     * number of timer durations and start timer.
     */
    if (event_code == TIMER_EVENT)
	{
	
	/* If the timer event is already active, then return an error.
	 */
	if (ap_pcis->timereventoff != 0 || ap_pcis->timereventseg != 0)
	    return (TIMER_REQ_IGNORED);
	
	/* Save the timer event code and the timer offset address.  The
	 * segment address is the same as the segment in the parameter
	 * block.
	 */
	ap_pcis->timerevent = event_type;
    	ap_pcis->timereventoff = addr_off;
    	ap_pcis->timereventseg = addr_seg;
    	
	/* Get the length of the timer event.  It must be a value greater than
	 * 0.  Start the timer.
	 */
        if (timelen == 0)
	    timelen = 1;
	start_timer(SIXTH_SEC_TIMER, TIM_USER, ap_pcis->logicalchnl, 
	    timelen, tim_event);
    	}
    
    /* The event was a check point.  If the device is in the wrong
     * state return. Otherwise, set up the check point.
     */
    else if (event_code == CHECK_EVENT)
	{
	
	/* A checkpoint event is only valid when device is in packet
	 * state and the channel is connected.
	 */
	if (dev_stat.devstate != PKT_STATE)
	    return(ILLEGAL_CHAR_FUNC);
	if (app_chnl == 0 || ap_pcis->chnlstate != CHNL_CONNECTED)
	    return(ILLEGAL_CHNL_FUNC);
	
	/* If the check point event is already active, return an
	 * error.
	 */
	if (ap_pcis->checkeventoff != 0 || ap_pcis->checkeventseg != 0)
	    return (CHECKPOINT_ACTIVE);
	
	/* If the number of free buflets is low, then return SUCCESS 
	 * but don't set up the check point event.
	 */
	if (free_count < MIN_REQUIRED_BUFLETS)
	    return(SUCCESS);             /*****THIS HAS TO CHANGE ******/
	
	/* Set up the checkpoint event.  Use the segment address which is
	 * contained in the application parameter block.
	 */
	ap_pcis->checkevent = event_type;
    	ap_pcis->checkeventoff = addr_off;
    	ap_pcis->checkeventseg = addr_seg;
	(VOID)send_ctrl_pkt(ap_pcis->lcistruct, ap_pcis->logicalchnl,
	    YELLOW_BALL, (UBYTE *)0, MUST_LINK);
	}
    /* Modem Event.  Modem event is only for channel 0.
     */
    else if (event_code == MODEM_EVENT)
	{
	
	/* If there already is a check point or modem event, then
	 * return an error.
	 */
	if (cis[0].checkeventoff != 0 || cis[0].checkeventseg != 0)
	    return (CHECKPOINT_ACTIVE);
	
	/* Set up the modem event using the segment address from the
	 * application parameter block.
	 */
	cis[0].checkevent = event_type;
    	cis[0].checkeventoff = addr_off;
    	cis[0].checkeventseg = addr_seg;
	}
	
    /* The event is not defined.
     */
    else
	return (ILLEGAL_EVENT_CODE);
    return (SUCCESS);
    }

/************************************************************************
 * LOCAL VOID reset_event()
 *
 *    reset_event() clears an update or interrupt event.  The event code, 
 *    segment address and offset address are cleared for the event 
 *    specified by the application.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
LOCAL VOID reset_event()
    {
    WORD event_code = 0;		/* event code */

    
    /* Find out which event is to be reset.
     */
    mov_param((UBYTE *)&event_code, sizeof(WORD), PARAM_1, 0, FROM_APPL);
    
    /* Clear the timer event.
     */
    if (event_code == TIMER_EVENT)
	{
	ap_pcis->timerevent = 0;
	ap_pcis->timereventseg = ap_pcis->timereventoff = 0;
	}
    
    /* Clear the check point event for specified channel.
     */
    else if (event_code == CHECK_EVENT) 
	{
        ap_pcis->checkevent = 0;
	ap_pcis->checkeventseg = ap_pcis->checkeventoff = 0;
	}
    
    /* Clear the modem event for channel 0
     */
    else if (event_code == MODEM_EVENT)
	{
	cis[0].checkevent = 0;
	cis[0].checkeventseg = cis[0].checkeventoff = 0;
	}
    }

/************************************************************************
 * INT set_update()
 *
 *    set_update() initializes an interrupt event.  The event code
 *    and the interrupt offset address are extracted from the
 *    application parameter structure.  The interrupt segment address
 *    is assumed to be the segment address in the application parameter
 *    packet.  If the event code is a timer event, the number of 
 *    timer ticks is also extracted from the application parameter
 *    buffer.  The event code, segment address and offset address
 *    are moved into the channel information structure.
 *
 * Returns:  
 *    SUCCESS - Processing Successful
 *    ILLEGAL_PORT - Port is not active.
 *    ILLEGAL_CHAR_FUNC - set_update of checkpoint is invalid when in
 *        character state.
 *    ILLEGAL_CHNL_FUNC - set udpate of checkpoint is invalid when channel
 *        is not connected or application specified channel 0.
 *    ILLEGAL_EVENT_CODE - The event code is not a timer event or checkpoint
 *        event.
 *    TIMER_REQ_IGNORED - The timer is already running i.e. the offset
 *        and segment addresses are non-zero.
 *    CHECKPOINT_ACTIVE - A checkpoint is already running.
 *
 ************************************************************************/
INT set_update()
    {
    WORD addr_off = 0;			/* event off */
    WORD timelen = 0;			/* time length */

    /* Extract the event offset address 
     * from the application parameter buffer.
     */
    mov_param((UBYTE *)&addr_off, sizeof(WORD), PARAM_2, 0, FROM_APPL);
    mov_param((UBYTE *)&timelen, sizeof(WORD), PARAM_3, 0, FROM_APPL);
    return(set_event(addr_off, (WORD)param_block[0], timelen, UPDATE_ON));
    }

/************************************************************************
 * INT reset_update()
 *
 *    reset_update() clears the specified event. The event code
 *    segment address and offset address are cleared for the
 *    event specified by the application.
 *
 * Returns: reset_update always returns SUCCESS.  
 *
 ************************************************************************/
INT reset_update()
    {
    
    reset_event();
    return (SUCCESS);
    }
    
/************************************************************************
 * INT set_interrupt()
 *
 *    set_interrupt() sets up an interrupt for the event specified by
 *    the application. The event code, offset address and segment address
 *    are extracted from the application parameter structure.  If the
 *    event is a timer event, the number of timer ticks is extracted
 *    from the application parameter structure and the timer is started.
 *    The event code, segment address and offset address are moved into
 *    the channel information structure.
 *
 * Returns:  
 *    SUCCESS - Processing Successful
 *    ILLEGAL_PORT - Port has not been initialized in device structure.
 *    ILLEGAL_CHAR_FUNC - set_interrupt of checkpoint is invalid when in
 *        character state.
 *    ILLEGAL_CHNL_FUNC - set_interrupt of checkpoint is invalid when channel
 *        is not connected or application specified channel 0.
 *    ILLEGAL_EVENT_CODE - The event code is not a timer event or checkpoint
 *        event.
 *    TIMER_REQ_IGNORED - The timer is already running i.e. the offset
 *        and segment addresses are non-zero.
 *    CHECKPOINT_ACTIVE - A checkpoint is already active.
 *
 ************************************************************************/
INT set_interrupt()
    {
    WORD addr_seg = 0;			/* event seg */
    WORD addr_off = 0;			/* event off */
    WORD timelen = 0;			/* timer length */

    mov_param((UBYTE *)&addr_seg, sizeof(WORD), PARAM_2, 0, FROM_APPL);
    mov_param((UBYTE *)&addr_off, sizeof(WORD), PARAM_3, 0, FROM_APPL);
    mov_param((UBYTE *)&timelen, sizeof(WORD), PARAM_4, 0, FROM_APPL);
    return(set_event(addr_off, addr_seg, timelen, INTERRUPT_ON));
    }

/************************************************************************
 * INT reset_interrupt()
 *
 *    reset_interrupt() clears the specified event. The event code
 *    segment address and offset address are cleared for the
 *    event specified by the application.
 *
 * Returns: reset_interrupt() always returns SUCCESS.  
 *
 ************************************************************************/
INT reset_interrupt()
    {
    
    reset_event();
    return (SUCCESS);
    }


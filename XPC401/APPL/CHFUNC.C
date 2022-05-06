/************************************************************************
 * ch_func.c - Application Functions For Character State
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    ch_func.c contains the modules needed to process application
 *    functions when device is in character state.  The processing
 *    of these functions is different for device which is not
 *    in character state.
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
#include "pkt.h"
#include "iocomm.h"
#include "state.h"
#include "param.h"
#include "timer.h"
#include "error.h"
 
IMPORT BYTES cpybuf();			/* copy buffer to buffer */
IMPORT VOID c_start_break();		/* start break */
IMPORT VOID init_new_state();		/* change to/from character and
					 * packet states.
					 */
IMPORT VOID init_pad();			/* initialize channels */
IMPORT VOID reset_break();		/* reset the break */
IMPORT VOID set_break();		/* set the break */
IMPORT VOID set_port();			/* set port parameters */
IMPORT VOID start_timer();		/* start the timer */
IMPORT VOID stop_timer();		/* stop the timer */
IMPORT VOID tim_break();		/* break time out routine */

IMPORT BOOL restart_flag;		/* restart the system */
IMPORT CHNLINFO *ap_pcis;	        /* pointer to channel information
				         * structure.
					 */
IMPORT CHNLINFO cis[];			/* channel information structure */
IMPORT COMMINFO comm_info;		/* comm info structure */
IMPORT DEVSTAT dev_stat;                /* device status structure */
IMPORT LCIINFO lcis[];			/* array of logical channel 
					 * information structures.
					 */
IMPORT PORTPARAMS port_params;		/* port parameter structure */

#ifdef DEBUG
IMPORT WORD diags[];
#endif
LOCAL BOOL break_flag = NO;		/* break flag */
 
/************************************************************************
 * INT c_set_charstate()
 *
 *    c_set_charstate() processes set character state when device is
 *    already in a character state.  Nothing is done.
 *
 * Returns:  c_set_charstate() always returns SUCCESS.
 *
 ************************************************************************/
INT c_set_charstate()
    {
    
    return (SUCCESS);
    }

/************************************************************************
 * INT c_set_pktstate()
 *
 *    c_set_pktstate() changes device state from character state to
 *    packet state.
 *
 * Usage notes: c_set_pktstate() assumes that ap_pcis is pointing to
 *    channel 0's channel information structure and that
 *    the channel information structure is linked to the logical
 *    channel information structure.
 *
 * Returns:  c_set_pktstate() always returns SUCCESS.
 *
 ************************************************************************/
INT c_set_pktstate()
    {
    PORTPARAMS tport;			/* temporary port parameter 
					 * structure.
					 */
    UWORD i;				/* counter */

    /* set the state for channel 0 to connected (CS0).  
     */
    ap_pcis->chnlstate = CHNL_CONNECTED;
    
    /* Update the portparameters to ensure that there is no parity,
     * xon-xoff is off and there are 8 data bits.
     */
    (VOID)cpybuf((UBYTE *)&tport, (UBYTE *)&port_params, 
	(BYTES)sizeof(PORTPARAMS));
    tport.databits = DATA_BIT_8;
    tport.parity = NO_PARITY;
    tport.xonxoff = XON_OFF;
    set_port(&tport);
    
    /* Make sure that the transmit assembly queues are cleared before going
     * from character to packet state.
     */
    init_new_state();

    /* Initialize the entries in the logical channel information
     * structure and the channel information structure which
     * are used by a session.
     */
    if (dev_stat.portaddr != 0)
	init_pad(-1, YES, NO);
    
    /* Initialize the index for character read.  The  starting data
     * position differs between character and packet states.
     */
    for (i = 1; i <= MAX_CHNL; ++i)
        cis[i].idxreadqueue = MOVED_FIRST_BYTE;
    comm_info.xmtidx = 0;
    
    
    /* Set the device state to packet mode.
     */
    dev_stat.devstate = PKT_STATE;

    /* If this is a DTE, then set the restart flag which will cause
     * link to output a RESTART packet. If we a DCE, then set the
     * restart state to waiting for restart.
     */
    if (port_params.dxemode == DTE_MODE)
	{
        restart_flag = YES;
	while (restart_flag)
	    ;
	}
    else
	lcis[0].restartstate = R0;
    return (SUCCESS);
    }
    

 
/************************************************************************
 * INT c_ill_func()
 *
 *    c_ill_func() returns an error for application functions which
 *    are invalid in character state.
 *
 * Returns:  c_ill__func always returns ILLEGAL_CHAR_FUNC.
 *
 ************************************************************************/
INT c_ill_func()
    {
 
    return (ILLEGAL_CHAR_FUNC);
    }
 
 
/************************************************************************
 * INT send_char_brk()
 *
 *    send_chark_brk() executes a break when the device is in character 
 *    state.
 *
 * Returns:  If the port is not currently active, send_char_brk()
 *    returns ILLEGAL_PORT.  Otherwise, send_char_brk() returns
 *    SUCCESS.
 *
 ************************************************************************/

INT send_char_brk()
    {
    
    /* If the port is not active, return an error.
     */
    if (dev_stat.portaddr == 0)
	return(ILLEGAL_PORT);
    
    /* start the character break.
     */
    c_start_break();
    return(SUCCESS);
    }
 
/************************************************************************
 * VOID c_start_break()
 *
 *    c_start_break() calls set_break() to start the break, clears
 *    the break flag, and starts the break timer.  When the
 *    timer expires it sets the break flag and c_start_break()
 *    can return.
 * 
 * Usage Notes: c_start_break() assumes that the break flag is
 *    going to be set by tim_break().  If it not set by tim_break()
 *    c_start_break() will loop forever.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID c_start_break()
    {
    
    /* Call set break and set a timer which will go off when break
     * is done, reset the break and exit.
     * When timeout is done, c_start_break returns.
     */
    set_break();
    break_flag = NO;
    
    /* Start the character break timer.
     */
    start_timer(SIXTH_SEC_TIMER, TIM_BREAK, 0, 2, tim_break);
    
    /* When the break flag is turned on because tim_break was called,
     * then stop processing.
     */
    while (!break_flag)
        ;
    }
    
/************************************************************************
 * VOID tim_break(ptim)
 *    TIMER *ptim;	- pointer to the timer structure
 *
 *    tim_break() stops the break timer, calls reset_break()
 *    to end the character break and sets the break flag so
 *    the c_start_break() can exit.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID tim_break(ptim)
    TIMER *ptim;
    {
    
    /* Stop the character break timer.
     */
    stop_timer(SIXTH_SEC_TIMER, TIM_BREAK, idtochnl(ptim->timid));
    
    /* Reset the character break.
     */
    reset_break();
    
    /* Clear character break condition.
     */
    break_flag = YES;
    }



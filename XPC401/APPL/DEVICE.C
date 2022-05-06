/************************************************************************
 * device.c  - Process Device Status Functions
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    device.c contains the modules which are called to clear the
 *    current device, reset the current device and report the
 *    status of the current device.  
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
#include "appl.h"
#include "device.h"
#include "state.h"
#include "param.h"
#include "iocomm.h"
#include "timer.h"
#include "error.h"

IMPORT BYTES cpybuf();			/* copy buffer to buffer */
IMPORT INT enable_comm();		/* enable communications interrupts */
IMPORT INT prc_reset_device();		/* reset the device */
IMPORT INT set_dtr();			/* set data terminal ready */
IMPORT INT set_rts();			/* set ready to send */
IMPORT VOID app_initialization();	/* application interface initialize */
IMPORT VOID dec_timer();		/* decrement timer */
IMPORT VOID disable_comm();		/* disable communications interrupts */
IMPORT VOID disable_timer();		/* disable timers */
IMPORT VOID enable_timer();		/* enable timer */
IMPORT VOID flush_all_queues();		/* flush all of the queues */
IMPORT VOID init_new_state();		/* change from/to character and packet
					 * states.
					 */
IMPORT VOID init_pad();	                /* flush queues */
IMPORT VOID mov_param();                /* move data to/from application
					 * buffer.
					 */
IMPORT VOID reset_modem_ctrl();		/* clear modem control */
IMPORT VOID reset_timers();		/* reset the timers */
IMPORT VOID set_port();			/* set port parameters */
IMPORT VOID start_timer();		/* start the timer */
IMPORT VOID stop_timer();		/* stop timer */

IMPORT BOOL restart_flag;		/* restart is being performed */
IMPORT BOOL use_timer_int;		/* set timer intvec on Reset? */
IMPORT BOOL xpc_timer_active;		/* timer interrupts active? */
IMPORT CHNLINFO cis[];                  /* channel information structure */
IMPORT COMMINFO comm_info;		/* comm info structure */
IMPORT DEVSTAT dev_stat;                /* device status structure */
IMPORT PORTPARAMS default_params;	/* initial port params */
IMPORT PORTPARAMS port_params;		/* current port params */
#ifdef DEBUG
IMPORT WORD diags[];			/* used for debugging. */
#endif
	
/************************************************************************
 * INT clr_device()
 *
 *    clr_device() calls prc_reset_device() to reset the port 
 *    which is currently active.
 *
 * Returns:  clr_device() always returns SUCCESS.
 *
 ************************************************************************/
INT clr_device()
    {

    /* If the device is not in a device reset state, then
     * clear the device.  If the device is currently in
     * packet state, then send a device RESTART packet.
     * Processing should stop until the restart packet has
     * been sent out.
     */
    
    if (dev_stat.portaddr != 0 && dev_stat.devstate != RESET_STATE)
	{
	
	/* If device is currently in a packet state, send a restart
	 * packet.  Processing stops until the restart packet is
	 * in the link output queue.
	 */
	if (dev_stat.devstate == PKT_STATE)
	    {
	    restart_flag = YES;
	    while (restart_flag)
		;
	    }
	
	/* Reset the port parameters to their default values.
	 */
        (VOID)cpybuf((UBYTE *)&port_params, (UBYTE *)&default_params, 
	    (BYTES)sizeof(PORTPARAMS));
	
	/* Reset the current device.
	 */
	(VOID)prc_reset_device((WORD)0);
	
	/* Set the current device state to RESET.
	 */
	dev_stat.devstate = RESET_STATE;
	}
    return (SUCCESS);
    }
 
/************************************************************************
 * INT reset_device()
 *
 *    reset_device() calls prc_reset_device() to either reset
 *    the currently active device, or to initialize the port
 *    which was specified by the application.
 *   
 * Returns:  
 *    SUCCESS - reset_ device encountered no errors.
 *    ILLEGAL_PORT - Port is not active.
 *    PORT_ALREADY_ACTIVE - reset_device called with non-zero  
 *        port address and the current port address is not zero.
 *
 ************************************************************************/
INT reset_device()
    {
    DEVSTAT tdev_stat;                  /* temporary device status structure */

    /* Get the port address from the application device status structure.
     */
    mov_param((UBYTE *)&tdev_stat, sizeof(DEVSTAT), PARAM_1, 0, FROM_APPL);
    
    /* Validate the application port address.
     */
    if (tdev_stat.portaddr < NO_PORT || tdev_stat.portaddr > COM2)
        return (ILLEGAL_PORT);
    return(prc_reset_device(tdev_stat.portaddr));
    }

/************************************************************************
 * INT prc_reset_device(newport)
 *    UBYTE newport;			 The new port address. If it
 *					 is zero, then the current port
 *					 is cleared. Otherwise, the 
 *					 specified port is initialized.
 *					
 *
 *    prc_reset_device() resets the port.  If the port is being started
 *    the port parameters are  initialized, the communications interrupts
 *    are enabled,  the logical channel information structure is initialized
 *    the timers are initialized, the one second timer is started,
 *    and the device is set to character state.  If the current port is 
 *    being cleared, the communications interrupts are disabled, the timers
 *    are cleared, the one second timer is stopped, the port parameter 
 *    structure is intialized, dtr and rts controls are set in the 
 *    modem control register, the queues are flushed, and the channel
 *    information structure is cleared.
 *   
 * Returns:  
 *    SUCCESS - reset device processing encountered no errors.
 *    ILLEGAL_PORT - Port is not active.
 *    PORT_ALREADY_ACTIVE - reset_device called to with non-zero  
 *        port address and the current port address is not zero.
 *
 ************************************************************************/
INT prc_reset_device(newport)
    WORD newport;			/* new port address */
    {
	
    /* The new port  is not currently active.
     */
    if (dev_stat.portaddr == 0)    
        {
        
        /* A port is being initialized.  Set the port parameters, 
         * initialize timers, enable communications interrupts
         * initialize logical channel information structure, 
         * start the one second timer and connect channel zero.
         * The device state is set to charcter.
         */
        if (newport != 0)
            {
    
	    /* Initialize index into circular read buffer.
	     */
	    comm_info.xmtidx = 1;
	    cis[CM_CHNL].idxreadqueue = 1;
	    
	    /* Connect channel 0 and set device to character state.
	     */
            cis[CM_CHNL].chnlstate = CHNL_CONNECTED;
	    dev_stat.devstate = CHAR_STATE;
            
	    /* Change the port address.
	     */
            dev_stat.portaddr = newport;
	    
	    /* Set up the port parameters.
	     */
	    set_port(&port_params);
	    
	    /* Stop all active timers and reinitialize timer lists array.
	     */
	    reset_timers();

	    /* Enable communications interrupts.
	     */
	    (VOID)enable_comm();
	    
	    /* Initialize the pad queues.
	     */
	    init_pad(-1, YES, NO);
	    
	    /* If the timer interrupt is being used, set the timer interrupt.
	     */
	    if (use_timer_int)
		enable_timer();
	    
	    /* Set the timer interrupt active.
	     */
	    xpc_timer_active = YES;
	    
	    /* Start the one second timer.
	     */
	    start_timer(SIXTH_SEC_TIMER, TIM_ONESEC, NULLUBYTE,
	    	LEN_ONESEC, dec_timer);

	    /* Set DTR and RTS to conform to the old driver.
	     * This is a modification to the Driver Interface Spec.
	     */
	    (VOID)set_dtr();
	    (VOID)set_rts();
            }
	
	/* clearing device which is already cleared.  No processing
 	 * is done.
	 */
	else
	    return(SUCCESS);
	}
	    
    /* New port address is zero.
     */
    else if (newport == 0)   /* new device is zero */
        {
	
	/* Set a flag, timer interrupt is not active.
	 */
	xpc_timer_active = NO;
	
	/* This call contains redundant code, but is called mainly
	 * to clear iocomm's transmit queue and receive buffer.
	 */
	init_new_state();
	
	/* Disable communications interrupts, stop timers, initialize
         * stop the one second timer, and initialize the port parameters.
         */ 
        disable_comm();
	
	/* If the timer interrupt is being used, the reset the timer
	 * interrupt.
	 */
	if (use_timer_int)
	    disable_timer();
	
	/* Stop the one second timer.
	 */
        stop_timer(SIXTH_SEC_TIMER, TIM_ONESEC, 0);
	
	/* Set port parameters.
	 */
	set_port(&port_params);
	
	/* store the new port address.
	 */
	dev_stat.portaddr = newport;
	
	/* Set DTR and RTS.
	 */
	reset_modem_ctrl(MCR_DTR | MCR_RTS);
	
	/* Flush all of the application, pad and link queues.  It is ok
	 * to flush the queues because communications interrupts have
	 * been disabled.
	 */
	flush_all_queues();
	
	/* Initialize all of the global data.  This routine is called to
	 * initialize all data including data used by link.  This is
	 * possible because interrupts are disabled.
	 */
	app_initialization();
	
	/* Set state to character. If this is a clear device, the
	 * state is changed to WAIT_RESET by clr_device.
	 */
	dev_stat.devstate = CHAR_STATE;
        }
    
    /* current port is not zero and new port is not zero (error)
     */
    else
        return (PORT_ALREADY_ACTIVE);
    return (SUCCESS);
    }

/************************************************************************
 * INT read_device()
 *
 *    read_device() moves the status of the device into the application
 *    parameter buffer.
 *
 * Returns:  read_device() always returns SUCCESS.
 *
 ************************************************************************/
INT read_devstatus()
    {

	
    /* Move the device state from the device status structure to the
     * application parameter buffer.
     */
    mov_param((UBYTE *)&dev_stat, sizeof(DEVSTAT), PARAM_1, 0, TO_APPL);    
    return (SUCCESS);
    }

    


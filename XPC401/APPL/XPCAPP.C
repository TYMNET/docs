
/************************************************************************
 * xpcapp.c - Application Driver Interface
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    xpcapp.c contains the application driver interface which
 *    is called by the the assembly module  appint.asm with
 *    the device number, channel number and function which have
 *    been extracted from the application parameters.
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
#include "state.h"
#include "error.h"

	
IMPORT INT jmp_char_funcs();           	/* call character functions */
IMPORT INT jmp_pkt_funcs();            	/* call packet functions */
IMPORT INT jmp_reset_funcs();          	/* call reset functions */
IMPORT VOID init_pad();			/* initialize the pad */
IMPORT VOID int_disable();		/* disable interrupts */
IMPORT VOID int_enable();    		/* enable interrupts */
IMPORT BOOL app_active;			/* application is active */
IMPORT BOOL clear_pad;			/* flag which is set by link
					 * when restarting, the pad
					 * must be cleared before the
					 * application exits.
					 */
IMPORT BOOL first_time;                 /* first time called */
IMPORT CHNLINFO cis[];		        /* array of channel information
				         * structures.
					 */
IMPORT CHNLINFO *ap_pcis;		/* pointer to channel information
				         * structure for channel specified
					 * by user.  If not in packet
					 * state, pointer to set to 
					 * channel zero.
					 */
IMPORT DEVSTAT dev_stat;                /* device status structure */
IMPORT LCIINFO lcis[];			/* logical channel information
					 * structure.
					 */
IMPORT UBYTE app_chnl;		   	/* application channel no. */
IMPORT UWORD free_count;		/* number of free buflet chains */
IMPORT UWORD program_error;             /* programing error code */
IMPORT WORD device_nbr;                 /* logical device number */ 

#ifdef DEBUG
IMPORT LCIINFO lcis[];			/* logical channel information 
					 * structure.
					 */ 
IMPORT WORD diags[];			/* array of diagnostices which
				 	 * can be displayed using
					 * the function for link
					 * statistics
					 */
LOCAL WORD cnts = 100;			/* counter */
#endif

/************************************************************************
 * INT app_interface(function, device, chnl)
 *    WORD function;	- application function 
 *    WORD device;	- application device number
 *    WORD chnl;	- session channel number (not used when device
 *			  is in character state.
 *
 *    app_interface() is called from the driver interface with the
 *    function, device and channel extracted from the application
 *    parameter packet.  The function specified by the application
 *    is performed depending on the current device state, and
 *    an error conditions which occur are returned to the application
 *    interface.  The application interface stores the error status
 *    in the application parameter packet.
 *
 * Returns:  app_interface() returns error conditions which occur during
 *    the processing of application functions.
 *
 ************************************************************************/
INT app_interface(function, device, chnl) 
    WORD function;                      /* function to be performed */
    WORD device;                        /* application device number */
    WORD chnl;	                        /* channel (0 - 15) for packet mode */
    {
    INT status;                         /* return status */
    WORD i;
    
    /* the first time driver is called, the logical device number
     * is saved and global data structures are initialized.
     * The device number is used to verify subsequent calls to
     * the device driver.
     */

    app_active = YES;
    
    /* If this is the first time that the driver is called, then
     * save the device number.  Originally, a check was going to
     * be made to ensure that the calling program always used 
     * the same device number.  This check has been turned off
     * for now.
     */
    if (first_time)
        {    
        first_time = NO;
        device_nbr = device;
        }
/*
    if (device != device_nbr)
        return (ILLEGAL_DEVICE);
 */

#ifdef DEBUG

    /* There is an extra test function when debugging.  This function is
     * used to create packets for pad functions.  These are used to
     * turn echoing on and off.
     */
    if (function < 0 || function > MAX_APPL_FUNC + 1)
#else
    if (function < 0 || function > MAX_APPL_FUNC)
#endif
        return (ILLEGAL_FUNC);
    
    /* If device is in packet state, then validate the channel number.
     * Otherwise, set the channel number to zero.
     */
    if (dev_stat.devstate == PKT_STATE)
	{
        if (chnl < 0 || chnl > MAX_CHNL)
            return (ILLEGAL_CHNL);
	app_chnl = (UBYTE)chnl;
        }
    else
	app_chnl = (UBYTE)CM_CHNL;

    /* Initialize the pointer to the application channel structure.  This
     * pointer to used by most of the application functions.
     */
    ap_pcis = &cis[app_chnl];
 
    /* Call routine depending on the current device state.
     * The device can be in WAIT/RESET state, character state or
     * packet state.
     */
    if (dev_stat.devstate == RESET_STATE )
	status = jmp_reset_funcs(function);
    else if (dev_stat.devstate == PKT_STATE)
	status = jmp_pkt_funcs(function);
    else if (dev_stat.devstate == CHAR_STATE)
	status = jmp_char_funcs(function);
    else
        program_error = WRONG_DEVICE_STATE;
#ifdef DEBUG
    diags[1] = ap_pcis->chnlstate;
#endif
    
    app_active = NO;
   
    /* If a RESTART or RESET  was received at the link level, then
     * the pad is cleared before the application exits.
     */
    if (clear_pad)
	{
	    
	/* If restart was received for channel 0, then reset all of
	 * the channels.
	 */
	if (lcis[0].resetreceived)
	    init_pad(-1, YES, YES);
	else
	    {
	    
	    /* Go through the channel information structure looking for
	     * channels which have been reset.  Make sure that the
	     * channel is connected.
	     */
	    for (i = 1; i <= MAX_CHNL; ++i)
		if (cis[i].lcistruct)
		    if (cis[i].lcistruct->resetreceived)
			init_pad(i, YES, YES);
	    }
	clear_pad = NO;
	}
#ifdef DEBUG
    if (ap_pcis->lcistruct)
	{
	diags[2] = ap_pcis->lcistruct->ssnstate;
	diags[3] = ap_pcis->lcistruct->restartstate;
	diags[4] = ap_pcis->lcistruct->resetstate;
	diags[8] = ap_pcis->lcistruct->restartstate;
	}
    diags[5] = program_error;
    diags[6] = free_count;
    cnts = 100;
		
    for (i = 1; i <= 15; ++i)
	{
	diags[cnts++] = i;
	diags[cnts++] = lcis[i].outpktqueue.nchains;
	diags[cnts++] = lcis[i].linkoutqueue.nchains;
	diags[cnts++] = lcis[i].inpktqueue.nchains;
	diags[cnts++] = lcis[i].waitackqueue.nchains;
	if (lcis[i].appchnl > 0)
	    {
	    diags[cnts++] = cis[lcis[i].appchnl].holdassembly;
	    diags[cnts++] = cis[lcis[i].appchnl].nwritebytes;
	    diags[cnts++] = cis[lcis[i].appchnl].assemblypkt;
	    diags[cnts++] = cis[lcis[i].appchnl].nreadbytes;
	    }
	else
	    diags[cnts++] = 0xff;
	diags[cnts++] = lcis[i].dteflowstate;
	diags[cnts++] = lcis[i].dxeflowstate;
	}
#endif
    return (status);
    }

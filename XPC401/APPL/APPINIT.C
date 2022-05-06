/************************************************************************
 * app_init.c - Initialize XPC driver Global Data
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    app_init.c contains the source module app_initialization which
 *    is called to initialize global data for the xpc driver.
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
#include "pkt.h"
#include "state.h"
#include "link.h"
#include "padprm.h"
#include "param.h"

IMPORT VOID adjust_time_len();		/* adjust length of time for T25 and
					 * T27 timer interrupts.
					 */
IMPORT VOID init_window();		/* initialize the window parameters */

IMPORT BOOL app_active;			/* flag which indicates if application
					 * is active.
					 */
IMPORT BOOL clear_pad;			/* flag which is set by link to tell
					 * the application to clear the pad
					 * structure before exiting.
					 */
IMPORT BUFLET *pecho;			/* pointer to echo buflet */
IMPORT BUFLET *ptrans;			/* current transmit assembly pointer */ 
IMPORT CHNLINFO cis[]; 			/* channel information structure */
IMPORT DEVSTAT dev_stat;                /* device status structure */
IMPORT INT numecho;			/* number of echo bytes */
IMPORT LCIINFO lcis[];                  /* logical channel information 
					 * structures.
					 */
IMPORT PORTPARAMS port_params;          /* port parameter structure */
IMPORT PORTPARAMS default_params;	/* initial port parameter structure */
IMPORT UBYTE restart_devmode;		/* restart cause */
IMPORT UWORD numprc;			/* number of characters processed */
IMPORT UWORD numreq;			/* number of characters required */
IMPORT UWORD program_error;             /* program error code */
IMPORT UWORD trans_idx;			/* transmit assembly index */
IMPORT WORD pend_inc_calls;	        /* number of pending incomming calls */
IMPORT UWORD rnr_buflets_req;		/* number of buflets required before
    					 * RNRing.
    					 */
    
/************************************************************************
 * VOID oth_initialization()
 *
 *    oth_initialization() initializes the XPC driver global data.
 *    This routine is called to initialize data when the device
 *    state changes to and from packet, or character states.
 *    This routine is called when link is active.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID oth_initialization()
    {
    INT i, j;
    

    /* Initialize Logical channel information structure for every channel.
     */
    for (i = 0; i <= MAX_CHNL; ++i)
        {
	
	/* Initialize the application channel number, the session state,
         * and the clear code.
	 */
	lcis[i].appchnl = -1;
	lcis[i].ssnstate = S1;
	lcis[i].ssncleartime = 0;
	lcis[i].ssnclearcode = 0;
	lcis[i].flowstate = 0;
	
	/* Clear the pad output queue pointers and the number of chains
         * in the pad output queue.  The pad output queue had to have
         * been flushed before this routine is called.
	 */
	lcis[i].outpktqueue.begqueue = 0;
	lcis[i].outpktqueue.endqueue = 0;
        lcis[i].outpktqueue.nchains = 0;
        lcis[i].inpktqueue.begqueue = 0;

	/* Clear the pad input queue pointers and the number of chains
	 * in the pad input queue.  The pad input queue has to have 
	 * been flushed before this routine is called.
	 */
        lcis[i].inpktqueue.endqueue = 0;
        lcis[i].inpktqueue.nchains = 0;
	}
	
    /* Clear information in the channel information structure for
     * every channel.
     */
    for (i = 0; i <= MAX_CHNL; ++i)
	{
	
	/* Clear the channel state and the logical channel number.
	 * and the session clear code.
	 */
	cis[i].chnlstate = CHNL_DISCONNECTED;
	cis[i].logicalchnl = 0;
	cis[i].clearcode = 0;
	
	/* Disconnect the channels.
	 */
        cis[i].lcistruct = (LCIINFO *)NULLBUF;
	
	/* Clear echo states.
	 */
	cis[i].tymechostate = T0;
	cis[i].mciechostate = M0;
	
	/* Clear pointers to transmit assembly packet, echo packet and
	 * session data.
	 */
	cis[i].assemblypkt = cis[i].echopkt = cis[i].ssndatapkt = NULLBUF;
	cis[i].idxssndata = 0;
	cis[i].nwritebytes = cis[i].idxechodata = 0;
	
	/* Clear pointers to application read queues.
	 */
	cis[i].readqueue.begqueue = cis[i].readqueue.endqueue = 0;
        cis[i].readqueue.nchains = 0;
        cis[i].nreadbytes = cis[i].idxreadqueue = 0;
        cis[i].idxreadqueue = cis[i].nreadbytes = cis[i].idxssndata = 0;
	
	/* Turn off flow control flag.
	 */
	cis[i].holdassembly = NO;
	
	/* clear interrupts
	 */
	cis[i].timerevent = cis[i].checkevent = 0;
	cis[i].timereventseg = cis[i].timereventoff = 0;
	cis[i].checkeventseg = cis[i].checkeventoff = 0;
	cis[i].errorstatus = cis[i].breakstatus = 0;

	/* Initialize the pad parameters to their default values.
	 */
	for (j = 0; j < NUM_PAD_PARAM; ++j)
            cis[i].padparams[j] = 0;
	cis[i].padparams[ECHO_LF] = DEFAULT_ECHO_LF;
        cis[i].padparams[ECHO_ENABLED] = DEFAULT_ECHO_ENABLED;
        cis[i].padparams[FWD_CHAR] = DEFAULT_FWD_CHAR;
        cis[i].padparams[FWD_TIME] = DEFAULT_FWD_TIME;
	}
    program_error = 0;
  
    /* Initialize the port parameters structure.
     */
    default_params.baudrate = port_params.baudrate = DEFAULT_BAUD;
    default_params.databits = port_params.databits = DEFAULT_DATA;
    default_params.stopbits = port_params.stopbits = DEFAULT_STOP;
    default_params.xonxoff = port_params.xonxoff = DEFAULT_XON;
    default_params.dxemode = port_params.dxemode = DEFAULT_MODE;
    restart_devmode = 0;
    default_params.parity = port_params.parity = DEFAULT_PARITY;

    /* Initialize the device status structure.
     */
    dev_stat.devstate = RESET_STATE;
    dev_stat.version = XPC_VERSION;
    dev_stat.revision = 0;
    dev_stat.portaddr = 0;
    dev_stat.padtype = DEFAULT_PAD;
    dev_stat.pvcchnls = DEFAULT_PVCCHNLS;
    dev_stat.inchnls = 0;
    dev_stat.twaychnls = 0;
    dev_stat.outchnls = 0;
 

    /* Clear the variables which are used by the application and
     * packet processing.
     */
    pend_inc_calls = 0;			/* number of pending incoming
					 * calls.
				         */
    program_error = 0;			/* The last program error which 
					 * occurred.
					 */
    numecho = 0;			/* The number of characters in
					 * the echo buflet.
					 */
    pecho = NULLBUF;			/* Pointer to echo buflet */
    ptrans = NULLBUF;			/* Current position in transmit 
					 * buffer.
					 */
    trans_idx = 0;			/* Index into transmit buffer */
    numprc = numreq = 0;		/* Number of characters output and
					 * number of characters requested
					 * to output.
					 */
    cis[0].lcistruct = &lcis[0];	/* Link channel 0 */
    lcis[0].appchnl = 0;
    rnr_buflets_req = BUFLET_LOWATER;	/* set number of buflets required
    					 * before RNRing.
    					 */
    adjust_time_len();			/* adjust the timeout values depending
					 * on the number of channels 
					 * and the baud rate.
					 */
    }

/************************************************************************
 * VOID app_initialization()
 *
 *    app_initialization() initializes the XPC driver global data.
 *    This routine is called to initialize data when the driver is
 *    loaded.  It initializes the data which  is only needed by the
 *    link and calls oth_initialization to initialize data which
 *    changes when the device state changes.  This routine is called
 *    when link is not active.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID app_initialization()
    {
    INT i;
    

    /* Initialize link information in the logical channel 
     * information structure.
     */
    for (i = 0; i <= MAX_CHNL; ++i)
        {
	
	/* Initialize link states.   app_initialization should not be called
         * once the driver has been loaded.
	 */
        lcis[i].resetstate = 0;
	lcis[i].resetcode = 0;
	lcis[i].resetreceived = NO;
	lcis[i].dxeflowstate = F1;
	lcis[i].dteflowstate = G1;
	lcis[i].restartstate = R1;
	
	/* Initialize the link retry counters.
	 */
	lcis[i].r20trans = 0;
	lcis[i].r22trans = 0;
	lcis[i].r25trans = 0;
	lcis[i].r27trans = 0;
	
	/* Initialize link flags.
	 */
	lcis[i].pktrejected = NO;	/* Last packet rejected */
	lcis[i].rnrtransmitted = NO;	/* Last control packet was RNR */
	lcis[i].retransmitdata = NO;	/* Last packet should be retransmitted
					 */
	
	/* Initialize the window parameters for the input and output
         * windows.
	 */
	
	init_window(&lcis[i]);
	
	/* Clear the pointers to link queues.
	 */
	lcis[i].linkoutqueue.begqueue = 0;
	lcis[i].linkoutqueue.endqueue = 0;
        lcis[i].linkoutqueue.nchains = 0;
	}
	
    
    /* Clear flags which are used by the application.
     */
    app_active = NO;			/* Application is active */
    clear_pad = NO;			/* Application should clear pad
					 * queues before exiting.
					 */
    /* Initialize the rest of the data
     */
    oth_initialization();
    }



    

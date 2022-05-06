/************************************************************************
 * linkinit.c - Initialize Link and Window Variables
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    linkinit.c contains the source module init_window() which
 *    is called to initialize the window parameters and
 *    init_chnl_data() which is called to initialize link  parameters
 *    in the logical channel information structure.
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
#include "link.h"

IMPORT BOOL in_a_frame;			/* in a frame */

#ifdef DEBUG
IMPORT WORD diags[];			/* array of words used for debugging
					 */
#endif

/************************************************************************
 * init_window(plcis)
 *    LCIINFO *plcis; - pointer to logical channel information structure
 *
 *    init_window() is called to initialize the window parameters
 *    during the initialization of a channel.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID init_window(plcis)
    FAST LCIINFO *plcis;	/* pointer to logical channel information
    				 * structure.
    				 */
    {

    /* Initialize input and output window parameters.  This
     * routine is called during the reset of a channel
     * or the restart of the device.    
     */
    plcis->inwindlow = 0;
    plcis->inwindhigh = plcis->inwindlow + WINDOW_SIZE - 1;
    plcis->inpktrecvseq = 0;
    plcis->inpktsendseq = 15;
    plcis->indcount = WINDOW_DATA;
    plcis->outwindlow = 0;
    plcis->outwindhigh = plcis->outwindlow + WINDOW_SIZE - 1;
    plcis->outpktrecvseq = 0;
    plcis->outpktsendseq = 0;
    plcis->outdcount = WINDOW_DATA;
    }




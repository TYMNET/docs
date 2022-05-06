/************************************************************************
 * adjlen.c - Adjust Length of Link Timer Interrupts
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    adjlen.c contains the source module adjust_timer_length().
 *    This routine is called when a channel is opened, a channel
 *    is closed or the port parameters are updated.  adjust_timer_length()
 *    calculates the length of time between interrupts for the
 *    T25 and the T27 timers.
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

IMPORT VOID int_disable();			/* disable interrupts */
IMPORT VOID int_enable();			/* enable interrupts */
IMPORT CHNLINFO cis[];			        /* channel information array */
IMPORT PORTPARAMS port_params;			/* port parameter structure */
IMPORT WORD len_mult;				/* time length multiplier */

LOCAL UBYTE len_table[8] = {16, 8, 4, 2, 1, 1, 1, 1};

/************************************************************************
 * VOID adjust_time_len()
 *
 *   adjust_time_len is called to calculate the length of time between
 *   timer interrupts for the T25 and T27 timers.  It is used for
 *   timers which are dependent on the amount of time it will take to
 *   transmit a packet.  The number of channels which are open and
 *   the baud rate are used to calculate the length of time.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID adjust_time_len()
    {
    UBYTE i, j;					/* counter */
    UBYTE num;					/* number of open channels */
    
    for (j = 0, i = 1; i <= MAX_CHNL; ++i)
	{
	if (cis[i].lcistruct)
	    ++j;
	}
    if (!j)
	++j;
    num = ((j + 1) / 2);
    int_disable();
    len_mult = (WORD)(num * len_table[port_params.baudrate]);
    int_enable();
    }


    


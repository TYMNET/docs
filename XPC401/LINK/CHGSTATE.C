/************************************************************************
 * chgstate.c - Clear IOCOMM When Change In State Occurs
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    chgstate.c contains the source modules which are called to
 *    to clear IOCOMM'S queues and stop the timers.  These routines
 *    are called when the device is cleared, and when the device
 *    state changes to and from character/packet states.
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
#include "pkt.h"
#include "timer.h"
#include "state.h" 
#include "iocomm.h"

IMPORT BUFLET *get_queue();		/* get buflet chain from queue */
IMPORT COMMINFO comm_info;		/* communications information
					 * structure.
					 */
IMPORT VOID clear_comm();		/* clear communications */
IMPORT VOID clear_xmt();		/* clear transmitter flags */
IMPORT VOID free_buf();			/* free buflet chain */
IMPORT VOID int_disable();		/* disable interrupts */
IMPORT VOID int_enable();		/* enable interrupts */
IMPORT VOID stop_timer();		/* stop timer */

IMPORT LCIINFO lcis[];			/* logical channel information
					 * structure.
					 */
#ifdef DEBUG
IMPORT WORD diags[];			/* XXX */
#endif
/************************************************************************
 * VOID init_state(plcis, chnl)
 *    LCIINFO *plcis; 	- pointer to logical channel information structure
 *    UBYTE chnl;	- channel to be reset
 *
 *    init_state() is called to clear the link output queue, the
 *    waiting to be acknowledged queue and to stop the timers which 
 *    are used by the pad.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID init_state(plcis, chnl)
    FAST LCIINFO *plcis;		/* logical channel information
					 * structure.
					 */
    UBYTE chnl;				/* channel to reset */
    {
    FAST BUFLET *pb;			/* pointer to packet */


    /* Stop all of the timers which are used by the link and the pad.
     * All except the one second timer.
     */
    stop_timer(ONE_SEC_TIMER, TIM_T20, chnl);
    stop_timer(TEN_SEC_TIMER, TIM_T22, chnl);
    stop_timer(ONE_SEC_TIMER, TIM_T25, chnl);
    stop_timer(ONE_SEC_TIMER, TIM_T27, chnl);
    stop_timer(ONE_SEC_TIMER, TIM_RRCHANZERO, chnl);
    stop_timer(ONE_SEC_TIMER, TIM_RRRNR, chnl);
    stop_timer(TEN_SEC_TIMER, TIM_T21, chnl);
    stop_timer(SIXTH_SEC_TIMER, TIM_CLRCONFIRM, chnl);
    stop_timer(SIXTH_SEC_TIMER, TIM_TYMNOACTIV, chnl);
    stop_timer(SIXTH_SEC_TIMER, TIM_FORWARDING, chnl);
    stop_timer(ONE_SEC_TIMER, TIM_BALLOUT, chnl);

    /* Clear out the waiting to be acknowledged queue.
     */
    while ((pb = get_queue(&plcis->waitackqueue)) != NULLBUF)
	free_buf(pb);
	
    /* Clear out the link output queue.
     */
    while ((pb = get_queue(&plcis->linkoutqueue)) != NULLBUF)
	free_buf(pb);
    
    }


/************************************************************************
 * VOID init_new_state()
 *
 *    init_new_state() is called when changing to/from packet/character
 *    mode or when the device is reset.  It ensures that all of the 
 *    queues are cleared and that iocomm is not transmitting packets.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID init_new_state()
    {
    UWORD x;				/* index for IOCOMM transfer queue */
    UBYTE i;				/* channel number */
    
    x = XMT_BUF_CHAINS;
	
    int_disable();
	
    /* Scan the iocomm transmit queue, high to low.  If an 
     * IOCOMM entry is found which contains an RR, RNR or REJECT 
     * packet the packet is freed.  In any case each IOCOMM transmit
     * entry is cleared.
     */
    while (x--)
	{
	/* Clear the IOCOMM entry.  Packets which are not in
         * the waiting to be acknowledged queue
	 * (RR, RNR and REJECT packets) are freed.
	 */
	if (comm_info.xmtptr[x])
	    {
	    if (comm_info.xmtflg[x])
		free_buf(comm_info.xmtptr[x]);
	    comm_info.xmtlen[x] = 0;
	    comm_info.xmtflg[x] = NO;
	    comm_info.xmtptr[x] = NULLBUF;
	    }
	}
    
    /* Clear IOCOMM's input pointers.
     */
    clear_comm();
    clear_xmt();
    int_enable();
    
    /* Clear link queues for all of the channels.  Stop all timers
     * which are used by pad and link.
     */
    for (i = 0; i <= MAX_CHNL; ++i)
	init_state(&lcis[i], i);
    }

	

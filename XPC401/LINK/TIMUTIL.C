/************************************************************************
 * timutil.c - timer list utility routines
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    This module contains utility routines used to manage timer lists.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KJB   Initial Draft
 *
 ************************************************************************/
#include "stddef.h"
#include "timer.h"

IMPORT TIMER *timer_lists[];		/* timer list start array */

/************************************************************************
 * VOID dec_sixths()
 *
 *     dec_sixths is called by the timer interrupt service routine to
 *     decrement type 0 (1/6 second) timers. Every TICKS_SIXTH_SEC (1/18
 *     second ticks per 1/6 second) times dec_sixths is called, the 1/6
 *     second timer list will be scanned and the length field in all 
 *     timers will be decremented.
 *
 * Notes: dec_sixths runs with interrupts disabled.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID dec_sixths()
    {
    INTERN WORD tick = TICKS_SIXTH_SEC;	/* 1/6 second tick counter */
    FAST TIMER *pt;			/* scratch timer pointer */

    /* if this is the TICKS_SIXTH_SEC time dec_sixths has been called,
     * decrement the lengths of all timers in the 1/6 second list
     */
    if (!--tick)
	{
	for (pt = timer_lists[SIXTH_SEC_TIMER]; pt; pt = pt->timnext)
	    --pt->timlength;
	tick = TICKS_SIXTH_SEC;		/* reset 1/6 second tick count */
	}
    }

/************************************************************************
 * VOID dec_timer(ptimer)
 *     TIMER *ptimer;		pointer to (this) timer entry
 *
 *     dec_timer is a type 0 (1/6 second) timer routine used (indirectly)
 *     by the timer interrupt service routine to maintain type 1 (1
 *     second) and type 2 (10 second) timer lengths. The timer is started
 *     upon Driver startup with a length of TICKS_ONE_SEC, in effect 
 *     becoming a 1 second timer. Each time dec_timer is called, the 1
 *     second timer list will be scanned and the length field in all 
 *     timers will be decremented. Every TEN_SECONDS (1 second ticks per
 *     10 seconds) times dec_timer is called, the 10 second list will be
 *     scanned and the length field in all timers will be decremented.
 *
 * Notes: Since internal counters are used (and cannot be reset), this
 *     timer must be active for the life of the Driver. 
 *
 *     Being a timeout routine, dec_timer runs with interrupts enabled.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID dec_timer(ptimer)
    TIMER *ptimer;			/* pointer to (this) timer */
    {
    INTERN WORD tock = TEN_SECONDS;	/* ten second counter */
    FAST TIMER *pt;			/* scratch timer pointer */
    
    /* decrement the lengths of all timers in the one second list
     */
    for (pt = timer_lists[ONE_SEC_TIMER]; pt; pt = pt->timnext)
	--pt->timlength;

    /* if this is the TEN_SECONDS time dec_timer has been called,
     * decrement the lengths of all timers in the ten second list
     */
    if (!--tock)
	{
	for (pt = timer_lists[TEN_SEC_TIMER]; pt; pt = pt->timnext)
	    --pt->timlength;
	tock = TEN_SECONDS;		/* reset ten second counter */
	}
    
    /* reset this timer to be called again in 1 second
     */
    ptimer->timlength = TICKS_ONE_SEC;
    }

/************************************************************************
 * VOID do_timers()
 *
 *     do_timers is called by the timer interrupt service routine, after
 *     all timers have been decremented, to scan the timer lists and call
 *     functions for those timers that have expired.
 *
 * Notes: do_timers runs with interrupts enabled.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID do_timers()
    {
    FAST TIMER *pt;			/* running timer list pointer */
    FAST type;				/* timer type index */

    /* for each timer type
     */    
    for (type = 0; type < TIMER_TYPES; ++type)

	/* scan the timer list; for each timer, if the length has reached
	 * 0 or less, call the timer function, passing the timer address
	 */
	for (pt = timer_lists[type]; pt; pt = pt->timnext)
	    if (pt->timlength <= 0)
		(*pt->timfunc)(pt);
    }

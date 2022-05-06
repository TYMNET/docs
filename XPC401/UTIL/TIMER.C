/************************************************************************
 * timer.c - timer management routines
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    This module contains routines responsible for maintaining timer
 *    lists used internally by the XPC Driver.
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
#include "timer.h"

IMPORT UBYTE *xpc_alloc();		/* returns address of new memory */
IMPORT VOID int_disable();		/* disables interrupts */
IMPORT VOID int_enable();		/* enables interrupts */

IMPORT TIMER *timer_lists[];		/* timer list start addresses */

LOCAL TIMER *timer_array = NULLTIM;	/* pointer to timer array */

/************************************************************************
 * VOID reset_timers()
 *
 *     reset_timers is called upon device reset to stop all active timers
 * and re-initialize the timer lists array.
 *
 * Notes: Only the pointer to timeout function in each active timer is
 *     re-initialized.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID reset_timers()
    {
    INT type;				/* timer type index */
    FAST TIMER *pt;			/* scratch timer pointer */

    /* for each entry in the timer list array
     */
    for (type = 0; type < TIMER_TYPES; ++type)
    
	/* if 1 or more entries exist in the timer list array
	 */
	if ((pt = timer_lists[type]) != NULLTIM)
	    {
	
	    /* initialize (stop) all timers in the list
	     */
	    do  {
		pt->timfunc = NULLFUNC;
		} while ((pt = pt->timnext) != NULLTIM);
	    
	    /* clear the timer list for the current type
	     */
	    timer_lists[type] = NULLTIM;
	    }
    }

/************************************************************************
 * TIMER *init_timer_array()
 *
 *     init_timer_array is called exactly once upon Driver initialization
 * to allocate and initialize memory for all required timers.
 *
 * Notes: Only the pointer to timeout function in each timer is
 *     initialized.
 *
 * Returns: If memory allocation fails, init_timer_array returns a null
 *     pointer; otherwise, a pointer to the initialized timer array is
 *     returned.
 *
 ************************************************************************/
TIMER *init_timer_array()
    {
    FAST TIMER *pt;			/* scratch timer pointer */
    FAST count;				/* timer initialization counter */

    /* allocate memory for TIMER_ARRAY_SIZ timers. if successful...
     */
    if ((timer_array =
	(TIMER *)xpc_alloc((BYTES)(TIMER_ARRAY_SIZ * TIMER_SIZ))) != NULLTIM)
	{
	
	/* initialize all entries in the timer array
	 */
	pt = timer_array;
	count = TIMER_ARRAY_SIZ;
	do  {
	    (pt++)->timfunc = NULLFUNC;	/* parens needed (lint 2.00g bug) */
	    } while (--count);
	}
    return (timer_array);		/* null if xpc_alloc failed */
    }

/************************************************************************
 * VOID start_timer(type, class, chnl, length, func)
 *     UBYTE type;		timer type index ([0 - 2])
 *     UBYTE class;		timer class ([0x00 - 0xf0])
 *     UBYTE chnl;		timer channel ([0x00 - 0x0f])
 *     WORD length;		timer length (ticks)
 *     VOID (*func)();		pointer to timeout function
 *
 *     start_timer is used to start a timer. If the timer is already
 * active (linked in the timer lists), only the length and pointer to
 * timeout function will be updated. If inactive, the timer type and
 * id (class | chnl) will also be updated, and the timer will be linked
 * into the timer list of the appropriate type.
 *
 * Notes: The type parameter is used as an index into the timer lists
 *     array; its value must be in the range [0 - 2]. type is not validated.
 *
 *     The timer id is formed by the inclusive OR of the timer class and
 *     chnl. Valid timer classes are in the range [0x00-0x0f]; start_timer
 *     expects the class parameter to contain the class in the high order
 *     nibble [0x00 - 0xf0]. Likewise, the chnl parameter should contain
 *     the timer channel in the low nibble; its value must be in the range
 *     [0x00 - 0x0f]. Neither class nor chnl are validated.
 *
 *     Since start_timer uses the pointer to timeout function, resident in
 *     the timer array, as an internal indication of an active timer (a
 *     null pointer indicates an inactive timer), passing a null function
 *     pointer is decidedly unhealthy. func is not validated.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID start_timer(type, class, chnl, length, func)
    UBYTE type;				/* timer type index */
    UBYTE class;			/* timer class */
    UBYTE chnl;				/* timer channel */
    WORD length;			/* timer length */
    VOID (*func)();			/* pointer to timer function */
    {
    UBYTE id;				/* timer id/index */
    FAST TIMER **ps;			/* pointer to timer start pointer */
    FAST TIMER *pt;			/* scratch timer pointer */

    /* calculate timer id and, using the id as an index, get the address of
     * the timer in the timer array
     */
    id = class | chnl;
    pt = timer_array + id;
    int_disable();			/* disable interrupts */
    
    /* if the timer is inactive, link it to the (head of the) timer list
     * of the appropriate type and install the timer type and id
     */
    if (!pt->timfunc)
	{
	
	/* set ps to the address of the list pointer for the specified timer
	 * type, and link the new timer to the start of the list. if another
	 * timer exists in the list (list pointer is non-null), set the
	 * next timer's previous pointer to the address of the new timer
	 */
	ps = &timer_lists[type];
	if ((pt->timnext = *ps) != NULLTIM)
	    (*ps)->timprev = pt;
	pt->timprev = NULLTIM;		/* first's previous is always null */
	*ps = pt;			/* new timer becomes first in list */
	pt->timtype = type;		/* install timer type (redundant) */
	pt->timid = id;			/* and timer id (redundant) */
	}
    pt->timlength = length;		/* install timer length */
    pt->timfunc = func;			/* and pointer to timeout function */
    int_enable();			/* enable interrupts */
    }

/************************************************************************
 * VOID stop_timer(type, class, chnl)
 *     UBYTE type;		timer type index ([0 - 2])
 *     UBYTE class;		timer class ([0x00 - 0xf0])
 *     UBYTE chnl;		timer channel ([0x00 - 0x0f])
 *
 *     stop_timer is used to stop a timer. If the requested timer is
 *     active, it will be removed from the timer list of the specified
 *     type and its pointer to timeout function will be cleared. If the
 *     timer is inactive, stop_timer will do nothing.
 *
 * Notes: The type parameter is used as an index into the timer lists
 *     array; its value must be in the range [0 - 2]. type is not validated.
 *
 *     The timer id is formed by the inclusive OR of the timer class and
 *     chnl. Valid timer classes are in the range [0x00-0x0f]; start_timer
 *     expects the class parameter to contain the class in the high order
 *     nibble [0x00 - 0xf0]. Likewise, the chnl parameter should contain
 *     the timer channel in the low nibble; its value must be in the range
 *     [0x00 - 0x0f]. Neither class nor chnl are validated.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID stop_timer(type, class, chnl)
    UBYTE type;				/* timer type index */
    UBYTE class;			/* timer class */
    UBYTE chnl;				/* timer channel */
    {
    FAST TIMER *pt;			/* scratch pointer timer */

    /* using the timer id as an index, get the address of the timer in the
     * timer array
     */
    pt = timer_array + (class | chnl);
    int_disable();			/* disable interrupts */
    if (pt->timfunc)			/* if the timer is active */
	{

	/* if this is not the last timer in the list (the "next timer"
	 * pointer is non-null), set the next timer's "previous timer"
	 * pointer to this timer's "previous timer" pointer
	 */
	if (pt->timnext)
	    pt->timnext->timprev = pt->timprev;

	/* if this is not the first timer in the list (the "previous timer"
	 * pointer is non-null), set the previous timer's "next timer" pointer
	 * to this timer's "next timer" pointer
	 */
	if (pt->timprev)
	    pt->timprev->timnext = pt->timnext;
	
	/* else the first timer in the list is being unlinked; set the timer
	 * list pointer for the specified type to the next timer in the list
	 */
	else
	    timer_lists[type] = pt->timnext;
	pt->timfunc = NULLFUNC;		/* indicates timer is inactive */
	}
    int_enable();			/* enable interrupts */
    }

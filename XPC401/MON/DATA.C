/************************************************************************
 * data.c - line monitor input data functions
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    This module contains functions used to maintain data received by
 * the X.PC line monitor program.
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
#include "mon.h"

IMPORT BUFLET *alloc_buf();		/* buflet allocation routine */
IMPORT VOID free_buf();			/* buflet free routine */
IMPORT VOID int_disable();		/* disables interrupts */
IMPORT VOID int_enable();		/* enables interrupts */

IMPORT UWORD tick_count;		/* clock tick counter */

LOCAL DATA data_pool[DATA_ENTRIES];	/* data buffer pool */
LOCAL DATA *free_list = data_pool;	/* data buffer free list */
LOCAL DATAQ data_queue = {NULLDATA,	/* received data queue */
			  NULLDATA};
WORD xmt_intcnt = 0;
BOOL xpc_timer_active = YES;

/************************************************************************
 * VOID init_data()
 *
 *     init_data is called by the line monitor program once upon initial-
 * ization to initialize the data buffer pool.
 *
 * Notes: Calling init_data more than once invites disaster.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID init_data()
    {
    DATA *pd;				/* running data pointer */
    INT i;				/* loop counter */

    /* link each of the first DATA_ENTRIES - 1 data entries to the next
     */
    pd = data_pool;
    i = DATA_ENTRIES;
    while (--i)
        pd = pd->next = pd + 1;		/* associates right to left */
    pd->next = NULLDATA;		/* free list must be null terminated */
    }

/************************************************************************
 * DATA *alloc_data(nbufs)
 *     BYTES nbufs;		number of buflets to allocate
 *
 *     alloc_data will attempt to allocate a data entry containing a
 * nbufs buflet chain.
 *
 * Notes: No checks are made for reasonable values of nbufs.
 *
 * Returns: If the data entry is successfully allocated, and a nbufs
 *     buflet chain is successfully allocated, alloc_data will return
 *     a pointer to the data entry; otherwise, a null pointer is returned.
 *
 ************************************************************************/
DATA *alloc_data(nbufs)
    BYTES nbufs;			/* number of buflets to allocate */
    {
    DATA *pd;				/* scratch data pointer */

    int_disable();			/* disable interrupts */
    
    /* save the address of the first entry in the data free list. if
     * non-null...
     */
    if ((pd = free_list) != NULLDATA)
    
	/* attempt to allocate a nbufs buflet chain. if successful...
	 */
	if ((pd->pbuf = alloc_buf(nbufs)) != NULLBUF)
	    {
	
	    /* remove data entry at pd from the free list, clear the
	     * entry's forward link pointer, and return its address
	     */
	    free_list = free_list->next;
	    int_enable();		/* enable interrupts */
	    pd->next = NULLDATA;
	    return (pd);
	    }
	    
    /* data or buflet allocation failed; enable interrupts and return
     * a null pointer
     */
    int_enable();
    return (NULLDATA);
    }

/************************************************************************
 * VOID free_data(pd)
 *     DATA *pd;		pointer to data
 *
 *     free_data will free the buflet chain in the data entry at pd and
 * return the data entry to the data free list.
 *
 * Notes: free_data assumes the data entry at pd was obtained via a call
 *     to alloc_data; the data entry must contain a valid buflet chain
 *     address. No checks are made for legal values of pd.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID free_data(pd)
    DATA *pd;				/* pointer to data entry to free */
    {

    free_buf(pd->pbuf);			/* free buflet chain in pd */
    int_disable();			/* disable interrupts */
    pd->next = free_list;		/* link pd to top of free list */
    free_list = pd;			/* adjust free list to include pd */
    int_enable();			/* enable interrupts */
    }

/************************************************************************
 * VOID put_data(pd, port, code)
 *     DATA *pd;		pointer to data entry
 *     INT port;		receive port (RPORT1 or RPORT2)
 *     INT code;		data code (RCMODE, RPMODE, RDROP)
 *
 *     put_data will install the receive port number and data code in the
 * data entry at pd and add the entry to the received data queue.
 *
 * Notes: put_data assumes the data entry at pd was obtained via a call
 *     to alloc_data; no checks are made for legal values of pd.
 *
 *     The receive port and data code parameters are not validated.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID put_data(pd, port, code)
    DATA *pd;				/* pointer to data entry */
    INT port;				/* receive port */
    INT code;				/* data code */
    {

    pd->port = port;			/* install receive port */
    pd->code = code;			/* install data code */
    pd->time = tick_count;		/* install tick counter */

    /* add the data entry to the receive data queue
     */
    int_disable();			/* disable interrupts */
    if (data_queue.tail)		/* if queue not empty */
	data_queue.tail->next = pd;	/* link pd to end of queue */
    else
	data_queue.head = pd;		/* else head points to pd */
    data_queue.tail = pd;		/* tail always points to new entry */
    int_enable();			/* enable interrupts */
    }

/************************************************************************
 * DATA *get_data()
 *
 *     get_data is used to retrieve data entries from the received data
 * queue.
 *
 * Notes: None.
 *
 * Returns: If one or more entries exist in the received data queue, 
 *     get_data returns a pointer to the first (oldest) entry; otherwise,
 *     a null pointer is returned.
 *
 ************************************************************************/
DATA *get_data()
    {
    DATA *pd;				/* data entry pointer (returned) */

    int_disable();			/* disable interrupts */

    /* set pd to the first entry in the received data queue. if an entry
     * exists...
     */
    if ((pd = data_queue.head) != NULLDATA)
    
	/* remove the entry from the queue
	 */
	if ((data_queue.head = pd->next) == NULLDATA)
	    data_queue.tail = NULLDATA;
    int_enable();			/* enable interrupts */
    return (pd);			/* null if queue empty */
    }

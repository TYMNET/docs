/************************************************************************
 * queue.c - queue management routines
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *     This module contains routines responsible for maintaining buflet
 *     chain queues used internally by the XPC Driver.
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

IMPORT VOID int_disable();		/* disables interrupts */
IMPORT VOID int_enable();		/* enables interrupts */

/************************************************************************
 * VOID add_queue(pq, pc)
 *     QUEUE *pq;		pointer to queue control structure
 *     BUFLET *pc;		pointer to buflet chain
 *
 *     add_queue will append the buflet chain at pc (obtained via a call
 *     to buf_alloc) to the queue described by the queue structure at
 *     pq; i.e., the buflet chain at pc will become the last entry in
 *     the queue. The queue chain count will be adjusted as necessary.
 *
 * Notes: The queue control structure must be initialized exactly
 *     once (to all nulls) before a call to add_queue can be
 *     made. No checks are made for illegal values of pq.
 *
 *     It is assumed the buflet chain at pc is a null terminated
 *     list of 1 or more buflets, obtained via buf_alloc. No
 *     checks are made for illegal values of pc.
 *
 *     In order to keep the queue null terminated, add_queue
 *     will clear the forward chain pointer of the first buflet
 *     in the chain at pc.
 *
 *     Interrupts will be temporarily disabled during queue
 *     manipulation.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID add_queue(pq, pc)
    FAST QUEUE *pq;			/* pointer to queue structure */
    BUFLET *pc;				/* pointer to buflet chain */
    {

    pc->chainnext = NULLBUF;		/* queue must be null terminated */
    int_disable();			/* disable interrupts */

    /* if the queue end pointer is non-null, at least 1 chain exists in the
     * queue; link the new chain to the end of the queue
     */
    if (pq->endqueue)
	pq->endqueue->chainnext = pc;

    /* else the queue is empty; set the queue begin pointer to the new chain
     */
    else
	pq->begqueue = pc;
    pq->endqueue = pc;			/* queue end points to new chain */
    ++pq->nchains;			/* increment queue chain count */
    int_enable();			/* enable interrupts */
    }

/************************************************************************
 * VOID insert_queue(pq, pc)
 *     QUEUE *pq;		pointer to queue control structure
 *     BUFLET *pc;		pointer to buflet chain
 *
 *     insert_queue will insert the buflet chain at pc (obtained via
 *     buf_alloc) in the queue described by the queue structure at
 *     pq; i.e., the buflet chain at pc will become the first entry in
 *     the queue. The queue chain count will be adjusted as necessary.
 *
 * Notes: The queue control structure must be initialized exactly
 *     once (to all nulls) before a call to insert_queue can be
 *     made. No checks are made for illegal values of pq.
 *
 *     It is assumed the buflet chain at pc is a null terminated
 *     list of 1 or more buflets, obtained via buf_alloc. No
 *     checks are made for illegal values of pc.
 *
 *     insert_queue will set the forward chain pointer of the
 *     first buflet in the chain at pc to the address of the
 *     next chain in the queue. The pointer will be cleared if
 *     the chain at pc is the only entry in the queue.
 *
 *     Interrupts will be temporarily disabled during queue
 *     manipulation.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID insert_queue(pq, pc)
    FAST QUEUE *pq;			/* pointer to queue structure */
    BUFLET *pc;				/* pointer to buflet chain */
    {

    int_disable();			/* disable interrupts */

    /* link the new buflet chain to the top of the queue. if the queue begin
     * pointer is null, the queue is empty; set the queue end address to the
     * new buflet chain address (assignment and check are done in two state-
     * ments to quiet lint)
     */
    if ((pc->chainnext = pq->begqueue) == NULLBUF)
	pq->endqueue = pc;
    pq->begqueue = pc;			/* queue begin points to new chain */
    ++pq->nchains;			/* increment queue chain count */
    int_enable();			/* enable interrupts */
    }

/************************************************************************
 * BUFLET *get_queue(pq)
 *     QUEUE *pq;		pointer to queue control structure
 *
 *     get_queue will remove a buflet chain from the queue controlled by
 *     the queue control structure at pq. Buflet chains are retrieved
 *     from the beginning (head) of the queue. The queue chain count will
 *     be adjusted as necessary.
 *
 * Notes: The queue control structure must be initialized exactly
 *     once (to all nulls) before a call to get_queue can be
 *     made. No checks are made for illegal values of pq.
 *
 *     Interrupts will be temporarily disabled during queue
 *     manipulation.
 *
 * Returns: A null pointer, if the queue at pq is empty, or a pointer to
 *     the buflet chain retrieved.
 *
 ************************************************************************/
BUFLET *get_queue(pq)
    FAST QUEUE *pq;			/* pointer to queue structure */
    {
    BUFLET *pc;				/* pointer to buflet chain (returned) */

    int_disable();			/* disable interrupts */

    /* set pc to the first entry in the queue. if at least 1 entry exists...
     */
    if ((pc = pq->begqueue) != NULLBUF)
	{

	/* set the queue begin pointer to the next chain in the queue. if the
	 * pointer is null, the last chain in the queue has been retrieved;
	 * clear the queue end pointer also
	 */
	if ((pq->begqueue = pc->chainnext) == NULLBUF)
	    pq->endqueue = NULLBUF;
	--pq->nchains;			/* decrement the queue chain count */
	}
    int_enable();			/* enable interrupts */
    return (pc);			/* null if queue was empty */
    }

/************************************************************************
 * BUFLET *remove_queue(pq, pc)
 *     QUEUE *pq;		pointer to queue control structure
 *     BUFLET *pc;		pointer to buflet chain to remove
 *
 *     remove_queue will attempt to remove the buflet chain at pc from
 *     the queue controlled by the queue control structure at pq. if the
 *     chain at pc is found in the queue, the queue chain count will be
 *     adjusted as necessary.
 *
 * Notes: The queue control structure must be initialized exactly
 *     once (to all nulls) before a call to remove_queue can be
 *     made. No checks are made for illegal values of pq.
 *
 *     Buflet chains removed from the queue are not freed.
 *
 *     Interrupts will be temporarily disabled during queue
 *     manipulation.
 *
 * Returns: A null pointer, if the queue at pq is empty, or a pointer to 
 *     the buflet chain retrieved.
 *
 ************************************************************************/
BUFLET *remove_queue(pq, pc)
    QUEUE *pq;				/* pointer to queue structure */
    BUFLET *pc;				/* pointer to buflet chain */
    {
    FAST BUFLET *pcb;			/* pointer to current buflet chain */
    FAST BUFLET *ppb;			/* pointer to previous buflet */

    ppb = NULLBUF;			/* initialize previous pointer */
    int_disable();			/* disable interrupts */

    /* scan the queue until the chain address pc is found or the end
     * of the queue is hit
     */
    pcb = pq->begqueue;
    while (pcb)
	{
	if (pcb == pc)
	    {

	    /* found it. if this is not the first chain in the queue, set the
	     * previous chain's "next chain" pointer to the current chain's
	     * "next chain" pointer
	     */
	    if (ppb)
		ppb->chainnext = pcb->chainnext;

	    /* else the first chain in the queue is being removed; set the
	     * queue begin pointer to the current buflet's "next" chain
	     */
	    else
		pq->begqueue = pcb->chainnext;

	    /* if the removed chain's "next chain" pointer is null, the last
	     * chain in the queue was removed; set the queue end pointer to
	     * the previous chain (note the ppb will be null if only 1
	     * chain existed in the queue)
	     */
	    if (!pcb->chainnext)
		pq->endqueue = ppb;
	    --pq->nchains;		/* decrement the queue chain count */
	    break;			/* address of removed chain returned */
	    }
	ppb = pcb;			/* previous chain becomes current */
	pcb = pcb->chainnext;		/* set pcb to next chain in queue */
	}
    int_enable();			/* enable interrupts */
    return (pcb);			/* null if chain not found */
    }

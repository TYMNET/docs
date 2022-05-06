/************************************************************************
 * buflet.c - buflet management routines
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    This module contains routines responsible for maintaining buflets
 *    and buflet chains used internally by the XPC Driver.
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

IMPORT UBYTE *xpc_alloc();		/* returns address of new memory */
IMPORT VOID int_disable();		/* disables interrupts */
IMPORT VOID int_enable();		/* enables interrupts */

IMPORT UWORD free_count;		/* buflet free list count */

LOCAL BUFLET *free_ptr = NULLBUF;	/* pointer to buflet free list */

/************************************************************************
 * BUFLET *init_buf(pool_size)
 *     BYTES pool_size;
 *       
 *     init_buf is called once during Driver initialization to allocate
 *     and initialize pool_size bytes as the the data buflet pool.
 *
 * Notes: If init_buf is not called, or memory allocation fails, no
 *     buflets will be available for I/O, timers, etc. This
 *     function must be called (and succeed) exactly once.
 *
 *     Ideally, pool_size should be an even multiple of DATA_BUF_SIZ;
 *     pool_size % DATA_BUF_SIZ will be unused. No checks are made for
 *     sensible values of pool_size.
 *
 * Returns: A null pointer, if memory allocation fails, or a pointer to
 *     the buflet free list.
 *
 ************************************************************************/
BUFLET *init_buf(pool_size)
    BYTES pool_size;			/* total size of buflet pool */
    {
    FAST BUFLET *pb;			/* scratch buflet list pointer */
    FAST UWORD count;			/* buflet counter */

    /* if memory for buflet pool is successfully allocated...
     */
    if ((free_ptr = (BUFLET *)xpc_alloc(pool_size)) != NULLBUF)
	{

	/* initialize buflet scratch pointer and buflet free count (and
	 * scratch count)
	 */
	pb = free_ptr;
	count = free_count = pool_size / BUFLET_SIZ;
	
	/* link the allocated space into a buflet free list (note buflet
	 * data space is not initialized)
	 */
	while (--count)
	    pb = pb->bufnext = pb + 1;	/* associated left to right */

	/* the free list (now the entire buflet pool) must be
	 * terminated by a null "next buflet" pointer
	 */
	pb->bufnext = NULLBUF;
	}
    return (free_ptr);			/* null if calloc failed */
    }

/************************************************************************
 * BUFLET *alloc_buf(nbufs)
 *     BYTES nbufs;		number of buflets to be allocated
 *
 *     alloc_buf will attempt to allocate a chain of nbufs buflets from 
 *     the buflet free list. The last buflet in the chain will contain a
 *     null forward link pointer.
 *
 * Notes: At least nbufs buflets must exist in the free list for alloc_buf
 *     to succeed.
 *
 *     Although nbufs is unsigned, no checks are made for unreasonable
 *     values.
 *
 *     Interrupts are temporarily disabled while the buflet free list is
 *     manipulated.
 *
 * Returns: A null pointer, if less than nbufs buflets exist in the free
 *     list, else a pointer to the allocated buflet chain.
 *
 ************************************************************************/
BUFLET *alloc_buf(nbufs)
    FAST BYTES nbufs;			/* number of buflets to allocate */
    {
    BUFLET *pc;				/* buflet chain pointer (returned) */
    FAST BUFLET *pb;			/* scratch buflet list pointer */

    int_disable();			/* disable interrupts */

    /* if at least nbufs buflets exist in the free list
     */
    if (nbufs <= free_count)
	{
	
	/* decrement the buflet free count by the number of buflets to be
	 * allocated. set pb (running buflet pointer) and pc (returned) to
	 * the start of the buflet free list, and find the nbufs'th buflet
	 * in the chain
	 */
	free_count -= nbufs;
	pb = pc = free_ptr;
	while (--nbufs)
	    pb = pb->bufnext;
	
	/* adjust the free list pointer to exclude the newly allocated
	 * buflets null terminate the allocated chain
	 */
	free_ptr = pb->bufnext;
	int_enable();			/* enable interrupts */
	pb->bufnext = NULLBUF;
	}

    /* else less than nbufs free buflets exist in the free list; a null
     * pointer will be returned
     */
    else
	{
	int_enable();			/* enable interrupts */
	pc = NULLBUF;			/* clear return pointer */
	}
    return (pc);			/* pointer to buflet chain or null */
    }

/************************************************************************
 * VOID free_buf(pc)
 *     BUFLET *pc;		pointer to buflet chain
 *
 *     free_buf will return the buflet chain at pc to the buflet free
 *     list.
 *
 * Notes: free_buf assumes the buflet chain at pc was obtained via a call
 *     to alloc_buf; the chain must be null terminated. No checks are made 
 *     for legal values of pc.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID free_buf(pc)
    BUFLET *pc;				/* pointer to buflet chain */
    {
    FAST BUFLET *pb;			/* scratch buflet pointer */
    FAST count;				/* freed buflet count */

    /* find the last buflet in the chain being freed
     */
    count = 1;				/* last buflet is not counted */
    pb = pc;
    while (pb->bufnext)
	{
	++count;
	pb = pb->bufnext;
	}
    int_disable();			/* disable interrupts */
    pb->bufnext = free_ptr;		/* link chain to top of free list */
    free_ptr = pc;			/* adjust free list pointer */
    free_count += count;		/* adjust buflet free count */
    int_enable();			/* enable interrupts */
    }






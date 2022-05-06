/************************************************************************
 * cwdata.c - character mode write data routines
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    This module contains interface routines to handle character mode
 *    output from the application.
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
#include "appl.h"
#include "error.h"
#include "padprm.h"
#include "timer.h"

#define WFT_CLASS	TIM_FORWARDING	/* write forwarding timeout class */
#define WFT_LENGTH	ap_pcis->padparams[FWD_TIME] /* timeout length */
#define WFT_TYPE	SIXTH_SEC_TIMER	/* forwarding timeout type */

IMPORT BUFLET *alloc_buf();		/* returns pointer to buflet chain */
IMPORT VOID add_queue();		/* installs buflet chain in queue */
IMPORT VOID free_buf();			/* returns buflet chain to free list */
IMPORT VOID int_disable();		/* disables interrupts */
IMPORT VOID int_enable();		/* enables interrupts */
IMPORT VOID mov_param();		/* application data interface */
IMPORT VOID start_timer();		/* start timer function */
IMPORT VOID stop_timer();		/* stop timer function */

IMPORT CHNLINFO *ap_pcis;		/* pointer to channel info structure */

LOCAL BUFLET *pb = NULLBUF;		/* running buflet chain pointer */
LOCAL UBYTE bdc = NULLUBYTE;		/* buflet data count */

/************************************************************************
 * LOCAL VOID queue_data()
 *
 *     queue_data is used internally by the character mode write data
 *     function and write forwarding timer routine to add a buflet chain
 *     of data to the outgoing packet queue for channel 0.
 *
 * Notes: Since static variables are used, queue_data is non-reentrant. 
 *
 * Returns: Nothing.
 *
 ************************************************************************/
LOCAL VOID queue_data()
    {

    ap_pcis->assemblypkt->bufdata[0] = bdc;	/* install buflet data count */

    /* if trailing buflets are unused in the buflet chain, free the unused
     * buflets and re-terminate the chain
     */
    if (pb->bufnext)
	{
	free_buf(pb->bufnext);
	pb->bufnext = NULLBUF;
	}

    /* add the buflet chain to the outgoing packet queue for the current
     * channel
     */
    add_queue(&ap_pcis->lcistruct->outpktqueue, ap_pcis->assemblypkt);
    int_disable();			/* disables interrupts */
    ap_pcis->nwritebytes += bdc;	/* add data count to queue count */
    int_enable();			/* enables interrupts */
    bdc = NULLUBYTE;			/* clear buflet data count */
    ap_pcis->assemblypkt = NULLBUF;	/* clear buflet chain pointer */
    }

/************************************************************************
 * LOCAL VOID write_fwd_timer(pt)
 *     TIMER *pt;		pointer to (this) timer
 *
 *     write_fwd_timer is the write forwarding timer routine. If 1 or
 *     more data bytes exist in the buflet chain provided by c_write_data,
 *     the chain will be added to the outgoing packet queue for channel 0.
 *
 * Notes: write_fwd_timer deactivates itself when called.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
LOCAL VOID write_fwd_timer(pt)
    TIMER *pt;				/* pointer to (this) timer */
    {

    /* stop this timer (note idtoclass and idtochnl are macros, defined
     * in timer.h)
     */
    stop_timer(pt->timtype, idtoclass(pt->timid), idtochnl(pt->timid));

    /* if 1 or more data bytes exist in the buflet chain provided by
     * c_write_data, above, add the chain to the outgoing packet queue
     * for the current channel (queue_data is a static function, above)
     */
    if (ap_pcis->assemblypkt && bdc)
	queue_data();
    }

/************************************************************************
 * INT c_write_data()
 *
 *     c_write_data is called from the application interface to transfer
 *     data from the application to the outgoing packet queue. Data is
 *     read from the application into a buflet chain until more than
 *     CWD_HIWATER bytes exist in the chain, and the chain is then added
 *     to the outgoing packet queue for channel 0. If less than CWD_HIWATER
 *     bytes have been read from the application, the write forwarding
 *     timer is started.
 *
 * Notes: If the write forwarding timer is active, c_write_data will
 *     temporarily stop it while processing.
 *
 * Returns: If more than MAX_WRITE_BYTES exist in the outgoing packet
 *     queue, c_write_data returns ILLEGAL_FLOW_STATE. If allocation of
 *     a buflet chain for reading application data fails, c_write_data
 *     returns NO_BUFFER_AVAIL. Otherwise, SUCCESS is returned.
 *
 ************************************************************************/
INT c_write_data()
    {
    INTERN INT bdoff;			/* buflet data offset */
    INT adoff;				/* application data offset */
    INT count;				/* byte move counter */
    WORD adc;				/* application data count */
    UWORD none = 0;			/* no bytes moved */

    /* if the number of bytes in the outgoing packet queue exceeds
     * MAX_WRITE_BYTES, return a flow control error
     */
    if (MAX_WRITE_BYTES < ap_pcis->nwritebytes)
	return (ILLEGAL_FLOW_STATE);

    /* if the length of the forwarding timer is non-zero, stop it (note
     * timer not active is ignored)
     */
    if (WFT_LENGTH)
	stop_timer(WFT_TYPE, WFT_CLASS, (UBYTE)CM_CHNL);
 
    /* request application data length
     */
    mov_param((UBYTE *)&adc, sizeof(adc), PARAM_2, 0, FROM_APPL);
    
    if (adc > CWD_HIWATER)
	adc = CWD_HIWATER;
  
    /* if no buflet chain exists (either no data has been read from the
     * application or the forwarding timeout has swiped it from us),
     * allocate a chain of CWD_BUFLETS buflets (enough to hold CWD_MAX_DATA
     * bytes). if alloc_buf returns null (buflets unavailable), an error
     * will be returned
     */
    if (!ap_pcis->assemblypkt)
	{
	if ((ap_pcis->assemblypkt = alloc_buf(CWD_BUFLETS)) == NULLBUF)
	    {
	    mov_param((UBYTE *)&none, sizeof(UWORD), PARAM_3, 0, TO_APPL);
	    return (SUCCESS);
	    }
	pb = ap_pcis->assemblypkt;	/* initialize running buflet pointer */
	bdoff = 1;			/* initialize buflet data offset */
	bdc = 0;
	}

    /* while the application data offset is less than the application
     * data count
     */
    adoff = 0;
    while (adoff < adc)
	{

	/* check the buflet data offset. if DATA_BUF_SIZ, the current buflet is
	 * full; set pb to the next buflet in the chain and clear the offset.
	 * note this is done here to insure pb will be pointing to the last
	 * buflet in the chain, even if that buflet is full
	 */
	if (bdoff == DATA_BUF_SIZ)
	    {
	    pb = pb->bufnext;
	    bdoff = 0;
	    }

	/* calculate the number of application data bytes to be installed
	 * in this buflet (note min is a macro (stddef.h); redundant calc-
	 * ulations are performed here)
	 */
	count = min(adc - adoff, DATA_BUF_SIZ - bdoff);

	/* move count bytes of application data to the current buflet
	 */
	mov_param(&pb->bufdata[bdoff], count, PARAM_1, adoff, FROM_APPL);

	/* adjust the buflet data offset (bdoff will never exceed DATA_BUF_SIZ)
	 * and application data offset
	 */
	bdoff += count;
	adoff += count;
	}

    /* add the application data count to the buflet data count. if non-zero...
     */
    bdc += adc;
    if (bdc)
   
	/* if the total number of data bytes in the buflet chain exceeds
	 * CWD_HIWATER, or the length of the write forwarding timer is non-
	 * zero, add the chain to the outgoing packet queue for the current
	 * channel (queue_data is a static function, above)
	 */
	if (CWD_HIWATER < bdc || !WFT_LENGTH)
	    queue_data();

	/* else start the write forwarding timer
	 */
	else
	    start_timer(WFT_TYPE,
		WFT_CLASS, (UBYTE)CM_CHNL, WFT_LENGTH, write_fwd_timer);

    /* report to the application the number of bytes output and return
     * SUCCESS code
     */
    mov_param((UBYTE *)&adc, sizeof(adc), PARAM_3, 0, TO_APPL);
    return (SUCCESS);
    }


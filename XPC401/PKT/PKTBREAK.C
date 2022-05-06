/************************************************************************
 * pktbreak.c - Process Break When In Packet Mode
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    pktbreak.c contains modules which send and receive breaks when
 *    device is in packet state.
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
#include "state.h"
#include "error.h"
#include "pkt.h"
 
IMPORT BUFLET *alloc_buf();             /* allocate buflet chains */
IMPORT INT send_cpkt();                 /* add buflet to queue */
IMPORT VOID free_buf();			/* free buflet chain */

IMPORT CHNLINFO *ap_pcis;	        /* pointer to channel information
					 * structure, set up by application.
					 */

IMPORT UBYTE app_chnl;			/* application channel number */
IMPORT UWORD free_count;		/* number of free buflets */

/************************************************************************
 * INT send_pkt_brk()
 *
 *    send_pkt_brk() builds begin and end break packets and 
 *    outputs them to the pad output queue.  Either both packets
 *    are written to the queue, or neither of them is
 *    written to the queue.
 *
 * Returns:  send_pkt_brk() always returns SUCCESS.
 *
 ************************************************************************/
INT send_pkt_brk()
    {
    FAST BUFLET *pb;                    /* pointer to buflet chain */
    FAST BUFLET *pbb;                   /* pointer to 2nd bufet */
 
    /* if the output queue is full, or two buffers are not
     * available, an error is returned.
     */
    if (ap_pcis->chnlstate != CHNL_CONNECTED || app_chnl == 0)
	return(ILLEGAL_CHNL_FUNC);

    /* If there are not enough buflets available then return SUCCESS but
     * do not output the break.
     */
    if (free_count > MIN_REQUIRED_BUFLETS)
    
	/* Allocate two buflets (1 for the begin break and 1 for the end
	 * break.
	 */
	if ((ap_pcis->lcistruct->outpktqueue.nchains + 1) < MAX_OUTPUT_PKTS)
	    {
	    if ((pb = alloc_buf((BYTES)2)) == NULLBUF)
		return (SUCCESS);
	    pbb = pb->bufnext;
	    pb->bufnext = NULLBUF;
    
	    /* Build begin and end break packets.
	     */
	    pbb->bufdata[STX] = pb->bufdata[STX] = STX_CHAR;
	    pbb->bufdata[GFI_LCI] = pb->bufdata[GFI_LCI] =
		QBIT | ap_pcis->logicalchnl;
	    (VOID)send_cpkt(ap_pcis->lcistruct, BEGIN_BRK, (UBYTE *)0,
		CHECK_LINK, pb);
	    (VOID)send_cpkt(ap_pcis->lcistruct, END_BRK, (UBYTE *)0,
		MUST_LINK,  pbb);
	    }
    return (SUCCESS);
    }
 
 
/************************************************************************
 * VOID recv_begin_brk(pcis, ppkt)
 *    CHNLINFO *pcis;   - pointer to channel information structure
 *    BUFLET *ppkt;	- pointer to incoming begin break packet
 *
 *    recv_begin_brk() currently discards the packet and returns.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID recv_begin_brk(pcis, ppkt)
    CHNLINFO *pcis;			/* pointer to channel information
    					 * structure (not used).
					 */
    BUFLET *ppkt;                       /* pointer to input packet */
    {
    
    free_buf(ppkt);
    }
 
/************************************************************************
 * VOID recv_end_brk(pcis, ppkt)
 *    CHNLINFO *pcis;	- pointer to channel information structure 
 *    BUFLET *ppkt;	- pointer to incoming receive break packet 
 *
 *    recv_end_brk() sets the break flag in the channel information
 *    structure, discards the packet and returns.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID recv_end_brk(pcis, ppkt)
    CHNLINFO *pcis;			/* pointer to channel information
    					 * structure.
    					 */
    BUFLET *ppkt;                       /* pointer to input packet */
    {
 
    pcis->breakstatus |= BREAK_BIT;
    free_buf(ppkt);
    }


/************************************************************************
 * linkxmit.c  - Link Xmit Packet to Output Queue
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *      linkxmit.c contains link_xmit which attemps to link the
 *      transmit buffer to the output queue.  If the output queue
 *      is full and the packet does not have to be linked to the
 *      output queue, flow control is turned on.  Otherwise, the
 *      assembly transmit packet is linked to the output queue
 *      and the assembly transmit pointers are reintialized.
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
#include "pkt.h"
#include "error.h"
	
IMPORT CRC get_crc();			/* get the crc for data */
IMPORT VOID add_queue();		/* add packet to queue */
IMPORT VOID free_buf();			/* free buflet */

IMPORT UWORD program_error;		/* programming error */
#ifdef DEBUG
IMPORT WORD diags[];			/* Used for debugging */
LOCAL WORD cnt = 401;			/* Used for debugging */
#endif
	
/************************************************************************
 * INT link_xmit_pkt(pcis, must_link)
 *    CHNLINFO *pcis;   - pointer to channel information structure 
 *    BOOL must_link;	- must link packet to output queue
 *
 *    link_xmit_pkt() links the transmit assembly packet to the pad 
 *    output queue.   The header for the data packet is built, the first
 *    byte of data is moved to its' correct packet position, a
 *    crc is calculated for the extra data and the packet
 *    is linked to the output queue.  
 *
 * Returns: If link_xmit_pkt() can not link the transmit assembly 
 *    packet to the output queue and the transmit assembly packet is
 *    not full QUEUE_FULL is returned.  If link_xmit_pkt() cannot link
 *    the transmit assembly packet and the packet is full,
 *    STOP_WRITE is returned. 
 *        If the transmit assembly packet is linked, link_xmit_pkt() 
 *   returns SUCCESS. 
 *    
 *
 ************************************************************************/
INT link_xmit_pkt(pcis, must_link)
    FAST CHNLINFO *pcis;		/* pointer to channel information
    					 * structure.
    					 */
    BOOL must_link;			/* must link assembly packet */
    {
    CRC crc;				/* crc for extra data */
    BUFLET *ppkt;			/* transmit buffer */
    UWORD idx;				/* transmit index */
	

    /* if the queue is not full or the assembly packet must be
     * linked, the packet header is built (including the CRC for 
     * the data), the packet is linked to the output queue, and
     * the transmit assembly pointers are updated.
     */
    
    if (pcis->nwritebytes > 0)
	{
	
	/* Set idx and ppkt to point to the end of the
	 * transmit assembly packet.
	 */
	for (idx = pcis->nwritebytes + MOVED_FIRST_BYTE,
	    ppkt = pcis->assemblypkt; idx >= DATA_BUF_SIZ && ppkt;
	    ppkt = ppkt->bufnext, idx -= DATA_BUF_SIZ)
	    ;
	
	/* There is a programming error. Stop processing.
	 */
	if (!ppkt)
	    {
	    program_error = XMIT_LINK_ERROR;
	    return(STOP_WRITE);
	    }

	/* Either there is room in the queue for the packet or
	 * the packet must be linked to the output queue.
	 * Set up the header for the assembly packet
	 */
	if ((pcis->lcistruct->outpktqueue.nchains < MAX_OUTPUT_PKTS)
	    || must_link)
	    {
	    pcis->assemblypkt->bufdata[FRAME_LEN] = pcis->nwritebytes - 1;
	    pcis->assemblypkt->bufdata[STX] = STX_CHAR;
	    pcis->assemblypkt->bufdata[GFI_LCI] = pcis->logicalchnl;
	    pcis->assemblypkt->bufdata[FIRST_DATA_BYTE] = 
	    pcis->assemblypkt->bufdata[MOVED_FIRST_BYTE];

	    /* Calculate the crc for the extra data.   The first byte of
	     * data is included in the header's crc.  If the crc will
	     * not fit in the current buflet, go the next buflet.
	     */
	    crc = get_crc(pcis->assemblypkt,(BYTES)EXTRA_DATA, 
		(BYTES)(pcis->nwritebytes - 1));

	    /* Store the crc for the extra data.
	     */
	    if (idx == DATA_BUF_SIZ)
		{
		ppkt = ppkt->bufnext;
		idx = 0;
		}
	    ppkt->bufdata[idx++] = (UBYTE)(crc >> 8);
	    if (idx == DATA_BUF_SIZ)
		{
		ppkt = ppkt->bufnext;
		idx = 0;
		}
	    ppkt->bufdata[idx] = (UBYTE)(crc &0x0ff);
 	
	    /* Free any of the extra buflets which were not used
	     * by the assembly packet.
	     */
	    if (ppkt->bufnext != NULLBUF)
		{
		free_buf(ppkt->bufnext);
		ppkt->bufnext = NULLBUF;
		}
	    
#ifdef DEBUG
	    if (diags[cnt] > 30000)
		cnt++;
	    int_disable();
	    diags[cnt] = diags[cnt] + pcis->nwritebytes;
	    int_enable();
#endif
	    /* Add the transmit assembly packet to the pad output queue.
	     */
	    add_queue(&pcis->lcistruct->outpktqueue, pcis->assemblypkt);
	    
	    /* Clear the pointer and length for the transmit assembly packet.
	     */
	    pcis->assemblypkt = NULLBUF;
	    pcis->nwritebytes = 0;
	    pcis->holdassembly = NO;
	    }
	else
	    {
	    
	    /* set the flow control flag 
	     */
	    pcis->holdassembly = YES;
	    
	    /* If the packet is being linked because it is full, return
	     * one error code.  Otherwise, return an error code which
	     * will allow the addition of more data to the transmit
	     * assembly packet.  Outputting will stop when the
	     * transmit assembly packet is full and cannot be linked.
	     */
	    if (pcis->nwritebytes == MAX_DATA_PKT)
		return (STOP_WRITE);
	    return (QUEUE_FULL);
	    }
	}
    return (SUCCESS);
    }





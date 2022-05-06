/************************************************************************
 * bldcpkt.c - Build Control Packets For Link
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    bldcpkt.c contains the source module send_link_cpkt().
 *    send_link_cpkt() is called to build link control packets and add
 *    the control packets to the link output queue..
 *    All packets in the link output queue are built during
 *    link processing and are c_bit control packets (bit 0x80 is set).
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
#include "link.h"
#include "error.h"

IMPORT BUFLET *alloc_buf();		/* allocate buflet chain */
IMPORT CRC get_crc();			/* get crc for extra data */
IMPORT VOID add_queue();		/* add buflet chain to queue */
IMPORT VOID free_buf();			/* free buflet chain */

IMPORT LCIINFO lcis[];			/* logical channel information
					 * structure.
					 */
IMPORT UBYTE linkchnl;			/* link channel number */
#ifdef DEBUG
IMPORT WORD diags[];			/* used for debugging */
#endif

/************************************************************************
 * VOID send_link_cpkt(ppkt, type, data1, data2, len)
 *    BUFLET *ppkt;	- pointer to buflet packet which will be used
 * 			  to build the control packet.
 *    UBYTE type;	- type of control packet
 *    UBYTE *data1;	- a pointer to the first byte of extra data
 *    UBYTE *data2;	- a pointer to the rest of the extra data
 *    UBYTE len;	- the length of the data2 data
 *
 *    send_link_cpkt() builds and outputs a packet during link processing.
 *    The packet is built using the type specified by the caller.  If
 *    data1 is not NULL, then it is pointing to one byte of data.
 *    If data2 is not NULL, then it is pointing to len number of
 *    bytes of data.  After the packet is built, it is added
 *    to the link output queue. 
 *
 * Usage notes: send_link_cpkt() assumes that the output packet can be
 *    contained in one buflet.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID send_link_cpkt(ppkt, type, data1, data2, len)
    FAST BUFLET *ppkt;                  /* pointer to packet */
    UBYTE type;                         /* type of function to process */
    UBYTE *data1;                       /* extra data  byte*/
    UBYTE *data2;			/* rest of extra data */
    UBYTE len;				/* length of data2  
					 * data must be contained in one
					 * buflet.
					 */
    {
    CRC crc;                            /* crc for extra data */
    UBYTE i;				/* loop counter */
    UBYTE n;				/* loop counter */
    
    /* Link control packets need a 1 buflet packet.  Free all but the 
     * first buflet in the buflet chain.
     */
    if (ppkt->bufnext != NULLBUF)
        {
        free_buf(ppkt->bufnext);
        ppkt->bufnext = NULLBUF;
        }

    ppkt->bufdata[STX] = STX_CHAR;

    /* If this is a diagnostic packet, then make sure that the
     * channel number is zero.
     */
    if (type == DIAG)
	ppkt->bufdata[GFI_LCI] = CBIT;
    else
	ppkt->bufdata[GFI_LCI] = CBIT | linkchnl;
    ppkt->bufdata[PKT_TYP_ID] = (UBYTE)type;
    
    /* If there is data for the packet, move data into packet,
     * calculate crc for extra data and move crc into packet.
     */
    if (data1)
        {
	n = EXTRA_DATA;
	ppkt->bufdata[FRAME_LEN] = len + 1;
	ppkt->bufdata[n++] = *data1;
	
	/* If there is more than 1 byte of data for the packet, move
	 * the additional data into the packet.
	 */
	if (data2)
	    {
	    i = 0;
	    while (i < len && i < (QPKT_DATA_SIZ - 1))
		ppkt->bufdata[n++] = data2[i++];
	    }
	
	/* Build and store crc for the extra data.
	 * It is assumed that the packet will only need one buflet.
	 * So there is no check for the end of the buflet.
	 */
	crc = get_crc(ppkt, EXTRA_DATA, len + 1);

	/* Move the first byte of the crc into the packet.  By shifting
	 * crc to right 8 bits, the high order 8 bits are stored.
	 */
	ppkt->bufdata[n++] = (UBYTE)(crc >> 8);
	
	/* Store the second byte of the crc in the packet.
	 * (the low order 8 bits of the crc).
	 */
        ppkt->bufdata[n] = (UBYTE)(crc & 0xff);
        }
    else
	
	/* There is not data, so set the frame length to zero.
	 */
        ppkt->bufdata[FRAME_LEN] = 0;
    
    /* Add packet to the link output queue.
     */
    add_queue(&lcis[linkchnl].linkoutqueue, ppkt);
    }




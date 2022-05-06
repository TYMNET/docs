/************************************************************************
 * sendctrl.c  - Build and Send Control Packets
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    sendctrl.c contains the routines which are called to build 
 *    and send q-bit control  packets.  send_ctrl_data_pkt builds q-bit
 *    packets which may contain more than one byte of data.  send_ctrl_pkt
 *    builds q-bit control  packets with at most one byte of data.  send_cpkt
 *    builds a q-bit control packet using  the current packet.  
 *    The q-bit control packets are added to the pad output queue.
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
#include "appl.h"
#include "error.h"
 
                                            /* calc_num_buflets();    
                                             * a macro used to calculate
                                             * number of buflets needed for
                                             * packet.   The macro is defined
                                             * in xpc.h
                                             */
IMPORT BUFLET *alloc_buf();                 /* get a buflet */
IMPORT CRC get_crc();                       /* generate crc for extra data bytes*/ 
IMPORT VOID add_queue();                    /* add buflet to queue */
IMPORT VOID free_buf();			    /* free buflet chain */
IMPORT VOID mov_param();		    /* move data to/from application
					     * parameter buffer.
					     */

#ifdef DEBUG
IMPORT WORD diags[];    		    /* used for debugging. */
#endif
/************************************************************************
 * INT send_ctrl_data_pkt(plcis, lu, type, len, check_que)
 *    LCIINFO *plcis;   - pointer to logical channel information
 *                        structure.
 *    UBYTE lu;		- logical channel number
 *    UBYTE type;	- type of q-bit control  packet to build
 *    WORD len;		- maximum number of bytes to move into packet
 *    BOOL check_que;	- link packet to output queue even if queue
 *                        is full
 *
 *    send_ctrl_data_pkt() builds q-bit control  packets which have more
 *    than 1 bit of data.  Currently there are only two types
 *    session request and session accept.
 *
 * Usage notes: send_ctrl_data_pkt assumes that the data to be sent
 *    is contained in the first application parameter buffer. If this
 *    changes, send_ctrl_data_pkt must be changed.
 *
 * Returns:  
 *    SUCCESS- Processing was successful.
 *    NO_BUFFERS_AVAIL - There are no buflet chains available for
 *        packet.
 *    QUEUE_FULL - The calling program specified check_que = yes, and
 *        the output queue is full.
 *
 ************************************************************************/
INT send_ctrl_data_pkt(plcis, lu, type, len, check_que)
    FAST LCIINFO *plcis;		    /* pointer to logical channel
    					     * information structure.
    					     */
    UBYTE lu;				    /* logical unit number */
    UBYTE type;                             /* type of function to process */
    WORD len;                               /* length of data */
    BOOL check_que;                         /* need to get link */
    {
    BUFLET *ppkt;                           /* pointer to buflet */
    BUFLET *pb;                             /* pointer to end data */
    CRC crc;                                /* crc for extra data */
    BYTE nleft;                             /* number of bytes left to copy */
    BYTE ipos;                              /* index into input buffer */
    BYTE n;
 
    /* If packet does not have to be linked to output queue, return
     * return error if output queue is full.
     */
    if (check_que)
        if (plcis->outpktqueue.nchains == MAX_OUTPUT_PKTS)
            return (QUEUE_FULL);
 
    /* Calculate number of buflets needed for packet, allocate buflet
     * chain and  move control  information into packet.
     */    
    n = calc_num_buflets(len);
    if ((pb = ppkt = alloc_buf((BYTES)n)) == NULLBUF)
        return (NO_BUFFER_AVAIL);
    pb->bufdata[STX] = STX_CHAR;
    pb->bufdata[FRAME_LEN] = (UBYTE)len;
    pb->bufdata[GFI_LCI] = QBIT | lu;
    pb->bufdata[PKT_TYP_ID] = (UBYTE)type;
    n = (len > QPKT_DATA_SIZ) ? QPKT_DATA_SIZ : len;
    mov_param((UBYTE *)&pb->bufdata[EXTRA_DATA], n, PARAM_1, 0, FROM_APPL);
  
    /* Move data from application parameter 1 buffer into
     * buflet.
     */
    if (len > n)
        {
	for (ipos = n, nleft = len - n; nleft > 0; nleft -= n, ipos += n)
	    {
	    pb = pb->bufnext;
	    n = (nleft > DATA_BUF_SIZ) ? DATA_BUF_SIZ : nleft;
	    mov_param((UBYTE *)pb->bufdata, n, PARAM_1, ipos, FROM_APPL);
	    }
	}
    else
	n += EXTRA_DATA;
 
    /* Calculate the crc for extra data in q-bit control  packet.  The
     * packet frame crc is calculated by link before packet is
     * sent.
     */
    crc = get_crc(ppkt, (BYTES)EXTRA_DATA, (BYTES)len);
    
    if (n == DATA_BUF_SIZ)
        {
        pb = pb->bufnext;
        n = 0;
        }
    
    /* Move the first 8 bits of the crc into the packet.
     */
    pb->bufdata[n++] = (UBYTE)(crc >> 8);
    
    /* If this is the end of the buflet, go to the next one.
     */
    if (n == DATA_BUF_SIZ)
        {
        pb = pb->bufnext;
        n = 0;
        }
    
    /* Move the second 8 bits of the crc into the packet.
     */
    pb->bufdata[n] = (UBYTE)(crc & 0xff);
 
    /* Add packet to the pad output queue.
     */
    add_queue(&plcis->outpktqueue, ppkt);
    return (SUCCESS);
    }


/************************************************************************
 * INT send_ctrl_pkt(plcis, lu, type, data, check_que)
 *   LCIINFO *plcis;    - pointer to logical channel information structure
 *   UBYTE lu;		- logical unit number
 *   UBYTE type;	- packet function type
 *   UBYTE *data;	- pointer to at most 1 data byte
 *   BOOL check_que;	- must link packet to queue even if packet is full
 *
 *    send_ctrl_pkt() builds q-bit control packets which contain at most one
 *    byte of data.  The packets are added to the pad output queue.
 * 
 * Returns:
 *    SUCCESS- Processing was successful.
 *    NO_BUFFERS_AVAIL - There are no buflet chains available for
 *        packet.
 *    QUEUE_FULL - The calling program specified check_que = yes, and
 *        the output queue is full.
 ************************************************************************/
INT send_ctrl_pkt(plcis,lu, type, data, check_que)
    FAST LCIINFO *plcis;		    /* pointer to logical channel
    					     * information structure.
    					     */
    UBYTE lu;				    /* logical unit number */
    UBYTE type;                             /* type of function to process */
    UBYTE *data;                            /* pointer to data byte */
    BOOL check_que;                         /* need to get link */
    {
    BUFLET *pb;                             /* pointer to buflet packet */
    CRC crc;                                /* crc for extra data */
    BYTE n = EXTRA_DATA;
    
    /* If this packet doesn't have to be sent, return error 
     * when the output queue is full.
      */
    if (check_que)
        if (plcis->outpktqueue.nchains == MAX_OUTPUT_PKTS)
            return (QUEUE_FULL);
 
    /* Allocate buflet chain for packet, move control information
     * into packet.
     */
    if ((pb = alloc_buf((BYTES)1)) == NULLBUF)
        return (NO_BUFFER_AVAIL);
    pb->bufdata[STX] = STX_CHAR;            /* set up STX character */
    pb->bufdata[GFI_LCI] = QBIT | lu;
    pb->bufdata[PKT_TYP_ID] = (UBYTE)type;
  
    /* If there is a data byte, move data into packet, calculate
     * extra data's crc, and move crc into packet. The frame's 
     * crc is calculated by link before packet is written.
     */
    if (data)
        {
        pb->bufdata[FRAME_LEN] = 1;         /* extra data length */
        pb->bufdata[n++] = *data;

	/* Calculate the crc.  Move the crc into the packet 8 bits at a
	 * time.  Because there is at most one byte of extra data, the
	 * crc cannot be extended to a second buflet.
	 */
        crc = get_crc(pb, (BYTES)EXTRA_DATA, (BYTES)1);
        pb->bufdata[n++] = (UBYTE)(crc >> 8);
        pb->bufdata[n] = (UBYTE)(crc & 0xff);
        }
    
    /* Set the number of bytes of extra data to 0.
     */
    else
        pb->bufdata[FRAME_LEN] = 0;
  
    /* Add the packet to the pad output queue.
     */
    add_queue(&plcis->outpktqueue, pb);
    return (SUCCESS);
    }

/************************************************************************
 * INT send_cpkt(plcis, type, data, check_que, ppkt)
 *   LCIINFO *plcis     - pointer to logical channel information structure
 *   UBYTE type;	- packet function type
 *   UBYTE *data;	- pointer to at most 1 data byte
 *   BOOL check_que;	- must link packet to queue even if packet is full
 *   BUFLET *ppkt;	- pointer to buflet packet.
 *
 *    send_cpkt() builds q-bit control  packets which contain at most one
 *    byte of data.  The input packet is reused by send_cpkt().
 * 
 * Returns:
 *    SUCCESS- Processing was successful.
 *    QUEUE_FULL - The calling program specified check_que = yes, and
 *        the output queue is full.
 ************************************************************************/
INT send_cpkt(plcis, type, data,  check_que, ppkt)
    FAST LCIINFO *plcis;		    /* pointer to logical channel
    					     * information structure.
    					     */
    UBYTE type;                             /* type of function to process */
    UBYTE *data;                            /* extra data */
    BOOL check_que;                         /* need to get link */
    FAST BUFLET *ppkt;                      /* pointer to packet */
    {
    CRC crc;                                /* crc for extra data */
    BYTE n = EXTRA_DATA;
    
 
    /* If packet doesn't have to be linked to output queue,
     * return error when queue is full.
     */
    if (check_que)
        if (plcis->outpktqueue.nchains == MAX_OUTPUT_PKTS)
            return (QUEUE_FULL);
    
    /* If the packet uses more than one buflet, free the extra buflets
     * from the packet.
     */
    if (ppkt->bufnext)
        {
        free_buf(ppkt->bufnext);
        ppkt->bufnext = NULLBUF;
        }
    ppkt->bufdata[PKT_TYP_ID] = (UBYTE)type;

    /* If there is data for new packet, move data into packet,
     * calculate crc for extra data and move crc into packet.
     */
    if (data)
        {
        ppkt->bufdata[FRAME_LEN] = 1;
        ppkt->bufdata[n++] = *data;
        
	/* Calculate the crc.  Move the crc into the packet 8 bits at a
	 * time.  Because there is at most one byte of extra data, the
	 * crc cannot be extended to a second buflet.
	 */
	crc = get_crc(ppkt, (BYTES)EXTRA_DATA, (BYTES)1);
        ppkt->bufdata[n++] = (UBYTE)(crc >> 8);
        ppkt->bufdata[n] = (UBYTE)(crc & 0xff);
        }
    
    /* Set the number of bytes of extra data to 0.
     */
    else
        ppkt->bufdata[FRAME_LEN] = 0;

    /* Add packet to the pad output queue.
     */
    add_queue(&plcis->outpktqueue, ppkt);
    return (SUCCESS);
    }
    


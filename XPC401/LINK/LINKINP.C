/************************************************************************
 * linkinp.c - Link Input Packets
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    linkinp.c contains the source module link_input(), which is called to
 *    read the data from iocomm's circular read buffer, build
 *    and validate the packet.
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
#include "iocomm.h"
#include "pkt.h"
#include "link.h"

IMPORT BUFLET *alloc_buf();		/* allocate buflet chain */
IMPORT CRC get_crc();			/* get crc for data */
IMPORT VOID free_buf();			/* discard packet */
IMPORT VOID validate_pkt();		/* validate input packet */

IMPORT BOOL in_a_frame;			/* flag which indicates if link_input
					 * is currently building an input
					 * packet.
					 */
IMPORT BUFLET *frame_pntr;		/* buflet pointer for next character
					 * of data
					 */
IMPORT COMMINFO comm_info;		/* communication informations
					 * structure.
					 */
IMPORT LCIINFO lcis[];			/* logical channel information
					 * structure.
					 */
#ifdef DEBUG
IMPORT UWORD free_count;		/* XXX */
IMPORT WORD diags[];			/* XXX */
#endif

/************************************************************************
 * VOID link_input()
 *
 *    link_input() builds incoming packets. link_input()
 *    reads characters from iocomm's circular read buffer, builds
 *    input packets, does validation of the packet header 
 *    validate_packet() is called  to validate the rest of the packet.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID link_input()
    {
    CRC crc;				/* crc which is calculated and
					 * checked against the crc in
					 * the packet header.
					 */
    BUFLET *pb;				/* buflet pointer */
    UBYTE c = 0;			/* input character */
    LOCAL BUFLET *cframe = NULLBUF;	/* Pointer to the current buflet 
					 * for the input packet.
    					 */
    LOCAL WORD inp_idx = 0;		/* index into buflet */
    LOCAL WORD inp_len = 0;		/* Number of bytes of extra data in
					 * the input packet.  inp_len does
					 * not include the length of
				         * the packet header.
					 */
    LOCAL UBYTE *next_pntr = 0;		/* Pointer which is used to 
					 * start building the next 
					 * packet if the input packet
					 * crc is incorrect.  It is
					 * a pointer to the character
				 	 * after the packet's STX.
					 */
    LOCAL UWORD num_chars_inframe = 0;	/* number of characters 
					 * in frame.
					 */

    /* If processing stopped when a packet was being built, and
     * processing stopped because, there was no buflet available,
     * then try to allocate a buflet.
     */
    if (in_a_frame)
	{
	
	/* If the last time link_input was called a buflet was not available
	 * try again. 
	 */
	if (inp_idx == DATA_BUF_SIZ)
	    {
	    if ((pb = alloc_buf(1)) == NULLBUF)
		return;
	    cframe->bufnext = pb;
	    cframe = pb;
	    inp_idx = 0;
 	    }
	}
	
    /* A new input packet is being built.  If there is not buflet
     * allocated for the packet, allocate another buflet.
     */
    else if (frame_pntr == NULLBUF)
	{
	if ((frame_pntr = cframe = alloc_buf(1)) == NULLBUF)
	    return;
	}
    
    /* While there is more data in the incoming circular buffer,
     * read data from the incoming link queue.
     */
    while (comm_info.readrecvbuf != comm_info.wrtrecvbuf)
	{
	c = *comm_info.readrecvbuf;
	
	/* If looking for the start of a new packet, then search
	 * for the STX character.  All characters before the
	 * STX character are skipped.
	 */
	if (!in_a_frame)
	    {
	    if (c == STX_CHAR)
		{

		/* Initialize the start of a frame -- the pointer to
		 * the next character in the frame, the number of
		 * characters in the frame,  and the index into the buflet.
		 * If the CRC is not valid, then characters are reread
		 * starting at the pointer to the next character in the
		 * frame.
		 */
		in_a_frame = YES;
		cframe = frame_pntr;
		num_chars_inframe = 1;
		inp_idx = 0;
		cframe->bufdata[inp_idx++] = STX_CHAR;
		}
	    
	    /* If reached the end of input buffer, then reset pointer to
	     * beginning of circular input buffer.
	     */
	    if (++comm_info.readrecvbuf > comm_info.endrecvbuf)
		comm_info.readrecvbuf = comm_info.begrecvbuf;
	    
	    /* Save the pointer to next character in IOCOMM'S circular read
	     * buffer. If the current packet is not valid, the building 
	     * of a new packet starts with the character at next_pntr.
	     */
	    next_pntr = comm_info.readrecvbuf;
	    continue;
	    }
	
	/* If the current buflet is full, add another buflet to the
	 * chain.  Return if no buflet is available.
	 */
	if (inp_idx == DATA_BUF_SIZ)
	    {
	    if ((pb = alloc_buf(1)) == NULLBUF)
		break;
	    cframe->bufnext = pb;
	    cframe = pb;
	    inp_idx = 0;
 	    }
	    
	/* If reached the end of the circular buffer, the set pointer to
	 * the beginning of the circular buffer.
	 */ 
        if (++comm_info.readrecvbuf > comm_info.endrecvbuf)
	    comm_info.readrecvbuf = comm_info.begrecvbuf;
	
	/* Move character into input packet.
	 */
	cframe->bufdata[inp_idx++] = c;
	
	/* If this character is the frame length, save it.  The frane
	 * length contains the length of extra data.  If there is extra
	 * data then there is an addition 2 charactere crc for the
	 * extra data.
	 */
	if (num_chars_inframe == FRAME_LEN)
	    if ((inp_len = (WORD)cframe->bufdata[FRAME_LEN]) > 0)
		inp_len += 2;

	/* If the header CRC was just put in packet, validate
	 * the crc.
	 */
	if (num_chars_inframe++ ==  (CRC1 + 1))
	    {
	    crc = get_crc(frame_pntr, FRAME_LEN, (CRC1 - FRAME_LEN));
	    

 	    /* The packet is not valid, start rebuilding a new packet
	     * starting at the character following the current STX.
	     */
	    if ((cframe->bufdata[CRC1] != (UBYTE)(crc >> 8))
		|| cframe->bufdata[CRC1 + 1] != (UBYTE)(crc & 0xff))
		{
		comm_info.readrecvbuf = next_pntr;
		++comm_info.linkstats[NBR_CRC_ERRS];
		in_a_frame = NO;
		continue;
		}
	    }
	
	/* Check for the end of the packet.  If the packet is complete,
	 * validate and process packet depending on current state of
	 * link (restart, reset, or flow control).
	 */
	
	if (num_chars_inframe < (inp_len + EXTRA_DATA))
	    continue;
					
	/* Increment the number of input packets processed.  This count
	 * is used for the link statistics.
	 */
	++comm_info.linkstats[PKTS_RECVD];
	
	/* Validate the input packet.
	 */
 	validate_pkt(frame_pntr);
	
	/* Start building the next packet.
	 */
	in_a_frame = NO;
	if ((cframe = frame_pntr = alloc_buf(1)) == NULLBUF)
	    break;
	}
    }


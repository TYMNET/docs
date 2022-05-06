/************************************************************************
 * monplink.c - X.PC line monitor link interface (packet mode)
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    This module contains routines responsible for link level input to
 * the X.PC line monitor.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    INI   Initial Draft
 *
 ************************************************************************/
#include "stddef.h"
#include "xpc.h"
#include "iocomm.h"
#include "mon.h"
#include "pkt.h"
#include "link.h"

IMPORT BUFLET *alloc_buf();		/* allocates buflet(s) */
IMPORT DATA *alloc_data();		/* allocates data entry */
IMPORT CRC get_crc();			/* calculates crc */
IMPORT VOID put_data();			/* adds data entry to received queue */
IMPORT VOID validate_pkt();		/* validates received packet */

IMPORT COMMINFO comm_info;		/* comm info structure 1 */
IMPORT COMMINFO comm_info2;		/* comm info structure 2 */

/************************************************************************
 * VOID p_link()
 *
 *     p_link is the X.PC line monitor link level input routine for
 * packet mode. This routine is called by the timer interrupt code
 * each time the interrupt is received.
 *
 * Notes: p_link runs with interrupts enabled.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID p_link()
    {
    CRC crc;				/* calculated crc 1 */
    UBYTE c;				/* saves received character */
    INTERN BOOL in_a_frame = NO;	/* stx received flag */
    INTERN BUFLET *ppkt;		/* pointer to current buflet */
    INTERN DATA *pd = NULLDATA;		/* received data pointer */
    INTERN WORD inp_idx;		/* buflet input index */
    INTERN WORD inp_len;		/* buflet input count */
    INTERN UBYTE dcount;		/* character drop count */
    INTERN UBYTE *next_pntr;		/* input re-scan pointer */
    INTERN UWORD num_chars_inframe;	/* frame character count */
        
    /* while data to be read exists in the circular read buffer
     */
    while (comm_info.readrecvbuf != comm_info.wrtrecvbuf)
        {
    
    	/* if data entry not yet allocated...
	 */
        if (!pd)
            {
	
	    /* skip read if data alloc fails
	     */
            if ((pd = alloc_data(1)) == NULLDATA)
                return;
            ppkt = pd->pbuf;
            inp_idx = num_chars_inframe = 0;
	    dcount = 0;
            }
	    
	/* read input character
	 */
        c = *comm_info.readrecvbuf;
        if (!in_a_frame)		/* if waiting for STX */
            {
            if (c == STX_CHAR)		/* if STX received */
                {
	
		/* if dropped characters exist in the current buflet,
		 * save the drop count in the first data position of the 
		 * buflet, add the data to the received data queue,
		 * clear the data pointer, and continue (a new data entry
		 * will be allocated, above)
		 */
		if (dcount)
		    {
		    ppkt->bufdata[0] = dcount;
		    put_data(pd, 1, 2);
		    pd = NULLDATA;
		    continue;
		    }
		
		/* else assume the STX received is the start of a packet;
		 * install in buflet, set STX received flag and initialize
		 * counters
		 */
		ppkt->bufdata[0] = c;
                in_a_frame = YES;
                inp_idx = num_chars_inframe = 1;
                }
		
	    /* non-STX characters received while waiting for STX. if the
	     * buflet is full, save the drop count in the first data position 
	     * of the buflet, add the data to the received data queue, clear
	     * the data pointer, and continue (a new data entry will be
	     * allocated, above)
	     */
            else if (dcount == DATA_BUF_SIZ - 1)
                {
		ppkt->bufdata[0] = dcount;
		put_data(pd, 1, 2);
		pd = NULLDATA;
		continue;
                }
	    /* else add the dropped character to the current buflet and
	     * bump the dropped character count
	     */
	    else
		ppkt->bufdata[++dcount] = c;
	    
	    /* bump the read pointer to the circular read buffer and check
	     * for wrap. save the read pointer for possible re-scan in case
	     * header crc's conflict
	     */
            if (++comm_info.readrecvbuf > comm_info.endrecvbuf)
                comm_info.readrecvbuf = comm_info.begrecvbuf;
            next_pntr = comm_info.readrecvbuf;
            continue;
            }
	    
	/* if the current buflet is full, allocate and link a new buflet
	 */
        if (inp_idx == DATA_BUF_SIZ)
            {
            if ((ppkt->bufnext = alloc_buf(1)) == NULLBUF)
                return;
            ppkt = ppkt->bufnext;
            inp_idx = 0;
            }
	/* bump the read pointer to the circular read buffer and check
	 * for wrap
	 */
        if (++comm_info.readrecvbuf > comm_info.endrecvbuf)
            comm_info.readrecvbuf = comm_info.begrecvbuf;
	
	/* save received character
	 */
        ppkt->bufdata[inp_idx++] = c;
	
	/* if entire packet header has been received, save extra data length.
	 * if non-zero, bump to accomodate data crc
	 */
        if (num_chars_inframe == FRAME_LEN)
            if ((inp_len = ppkt->bufdata[FRAME_LEN]))
                inp_len += sizeof(CRC);
	
	/* if header crc has been received, validate
	 */
        if (num_chars_inframe++ == CRC1 + 1)
            {
            crc = get_crc(pd->pbuf, FRAME_LEN, CRC1 - FRAME_LEN);
            if ((ppkt->bufdata[CRC1] != (UBYTE)(crc >> 8))
                || ppkt->bufdata[CRC1 + 1] != (UBYTE)(crc & 0xff))
                {
	
		/* invalid header crc received. flag the error, add the data
		 * entry to the received data queue, reset data pointer and 
		 * STX received flag, and set the read pointer to the circular
		 * read buffer back to where the last STX was read
		 */
                pd->pbuf->bufdata[0] = MON_CRC1_ERR;
                put_data(pd, 1, 1);
		pd = NULLDATA;
                in_a_frame = NO;
                comm_info.readrecvbuf = next_pntr;
                continue;
                }
            }
	    
	/* if entire packet received, validate, add to received data queue,
	 * reset data pointer and STX received flag, and continue
	 */
        if (inp_len + EXTRA_DATA <= num_chars_inframe)
	    {
	    validate_pkt(pd->pbuf);
	    put_data(pd, 1, 1);
	    pd = NULLDATA;
	    in_a_frame = NO;
	    }
        }
    }

/************************************************************************
 * VOID p_link2()
 *
 *     p_link2 is the X.PC line monitor link level input routine for
 * packet mode. This routine is called by the timer interrupt code
 * each time the interrupt is received.
 *
 * Notes: p_link2 runs with interrupts enabled.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID p_link2()
    {
    CRC crc;				/* calculated crc 1 */
    UBYTE c;				/* saves received character */
    INTERN BOOL in_a_frame = NO;	/* stx received flag */
    INTERN BUFLET *ppkt;		/* pointer to current buflet */
    INTERN DATA *pd = NULLDATA;		/* received data pointer */
    INTERN WORD inp_idx;		/* buflet input index */
    INTERN WORD inp_len;		/* buflet input count */
    INTERN UBYTE dcount;		/* character drop count */
    INTERN UBYTE *next_pntr;		/* input re-scan pointer */
    INTERN UWORD num_chars_inframe;	/* frame character count */
        
    /* while data to be read exists in the circular read buffer
     */
    while (comm_info2.readrecvbuf != comm_info2.wrtrecvbuf)
        {
    
    	/* if data entry not yet allocated...
	 */
        if (!pd)
            {
	
	    /* skip read if data alloc fails
	     */
            if ((pd = alloc_data(1)) == NULLDATA)
                return;
            ppkt = pd->pbuf;
            inp_idx = num_chars_inframe = 0;
	    dcount = 0;
            }
	    
	/* read input character
	 */
        c = *comm_info2.readrecvbuf;
        if (!in_a_frame)		/* if waiting for STX */
            {
            if (c == STX_CHAR)		/* if STX received */
                {
	
		/* if dropped characters exist in the current buflet,
		 * save the drop count in the first data position of the 
		 * buflet, add the data to the received data queue,
		 * clear the data pointer, and continue (a new data entry
		 * will be allocated, above)
		 */
		if (dcount)
		    {
		    ppkt->bufdata[0] = dcount;
		    put_data(pd, 2, 2);
		    pd = NULLDATA;
		    continue;
		    }
		
		/* else assume the STX received is the start of a packet;
		 * install in buflet, set STX received flag and initialize
		 * counters
		 */
		ppkt->bufdata[0] = c;
                in_a_frame = YES;
                inp_idx = num_chars_inframe = 1;
                }
		
	    /* non-STX characters received while waiting for STX. if the
	     * buflet is full, save the drop count in the first data position 
	     * of the buflet, add the data to the received data queue, clear
	     * the data pointer, and continue (a new data entry will be
	     * allocated, above)
	     */
            else if (dcount == DATA_BUF_SIZ - 1)
                {
		ppkt->bufdata[0] = dcount;
		put_data(pd, 2, 2);
		pd = NULLDATA;
		continue;
                }
	    /* else add the dropped character to the current buflet and
	     * bump the dropped character count
	     */
	    else
		ppkt->bufdata[++dcount] = c;
	    
	    /* bump the read pointer to the circular read buffer and check
	     * for wrap. save the read pointer for possible re-scan in case
	     * header crc's conflict
	     */
            if (++comm_info2.readrecvbuf > comm_info2.endrecvbuf)
                comm_info2.readrecvbuf = comm_info2.begrecvbuf;
            next_pntr = comm_info2.readrecvbuf;
            continue;
            }
	    
	/* if the current buflet is full, allocate and link a new buflet
	 */
        if (inp_idx == DATA_BUF_SIZ)
            {
            if ((ppkt->bufnext = alloc_buf(1)) == NULLBUF)
                return;
            ppkt = ppkt->bufnext;
            inp_idx = 0;
            }
	/* bump the read pointer to the circular read buffer and check
	 * for wrap
	 */
        if (++comm_info2.readrecvbuf > comm_info2.endrecvbuf)
            comm_info2.readrecvbuf = comm_info2.begrecvbuf;
	
	/* save received character
	 */
        ppkt->bufdata[inp_idx++] = c;
	
	/* if entire packet header has been received, save extra data length.
	 * if non-zero, bump to accomodate data crc
	 */
        if (num_chars_inframe == FRAME_LEN)
            if ((inp_len = ppkt->bufdata[FRAME_LEN]))
                inp_len += sizeof(CRC);
	
	/* if header crc has been received, validate
	 */
        if (num_chars_inframe++ == CRC1 + 1)
            {
            crc = get_crc(pd->pbuf, FRAME_LEN, CRC1 - FRAME_LEN);
            if ((ppkt->bufdata[CRC1] != (UBYTE)(crc >> 8))
                || ppkt->bufdata[CRC1 + 1] != (UBYTE)(crc & 0xff))
                {
	
		/* invalid header crc received. flag the error, add the data
		 * entry to the received data queue, reset data pointer and 
		 * STX received flag, and set the read pointer to the circular
		 * read buffer back to where the last STX was read
		 */
                pd->pbuf->bufdata[0] = MON_CRC1_ERR;
                put_data(pd, 2, 1);
		pd = NULLDATA;
                in_a_frame = NO;
                comm_info2.readrecvbuf = next_pntr;
                continue;
                }
            }
	    
	/* if entire packet received, validate, add to received data queue,
	 * reset data pointer and STX received flag, and continue
	 */
        if (inp_len + EXTRA_DATA <= num_chars_inframe)
	    {
	    validate_pkt(pd->pbuf);
	    put_data(pd, 2, 1);
	    pd = NULLDATA;
	    in_a_frame = NO;
	    }
        }
    }

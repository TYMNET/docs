/************************************************************************
 * valpkt.c - Validate Input Packet
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    valpkt.c contains the source modules which are called to
 *    validate the input packet header.  Valid control packets
 *    are passed onto restart_state().  Valid data packets
 *    are passed onto data_pkt().  If the input packet is
 *    invalid reject_pkt() is called to output a REJECT packet. 
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KS    Initial Draft
 *
 ************************************************************************/
#include "stddef.h"
#include "pkt.h"
#include "timer.h"
#include "link.h"
#include "state.h"
#include "xpc.h"
#include "error.h"
#include "iocomm.h"
 
IMPORT BOOL valid_crc(); 		/* valid crc */
IMPORT BUFLET *alloc_buf();		/* allocate buflet chain */
IMPORT CRC get_crc();			/* get crc for data */
IMPORT INT valid_recvseq();		/* valid recv sequence number */
IMPORT VOID data_pkt();			/* process data packet */
IMPORT VOID free_buf();			/* discard packet */
IMPORT VOID init_window();		/* initialize window */
IMPORT VOID restart_state();            /* process packet */
IMPORT VOID reject_pkt();		/* reject packet */
IMPORT VOID send_link_cpkt();		/* build and link control packet
					 * to link output queue.
					 */
IMPORT VOID start_timer();		/* start the timer */
IMPORT VOID stop_timer();		/* stop timer */
IMPORT VOID tim_t27();			/* timeout routine for reject
					 * response timer.
					 */

IMPORT COMMINFO comm_info;		/* communcations information
					 * structure.
					 */
IMPORT LCIINFO lcis[];			/* logical channel information
					 * structure.
					 */
IMPORT UBYTE linkchnl;			/* link channel number */
IMPORT UBYTE pkttype;			/* type of packet */
IMPORT WORD len_mult;			/* multiplier for T25 and T27
					 * timers.
					 */
IMPORT UWORD free_count;		/* Number of free buflets */
#ifdef DEBUG
IMPORT WORD diags[];			/* XXX */
#endif

/************************************************************************
 * VOID validate_pkt(ppkt)
 *    BUFLET *ppkt;	- pointer to incoming link packet
 *
 *    validate_pkt() validates the incoming packet.  If the packet
 *    is not valid, reject_pkt() is called to output a REJECT packet.
 *    Valid control packets are passed onto restart_state().  Valid
 *    data packet are passed to data_pkt().
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID validate_pkt(ppkt)
    BUFLET *ppkt;			/* pointer to packet */
    {
    BUFLET *pend;                       /* pointer to buflet end */
    CRC crc;				/* crc for extra data */
    INT ret;				/* return value */
    UBYTE diag33 = DIAG33;		/* diagnostic 33 */
    UBYTE diag36 = DIAG36;		/* diagnostic 36 */
    UBYTE diag40 = DIAG40;		/* m or d bit set */
    UBYTE len;				/* length of packet */
    UWORD idx;				/* index for crc for extra data*/

    
    /* Extract channel number and the type from the packet.
     */
    linkchnl = ppkt->bufdata[GFI_LCI] & EXTRACT_CHNL;
    pkttype = ppkt->bufdata[PKT_TYP_ID];

    
    /* If the input packet is a data packet and it's channel is
     * in a flow controlled state, then the packet is discarded
     * and an RNR is outputted.
     */ 
    if ((ppkt->bufdata[GFI_LCI] & CBIT) == 0)
	{
	
	/* If channel cannot accept any more input packets, send
	 * an RNR packet and set the state to dte not ready for input.
	 */
	if (free_count <= BUFLET_LOWATER)
	    {
	    send_link_cpkt(ppkt, RNR, (UBYTE *)0, (UBYTE *)0, 0);
	    lcis[linkchnl].dteflowstate = G2; 
	    return;
	    }
	}
    
    /* Initialize window when packet is a restart or a reset.
     */
    else if (pkttype == RESTART || pkttype == RESET)
	init_window(&lcis[linkchnl]);
    
    /* If the packet is not a flow control packet then
     * validate the incoming packet data CRC, and 
     * the input P(S).
     */
    if (((ppkt->bufdata[GFI_LCI] & CBIT) == 0) || 
	(pkttype != RR && pkttype != RNR && pkttype != REJECT))
	{    

	/* validate P(S) in received packet
	 */
	if ((ret = valid_recvseq(&lcis[linkchnl], 
	    ppkt->bufdata[SEQ_NUM] & EXTRACT_SENDSEQ)) != SUCCESS)
	    {
	    
	    /* If the incoming packet P(S) is equal to the last packets
	     * P(S), then discard the packet.  Duplicate packet was probably
	     * sent because of a timeout.
	     */
	    if (ret == PS_DUPLICATE)
		send_link_cpkt(ppkt, RR, (UBYTE *)0,  (UBYTE *)0, 0);

	    /* If the packet is invalid, and the previous packet was
	     * rejected, the packet is freed.  Otherwise, a packet
	     * reject is sent.
	     */
	    else if (lcis[linkchnl].pktrejected)
		free_buf(ppkt);
	    else
		reject_pkt(ppkt);
	    return;
	    }

	
	/* If there is extra data in packet, validate crc for
	 * the extra data.
	 */
	if (ppkt->bufdata[FRAME_LEN] > 0)
	    {

	    crc = get_crc(ppkt, EXTRA_DATA, (len = ppkt->bufdata[FRAME_LEN]));
	    
	    /* Search for position of crc in the packet. pend points to
	     * last packet, and idx is the position of the crc.
	     */
	    for (pend = ppkt, idx = (len + EXTRA_DATA); pend &&
	        idx >= DATA_BUF_SIZ; idx -= DATA_BUF_SIZ, pend = pend->bufnext)
		;
	    
	    if (pend->bufdata[idx] != (UBYTE)(crc >> 8))
		{
		++comm_info.linkstats[NBR_CRC_ERRS];
		reject_pkt(ppkt);
		return;
		}
	    if (++idx == DATA_BUF_SIZ)
		{
		pend = pend->bufnext;
		idx = 0;
		}
		
	    if (pend->bufdata[idx] != (UBYTE)(crc & 0xff))
		{
		++comm_info.linkstats[NBR_CRC_ERRS];
		reject_pkt(ppkt);
		return;
		}
	    }
	
	
	/* The packet is  valid. If the last input packet was rejected, 
	 * clear the packet rejected flag.
	 */
	if (lcis[linkchnl].pktrejected)
	    {
	    lcis[linkchnl].pktrejected = NO;
	    stop_timer(ONE_SEC_TIMER, TIM_T27, linkchnl); 
	    }
	}

    /* If the current link state is ok (restart state is R1 and
     * reset state is D1) data packets are passed to data_packet.
     * The M and D bits should not be set in data packets.  Data packets
     * are only valid for channels 1 thru 15 when the restart
     * state is packet level ready (R1) and when the reset state
     * is flow control ready (D1).
     */
    if ((ppkt->bufdata[GFI_LCI] & CBIT) == 0)
	{
	
	if ((ppkt->bufdata[GFI_LCI] & (MBIT | DBIT)) != 0)
	    send_link_cpkt(ppkt, DIAG, &diag40,  (UBYTE *)0, 0);
	else if (linkchnl == 0)
	    send_link_cpkt(ppkt, DIAG, &diag36,  (UBYTE *)0, 0);
	else 
	    {
	    if (lcis[0].restartstate == R1)
		{
	
		/* Validate QBIT bit data packet function codes.
		 */
		if (ppkt->bufdata[GFI_LCI] & QBIT)
		    if (pkttype < ENABLE_ECHO || pkttype > MAX_PKT_FUNC)
			{
			send_link_cpkt(ppkt, DIAG, &diag33,  (UBYTE *)0, 0);
			return;
			}
		
		/* Input packet is not a control packet.  If the
		 * reset state is valid, then process data packet.
		 */
		if (lcis[linkchnl].resetstate == D1)
		    {
		    data_pkt(&lcis[linkchnl], ppkt);
		    return;
		    }
		}
	    free_buf(ppkt);
	    }
	}
    
    /* If the input packet is not a control packet then send a
     * diagnostic packet.
     */
    else if ((ppkt->bufdata[GFI_LCI] & 0xf0) != CBIT)
	send_link_cpkt(ppkt, DIAG, &diag40,  (UBYTE *)0, 0);

    /* The packet is valid.  Process packet depending on the current
     * state of the link.
     */
    else
	restart_state(&lcis[linkchnl], ppkt);
    }

/************************************************************************
 * VOID reject_pkt(ppkt)
 *    BUFLET *ppkt;	- buflet pointer for packet being built
 *
 *    reject_pkt() builds a reject control packet to be sent to
 *    the host.  The packet rejected flag is set in the logical
 *    channel information structure.  The packet rejected flag is
 *    cleared as soon as a valid packet is received.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID reject_pkt(ppkt)
    BUFLET *ppkt;			/* pointer to packet */
    {
    
    /* Send a reject packet to the host and update channel status.
     */
    send_link_cpkt(ppkt, REJECT, (UBYTE *)0, (UBYTE *)0, 0);
    lcis[linkchnl].pktrejected = YES;
    lcis[linkchnl].r27trans = INIT_R27_COUNT;
    start_timer(ONE_SEC_TIMER, TIM_T27, linkchnl, (len_mult * LEN_T27),
	tim_t27);
    }




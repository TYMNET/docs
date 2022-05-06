/************************************************************************
 * monvpkt.c - X.PC line monitor packet validation routines
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    This module contains the routines used by the X.PC line monitor to
 *    validate incoming packets. Invalid packets are flagged by installing
 *    a code other than STX in the first buflet data position.
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
#include "link.h"
#include "mon.h"
#include "pkt.h"
#include "state.h"
 
IMPORT CRC get_crc();			/* get crc for data */

/************************************************************************
 * VOID validate_pkt(ppkt)
 *    BUFLET *ppkt;		pointer to incoming link packet
 *
 *    validate_pkt validates packets received by the X.PC line monitor.
 *    Invalid packets are flagged by installing a code other than STX
 *    in the first buflet data position.
 *
 * Notes: No sequence checks are made.
 *
 * Returns:  Nothing.
 *
 ************************************************************************/
VOID validate_pkt(ppkt)
    BUFLET *ppkt;			/* pointer to packet */
    {
    BUFLET *pend;                       /* pointer to buflet end */
    CRC crc;				/* crc for extra data */
    UWORD idx;				/* scratch index */
    UBYTE len;				/* length of packet */
    UBYTE linkchnl;			/* link channel number */
    UBYTE pkttype;			/* type of packet */
    
    /* extract channel number and packet type
     */
    linkchnl = ppkt->bufdata[GFI_LCI] & EXTRACT_CHNL;
    pkttype = ppkt->bufdata[PKT_TYP_ID];
    
    /* If the packet is not a flow control packet then
     * validate the incoming packet data CRC
     */
    if (((ppkt->bufdata[GFI_LCI] & CBIT) == 0) || 
	(pkttype != RR && pkttype != RNR && pkttype != REJECT))
	
	/* if there is extra data in packet, validate crc for
	 * the extra data.
	 */
	if (ppkt->bufdata[FRAME_LEN] > 0)
	    {

	    crc = get_crc(ppkt, EXTRA_DATA, (len = ppkt->bufdata[FRAME_LEN]));
	    for (pend = ppkt, idx = (len + EXTRA_DATA); pend &&
	        idx >= DATA_BUF_SIZ; idx -= DATA_BUF_SIZ, pend = pend->bufnext)
		;
	    if (pend->bufdata[idx] != (UBYTE)(crc >> 8))
		{
		ppkt->bufdata[STX] = MON_CRC2_ERR;
		return;
		}
	    if (++idx == DATA_BUF_SIZ)
		{
		pend = pend->bufnext;
		idx = 0;
		}
	    if (pend->bufdata[idx] != (UBYTE)(crc & 0xff))
		{
		ppkt->bufdata[STX] = MON_CRC2_ERR;
		return;
		}
	    }

    if ((ppkt->bufdata[GFI_LCI] & CBIT) == 0)
	if ((ppkt->bufdata[GFI_LCI] & (MBIT | DBIT)) != 0)
	    ppkt->bufdata[STX] = MON_GFI_ERR;
	else if (linkchnl == 0)
	    ppkt->bufdata[STX] = MON_CHNL_ERR;
	else 
	    
	    /* Validate QBIT bit data packet function codes.
	     */
	    if (ppkt->bufdata[GFI_LCI] & QBIT)
		if (pkttype < ENABLE_ECHO || pkttype > MAX_PKT_FUNC)
		    {
		    ppkt->bufdata[STX] = MON_QBIT_ERR;
		    return;
		    }
    }

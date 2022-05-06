/************************************************************************
 * queflush.c - Flush All of the Queues
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    queflush.c contains the source modules flush_all_queues()
 *    which is called to flush all queues during a clear device
 *    or reset device.
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
#include "state.h"
#include "device.h"
#include "timer.h"

IMPORT BUFLET *get_queue();                     /* get first packet in queue */
IMPORT VOID free_buf();                         /* free buflet chain */

IMPORT CHNLINFO cis[];			        /* channel information array */
IMPORT LCIINFO lcis[];				/* logical channel information
						 * structure.
					         */
/************************************************************************
 * VOID flush_all_queues()
 *
 *    flush_all_queues() is called during a device reset or device clear
 *    to flush all of the  queues.  This includes application queues
 *    pad queues and link queues.  It is all right to flush the link
 *    queues because communications interrupts are turned off when
 *    flush_all_queues is called.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID flush_all_queues()
    {
    UBYTE chnl;				/* channel number */
    BUFLET *ppkt;			/* pointer to packet */
    
    for (chnl = 0; chnl <= MAX_CHNL; ++chnl)
	{
	
	/* Clear the application read queue, the number of bytes in the
	 * application read queue and the current read position in the
	 * application read queue.
	 */
	while((ppkt = get_queue(&cis[chnl].readqueue)) != NULLBUF)
	    free_buf(ppkt);
	cis[chnl].nreadbytes = 0;
	cis[chnl].idxreadqueue = MOVED_FIRST_BYTE;
	
	/* Clear the pointer to the echo packet and index into the
	 * echo packet.  The echo packet is part of the application
	 * read queue.  Therefore, it's buflet does not have to be
	 * freed.
	 */
	cis[chnl].echopkt = 0;
	cis[chnl].idxechodata = MOVED_FIRST_BYTE;

	/* Clear pointer to session data.
	 */
	if (cis[chnl].ssndatapkt)
	    free_buf(cis[chnl].ssndatapkt);
	cis[chnl].idxssndata = EXTRA_DATA;
	cis[chnl].ssndatapkt = NULLBUF;

	/* Clear the transmit assembly packet and clear the number of
	 * bytes in the transmit assembly packet.
	 */
	if (cis[chnl].assemblypkt)
	    free_buf(cis[chnl].assemblypkt);
	cis[chnl].nwritebytes = 0;
	cis[chnl].assemblypkt = NULLBUF;

	/* Clear the pad input queue.
	 */
	while((ppkt = get_queue(&lcis[chnl].inpktqueue)) != NULLBUF)
	    free_buf(ppkt);
	
	/* Clear the pad output queue.
	 */
	while((ppkt = get_queue(&lcis[chnl].outpktqueue)) != NULLBUF)
	    free_buf(ppkt);
	
	/* Clear the link output queue.
	 */
	while((ppkt = get_queue(&lcis[chnl].linkoutqueue)) != NULLBUF)
	    free_buf(ppkt);
	
	/* Clear the waiting to be acknowledged queue.
	 */
	while((ppkt = get_queue(&lcis[chnl].waitackqueue)) != NULLBUF)
	    free_buf(ppkt);
	}
    }

	



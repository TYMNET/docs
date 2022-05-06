/************************************************************************
 * okfree.c - Check If All Right To Free Packet
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    oktfree.c contains the source module ok_to_free().  ok_to_free()
 *    is called when a packet is being freed.  It ensures that the
 *    packet being freed is not currently being output by iocomm.
 *    ok_to_free() is called when packets are being freed from the
 *    waiting to be acknowledged queue or the pad output queue (during
 *    an output packet flush).
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
#include "link.h"

    
IMPORT VOID int_disable();		/* diable interrupts */
IMPORT VOID int_enable();		/* enable interrupts */

IMPORT COMMINFO comm_info;		/* communications information
					 * structure.
					 */

/************************************************************************
 * BOOL ok_to_free(ppkt)
 *    BUFLET *ppkt;	- pointer to incoming link packet
 *
 *    ok_to_free ensures that the packet pointed to be ppkt is
 *    not currently being output or scheduled to be output by iocomm.
 *    ok_to_free is called when packets in the link output queue
 *    or the waiting to be acknowledged queue are being freed.
 *    If the packet pointed to by ppkt is scheduled to be output,
 *    then NO is returned, and the iocomm flag is set to YES.  This
 *    ensures that the buflet will not be freed until it is outputted.
 *    When the packet has been transmitted, the length is set to
 *    zero and since the flag has been set, the buflet is freed. 
 *
 * Returns:  If it is all right to free the buflet (the buflet is
 *    not scheduled for output by iocomm), ok_to_free returns YES.
 *    Otherwise, ok_to_free returns NO.
 *
 ************************************************************************/
BOOL ok_to_free(ppkt)
    BUFLET *ppkt;			/* Pointer to packet which is to
					 * be freed.
					 */
    {
    UWORD x;
    

    x = XMT_BUF_CHAINS;
    int_disable();
	
    /* Search thru table which is used by iocommm.  If the packet is
     * currently being outputted by iocomm or is scheduled for output
     * ok_to_free returns NO.
     */
    while (x--)
	{
	if (comm_info.xmtlen[x])
	    if (comm_info.xmtptr[x] == ppkt)
		{
		comm_info.xmtflg[x] = YES;
		int_enable();
		return(NO);
		}
	}
    int_enable();		/* enable interrupts */

    /* The packet is not scheduled for output.
     */
    return(YES);
    }


    



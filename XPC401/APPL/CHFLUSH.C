/************************************************************************
 * ch_flush.c - Flush Queues When Device In Character State
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    ch_flush.c contains the modules flush_char_input and flush_char_output.
 *    These routines are called to flush the queues when the device is
 *    in character state.
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
#include "device.h"
#include "error.h"

IMPORT BUFLET *get_queue();             /* get first packet in queue */
IMPORT VOID free_buf();                 /* free buflet chain */
IMPORT VOID int_disable();		/* disable interrupts */
IMPORT VOID int_enable();		/* enable interrupts */
 
IMPORT CHNLINFO *ap_pcis;		/* current pointer to channel
					 * information structure.
					 */
IMPORT DEVSTAT dev_stat;		/* device status structure */

/************************************************************************
 * INT flush_char_input()
 *
 *    flush_char_input() is called to flush the input buffers and queues
 *    when the device is in a character state.  
 *
 * Usage notes: flush_char_input  assumes that ap_pcis is pointing to
 *    channel 0's channel information structure and that
 *    the channel information structure is linked to the logical
 *    channel information structure.
 *
 * Returns:  If no port is active, flush_char_input returns 
 *    ILLEGAL_PORT.  Otherwise, SUCCESS is returned.
 *
 ************************************************************************/
INT flush_char_input()
    {
    BUFLET *pb;                         /* pointer to buflet in queue */
    
    /* Return error is the port is not initialized.
     */ 
    if (dev_stat.portaddr == 0)
	return(ILLEGAL_PORT);
    
    /* Flush all packets in the applications read queue.
     */
    
    while((pb = get_queue(&ap_pcis->readqueue)) != NULLBUF)
        free_buf(pb);

    /* If by chance something was added to the read queue, 
     * disable interrupts remove rest of packets from queue and
     * clear the number of bytes in the application read queue
     * and the index into the current application read queue
     * buflet.
     */
    int_disable();
    if (ap_pcis->readqueue.begqueue)
	while((pb = get_queue(&ap_pcis->readqueue)) != NULLBUF)
	    free_buf(pb);
    ap_pcis->nreadbytes = 0;
    ap_pcis->idxreadqueue = 0;
    int_enable();
    
    /* Flush packets for the link input queue of the specified
     * application channel.
     */
    while ((pb = get_queue(&ap_pcis->lcistruct->inpktqueue)) != NULLBUF)
        free_buf(pb);
    return(SUCCESS);
    }
    
/************************************************************************
 * INT flush_char_output()
 *
 *    flush_char_output() is called to flush the output buffers and queues 
 *    when the device is in a character state.  
 *
 * Usage notes: flush_char_output assumes that ap_pcis is pointing to
 *    channel 0's channel information structure and that
 *    the channel information structure is linked to the logical
 *    channel information structure.
 *
 * Returns:  If the port has not been initialized, flush_char_output() 
 *    returns ILLEGAL_PORT. Otherwise, SUCCESS is returned.
 *
 ************************************************************************/
INT flush_char_output()
    {
    BUFLET *pb;            		/* pointer to buflet in queue */

    /* If the port is not initialized, return an error.
     */
    if (dev_stat.portaddr == 0)
	return(ILLEGAL_PORT);

    /* Flush data in the assembly packet and the output queue.
     */
    if (ap_pcis->assemblypkt)
        {
	
	/* Clear the number of bytes in the transmit assembly packet.
	 */
        ap_pcis->nwritebytes = 0;
	
	/* Clear the transmit assembly packet.
	 */
        free_buf(ap_pcis->assemblypkt);
        ap_pcis->assemblypkt = NULLBUF;
        }
    
    /* Flush all of the data from the pad output queue.
     */
    while ((pb = get_queue(&ap_pcis->lcistruct->outpktqueue)) != NULLBUF)
        free_buf(pb);
    return(SUCCESS);
    }

	
	
    


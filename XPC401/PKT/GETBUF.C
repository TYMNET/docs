/************************************************************************
 * getbuf.c  - Get Buflet Chains for Output of Data
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    getbuf.c contains the modules which are called to allocate buflets
 *    when building output data packets.  get_xmit_pkt gets buflet 
 *    for the assembly transmit packet.  get_echo_pkt gets
 *    buflets for echo packets which are linked to the end of the
 *    read queue.  getebufs allocates buflets when character or
 *    line deletion is being performed.
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

					    /* calc_num_buflets() is a macro which 
					     * is defined in pkt.h.
					     */
IMPORT BUFLET *alloc_buf();		    /* allocate buflet */
IMPORT VOID add_queue();		    /* add packet to queue */
 
IMPORT BUFLET *pecho;			    /* current pointer to echo buflet */
IMPORT BUFLET *ptrans;			    /* current pointer to transfer
					     * buffer
					     */
IMPORT UWORD trans_idx;			    /* index into transfer buffer */
IMPORT INT numecho;			    /* number of bytes in
					     * echo buflet.
					     */
#ifdef DEBUG
IMPORT WORD diags[];
#endif
        
/************************************************************************
 * INT get_xmit_pkt(pcis)
 *     FAST CHNLINFO *pcis - pointer to channel information structure	
 *
 *    get_xmit_pkt() gets the maximum number of buflets which might
 *    be needed for the transmit assembly packet.  
 *
 * Returns:  If there are no buflets available, get_xmit_pkt returns
 *    STOP_WRITE.  Otherwise, SUCCESS is returned.
 *    buflet chain or null.
 *
 ************************************************************************/
INT get_xmit_pkt(pcis)
    FAST CHNLINFO *pcis;		/* pointer to channel information
    					 * structure.
    					 */
    {
    BYTES num;
    
    /* get_xmit_pkt attempts to allocate the maxmum number of buflets
     * for the assembly transmit packet.  If buflets are not available
     * an error is returned.  Otherwise, the assembly packet pointer
     * and index are initialized and success is returned.
     */
    num = calc_num_buflets(MAX_DATA_PKT);

    /* Initialize the pointer to the transmit assembly packet, the number
     * of bytes in the transmit assembly packet and the index into the
     * transmit assembly packet.
     */
    if ((ptrans = alloc_buf(num)) != NULLBUF)
	{
    	pcis->assemblypkt = ptrans;
	trans_idx =  MOVED_FIRST_BYTE;
    	pcis->nwritebytes = 0;
    	}
    else
	return(STOP_WRITE);
    return (SUCCESS);
    }


    
/************************************************************************
 * INT get_echo_buf(pcis)
 *     FAST CHNLINFO *pcis - pointer to channel information structure	
 *
 *    get_echo_buf() gets a buflet for echo data.  The pointer to the
 *    echo buflet is updated, the echo buflet index is reset and
 *    the number of characters in the echo buflet is cleared.
 *
 * Returns:  If there are no buflets available, get_echo_buf 
 *    returns STOP_WRITE.  Otherwise, get_echo_buf
 *    returns SUCCESS.
 *
 ************************************************************************/
INT get_echo_buf(pcis)
    FAST CHNLINFO *pcis;		/* pointer to channel information
    					 * structure.
    					 */
    {

    /* If currently there is an echo buflet, store the number of
     * characters in the echo buflet before adding the next 
     * echo buflet to the queue.
     */
    if (pcis->echopkt)
        pcis->echopkt->bufdata[FRAME_LEN] = (UBYTE)numecho;
	
    /* get_echo_buf attempts to allocate a buflet for echo data.  If
     * no buflet is available, an error code is set.  Otherwise, the
     * buflet is added to the end of the application read queue.  
     */
    if ((pcis->echopkt = pecho = alloc_buf((BYTES)1)) != NULLBUF)
	{
    
	pcis->echopkt->bufdata[FRAME_LEN] = 0;
	numecho = -1;
	pcis->echopkt->bufdata[GFI_LCI] = 0;
	add_queue(&pcis->readqueue, pecho);
	pcis->idxechodata = MOVED_FIRST_BYTE;
	}
    else
	return(STOP_WRITE);
    return (SUCCESS);
    }
    
/************************************************************************
 * INT getebufs(pcis, nn)
 *    CHNLINFO *pcis    - pointer to channel information structure	
 *    BYTES n;		- number of characters to be put in buflet
 *
 *    getebufs() allocates enough buflets for n number of bytes of
 *    echo data.  The echo pointer, index and number of bytes are
 *    updated.
 *
 * Returns:  If the number of buflets specified by the caller is
 *    unavailable, getebufs returns STOP_WRITE.  Otherwise,
 *    getebufs returns SUCCESS.  
 *
 ************************************************************************/
INT getebufs(pcis, n)
    FAST CHNLINFO *pcis;		/* pointer to channel information
    					 * structure.
    					 */
    BYTES n;			        /* number of characters to get */
    {
    BUFLET *p, *pp;
    BYTES nn;		  		/* number of buflets to allocate */
    BYTES num = 0;			/* number of bytes available in 
				 	 * assembly packet.
				 	 */
 
    /* If there is enough room in the current echo buflet, return.
     * Otherwise, allocate enough buflets for n number of data bytes 
     * and set up the echo buffer pointers.  Return an error if
     * the buffer allocation failed.
     */
    if (pcis->echopkt)
	{
	pp = pecho;
	for (num = (DATA_BUF_SIZ - pcis->idxechodata + 1), 
	    p = pecho->bufnext; p; p = p->bufnext, num += DATA_BUF_SIZ)
		pp = p;
	
	}

    /* If the number of bytes to be allocated is greater than the number
     * left in the echo buflet chain, allocate additional buflets.
     */
    if (n > num)
	{
	
	/* Calculate the number of buflets needed.
	 */
	nn = calc_num_buflets(n - num);
	
	/* If there are not enough buflets available, return an error.
	 */
	if ((p = alloc_buf(nn)) == NULLBUF)
	    return (STOP_WRITE);

	/* Link packet to the end of the echo buflet.
	 */
	if (pcis->echopkt)
	    pp->bufnext = p;
	else
	    {
	    pcis->echopkt = pecho = p;
	    pcis->idxechodata = MOVED_FIRST_BYTE;
	    }
	}
    return (SUCCESS);
    }

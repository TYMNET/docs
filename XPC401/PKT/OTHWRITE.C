/************************************************************************
 * oth_write.c - Output Data When No Echo Is Active
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    oth_write.c contains the module noecho_write which is called
 *    to copy data from a packet to the transmit assembly buffer.
 *    No echoing is active.   noecho_write returns when the specified
 *    number of characters have been output or  no more buffers are 
 *    available for data.  noecho_write also returns if the
 *    transmit buffer becomes full and link_xmit is unable to
 *    link it to the output queue.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KS    Initial Draft
 * 06/19/87   4.01    KS    Allow forwarding character for non-echo data.
 *
 ************************************************************************/
#include "stddef.h"
#include "xpc.h"
#include "appl.h"
#include "pkt.h"
#include "error.h"
 
IMPORT BYTES cpybuf();			/* copy buffer to buffer */
IMPORT INT get_xmit_pkt();		/* get transmit assembly packet */
IMPORT INT link_xmit_pkt();		/* link the current transmit assembly
					 * packet.
					 */
IMPORT VOID free_buf();			/* free buflet chain */
IMPORT VOID mov_param();		/* move data to/from parameter
					 * packet 
				         */
IMPORT VOID send_char();		/* move character to transmit
					 * assembly packet.
					 */

IMPORT BUFLET *ptrans;			/* current pointer to transmit
					 * buflet.
					 */
IMPORT CHNLINFO *ap_pcis;		/* pointer to channel information
					 * structure.
					 */    
IMPORT INT write_error;			/* write error */
IMPORT UWORD numreq;			/* number of output bytes requested */
IMPORT UWORD numprc;			/* number of bytes outputted */
IMPORT UWORD trans_idx;			/* index into transmit buflet */
#ifdef DEBUG
IMPORT WORD diags[];			/* debugging aid */
#endif

/************************************************************************
 * VOID noecho_write(p)
 *    BYTES *p;				 pointer to input buffer 
 *					 if data has already been
 *					 moved from application 
 *					 parameter buffer.
 *					
 *
 *    noecho_write() moves nchar number of characters from the 
 *    application buffer into the transmit assembly buffer. 
 *
 * Usage notes: tym_write() assumes that the pointer to the
 *    current transmit assembly packet buflet, ptran, and the index into 
 *    the  current transmit assembly packet buflet, trans_idx,  have 
 *    been set up  by p_write_data().
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID noecho_write(p)
    UBYTE *p;				/* pointer to buffer when data
     					 * has already been moved from
    					 * application buffer.
    					 */
    {

/* Variables which were previously used, when forwarding character
 * timeout not valid in no echo mode.

    UWORD n = 0; 			xx counter xx
    UWORD nleft;			xx number left to move xx
 
 */    

    for (write_error = SUCCESS; write_error != STOP_WRITE &&
        numprc < numreq; )
	send_char(*(p + numprc));

/* The following code was replaced with a call to send_char,
 * which will allow for forwarding characters when echo is
 * not on.  6/19/87
 */


/*    
    * Move data from the application parameter buffer into the
     * transmit assembly packet.  When the transmit assembly packet
     * is full, link it to the pad output queue.  If unable to get
     * a buflet or unable to link assembly packet to the
     * output queue, return the number of bytes moved from the
     * application parameter buffer.
     *
    for (nleft = (numreq - numprc); nleft; )
	{
	
	* Set the number of characters to write equal to either
	 * the number of characters left to write or the number
	 * of characters which can fit in the current buflet.
	 *
	n = (DATA_BUF_SIZ - trans_idx) > nleft ? nleft 
	    : (DATA_BUF_SIZ - trans_idx);
	
	* Adjust the number of characters to write to ensure that
	 * the maximum number of output bytes is not exceeded.
	 *
        n = (n > (MAX_DATA_PKT - ap_pcis->nwritebytes)) ? 
            (MAX_DATA_PKT - ap_pcis->nwritebytes) : n;
	
	* If the characters have not been moved to a temporary buffer
	 * by tym_write(), then call mov_param() to move data from 
	 * application parameter buffer to transmit assembly packet.
	 * Otherwise, copy bytes from temporary buffer to transmit
	 * assembly buflet.
	 *
	if (p == 0)
	    mov_param((UBYTE *)&ptrans->bufdata[trans_idx], (INT)n, 
		PARAM_1, (INT)numprc, FROM_APPL); 
	else
	    (VOID)cpybuf((UBYTE *)&ptrans->bufdata[trans_idx], 
	        (UBYTE *)(p + numprc), (BYTES)n);
	
	* Decrement the number of characters left to write and increment
	 * the number of characters in the transmit assembly packet.
	 * 
	nleft -= n;
	numprc += n;
	ap_pcis->nwritebytes += n;
        
	* There are more characters to be written.  If the assembly
	 * packet is full, try to add it to the pad output queue and
	 * get another packet.  Otherwise, increment the buffer index
	 * and if necessary, go to next transmit assembly buflet.
	 *
	if (nleft)
	    {
	    if (ap_pcis->nwritebytes == MAX_DATA_PKT)
		{
		if ((write_error = link_xmit_pkt(ap_pcis, NO)) == SUCCESS)
		    if ((write_error = get_xmit_pkt(ap_pcis)) == SUCCESS)
			continue;
		return;
		}
	    trans_idx += n;
	    if (trans_idx == DATA_BUF_SIZ)
		{
		ptrans = ptrans->bufnext;
		trans_idx = 0;
		}
	    }
	}
 */

    }


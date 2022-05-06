/************************************************************************
 * mciwrite.c - Write Characters With MCI Echoing
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    mciwrite.c contains the source module mci_write.   mci_write
 *    is called to output data when MCI echoing has been enabled.
 *    If MCI editing has been enabled (mci echo state is M2), then
 *    mci_write checks for backspace character, backspace line and
 *    redisplay line pad parameter values.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KS    Initial Draft
 * 06/19/87   4.01    KS    Move data from application buffer in pktwrite,
 *                          to allow forwarding character for non-echo 
 *                          write.
 ************************************************************************/
#include "stddef.h"
#include "xpc.h"
#include "appl.h"
#include "pkt.h"
#include "state.h"
#include "error.h"
#include "param.h"
#include "padprm.h"

IMPORT BUFLET *remove_queue();		/* remove buflet chain from queue */
IMPORT VOID backspace();		/* backspace character */
IMPORT VOID echo_char();		/* echo character */
IMPORT VOID echo_cr();			/* echo carriage return */
IMPORT VOID free_buf();			/* free buflet chain */
IMPORT VOID mov_param();		/* move datra from/to application
					 * parameter buffer.
					 */
IMPORT VOID redisplay();		/* redisplay line */
IMPORT VOID send_char();		/* send character */

IMPORT CHNLINFO *ap_pcis;		/* pointer to channel information
					 * structure.
					 */
IMPORT INT numecho;			/* number of bytes in echo
					 * buflet chain.
					 */
IMPORT INT write_error;			/* packet write error code 
					 * set in send_char and echo_char
					 */
IMPORT UWORD numprc;			/* number of characters processed */
IMPORT UWORD numreq;			/* number of characters requested */
#ifdef DEBUG
IMPORT WORD diags[];			/* used for diagnostic purposes */
#endif

/************************************************************************
 * VOID mci_write(pbuf)			pointer to output buffer
 *
 *    mci_write() moves characters from the application parameter
 *    buffer into the transmit assembly packet when MCI echoing
 *    is active.  If MCI editing has been enabled and the current
 *    character is a backspace character, backspace line character
 *    or redisplay line charater, subroutines are called to perform
 *    special processing.  Otherwise, the character is added to the
 *    output queue and the current echo buflet.
 *
 * Usage notes: mci_write() assumes that the pointer to the
 *    current assembly packet buflet, ptran, the index into the
 *    current assembly packet buflet, trans_idx, the pointer
 *    to the current echo packet buflet, pecho, and the number
 *    of characters in the echo buflet chain have been set up
 *    by p_write_data().
 *       A transmit assembly packet which is large enough to
 *    to contain all of the output characters has already been
 *    allocated along with an echo packet.
 *
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID mci_write(pbuf)
    UBYTE *pbuf;			/* pointer to buffer which
					 * contains mci data.
					 */
    {
    FAST UBYTE c;
    
    /* move data from the application parameter buffer into the
     * assembly transmit buffer.  When the transmit buffer is
     * full, move data to the output queue.   Processing stops
     * when no buffer is available or the output queue is full.
     * Processing does not start if there is no room in the
     * read queue for echo data.
     */

    write_error = SUCCESS;
    for (numprc = 0; write_error != STOP_WRITE && numprc < numreq; )
	{
	
	c = *(pbuf + numprc);
	
	/* The output character is a carriage return.
	 */
	if (c == CR)
	    {
	    send_char(c);
	    echo_cr();
	    }
	    
	/* If MCI editing is not active, then send and echo any character
	 * but the carriage return.
	 */
	else if (ap_pcis->mciechostate == M1)	/* MCI echo on */
	    {
	    send_char(c);
	    echo_char(c);
	    }
	
	/* If MCI edit is on and the character is
	 * delete character, then backspace one character 
	 */
	else if (c == ap_pcis->padparams[EDIT_DEL_CHAR])
	    backspace(YES);
	
	/* if MCI edit is on and the input character is delete line, then
	 * delete all the characters in the transmit assembly buffer.
	 */
	else if (c == ap_pcis->padparams[EDIT_DEL_LINE])
	    backspace(NO);
	
	/* if MCI edit is on and the input character is redisplay line, then
	 * redisplay all of the characters in the transmit assembly buffer.
	 */
	else if (c == ap_pcis->padparams[EDIT_DISP_LINE])
	    redisplay();
	
	/* Output the current character with no editing. 
	 */
	else
	    {
	    send_char(c);
	    echo_char(c);
	    }
	}
	
    /* If there are no characters in the echo buffer, then
     * free the echo buffer.
     */
    if (ap_pcis->echopkt)
	if (numecho < 0)
	    {
	    (VOID)remove_queue(&ap_pcis->readqueue, ap_pcis->echopkt);
	    free_buf(ap_pcis->echopkt);
	    ap_pcis->echopkt = NULLBUF;
	    ap_pcis->idxechodata = MOVED_FIRST_BYTE;
	    }
    
	/* Store the number of echo characters in the echo buffer.  This is
	 * done in case the echo buffer is terminated the addition of another
	 * packet to the application read queue.
	 */
	else
	    ap_pcis->echopkt->bufdata[FRAME_LEN] = (UBYTE)numecho;
    return;
    }


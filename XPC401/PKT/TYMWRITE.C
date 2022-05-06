/************************************************************************
 * tymwrite.c - Write Buffer With Tymnet Echoing 
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    tymwrite.c contains the source module tym_write() which 
 *    is called to move characters into the assembly packet when
 *    tymnet echoing is active.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KS    Initial Draft
 * 06/19/87   4.01    KS    Change made to move output buffer from application
 *                          parameter buffer in pktwrite.  Allows forwarding
 *                          character for non-echo write.
 *
 ************************************************************************/
#include "stddef.h"
#include "xpc.h"
#include "pkt.h"
#include "device.h"
#include "appl.h"
#include "state.h"
#include "param.h"
#include "padprm.h"
#include "error.h"

IMPORT BUFLET *remove_queue();		/* remove packet from queue */
IMPORT VOID echo_char();		/* echo character */
IMPORT VOID echo_cr();			/* echo carriage return */
IMPORT VOID enter_dem();		/* enter deferred echo mode */
IMPORT VOID free_buf();			/* free the buflet chain */
IMPORT VOID mov_param();		/* move data to/from application
				  	 * parameter buffer.
					 */
IMPORT VOID noecho_write();		/* output with no echoing of 
					 * characters.
					 */
IMPORT VOID send_char();		/* send character */

IMPORT BUFLET *ptrans;			/* pointer to current buflet in
					 * transmit assembly packet.
					 */
IMPORT CHNLINFO *ap_pcis;		/* pointer to channel information
					 * structure.
					 */
IMPORT INT numecho;			/* number of bytes in echo
					 * buflet chain.
					 */
IMPORT INT write_error;			/* unable to get a buflet or
					 * the output queue is full.
					 * stop processing.
					 */
IMPORT PORTPARAMS port_params;		/* port parameter structure */
IMPORT UWORD trans_idx;			/* index into assembly transmit
					 * buflet.
					 */
IMPORT UWORD numprc;			/* number characters processed */
IMPORT UWORD numreq;			/* number characters requested */
#ifdef DEBUG
IMPORT WORD diags[];			/* used for debugging. */
#endif

/************************************************************************
 * VOID tym_write(pbuf)
 *    UBYTE *pbuf;			 pointer to output buffer for
 *					 tymnet echo.
 *					 
 *
 *    tym_write moves characters to the output queue when tymnet
 *    echoing is active.  As long as tymnet echoing is active
 *    characters are moved from the application buffer into
 *    the transmission assembly output packet.  The tymnet echoing
 *    is performed according to entries in the pad parameter
 *    table. When local echoing is turned off, the remaining application
 *    buffer characters are moved into the transmission assembly
 *    packet.
 
 *
 * Usage notes: tym_write() assumes that the pointer to the
 *    current assembly packet buflet, ptran, the index into the
 *    current assembly packet buflet, trans_idx, the pointer
 *    to the current echo packet buflet, pecho, and the number
 *    of characters in the echo buflet chain have been set up
 *    by p_write_data().
 *       A transmit assembly packet which is large enough to
 *    to contain all of the output characters has already been
 *    allocated along with an echo packet.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID tym_write(pbuf)
    UBYTE *pbuf;			/* pointer to buffer */
    {
    FAST UBYTE c;			/* current output character */
        
 
    /* Move data to the transmit assembly packet until the
     * all of the requested data has been written or the channel
     * is in a flow control state.
     */
    for (numprc = 0, write_error = SUCCESS; write_error != STOP_WRITE && 
	numprc < numreq; )
	{
    
	/* If local echo is off then move rest of data from the
	 * buffer to the transmit assembly packet.
	 */
	if (ap_pcis->tymechostate != T5)	/* local echo is off */
	    {
	    noecho_write(pbuf);
	    break;
	    }
	else if (port_params.dxemode == DCE_MODE)
	    enter_dem();

	/* Tymnet local echoing is on.
	 */
	else
	    {
	    switch((c = *(pbuf + numprc)))
		{
		
		/* Output character is carriage return.  If pad parameter
                 * convert carriage return to line feed is set, then
                 * echo carriage return and line feed.  There is the
                 * slim possibility that the line feed will not be
                 * echoed if there are no buflets available.
		 */
		case CR: 
		    send_char(c);
		    echo_cr();
		    break;

		/* Output character is line feed. If the pad parameter
                 * convert line feed to line feed, carriage return is
                 * on then a line feed, carriage return and 0x7f is
		 * echoed.  There is a slim possibility that the
		 * carriage return and 0x7f may not be echoed if no
		 * buflet is available.
		 */
		case LF:
		    send_char(c);
		    if (ap_pcis->padparams[ECHO_CR])
			{
			echo_char((UBYTE)LF);
			echo_char((UBYTE)CR);
			echo_char((UBYTE)0x7F);
			}
		    else
			echo_char(c);
		    break;

		/* Output character is tab.  If the pad parameter echo 
		 * tab is set then the tab is echoed.  Otherwise, 
		 * local echoing is disabled.
		 */
		case TAB:
		    if (ap_pcis->padparams[ECHO_TABS])
			{
			send_char(c);
			echo_char(c);
			}
		    else
			enter_dem();
		    break;

		/* Output character is a backspace.  If the pad parameter
		 * echo backspace is turned on, then the backspace is
		 * echoed.  Otherwise, local echoing is deferred.
		 */
		case BACKSPACE:
		    if (ap_pcis->padparams[ECHO_BKSP])
			{
			send_char(c);
			echo_char(c);
			}
		    else
			enter_dem();
		    break;
		
		/* The output character is an escape.  If the pad parameter
		 * echo escape is turned on, then the escape is echoed.
		 * Otherwise, local echoing is deferred.
		 */
		case ESCAPE:
		    if (ap_pcis->padparams[ECHO_ESC])
			{
			send_char(c);
			echo_char(c);
			}
		    else
			enter_dem();
		    break;

		
		/* Send and echo character.
		 */
		default:
		    send_char(c);
		    if (c >= 0x20 && (c != 0x7d && c != 0x7e && c != 0x7f))
			echo_char(c);
		    else
			enter_dem();
		    break;
		}
	    }
	}
    
    /* If echoing was turned off and no characters were put in 
     * the echo buffer, then free the echo buflet chain.
     */
    if (ap_pcis->echopkt && numecho < 0)
	{
	(VOID)remove_queue(&ap_pcis->readqueue, ap_pcis->echopkt);
	free_buf(ap_pcis->echopkt);
	ap_pcis->echopkt = NULLBUF;
	ap_pcis->idxechodata = MOVED_FIRST_BYTE;
	}
    
    /* update the number of characters in the echo buflet.
     */
    else if (numecho >= 0)
	ap_pcis->echopkt->bufdata[FRAME_LEN] = (UBYTE)numecho;
    return;
    }

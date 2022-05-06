/************************************************************************
 * wrtsubs.c - Packet Write Data Subroutines
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    wrtsubs.c contains subroutines which are used to write packet
 *    data.  send_char() is called to move the specified character
 *    into the transmit assembly packet.  echo_char() is called to
 *    echo output characters and enter_dem() sets the echo 
 *    state to deferred echoing.  backspace() is called to backspace
 *    output one character or to delete all characters in the
 *    transmit assembly buffer. redisplay redisplays the contents
 *    of the transmit assembly packet.
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
#include "appl.h"
#include "pkt.h"
#include "padprm.h"
#include "state.h"
#include "timer.h"
#include "error.h"

IMPORT INT getebufs();	                /* get enough buffers for n number of
				         * characters.
				         */
IMPORT INT get_echo_buf();	        /* get buflet for echoing characters */ 
IMPORT INT get_xmit_pkt();		/* get a transmit buflet */  
IMPORT INT link_xmit_pkt();		/* link xmit packet to output
					 * output queue.
					 */
IMPORT INT send_ctrl_pkt();		/* send control packet */
IMPORT VOID tim_noact();		/* function called for NO ACTIVITY
					 * timer.
					 */

IMPORT BUFLET *pecho;		        /* pointer to echo packet */
IMPORT BUFLET *ptrans;			/* pointer to assembly packet */
IMPORT CHNLINFO *ap_pcis;  		/* pointer to channel information
					 * structure.
					 */
IMPORT INT numecho;			/* number of echo bytes */
IMPORT INT write_error;		        /* error occurred
					 * buflet .
					 */
IMPORT UWORD numreq;			/* number of characters requested
					 * to write.
					 */
IMPORT UWORD free_count;
IMPORT UWORD numprc;			/* number of characters processed */
IMPORT UWORD trans_idx;			/* index into assembly packet */
#ifdef DEBUG
IMPORT WORD diags[];			/* used for debugging. */
#endif
    

/************************************************************************
 * VOID send_char(c)
 *    UBYTE c;		- character to be written
 *
 *    send_char() moves the specified character into the transmit 
 *    assembly packet.  When the transmit packet is full or the
 *    forwarding  character is being output, link_xmit_pkt
 *    is called to link the packet to the pad output queue.  write_error
 *    is set when link_xmit_pkt is unable to link the output queu
 *    or get_xmit_pkt() is unable to get a transmit assembly packet.
 *    send_char is called for  MCI and tymnet packet output.
 *
 * Usage notes: write_error is set by send_char but will be checked
 *    by the calling process (tym_write or mci_write).
 *    The output of characters stops when write_error is set
 *    to STOP_WRITE.  write_error is set to STOP_WRITE when
 *    link_xmit_pkt is unable to link the transmit assembly
 *    packet and the transmit assembly packet is full.  If link_xmit_pkt
 *    is unable to link the transmit assembly packet and the transmit
 *    assembly packet is not full, write_error is set to QUEUE_FULL
 *    and processing will continue until to the transmit assembly
 *    packet becomes full.
 *
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID send_char( c)
    UBYTE c;			        /* character to send */
    {
    
    /* Move character to transmit assembly packet.  Add packet to
     * pad output queue when character equals forwarding character 
     * specified by the pad parameters. The first data byte in
     * the transmit assembly packet is MOVED_FIRST_DATA byte.
     * When the packet is added to the pad output queue the
     * first data  byte is moved from MOVED_FIRST_DATA 
     * to FIRST_DATA_BYTE. This makes the building of the transmit 
     * assembly packet must simpler.
     */

    ptrans->bufdata[trans_idx++] = c;

    /* Increment number of characters processed for write and number
     * of characters in transmit assembly packet.
     */
	
    ++numprc;
    ++ap_pcis->nwritebytes;
    
    /* If this is the forwarding character then try to link packet to
     * pad output queue
     */
    if (ap_pcis->padparams[FWD_CHAR] != 0 && 
	ap_pcis->padparams[FWD_CHAR] == c)
	{
    	if ((write_error = link_xmit_pkt(ap_pcis, NO)) == SUCCESS)
	    
	    /* If there are more output characters to process, get the
	     * next transmit assembly packet. 
	     */
	    if (numprc != numreq)
		write_error = get_xmit_pkt(ap_pcis);
	}
    
    /* If the packet is full and there are more data to output,
     * link the transmit assembly packet and get a new one.
     */
    else if (ap_pcis->nwritebytes == MAX_DATA_PKT)
	{
    	if ((write_error = link_xmit_pkt(ap_pcis, NO)) == SUCCESS)
	    if (numprc != numreq)
		write_error = get_xmit_pkt(ap_pcis);
	}
		
    /* If at the end of a buflet, go to next buflet in chain.
     */
    if (trans_idx == DATA_BUF_SIZ && write_error != STOP_WRITE)
        {
    	ptrans = ptrans->bufnext;
	trans_idx = 0;
    	}
    }
 

/************************************************************************
 * VOID echo_char(c)
 *    UBYTE c;		- character to be moved to echo buffer
 *
 *    echo_char moves the specified character to the echo buffer
 *    which is at the end of the application read queue.  If an
 *    error occurs getting the next echo buffer, write_error
 *    is set.  echo_char always has a buffer available before
 *    the character is moved in. write_error is set when echo_char
 *    is unable to get the next echo buflet.  echo_char() is called
 *    for MCI and tymnet echoing. 
 *
 * Usage notes: write_error is set by echo_char but will be checked
 *    by the calling process (tym_write or mci_write).
 *    The output of characters stops when write_error is set
 *    to STOP_WRITE.   write_error is set to STOP_WRITE when 
 *    get_echo_buf() is unable to get an echo buflet.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID echo_char(c)
    UBYTE c;			        /* character to echo */
    {
     
    /* Put character into the echo buffer. If at end of echo
     * buflet chain, get the next echo buflet.
     */
    if (pecho)
	{
	if (ap_pcis->padparams[ENABLE_PARITY])
	    pecho->bufdata[ap_pcis->idxechodata] = (c & 0x7f);
	else
	    pecho->bufdata[ap_pcis->idxechodata] = c;
	++ap_pcis->nreadbytes;
	++numecho;

	/* If this is the end of an echo buflet, then either go to
	 * the next buflet or allocate a another echo packet.
	 */
	if (++ap_pcis->idxechodata == DATA_BUF_SIZ)
	    {
	    if (pecho->bufnext == NULLBUF)
		write_error = get_echo_buf(ap_pcis);

	    /* Allocate the next echo buflet and set up the
	     * index into the echo buflet.
	     */
	    else
		{
		pecho = pecho->bufnext;
		ap_pcis->idxechodata = 0;
		}
	    }
	}
    }

/************************************************************************
 * VOID echo_cr()
 *
 *    echo_cr echos a carriage return. If echo lf pad parameter is set,
 *    then a carriage return and line feed are echoed.  Otherwise,
 *    echo carriage return.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID echo_cr()
    {
 
    /* If echo carriage return, line feed pad parameter is set, then
     * echo carriage return and line feed for carriage return.
     * Otherwise, echo carriage return.
     */ 
    if (ap_pcis->padparams[ECHO_LF])
	{
	echo_char((UBYTE)CR);
	echo_char((UBYTE)LF);
	}
    else
	echo_char(CR);
    }

/************************************************************************
 * VOID enter_dem()
 *
 *    enter_dem() enters deferred echo mode. The transmit assembly packet
 *    is linked to the output queue, an enter_deffered_echo packet is
 *    outputted, and the tymnet echo state is set to no balls out.  
 *    write_error is set when enter_dem() is unable to send a
 *    ENTER_DEFFERED_ECHO control packet or there are no buflets
 *    available for next transmit packet.   enter_dem() is called
 *    when tymnet echoing is enabled.
 *
 * Usage notes: write_error is set by enter_dem and will be checked
 *    by the calling process (tym_write). The output of characters 
 *    stops when write_error is set to STOP_WRITE.  write_error is
 *    set to STOP_WRITE when unable to allocate buflet chain for
 *    enter deferrred echo packet or unable to allocate transmit
 *    assembly packet.    
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID enter_dem()
    {
    
	
    /* tymnet echoing is active, but character cannot be echoed
     * locally.  local echo is turned off.
     */
    if (ap_pcis->nwritebytes != 0)
	(VOID)link_xmit_pkt(ap_pcis, YES);
    
    /* Send an enter deferred echo mode packet.
     */
    if ((write_error = send_ctrl_pkt(ap_pcis->lcistruct, 
	ap_pcis->logicalchnl, ENTER_DEFER_ECHO,	(UBYTE *)0, MUST_LINK)) 
	== SUCCESS)
	{
    
	/* If there is no transmit assembly packet, then allocate a new
	 * transmit assembly packet.
	 */
	if (!ap_pcis->assemblypkt)
	    write_error = get_xmit_pkt(ap_pcis);
	ap_pcis->tymechostate = T1;	/* No balls out */
	}
    else
	write_error = STOP_WRITE;
    }

/************************************************************************
 * VOID backspace(del_char)
 *    BOOL del_char     - delete character, not line
 *
 *    backspace() is called, when MCI echoing is enabled, to 
 *    backspace the transmit assembly buffer one character or
 *    one line.  write_error is set when backspace is unable
 *    to get the buflets needed to backspace.  backspace() is
 *    called when MCI echoing is enabled.
 *
 * Usage notes: backspace will not backspace past the beginning of
 *    the transmit buffer. write_error is set by backspace but will 
 *    be checked by the calling process (mci_write).
 *    The output of characters stops when write_error is set
 *    to STOP_WRITE.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID backspace(del_char)
    BOOL del_char;		        /* delete character */
    {
    BUFLET *p, *pp;		        /* temporary buffer pointers */
    UWORD n, i;
    
    ++numprc;
    
    /* If del_char is on, backspace one character in the transmit
     * buffer.  Otherwise, delete entire line in transmit buffer.
     */ 
    if ((n = ap_pcis->nwritebytes) != 0)
        {
	if (del_char)
	    n = 1;
    	
	/* Need three times the number of characters to backspace so
         * that have room to output backspace blank backspace.
         */
	if ((write_error = getebufs(ap_pcis, 3 * n)) != SUCCESS)
	    return;
    	
	/* Delete one character.  If pointing to beginning of buflet,
         * must backspace to previous buflet.
         */
	if (del_char)
	    {
	    if (trans_idx == 0 && ptrans != ap_pcis->assemblypkt)
		{
		for (pp = p = ap_pcis->assemblypkt; p != ptrans && p; p = p->bufnext)
		    pp = p;
		ptrans = pp;
	        trans_idx = DATA_BUF_SIZ;
		}
	    --trans_idx;
	    --ap_pcis->nwritebytes;
	    }
    	
	/* Blank entire transmit buffer by backspacing and outputing a
         * blank.
         */
	else
	    {
	    ptrans = ap_pcis->assemblypkt;
	    ap_pcis->nwritebytes = 0;
	    trans_idx = MOVED_FIRST_BYTE;
	    }    

	/* Echo backspace, blank, backspace for specified number
	 * of characters until finished or error is encountered.
	 */
	for (i = 0; i < n && (write_error == SUCCESS); ++i)
	    {
	    echo_char((UBYTE)BACKSPACE);
	    if (write_error == SUCCESS)
		{
		echo_char((UBYTE)' ');
		if (write_error == SUCCESS)
		    echo_char((UBYTE)BACKSPACE);
		}
	    }
	}
	
    }
    
/************************************************************************
 * VOID redisplay()
 *
 *    redisplay() redisplays the contents of the transmit assembly 
 *    packet.  All characters in the transmit buffer are appended to the
 *    echo buffer.  write_error is set when redisplay() is unable
 *    to get the buflets needed to redisplay the transmit assembly
 *    packet.  redisplay is called when MCI echoing is enabled.
 *
 * Usage notes: write_error is set by redisplay but will be checked
 *    by the calling process (mci_write).  The output of characters 
 *    stops when write_error is set to STOP_WRITE.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID redisplay()
    {
    BUFLET *p;				/* pointer to buflet */
    UWORD idx;				/* index into the buflet */
    UWORD i, n;				/* temporary counters */
     
    
    ++numprc;
    
    /* Redisplay gets enough space to display the current
     * line in the transmit packet.  The contents of the 
     * transmit packet is redisplayed.
     */	
    if ((write_error = getebufs(ap_pcis, ap_pcis->nwritebytes + 1))
	!= SUCCESS)
        return;
    
    /* Echo carriage return.
     */
    echo_cr();
    for (p = ap_pcis->assemblypkt, n = ap_pcis->nwritebytes, i = 0, 
	idx = MOVED_FIRST_BYTE; i < n && p; p = p->bufnext, idx = 0)
	{
	while ((write_error == SUCCESS) && idx < DATA_BUF_SIZ && i++ < n)
	    echo_char(p->bufdata[idx++]);
	}
    }





    

/************************************************************************
 * pkt_write.c  - Output Data to Assembly Transmit Packet
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    pkt_write.c contains the function p_write_data which controls the
 *    building of output data packets.  The type of processing performed
 *    depends on the current echo state.  If tymnet echo is active,
 *    tym_write_data is called.  mci_write is called to build data
 *    packets when mci echo/edit is active.  Otherwise, noecho_write
 *    is called.   
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KS    Initial Draft
 * 06/19/87   4.01    KS    Move data from application buffer in pktwrite
 *                          to allow for forwarding character of non-echo
 *			    data.
 *
 ************************************************************************/
#include "stddef.h"
#include "xpc.h"
#include "pkt.h"
#include "appl.h"
#include "timer.h"
#include "padprm.h"
#include "state.h"
#include "error.h"


IMPORT INT get_echo_buf();		/* get an echo buffer */
IMPORT INT get_xmit_pkt();		/* get transmit assembly packet */
IMPORT INT link_xmit_pkt();		/* link xmit packet to output queue,
					 * error if queue is full.
					 */
IMPORT INT send_ctrl_pkt();		/* send control packet */
IMPORT VOID int_disable();		/* disable interrupts */
IMPORT VOID int_enable();		/* enable interrupts */
IMPORT VOID mci_write();		/* write data when MCI echoing on */
IMPORT VOID mov_param();		/* move data in/out of application
					 * parameter buffer.
					 */ 
IMPORT VOID noecho_write();		/* write data with no echoing */
IMPORT VOID start_timer();		/* start timer */
IMPORT VOID stop_timer();		/* stop timer */
IMPORT VOID tim_forwarding();		/* forwarding character timer */
IMPORT VOID tim_ballout();		/* colored ball out timer */
IMPORT VOID tim_noact();		/* no activity timer*/
IMPORT VOID tym_write();		/* write with tymet echo on */
 
IMPORT BUFLET *pecho;			/* pointer to echo buffer */
IMPORT BUFLET *ptrans;			/* pointer to transmit buffer */
IMPORT CHNLINFO *ap_pcis;		/* current channel information
					 * structure pointer. This is
					 * set up by application 
					 * interface.
					 */
IMPORT INT numecho;			/* number of echo bytes */
IMPORT UBYTE app_chnl;			/* channel specified by application */

IMPORT UWORD free_count;		/* number of buflets which are
					 * left.
					 */
IMPORT UWORD numprc;			/* number of characters processed */
IMPORT UWORD numreq;			/* number requested by application */
IMPORT UWORD program_error;		/* a programing error has 
					 * occurred.
					 */
IMPORT UWORD trans_idx;			/* index into current transmit
					 * buffer.
					 */

#ifdef DEBUG
IMPORT WORD diags[];			/* used for debugging */
#endif

/************************************************************************
 * LOCAL INT end_write_prc()
 *
 *    end_write_prc() finishes processing of packet write.
 *    The no activity timer is started.  If another write
 *    will not fit in the current packet, the packet is linked.
 *    If flow is controled, the forwarding character timer
 *    is started.  The number of bytes processed is moved into
 *    the application parameter buffer.
 *
 * Returns:  end_write_prc() always returns SUCCESS.
 *
 ************************************************************************/
LOCAL INT end_write_prc()
    {
 
/*

    Originally, the no activity timer was set when tymnet echoing
    was in red ball out state.  Processing modified, to only start
    timer when in no balls out state.  Waiting for answer to question
    before code is removed.
    
    if (ap_pcis->tymechostate == T1 || ap_pcis->tymechostate == T3)
 */
    
    /* If no balls have been sent out, then start the no activity timer.
     */
    if (ap_pcis->tymechostate == T1)
	start_timer(SIXTH_SEC_TIMER, TIM_TYMNOACTIV, 
	    ap_pcis->logicalchnl, LEN_TYMNOACTIV, tim_noact);
    
    /* If there is no forwarding timeout  and MCI editing is not
     * active then try to link the transmit assembly packet.
     */
    if (ap_pcis->padparams[FWD_TIME] == 0)
	{
	if (ap_pcis->mciechostate != M2)
	    (VOID)link_xmit_pkt(ap_pcis, NO);
	}

    /* If more than a certain number of characters is in the transmit
     * assembly packet then try to link the packet.
     */
    else
	{
	if (ap_pcis->nwritebytes > IO_DATA_LEN)
	    (VOID)link_xmit_pkt(ap_pcis, NO);
    
	/* If the transmit assembly packet was not linked then start
	 * the forwarding timer.
	 */
	if (ap_pcis->nwritebytes > 0)
	    start_timer(SIXTH_SEC_TIMER, TIM_FORWARDING, ap_pcis->logicalchnl,
		ap_pcis->padparams[FWD_TIME], tim_forwarding);
	}
	
    /* Copy the number of characters written to the application parameter
     * buffer.
     */
    mov_param((UBYTE *)&numprc, sizeof(UWORD), PARAM_3, 0, TO_APPL);
    return (SUCCESS);
    }

/************************************************************************
 * INT p_write_data()
 *
 *    pkt_write() moves data from the application parameter into
 *    a packet and outputs the packet(s) to the outputqueue.  
 *    The processing which is performed is dependent on the
 *    current echo state.  Processing stops when the queue is
 *    full or there are no more buflet chains available.
 *    The application should call report I/O status before p_write_data().
 *    The report I/O status will tell the application when the output
 *    is flow controlled.
 *
 * Returns:  
 *    SUCCESS - Write was successful.
 *    BUFFER_OVERFLOW - Application specified invalid number of 
 *        bytes to write.
 *    ILLEGAL_DATA_TYPE - invalid format.
 *    ILLEGAL_FLOW_STATE - There are no buffers available or
 *        flow control is on when p_write_data() is called. 
 *
 ************************************************************************/
INT p_write_data()
    {
    UBYTE buffer[CWD_MAX_DATA];		/* output buffer */
    UWORD format;			/* output character format */
 

   /* If the channel has been connected, extract the length of the
    * write request from the application parameters, move data
    * from the application parameter buffer into packets, and
    * link the packet frame to the output queue.  The number of
    * characters written is moved into the application buffer.
    */
    
    numprc = 0;
    
    if (ap_pcis->chnlstate != CHNL_CONNECTED || app_chnl == 0)
	return (ILLEGAL_CHNL_FUNC);

    /* If there is no assembly packet, then allocate a new one.
     */
    if (!ap_pcis->assemblypkt)
	{
	if (free_count >= MIN_WRTBUFLETS)
	    if (get_xmit_pkt(ap_pcis) != SUCCESS)
	    	program_error = FREE_COUNT_ERROR;
	}
    
    /* If the transmit assembly packet could not be linked or there is
     * no transmit assembly packet, then return error.
     */
    if (!ap_pcis->assemblypkt || ap_pcis->holdassembly)
	return (ILLEGAL_FLOW_STATE);

    /* Extract the number of characters requested for write and
     * the data format.  Currently, the data format is not
     * used.
     */

    mov_param((UBYTE *)&numreq, sizeof(UWORD), PARAM_2, 0, FROM_APPL);
    mov_param((UBYTE *)&format, sizeof(UWORD), PARAM_4, 0, FROM_APPL);
    numreq = (numreq > 128) ? 128 : numreq;
    mov_param((UBYTE *)buffer, (INT)numreq, PARAM_1, 0, FROM_APPL);
    
    if (format != TYMNET_STRING)
        return(ILLEGAL_DATA_TYPE);

    /* Stop the forwarding character timer.  This is turned back
     * on when  p_write_data  has completed.  
     */
    stop_timer(SIXTH_SEC_TIMER, TIM_FORWARDING, ap_pcis->logicalchnl);
    
    /* If there is an echo packet then extract the number of characters
     * which have been echoed and the current echo pointer.
     */
    numecho = -1;
    if (ap_pcis->echopkt != NULLBUF)
	{
        numecho = (INT)ap_pcis->echopkt->bufdata[FRAME_LEN];
	
	/* Calculate current echo buflet position (address and buflet index).
	 */
	for (ap_pcis->idxechodata = (numecho + EXTRA_DATA), 
	    pecho = ap_pcis->echopkt;
	    ap_pcis->idxechodata >= DATA_BUF_SIZ; pecho = pecho->bufnext, 
	    ap_pcis->idxechodata -= DATA_BUF_SIZ)
	    ;
	}
    
    /* Calculate the current transmit assembly buflet pointer
     *  and the index into buflet.
     */
    for (trans_idx = (ap_pcis->nwritebytes + MOVED_FIRST_BYTE), 
	ptrans = ap_pcis->assemblypkt; trans_idx >= DATA_BUF_SIZ; 
	ptrans = ptrans->bufnext, trans_idx -= DATA_BUF_SIZ)
	;
 
    /* Write data when tymnet echoing is active.
     */
    if (ap_pcis->tymechostate != T0)
	{
	int_disable();
/*    
	
    Originally, the no activity timer was set when tymnet echoing
    was in red ball out state.  Processing modified, to only start
    timer when in no balls out state.  Waiting for answer to question
    before code is removed.
    
    
	if (ap_pcis->tymechostate == T1 || ap_pcis->tymechostate == T3)
	    {
	    stop_timer(SIXTH_SEC_TIMER, TIM_TYMNOACTIV, ap_pcis->logicalchnl);
	    int_enable();
	    tym_write(buffer);
	    }
 */
	/* tymnet state is NBO  or RBO */
	if (ap_pcis->tymechostate == T1 || ap_pcis->tymechostate == T3)
	    {
	   
	    /* If the tymnet state is no balls out, then stop the no
	     * activity timer.
	     */
	    if (ap_pcis->tymechostate == T1)
		stop_timer(SIXTH_SEC_TIMER, TIM_TYMNOACTIV, 
		    ap_pcis->logicalchnl);
	    int_enable();
	    
	    /* State is no balls out or red ball out.  Move data from
	     * the application parameter buffer to the transmit 
	     * assembly packet.
	     */
	    tym_write(buffer);
	    }

	else if (ap_pcis->tymechostate == T2)      /* GBO state */
	    {
	    stop_timer(ONE_SEC_TIMER, TIM_BALLOUT, ap_pcis->logicalchnl);
	    int_enable();

	    /* Send a RED Ball packet and set the state to RBO.
	     */
	    if (send_ctrl_pkt(ap_pcis->lcistruct,ap_pcis->logicalchnl, 
		RED_BALL, (UBYTE *)0, MUST_LINK) != SUCCESS)
		start_timer(ONE_SEC_TIMER, TIM_BALLOUT,
		    ap_pcis->logicalchnl, LEN_BALLOUT, tim_ballout);
	    else
		{
		ap_pcis->tymechostate = T3;        /*  RBO state */
		start_timer(ONE_SEC_TIMER, TIM_BALLOUT, 
		    ap_pcis->logicalchnl, LEN_BALLOUT, tim_ballout);
		
		/* State is now RBO, move data from the application
		 * parameter buffer to transmit assembly packet 
		 * using tymnet echoing.
		 */
		tym_write(buffer);
		}
	    }
	else
	    {
	    int_enable();
	    if (ap_pcis->tymechostate == T5)	   /* local echo on */
		{

		/* If the read queue is full, then don't write the data.
		 */
		if (ap_pcis->nreadbytes > MAX_ECHO_CHARS)
		    return (end_write_prc());
		
		/* Since echoing is on, make sure there is an echo
		 * packet.
		 */
		if (!ap_pcis->echopkt)
		    {
		    if (get_echo_buf(ap_pcis) != SUCCESS)
			return (end_write_prc());
		    }
		
		/* Move data from the application parameter buffer
		 * to the transmit assembly packet using tymnet
		 * echoing.
		 */
		tym_write(buffer);
		}
	    else
		tym_write(buffer);
	    }
	}
    
    /* Write data when MCI echoing is active.
     */
    else if (ap_pcis->mciechostate != M0)
	{
	
	/* if the application read queue is full then don't write
    	 * any more characters.
	 */
	if (ap_pcis->nreadbytes > MAX_ECHO_CHARS)
	    return (end_write_prc());

	/* Make sure there is an echo buflet before writing characters.
	 */
	if (!ap_pcis->echopkt)
	    {
	    if (get_echo_buf(ap_pcis) != SUCCESS)
		return (end_write_prc());
	    }

	/* Move data to from the application parameter buffer into 
	 * the transmit assembly packet using MCI echoing.
	 */
	mci_write(buffer);
	}
	
    /* No echoing is active now.  Set the number of characters processed
     * to 0 and move data from application parameter buffer to
     * transmit assembly packet.
     */
    else
	noecho_write(buffer);
    return (end_write_prc());
    }


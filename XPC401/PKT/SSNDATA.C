
/************************************************************************
 * ssndata.c  - Process Session Call Data
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    ssndata.c() contains the module read_ssndata().  read_ssndata()
 *    processes any session call data packets which have linked to
 *    the channel information structure when a session request
 *    or session accept packet was received by the pad.
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
#include "state.h"
#include "error.h"

IMPORT INT read_buflet_data();            /* read buflet data */
IMPORT VOID free_buf();			  /* free buflet chain */
IMPORT VOID mov_param();                  /* move data to/from application
                                           * parameter buffer.
                                           */

IMPORT CHNLINFO *ap_pcis;  		  /* pointer to channel information
					   * structure.
					   */ 
IMPORT UBYTE app_chnl;			  /* application channel number*/
IMPORT WORD diags[];
IMPORT UWORD free_count;

/************************************************************************
 * INT read_ssndata()
 *
 *    read_ssndata copies session call data from a session request
 *    or a session accept packet, into the application parameter
 *    buffer.
 *
 * Usage notes: read_ssndata calls read_buflet_data to 
 *    move data from the buflet into the application parameter
 *    buffer. It is assumed that the session data will always
 *    be pointed to by application parameter 1.
 *
 * Returns:  
 *    SUCCESS - Processing was successful.
 *    INVALID_CHNL_FUNC - Channel 0 was specified by the application     
 *        or the channel is in an incorrect state.
 *    ILLEGAL_CALL_REQ - Invalid number of characters requeusted.
 *
 ************************************************************************/
INT read_ssndata()
    {
    BUFLET *pb;                           /* starting packet pointer */
    BUFLET *ppkt;			  /* current packet pointer */
    INT num = 0;			  /* number of characters read */
    UWORD idx;				  /* index into buflet data */
    UWORD nreq;                           /* maximum number of bytes to read */
    UWORD opos = 0;                       /* position for output buffer */
    UWORD format = TYMNET_STRING;         /* buffer format */
 


    /* If channel state is valid, move session data from the 
     * input packet to the application parameter buffer. The
     * pointer to the session data is stored in the channel 
     * information structure when a session request or accept
     * is received.
     */
 

    if ((ap_pcis->chnlstate != CHNL_CALL_ACP && 
	ap_pcis->chnlstate != CHNL_RECV_CALL) || (app_chnl == 0))
	return (ILLEGAL_CHNL_FUNC);

    /* Get the number of bytes requested from the application parameter
     * buffer. 
     */
    mov_param((UBYTE *)&nreq, sizeof(UWORD), PARAM_2, 0, FROM_APPL);
    
    /* If there is session data then calculate the current buflet
     * position.
     */
    if ((ppkt = pb = ap_pcis->ssndatapkt) != NULLBUF)
	{
	for (idx = ap_pcis->idxssndata; ppkt && (idx >= DATA_BUF_SIZ);
	    idx -= DATA_BUF_SIZ, ppkt = ppkt->bufnext)
		;
	
	/* Move data from session request packet into application
	 * parameter buffer.
	 */
	num = read_buflet_data(ppkt, nreq, (UWORD)pb->bufdata[FRAME_LEN],
	    (WORD)idx, &opos );


	/* If all of the data was moved then free the buflet.
	 * and update the session state. 
	 */
	if (num == (INT)pb->bufdata[FRAME_LEN])
	    {
	    free_buf(pb);

	    ap_pcis->ssndatapkt = NULLBUF;
	    ap_pcis->idxssndata = EXTRA_DATA;
    
	    /* Update the channel state depending on the session state.
	     * If the device is a DTE, then set the channel state
	     * to pending call accept.  Otherwise, set the channel state
	     * to connected.
	     */
	    if (ap_pcis->lcistruct->ssnstate == S3)
		ap_pcis->chnlstate = CHNL_PEND_ACP;
	    else
		ap_pcis->chnlstate = CHNL_CONNECTED;
	    }
	
	/* Update the number of bytes in the session packet
	 * and the index to the session data.
	 */
	else
	    {
	    pb->bufdata[FRAME_LEN] -= (UBYTE)num;
	    ap_pcis->idxssndata += num;
	    }
	}
    
    /* Move the number of bytes of session data read and a hard-coded
     * format to the application parameter buffer.
     */
    mov_param((UBYTE *)&num, sizeof(UWORD), PARAM_3, 0, TO_APPL);
    mov_param((UBYTE *)&format, sizeof(UWORD), PARAM_4, 0, TO_APPL);
    return (SUCCESS);
    }



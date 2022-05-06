/************************************************************************
 * padprm.cc - Report/ Set/Reset Pad Parameters
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    padprm.c contains the source modules which are called to 
 *    report, set or reset pad parameters.   rpt_padparam()
 *    reports the current pad parameters; set_padparam() sets
 *    the current pad parameters;  edit_on() turns MCI 
 *    editing on and sets the editing pad parameters; edit_off() 
 *    turns MCI editing off and clears the editing pad parameters; 
 *    set_fchar() sets the forwarding character pad parameter; 
 *    set_ftimout() sets the forwarding timeout value.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KS    Initial Draft
 *
 ************************************************************************/

#define MIN_CHAR	0		/* minimum character value */
#define MAX_CHAR	255		/* maximum character value */
#define MIN_LOCAL_ECHO	0		/* minimum value for local echoing */
#define MAX_LOCAL_ECHO	2		/* maximum define for local echoing */
#define MIN_ONOFF	0		/* minimum value for on off */
#define MAX_ONOFF	1		/* maximum value for on off */
#define MIN_NUMBER	0		/* minimum numeric value */
#define MAX_NUMBER	-1		/* maximum number (there is none) */

#include "stddef.h"
#include "xpc.h"
#include "pkt.h"
#include "appl.h"
#include "state.h"
#include "padprm.h"
#include "timer.h"
#include "error.h"

IMPORT VOID free_buf();	         	/* free buflet chain */
IMPORT VOID mov_param();		/* move data to/from application 
					 * parameter buffers.
					 */
IMPORT VOID pkt_in_error();		/* packet in error */

IMPORT CHNLINFO *ap_pcis;		/* pointer to channel information
					 * structure, set up by application.
					 */
IMPORT UBYTE app_chnl;			/* application channel number */
/* table containing minimum values for pad parameters
 */
LOCAL WORD min_pad_value[NUM_PAD_PARAM] = 
	    {0,				/* unused */
	     MIN_LOCAL_ECHO,		/* local echo minimum */
	     MIN_ONOFF,			/* echo tabs minimum */
	     MIN_ONOFF,			/* echo backspace minimum */
	     MIN_ONOFF,			/* echo escape minimum */
	     MIN_ONOFF,			/* echo line feed minimum */
	     MIN_ONOFF,			/* echo carriage return minimum */
	     MIN_CHAR,			/* forwarding character minimum */
	     MIN_NUMBER,		/* forwarding time out minimum */
	     MIN_ONOFF,			/* edit enable minimum */
	     MIN_CHAR,			/* delete character minimum */
	     MIN_CHAR,			/* delete line minimum */
	     MIN_CHAR,			/* redisplay line minimum */
	     -1,			/* unused */
	     -1,			/* unused */
	     -1,			/* unused */
	     MIN_ONOFF};		/* enable parity minimum */

/* table containing maximum values for pad parameters 
 * If maximum is negative, there is no maximum.
 */
LOCAL WORD  max_pad_value[NUM_PAD_PARAM] = 
	    
	    {0,				/* unused */
	     MAX_LOCAL_ECHO,		/* local echo maximum */
	     MAX_ONOFF,			/* echo tabs maximum */
	     MAX_ONOFF,			/* echo backspace maximum */
	     MAX_ONOFF,			/* echo escape maximum */
	     MAX_ONOFF,			/* echo line feed maximum */
	     MAX_ONOFF,			/* echo carriage return maximum */
	     MAX_CHAR,			/* forwarding character maximum */
	     MAX_NUMBER,		/* forwarding time out maximum */
	     MAX_ONOFF,			/* edit enable maximum */
	     MAX_CHAR,			/* delete character maximum */
	     MAX_CHAR,			/* delete line maximum */
	     MAX_CHAR,			/* redisplay line maximum */
	     -1,			/* unused */
	     -1,			/* unused */
	     -1,			/* unused */
	     MAX_ONOFF};		/* enable parity maximum */


#ifdef DEBUG
IMPORT WORD diags[];
#endif
/************************************************************************
 * INT rpt_padparam()
 *
 *    rpt_padparam() copies the value of the specified pad parameter
 *    from the channel information structure to the application 
 *    parameter buffer.
 *
 * Returns:
 *    ILLEGAL_PAD_PARAMETER - The specified pad parameter number
 *        is illegal.
 *    SUCCESS - Processing successful.
 *
 ************************************************************************/
INT rpt_padparam()
    {
    WORD pnum;			        /* number of pad parameter to report */

    mov_param((UBYTE *)&pnum, sizeof(WORD), PARAM_1, 0, FROM_APPL);
    
    /* If the parameter number is invalid, return an error.
     */
    if (pnum < 1 || (pnum > MAX_PAD_PARAM  && pnum != ENABLE_PARITY))
        return(ILLEGAL_PAD_PARAM);
    
    /* Return the value of the specified pad parameter.
     */
    mov_param((UBYTE *)&ap_pcis->padparams[pnum], sizeof(WORD), PARAM_2, 0, TO_APPL);
    return (SUCCESS);
    }

/************************************************************************
 * INT set_padparam()
 *
 *    set_padparam() copies the new pad parameters from the
 *    application parameter buffer into the channel information
 *    structure.
 *
 * Returns:  
 *    ILLEGAL_PAD_PARAMETER - The specified pad parameter or pad
 *        parameter value is illegal.
 *    SUCCESS - Processing successful.
 *
 ************************************************************************/
INT set_padparam()
    {
    WORD pnum;			        /* pad parameter number */
    WORD pad;		      	        /* pad parameter value */

    mov_param((UBYTE *)&pnum, sizeof(WORD), PARAM_1, 0, FROM_APPL);
    
    /* If the pad parameter number is invalid or the pad parameter
     * value is invalid, return an error.
     */
    if (pnum < 1 || (pnum > MAX_PAD_PARAM  && pnum != ENABLE_PARITY))
        return(ILLEGAL_PAD_PARAM);
    mov_param((UBYTE *)&pad, sizeof(WORD), PARAM_2, 0, FROM_APPL);

    /* If the pad parameter value is invalid for the specified pad
     * parameter, return an error.
     */
    if (min_pad_value[pnum] > pad)
        return(ILLEGAL_PAD_PARAM);
    if (max_pad_value[pnum] > 0)
	if (max_pad_value[pnum] < pad)
	    return(ILLEGAL_PAD_PARAM);
    
    /* Store the value of the specified pad parameter.
     */
    ap_pcis->padparams[pnum] = pad;
    return (SUCCESS);
    }
 
/************************************************************************
 * VOID edit_on(pcis, ppkt)
 *    FAST CHNLINFO *pcis; - pointer to channel information structure
 *    FAST BUFLET *ppkt;- pointer to incoming packet
 *
 *    edit_on turns MCI editing on.  If MCI echoing is currently
 *    enabled, edit_on moves the editing characters from the
 *    incoming packet into the pad parameters for the current
 *    channel and sets the MCI echo state to echo + edit.  
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID edit_on(pcis, ppkt)
    FAST CHNLINFO *pcis;		/* pointer to channel information
   					 * strucuture.
    					 */
    FAST BUFLET *ppkt;		        /* pointer to packet */
    {

    /* if MCI echoing is active, the set the mci state to edit and
     * move the editing parameters from the application parameter
     * buffer into the channel pad parameters buffer.
     */
    if (pcis->mciechostate == M1)	/* MCI echo on */
	{
	pcis->mciechostate = M2;	/* MCI echo and edit on */
	
	pcis->padparams[EDIT_DEL_CHAR] = (WORD)ppkt->bufdata[EXTRA_DATA];
	pcis->padparams[EDIT_DEL_LINE] = (WORD)ppkt->bufdata[EXTRA_DATA + 1];
	pcis->padparams[EDIT_DISP_LINE] = (WORD)ppkt->bufdata[EXTRA_DATA + 2];
	free_buf(ppkt);
	}
    
    /* MCI echoing is not active, clear the channel.
     */
    else
	pkt_in_error(pcis, ppkt);
    }

/************************************************************************
 * VOID edit_off(pcis, ppkt)
 *    FAST CHNLINFO *pcis; - pointer to channel information structure
 *    FAST BUFLET *ppkt;- pointer to incoming packet.
 *
 *    edit_off() turns MCI editing off.   If the current MCI echo state
 *    is edit + echo, edit_off() sets the MCI echo state to MCI echo 
 *    and the MCI editing parameters are cleared.
 *
 * Usage notes: Check if the editing parameters should be cleared.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID edit_off(pcis, ppkt)
    FAST CHNLINFO *pcis;		/* pointer to channel information
   					 * strucuture.
    					 */
    FAST BUFLET *ppkt;			/* pointer to packet */
    {
    
    /* If channel is in MCI editing mode, then set the MCI mode to
     * echo.   The pad parameters may have to be cleared.
     */
    if (pcis->mciechostate == M2)	/* MCI echo and edit */
    	{
	pcis->padparams[EDIT_DEL_CHAR] = 0;
	pcis->padparams[EDIT_DEL_LINE] = 0;
	pcis->padparams[EDIT_DISP_LINE] = 0;
	pcis->mciechostate = M1;	/* MCI echo state */
	free_buf(ppkt);
	}
    
    /* MCI editing is not active, clear the channel.
     */
    else
	pkt_in_error(pcis, ppkt);
    }
 
/************************************************************************
 * VOID set_fchar(pcis, ppkt)
 *    FAST CHNLINFO *pcis; - pointer to channel information structure
 *    BUFLET *ppkt;	- pointer to incoming packet 
 *
 *    set_fchar moves the forwarding character pad parameter from
 *    the incoming packet to the current channel's pad parameter.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID set_fchar(pcis, ppkt)
    FAST CHNLINFO *pcis;		/* pointer to channel information
   					 * strucuture.
    					 */
    BUFLET *ppkt;			/* pointer to packet */
    {
    
    /* If MCI echoing is active, the set the forwarding character
     * to the value specified in the packet.
     */
    if (pcis->mciechostate > M0)	/* MCI echo active */
	{
	pcis->padparams[FWD_CHAR] = (WORD)ppkt->bufdata[EXTRA_DATA];
        free_buf(ppkt);
    	}
    
    /* MCI echoing is not active, clear the channel.
     */
    else
	pkt_in_error(pcis, ppkt);
    }
 
/************************************************************************
 * VOID set_ftimout(pcis, ppkt)
 *    FAST CHNLINFO *pcis; - pointer to channel information structure
 *    BUFLET *ppkt;	- pointer to incoming packet
 *
 *    set_ftimout() sets the forwarding time out value to the
 *    value in the incoming packet.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID set_ftimout(pcis, ppkt)
    FAST CHNLINFO *pcis;		/* pointer to channel information
   					 * strucuture.
    					 */
    BUFLET *ppkt;			/* pointer to packet */
    {
    
    /* If MCI echoing is active, then set the forwarding timeout value
     * to the value in the packet.
     */
    if (pcis->mciechostate > M0)	/* MCI echo active */
	{
	pcis->padparams[FWD_TIME] = (WORD)ppkt->bufdata[EXTRA_DATA];
	free_buf(ppkt);
	}
    
    /* MCI echoing is not active, clear the channel.
     */
    else
	pkt_in_error(pcis, ppkt);
    }

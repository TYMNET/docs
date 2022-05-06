/************************************************************************
 * pktfunc.c - Application Packet Functions
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    pktfunc.c contains subroutines which execute application functions
 *    when device is in packet state.  These functions are not
 *    session functions but the packet functionality is different
 *    than character state processing.
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
#include "device.h"
#include "iocomm.h"
#include "state.h"
#include "error.h"
    
IMPORT BUFLET *alloc_buf();		/* allocate buflet chain */
IMPORT BYTES cpybuf();			/* copy buffer to buffer */
IMPORT CRC get_crc();			/* get crc for buffer */ 
IMPORT INT send_ctrl_data_pkt();	/* build and send control packet.
					 * when there is more than one 
					 * byte of data.
					 */
IMPORT INT send_ctrl_pkt();		/* build and send control packet when
					 * there is at most 1 byte of data.
					 */
IMPORT VOID init_new_state();		/* clear the iocomm transmit queue
					 * and reset circular read pointers.
					 */
IMPORT VOID add_queue();		/* add buflet chain to queue */
IMPORT VOID init_pad();                 /* flush all of the packet queues */
IMPORT VOID mov_param();		/* move data to and from application
					 * parameter buffers.
					 */
IMPORT VOID set_port();			/* set port parameters */

IMPORT BOOL xpc_timer_active;			/* xpc timer active */
IMPORT CHNLINFO *ap_pcis;		/* pointer to current channel info
					 * structure.
					 */
IMPORT CHNLINFO cis[];                  /* array of channel info             
                                         * structures.
                                         */
IMPORT COMMINFO comm_info;		/* comm info structure */
IMPORT DEVSTAT dev_stat;		/* device status structure */
IMPORT PORTPARAMS port_params;		/* port parameter structure */


LOCAL UBYTE restart_text[12] = {0x0B, 0x02, 0x02, 0x80, 0x00, 0xFB, 0x00, 0x00, 
				0x00, 0xF1, 0x00, 0x00};

/************************************************************************
 * INT p_set_charstate()
 *
 *    p_set_charstate() sets device from packet state to character
 *    state.
 *
 * Returns: p_set_charstate returns PACKET_CHNL_OPEN if any of the
 *    packet channels have not been disconnected.  Otherwise,
 *    p_set_charstate returns SUCCESS.  
 *
 ************************************************************************/
INT p_set_charstate()
    {
    BUFLET *pb;
    CRC crc;
    SHORT i;
    

    /* If there is a packet channel which is still connected, return
     * an error.
     */
    for (i = 1; i <= MAX_CHNL; ++i)
	if (cis[i].chnlstate != CHNL_DISCONNECTED)
	    return (PACKET_CHNL_OPEN);
    
    /* Clear the iocomm's transmit queues and the circular read buffer
     * pointer.
     */
    xpc_timer_active = NO;
    init_new_state();
    
    /* Initialize the pad.
     */
    init_pad(-1, YES, NO);
    
    /* Reset the port parameters for character state.  The port parameters
     * are modified during packet state.  They must be reset for character
     * state.
     */
    set_port(&port_params);
    
    /* Reset the communications input index for character state.  Index is
     * 0 during packet mode and 1 during character mode.
     */
    comm_info.xmtidx = 1;
    
    /* Add a restart to the output queue for channel 0.
     */
     
    if ((pb = alloc_buf(1)) != NULLBUF)
	{
	(VOID)cpybuf(pb->bufdata, restart_text,  12);

	crc = get_crc(pb, FRAME_LEN + 1, 4);
	pb->bufdata[CRC1 + 1] = (UBYTE)(crc >> 8);
	pb->bufdata[CRC1 + 2] = (UBYTE)(crc & 0xff);
	crc = get_crc(pb, EXTRA_DATA + 1, 2);
	pb->bufdata[EXTRA_DATA + 3] = (UBYTE)(crc >> 8);
	pb->bufdata[EXTRA_DATA + 4] = (UBYTE)(crc & 0xff);
	
	add_queue(&cis[CM_CHNL].lcistruct->outpktqueue, pb);
	cis[CM_CHNL].nwritebytes += 11;
	}
    
    /* Change the device status.
     */
    dev_stat.devstate = CHAR_STATE;
    xpc_timer_active = YES;
    
    return (SUCCESS);
    }


#ifdef DEBUG
/************************************************************************
 * INT bld_pad_pkt()
 *
 *    bld_pad_pkt() is a subroutine which is used to debugging to
 *    generate pad packets which turn echoing on and off.
 *
 * Returns:  bld_pad_pkt() always returns SUCCESS.
 *
 ************************************************************************/
INT bld_pad_pkt()
    {
    UWORD len;				/* length of data for packet */
    UWORD func;				/* function code for packet to be
    					 * built.
    					 */
    if (ap_pcis->lcistruct)
        { 
	mov_param((UBYTE *)&len, sizeof(UWORD), PARAM_2, 0, FROM_APPL);
	mov_param((UBYTE *)&func, sizeof(UWORD), PARAM_3, 0, FROM_APPL);
	if (len)
	    (VOID)send_ctrl_data_pkt(ap_pcis->lcistruct, ap_pcis->logicalchnl,
		func, len, YES);
        else
	    (VOID)send_ctrl_pkt(ap_pcis->lcistruct, ap_pcis->logicalchnl,
		 func, (UBYTE *)0, YES);
	}
    return(SUCCESS);
    }
#endif

/************************************************************************
 * INT p_set_pktstate()
 *
 *    p_set_pktstate() sets device to packet state when device is
 *    already in packet state. No processing is performed.
 *
 * Returns:  p_set_pktstate() always returns SUCCESS.
 *
 ************************************************************************/
INT p_set_pktstate()
    {

    return (SUCCESS);
    }


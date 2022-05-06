/************************************************************************
 * pktmsg - generate packet message text
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    This module contains routines used by the X.PC line monitor to
 * generate messages based on received packets.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KJB   Initial Draft
 *
 ************************************************************************/
#include "stddef.h"
#include "xpc.h"
#include "link.h"
#include "mon.h"
#include "pkt.h"

/************************************************************************
 * TEXT *pktmsg(pb)
 *     BUFLET *pb;		pointer to buflet chain
 *
 *     pktmsg will generate a text description of the packet contained
 * in the buflet chain at pb for subsequent output by the X.PC line
 * monitor.
 *
 * Notes: The first data position of the buflet at pb, normally containing
 *     a STX, may contain other codes flagging the packet as invalid (see
 *     validate_pkt).
 *
 * Returns: pktmsg returns a pointer to a static text string describing
 *     the packet at pb.
 *
 ************************************************************************/
TEXT *pktmsg(pb)
    BUFLET *pb;				/* pointer to buflet chain */
    {
    UBYTE gfi;				/* calculated gfi */

    /* generate message text based on packet type
     */
    switch (pb->bufdata[STX])
	{
	case MON_CRC1_ERR:
	    return ("CRC 1 ERR");
	case MON_CRC2_ERR:
	    return ("CRC 2 ERR");
	case MON_GFI_ERR:
	    return ("GFI BIT ERR");
	case MON_CHNL_ERR:
	    return ("INV CHNL NUM");
	case MON_QBIT_ERR:
	    return ("INV QBIT FUNC");
	case STX_CHAR:
	    gfi = pb->bufdata[GFI_LCI] & 0xf0;
	    if (!gfi)
	    	return ("DATA");
	    if (gfi == CBIT)
		switch (pb->bufdata[PKT_TYP_ID])
		    {
		    case 0x0b:
		    	return ("CALL REQ");
		    case 0x0f:
		    	return ("CALL ACC");
		    case 0x13:
		    	return ("CLR REQ");
		    case 0x17:
		    	return ("CLR CONF");
		    case 0x23:
		    	return ("INT");
		    case 0x27:
		    	return ("INT CONF");
		    case RR:
		    	return ("RR");
		    case RNR:
		    	return ("RNR");
		    case REJECT:
		    	return ("REJECT");
		    case RESET:
		    	return ("RESET");
		    case RESET_CONFIRM:
		    	return ("RESET CONF");
		    case RESTART:
		    	return ("RESTART");
		    case RESTART_CONFIRM:
		    	return ("RESTART CONF");
		    case DIAG:
		    	return ("DIAG");
		    default:
		    	break;
		    }
	    else if (gfi == QBIT)
		switch (pb->bufdata[PKT_TYP_ID])
		    {
		    case ENABLE_ECHO:
			return ("ENA ECHO");
		    case DISABLE_ECHO:
		    	return ("DIS ECHO");
		    case ENTER_DEFER_ECHO:
		    	return ("ENA DEF ECHO");
		    case EXIT_DEFER_ECHO:
		    	return ("DIS DEF ECHO");
		    case GREEN_BALL:
		    	return ("GREEN BALL");
		    case RED_BALL:
		    	return ("RED BALL");
		    case BEGIN_BRK:
		    	return ("BEG LOG BRK");
		    case END_BRK:
		    	return ("END LOG BRK");
		    case SESSION_REQ:
		    	return ("SSN REQ");
		    case SESSION_ACP:
		    	return ("SSN ACC");
		    case SESSION_CLR:
		    	return ("SSN CLR");
		    case SSN_CLR_ACP:
		    	return ("SSN CLR ACC");
		    case YELLOW_BALL:
		    	return ("YELLOW BALL");
		    case ORANGE_BALL:
		    	return ("ORANGE BALL");
		    case ENABLE_PERM_ECHO:
		    	return ("ENA PRM ECHO");
		    case DISABLE_PERM_ECHO:
		    	return ("DIS PRM ECHO");
		    case SET_FWD_CHAR:
		    	return ("SET FWD CHAR");
		    case SET_FWD_TIMEOUT:
		    	return ("SET FWD TIMO");
		    case ENABLE_LOCAL_EDIT:
		    	return ("ENA LOC EDIT");
		    case DISABLE_LOCAL_EDIT:
		    	return ("DIS LOC EDIT");
		    default:
		    	break;
		    }
	    break;
	default:
	    break;
	}
    return ("INV TYPE");		/* unrecognized type */
    }

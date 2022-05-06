/************************************************************************
 * port.c - Set/Report Current Port Parameters
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    port.c contains the modules which are called to set and
 *    read the port parameters.
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
#include "device.h"
#include "state.h"
#include "appl.h"
#include "param.h"
#include "error.h"

IMPORT INT validate_portparam();        /* validate port params */
IMPORT VOID adjust_time_len();		/* adjust length of time between
					 * interrupts for T25 and T27 
					 * timers.
					 */
IMPORT VOID mov_param();                /* move parameter data */
IMPORT VOID set_port();			/* set port parameters */

IMPORT DEVSTAT dev_stat;                /* device status structure */
IMPORT PORTPARAMS port_params;          /* port parameter structure */
IMPORT UBYTE restart_devmode;		/* mode for restart */
    
/************************************************************************
 * INT read_portparam()
 *
 *    read_portparam() moves the port parameters from the port parameter
 *    structure to the application parameter buffer.
 *
 * Returns:  read_portparam always returns SUCCESS.
 *
 ************************************************************************/
INT read_portparam()
    {

    /* Move the contents of the port parameter structure into the
     * application buffer. If currently we are in packet state,
     * the port parameters may not match the current port parameters
     * because when in device state parity is always set to none,
     * 8 data bits are always set and xon_xoff are turned off.
     */
    mov_param((UBYTE *)&port_params, sizeof(PORTPARAMS), PARAM_1, 0, TO_APPL);    
    return (SUCCESS);
    }

/************************************************************************
 * INT set_portparam()
 *
 *    set_portparam() moves the port parameter structure from the
 *    application parameter buffer to the port parameter structure.
 *    When device is in packet mode parity is always turned off
 *    and 8 data bits are set.
 *
 * Returns:  
 *    SUCCESS - Processing Successful
 *    ILLEGAL_BAUD_RATE - Illegal Baud Rate
 *    ILLEGAL_PARITY - Illegal Parity
 *    ILLEGAL_NO_DATA_BITS - Illegal Numbver of Data Bits
 *    ILLEGAL_NO_STOP_BITS - Illegal Number of Stop Bits
 *    ILLEGAL_DTE_SPEC - Illegal DTE specification
 *
 ************************************************************************/
INT set_portparam()
    {
    PORTPARAMS t_port_params;           /* temporary port parameters */
    INT ret;                            /* return code */

    /* Move the new port parameters from the application read buffer.
     */
    mov_param((UBYTE *)&t_port_params, sizeof(PORTPARAMS), PARAM_1, 
	0,  FROM_APPL);
    
    /* Validate the port parameters.
     */
    if ((ret = validate_portparam(&t_port_params)) != SUCCESS)
        return (ret);
    
    port_params.baudrate = t_port_params.baudrate;
    port_params.stopbits = t_port_params.stopbits;
    port_params.dxemode = t_port_params.dxemode;
    
    /* DCE restart cause is 1 and DTE restart cause is 0 ( the opposite
     * of the mode specification.
     */
    restart_devmode = port_params.dxemode ? 0 : 1;
    port_params.databits = t_port_params.databits;
    port_params.parity = t_port_params.parity;
    port_params.xonxoff = t_port_params.xonxoff;
    
    /* When in packet state, then the number of data bits, parity
     * and xon_xoff are hard coded.
     */
    if (dev_stat.devstate == PKT_STATE)
	{
	t_port_params.databits = DATA_BIT_8;
	t_port_params.parity = NO_PARITY;
	t_port_params.xonxoff = XON_OFF;
	}

    /* If a port is active set the port parameters.
     */
    if (dev_stat.portaddr != 0)
	set_port(&t_port_params);
    
    /* Since the baud rate has changed, the timeout values for
     * packet acknowledgment and packet retransmission are
     * updated to reflect the new baud rate.
     */
    adjust_time_len();
    return (SUCCESS);
    }

/************************************************************************
 * INT validate_portparam(pp)
 *    PORTPARAMS *pp    - pointer to port parameter structure
 *
 *    validate_portparam validates the port parameter structure
 *    parameter entries.
 *
 * Returns:  
 *    SUCCESS - Processing Successful
 *    ILLEGAL_BAUD_RATE - Illegal Baud Rate
 *    ILLEGAL_PARITY - Illegal Parity
 *    ILLEGAL_NO_DATA_BITS - Illegal Numbver of Data Bits
 *    ILLEGAL_NO_STOP_BITS - Illegal Number of Stop Bits
 *    ILLEGAL_DTE_SPEC - Illegal DTE specification
 *
 ************************************************************************/
LOCAL INT validate_portparam(pp)
    PORTPARAMS *pp;                     /* port parameter structure */
    {

    /* Validate baud rate, number of stop bits, mode, parity and 
     * the number of data bits.
     */
    if (pp->baudrate < BAUD_110 || pp->baudrate > BAUD_9600)
        return (ILLEGAL_BAUD_RATE);
    if (pp->stopbits < ONE_STOP || pp->stopbits > TWO_STOP)
        return (ILLEGAL_NO_STOP_BITS);
    if (pp->dxemode < DCE_MODE || pp->dxemode > DTE_MODE)
        return (ILLEGAL_DTE_SPEC);
    if (pp->parity < NO_PARITY || pp->parity > SPACE_PARITY)
	return (ILLEGAL_PARITY);
    if (pp->databits < DATA_BIT_5 || pp->databits > DATA_BIT_8)
	return (ILLEGAL_NO_DATA_BITS);
    return (SUCCESS);
    }



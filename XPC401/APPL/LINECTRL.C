/************************************************************************
 * linectrl.c - Line Control and Line Status Application Functions
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    linectrl.c contains subroutines which are used to set
 *    the modem control register and test the modem status
 *    register.  The line control subroutines can be called
 *    when device is in character or packet state.
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
#include "device.h"
#include "iocomm.h"
#include "error.h"
 
IMPORT UWORD comm_status();		/* return communication line
					 * status.
					 */
IMPORT VOID mov_param();                /* move application parameter data */
IMPORT VOID reset_modem_ctrl();		/* clear modem control */
IMPORT VOID set_modem_ctrl();		/* set modem control */

IMPORT DEVSTAT dev_stat;                /* device status structure */
IMPORT WORD diags[]; 
 
/************************************************************************
 * INT test_dcd()
 *
 *    test_dcd() tests the state of the static carrier detect.
 *
 * Returns: test_dcd() returns ILLEGAL_PORT if port is not
 *   active.   Otherwise, test_dcd() returns SUCCESS. 
 *
 ************************************************************************/
INT test_dcd()
    {
    UWORD dcd_state;                     /* dcd state */


    /* Return error if no port is active.
     */
    if (dev_stat.portaddr == 0)
        return (ILLEGAL_PORT);
    
    /* Get status of carrier detect from the modem status register.
     */
    dcd_state = comm_status(MSR_RSLD) ? 1 : 0;
    mov_param((UBYTE *)&dcd_state, sizeof(UWORD), PARAM_1, 0, TO_APPL);
    return (SUCCESS);
    }
 
/************************************************************************
 * INT test_dsr()
 *
 *    test_dsr() tests the state of the data set ready.
 *
 * Returns: test_dsr() returns ILLEGAL_PORT if no port is
 *   active.   Otherwise, test_dsr() returns SUCCESS. 
 *
 ************************************************************************/
INT test_dsr()
    {
    UWORD dsr_state;                     /* dsr state */


    /* Return error if no port is active.
     */
    if (dev_stat.portaddr == 0)
        return (ILLEGAL_PORT);

    /* Call comm_status to get data set ready from the modem status
     * register.
     */
    dsr_state = comm_status(MSR_DSR) ? 1 : 0;
    mov_param((UBYTE *)&dsr_state, sizeof(UWORD), PARAM_1, 0, TO_APPL);
    return (SUCCESS);
    }
 
 
/************************************************************************
 * INT test_ri()
 *
 *    test_ri() tests the current state of the ring indicator.
 *
 * Returns: test_ri() returns ILLEGAL_PORT if no port is
 *   active.   Otherwise, test_ri() returns SUCCESS. 
 *
 ************************************************************************/
INT test_ri()
    {
    UWORD ri_state;                     /* ri state */


    /* Return error if no port is active.
     */
    if (dev_stat.portaddr == 0)
        return (ILLEGAL_PORT);
    
    /* Call comm_status to get status of ring indicator from modem
     * status register.
     */
    ri_state = comm_status(MSR_RI) ? 1 : 0;
    mov_param((UBYTE *)&ri_state, sizeof(UWORD), PARAM_1, 0, TO_APPL);
    return (SUCCESS);
    }
 
/************************************************************************
 * INT set_dtr()
 *
 *    set_dtr() sets data terminal ready.
 *
 * Returns: set_dtr() returns ILLEGAL_PORT if no port is active.
 *   Otherwise, set_dtr() returns SUCCESS. 
 *
 ************************************************************************/
INT set_dtr()
    {

    /* Return error if no port is active.
     */
    if (dev_stat.portaddr == 0)
        return (ILLEGAL_PORT);
    
    /* Set data terminal ready.
     */
    set_modem_ctrl(MCR_DTR);
    return(SUCCESS);
    }


/************************************************************************
 * INT reset_dtr()
 *
 *    reset_dtr() clears the data terminal ready.
 *
 * Returns: reset_dtr() returns ILLEGAL_PORT if no port is active.
 *   Otherwise, reset_dtr() returns SUCCESS. 
 *
 ************************************************************************/
INT reset_dtr()
    {
    
    /* Return an error if no port is active.
     */
    if (dev_stat.portaddr == 0)
        return (ILLEGAL_PORT);
    
    /* Clear data terminal ready.
     */
    reset_modem_ctrl(MCR_DTR);
    return(SUCCESS);
    }
 
/************************************************************************
 * INT set_rts()
 *
 *    set_rts() sets the requeust to send.
 *
 * Returns: set_rts() returns ILLEGAL_PORT if no port is active.
 *   Otherwise, set_rts() returns SUCCESS. 
 *
 ************************************************************************/
INT set_rts()
    {

    /* Return error if no port is active.
     */
    if (dev_stat.portaddr == 0)
        return (ILLEGAL_PORT);
    
    /* Set request to send.
     */
    set_modem_ctrl(MCR_RTS);
    return(SUCCESS);
    }

/************************************************************************
 * INT reset_rts()
 *
 *    reset_rts() clears request to send.
 *
 * Returns: reset_rts() returns ILLEGAL_PORT if no port is active.
 *   Otherwise, reset_rts() returns SUCCESS. 
 *
 ************************************************************************/
INT reset_rts()
    {

    /* Return error if no port is active.
     */
    if (dev_stat.portaddr == 0)
        return (ILLEGAL_PORT);
    
    /* Clear request to send.
     */
    reset_modem_ctrl(MCR_RTS);
    return(SUCCESS);
    }


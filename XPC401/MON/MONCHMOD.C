/************************************************************************
 * monchmod.c - X.PC line monitor mode routines
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    This module contains routines used by the X.PC line monitor to
 * format and dump received data to the screen/disk.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KJB   Initial Draft
 *
 ************************************************************************/
#include "stddef.h"
#include "device.h"

IMPORT VOID hout();			/* header output routine */
IMPORT VOID int_disable();		/* disables interrupts */
IMPORT VOID int_enable();		/* enables interrupts */

IMPORT DEVSTAT dev_stat;		/* device status structure #1 */
IMPORT DEVSTAT dev_stat2;		/* device status structure #2 */

/************************************************************************
 * VOID chmod(mode)
 *     INT mode;		new monitor mode
 *
 *     chmod is used to change the X.PC line monitor mode. If the new
 * mode does not match the current mode, the mode will be changed and a
 * header will be output.
 *
 * Notes: mode is assumed to be either CHAR_STATE or PKT_STATE.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID chmod(mode)
    INT mode;				/* new monitor mode */
    {
    
    /* if new mode is different than the current mode
     */
    if (mode != dev_stat.devstate)
	{
    
	/* with interrupts disabled, change the mode in each device
	 * status structure
	 */
	int_disable();
	dev_stat.devstate = dev_stat2.devstate = mode;
	int_enable();
	hout();				/* output new header */
	}
    }

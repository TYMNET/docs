/************************************************************************
 * chnl.c - AI Channel Functions
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    chnl.c contains the source modules which are called to open 
 *    and close channels for the AI Test program.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KS    Initial Draft
 *
 ************************************************************************/
#include "stddef.h"
#include "stdio.h"
#include "xpc.h"

IMPORT BOOL getword();
IMPORT INT printf();

IMPORT BOOL chnl_tab[];

/* reset_chnl - reset channel table
 */
VOID reset_chnl()
    {
    INT chnl;				/* running channel counter */
    
    for (chnl = 0; chnl < NUM_CHNL; ++chnl)
	chnl_tab[chnl] = chnl == 1;
    }

/* show_chnl - display "open" channels
 */
LOCAL VOID show_chnl()
    {
    INT chnl;				/* running channel counter */

    (VOID)printf("    open channels: ");
    for (chnl = 0; chnl < NUM_CHNL; ++chnl)
	if (chnl_tab[chnl])
	    (VOID)printf("%d  ", chnl);
    putchar('\n');
    }

/* val_chnl - validate channel
 */
LOCAL VOID val_chnl(chnl, flag)
    WORD chnl;				/* channel to be validated */
    BOOL flag;				/* channel open/close flag */
    {

    if (0 <= chnl && chnl < NUM_CHNL)
	chnl_tab[chnl] = flag;
    else
	(VOID)printf("\tinvalid channel number\n");
    }

/* count_chnl - count "open" channels
 */
LOCAL INT count_chnl()
    {
    INT chnl;				/* running channel counter */
    INT count;				/* open channel count */
    
    count = 0;
    for (chnl = 0; chnl < NUM_CHNL; ++chnl)
	if (chnl_tab[chnl])
	    ++count;
    return (count);
    }

/* close_chnl - "close" a channel
 */
VOID close_chnl()
    {
    WORD chnl;				/* input channel number */

    show_chnl();
    if (1 < count_chnl() && getword("close channel", &chnl))
	val_chnl(chnl, NO);
    putchar('\n');
    }

/* open_chnl - "open" a channel
 */
VOID open_chnl()
    {
    WORD chnl;				/* input channel number */

    show_chnl();
    if (getword("open channel", &chnl))
	val_chnl(chnl, YES);
    putchar('\n');
    }

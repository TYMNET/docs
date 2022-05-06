/************************************************************************
 * linkstat.c - Report Link Statistics
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    linkstat.c contains the source module link_stats which
 *    is called to report the link statistics.
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
#include "iocomm.h"

IMPORT VOID mov_param();		/* move data to/from
					 * application buffer.
					 */
IMPORT COMMINFO comm_info;		/* communications informations
					 * structure.
					 */
#ifdef DEBUG
WORD diags[500] = {0};			/* diagnostic buffer */
#endif
/************************************************************************
 * INT link_stats()
 *
 *    link_stats() is called to report the current link statistics.
 *
 *
 * Returns:  Link stats always returns SUCCESS.
 *
 ************************************************************************/
INT link_stats()
    {
    UWORD i;				/* counter */

    /* Copy the link statistics from the iocomm structure to the
     * application interface buffer.  The statistics are gathered
     * at the link level.
     */
    mov_param((UBYTE *)comm_info.linkstats, 6 * sizeof(UWORD), 
	PARAM_1, 0, TO_APPL);
    
    /* Clear the link statistics.
     */
    for (i = 0; i < 6; ++i)
	comm_info.linkstats[i] = 0;
    return (SUCCESS);
    }

/************************************************************************
 * INT debug_stats()
 *
 *    debug_stats() is called to report the debug statistics.
 *
 *
 * Returns:  Link stats always returns SUCCESS.
 *
 ************************************************************************/
INT debug_stats()
    {
#ifdef DEBUG
    
    /* Move diagnostics into application parameter buffer.
     */
    mov_param((UBYTE *)diags, sizeof(diags), PARAM_1, 0, TO_APPL);
#endif
    return(SUCCESS);
    }

    
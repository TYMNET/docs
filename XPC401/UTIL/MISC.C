/************************************************************************
 * misc.c - miscellaneous support functions
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    This module contains miscellaneous support functions used by the
 *    XPC Driver. Note these functions (or similar ones) are normally
 *    provided as part of a run-time library supplied by the compiler
 *    vendor; they are included here to reduce compiler dependencies.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KJB   Initial Draft
 *
 ************************************************************************/
#include "stddef.h"

/************************************************************************
 * BYTES cpybuf(to, from, nbytes)
 *     UBYTE *to;		copy destination address
 *     UBYTE *from;		copy source address
 *     BYTES nbytes;		number of bytes to copy
 *
 *     cpybuf copies nbytes bytes from location from to location to.
 *
 * Notes: source and destination addresses are not validated. No checks
 *     made for storage overwrites at to.
 *
 * Returns: cpybuf returns nbytes, the number of bytes copied.
 *
 ************************************************************************/
BYTES cpybuf(to, from, nbytes)
    FAST UBYTE *to;			/* copy destination address */
    FAST UBYTE *from;			/* copy source address */
    BYTES nbytes;			/* number of bytes to copy */
    {
    BYTES count;			/* running byte count */

    /* copy nbytes bytes from source from to destination to
     */
    for (count = nbytes; count; --count)
	*to++ = *from++;
    return (nbytes);			/* return number of bytes copied */
    }

/************************************************************************
 * BYTES fill(addr, nbytes, fchar)
 *     UBYTE *addr;		fill start address
 *     BYTES nbytes;		fill length
 *     UBYTE fchar;		fill character
 *
 *     fill writes nbytes fchar's to location addr.
 *
 * Notes: the address addr is not validated. No checks made for storage
 *     overwrites at addr.
 *
 * Returns: fill returns nbytes, the fill length.
 *
 ************************************************************************/
BYTES fill(addr, nbytes, fchar)
    FAST UBYTE *addr;			/* fill start address */
    BYTES nbytes;			/* fill length */
    UBYTE fchar;			/* fill character */
    {
    FAST BYTES count;			/* running byte count */
    
    /* write nbytes fchar's to location addr
     */
    for (count = nbytes; count; --count)
	*addr++ = fchar;
    return (nbytes);			/* return fill length */
    }

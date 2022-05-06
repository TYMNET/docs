/************************************************************************
 * dosutl.c - MS-DOS utility functions
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    This module contains MS-DOS utility functions used by early versions
 * of the XPC Driver and by miscellaneous test programs.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KJB   Initial Draft
 *
 ************************************************************************/
#include "stddef.h"
#include "dos.h"

IMPORT BYTE *malloc();			/* MSC's malloc */
IMPORT VOID free();			/* MSC's free */
IMPORT VOID int86();			/* MSC'S int86 (actually an INT) */

/************************************************************************
 * UBYTE *dos_alloc(nbytes)
 *     BYTES nbytes;		number of bytes to allocate
 *
 *     dos_alloc is used to dynamically allocate nbytes bytes from the
 * operating system. 
 *
 * Notes: nbytes is not validated.
 *
 *     This version of dos_alloc uses Microsoft's malloc function to
 *     allocated memory off the heap. This function will change when the
 *     XPC load-time memory allocation scheme is designed/implemented.
 *
 * Returns: If nbytes bytes are successfully allocated, a pointer to the
 *     allocated memory is returned; otherwise, a null pointer will be
 *     returned.
 *
 ************************************************************************/
UBYTE *dos_alloc(nbytes)
    BYTES nbytes;			/* number of bytes to alloc */
    {

    return ((UBYTE *)malloc((unsigned)nbytes));
    }

/************************************************************************
 * VOID dos_free(ptr)
 *     UBYTE *ptr;		pointer to alloc'ed area
 *
 *     dos_free is used to return allocated memory to the operating
 * system. ptr is a pointer to allocated memory obtained via dos_alloc.
 *
 * Notes: ptr is not validated.
 *
 *     This version of dos_free uses Microsoft's free function to return
 *     allocated memory to the heap. This function will change when the
 *     XPC load-time memory allocation scheme is designed/implemented.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID dos_free(ptr)
    UBYTE *ptr;				/* pointer to alloc'ed area */
    {
    
    free(ptr);
    }

/************************************************************************
 * INT inchar()
 *
 *     inchar performs a 1 byte read from the console.
 *
 * Notes: inchar is provided for diagnostic purposes only.
 *
 * Returns: If a character is waiting to be read at the console, the
 *     character is returned; else null.
 *
 ************************************************************************/
INT inchar()
    {
    union REGS in;			/* input "register" */
    union REGS out;			/* output "register" */

    in.x.ax = 0x600;
    in.x.dx = 0xff;
    int86(0x21, &in, &out);
    return ((INT)out.x.ax & 0xff);
    }

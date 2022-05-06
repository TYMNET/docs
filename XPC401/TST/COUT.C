/************************************************************************
 * cout.c - Display Character
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    cout.c contains the source module cout.  cout displays characters.  
 *    If the character to be displayed is printable, it is  displayed.
 *    Otherwise, the character's hex value is displayed.
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
#include "ctype.h"

/* cout - output character
 */
    VOID cout(c)				/* character to be output */
    UBYTE c;
    {
    INTERN UBYTE hex[] =		/* hex character array */
	{"0123456789ABCDEF"};
    
    if (isprint(c))			/* if printable ascii */
	putchar(c);
    else				/* else output in hex */
	{
	putchar('$');
	putchar(hex[c >> 4]);
	putchar(hex[c & 0x0f]);
	}
    }

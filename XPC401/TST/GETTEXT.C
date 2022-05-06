/************************************************************************
 * gettext.c  - Input Characters from keyboard
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    gettext.c contains the source module gettext() which is called
 *    to wait for input data and copy the input data to a specified buffer.
 *    Input stops when carriage return or escape characters is pressed.
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
#include "xpc.h"

IMPORT INT inchar();			/* input character */
IMPORT INT printf();			/* returns output count */
IMPORT VOID cout();			/* character output */

/* gettext - input text
 */
INT gettext(p, flag)
    TEXT *p;				/* pointer to input buffer */
    BOOL flag;				/* if flag is yes, append /r to
     					 * end of string.
					 */
    {
    BOOL esc;				/* escape character flag */
    INT c;				/* saves input character */
    INT i;				/* character count (returned) */
    
    esc = NO;
    i = 0;
    FOREVER
	{

	/* wait for input character
	 */
	while ((c = inchar()) == 0)
	    ;
	
	if (esc)			/* character was escape'ed */
	    {
	    *(p + i++) = c;		/* install character in buffer */
	    cout(c);			/* echo character */
	    esc = NO;			/* clear escape flag */
	    }
	else if (c == '\\')		/* if escape next character */
	    esc = YES;			/* set escape flag */
	else				/* else process character */
	    if (c == ESCAPE)
	
		/* ESC hit. if text has been entered, clear it
		 */
		if (i)
		    do  {
			(VOID)printf("\b \b");
			--i;
			
			/* if character is non-ascii, it was displayed in the
			 * format "$XX" (hex). backspace two more
			 */
			if (!isprint(*(p + i)))
			    {
			    (VOID)printf("\b \b");
			    (VOID)printf("\b \b");
			    }
			} while (i);
		
		/* else return negative length
		 */
		else
		    return (-1);
	    
	    /* if CR or LF hit, echo a newline, null terminate the input
	     * buffer, and return its length
	     */
	    else if (c == '\r' || c == '\n')
		{
		putchar('\n');
		if (flag)
		    *(p + i++) = '\r';
		*(p + i) = 0;
		return (i);
		}
	    
	    /* if BS or DEL hit, backspace (if necessary)
	     */
	    else if (c == BACKSPACE || c == 0x7f)
		{
		if (i)			/* if something in input buffer */
		    {
		    (VOID)printf("\b \b");
		    --i;
		
		    /* if character is non-ascii, it was displayed in the
		     * format "$XX" (hex). backspace two more
		     */
		    if (!isprint(*(p + i)))
			{
			(VOID)printf("\b \b");
			(VOID)printf("\b \b");
			}
		    }
		}
	    
	    /* else install character in buffer and echo
	     */
	    else
		{
		*(p + i++) = c;
		cout(c);
		}
	}    
    }
	

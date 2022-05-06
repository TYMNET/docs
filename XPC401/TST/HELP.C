/************************************************************************
 * help.c - Display Help Data
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    help.c displays the functions which are available for the AI
 *    program.
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
#include "ai.h"
#include "appl.h"

IMPORT INT printf();			/* returns output count */

IMPORT FUNC func[];			/* function table */

/* help - display help
 */
VOID help()
    {
    INT i;				/* loop counter */
    INT j;				/* loop counter offset */
    
    putchar('\n');

    /* output function names, 15 lines, 3 columns
     */
    for (i = 0; i < 15; ++i)
	{
	(VOID)printf("%-3d %-23s", i, func[i + 1].name);
	j = i + 15;
	(VOID)printf("%-3d %-23s", j, func[j + 1].name);
	if ((j + 15) < 45)
	    {
	    switch (j += 15)
		{
		case MAX_APPL_FUNC + 2:
		    (VOID)printf("%s", "34  Disp event variables");
		    break;
		case MAX_APPL_FUNC + 3:
		    (VOID)printf("%s", "35  Upd event variables");
		    break;
		case MAX_APPL_FUNC + 5:	/* second entry past func end */
		    (VOID)printf("%s", "c   \"close\" channel");
		    break;
		case MAX_APPL_FUNC + 6:	/* third entry past func end */
		    (VOID)printf("%s", "o   \"open\" channel");
		    break;
		case MAX_APPL_FUNC + 7:	/* fourth entry past func end */
		    (VOID)printf("%s", "t   tty mode");
		    break;
		case MAX_APPL_FUNC + 8:	/* fifth entry past func end */
		    (VOID) printf("%s","u   tty no display");
		    break;
		case MAX_APPL_FUNC + 9:
		    (VOID) printf("%s","l   test flush output");
		    break;
		case MAX_APPL_FUNC + 10:
		    (VOID) printf("%s","m   test flush input");
		    break;
		case MAX_APPL_FUNC + 11:
		    (VOID) printf("%s","v   view event variables");
		     break;
		case MAX_APPL_FUNC + 12:	/* fifth entry past func end */
		    (VOID)printf("%s", "esc quit");
		    break;

		default:			/* valid function index */
		    (VOID)printf("%-3d %s", j, func[j + 1].name);
		
		case MAX_APPL_FUNC + 4:	/* first entry past func end */
		    break;			/* blank line */
	    }
	}
	putchar('\n');
	}
    putchar('\n');
    }

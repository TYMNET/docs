/************************************************************************
 * monerror.c - X.PC line monitor error routines
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    This module contains routines used by the X.PC line monitor to
 * report/handle errors.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KJB   Initial Draft
 *
 ************************************************************************/
#include "stddef.h"

IMPORT INT close();			/* returns 0 or -1 */
IMPORT INT printf();			/* returns # of characters output */
IMPORT VOID exit();			/* performs MS/DOS exit */

IMPORT INT fd;				/* (optional) output file descriptor */

/************************************************************************
 * VOID error(p1, p2)
 *     TEXT *p1;		pointer to (optional) message 1
 *     TEXT *p2;		pointer to (optional) message 2
 *
 *     error will output the string "mon: ", the message strings at p1
 * and p2, and a newline. An exit will then be performed.
 *
 * Notes: Either p1 or p2 may be null.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID error(p1, p2)
    TEXT *p1;				/* pointer to (optional) message 1 */
    TEXT *p2;				/* pointer to (optional) message 2 */
    {

    /* output error message to stdout
     */
    (VOID)printf("mon: %s%s\n", p1 ? p1 : "", p2 ? p2 : "");
    
    /* if an output file was specified, close it
     */
    if (0 <= fd)
	(VOID)close(fd);
    exit();
    }

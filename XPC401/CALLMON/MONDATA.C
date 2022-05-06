/************************************************************************
 * MONDATA - CALLMON external data module
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *     This module contains external data declarations for the XPC Driver.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/86   4.00A   SB    Initial Draft
 *
 ************************************************************************/
#include "stddef.h"

/* application interface data
 */
UWORD app_sp;				/* driver stack pointer */
UWORD app_ss;				/* driver stack segment */
UWORD asave_sp;				/* saves application stack pointer */
UWORD asave_ss;				/* saves application stack segment */
UWORD in_application;			/* flag used to prevent reentrancy */
UWORD param_block[15];			/* saves application parameter block */

/* driver load-time data
 */
UWORD app_ivec;				/* calling interrupt vector */
UWORD aivec_cs;				/* saves old code segment */
UWORD aivec_ip;				/* saves old instruction pointer */

UWORD xpc_cs = NULL;    		/* xpc driver code segment */
UWORD nbr_disables = 0;
INT monfd = -1;
TEXT monname[82];
BOOL verbose = NO;
BOOL terse = NO;
BOOL packetonly = NO;
BOOL waitclose = NO;
BOOL inoutstatus = NO;
/************************************************************************
 * mondata.c - X.PC line monitor external data module
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    This module contains defining instances of all external data used
 * by the X.PC line monitor.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KJB   Initial Draft
 *
 ************************************************************************/
#include "stddef.h"
#include "xpc.h"
#include "device.h"
#include "iocomm.h"
#include "timer.h"
#include "param.h"

/* comm info structure (uninitialized)
 */
COMMINFO comm_info;
COMMINFO comm_info2;

/* device status structures (initialized)
 */
DEVSTAT dev_stat = {1, 0, 0, 0, 1, 0, 0, 0, 0};
DEVSTAT dev_stat2 = {1, 0, 0, 0, 2, 0, 0, 0, 0};

/* default parameters structure (initialized)
 */
PORTPARAMS default_params = {(WORD)DEFAULT_BAUD,
			     (WORD)DEFAULT_DATA,
			     (WORD)DEFAULT_PARITY,
			     (WORD)DEFAULT_STOP,
			     (WORD)XON_OFF,
			     (WORD)DEFAULT_MODE};

/* port parameters structure (uninitialized)
 */
PORTPARAMS port_params;

/* timer lists (initialized)
 */
TIMER *timer_lists[TIMER_TYPES] = {NULLTIM,
				   NULLTIM,
				   NULLTIM};

/* define miscellaneous
 */
INT dsoflag = 0;			/* disable screen output flag */
INT fd = -1;				/* (optional) output file descriptor */
UBYTE mask = 0xff;			/* data byte mask */
UBYTE xpc_stack[XPC_STACK_LEN];		/* xpc internal stack */
UWORD free_count = 0;			/* buflet free list count */
UWORD tick_count = 0;			/* timer interrupt counter */
UWORD xpc_cs = 0;			/* xpc driver code segment */
WORD nbr_disables = 0;			/* interrupt disable nest count */


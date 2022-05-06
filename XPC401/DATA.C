/************************************************************************
 * data.c - XPC external data module
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
 * 03/04/87   4.00    All   Initial Draft
 *
 ************************************************************************/
#include "stddef.h"
#include "xpc.h"
#include "device.h"
#include "iocomm.h"
#include "param.h"
#include "timer.h"

/* channel and logical channel info structure arrays and pointers
 * (uninitialized)
 */
CHNLINFO cis[NUM_CHNL];
CHNLINFO *ap_pcis;
LCIINFO lcis[NUM_CHNL];

/* comm info structure (uninitialized)
 */
COMMINFO comm_info;
UWORD xmt_intcnt = 0;			/* Number of ticks since last
					 * transmit interrupt.
					 */

UBYTE prev_mdmstatus = 0;		/* previous modem status */

/* device status structure (uninitialized)
 */
DEVSTAT dev_stat;

/* port parameters current (uninitialized) and default (initialized) structures
 */
PORTPARAMS default_params = {(WORD)DEFAULT_BAUD,
			     (WORD)DEFAULT_DATA,
			     (WORD)DEFAULT_PARITY,
			     (WORD)DEFAULT_STOP,
			     (WORD)DEFAULT_XON,
			     (WORD)DEFAULT_MODE};
PORTPARAMS port_params;

/* declare the timer list array (initialized)
 */
TIMER *timer_lists[TIMER_TYPES] = {NULLTIM,
				   NULLTIM, 
				   NULLTIM};

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

/* external data used by the PAD interface
 */
BUFLET *pecho = NULLBUF;		/* pointer to echo buffer */
BUFLET *ptrans = NULLBUF;		/* pointer to assembly xmit packet */
INT numecho = 0;			/* number of echo bytes */

#ifdef DEBUG
INT num_readbytes = 0;			/* number of bytes read */
INT num_written = 0;			/* number of characters written */
#endif
            
INT write_error = 0;			/* error occurred during write */
UWORD numprc = NULLUWORD;		/* number of characters written */
UWORD numreq = NULLUWORD;		/* number of characters requested */
UWORD trans_idx = NULLUWORD;		/* index into xmit packet */
WORD pend_inc_calls = NULLWORD;		/* number of pending incoming calls */
WORD len_mult = 1;			/* multiplication variable used
					 * for T25 and T27 timers.
					 */
/* external link data
 */
BUFLET *frame_pntr = 0;			/* pointer to frame */
BOOL in_a_frame = NO;			/* frame pointer is currently in
					 * a frame.
					 */
BOOL restart_flag = NO;			/* restart is being performed by
					 * the link.
					 */
BYTE restart_clear_code = 0;		/* restart clear code */
UWORD num_chars_inframe = 0;		/* the number of characters int
					 * the current input packet.
					 */
UBYTE linkchnl = 0;			/* the logical channel number */
UBYTE pkttype = 0;			/* the type of input packet */
UBYTE restart_devmode = 0;		/* restart cause */
BOOL app_active = NO;			/* the application is active */
BOOL clear_pad = NO;			/* the application should clear 
					 * the pad before exiting.
					 */

/* miscellaneous data
 */
BOOL first_time = YES;			/* first time driver called if true */
UBYTE app_chnl = NULLUBYTE;		/* application channel number */
UBYTE log_chnl = NULLUBYTE;		/* logical channel number */
UBYTE xpc_stack[XPC_STACK_LEN + 2];	/* xpc timer internal stack */
UWORD free_count = NULLUWORD;		/* buflet free list count */
UWORD rnr_buflets_req = BUFLET_LOWATER; /* number of buflets required 
					 * before RNRing.
					 */
UWORD program_error = NULLUWORD;	/* internal program error flag */
UWORD xpc_cs = NULLUWORD;		/* xpc driver code segment */
WORD device_nbr = NULLWORD;		/* logical device number */
WORD got_ints = NULLWORD;		/* iocomm diagnostic */
WORD nbr_disables = NULLWORD;		/* interrupt disable nest count */
BOOL xpc_timer_active = NO;		/* Should X.PC timer interrupt
					 * handling pass control to LINK?
					 */
BOOL use_timer_int = YES;		/* Should Device Reset set and
					 * reset the timer interrupt
					 * vectors?
					 */



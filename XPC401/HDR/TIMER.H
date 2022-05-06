/************************************************************************
 * timer.h - Timer Definitions
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    This file contains the definition of the timer structure, along
 * with constants and macros used by the X.PC timer mechanisms.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KJB   Initial Draft
 *
 ************************************************************************/

/* define timer types
 */
#define SIXTH_SEC_TIMER	(UBYTE)0	/* 1/6 second timer */
#define ONE_SEC_TIMER	(UBYTE)1	/* 1 second timer */
#define TEN_SEC_TIMER	(UBYTE)2	/* 10 second timer */
#define TIMER_TYPES	3		/* number of timer types */

/* define timer classes (note all bytes in high nibble)
 */
#define TIM_ONESEC	(UBYTE)0x00	/* generic one second timer */
#define TIM_T10		(UBYTE)0x10	/* T10 timer */
#define TIM_T11		(UBYTE)0x20	/* T11 timer */
#define TIM_T12		(UBYTE)0x30	/* T12 timer */
#define TIM_T13		(UBYTE)0x40	/* T13 timer */
#define TIM_T15		(UBYTE)0x60	/* T15 timer */
#define TIM_T17		(UBYTE)0x80	/* T17 timer */
#define TIM_T20		(UBYTE)0x10	/* Restart Request Timer */
#define TIM_T21		(UBYTE)0x20	/* T21 timer */
#define TIM_T22		(UBYTE)0x30	/* Reset Request Timer */
#define TIM_T23		(UBYTE)0x40	/* T23 timer */
#define TIM_T24		(UBYTE)0x50	/* T24 timer */
#define TIM_T25		(UBYTE)0x60	/* Window Rotation Timer */
#define TIM_T26		(UBYTE)0x70	/* T26 timer */
#define TIM_T27		(UBYTE)0x80	/* Reject Packet Timer */
#define TIM_BALLOUT	(UBYTE)0x90	/* colored ball out timer */
#define TIM_FORWARDING	(UBYTE)0xa0	/* forwarding timeout */
#define TIM_CLRCONFIRM	(UBYTE)0xb0	/* pending clear confirm */
#define TIM_TYMNOACTIV	(UBYTE)0xc0	/* tymnet no activity */
#define TIM_RRCHANZERO	(UBYTE)0xd0	/* rr channel zero */
#define TIM_RRRNR	(UBYTE)0xe0	/* rr repeat after rnr */
#define TIM_USER	(UBYTE)0xf0	/* generic user timer */
#define TIM_XOFFRECVD	(UBYTE)0x10	/* xoff received timer */
#define TIM_BREAK	(UBYTE)0x20	/* break timer */
#define TIMER_CLASSES	16		/* number of timer classes */

/* defines for timer lengths
 */
#define LEN_ONESEC	(WORD)6		/* generic one second timer */
#define LEN_T10		(WORD)6		/* T10 timer */
#define LEN_T11		(WORD)18	/* T11 timer */
#define LEN_T12		(WORD)6 	/* T12 timer */
#define LEN_T13		(WORD)6 	/* T13 timer */
#define LEN_T15		(WORD)4 	/* T15 timer */
#define LEN_T17		(WORD)4 	/* T17 timer */
#define LEN_T20		(WORD)180	/* Restart request timer */
#define LEN_INITT20	(WORD)15	/* Initial restart request timer */
#define LEN_T21		(WORD)20	/* T21 timer */
#define LEN_T22		(WORD)18	/* Reset Request timer */
#define LEN_T23		(WORD)18	/* T23 timer */
#define LEN_T24		(WORD)6 	/* T24 timer */
#define LEN_T25		(WORD)8 	/* Window Rotation timer */
#define LEN_T26		(WORD)18	/* T26 timer */
#define LEN_T27		(WORD)8 	/* Reject Packet timer */
#define LEN_BALLOUT	(WORD)8 	/* colored ball out timer */
#define LEN_CLRCONFIRM	(WORD)0		/* pending clear confirm */
#define LEN_TYMNOACTIV	(WORD)6		/* tymnet no activity */
#define LEN_RRCHANZERO	(WORD)32	/* rr channel zero */
#define LEN_RRRNR	(WORD)4		/* rr repeat after rnr */
#define LEN_USER	(WORD)0		/* generic user timer */
#define LEN_XOFFRECVD	(WORD)6		/* xoff received timer */
 
/* Counts which are used by the link timers.
 * These constants define the number of times a timer 
 * is called.
 */
#define INIT_R15_COUNT  (UBYTE)4	/* number of times t15 timer is
					 * tried.
					 */
#define INIT_R17_COUNT  (UBYTE)4	/* number of times t17 timer is
					 * tried.
					 */
#define INIT_R20_COUNT	(UBYTE)1	/* number of times restart confirm
					 * timer is  tried.
					 */
#define INIT_R22_COUNT	(UBYTE)1	/* number of times reset confirm
					 * timer is  tried.
					 */
					
#define INIT_R23_COUNT	(UBYTE)1	/* number of time t23 timer is
				         * tried.
				         */
#define INIT_R25_COUNT	(UBYTE)4	/* number of times window rotation
					 * timer is tried.
					 */
#define INIT_R27_COUNT	(UBYTE)4	/* number of times reject timer is
					 * tried.
					 */


/* define miscellaneous (sizes, etc.)
 */
#define NULLTIM		(TIMER *)0	/* null timer pointer */
#define TEN_SECONDS	(WORD)10	/* 1 second ticks in 10 seconds */
#define TICKS_SIXTH_SEC	(WORD)3		/* 1/18.2 second ticks in 1/6 second */
#define TICKS_ONE_SEC	(WORD)6		/* 1/6 second ticks in 1 second */
#define TICKS_TEN_SEC	(WORD)60	/* 1/6 second ticks in 10 seconds */
#define TIMER_ARRAY_SIZ	(TIMER_CLASSES * NUM_CHNL) /* timer array size */
#define TIMER_SIZ	sizeof(TIMER)	/* timer entry size (bytes) */

#define TIMER_EVENT	1		/* timer event */
#define MODEM_EVENT 	2		/* modem event */
#define CHECK_EVENT	3		/* checkpointer event */
#define UPDATE_ON	1		/* update turned on */
#define INTERRUPT_ON	2		/* interrupt turned on */
 
/* define the TIMER structure
 */
typedef struct timer 
    {
    struct timer *timnext;		/* pointer to next timer */
    struct timer *timprev;		/* pointer to previous timer */
    UBYTE timtype;			/* timer type */
    UBYTE timid;			/* timer id (class | channel) */
    WORD timlength;			/* timer length (ticks) */
    VOID (*timfunc)();			/* pointer to timeout function */
    } TIMER;

/* define macros
 */
#define idtochnl(i)	(UBYTE)((i) & 0x0f)	/* timer index to channel */
#define idtoclass(i)	(UBYTE)((i) & 0xf0)	/* timer index to class */



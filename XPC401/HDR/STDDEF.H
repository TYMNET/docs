/************************************************************************
 * stddef.h - Standard Definitions
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    stddef.h contains defines for the storage classes,
 *    and data types which are used throughout the XPC Driver.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KS    Initial Draft
 *
 ************************************************************************/
/* storage classes
 */
#define FAST		    register	/* register class (if supported) */
#define IMPORT		    extern	/* used to refernce externals */
#define INTERN		    static	/* static within program block */
#define LOCAL		    static	/* static outside program block */

/* pseudotypes
 */
typedef char TEXT, BYTE;		/* arbitrary text */
typedef unsigned char UBYTE;		/* arbitrary bytes */
typedef short WORD, SHORT, BOOL;	/* miscellaneous counters, boolean */
typedef unsigned short UWORD, BYTES;	/* counter, byte count */
typedef long LONG;			/* arbitrary long */
typedef unsigned long ULONG;		/* arbitrary unsigned long */
typedef float FLOAT;			/* single float */
typedef double DOUBLE;			/* long float */
typedef int INT;			/* arbitrary int */

/* directive to quiet lint
 */
#ifdef _lint
#define VOID void			/* used while lint'ing */
#else
typedef int VOID;			/* MSC 4.0 does not support void */
#endif
 
/* xpc specific pseudotypes
 */
typedef unsigned short CRC;		/* data CRC value */
 
/* widely used params
 */
#define YES		    1		/* boolean true */
#define NO		    0		/* boolean false */
#define SUCCESS		    0		/* when error is indicated negative */
#define NULL		    0		/* null value */

/* macros
 */
#define FOREVER		    for (;;)
#define abs(x)		    ((x) < 0 ? -(x) : (x))
#define max(a,b)	    ((a) > (b) ? (a) : (b))
#define min(a,b)	    ((a) <= (b) ? (a) : (b))

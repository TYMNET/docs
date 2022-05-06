/************************************************************************
 * mon.h - X.PC line monitor header file
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    This module contains definitions used by the X.PC line monitor
 * program.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KJB   Initial Draft
 *
 ************************************************************************/

/* define port indexes
 */
#define RPORT1		1		/* received - port 1 */
#define RPORT2		2		/* received - port 2 */

/* define data codes
 */
#define RCMODE		0		/* data received in character mode */
#define RPMODE		1		/* data received in packed mode */
#define RPDROP		2		/* data dropped in packet mode */

/* define packet error codes
 */
#define MON_CRC1_ERR	0		/* crc 1 error */
#define MON_CRC2_ERR	1		/* crc 2 error */
#define MON_GFI_ERR	3		/* invalid gfi error */
#define MON_CHNL_ERR	4		/* invalid channel number error */
#define MON_QBIT_ERR	5		/* invalid qbit function code */

/* define miscellaneous
 */
#define BUF_POOL_SIZE	(BYTES)43200	/* buflet pool size in bytes */
#define DATA_ENTRIES	800		/* number of data entries supported */
#define DEF_STATE	PKT_STATE	/* default monitor mode */
#define NULLDATA	(DATA *)0	/* null data pointer */

/* define the data entry structure
 */
typedef struct data
    {
    struct data *next;			/* pointer to next entry */
    UBYTE port;				/* received port number */
    UBYTE code;				/* data code */
    UWORD time;				/* time received */
    BUFLET *pbuf;			/* pointer to buflet chain */
    } DATA;

/* define the data queue structure
 */
typedef struct {
    DATA *head;				/* pointer to queue head */
    DATA *tail;				/* pointer to queue tail */
    } DATAQ;

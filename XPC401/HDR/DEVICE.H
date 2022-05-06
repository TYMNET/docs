/************************************************************************
 * device.h - Device Definitions
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    device.h contains the definition of the device status
 *    structure, and the definition of the port parameter structure.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KS    Initial Draft
 *
 ************************************************************************/
/* defines of the communication port addresses
 */
#define NO_PORT		    0		/* no communications port assigned */
#define COM1		    1		/* communications port 1 assigned */
#define COM2		    2		/* communications port 2 assigned */
 
/* device status structure - defines the current status of a device
 */
typedef struct		/* DEVSTAT structure */
    {
    WORD devstate;			/* current state of device */
    WORD version;			/* version of xpc software */
    WORD revision;			/* xpc software revision number */
    WORD padtype;			/* type of PAD installed */
    WORD portaddr;			/* port address */
    WORD pvcchnls;			/* number of pvc channels */
    WORD inchnls;			/* number of incoming call channels */
    WORD twaychnls;			/* number of two way call channels */
    WORD outchnls;			/* number of outgoing call channels */
    } DEVSTAT;
 
/* port_parameter structure - defines a port
 */
typedef struct		/* PORTPARAMS structure */
    {
    WORD baudrate;			/* baud rate */
    WORD databits;			/* number of data bits */
    WORD parity;			/* parity */
    WORD stopbits;			/* number of stop bits */
    WORD xonxoff;			/* xon/xoff enabled or disabled  */
    WORD dxemode;			/* dte or dce */
    } PORTPARAMS;

/* define macros
 */

/* valid_port_addr - validates port address
 */ 
#define valid_port_addr(c)		((c) >= NO_PORT && (c) >= COM2)


/************************************************************************
 * mon.c - X.PC line monitor
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    This module contains the entry point and support functions for the
 * X.PC line monitor program.
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
#include "mon.h"
#include "padprm.h"
#include "param.h"
#include "state.h"

IMPORT INT dsoflag;			/* disable screen output flag */
IMPORT INT fd;				/* (optional) output file descriptor */
IMPORT UBYTE mask;			/* data byte mask */

IMPORT BUFLET *init_buf();		/* initializes buflet pool */
IMPORT BYTES fill();			/* returns its second argument */
IMPORT DATA *get_data();		/* retrieves received data */
IMPORT INT atoi();			/* ascii to integer */
IMPORT INT close();			/* close file */
IMPORT INT creat();			/* create file */
IMPORT INT enable_comm();		/* comm enable (port 1) */
IMPORT INT enable_com2();		/* comm enable (port 2) */
IMPORT INT inchar();			/* input console character */
IMPORT INT init_comm();			/* init comm (port 1) */
IMPORT INT init_com2();			/* init comm (port 2) */
IMPORT VOID chmod();			/* changes monitor mode */
IMPORT VOID disable_comm();		/* disable comm (port 1) */
IMPORT VOID disable_com2();		/* disable comm (port 2) */
IMPORT VOID disable_timer();		/* stops timer interrupt */
IMPORT VOID dump_data();		/* outputs received data */
IMPORT VOID enable_timer();		/* starts timer imterrupt */
IMPORT VOID error();			/* generic error handler */
IMPORT VOID free_data();		/* frees received data */
IMPORT VOID init_data();		/* initializes data pool */
IMPORT VOID save_ds();			/* saves monitor data segment */
IMPORT VOID set_port();			/* initializes port 1 parameters */
IMPORT VOID set_port2();		/* initializes port 2 parameters */
    
IMPORT COMMINFO comm_info;		/* comm info strcuture 1 */
IMPORT COMMINFO comm_info2;		/* comm info strcuture 2 */
IMPORT PORTPARAMS default_params;	/* default port parameters */
IMPORT PORTPARAMS port_params;		/* port parameters */

/************************************************************************
 * LOCAL VOID init(av)
 *     TEXT **av;
 *
 *     init is called by the X.PC line monitor upon startup to parse
 * (optional) command line input. av is the argument vector.
 *
 * Notes: init assumes no spaces between flags and flag values.
 *
 *     In the case of duplicate flags, the last flag parsed wins.
 *
 * Returns: Nothing. An exit will be taken if flag parsing fails or 
 *     an invalid flag value is received.
 *
 ************************************************************************/
LOCAL VOID init(av)
    FAST TEXT **av;			/* argument vector */
    {
    INT baud;				/* saves converted baud rate code */
    TEXT *name = 0;			/* pointer to output file name */
    
    /* parse command line
     */
    while (*++av && **av == '-')
	switch (*++*av)
	    {
	    case 'd':
	    case 'D':
	    
		/* -[d] - disable screen output
		 */
		dsoflag = 1;
		break;    
	    case 'm':
	    case 'M':
	    
		/* -[m] - mask high order bit of data
		 */
		mask = 0x7f;
		break;
	    case 'o':
    	    case 'O':

		/* -[o*] - output to file *. save pointer to filename
		 */
		name = *av + 1;
		break;
	    case 's':
	    case 'S':
		
		/* -[s#] - use baud rate #. convert baud rate; if invalid,
		 * output a message and exit, else install in default
		 * parameters structure
		 */
		baud = atoi(*av + 1);
		if (baud < BAUD_110 || BAUD_9600 < baud)
		    error("illegal baud rate specified", 0);
		default_params.baudrate = baud;
		break;
	    default:

		/* invalid flag; output usage message and exit
		 */
		error("usage: mon -[d m o* s#]", 0);
		break;
	    }
    
    /* if an output file was specified, attempt to create; if create fails,
     * output an error message and exit
     */
    if (name && (fd = creat(name, 0x180)) < 0)
	error("can't create output file ", name);
    }
		
/************************************************************************
 * VOID main(ac, av)
 *     INT ac;			argument count
 *     TEXT **av;		argument vector
 *
 *     This is the entry point for the X.PC line monitor. The monitor is
 * invoked at the MS-DOS command level via:
 * 
 *     mon -[d m o* s#]
 *
 * Flags are:
 *
 *     -[d]  = disable screen output
 *     -[m]  = mask off high order data bits
 *     -[o*] = save output to file *.
 *     -[s#] = set baud rate to code #. Recognized values of # are:
 *
 *             0 = 110 bps
 *             1 = 150 bps
 *             2 = 300 bps
 *             3 = 600 bps
 *             4 = 1200 bps (default)
 *             5 = 2400 bps
 *             6 = 4800 bps
 *             7 = 9600 bps
 *
 * Notes: Currently, the line monitor is initialized to packet mode.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID main(ac, av)
    INT ac;				/* argument count */
    TEXT **av;				/* argument vector */
    {
    DATA *pd;				/* scratch data pointer */
    INT c;				/* saves input character */

    init(av);				/* parse command line */
    save_ds();				/* save data segment */

    /* if buflet pool is successfully allocated and initialized...
     */
    if (init_buf(BUF_POOL_SIZE))
	{
    
	/* initialization...
	 */
	chmod(DEF_STATE);		/* initialize to default mode */
	init_data();			/* initialize data pool */
	(VOID)init_comm();		/* initialize comm port 1 */
	(VOID)init_com2();		/* initialize comm port 2 */
	enable_timer();			/* start timer interrupt */
	set_port(&default_params);	/* set port 1 parameters */
	set_port2(&default_params);	/* set port 2 parameters */
	port_params = default_params;	/* set default port parameters */
	(VOID)enable_comm();		/* enable comm port 1 */
	(VOID)enable_com2();		/* enable comm port 2 */

	/* process input until ESC is typed at the console
	 */
	while ((c = inchar()) != ESCAPE)
	    {
	
	    /* if 'c' ('C') is typed, set monitor to character mode
	     */
	    if (c == 'c' || c == 'C')
		chmod(CHAR_STATE);
	    
	    /* if 'p' ('P') is typed, set monitor to packet mode
	     */
	    else if (c == 'p' || c == 'P')
		chmod(PKT_STATE);
	    
	    /* if data was received, retrieve the data, format and output,
	     * and free the data
	     */
	    if ((pd = get_data()) != NULLDATA)
		{
		dump_data(pd);
		free_data(pd);
		}
	    }
	disable_comm();			/* disable port 1 */
	disable_com2();			/* disable port 2 */
	disable_timer();		/* disable timer interrupt */
	}
	    
    /* allocation and initialization of buflet pool failed; output an error
     * message and exit
     */
    else
	error("can't initialize buflet pool", 0);
    
    /* if an output file was specified, close it
     */
    if (0 <= fd)
	(VOID)close(fd);
    }

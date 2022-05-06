/************************************************************************
 * ai.c - Application Interface Test Program
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    ai.c contains the main application interface test program.
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
#include "ctype.h"
#include "xpc.h"

BOOL chnl_tab[NUM_CHNL];		/* "open" channel table */
BOOL no_display = NO;			/* don't display output */
BOOL ttymode = NO;    			/* tty mode flag */
INT diags_fd = 0;			/* file fd for diags dump */
INT vector = 0x7a;			/* driver interrupt vector */
TEXT *pname;				/* pointer to program name */
UBYTE inbuf[IOSIZ];			/* input buffer */
UBYTE outbuf[IOSIZ];			/* output buffer */
WORD inlen;				/* input length */
WORD outlen;				/* output length */
LONG numread = 0;
LONG numwritten = 0;
IMPORT BOOL getword();			/* solicits word input */
IMPORT INT close();			/* close file */
IMPORT INT inchar();			/* input character */
IMPORT INT gettext();			/* input string */
IMPORT INT open();			/* open file */
IMPORT INT printf();			/* formatted output */
IMPORT INT read();			/* read file */
IMPORT INT sscanf();			/* formatted string conversion */
IMPORT REQ *call();			/* performs interface call */
IMPORT REQ *getreq();			/* allocates request block */
IMPORT VOID error();			/* fatal error routine */
IMPORT VOID freereq();			/* frees request block */
IMPORT VOID get_intvec();		/* reads interrupt vector */
IMPORT VOID help();			/* help routine */
IMPORT VOID close_chnl();		/* "close" channel */
IMPORT VOID open_chnl();		/* "open" channel */
IMPORT VOID reset_chnl();		/* reset channels */
IMPORT WORD instat();			/* read input status */
IMPORT WORD outstat();			/* read output status */
LONG ireq = 0;
LONG iprc = 0;
LONG oreq = 0;
LONG oprc = 0;
LONG loprc = 0;
LONG liprc = 0;
LONG poprc = 0;
LONG piprc = 0;

/* send_file - transmit a file
 */
LOCAL VOID send_file()
    {
    INT fd;				/* file descriptor */
    INT inchnl;				/* input channel counter */
    INT outchnl;			/* output channel counter */
    TEXT name[20];			/* saves filename */
    
    (VOID)printf("\nfile? ");		/* prompt for file */
    if (0 < gettext(name, NO))		/* if ESC not hit */

	/* if file successfully opened...
	 */
	if (0 <= (fd = open(name, 0x8000, 0)))
	    {
	    inlen = IOSIZ;		/* used by call 16 */
	
	    /* while file read successful...
	     */
	    while (0 < (outlen = read(fd, outbuf, IOSIZ)))
		{
	    
		/* output data read on each "open" channel until all
		 * channels successful
		 */
		inchnl = outchnl = 0;
		do  {
		
		    /* if channel "open"...
		     */
		    if (chnl_tab[outchnl])	
			{
		    
			/* if output status ok, output data and bump output
			 * channel number
			 */
			if (outstat(outchnl) != 2)
			    freereq(call(17, NULLREQ, outchnl++));
			}
		    else
			++outchnl;	/* skip to next output channel */
			
		    /* attempt to read current input channel
		     */
		    if (chnl_tab[inchnl] && 0 < instat(inchnl))
			freereq(call(16, NULLREQ, inchnl));
		    
		    /* since this loop is performed until output is successful
		    * on all "open" channels, input channel wraps
		    */
		    inchnl = ++inchnl % NUM_CHNL;
		    
		    /* check for user termination
		    */
		    if (inchar() == ESCAPE)
			{
			(VOID)close(fd);
			return;
			}
		    } while (outchnl < NUM_CHNL);
		}
	    if (no_display)
		{
		outlen = IOSIZ;
		outchnl = 0;
		do  {
	
		    /* if channel "open"...
		     */
		    if (chnl_tab[outchnl])	
			{
	    
			fill(outbuf, IOSIZ, 0);
		    
			/* if output status ok, output data and bump output
			 * channel number
			 */
			if (outstat(outchnl) != 2)
			    freereq(call(17, NULLREQ, outchnl++));
			}
		    else		/* else channel "closed"; */
			++outchnl;	/* skip to next output channel */
			
		    } while (outchnl < NUM_CHNL);
            
		outlen = 0;
		}
	    
	    (VOID)close(fd);
	    }
	else				/* can't open specified file */
	    (VOID)printf("can't open \"%s\"\n", name);
    else				/* ESC hit */
	putchar('\n');
    }

/* flush_ifile - flush input file 
 */
LOCAL VOID flush_ifile()
    {
    INT inchnl;				/* input channel counter */
    
    ttymode = YES;
    inlen = IOSIZ;		/* used by call 16 */
	
    /* while file read successful...
     */
    FOREVER
	{
	inchnl = 0;
	do  {
		
			
	    /* attempt to read current input channel
	     */
	    if (chnl_tab[inchnl] && 0 < instat(inchnl))
		{
		freereq(call(30, NULLREQ, inchnl));
		freereq(call(16, NULLREQ, inchnl));
		}
		    
	    /* since this loop is performed until output is successful
	     * on all "open" channels, input channel wraps
	     */
	    inchnl = ++inchnl % NUM_CHNL;
	
	    /* check for user termination
	     */
	    if (inchar() == ESCAPE)
		return;
	    } while (inchnl < NUM_CHNL);
	}
    }
    
/* flush_ofile - Flush Output file 
 */
LOCAL VOID flush_ofile()
    {
    INT fd;				/* file descriptor */
    INT inchnl;				/* input channel counter */
    INT outchnl;			/* output channel counter */
    TEXT name[20];			/* saves filename */
    
    (VOID)printf("\nfile? ");		/* prompt for file */
    if (0 < gettext(name, NO))		/* if ESC not hit */

	/* if file successfully opened...
	 */
	if (0 <= (fd = open(name, 0x8000, 0)))
	    {
	    inlen = IOSIZ;		/* used by call 16 */
	
	    /* while file read successful...
	     */
	    while (0 < (outlen = read(fd, outbuf, IOSIZ)))
		{
	    
		/* output data read on each "open" channel until all
		 * channels successful
		 */
		inchnl = outchnl = 0;
		do  {
		
		    /* if channel "open"...
		     */
		    if (chnl_tab[outchnl])	
			{
		    
			/* if output status ok, output data and bump output
			 * channel number
			 */
			if (outstat(outchnl) != 2)
			    {
			    freereq(call(17, NULLREQ, outchnl));
			    freereq(call(31, NULLREQ, outchnl++));
			    }
			}
		    else		/* else channel "closed"; */
			++outchnl;	/* skip to next output channel */
			
		    /* check for user termination
		     */
		    if (inchar() == ESCAPE)
			{
			(VOID)close(fd);
			return;
			}
		    } while (outchnl < NUM_CHNL);
		}
	    (VOID)close(fd);
	    }
	else				/* can't open specified file */
	    (VOID)printf("can't open \"%s\"\n", name);
    else				/* ESC hit */
	putchar('\n');
    }

/* tty - tty mode
 */
LOCAL VOID tty(param)
    UBYTE param;
    {
    INT c;				/* saves input character */
    INT inchnl;				/* input channel counter */
    INT outchnl;			/* output channel counter */

    /* initialization...
     */
    (VOID)printf("\nstart tty mode...\n");
    ttymode = YES;
    inlen = IOSIZ;
    outlen = 0;
    
    /* until ESC is hit...
     */
    do  {
    
	/* while input is available from the console
	 */
	while ((c = inchar()) != 0 && c != ESCAPE)
	
	    /* if ctrl-f is hit, send file
	     */
	    if (c == 6)
		{
		if (param == 1)
		    flush_ofile();
		else
		    send_file();
		outlen = 0;
		}
	    else			/* else add char to output buffer */
		{
		outbuf[outlen++] = c;
		if (IOSIZ <= outlen)	/* wrap should "never happen" */
		    outlen = 0;
		}
	    
	/* output data on each "open" channel until all channels successful
	 */
	inchnl = outchnl = 0;
	do  {
	    if (outlen && chnl_tab[outchnl])
	    
		/* if output status indicates flow control, driver may be
		 * hung up; check the console to see if ESC was hit
		 */
		if (outstat(outchnl) == 2)
		    c = inchar();
		
		/* else output data and bump output channel number
		 */
		else
		    freereq(call(17, NULLREQ, outchnl++));
		
	    /* else no input was read or channel "close"; skip to next
	     * output channel
	     */
	    else
		++outchnl;
	    
	    /* if current input channel "open" and data is waiting to be
	     * input, read it
	     */
	    if (chnl_tab[inchnl] && 0 < instat(inchnl))
		freereq(call(16, NULLREQ, inchnl));
	    
	    /* since this loop is performed until output is successful
	     * on all "open" channels, input channel wraps
	     */
	    inchnl = ++inchnl % NUM_CHNL;
	    } while (c != ESCAPE && outchnl < NUM_CHNL);
	outlen = 0;
	} while (c != ESCAPE);
    ttymode = NO;
    (VOID)printf("\n...stop tty mode\n\n");
    }

VOID view_update()
    {
    INT c;
    
    while (c != ESCAPE)
	{
	disp_event(0, 1);
	while ((c = inchar()) != 0 && c != ESCAPE)
	    ;
	}
    }

/* getfid - prompt for interface function index
 */
LOCAL WORD getfid()
    {
    INT x;				/* saves return from gettext */
    TEXT buf[20];			/* input buffer */
    WORD fid;				/* function index (returned) */

    putchar('\n');
    FOREVER
	{
	(VOID)printf("function ? ");	/* prompt for function index */
	if ((x = gettext(buf, NO)) < 0)	/* if ESC hit */
	    return ((WORD)x);		/* return negative value */
	if (x)				/* if anything input */
	    {
	    no_display = NO;
	    buf[0] = tolower(buf[0]);	/* convert to lowercase */
	    if (buf[0] == '?')		/* help key hit */
		help();
	    else if (buf[0] == 'c')	/* "close" channel */
		close_chnl();
	    else if (buf[0] == 'o')	/* "open" channel */
		open_chnl();
	    else if (buf[0] == 't')	/* tty mode */
		tty(0);
	    else if (buf[0] == 'u')	/*  tty mode - no display */
	        {
		no_display = YES;
		numread = 0;
		numwritten = 0;
		tty(0);
		}
	    else if (buf[0] == 'l')	/* flush output */
		tty(1);
	    else if (buf[0] == 'm')	/* flush input */
		flush_ifile();
	    else if (buf[0] == 'v')
		view_update();
	    else			/* break loop */
		break;
	    }
	}
    fid = 99;				/* initialize to illegal value */
    (VOID)sscanf(buf, "%d", &fid);	/* convert function index */
    return (abs(fid));			/* return non-negative index */
    }

/* init - static initialization routine
 */
LOCAL VOID init(av)
    FAST TEXT **av;			/* argument vector */
    {
    UWORD cs;				/* code segment */
    UWORD ip;				/* instruction pointer */

    pname = *av;			/* save program name */
	    
    /* parse input parameters
     */
    while (*++av && **av == '-')
	switch (*++*av)
	    {
	    case 'x':
	    case 'X':
    
		/* -[x#] - use interrupt vector #
		 */
		(VOID)sscanf(*av + 1, "%x", &vector);
		if (vector < 0 || 0xff < vector)
		    error("invalid vector specified");
		break;
	    default:
		error("usage: ai [-x<vector>]");
		break;
	    }
    
    /* make sure driver is running
     */
    get_intvec(vector, &cs, &ip);
    if (!cs && !ip)
	error("driver not loaded");
    }
    
/************************************************************************
 * VOID main(ac, av)
 *     INT ac;			argument count
 *     TEXT **av;		argument vector
 *
 *     This is the entry point for the X.PC application interface test
 * program. The program is invoked at the MS-DOS command level via:
 * 
 *     ai -[x#]
 *
 * Flags are:
 *
 *     -[x#] = use software interrupt vector number #. Legal values of #
 *             are [0 - 0xff]. The default is 0x7a.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID main(ac, av)
    INT ac;				/* argument count */
    TEXT **av;				/* argument vector */
    {
    INT chnl;				/* channel counter */
    REQ *pr;				/* pointer to request block */
    WORD fid;				/* function index */

    init(av);				/* parse command line */
    reset_chnl();			/* initialize channel table */
    pr = getreq();			/* allocate request block */

    /* until ESC hit...
     */
    if ((diags_fd = creat("diag.sav", 0x0180)) < 0)
        error("can't create output file");
    while (0 <= (fid = getfid()))
    
	/* for each channel...
	 */
	for (chnl = 0; chnl < NUM_CHNL; ++chnl)
	    if (chnl_tab[chnl])		/* if channel "open" */
		{
	
		/* call requested function. if this function ignores the
		 * channel number, break loop
		 */
		(VOID)call(fid, pr, chnl);
		if (ignore_chnl(fid))
		    break;
		}
    ttymode = YES;			/* silences reset device call */
    freereq(call(0, pr, 0));		/* reset device */
    close(diags_fd);
    }

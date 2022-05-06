/************************************************************************
 * aifunc.c - AI Process Application Function Calls
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    aifunc.c contains the source modules which are called to 
 *    test specific XPC driver application functions.
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
#include "device.h"
#include "xpc.h"
#include "dos.h"

#define DIAGS		500		/* diagnostic array size */

IMPORT BOOL gethword();			/* input word in hex */
IMPORT BOOL getword();			/* input word in decimal */
IMPORT BYTES fill();			/* fill buffer with character */
IMPORT INT gettext();			/* input text string */
IMPORT INT inchar();			/* input character */
IMPORT INT printf();			/* formatted output */
IMPORT VOID cout();			/* formatted character output */
IMPORT VOID reset_chnl();		/* resets internal channel table */
IMPORT VOID intfunc();
    
IMPORT BOOL ttymode;			/* tty mode flag */
IMPORT BOOL no_display;			/* don't display */
IMPORT INT diags_fd;			/* diags fd*/
IMPORT LONG ireq;
IMPORT LONG iprc;
IMPORT LONG oreq;
IMPORT LONG oprc;
IMPORT LONG piprc;
IMPORT LONG poprc;
IMPORT LONG loprc;
IMPORT LONG liprc;

IMPORT UBYTE inbuf[IOSIZ];		/* driver input buffer */
IMPORT UBYTE outbuf[IOSIZ];		/* driver output buffer */
IMPORT WORD inlen;			/* driver input length */
IMPORT WORD outlen;			/* driver output length */

LOCAL DEVSTAT devstat;			/* device status structure */
LOCAL PORTPARAMS portparams;		/* port parameters structure */
LOCAL WORD diags[DIAGS];		/* diagnostic array */
LOCAL WORD link_array[6];		/* link statistics array */
IMPORT LONG numwritten;			/* number of characters written */
IMPORT LONG numread;			/* number read */
/* ndisplay - process input data with no display
 */
LOCAL VOID ndisplay(p, nbytes)
    FAST UBYTE *p;			/* pointer to output text */
    FAST nbytes;			/* number if bytes to display */
    {
 
    numread = numread + nbytes;

    /* if not in tty mode, output byte count
     */
	    
    if (!ttymode)
	(VOID)printf("\t%d byte%s read: ", nbytes, nbytes == 1 ? "" : "s");
    
    /* output bytes, formatted
     */
    while (0 <= --nbytes)
	{
	if (*p++ == 0)
	    {
	    printf("Number of characters read = %ld\n", numread);
	    printf("Number of characters written = %ld\n", numwritten);
	    break;
	    }
	}
    }

/* display - display nbytes at p, formatted
 */
LOCAL VOID display(p, nbytes)
    FAST UBYTE *p;			/* pointer to output text */
    FAST nbytes;			/* number if bytes to display */
    {

    /* if not in tty mode, output byte count
     */
    if (!ttymode)
	(VOID)printf("\t%d byte%s read: ", nbytes, nbytes == 1 ? "" : "s");
    
    /* output bytes, formatted
     */
    while (0 <= --nbytes)
	cout(*p++);
    if (!ttymode)
	putchar('\n');			/* trailing null */
    }

/* clear_device - application function #0
 */
LOCAL BOOL clear_device(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {

    /* if second call, and call to driver successful, reset channel table
     * (first call is ignored; clear_device requires no parameters)
     */
    if (!flag && !*pr->scode)
	reset_chnl();
    return (YES);
    }

/* read_device_status - application function #1
 */
LOCAL BOOL read_device_status(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {

    if (flag)				/* first call */
	{
    
	/* initialize device status structure and set request block
	 * parameter 1
	 */
	(VOID)fill((UBYTE *)&devstat, sizeof(devstat), FILLCHAR);
	pr->par1 = (WORD *)&devstat;
	}
    
    /* second call; output device status entries
     */
    else
	{
	(VOID)printf("\tdevstate:  %d\n", devstat.devstate);
	(VOID)printf("\tversion:   %d\n", devstat.version);
	(VOID)printf("\trevision:  %d\n", devstat.revision);
	(VOID)printf("\tpadtype:   %d\n", devstat.padtype);
	(VOID)printf("\tportaddr:  %d\n", devstat.portaddr);
	(VOID)printf("\tpvcchnls:  %d\n", devstat.pvcchnls);
	(VOID)printf("\tinchnls:   %d\n", devstat.inchnls);
	(VOID)printf("\ttwaychnls: %d\n", devstat.twaychnls);
	(VOID)printf("\toutchnls:  %d\n", devstat.outchnls);
	}
    return (YES);
    }

/* device_reset - application function #2
 */
LOCAL BOOL device_reset(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {

    if (flag)				/* first call */
	{
	
	/* prompt for port address and set request block parameter 1
	 */
	if (!getword("portaddr", &devstat.portaddr))
	    return (NO);		/* ESC hit */
	pr->par1 = (WORD *)&devstat;
	}
    
    /* second call ignored; driver provides no output parameters
     */
    return (YES);
    }

/* set_character_mode - application function #3
 */
LOCAL BOOL set_character_mode(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {

    /* if second call, and call to driver successful, reset channel table
     * (first call is ignored; set_character_mode requires no parameters)
     */
    if (!flag && !*pr->scode)
	reset_chnl();
    return (YES);
    }

/* set_port_parameters - application function #5
 */
LOCAL BOOL set_port_parameters(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {

    if (flag)				/* first call */
	{
	
	/* prompt for port parameters and set request block parameter 1
	 */
	if (!getword("baudrate", &portparams.baudrate) ||
	    !getword("databits", &portparams.databits) ||
	    !getword("parity", &portparams.parity) ||
	    !getword("stopbits", &portparams.stopbits) ||
	    !getword("xonxoff", &portparams.xonxoff) ||
	    !getword("dxemode", &portparams.dxemode))
		return (NO);		/* ESC hit */
	pr->par1 = (WORD *)&portparams;
	}

    /* second call ignored; driver provides no output parameters
     */
    return (YES);
    }

/* read_port_parameters - application function #6
 */
LOCAL BOOL read_port_parameters(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {

    if (flag)				/* first call */
	{
	
	/* clear port parameters structure and initialize request block
	 * parameter 1
	 */
	(VOID)fill((UBYTE *)&portparams, sizeof(portparams), FILLCHAR);
	pr->par1 = (WORD *)&portparams;
	}
    
    /* second call; output port parameters
     */
    else
	{
	(VOID)printf("\tbaudrate: %d\n", portparams.baudrate);
	(VOID)printf("\tdatabits: %d\n", portparams.databits);
	(VOID)printf("\tparity:   %d\n", portparams.parity);
	(VOID)printf("\tstopbits: %d\n", portparams.stopbits);
	(VOID)printf("\txonxoff:  %d\n", portparams.xonxoff);
	(VOID)printf("\tdxemode:  %d\n", portparams.dxemode);
	}
    return (YES);
    }

/* test_dsr - application function #11
 */
LOCAL BOOL test_dsr(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {

    /* if second call, output dsr state (first call is ignored; test_dsr
     * requires no parameters)
     */
    if (!flag)
	(VOID)printf("\tdsr state: %d\n", *pr->par1);
    return (YES);
    }
    
/* test_dcd - application function #12
 */
LOCAL BOOL test_dcd(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {

    /* if second call, output dcd state (first call is ignored; test_dcd
     * requires no parameters)
     */
    if (!flag)
	(VOID)printf("\tdcd state: %d\n", *pr->par1);
    return (YES);
    }

/* test_ring_indicator - application function #13
 */
LOCAL BOOL test_ring_indicator(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {
		    
    /* if second call, output ri state (first call is ignored;
     * test_ring_indicator requires no parameters)
     */
    if (!flag)
	(VOID)printf("\tri state: %d\n", *pr->par1);
    return (YES);
    }

/* report_io_status - application function #15
 */
LOCAL BOOL report_io_status(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {

    /* if second call, and not called from tty mode, output io status
     * (first call is ignored; report_io_status requires no parameters)
     */
    if (!flag && !ttymode)
	{
	(VOID)printf("\tinput count:   %d\n", *pr->par1);
	(VOID)printf("\tinput status:  %d\n", *pr->par2);
	(VOID)printf("\toutput status: %d\n", *pr->par3);
	}
    return (YES);
    }

/* input_data - application function #16
 */
LOCAL BOOL input_data(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {

    if (flag)				/* first call */
	{
    
	/* if not in ttymode, prompt for the number of bytes to input
	 */
	if (!ttymode && !getword("input length", &inlen))
	    return (NO);		/* ESC hit */
	
	ireq = ireq + inlen;
	
	/* clear the input buffer and set up request block parameters
	 */
	(VOID)fill(inbuf, sizeof(inbuf), FILLCHAR);
	pr->par1 = (WORD *)inbuf;
	*pr->par2 = inlen;
	*pr->par4 = 0;
	}
    else
	{
	/* second call; display input data
	 */
        iprc = iprc + *pr->par3;
        
	if (no_display)
	    ndisplay(inbuf, *pr->par3);
	else
	    display(inbuf, *pr->par3);
	}
    return (YES);
    }

/* output_data - application function #17
 */
LOCAL BOOL output_data(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {

    if (flag)				/* first call */
	{
	
	/* if not in tty mode, prompt for output data and save length
	 */
	if (!ttymode)
	    {
	    (VOID)printf("    data? ");
	    if ((outlen = (WORD)gettext(outbuf, NO)) < 0)
		return (NO);		/* ESC hit */
	    }
    
	 oreq = oreq + outlen;
	 numwritten = numwritten + outlen;
	
	/* set up request block parameters
	 */
	pr->par1 = (WORD *)outbuf;
	*pr->par2 = outlen;
	*pr->par4 = 0;
	}
	
    /* else second call; if not in tty mode, display output count
     */
    else
	{
	oprc = oprc + *pr->par3;
	if (!ttymode)
	    (VOID)printf("\tbytes written: %d\n", *pr->par3);
	}
    return (YES);
    }

/* read_channel_state - application function #18
 */
LOCAL BOOL read_channel_state(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {

    /* if second call, output channel state and clearing code (first call is
     * ignored; read_channel_state requires no parameters)
     */
    if (!flag)
	{
	(VOID)printf("\tchannel state: %d\n", *pr->par1);
	(VOID)printf("\tclearing code: %d\n", *pr->par2);
	}
    return (YES);
    }

/* send_session_request - application function #19
 */
LOCAL BOOL send_session_request(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {

    if (flag)				/* first call */
	{
    
	/* prompt for request data and save length
	 */
	(VOID)printf("    request data? ");
	if ((outlen = (WORD)gettext(outbuf, YES)) < 0)
	    return (NO);		/* ESC hit */
	
	/* set up request block parameters
	 */
	pr->par1 = (WORD *)outbuf;
	*pr->par2 = outlen;
	*pr->par3 = 0;
	}

    /* second call ignored; driver provides no output parameters
     */
    return (YES);
    }

/* send_session_clear - application function #20
 */
LOCAL BOOL send_session_clear(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {

    if (flag)				/* first call */
	
	/* prompt for cause code and timeout
	 */
	if (!getword("cause code", pr->par1) || !getword("timeout", pr->par2))
	    return (NO);		/* ESC hit */

    /* second call ignored; driver provides no output parameters
     */
    return (YES);
    }

/* send_session_accept - application function #22
 */
LOCAL BOOL send_session_accept(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {

    if (flag)				/* first call */
	{
	
	/* prompt for accept data and save length
	 */
	(VOID)printf("    accept data? ");
	if ((outlen = (WORD)gettext(outbuf, YES)) < 0)
	    return (NO);		/* ESC hit */
	
	/* set up request block parameters
	 */
	pr->par1 = (WORD *)outbuf;
	*pr->par2 = outlen;
	*pr->par3 = 0;
	}

    /* second call ignored; driver provides no output parameters
     */
    return (YES);
    }

/* read_session_data - application function #23
 */
LOCAL BOOL read_session_data(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {

    if (flag)				/* first call */
	{
	
	/* prompt for input length
	 */
	if (!getword("read length", &inlen))
	    return (NO);		/* ESC hit */
	
	/* initialize input buffer and set up request block parameters
	 */
	(VOID)fill(inbuf, sizeof(inbuf), FILLCHAR);
	pr->par1 = (WORD *)inbuf;
	*pr->par2 = inlen;
	*pr->par4 = 0;
	}
    else
	
	/* second call; display input data
	 */
	display(inbuf, *pr->par3);
    return (YES);
    }

/* set_interrupt - application function #24
 */
LOCAL BOOL set_interrupt(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {
    struct SREGS segs;

    if (flag)				/* first call */
	{
	/* prompt for event code, code segment and offset (hex), and data
	 */
	if (!getword("event code", pr->par1) ||
	    !getword("data", pr->par4))
		return (NO);		/* ESC hit */
	segread(&segs);
	*pr->par2 = segs.cs;
	*pr->par3 = intfunc;
	}
    /* second call ignored; driver provides no output parameters
     */
    return (YES);
    }

/* reset_interrupt - application function #25
 */
LOCAL BOOL reset_interrupt(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {

    /* if first call, prompt for event code (NO is returned if ESC is hit).
     * second call is ignored; driver provides no output parameters
     */
    return (!flag || getword("event code", pr->par1));
    }

/* set_update - application function #26
 */
LOCAL WORD update_var = 0;
LOCAL WORD int_word = 0;

LOCAL BOOL set_update(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {

    if (flag)				/* first call */
	
	/* prompt for event code, update address, and data
	 */
	if (!getword("event code", pr->par1) ||
	    !getword("data 1", pr->par3) ||
	    !getword("data 2", pr->par4))
		return (NO);		/* ESC hit */

    *pr->par2 = &update_var;

    /* second call ignored; driver provides no output parameters
     */
    return (YES);
    }

/* reset_update - application function #27
 */
LOCAL BOOL reset_update(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {

    /* if first call, prompt for event code (NO is returned if ESC is hit).
     * second call is ignored; driver provides no output parameters
     */
    return (!flag || getword("event code", pr->par1));
    }

/* read_pad_parameter - application function #28
 */
LOCAL BOOL read_pad_parameter(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {

    /* if second call, output parameter value
     */
    if (!flag)
	(VOID)printf("\tparameter value: %d\n", *pr->par2);
    
    /* else first call; prompt for parameter number
     */
    else if (!getword("parameter number", pr->par1))
	return (NO);			/* ESC hit */
    return (YES);
    }

/* set_pad_parameter - application function #29
 */
LOCAL BOOL set_pad_parameter(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {

    if (flag)				/* first call */
    
	/* prompt for parameter number and value
	 */
	if (!getword("parameter number", pr->par1) ||
	    !getword("parameter value", pr->par2))
		return (NO);		/* ESC hit */

    /* second call ignored; driver provides no output parameters
     */
    return (YES);
    }

/* link statistics - application statistics function #32
 */
LOCAL BOOL link_stats(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {
    INT i;				/* loop counter */
	
    if (flag)				/* first call */
	{
	
	/* clear diagnostic array and set request block parameter 1
	 */
	(VOID)fill((UBYTE *)link_array, sizeof(link_array), 0);
	pr->par1 = link_array;
	}
    else
	{
	
	/* second call; output all non-zero diagnostic fields
	 */
	
	for (i = 0; i < 6; ++i)
	    {
	    (VOID)printf("link_stat[%d]: %04x ", i, link_array[i]);
	    putchar('\n');
	    }
	}
    return (YES);
    }

/* statistics - application (pseudo) function #33
 */
LOCAL BOOL statistics(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {
    INT c;				/* saves input character */
    INT i;				/* loop counter */
    char tbuf[100];
    INT tn;
    BOOL tflag = YES;
    LONG ototal = 0;
    LONG itotal = 0; 
	
    if (flag)				/* first call */
	{
	
	/* clear diagnostic array and set request block parameter 1
	 */
	(VOID)fill((UBYTE *)diags, sizeof(diags), 0);
	pr->par1 = diags;
	}
    else
	{
	
	poprc = piprc = liprc = loprc = 0;
	for ( i = 401; i < 421; ++i)
	    poprc = poprc + diags[i];
	for ( i = 421; i < 441; ++i)
	    piprc = piprc + diags[i];
	for ( i = 441; i < 461; ++i)
	    liprc = liprc + diags[i];
	for ( i = 461; i < 481; ++i)
	    loprc = loprc + diags[i];
	    
        printf (" total number requested input characters = %ld, number processed = %ld\n",
	    ireq, iprc);
        printf (" total number requested output characters = %ld, number processed = %ld\n",
	    oreq, oprc);
	printf (" total number link output characters = %ld, number input charas = %ld\n",
	    loprc, liprc);
	printf("  total number packet output characters = %ld, number input chars = %ld\n",
	    poprc, piprc);
	    
        tn = sprintf (tbuf, " total number requested input characters = %ld, number processed = %ld\n",
	    ireq, iprc);
	tbuf[tn] = '\0';
	write(diags_fd, tbuf, tn + 1);
        tn = sprintf (tbuf, " total number requested output characters = %ld, number processed = %ld\n",
	    oreq, oprc);
	tbuf[tn] = '\0';
	write(diags_fd, tbuf, tn + 1);
	tn = sprintf (tbuf, " total number link output characters = %ld, number input charas = %ld\n",
	    loprc, liprc);
	tbuf[tn] = '\0';
	write(diags_fd, tbuf, tn + 1);
	tn = sprintf(tbuf, "  total number packet output characters = %ld, number input chars = %ld\n",
	    poprc, piprc);
	tbuf[tn] = '\0';
	write(diags_fd, tbuf, tn + 1);
	    
	/* second call; output all non-zero diagnostic fields
	 */
	
	for (i = 0; i < DIAGS; ++i)
	    {
	    if (diags[i])
		{
                tn = sprintf(tbuf, "diags[%d]: %04x  \n", i, diags[i]);
    		tbuf[tn] = '\0';
                write(diags_fd, tbuf, tn + 1);
		
		if (tflag)
		    {
		    (VOID)printf("diags[%d]: %04x ", i, diags[i]);
	        
		    /* wait for keystroke
		     */
		    while ((c = inchar()) == 0)
			;
		    putchar('\n');
		    if (c == ESCAPE)	/* return if ESC hit */
		    tflag = NO;
		    }
		}
	    }
	tn = sprintf(tbuf, "end: \n");
	tbuf[tn] = '\0';
	write(diags_fd, tbuf, tn + 1);
	}
    return (YES);
    }

/* send_qbit_packet - application (pseudo) function #34
 */
LOCAL BOOL send_qbit_packet(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {

    if (flag)				/* first call */
	{
		
	/* prompt for packet type (function number)
	 */
	if (!getword("packet type", pr->par3))
	    return (NO);		/* ESC hit */
	    
	/* prompt for data and save length
	 */
	(VOID)printf("    data? ");
	if ((outlen = gettext(outbuf, NO)) < 0)
	    return (NO);		/* ESC hit */
	
	/* set request block parameters
	 */
	pr->par1 = (WORD *)outbuf;
	*pr->par2 = outlen;
	}


    /* second call ignored; driver provides no output parameters
     */
    return (YES);
    }

BOOL disp_event(pr, flag)
    REQ *pr;				/* poiter to request block */
    BOOL flag;				/* first call flag */
    {

    if (flag)				/* first call */
	{
		
	printf("This is contents of address %x\n", update_var);
	printf("This is the interrupt storage word: %x\n", int_word);
	return(YES);
	}

    /* second call ignored; driver provides no output parameters
     */
    return (YES);
    }


LOCAL BOOL upd_event(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {

    if (flag)				/* first call */
	{
		
	update_var = 1;
	int_word = 0;
	return(YES);
	}

    /* second call ignored; driver provides no output parameters
     */
    return (YES);
    }


LOCAL BOOL clr_seq(pr, flag)
    REQ *pr;				/* pointer to request block */
    BOOL flag;				/* first call flag */
    {

    if (flag)				/* first call */
	{
	return(YES);
	}

    /* second call ignored; driver provides no output parameters
     */
    return (YES);
    }

VOID do_int()
    {
    int_word = 999;
    }
    
/* declare and initialize the function table. A null function pointer (NOFUNC)
 * indicates the application function is to be called with no input parameters
 * and the driver will generate no output parameters
 */
FUNC func[] = {
	      
	       /* device status and control functions
		*/
	       {"undefined function", NOFUNC},			/* undefined */
	       {"clear device", clear_device},			/* 0 */
	       {"read device status", read_device_status},	/* 1 */
	       {"device reset", device_reset},			/* 2 */
	       {"set character mode", set_character_mode},	/* 3 */
	       {"set packet mode", NOFUNC},			/* 4 */
		
	       /* port status and control
		*/
	       {"set port parameters", set_port_parameters},	/* 5 */
	       {"read port parameters", read_port_parameters},	/* 6 */
	       {"set DTR", NOFUNC},				/* 7 */
	       {"reset DTR", NOFUNC},				/* 8 */
	       {"set RTS", NOFUNC},				/* 9 */
	       {"reset RTS", NOFUNC},				/* 10 */
	       {"test DSR", test_dsr},				/* 11 */
	       {"test DCD", test_dcd},				/* 12 */
	       {"test ring indicator", test_ring_indicator},	/* 13 */
	       {"send break", NOFUNC},				/* 14 */
		
	       /* data transfer and status
		*/
	       {"report I/O status", report_io_status},		/* 15 */
	       {"input data", input_data},			/* 16 */
	       {"output data", output_data},			/* 17 */
		
	       /* virtual circuit signalling
		*/
	       {"read channel state", read_channel_state},	/* 18 */
	       {"send session request", send_session_request},	/* 19 */
	       {"send session clear", send_session_clear},	/* 20 */
	       {"session answer set", NOFUNC},			/* 21 */
	       {"send session accept", send_session_accept},	/* 22 */
	       {"read session data", read_session_data},	/* 23 */
		
	       /* advanced functions
		*/
	       {"set interrupt", set_interrupt},		/* 24 */
	       {"reset interrupt", reset_interrupt},		/* 25 */
	       {"set update", set_update},			/* 26 */
	       {"reset update", reset_update},			/* 27 */
	       {"read PAD parameter", read_pad_parameter},	/* 28 */
	       {"set PAD parameter", set_pad_parameter},	/* 29 */
	       {"flush input data", NOFUNC},			/* 30 */
	       {"flush output data", NOFUNC},			/* 31 */
	       {"link statistics", link_stats},			/* 32 */
	       /* diagnostic pseudofunctions
		*/
	       {"statistics", statistics},			/* 33 */
	       {"display update event", disp_event},		/* 34 */
	       {"updatge event ", upd_event},			/* 35 */
	       {"clear seq num ", clr_seq}};			/* 36 */

/************************************************************************
 * CALLMON - X.PC Driver call monitoring
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    This module contains the main() function and support code, as well
 * as the C code necessary to unload the call monitor.  Much of this code
 * is duplicated from XPCMAIN.C, hence the references to the "driver" in
 * the comments.
 *
 * REVISION HISTORY:
 *
 *   Date    Version    By        Purpose of Revision
 * --------  ------- ----------   -----------------------------------------
 * 03/04/87   4.00A  S. Bennett   Initial Draft
 *
 ************************************************************************/

/* Include files
 */
#include "stddef.h"
#include "xpc.h"
#include "iocomm.h"
#include "dos.h"
#include "timer.h"

/* External Functions
 */
IMPORT UWORD save_ds();			/* Saves Data & Code segments */
IMPORT VOID exit();			/* Microsoft exit routine */
IMPORT VOID get_intvec();		/* Reads current interrupt vector */ 
IMPORT VOID get_sp();			/* Gets safe driver stack address */
IMPORT VOID int86();			/* Microsoft software int caller */
IMPORT VOID int86x();			/* int86() with segment reg passing */
IMPORT VOID movedata();			/* far addressing move memory */
IMPORT VOID set_intvec();		/* Sets an interrupt vector */
IMPORT VOID xpc_appint();		/* Software interrupt 0x7A handler */

/* External variables
 */
IMPORT UWORD xpc_cs;			/* Place to save CS for driver use */
IMPORT UWORD app_ss;			/* Place to save driver SS */
IMPORT UWORD app_sp;			/* Place to save driver SP */
IMPORT UWORD app_ivec;			/* Software interrupt vector no. */
IMPORT UWORD _asizds;			/* MSC size of data segment */
IMPORT UWORD _psp;			/* MSC program segment prefix */
IMPORT UWORD aivec_cs;			/* Save old CS of software int */
IMPORT UWORD aivec_ip;			/* Save old IP of software int */ 
IMPORT INT monfd;
IMPORT TEXT monname[];
IMPORT BOOL verbose;
IMPORT BOOL terse;
IMPORT BOOL packetonly;
IMPORT BOOL waitclose;
IMPORT BOOL inoutstatus;

/* Local (Static) variables
 */
LOCAL TEXT *version = "Call Monitor for X.PC Driver loaded$";
LOCAL TEXT *unloaded = "Call Monitor unloaded$";
LOCAL TEXT *usg_msg[] = {
"Usage: callmon [-o<filename>][-x<vec>] [-u] [-v] [-t] [-p] [-s] [-c]\r\n$",
"       where -x<vec> sets the interrupt vector number (default 0x7f)\r\n$",
"             -u unloads the monitor or driver, if it is already loaded.\r\n$",
"             -o<filename> sets the path and filename for output.\r\n$",
"             -v specifies verbose output.  This means that actual data\r\n$",
"                sent and received will be placed in the output file. \r\n$",
"             -t specifies terse output.  This means that calls to\r\n$",
"                Report I/O Status, Read DCD, Read Device Status, and\r\n$",
"                Read Channel Status produce no output whatsoever.\r\n$",
"             -p tells the monitor to record nothing unless the driver\r\n$",
"                is in packet mode.\r\n$",
"             -s tells the monitor to write out an I/O status line for\r\n$",
"                each input data or output data call, giving the number\r\n$",
"                of report I/O status calls since last input or output\r\n$",
"                call, as well as the last status read.\r\n$",
"             -c tells the monitor to close the file every 100 lines\r\n$",
"                instead of every line.  You might lose some stuff this\r\n$",
"                way, so be warned.\r\n$",
NULL };



/************************************************************************
 * LOCAL VOID load_error(s)
 *    TEXT *s;			 Error message to display on exit 
 *
 *    This function displays a string to the console and exits without
 *    staying resident.  It is used by the main() routine to stop loading
 *    should an error occur during it.      
 *
 * Notes: Should only be called at load time.
 *
 * Returns: Never returns
 *
 ************************************************************************/
LOCAL VOID load_error(s)
    TEXT *s;
    {
    union REGS regs;
    
    /* Output a message via DOS function 9.
     */
    regs.x.ax = 0x0900;
    regs.x.dx = (UWORD)s;
    int86(0x21, &regs, &regs);
    exit(-1);
    }


/************************************************************************
 * LOCAL VOID usage()
 *
 *    Called when the user tries to execute the driver with an unrecognized
 *    or illegal command line parameter.  This routine prints out the
 *    calling sequence & valid command line parameters, then exits.
 *
 * Notes: Should only be called at load time.
 *
 * Returns: Never returns
 *
 ************************************************************************/
LOCAL VOID usage()
    {
    union REGS regs;
    INT i;
    
    /* Output all lines in the usage message via DOS function 9.
     */
    for (i = 0; usg_msg[i]; ++i)
	{
	regs.x.ax = 0x0900;
	regs.x.dx = (UWORD)(usg_msg[i]);
	int86(0x21, &regs, &regs);
	}
    exit(-1);
    }
   

/************************************************************************
 * LOCAL VOID unload_driver()
 *
 *    Called when the program has been executed with the "-u" parameter
 *    set.  In this case, the program will attempt to unload a previously
 *    loaded copy of the driver.  The program then exits.
 *
 * Notes: Should only be called at load time
 *
 * Returns: none
 *
 ************************************************************************/
LOCAL VOID unload_driver()
    {
    union REGS regs;
    struct SREGS sregs;
    UWORD driver_psp;
    
    /* If the driver has not been loaded, don't even try to unload it.
     */
    if (aivec_cs == 0 && aivec_ip == 0)
	load_error("Driver not loaded$");
	
    /* Call the already loaded driver with an ES:BX of 0:0.  This will
     * tell it to return its PSP (Program Segment Prefix) in BX.  We could
     * alternatively, get the PSP from the code segment of the interrupt
     * vector, but this would fail should we ever move to big model.  This
     * method is far safer.  It also allows for the already loaded driver
     * to restore the interrupt vector.
     */
    regs.x.bx = 0;
    sregs.es = 0;
    int86x(app_ivec, &regs, &regs, &sregs);
    
    /* Save the PSP.
     */
    driver_psp = regs.x.bx;
    
    /* Get the already loaded driver's Environment segment.  The segment
     * address for this is located at offset 0x2c in the PSP segment.
     * Store the resulting segment address in ES and free it with the
     * Free Allocated Memory DOS call (0x49)
     */
    segread(&sregs);
    movedata(driver_psp, 0x002c, sregs.ds, &sregs.es, 2);
    regs.x.ax = 0x4900;
    int86x(0x21, &regs, &regs, &sregs);
    
    /* Free up the memory used by the loaded driver.  Then we're done.
     * (Note that because the memory allocation routine we are using
     * works by extending the size of the PSP memory block, there is no
     * need for any other cleanup.)
     */
    regs.x.ax = 0x4900;
    sregs.es = driver_psp;
    int86x(0x21, &regs, &regs, &sregs);
    }


/************************************************************************
 * VOID xpc_unload()
 *
 *    Called by the software interrupt handler when it is asked to unload
 *    the program.  This is the driver-side counterpart to unload_driver(),
 *    executed by the driver call with ES:BX set to 0.  It turns off all
 *    interrupts (if any are running) and restores the software interrupt
 *    vector to it's original value.
 *
 * Notes: Should only be called by the software interrupt handler at
 *    unload time.
 *
 * Returns: none
 *
 ************************************************************************/
VOID xpc_unload()
    {
    
    if (monfd >= 0)
	{
	iclose(monfd);
	monfd = -1;
	}
    /* Restore the original software interrupt vector
     */
    set_intvec(app_ivec, aivec_cs, aivec_ip);
    }


/************************************************************************
 * VOID main(ac, av)
 *    INT ac;			 Number of command line parameters 
 *    TEXT *av[];		 Array of command line param strings
 *
 *    This is the main program for the call monitor.  It parses the command
 *    line, setting options according to the specified parameters, then it
 *    allocates all needed memory, initializes necessary structures and
 *    arrays, and sets up the software interrupt.  Then if all has been
 *    successful, it terminates and stays resident.
 *
 * Notes: Called by Microsoft C.  
 *
 * Returns: never returns
 *
 ************************************************************************/
VOID main(ac, av)
    INT ac;
    TEXT *av[];
    {
    UWORD ds;
    UWORD bufsiz;
    BOOL cts, unload;
    union REGS regs;
    
    /* Set up default parameter settings
     * (Currently, CTS checking is disabled for testing. Change it later.)
     */
    unload = NO;			/* Default unload */
    app_ivec = 0x7a;			/* Default software interrupt vec */
    strcpy(monname, "MONDAT.DAT");	/* Default monitor datafile name */
    
    /* Parse the parameters
     */
    while (--ac)
	{
	++av;
	if (av[0][0] == '-')
	    {
	    switch (av[0][1])
		{
	    case 'x':
	    case 'X':
		/* Select interrupt vector
		 */
		app_ivec = atoi(&av[0][2]);
		if (app_ivec < 0 || app_ivec > 0xff)
		    load_error("Illegal vector number\n$");
		break;
	    case 'u':
	    case 'U':
		/* Unload already loaded driver
		 */
		unload = YES;
		break;
	    case 'o':
	    case 'O':
		/* change the name of the output file
		 */
		if (av[0][2])
		    strcpy(monname, &av[0][2]);
		break;
	    case 'v':
	    case 'V':
		/* Set verbose monitoring... Ie.. We write out data
		 * on input and output calls...
		 */
		verbose = YES;
		break;
	    case 't':
	    case 'T':
		/* Set terse monitoring.
		 */
		terse = YES;
		break;
	    case 'p':
	    case 'P':
		/* Set packet mode only monitoring.
		 */
		packetonly = YES;
		break;
	    case 's':
	    case 'S':
		/* Set report status on input/output data
		 */
		inoutstatus = YES;
		break;
	    case 'c':
	    case 'C':
		/* Set close only every 100 lines
		 */
		waitclose = YES;
		break;
	    default:
		/* If it matches no valid param, display usage & exit.
		 */
		usage();
		}
	    }
	}

    /* Get the existing software interrupt vector
     */
    get_intvec(app_ivec, &aivec_cs, &aivec_ip);
    if (unload)
	{
	/* If unloading the driver, call unload_driver() and print out
	 * "driver unloaded" message.
	 */
	unload_driver();
	regs.x.ax = 0x0900;
	regs.x.dx = (UWORD)unloaded;
	int86(0x21, &regs, &regs);
	exit(0);
	}
    
    /* If we get here, we aren't unloading.  So if the vector is non-zero,
     * then the driver (or *something*...) is already loaded.
     */
    if (!aivec_cs && !aivec_ip)
	load_error("X.PC Driver not yet loaded!$");
    
    /* Create the monitor file
     */
    icreate(monname);
    
    /* Perform various allocations and initializations
     */
    ds = save_ds();
    get_sp(&app_ss, &app_sp);

    /* Set the software interrupt vector to call xpc_appint()
     */
    set_intvec(app_ivec, xpc_cs, (UWORD)xpc_appint);
    set_intvec(0x6A, aivec_cs, aivec_ip);
    
    /* Print out the "driver loaded" message
     */
    regs.x.ax = 0x0900;
    regs.x.dx = (UWORD)version;
    int86(0x21, &regs, &regs);
    
    /* Set the size of the PSP based memory block to just enough for
     * the driver and it's data, then terminate and stay resident.
     * _asizds is the size of the data segment in bytes.
     */
    regs.x.ax = 0x3100;
    regs.x.dx = (_asizds >> 4) + (ds - _psp) + 1;
    int86(0x21, &regs, &regs);
    }

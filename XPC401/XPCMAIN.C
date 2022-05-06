/************************************************************************
 * XPCMAIN - X.PC Driver main program & load/unload code.
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    This module contains the main() function and support code, as well
 * as the C code necessary to unload the driver.
 *
 * REVISION HISTORY:
 *
 *   Date    Version    By        Purpose of Revision
 * --------  ------- ----------   -----------------------------------------
 * 12/10/86   4.00    S. Bennett  Initial Draft
 *  6/23/87   4.01    SAB         Allowed char forwarding in non-echo mode
 *                                Fixed yellow ball response problem
 ************************************************************************/

/* Include files
 */
#include "stddef.h"
#include "ctype.h"
#include "xpc.h"
#include "iocomm.h"
#include "dos.h"
#include "timer.h"

/* External Functions
 */
IMPORT BUFLET *init_buf();      /* Allocates & sets up buflets */
IMPORT INT atoi();          /* Microsoft Ascii to integer */
IMPORT TIMER *init_timer_array();   /* Allocates & sets up timer array */
IMPORT UBYTE *init_comm();      /* Allocates comm buffers */
IMPORT UWORD save_ds();         /* Saves Data & Code segments */
IMPORT VOID app_initialization();   /* Initializes Driver structures */
IMPORT VOID disable_comm();     /* Turns off comm interrupts */
IMPORT VOID disable_timer();        /* Turns off timer interrupts */
IMPORT VOID enable_timer();     /* enable timer */
IMPORT VOID exit();         /* Microsoft exit routine */
IMPORT VOID get_intvec();       /* Reads current interrupt vector */ 
IMPORT VOID get_sp();           /* Gets safe driver stack address */
IMPORT VOID int86();            /* Microsoft software int caller */
IMPORT VOID int86x();           /* int86() with segment reg passing */
IMPORT VOID movedata();         /* far addressing move memory */
IMPORT VOID set_intvec();       /* Sets an interrupt vector */
IMPORT VOID xpc_appint();       /* Software interrupt 0x7A handler */

/* External variables
 */
IMPORT BOOL use_timer_int;
IMPORT BOOL xpc_timer_active;
IMPORT COMMINFO comm_info;      /* IOCOMM information structure */
IMPORT UWORD xpc_cs;            /* Place to save CS for driver use */
IMPORT UWORD app_ss;            /* Place to save driver SS */
IMPORT UWORD app_sp;            /* Place to save driver SP */
IMPORT UWORD app_ivec;          /* Software interrupt vector no. */
IMPORT UWORD _asizds;           /* MSC size of data segment */
IMPORT UWORD _abrktb;           /* MSC size of alloced heap */
IMPORT UWORD _psp;          /* MSC program segment prefix */
IMPORT UWORD aivec_cs;          /* Save old CS of software int */
IMPORT UWORD aivec_ip;          /* Save old IP of software int */ 

/* Local (Static) variables
 */
LOCAL TEXT *version = "X.PC Driver, Vn 4.01 loaded\r\n$";
LOCAL TEXT *confide = "Confidential Beta Release -- Do not redistribute\r\n$";
LOCAL TEXT *unloaded = "X.PC Driver unloaded$";
LOCAL TEXT *usg_msg[] = {
"Usage: xpc [-x<vec>] [-b<buffers>] [-u] [-t] [-c]\r\n$",
"       where -x<vec> sets the interrupt vector number (default 7A hex)\r\n$",
"             -b<buffers 2750 <= x <= 50000 > sets the buffer size\r\n$", 
"        (default 6000 decimal)\r\n$",
"             -u unloads the driver, if it is already loaded.\r\n$",
"             -t should be used with programs which make use of\r\n$",
"                the interrupt 8 timer.  (See documentation)\r\n$",
"         and -c turns off checking of Clear to Send\r\n$",
NULL };



/************************************************************************
 * LOCAL VOID load_error(s)
 *    TEXT *s;           Error message to display on exit 
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
    
    /* turn off comm & timer interrupts
     */
    disable_comm();
    disable_timer();

    /* Restore the original software interrupt vector
     */
    set_intvec(app_ivec, aivec_cs, aivec_ip);
    }



/************************************************************************
 * LOCAL UWORD ahtoi(s)
 *    TEXT *s;
 *
 *    Converts an ascii hex value to unsigned integer.
 *
 * Returns: Value returned
 *
 ************************************************************************/
LOCAL UWORD ahtoi(s)
    TEXT *s;
    {
    TEXT c;
    UWORD val;
    
    val = 0;
    while (*s)
    {
    c = toupper(*s);
    ++s;
    if (c >= '0' && c <= '9')
        {
        val <<= 4;
        val += (c - '0');
        }
    else if (c >= 'A' && c <= 'F')
        {
        val <<= 4;
        val += ((c - 'A') + 10);
        }
    else
        break;
    }
    return(val);
    }


/************************************************************************
 * VOID main(ac, av)
 *    INT ac;            Number of command line parameters 
 *    TEXT *av[];        Array of command line param strings
 *
 *    This is the main program for the X.PC driver.  It parses the command
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
     */
    cts = YES;              /* Default CTS Checking */
    unload = NO;            /* Default unload */
    app_ivec = 0x7a;            /* Default software interrupt vec */
    bufsiz = 6000;          /* Default buffer size */
    use_timer_int = YES;        /* Default use of timer int vec */
    
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
        app_ivec = ahtoi(&av[0][2]);
        if (app_ivec < 0 || app_ivec > 0xff)
            load_error("Illegal vector number\n$");
        break;
        case 'b':
        case 'B':
        /* Set new buffer size
         */
        bufsiz = atoi(&av[0][2]);
        if (bufsiz < 2750 || bufsiz > 55000)
            load_error("Illegal buffer size (range 2500-55000)\n$");
        break;
        case 'c':
        case 'C':
        /* Turn off CTS checking
         */
        cts = NO;
        break;
        case 'u':
        case 'U':
        /* Unload already loaded driver
         */
        unload = YES;
        break;
        case 't':
        case 'T':
        /* Set up immediate timer interrupt initialization
         */
        use_timer_int = NO;
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
    if (aivec_cs || aivec_ip)
    load_error("X.PC Driver already loaded$");
    
    /* Perform various allocations and initializations
     */
    ds = save_ds();
    get_sp(&app_ss, &app_sp);
    if (init_buf(bufsiz) == NULL ||
    init_comm() == NULL ||
    init_timer_array() == NULL)
    load_error("Not enough memory to load$");
    app_initialization();
    comm_info.ctschecking = cts;
    
    /* Startpace and endpace are fixed as XON/XOFF currently.  This may
     * change later.
     */
    comm_info.startpace = XOFF;
    comm_info.endpace = XON;

    /* Set the software interrupt vector to call xpc_appint()
     */
    set_intvec(app_ivec, xpc_cs, (UWORD)xpc_appint);
    
    /* If the -t flag was specified, then we should 
     * set up the timer interrupt vector in advance to
     * bypass problems with applications which use the
     * 0x8 timer interrupt vector.  Link will only gain
     * control if the xpc_timer_active flag is set.
     */
    if (!use_timer_int)
    {
    xpc_timer_active = NO;
    enable_timer();
    }
    
    /* Print out the "driver loaded" message
     */
    regs.x.ax = 0x0900;
    regs.x.dx = (UWORD)version;
    int86(0x21, &regs, &regs);
    
    /* Print out the "confidential" message
     *
    regs.x.ax = 0x0900;
    regs.x.dx = (UWORD)confide;
    int86(0x21, &regs, &regs);
     */
    
    /* Set the size of the PSP based memory block to just enough for
     * the driver and it's data, then terminate and stay resident.
     * _asizds is the size of the data segment in bytes.
     */
    _asizds = _abrktb + 16;
    regs.x.ax = 0x3100;
    regs.x.dx = (_asizds >> 4) + (ds - _psp) + 1;
    int86(0x21, &regs, &regs);
    }


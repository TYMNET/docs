/************************************************************************
 * jmppkt.c - Jump Table for Packet Functions
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    jmppkt.c contains a jump table which is used to execute
 *    application functions when device is in packet state.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KS    Initial Draft    
 *
 ************************************************************************/
#include "stddef.h"
#include "xpc.h"
#include "device.h"
#include "appl.h"
#include "error.h"
 
IMPORT INT chnl_status();             /* read channel status */
IMPORT INT clr_device();              /* clear device */
#ifdef DEBUG
IMPORT INT debug_stats();	      /* debug statistics */
#endif
IMPORT INT flush_pkt_input();         /* flush input buffer */
IMPORT INT flush_pkt_output();        /* flush output buffer */
IMPORT INT link_stats();              /* link statistics */
IMPORT INT p_read_data();             /* input data from device */
IMPORT INT p_rpt_iostat();            /* report I/O status */
IMPORT INT p_set_charstate();         /* set to chararcter mode */
IMPORT INT p_set_pktstate();          /* set to packet mode */
IMPORT INT p_write_data();            /* output data to device */
IMPORT INT read_devstatus();          /* read device status */
IMPORT INT read_portparam();          /* read port parameters */
IMPORT INT read_ssndata();            /* read session data */
IMPORT INT reset_device();            /* reset device */
IMPORT INT reset_dtr();               /* reset data terminal ready */
IMPORT INT reset_rts();               /* reset request to send */
IMPORT INT rpt_padparam();            /* report pad parameters */
IMPORT INT reset_interrupt();         /* reset interrupt */
IMPORT INT reset_update();            /* reset update */
IMPORT INT send_pkt_brk();            /* send break */
IMPORT INT send_ssnacp();             /* send session accept */
IMPORT INT send_ssnclr();             /* send session clear */
IMPORT INT send_ssnreq();             /* send session request */
IMPORT INT ssn_answerset();           /* session answer set */
IMPORT INT set_dtr();                 /* set data terminal ready */
IMPORT INT set_interrupt();           /* set interrupt */
IMPORT INT set_rts();                 /* set ready to send */
IMPORT INT set_update();              /* set udpate */
IMPORT INT set_padparam();            /* set pad parameters */
IMPORT INT set_portparam();           /* set port parameters */
IMPORT INT test_dcd();                /* test static carrier detect */
IMPORT INT test_dsr();                /* test data set ready */
IMPORT INT test_ri();                 /* test ring indicator */


/* Table which is used to call the appropriate routine to process
 * the specified function when the device is in packet mode.
 */ 
#ifdef DEBUG
LOCAL INT (*pkttab[MAX_APPL_FUNC + 1 + 2])() =
#else
LOCAL INT (*pkttab[MAX_APPL_FUNC + 1 + 1])() =
#endif
			             /* array of pointers to functions 
                       			returning ints */
    {
    clr_device,                      /* Clear Device */
    read_devstatus,                  /* Read Device Status */
    reset_device,                    /* Reset Device */
    p_set_charstate,                 /* Set Character Mode */
    p_set_pktstate,                  /* Set Packet Mode */
    set_portparam,                   /* Set Port Parameters */
    read_portparam,                  /* Read Port Parameters */
    set_dtr,                         /* Set DTR */
    reset_dtr,                       /* Reset Dataterminal Ready */
    set_rts,                         /* Set Request to Send */
    reset_rts,                       /* Reset Request to Send */
    test_dsr,                        /* Test Dataset Ready */
    test_dcd,                        /* Test Static Carrier Detect */
    test_ri,                         /* Test Ring Indicator */
    send_pkt_brk,                    /* Send Break */
    p_rpt_iostat,                    /* Report I/O Status */
    p_read_data,                     /* Input Data */
    p_write_data,                    /* Output Data */
    chnl_status,                     /* read_ channel status */
    send_ssnreq,                     /* Send Session Request */
    send_ssnclr,                     /* Send Session Clear */
    ssn_answerset,                   /* Session Answer Set */
    send_ssnacp,                     /* Send Session Accept */
    read_ssndata,                    /* Read Session Data */
    set_interrupt,                   /* Set Interrupt */
    reset_interrupt,                 /* Reset Interrupt */
    set_update,                      /* Set Update */
    reset_update,                    /* Reset Update */
    rpt_padparam,                    /* Report PAD Params */
    set_padparam,                    /* Set PAD Params */
    flush_pkt_input,                 /* Flush Input Buffer */
    flush_pkt_output,                /* Flush Output */
#ifdef DEBUG
    link_stats,			     /* Link Statistics */
    debug_stats};		     /* Debug Statistics */
#else
    link_stats};                     /* Link Statistics */
#endif

/************************************************************************
 * INT jmp_pkt_funcs(function)
 *    WORD funcion;     - function to be performed
 *
 *    jmp_pkt_funcs() processes application functions when device
 *    is in a packet state.   
 *
 * Returns:  jmp_pkt_funcs() returns errors encountered during processing
 *
 ************************************************************************/
INT jmp_pkt_funcs(function)
    WORD function;                    /* function to be performed */
    {
 
    /* Call pkttab function which was specified by the application.
     */
    return ((*pkttab[function])());
    }


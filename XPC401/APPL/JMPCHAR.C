/************************************************************************
 * jmpchar.c - Character State Jump Table
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    jmpchar.c contains the jump table which is used to process
 *    application functions when device is in character state.
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
#include "appl.h"
#include "error.h"

#ifdef DEBUG
IMPORT INT debug_stats();		/* debug statistics*/
#endif
IMPORT INT c_chnl_status();             /* get status of channel 0 */
IMPORT INT c_ill_func();                /* illegal character state function */
IMPORT INT c_read_data();               /* input data */
IMPORT INT c_set_charstate();           /* set state to character */
IMPORT INT c_set_pktstate();            /* set state to packet */
IMPORT INT c_write_data();              /* output data to device */
IMPORT INT clr_device();                /* clear device */
IMPORT INT flush_char_input();          /* flush input buffer */
IMPORT INT flush_char_output();         /* flush output buffer */
IMPORT INT link_stats();                /* link statistics */
IMPORT INT read_devstatus();            /* read device status */
IMPORT INT reset_device();              /* reset device */
IMPORT INT reset_dtr();                 /* clear data terminal ready */
IMPORT INT reset_interrupt();           /* reset interrupt */
IMPORT INT reset_rts();                 /* clear request to send */
IMPORT INT reset_update();              /* reset update */
IMPORT INT read_portparam();            /* read port parameters */
IMPORT INT rpt_char_iostat();           /* report I/O status */
IMPORT INT rpt_padparam();	        /* report PAD parameters */
IMPORT INT send_char_brk();             /* send break */
IMPORT INT set_dtr();                   /* set data terminal ready */
IMPORT INT set_interrupt();             /* set interrupt */
IMPORT INT set_padparam();		/* set PAD parameters */
IMPORT INT set_rts();                   /* set request to send */
IMPORT INT set_update();                /* set update */
IMPORT INT set_portparam();             /* set port parameters */
IMPORT INT test_dcd();                  /* test static carrier detect */
IMPORT INT test_dsr();                  /* test data set ready */
IMPORT INT test_ri();                   /* test ring indicator */
 
#ifdef DEBUG
LOCAL INT (*chartab[MAX_APPL_FUNC + 2])() =
#else
LOCAL INT (*chartab[MAX_APPL_FUNC + 1])() =
#endif
    {
    clr_device,                         /* Clear Device */
    read_devstatus,             	/* Read Device Status */
    reset_device,           		/* Device Reset */
    c_set_charstate,          		/* Set Character state */
    c_set_pktstate,            		/* Set Packet state */ 
    set_portparam,             		/* Set Port Parameters */
    read_portparam,            		/* Read Port Parameters */
    set_dtr,          		        /* set DTR */
    reset_dtr,            		/* reset DTR */
    set_rts,           		        /* set RTS*/
    reset_rts,           		/* reset RTS */
    test_dsr,        		        /* Test DSR */
    test_dcd,            		/* Test DCD */
    test_ri,                 		/* test RI */
    send_char_brk,      		/* Send break */
    rpt_char_iostat,      	        /* Report I/O status */
    c_read_data,            		/* Input Data */
    c_write_data,    		        /* Output Data */
    c_chnl_status,    		        /* read channel zero */
    c_ill_func,      		        /* Virtual Session Functions */
    c_ill_func,       		        /* Virtual Session Functions */
    c_ill_func,          	        /* Virtual Session Functions */
    c_ill_func,            		/* Virtual Session Functions */
    c_ill_func,        		        /* Virtual Session Functions */
    set_interrupt,      	        /* Set Interrupt */
    reset_interrupt,       	        /* Reset Interrupt */
    set_update,     		        /* Set Update */
    reset_update,  	                /* Reset Update */
    rpt_padparam,    		        /* Report PAD Parameters */
    set_padparam,        		/* Set PAD Parameters */
    flush_char_input,       		/* Flush Input Data */
    flush_char_output,      		/* Flush Output Data */
#ifdef DEBUG
    link_stats,				/* link statistics */
    debug_stats};			/* debug statistics */
#else
    link_stats};			/* Link Statistics */
#endif

/************************************************************************
 * INT jmp_char_funcs(function)
 *    WORD function;	- application function to be performed
 *
 *    INT jmp_char_funcs() executes application functions when device is
 *    in characeter state.
 *
 * Returns:  jmp_char_funcs() returns any errors encountered during
 *    processing.
 *
 ************************************************************************/
INT jmp_char_funcs(function)
    WORD function;                      /*  function to be performed */
    {

    /* Call routine in chartab table specified by application function.
     */
    return ((*chartab[function])());
    }






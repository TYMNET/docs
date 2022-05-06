/************************************************************************
 * jmprs.c - Execute Application Functions When in Wait/Reset State
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    jmprs.c contains the module jmp_reset_funcs which calls
 *    the function specified by the application. The status returned 
 *    by the called function is returned.  
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
#include "state.h"
#include "error.h"

IMPORT INT clr_device();           	/* clear device */
#ifdef DEBUG
IMPORT INT debug_stats();	   	/* debug statisitcs */
#endif
IMPORT INT link_stats();	   	/* link statistics */
IMPORT INT read_devstatus();       	/* read device status */
IMPORT INT read_portparam();            /* read port parameters */
IMPORT INT rpt_rs_iostat();     	/* report I/O status */
IMPORT INT rs_ill_func();		/* illegal wait/reset function */
IMPORT INT reset_device();      	/* reset the device */
IMPORT INT set_portparam();	 	/* set port parameters */
IMPORT VOID mov_param();		/* move data to/from parameter
					 * block.
					 */

/* table which determines processing for application functions
 * when the device is in wait reset mode.
 */
#ifdef DEBUG
LOCAL INT (*rstab[MAX_APPL_FUNC + 2])() =
#else
LOCAL INT (*rstab[MAX_APPL_FUNC + 1])() =
#endif
					/* Array of pointers to functions 
					   returning ints. */    
    { clr_device,                  	/* Clear Device */
      read_devstatus,              	/* Read Device Status */
      reset_device,              	/* Device Reset */
      rs_ill_func,               	/* Set Character State */
      rs_ill_func,            	     	/* Set Packet State */
      set_portparam,              	/* Set Port Parameters */
      read_portparam,                 	/* Read Port Parameters */
      rs_ill_func,                 	/* Set DTR */
      rs_ill_func,                 	/* Reset DTR */
      rs_ill_func,                 	/* Set RTS */
      rs_ill_func,                 	/* Reset RTS */
      rs_ill_func,                 	/* Test DSR */
      rs_ill_func,                 	/* Test DCD */
      rs_ill_func,                 	/* Test RI */
      rs_ill_func,                 	/* Send Break */
      rpt_rs_iostat,               	/* Report I/O Status */
      rs_ill_func,                 	/* Input Data */
      rs_ill_func,                 	/* Output Data */
      rs_ill_func,                 	/* Virtual Session Functions */
      rs_ill_func,                 	/* Virtual Session Functions */
      rs_ill_func,                 	/* Virtual Session Functions */
      rs_ill_func,                 	/* Virtual Session Functions */
      rs_ill_func,                 	/* Virtual Session Functions */
      rs_ill_func,                 	/* Virtual Session Functions */
      rs_ill_func,                 	/* Set Interrupt */
      rs_ill_func,                 	/* Reset Interrupt */
      rs_ill_func,                 	/* Set Update */
      rs_ill_func,                 	/* Reset Update */
      rs_ill_func,                 	/* Report PAD Parameter */
      rs_ill_func,                 	/* Set PAD Parameter */
      rs_ill_func,                 	/* Flush Input Buffer */
      rs_ill_func,                	/* Flush Output Buffer */
#ifdef DEBUG
      link_stats,                   	/* Link Statistics  */
      debug_stats			/* debug statisitcs */
#else
      link_stats		    	/* Link Statistics */
#endif      
      };

/************************************************************************
 * LOCAL INT rs_ill_func()
 *
 *    rs_ill_func() returns invalid wait-reset error for functions which
 *    are invalid in wait reset mode.
 *
 * Returns:  rs_ill_func always returns ILLEGAL_WS_FUNC (illegal
 *    application function when in wait/reset state.
 *
 ************************************************************************/
LOCAL INT rs_ill_func()
    {
    
    return (ILLEGAL_WS_FUNC);
    }

/************************************************************************
 * INT rpt_rs_iostat()
 *
 *    rpt_rs_iostat() returns the current I/O status for 
 *    channel 0 when device is in wait reset state.  When in wait reset
 *    state, the output status is always flow controlled.
 *
 *
 * Returns: rpt_rs_stat always returns SUCCESS.
 *
 ************************************************************************/
LOCAL INT rpt_rs_iostat()
    {
    WORD inp_status = INPUT_NORMAL;	/* input status */
    WORD out_status = FLOWCTRL_OUTPUT;	/* output status */ 
    WORD inp_count = 0;			/* number of input bytes */
   
    mov_param((UBYTE *)&inp_count, sizeof(WORD), PARAM_1, 0, TO_APPL);
    mov_param((UBYTE *)&inp_status, sizeof(WORD), PARAM_2, 0, TO_APPL);
    mov_param((UBYTE *)&out_status, sizeof(WORD), PARAM_3, 0, TO_APPL);
    return (SUCCESS);
    }

/************************************************************************
 * INT jmp_reset_funcs()
 *    WORD function;	- application function
 *
 *    jmp_reset_funcs() executes application functions when device
 *    is in wait-reset state.
 *
 * Returns:  Any errors encountered during processing or SUCCESS.
 *
 ************************************************************************/
INT jmp_reset_funcs(function)
    WORD function;                 /* function specified by user */
    {
 
    /* Call rstab routine specified by function.  
     */
    return ((*rstab[function])());
    }



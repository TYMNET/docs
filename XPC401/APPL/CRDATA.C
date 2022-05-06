/************************************************************************
 * crdata.c - Character Mode Read Data
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    crdata.c contains the module c_read_data() which reads
 *    data from the application read queue into the application
 *    parameter buffer.
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
#include "pkt.h"
#include "error.h"

IMPORT BUFLET *get_queue();             /* get next buflet chain from queue */
IMPORT INT read_buflet_data();		/* read data from one buflet chain */
IMPORT VOID free_buf();			/* free buflet chain */
IMPORT VOID int_disable();		/* disable interrupts */
IMPORT VOID int_enable();		/* enable interrupts */
IMPORT VOID mov_param();                /* move data in/out application 
				 	 * parameter
                                         */
 
IMPORT CHNLINFO *ap_pcis;	        /* pointer to channel info structure */
IMPORT DEVSTAT dev_stat;                /* device status structure */
IMPORT UWORD program_error;             /* program error word */

/************************************************************************
 * INT c_read_data()
 *
 *    c_read_data() reads data from the application read queue.  The
 *    data is moved into the application parameter buffer.  Data is
 *    always read from channel zero's application read queue.
 *
 * Usage notes: Character data packets are put on the queue by
 *    the link.  It is assumed that data is to be moved into
 *    buffer pointed to by parameter 1.
 *
 * Returns: If the port is not active, c_read_data returns ILLEGAL_PORT,  
 *    Otherwise, c_read_data returns SUCCESS.
 *
 ************************************************************************/
INT c_read_data()
    {
    BUFLET *pb;                         /* pointer to first buflet */
    BUFLET *ppkt;		        /* pointer to current buflet */
    UWORD totleft;                      /* total number left to read */
    INT num = 0;                        /* output buffer position */
    UWORD idx;                          /* starting read index */
    UWORD nreq;                         /* maximum number of bytes to read */
    UWORD opos;                         /* starting output position */

    /* If port is not active, return error.
     */
    if (dev_stat.portaddr == 0)
        return (ILLEGAL_PORT);
    
    /* extract the number of bytes to read from application parameters
     */    
    mov_param((UBYTE *)&nreq, sizeof(UWORD), PARAM_2, 0, FROM_APPL);
    ppkt = pb = ap_pcis->readqueue.begqueue;
	
    /* Search for buflet containing next byte of data to read.
     * When loop is finished, idx points to next byte of data
     * to be read and ppkt is the address of the buflet which
     * contains the data to be read. 
     */
    for (idx = ap_pcis->idxreadqueue; (ppkt && idx >= DATA_BUF_SIZ);
        idx -= DATA_BUF_SIZ, ppkt = ppkt->bufnext)
        ;
    
    /* Read data until maximum number of bytes requeusted is read or
     * there is no more data in application read queue.
     */
    for (opos = 0, totleft = nreq; ppkt && totleft; totleft -= num, idx = 1)
        {
	
	/* Move data from packet until packet is empty or
         * application parameter buffer is full.
         */
        num = read_buflet_data(ppkt, totleft, (UWORD)(pb->bufdata[CH_LEN]),
	    (WORD)idx, &opos);

	/* Decrement the number of bytes in the application read queue.
	 */
        int_disable();
        ap_pcis->nreadbytes -= num;
        int_enable();
        
	/* If more input data is needed, get next buflet chain from
         * application read queue.
         */
        if (num >= (INT)(pb->bufdata[CH_LEN]))
            {
            if ((pb = get_queue(&ap_pcis->readqueue)) == NULLBUF)
                {
                program_error = (UWORD)C_READ_ERROR;
                break;
                }
	    free_buf(pb);
	   
	    /* Reset the current pointer to the read queue and the
	     * index into the read queue.
	     */
	    ap_pcis->idxreadqueue = 1;
	    ppkt = pb = ap_pcis->readqueue.begqueue;
	    }
	    
	/* The number of requested bytes have been moved to the
	 * application parameter buffer.  Update the index for
	 * the read queue and the number of characters left to
	 * process in the current buflet chain.
	 */
        else
	    {
	    ap_pcis->idxreadqueue += num;
            pb->bufdata[CH_LEN] -= num;
	    }
	}    
    
    /* Update the index into the current buflet chain and
     * the number of bytes to read in buflet chain.
     */
    mov_param((UBYTE *)&opos, sizeof(UWORD), PARAM_3, 0, TO_APPL);
    return (SUCCESS);
    }



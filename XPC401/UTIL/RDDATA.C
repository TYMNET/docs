/************************************************************************
 * rddata.c - Read Data From Buflet Chain
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    rddata.c contains the module read_buflet_data() which is called
 *    to read data from a buflet chain.   It is called to read 
 *    character data, read session data and read packet data.
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

IMPORT VOID mov_param();                /* moves data to/from application
                                         * parameter buffers
                                         */

/************************************************************************
 * INT read_buflet_data(pb, nreq, nchar, ipos, opos)
 *    BUFLET *pb;	- pointer to start of buflet chain read
 *    UWORD nreq;	- number of characters requested 
 *    UWORD nchar;	- number of characters available to read
 *    UWORD ipos;	- starting input buffer position
 *    UWORD *opos;	- pointer to output buffer position
 *
 *    read_buflet_data() is called to move up to nreq number of
 *    bytes from the buflet chain pb to the application parameter
 *    buffer.  Processing when nreq number of characters have been
 *    read or there is no more data in the buflet chain.
 *
 * Usage notes: read_buflet_data() assumes that the data to be read
 *    is to be moved into the first parameter in the application
 *    packet.  
 *
 * Returns:  read_buflet_data() returns the number of characters
 *    read.
 *
 ************************************************************************/
INT read_buflet_data(pb, nreq, nchar, ipos, opos)
    BUFLET *pb;                 	/* pointer to buflet chain */
    UWORD nreq;				/* number of bytes required */
    UWORD nchar;              		/* number of characters to move */
    WORD ipos;	                 	/* start input position */
    UWORD *opos;			/* output position */
    {
    BUFLET *ppkt = pb;			/* pointer to buflet */
    WORD l, n;                		/* temporary counters */
    WORD nleft;				/* number of bytes to transfer */
    

    /* Determine the number of characters available for read.  The 
     * number of characters is either the number of characters 
     * requested or the number of characters available.
     */
    nleft =(WORD)(nreq > nchar ? nchar : nreq);
    
    /* Move data from packet to the 1st application parameter buffer.
     */
    for (l = nleft; (ppkt && l > 0); ppkt = ppkt->bufnext, ipos = 0)
        {
        n = (l > (DATA_BUF_SIZ - ipos)) ? (DATA_BUF_SIZ - ipos) : l;
        mov_param(&ppkt->bufdata[ipos], n, PARAM_1, *opos, TO_APPL);
        l  -= n;
        
	/* Increment the total number of characters processed.  If
	 * read_buflet_data is called multiple times, opos contains
	 * the total number of characters processed.
	 */
	*opos += n;
        }
	    
    /* Return the number of characters processed during this call to
     * read_buflet_data().
     */
    return((INT)(nleft - l));
    }



    
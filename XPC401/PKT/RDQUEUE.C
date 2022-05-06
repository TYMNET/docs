/************************************************************************
 * rdqueue.c - Read Packets From Queue
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    rdqueue.c contains the source modules which are called to
 *    process the incoming packet queue.
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
#include "state.h"
#include "pkt.h"
#include "appl.h"
#include "error.h"
 
IMPORT BUFLET *get_queue(); 		/* get next packet in queue */
IMPORT VOID add_queue();                /* put packet on queue */
IMPORT VOID data_packet();              /* process data packet */
IMPORT VOID discard_pkt();              /* discard packet */
IMPORT VOID echo_off();                 /* turn echo off */
IMPORT VOID echo_on();                  /* turn echo on */
IMPORT VOID edit_off();                 /* turn edit off */
IMPORT VOID edit_on();                  /* turn edit on */
IMPORT VOID green_ball();               /* received green ball */
IMPORT VOID int_disable();		/* disable interrupts */
IMPORT VOID int_enable();		/* enable interrupts */
IMPORT VOID orange_ball();              /* received orange ball */
IMPORT VOID pkt_in_error();		/* packet error when channel 
					 * is linked.
					 */
IMPORT VOID perm_echo_off();            /* permanent echo turned off */
IMPORT VOID perm_echo_on();             /* permanent echo turned on */
IMPORT VOID process_pkt();              /* process packet */
IMPORT VOID pkt_error();                /* illegal state for packet */
IMPORT VOID red_ball();                 /* received red ball */
IMPORT VOID recv_begin_brk();           /* begin break */
IMPORT VOID recv_clracp();		/* received session clear accept */
IMPORT VOID recv_end_brk();             /* end break */
IMPORT VOID recv_ssnacp();              /* received session accept */
IMPORT VOID recv_ssnclr();              /* received session clear */
IMPORT VOID recv_ssnreq();              /* received session request */
IMPORT VOID set_fchar();                /* set forward character */
IMPORT VOID set_ftimout();              /* set forward time out */
IMPORT VOID yellow_ball();              /* received yellow ball */

IMPORT CHNLINFO cis[];                  /* channel structure */
#ifdef DEBUG
IMPORT LCIINFO lcis[];			/* logical channel information
					 * structure.
					 */
#endif
IMPORT UBYTE log_chnl;			/* logical channel number */
#ifdef DEBUG
IMPORT WORD diags[];			/* diagsnostics which are used for
					 * debugging.
					 */
LOCAL WORD cnt = 421;
#endif

/* jump vector table which is used for control packets which 
 * are received when the session state is S4 ( data transfer )
 */
LOCAL VOID (*jmp_qpkt[MAX_PKT_FUNC + 1])() =
    { 0,                                /* unused */
      echo_on,                          /* enable echo */
      echo_off,                         /* disable echo */
      pkt_in_error,	                /* packet error */
      pkt_in_error,     	        /* packet error */
      green_ball,                       /* received green ball */
      red_ball,                         /* received red ball */
      recv_begin_brk,                   /* begin break */
      recv_end_brk,                     /* end break */
      0,                                /* received session request */
      0,                                /* received session accept */
      0,                                /* received session clear */
      0,                                /* received session clear acp */
      yellow_ball,                      /* received yellow ball */
      orange_ball,                      /* received orange ball */
      perm_echo_on,                     /* enable permanent echo */
      perm_echo_off,                    /* diable permanent echo */
      set_fchar,                        /* set forwarding character */
      set_ftimout,                      /* set forwarding time out */
      edit_on,                          /* enable local edit */
      edit_off};                        /* disable local edit */

/* jump vector table which is used when session control packets
 * are received.  Which of the packet subroutines are called
 * is dependent on the session state (S1-S7).  
 */
LOCAL VOID (*jmp_spkt[][4])() = 
    {{recv_ssnreq,                      /* State S1 recvd session request */ 
      discard_pkt,                      /* State S1 recvd session accept */    
      discard_pkt,                      /* State S1 received session clear */
      discard_pkt},                     /* State S1 received session clear acp */

     {recv_ssnreq,                      /* State S2 received session request */
      recv_ssnacp,                      /* State S2 received session accept */
      recv_ssnclr,                      /* State S2 received session clear */
      pkt_error},                       /* State S2 received session clear acp */
     
     {pkt_error,                        /* State S2 received session request */
      pkt_error,                        /* State S3 received session accept */
      recv_ssnclr,                      /* State S3 received session clear */
      pkt_error},                       /* State S3 received session clear acp */
     
     {pkt_error,                        /* State S4 received session request */
      pkt_error,                        /* State S4 received session accept */
      recv_ssnclr,                      /* State S4 received session clear */
      pkt_error},                       /* State S4 received session clear acp */
     
     {pkt_error,                        /* State S5 received session request */
      pkt_error,                        /* State S5 received session accept */
      pkt_error,                        /* State S5 received session clear */
      pkt_error},                       /* State S5 received session clear acp */
     
     {discard_pkt,                      /* State S6 received session request */
      discard_pkt,                      /* State S6 received session accept */
      recv_ssnclr,                      /* State S6 received session clear */
      recv_clracp},                     /* State S6 received session clear acp */
     
     {discard_pkt,                      /* State S7 received session request */
      discard_pkt,                      /* State S7 received session accept */
      discard_pkt,                      /* State S7 received session clear */
      discard_pkt}};                    /* State S7 received session clear acp */    
 
/************************************************************************
 * VOID process_queue(plcis)
 *    LCIINFO *plcis;	- pointer to logical channel information structure 
 *
 *    process_queue() processes packets from the pad input queue.
 *    Processing stops when the application read queue is full or
 *    the pad input queue is empty.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID process_queue(plcis)
    FAST LCIINFO *plcis;		/* pointer to logical channel
    					 * structure.
    					 */
    {
    CHNLINFO *pcis;			/* pointer to channel information
    					 * structure.
    					 */
    FAST BUFLET *ppkt;                  /* pointer to packet */

    /* Get buflet from the pad input queue and process the packet.
     * If the buflet is not accessed elsewhere, free the
     * buflet chain.
     */
    for (ppkt = plcis->inpktqueue.begqueue; ppkt;
	ppkt = plcis->inpktqueue.begqueue)
	{
		
	/* If this is a data packet and the application read queue
         * if full, then return.
	 */
	if ((ppkt->bufdata[GFI_LCI] & QBIT) == 0)
	    if (plcis->appchnl >= 0)
		{
		pcis = &cis[plcis->appchnl];
		if (pcis->nreadbytes >= MAX_RDQUEUE_CHARS)
	    	    break;
		}
	
	/* Read packet from pad input queue.
	 */
	ppkt = get_queue(&plcis->inpktqueue);
	log_chnl = ppkt->bufdata[GFI_LCI] & 0x0f;
	
	/* Process pad input packet.
	 */
	process_pkt(plcis, ppkt);
	}
    }

/************************************************************************
 * VOID process_pkt(plcis, ppkt)
 *    LCIINFO *plcis;	- pointer to logical channel information structure
 *    BUFLET *ppkt;	- pointer to packet received by pad
 *
 *    process_pkt processes an input pad packet pointed to by
 *    ppkt.  The type of processing performed depends on the
 *    type of packet (q-bit or data) and the type of pad function.
 *    Session packets are dependent on the current session
 *    state.
 *
 * Usage notes: When process_pkt() is processing a session handling 
 *    function, interrupts will be disabled before the function is
 *    called.  This is done to prevent a change in the channel state
 *    by a timer interrupt.  Iterrupts are enabled by the called
 *    function.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID process_pkt(plcis, ppkt)
    LCIINFO *plcis;			/* pointer to logical information
    					 * structure.
    					 */
    BUFLET *ppkt;                       /* packet to process */
    {
    BYTE pkt_func;                      /* packet function */
    UBYTE qbit;                         /* q bit extracted from packet */
 

    int_disable();
	
    /* If the input packet is a session handling packet
     * (session accept, session request, session clear etc.) then
     * call function depending on current session state.
     */
    if ((qbit = (ppkt->bufdata[GFI_LCI] & QBIT)) != 0)
        {
	if ((pkt_func = (BYTE)(ppkt->bufdata[PKT_TYP_ID] - START_PSSN_FUNC)) >= 0)
	    if (pkt_func <= NUM_PSSN_FUNC)
		{

		/* Table of subroutine calls which are indexed by session 
		 * state and packet function. A pointer to the  packet is
		 * passed to the subroutines.  The subroutines are responsible
		 * for turning interrupts back on.
                 */
                ((*jmp_spkt[plcis->ssnstate - 1][pkt_func])(plcis, ppkt));
		return;
		}
        }

    /* Packet was received when a session clear has been sent,
     * or a session clear has been confirmed/received.
     */  
    if (plcis->ssnstate == S6 || plcis->ssnstate == S7)
        discard_pkt(plcis, ppkt);

    /* Packet received when session is not in data transfer state.
     */
    else if (plcis->ssnstate == S1 || plcis->ssnstate == S3  || 
	plcis->ssnstate == S2 || plcis->ssnstate == S5)
        pkt_error(plcis, ppkt);

    /* Data packet has been received when session is in data transfer mode.
     */
    else if (qbit == 0)
	{
	int_enable();
	data_packet(&cis[plcis->appchnl], ppkt);
	}

    /* Q-bit packet has been received when session is in data transfer mode. 
     * Call subroutine indexed by function.  
     */    
    else
	{
	int_enable();
        ((*jmp_qpkt[ppkt->bufdata[PKT_TYP_ID]])(&cis[plcis->appchnl], ppkt));
	}
    }


/************************************************************************
 * VOID data_packet(ppkt)
 *    BUFLET *ppkt;	- pointer to data packet
 *
 *    data_packet() processes data packets received by the pad.
 *    The data  packets are added to the application read queue and the
 *    number of bytes in the application read queue is updated.  The
 *    data is read by the p_read_data() function.
 *
 * Returns:  None.
 *
 ************************************************************************/
VOID data_packet(pcis, ppkt)
    FAST CHNLINFO *pcis;		/* channel information structure */
    BUFLET *ppkt;                       /* pointer to data packet */
    {

    /* Basic logic for data_packet is to put the data packet
     * received by pad into the application read queue.
     * The first byte is moved to make it easier to read 
     * the data.
     */
    ppkt->bufdata[MOVED_FIRST_BYTE] = ppkt->bufdata[FIRST_DATA_BYTE];
    add_queue(&pcis->readqueue, ppkt);
    pcis->echopkt = NULLBUF;
    pcis->idxechodata = MOVED_FIRST_BYTE;
#ifdef DEBUG
    if (diags[cnt] > 30000)
	++cnt;
    diags[cnt] = diags[cnt] + ppkt->bufdata[FRAME_LEN] + 1;
#endif

    /* Update the number of bytes in the application read queue.
     */
    pcis->nreadbytes += ppkt->bufdata[FRAME_LEN] + 1;
    }



/************************************************************************
 * FMON.C - Monitor function processing
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    This module contains the functions called based on the function
 * call number passed to the driver, as well as the jump tables and
 * the function called by the interrupt routine.
 *
 * REVISION HISTORY:
 *
 *   Date    Version    By        Purpose of Revision
 * --------  ------- ----------   --------------------------------------
 * 03/04/87   4.00   S. Bennett   Initial Draft
 *
 ************************************************************************/
#include <stddef.h>
#include <appl.h>

#define IN 0
#define OUT 1

INT clr_device();
INT rd_devstat();
INT dev_reset();
INT set_charstate();
INT set_pktstate();
INT set_portparam();
INT rd_portparam();
INT set_dtr();
INT reset_dtr();
INT set_rts();
INT reset_rts();
INT test_dsr();
INT test_dcd();
INT test_ri();
INT send_break();
INT rpt_iostat();
INT input_data();
INT output_data();
INT rd_chanstate();
INT send_ssnreq();
INT send_ssnclr();
INT ssn_answerset();
INT send_ssnacc();
INT rd_ssndata();
INT set_int();
INT reset_int();
INT set_update();
INT reset_update();
INT rpt_padparam();
INT set_padparam();
INT flush_input();
INT flush_output();
INT link_stat();

LOCAL INT (*farray[])() = {
    clr_device,
    rd_devstat,
    dev_reset,
    set_charstate,
    set_pktstate,
    set_portparam,
    rd_portparam,
    set_dtr,
    reset_dtr,
    set_rts,
    reset_rts,
    test_dsr,
    test_dcd,
    test_ri,
    send_break,
    rpt_iostat,
    input_data,
    output_data,
    rd_chanstate,
    send_ssnreq,
    send_ssnclr,
    ssn_answerset,
    send_ssnacc,
    rd_ssndata,
    set_int,
    reset_int,
    set_update,
    reset_update,
    rpt_padparam,
    set_padparam,
    flush_input,
    flush_output,
    link_stat };

	
LOCAL TEXT *name[] = {
    "clear device       ",
    "read device status ",
    "device reset       ",
    "set character state",
    "set packet state   ",
    "set port params    ",
    "read port params   ",
    "set dtr            ",
    "reset dtr          ",
    "set rts            ",
    "reset rts          ",
    "test dsr           ",
    "test dcd           ",
    "test ri            ",
    "send break         ",
    "report i/o status  ",
    "input data         ",
    "output data        ",
    "read channel state ",
    "send session request",
    "send session clear  ",
    "session answer set  ",
    "send session accept ",
    "read session data   ",
    "set interrupt       ",
    "reset interrupt     ",
    "set update          ",
    "reset update        ",
    "report pad parameter",
    "set pad parameter   ",
    "flush input         ",
    "flush output        ",
    "link statistics     "
    };

LOCAL INT status;
LOCAL TEXT buff[92];
LOCAL BOOL inpacket = NO;
LOCAL INT closecount = 0;
LOCAL WORD num_rios = 0;
LOCAL UWORD last_incnt = 0;
LOCAL UWORD last_instt = 0;
LOCAL UWORD last_outstt = 0;
	
IMPORT UWORD param_block[];
IMPORT TEXT monname[];
IMPORT INT monfd;
IMPORT BOOL verbose;
IMPORT BOOL terse;
IMPORT BOOL packetonly;
IMPORT BOOL waitclose;
IMPORT BOOL inoutstatus;

into_app(func, dvc, chan)
    UWORD func;
    UWORD dvc;
    UWORD chan;
    {
    
    /* If we only record while in packet mode, and we aren't, return
     */
    if (packetonly && !inpacket)
	return;

    /* If we are in TERSE recording mode, do not write out info for
     * Read Device Status, Test DCD, Report I/O Status, or Read
     * Channel Status function calls.
     */
    if (terse && (func == 1 || func == 12 || func == 15 || func == 18))
	return;
    
    if (func < 0 || func > 32)
	{
	sprintf(buff, "--> Ill func %d\r\n", func);
	outbuff();
	}
    else
	{
	sprintf(buff, "--> %s    Chan = %3d    Device = %6d\r\n",
	    name[func], chan, dvc);
	outbuff();
	(*farray[func])(dvc, chan, IN);
	}
    }
    
out_app(func, dvc, chan)
    UWORD func;
    UWORD dvc;
    UWORD chan;
    {

    /* The following is an undocumented sneaky use of mov_param...
     */
    mov_param(&status, 2, 0, 0, FROM_APPL);
	
    /* If successfully did Set Character State, set inpacket false
     */
    if (status == 0 && func == 3)
	inpacket = NO;
    
    /* If successfully did Set Packet State, set inpacket true
     */
    if (status == 0 && func == 4)
	inpacket = YES;
    
    /* If we only record while in packet mode, and we aren't, return
     */
    if (packetonly && !inpacket)
	return;
    
    /* If function was report I/O Status, then load last_xxx params
     * and increment the number of calls to report I/O status.
     */
    if (func == 15)
	{
	mov_param(&last_incnt, sizeof(UWORD), PARAM_1, 0, FROM_APPL);
	mov_param(&last_instt, sizeof(UWORD), PARAM_2, 0, FROM_APPL);
	mov_param(&last_outstt, sizeof(UWORD), PARAM_3, 0, FROM_APPL);
	++num_rios;
	}
	
    /* If we are in TERSE recording mode, do not write out info for
     * Read Device Status, Test DCD, Report I/O Status, or Read
     * Channel Status function calls.
     */
    if (terse && (func == 1 || func == 12 || func == 15 || func == 18))
	return;
    
    if (func < 0 || func > 32)
	{
	sprintf(buff, "<-- Ill func %d\r\n", func);
	outbuff();
	}
    else
	{
	sprintf(buff, "<-- %s    Stat = %d\r\n", name[func], status);
	outbuff();
	(*farray[func])(dvc, chan, OUT);
	}
    }

clr_device(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    }
	
rd_devstat(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    int t1, t2;
    
    if (inout == IN)
	return;
    mov_param(&t1, sizeof(UWORD), PARAM_1, 0, FROM_APPL);
    mov_param(&t2, sizeof(UWORD), PARAM_1, 8, FROM_APPL);
    sprintf(buff, "     Device state = %3d   Port Address = %d\r\n", t1, t2);
    outbuff();
    }
	
dev_reset(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    int t1, t2;
    
    mov_param(&t1, sizeof(UWORD), PARAM_1, 0, FROM_APPL);
    mov_param(&t2, sizeof(UWORD), PARAM_1, 8, FROM_APPL);
    sprintf(buff, "     Device state = %3d   Port Address = %d\r\n", t1, t2);
    outbuff();
    }

set_charstate(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    }
    
set_pktstate(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    }
    
set_portparam(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    UWORD t1[10];
    
    if (inout == OUT)
	return;
    mov_param(t1, 6 * sizeof(UWORD), PARAM_1, 0, FROM_APPL);
    sprintf(buff, "     Baud Rate: %3d   Data Bits: %3d     Parity: %3d\r\n",
	t1[0], t1[1], t1[2]);
    outbuff();
    sprintf(buff, "     Stop Bits: %3d    XON/XOFF: %3d   Dxe Mode: %3d\r\n",
	t1[3], t1[4], t1[5]);
    outbuff();
    }
    
rd_portparam(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    UWORD t1[10];
    
    if (inout == IN)
	return;
    mov_param(t1, 6 * sizeof(UWORD), PARAM_1, 0, FROM_APPL);
    sprintf(buff, "     Baud Rate: %3d   Data Bits: %3d     Parity: %3d\r\n",
	t1[0], t1[1], t1[2]);
    outbuff();
    sprintf(buff, "     Stop Bits: %3d    XON/XOFF: %3d   Dxe Mode: %3d\r\n",
	t1[3], t1[4], t1[5]);
    outbuff();
    }
    
set_dtr(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    }
    
reset_dtr(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    }
    
set_rts(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    }
    
reset_rts(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    }
    
test_dsr(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    UWORD t1;
    
    if (inout == IN)
	return;
    mov_param(&t1, sizeof(UWORD), PARAM_1, 0, FROM_APPL);
    sprintf(buff, "     DSR = %d\r\n", t1);
    outbuff();
    }

test_dcd(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    UWORD t1;
    
    if (inout == IN)
	return;
    mov_param(&t1, sizeof(UWORD), PARAM_1, 0, FROM_APPL);
    sprintf(buff, "     DCD = %d\r\n", t1);
    outbuff();
    }

test_ri(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    UWORD t1;
    
    if (inout == IN)
	return;
    mov_param(&t1, sizeof(UWORD), PARAM_1, 0, FROM_APPL);
    sprintf(buff, "     RI = %d\r\n", t1);
    outbuff();
    }

send_break(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    }

rpt_iostat(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    
    if (inout == IN)
	return;
    sprintf(buff,
	"     Input count: %5d   Input status: %3d  Output Status %3d\r\n",
	last_incnt, last_instt, last_outstt);
    outbuff();
    }

input_data(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    UWORD t1, t2, t3;
    
    if (inout == IN)
	{
	mov_param(&t1, sizeof(UWORD), PARAM_2, 0, FROM_APPL);
	sprintf(buff, "     Bytes Requested: %d\r\n", t1);
	}
    else
	{
	mov_param(&t1, sizeof(UWORD), PARAM_3, 0, FROM_APPL);
	mov_param(&t2, sizeof(UWORD), PARAM_4, 0, FROM_APPL);
	sprintf(buff, "      Bytes Received: %5d   Type: %3d\r\n", t1, t2);
	if (inoutstatus)
	    {
	    outbuff();
	    sprintf(buff,
		"     RIOS# %5d   Incnt: %5d   Instt: %3d   Outstt: %3d\r\n",
		num_rios, last_incnt, last_instt, last_outstt);
	    num_rios = 0;
	    }
	if (t1 > 0 && verbose)
	    {
	    t3 = 0;
	    while (t1)
		{
		outbuff();
		strcpy(buff, "      Data Received:   ");
		t2 = t1;
		if (t2 > 32)
		    t2 = 32;
		mov_param(&buff[21], t2, PARAM_1, t3, FROM_APPL);
		buff[21 + t2] = '\r';
		buff[22 + t2] = '\n';
		buff[23 + t2] = '\0';
		t1 -= t2;
		t3 += t2;
		}
	    }
	}
    outbuff();
    }

output_data(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    UWORD t1, t2, t3;
    
    if (inout == OUT)
	{
	mov_param(&t1, sizeof(UWORD), PARAM_3, 0, FROM_APPL);
	sprintf(buff, "      Bytes Written: %d\r\n", t1);
	}
    else
	{
	mov_param(&t1, sizeof(UWORD), PARAM_2, 0, FROM_APPL);
	mov_param(&t2, sizeof(UWORD), PARAM_4, 0, FROM_APPL);
	sprintf(buff, "     Bytes to write: %5d   Type: %3d\r\n", t1, t2);
	if (inoutstatus)
	    {
	    outbuff();
	    sprintf(buff,
		"     RIOS# %5d   Incnt: %5d   Instt: %3d   Outstt: %3d\r\n",
		num_rios, last_incnt, last_instt, last_outstt);
	    num_rios = 0;
	    }
	if (t1 > 0 && verbose)
	    {
	    t3 = 0;
	    while (t1)
		{
		outbuff();
		strcpy(buff, "      Data Sending :   ");
		t2 = t1;
		if (t2 > 32)
		    t2 = 32;
		mov_param(&buff[21], t2, PARAM_1, t3, FROM_APPL);
		buff[21 + t2] = '\r';
		buff[22 + t2] = '\n';
		buff[23 + t2] = '\0';
		t1 -= t2;
		t3 += t2;
		}
	    }
	}
    outbuff();
    }

rd_chanstate(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    UWORD t1, t2;
    
    if (inout == IN)
	return;
    mov_param(&t1, sizeof(UWORD), PARAM_1, 0, FROM_APPL);
    mov_param(&t2, sizeof(UWORD), PARAM_2, 0, FROM_APPL);
    sprintf(buff, "     Channel State: %3d   Clear Code: %d\r\n", t1, t2);
    outbuff();
    }
    
send_ssnreq(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    UWORD t1, t2;
    TEXT t3[42];
    
    if (inout == OUT)
	return;
    mov_param(&t1, sizeof(UWORD), PARAM_2, 0, FROM_APPL);
    mov_param(&t2, sizeof(UWORD), PARAM_3, 0, FROM_APPL);
    mov_param(t3, ((t1 > 40) ? 40 : t1), PARAM_1, 0, FROM_APPL);
    t3[((t1 > 40) ? 40 : t1)] = '\0';
    sprintf(buff, "     Length of session data: %5d   Format: %d\r\n", t1, t2);
    outbuff();
    sprintf(buff, "     Session Data: %40s\r\n", t3);
    outbuff();
    }

send_ssnclr(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    UWORD t1, t2;

    if (inout == OUT)
	return;
    mov_param(&t1, sizeof(UWORD), PARAM_1, 0, FROM_APPL);
    mov_param(&t2, sizeof(UWORD), PARAM_2, 0, FROM_APPL);
    sprintf(buff, "     Clear code: %3d   Timeout: %d\r\n", t1, t2);
    outbuff();
    }

ssn_answerset(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    }
    
send_ssnacc(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    UWORD t1, t2;
    TEXT t3[42];
    
    if (inout == OUT)
	return;
    mov_param(&t1, sizeof(UWORD), PARAM_2, 0, FROM_APPL);
    mov_param(&t2, sizeof(UWORD), PARAM_3, 0, FROM_APPL);
    mov_param(t3, ((t1 > 40) ? 40 : t1), PARAM_1, 0, FROM_APPL);
    t3[((t1 > 40) ? 40 : t1)] = '\0';
    sprintf(buff, "     Length of accept data: %5d   Format: %d\r\n", t1, t2);
    outbuff();
    sprintf(buff, "     Accept Data: %40s\r\n", t3);
    outbuff();
    }

rd_ssndata(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    UWORD t1, t2;
    TEXT t3[42];
    
    if (inout == IN)
	{
	mov_param(&t1, sizeof(UWORD), PARAM_2, 0, FROM_APPL);
	sprintf(buff, "     Bytes Requested: %d\r\n", t1);
	}
    else
	{
	mov_param(&t1, sizeof(UWORD), PARAM_3, 0, FROM_APPL);
	mov_param(&t2, sizeof(UWORD), PARAM_4, 0, FROM_APPL);
	mov_param(t3, ((t1 > 40) ? 40 : t1), PARAM_1, 0, FROM_APPL);
	t3[((t1 > 40) ? 40 : t1)] = '\0';
	sprintf(buff, "     Bytes Received: %5d   Type: %3d\r\n", t1, t2);
	outbuff();
	sprintf(buff, "     Session Data: %40s\r\n", t3);
	}
    outbuff();
    }

set_int(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    UWORD t1, t2, t3, t4;
    
    if (inout == OUT)
	return;
    mov_param(&t1, sizeof(UWORD), PARAM_1, 0, FROM_APPL);
    mov_param(&t2, sizeof(UWORD), PARAM_2, 0, FROM_APPL);
    mov_param(&t3, sizeof(UWORD), PARAM_3, 0, FROM_APPL);
    if (t1 == 1)
	mov_param(&t4, sizeof(UWORD), PARAM_4, 0, FROM_APPL);
    sprintf(buff, "     Event: %2d    Segment: %5x   Offset: %5x\r\n",
	t1, t2, t3);
    outbuff();
    if (t1 == 1)
	{
	sprintf(buff, "     Timer length: %d\r\n", t4);
	outbuff();
	}
    }

reset_int(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    UWORD t1;
    
    mov_param(&t1, sizeof(UWORD), PARAM_1, 0, FROM_APPL);
    sprintf(buff, "    Event: %d\r\n", t1);
    outbuff();
    }
    
set_update(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    UWORD t1, t2, t3, t4;
    
    if (inout == OUT)
	return;
    mov_param(&t1, sizeof(UWORD), PARAM_1, 0, FROM_APPL);
    mov_param(&t2, sizeof(UWORD), PARAM_2, 0, FROM_APPL);
    if (t1 == 1)
	mov_param(&t3, sizeof(UWORD), PARAM_3, 0, FROM_APPL);
    sprintf(buff, "     Event: %2d    Address: %5x\r\n", t1, t2);
    outbuff();
    if (t1 == 1)
	{
	sprintf(buff, "     Timer length: %d\r\n", t3);
	outbuff();
	}
    }

reset_update(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    UWORD t1;
    
    mov_param(&t1, sizeof(UWORD), PARAM_1, 0, FROM_APPL);
    sprintf(buff, "    Event: %d\r\n", t1);
    outbuff();
    }
    
rpt_padparam(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    UWORD t1, t2;
    
    if (inout == IN)
	return;
    mov_param(&t1, sizeof(UWORD), PARAM_1, 0, FROM_APPL);
    mov_param(&t2, sizeof(UWORD), PARAM_2, 0, FROM_APPL);
    sprintf(buff, "    Param Number: %3d   Param Value: %d\r\n", t1, t2);
    outbuff();
    }

set_padparam(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    UWORD t1, t2;
    
    if (inout == OUT)
	return;
    mov_param(&t1, sizeof(UWORD), PARAM_1, 0, FROM_APPL);
    mov_param(&t2, sizeof(UWORD), PARAM_2, 0, FROM_APPL);
    sprintf(buff, "    Param Number: %3d   Param Value: %d\r\n", t1, t2);
    outbuff();
    }

flush_input(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    }
    
flush_output(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    }
    
link_stat(dvc, chan, inout)
    UWORD dvc;
    UWORD chan;
    UWORD inout;
    {
    }

outbuff()
    {
    if (monfd == -2)
	return;
    if (monfd == -1)
	{
	if ((monfd = iopen(monname, 1)) < 0)
	    {
	    monfd = -2;
	    return;
	    }
	ilseek(monfd, 0L, 2);
	}
    if (iwrite(monfd, buff, strlen(buff)) != strlen(buff))
	{
	iclose(monfd);
	monfd = -2;
	}
    if (!waitclose || ++closecount == 100)
	{
	iclose(monfd);
	monfd = -1;
	closecount = 0;
	}
    }

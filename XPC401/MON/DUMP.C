/************************************************************************
 * dump.c - X.PC line monitor data dump routines
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    This module contains routines used by the X.PC line monitor to
 * format and dump received data to the screen/disk.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KJB   Initial Draft
 *
 ************************************************************************/
#include "stddef.h"
#include "ctype.h"
#include "xpc.h"
#include "device.h"
#include "mon.h"
#include "pkt.h"
#include "state.h"
#define BSIZ            79		/* output buffer size */
#define PAGE		20		/* page length */

IMPORT BYTES fill();			/* returns its second argument */
IMPORT INT puts();			/* returns last character output */
IMPORT INT sprintf();			/* returns # of characters in buffer */
IMPORT INT write();			/* returns # of characters output */
IMPORT TEXT *pktmsg();			/* returns pointer to packet message */
IMPORT VOID chmod();			/* changes monitor mode */
IMPORT VOID int_disable();		/* disables interrupts */
IMPORT VOID int_enable();		/* enables interrupts */

IMPORT DEVSTAT dev_stat;		/* device status structure #1 */
IMPORT DEVSTAT dev_stat2;		/* device status structure #2 */
IMPORT INT dsoflag;			/* disable screen output flag */
IMPORT INT fd;				/* (optional) output file descriptor */
IMPORT UBYTE mask;			/* data byte mask */

LOCAL INT index;			/* output buffer index */
LOCAL INT line = 0;			/* line counter */
LOCAL INT mindex;			/* output buffer message index */
LOCAL INT offset;			/* output buffer offset */
LOCAL INT width = 24;			/* output buffer width */
LOCAL TEXT buf[BSIZ];			/* output buffer */
LOCAL TEXT chdr[] = 			/* character mode header */
    "TIME TEXT                     | TIME TEXT                     |";
LOCAL TEXT dchar;			/* direction character */
LOCAL TEXT phdr[] =			/* packet mode header */
    "TIME STX LEN GFI SEQ TYP CRC  | TIME STX LEN GFI SEQ TYP CRC  |";
LOCAL UBYTE hex[] =			/* hex character array */
    {"0123456789ABCDEF"};
    
/************************************************************************
 * LOCAL VOID output(pb, len)
 *     TEXT *pb;		pointer to buffer
 *     INT len;			length of output buffer
 *
 *     output is a static routine through which all X.PC line monitor
 * output is channeled. Screen output is performed via puts; if an
 * utput file has been specified, a write will be performed.
 *
 * Notes: Input parameters are not validated.
 *
 *     Output errors are ignored.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
LOCAL VOID output(pb, len)
    TEXT *pb;				/* pointer to buffer */
    INT len;				/* length of output buffer */
    {

    if (!dsoflag)			/* if screen output not disabled */
	{
	*(pb + len) = '\0';		/* buffer must be null terminated */
	(VOID)puts(pb);			/* output buffer to stdout (screen) */
	}
    if (0 <= fd)			/* if output to file specified */
	{
	*(pb + len) = '\n';		/* terminate with newline */
	(VOID)write(fd, pb, len + 1);	/* and output to file */
	}
    }

/************************************************************************
 * VOID hout()
 *     
 *     hout is called by the X.PC line monitor to output the appropriate
 * header upon page break.
 *
 * Notes: Only the device status structure for device #1 is read to
 *     determine monitor mode.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID hout()
    {

    /* if running in character mode, output the character mode header
     */
    if (dev_stat.devstate == CHAR_STATE)
	output(chdr, sizeof(chdr) - 1);
    
    /* else output the packet mode header, by default
     */
    else
	output(phdr, sizeof(phdr) - 1);
    line = 0;				/* reset line counter */
    }

/************************************************************************
 * LOCAL VOID ibuf()
 *
 *     ibuf is a static routine used to initialize the line monitor
 * output buffer.
 *
 * Notes: None.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
LOCAL VOID ibuf()
    {

    (VOID)fill(buf, sizeof(buf), ' ');	/* fill output buffer with spaces */
    buf[30] = buf[62] = '|';		/* install separator */
    buf[64] = dchar;			/* and direction characters */
    index = 0;				/* reset output index */
    mindex = 65;			/* reset message index */
    }

/************************************************************************
 * LOCAL VOID bout()
 *
 *     bout is a static routine used to output and reinitialize the line
 * monitor output buffer.
 *
 * Notes: None.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
LOCAL VOID bout()
    {
    
    output(buf, mindex);		/* output buffer */
    ++line;				/* bump line count */
    ibuf();				/* reinitialize output buffer */
    }
    
/************************************************************************
 * LOCAL VOID _cout(c)
 *     UBYTE c;			character to output
 *
 *     _cout is a static routine used to copy 1 character to the output
 * buffer. If index characters already exist in the buffer, the buffer
 * will be output and re-initialized before the character is installed.
 *
 * Notes: None.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
LOCAL VOID _cout(c)
    UBYTE c;				/* character to output */
    {
    
    if (index == width)			/* if width chars already installed */
	bout();				/* output buffer and re-initialize */
    buf[index++ + offset] = (TEXT)c;	/* copy character to buffer */
    }

/************************************************************************
 * LOCAL VOID cout(c, hflag)
 *     UBYTE c;			character to output
 *     BOOL hflag;		hex flag
 *
 *     cout is a static routine used to install 1 character in the output
 * buffer. If hflag is non-zero, the character is output as a 2 digit hex
 * value. If hflag is zero, the character c is checked. If printable, it
 * will be output in ascii; otherwise, it will be output as a 2 digit hex
 * value preceeded by a '$'.
 *
 * Notes: None.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
LOCAL VOID cout(c, hflag)
    UBYTE c;				/* character to output */
    BOOL hflag;				/* hex flag */
    {
    
    /* if the hex flag is non-zero, output the hex character for each
     * nibble in c
     */
    if (hflag)
	{
	_cout(hex[c >> 4]);
	_cout(hex[c & 0xf]);
	}
    
    /* else if c is printable, output c in ascii
     */
    else if (isprint(c))
	_cout(c);
    
    /* else output c in hex, preceeded by a '$'
     */
    else
	{
	_cout('$');
	_cout(hex[c >> 4]);
	_cout(hex[c & 0xf]);
	}
    }

/************************************************************************
 * LOCAL VOID mout(pm)
 *     TEXT *pm;		pointer to message
 *
 *     mout is a static routine used to copy the message string at pm to
 * the output buffer, stating at position mindex.
 *
 * Notes: None.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
LOCAL VOID mout(pm)
    TEXT *pm;				/* pointer to message text */
    {

    while (*pm)				/* for each character at pm */
	{
	if (*pm == '\n')		/* if a newline in encountered */
	    bout();			/* output buffer and re-initialize */
	else
	    {
	    if (mindex == BSIZ - 1)	/* if at end of buffer */
		bout();			/* output buffer and re-initialize */
	    buf[mindex++] = *pm;	/* copy character to output buffer */
	    }
	++pm;				/* next message character */
	}
    }

/************************************************************************
 * LOCAL VOID dump_char(pd)
 *     DATA *pd;		pointer to data entry
 *
 *     dump_char is a static routine used to dump data received in
 * character mode.
 *
 * Notes: None.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
LOCAL VOID dump_char(pd)
    DATA *pd;				/* pointer to data entry */
    {
    BUFLET *pb;				/* scratch buflet pointer */
    INT bdo;				/* buflet data offset */
    UBYTE len;				/* saves data length */
    FAST UBYTE *p;			/* running buflet data pointer */
    FAST x;				/* scratch int */
    
    buf[63] = 'X';			/* output dummy channel number */
    pb = pd->pbuf;			/* set pb to first buflet in chain */
    len = pb->bufdata[0];		/* save data length */
    bdo = 1;				/* skip length byte */
    while (len)				/* for len bytes of data */
        {
        p = &pb->bufdata[bdo];		/* set p to buflet data */
    
	/* calculate length of data to output for the current buflet
	 */
        x = min(len, DATA_BUF_SIZ - bdo);
        len -= x;			/* adjust total data length */
	
	/* output x data bytes
	 */
        do  {
            cout(*p++ & mask, NO);
            } while (--x);
        pb = pb->bufnext;		/* set pb to next buflet in chain */
        bdo = 0;			/* offset applicable to first buflet */
        }
    bout();				/* output trailing data */
    }

/************************************************************************
 * LOCAL VOID dump_drop(pd)
 *     DATA *pd;		pointer to data entry
 *
 *     dump_drop is a static routine used to dump data dropped in packet
 * mode.
 *
 * Notes: None.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
LOCAL VOID dump_drop(pd)
    DATA *pd;				/* pointer to data entry */
    {
    UBYTE len;				/* length of dropped data */
    FAST UBYTE *p;			/* pointer to dropped data */

    buf[63] = 'X';			/* output dummy channel number */
    mout("DROPPED");			/* output "dropped" message */
    p = pd->pbuf->bufdata;		/* set p to buflet data area */
    len = *p++;				/* save data length */
    do  {				/* output len data bytes */
	cout(*p++ & mask, NO);
	} while (--len);
    bout();				/* output trailing data */
    }

/************************************************************************
 * LOCAL VOID dump_pkt(pd)
 *     DATA *pd;		pointer to data entry
 *
 *     dump_pkt is a static routine used to dump data received in packet
 * mode.
 *
 * Notes: None.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
LOCAL VOID dump_pkt(pd)
    DATA *pd;				/* pointer to data entry */
    {
    BUFLET *pb;				/* scratch buflet pointer */
    INT bdo;				/* buflet data offset */
    UBYTE len;				/* saves data length */
    FAST UBYTE *p;			/* running buflet data pointer */
    FAST x;				/* scratch int */

    pb = pd->pbuf;			/* set pb to first buflet in chain */
    p = pb->bufdata;			/* set p to start of data area */
    cout(STX_CHAR, YES);		/* output STX character */
    index += 2;				/* bump index to next output field */
    len = *++p;				/* save frame length */
    cout(*p++, YES);			/* output frame length */
    index += 2;				/* bump index to next output field */
    cout(*p, YES);			/* output gfi/lci */
    buf[63] = hex[*p++ & 0xf];		/* output channel number */
    index += 2;				/* bump index to next output field */
    cout(*p++, YES);			/* output sequence numbers */
    index += 2;				/* bump index to next output field */
    cout(*p++, YES);			/* output packet type/id */
    index += 2;				/* bump index to next output field */
    cout(*p++, YES);			/* output crc 1 (first byte) */
    cout(*p++, YES);			/* output crc 1 (second byte) */
    mout(pktmsg(pb));			/* get packet message and output */
    bout();				/* output buffer and re-initialize */

    /* if len is non-zero (extra data exists) and the header crc is valid
     * (len should be valid)...
     */
    if (len && pb->bufdata[0] != MON_CRC1_ERR)
	{
	width = 18;			/* temporarily shorten width */
	bdo = EXTRA_DATA;		/* set buflet data offset */
	do  {				/* for len bytes of data */
	    p = &pb->bufdata[bdo];	/* set p to buflet data */
	    
	    /* calculate number of bytes to output from the current buflet
	     */
	    x = min(len, DATA_BUF_SIZ - bdo);
	    len -= x;			/* adjust total data length */
	    bdo += x;			/* adjust buflet data offset */
	    
	    /* output x data bytes
	     */
	    do  {
		cout(*p++ & mask, NO);
		} while (--x);
	    if (bdo == DATA_BUF_SIZ)	/* if entire buflet was output */
		{
		bdo = 0;		/* clear buflet data offset */
		pb = pb->bufnext;	/* set pb to next buflet in chain */
		}
	    } while (len);
	index = 20;			/* set index for second crc */
	width = 24;			/* reset output width */
	cout(pb->bufdata[bdo], YES);	/* output crc 2 (first byte) */
	    
	/* adjust buflet data offset; if DATA_BUF_SIZ, proceed to next
	 * buflet
	 */
	if (++bdo == DATA_BUF_SIZ)
	    {
	    bdo = 0;
	    pb = pb->bufnext;
	    }
	cout(pb->bufdata[bdo], YES);	/* output crc 2 (second byte) */
	bout();				/* output trailing data */
	}
    }

/************************************************************************
 * LOCAL VOID dump_data(pd)
 *     DATA *pd;		pointer to data entry
 *
 *     dump_pkt is a used by the X.PC line monitor to dump data retrieved
 * from the data received queue.
 *
 * Notes: None.
 *
 * Returns: Nothing.
 *
 ************************************************************************/
VOID dump_data(pd)
    DATA *pd;				/* pointer to data entry */
    {
    TEXT tbuf[5];			/* time buffer */

    /* if line count exceeds or equals page count, output header line
     */
    if (PAGE <= line)
	hout();

    /* set direction character and output buffer offset based on port
     * received
     */
    if (pd->port == RPORT1)
	{
	dchar = '<';
	offset = 0;
	}
    else
	{
	dchar = '>';
	offset = 32;
	}
    ibuf();				/* initialize output buffer */
    
    /* convert received time and install in output buffer
     */
    (VOID)sprintf(tbuf, "%04X", pd->time);
    buf[offset++] = tbuf[0];
    buf[offset++] = tbuf[1];
    buf[offset++] = tbuf[2];
    buf[offset++] = tbuf[3];
    ++offset;
    
    /* output data based on code
     */
    switch (pd->code)
        {
        case RPMODE:			/* received in packet mode */
            dump_pkt(pd);
            break;
        case RPDROP:			/* dropped in packet mode */
            dump_drop(pd);
            break;
	case RCMODE:			/* received in character mode */
        default:			/* should "never happen" */
            dump_char(pd);
            break;
        }
    }

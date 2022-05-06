/************************************************************************
 * MONIIO.C - CALLMON File I/O routines
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *
 *    Since it is highly undesirable to use standard file I/O routines
 *    in a Terminate and stay resident program (such as CALLMON) due to
 *    their tendency to require memory allocation ability or pre allocated
 *    buffers, we have written a set of very simple file I/O routines which
 *    interface directly to DOS, instead.
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00A   SB    Initial Draft
 *
 ************************************************************************/
#include <stddef.h>
#include <dos.h>

INT icreate(s)
    TEXT *s;
    {
    struct SREGS sr;
    union REGS ir;
    union REGS or;
    
    segread(&sr);
    ir.x.ax = 0x3c00;
    ir.x.cx = 0;
    ir.x.dx = s;
    intdosx(&ir, &or, &sr);
    if (or.x.cflag & 1)
	return(-1 * or.x.ax);
    iclose(or.x.ax);
    return(0);
    }

INT iopen(s, mode)
    TEXT *s;
    INT mode;		/* 0 = Read, 1 = Write, 2 = Update */
    {
    struct SREGS sr;
    union REGS ir;
    union REGS or;
    
    segread(&sr);
    ir.x.ax = 0x3d00 + mode;
    ir.x.dx = s;
    intdosx(&ir, &or, &sr);
    if (or.x.cflag & 1)
	return(-1 * or.x.ax);
    return(or.x.ax);
    }

INT iclose(fd)
    INT fd;
    {
    union REGS ir;
    union REGS or;
    
    ir.x.ax = 0x3e00;
    ir.x.bx = fd;
    intdos(&ir, &or);
    if (or.x.cflag & 1)
	return(-1 * or.x.ax);
    return(0);
    }

INT iread(fd, buff, len)
    INT fd;
    TEXT *buff;
    INT len;
    {
    struct SREGS sr;
    union REGS ir;
    union REGS or;

    segread(&sr);
    ir.x.ax = 0x3f00;
    ir.x.bx = fd;
    ir.x.cx = len;
    ir.x.dx = buff;
    intdosx(&ir, &or, &sr);
    if (or.x.cflag & 1)
	return(-1 * or.x.ax);
    return(or.x.ax);
    }

INT iwrite(fd, buff, len)
    INT fd;
    TEXT *buff;
    INT len;
    {
    struct SREGS sr;
    union REGS ir;
    union REGS or;

    segread(&sr);
    ir.x.ax = 0x4000;
    ir.x.bx = fd;
    ir.x.cx = len;
    ir.x.dx = buff;
    intdosx(&ir, &or, &sr);
    if (or.x.cflag & 1)
	return(-1 * or.x.ax);
    return(or.x.ax);
    }

LONG ilseek(fd, offset, type)
    INT fd;
    LONG offset;
    INT type;			/* 0 = Start-of-File relative,
    				 * 1 = Current position relative,
    				 * 2 = End-of-File relative.
    				 */
    {
    union REGS ir;
    union REGS or;

    ir.x.ax = 0x4200 + type;
    ir.x.bx = fd;
    ir.x.cx = offset >> 16;
    ir.x.dx = offset & 0xffff;
    intdos(&ir, &or);
    if (or.x.cflag & 1)
	return(-1 * or.x.ax);
    offset = or.x.dx;
    offset <<= 16;
    offset |= or.x.ax;
    return(offset);
    }

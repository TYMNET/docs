/************************************************************************
 * ai.h - Driver Test Program Defines
 * Copyright (C) 1987, Tymnet MDNSC
 * All Rights Reserved
 *
 * SUMMARY:
 *    ai.h contains the defines which are used by ai (the application
 *    test interface program).
 *
 * REVISION HISTORY:
 *
 *   Date    Version  By    Purpose of Revision
 * --------  ------- -----  ---------------------------------------------
 * 03/04/87   4.00    KS    Initial Draft
 *
 ************************************************************************/

#define FILLCHAR	(UBYTE)0xff
#define IOSIZ		127
#define NOFUNC		(BOOL (*)())0
#define NULLREQ		(REQ *)0
#define NPARAMS		8
#define PARSIZ		sizeof(WORD)

typedef struct
    {
    UWORD ds;
    WORD *fid;
    WORD *dev;
    WORD *chnl;
    WORD *scode;
    WORD *par1;
    WORD *par2;
    WORD *par3;
    WORD *par4;
    } REQ;

typedef struct
    {
    TEXT *name;
    BOOL (*func)();
    } FUNC;

#define ignore_chnl(c)	((c) < 15 || (23 < (c) && (c) != 34))

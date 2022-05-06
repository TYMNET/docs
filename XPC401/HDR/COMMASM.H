;************************************************************************
;* commasm.h - IOCOMM assembly equates
;* Copyright (C) 1987, Tymnet MDNSC
;* All Rights Reserved
;*
;* SUMMARY:
;*    commasm.h contains the defines which are used communications.
;*
;* REVISION HISTORY:
;*
;*   Date    Version  By    Purpose of Revision
;* --------  ------- -----  ---------------------------------------------
;* 03/04/87   4.00    SAB    Initial Draft
;*
;************************************************************************/
;
;
; Offsets into the COMMINFO struct
;
; NOTE - if these change, so also must iocomm.h
;
;
XMTPTR		EQU	0	; BUFLET *xmtptr[XMT_BUF_CHAINS + 1]
XMTLEN		EQU	8	; UWORD xmtlen[XMT_BUF_CHAINS + 1]
XMTFLG          EQU	16	; BOOL xmtflg[XMT_BUF_CHAINS + 1]
XMTACTIVE	EQU	24	; BOOL xmtactive
PACEACTIVE	EQU	26	; BOOL paceactive
XOFFRECVD	EQU	28	; BOOL xoffrecvd
XMTXOFF		EQU	30	; WORD xmtxoff
 
BEGRECVBUF	EQU	32	; UBYTE *begrecvbuf
ENDRECVBUF	EQU	34	; UBYTE *endrecvbuf
WRTRECVBUF	EQU	36	; UBYTE *wrtrecvbuf
READRECVBUF	EQU	38	; UBYTE *readrecvbuf
COMMENABLED	EQU	40	; BOOL commenabled
 
MDMSTATUS	EQU	42	; UBYTE mdmstatus
LINESTATUS	EQU	43	; UBYTE linestatus
 
LINKSTATS	EQU	44	; UWORD linkstats[NBR_LINKSTATS]

BREAKRECVD	EQU	58	; BOOL breakrecvd
ERRORRECVD	EQU	60	; BOOL errorrecvd
 
CTSCHECKING	EQU	62	; BOOL ctschecking

STARTPACE	EQU	64	; UBYTE startpace
ENDPACE		EQU	65	; UBYTE endpace

OLDCOMMCS	EQU	66	; UWORD oldcommcs
OLDCOMMIP	EQU	68	; UWORD oldcommip

SAVEIER		EQU	70	; UBYTE saveier
SAVEMCR		EQU	71	; UBYTE savemcr
SAVEPIC		EQU	72	; UBYTE savepic
 
; 1 byte automatic gap...

COMMBASE	EQU	74	; UWORD commbase
COMMINTVEC	EQU	76	; UBYTE commintvec
COMMINTDISABLE	EQU	77	; UBYTE commintdisable

XMTIDX		EQU	78	; UBYTE xmtidx

COMMINFOSIZ	EQU	79	; sizeof(COMMINFO)

;
; Timer handling defines
;
XOFF_TIMEOUT_LEN	EQU	1	; 10 seconds
XOFF_TIMETYPE		EQU	2	; 10 second timer
TIM_XOFFRECVD		EQU	010H	; Timer class for xoff received

;
; Various other defines
;
XMTBUFSIZE	EQU	3	; XMT_BUF_SIZE
DATABUFSIZ	EQU	50	; DATA_BUF_SIZ
PIC0		EQU	020H	; Programmable Interrupt Controller #0
PIC1		EQU	021H	; Programmable Interrupt Controller #1

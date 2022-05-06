;************************************************************************
;* IOC2.ASM - Communications interrupt handler routines.
;* Copyright (C) 1987, Tymnet MDNSC
;* All Rights Reserved
;*
;* SUMMARY:
;*     
;*     Contains the interrupt routines required to process communications
;*     interrupts of all types, including transmit interrupts.  These
;*     routines interact directly with the 8250 Asynchronous Comm chip,
;*     and the 8259 Programmable Interrupt Controller.  
;*
;*     NOTE: Sections of this code were produced by running a template
;*     program (IOCOMM.C) through the Microsoft C compiler and hand
;*     optomizing or replacing where necessary.
;*
;*     THIS MODULE IS VIRTUALLY IDENTICAL TO IOCOMM.C, SAVE FOR FUNCTION
;*     NAMES AND THE USE OF _comm_info2 INSTEAD OF _comm_info TO ALLOW FOR
;*     THE LINE MONITOR TO USE TWO COMM PORTS SIMULTANEOUSLY.
;*
;*     THIS MODULE SHOULD *ONLY* BE USED AS PART OF THE LINE MONITOR.
;*
;* REVISION HISTORY:
;*
;*   Date    Version      By       Purpose of Revision
;* --------  ------- ------------  ----------------------------------------
;* 03/04/87   4.00    S. Bennett    Initial Draft
;*
;************************************************************************


	TITLE   ioc2

include	commasm.h			; contains equivilences to iocomm.h

;
; Microsoft C segment declarations
; (Produced with the Microsoft C 4.0 compiler)
;
_TEXT	SEGMENT  BYTE PUBLIC 'CODE'
_TEXT	ENDS
_DATA	SEGMENT  WORD PUBLIC 'DATA'
_DATA	ENDS
CONST	SEGMENT  WORD PUBLIC 'CONST'
CONST	ENDS
_BSS	SEGMENT  WORD PUBLIC 'BSS'
_BSS	ENDS
DGROUP	GROUP	CONST,	_BSS,	_DATA
	ASSUME  CS: _TEXT, DS: DGROUP, SS: DGROUP, ES: DGROUP

;
; External references.
; (Also produced via MSC v4.0.
; I have no idea why _comm_info was declared as BYTE)
;
EXTRN	_comm_info2:BYTE

;
; Static (LOCAL) references
;
_BSS      SEGMENT
left_in_buflet	DW 01H DUP (?)
xmt_point	DW 01H DUP (?)
xmtbufptr	DW 01H DUP (?)
_BSS      ENDS

;
; Start of code segment
;
_TEXT      SEGMENT

;
; The following external reference is the data segment for the X.PC driver.
; It is here in the code segment because it is declared in the code segment.
; This must be the case, otherwise the interrupt routines would not be able
; to find it.
;
EXTRN	_xpc_ds:WORD				; The data segment for xpc.



;************************************************************************
;* VOID com2_interrupt()
;*
;*     This is the main communications interrupt server for the X.PC
;*     driver.  It is written in assembly language for speed.  This is
;*     called automatically by the 8088 chip upon receiving a hardware
;*     interrupt from the comm chip.  It's address is placed in the
;*     interrupt vector table by enable_comm() in IOUTIL.C
;*
;* Notes: This really isn't a C callable routine and should only be
;*     called by the hardware interrupt routine.  Hardware interrupts
;*     of higher priority than the serial interrupt are still enabled,
;*     but this is quite likely to change as this could cause problems with
;*     timer interrupts.
;*
;* Returns:  none
;*
;************************************************************************

	PUBLIC	_com2_interrupt
_com2_interrupt	PROC FAR
	push	ax				; save registers
	push	bx
	push	cx
	push	dx
	push	ds
	push	es
	
	mov	ds,cs:_xpc_ds			; Load the XPC data segment
	mov	es,cs:_xpc_ds			; ES is the same thing.


;
; Here we read the Interrupt Identification Register, to determine if there
; are any more interrupts to process.  This is done in a loop until there
; are no more such interrupts, based on the INS-8250 bugs list, which notes
; that multiple interrupts may only be visible as a single interrupt to the
; 8259...
;
ciloop:
	mov	ax,WORD PTR _comm_info2+COMMBASE
	add	ax,2				; IIR_PORT
	mov	dx,ax
	in	al,dx				; Read the Interrupt ID Reg
	test	al,1				; Is there another interrupt?
	je	ciint				; Yes... Process it
	jmp	ciend				; No... End

ciint: 
	and	ax,6
	shr	ax,1				; Get the ID bits only

	cmp	ax,1				; Is it a transmit interrupt?
	jne	cinotxmt			; No...
	call	_xmt2_one_char			; Otherwise, transmit a char
	jmp	ciloop

cinotxmt:
	cmp	ax,2				; Is it a receive interrupt?
	je	cirecv				; Yes...
	jmp	cinotrecv			; No...

cirecv:
	mov	dx,WORD PTR _comm_info2+COMMBASE
	in	al,dx				; Otherwise, Get received char

	cmp	WORD PTR _comm_info2+PACEACTIVE,0
	je	cinopace			; If no pacing, skip

	cmp	al,BYTE PTR _comm_info2+STARTPACE
	jne	cipace2				; If not XOFF, then check XON

	;
	; Start pacing:  We have received an XOFF  (or whatever alternate
	; pacing character we may have be told to watch for...) and must turn
	; off the transmitter.  Easy.  We must also start the Paced off timer.
	; Not quite as easy.  We call start_timer() to get the job done.
	;
	; Note that the XOFF character is discarded
	;
	mov	WORD PTR _comm_info2+XOFFRECVD,1	; Turn off transmitter.
	jmp	ciloop				; continue the loop

cipace2:
	cmp	al,BYTE PTR _comm_info2+ENDPACE	; If incoming character is not
	jne	cinopace			; end pacing (XON), then skip

	;
	; Stop pacing: Here we have received an XON (or whatever...) and have
	; to restart the transmitter and stop the paced off timer.  Turning
	; off the timer is accomplished via stop_timer(), and turning on the
	; transmitter is accomplished by enable_xmt().
	;
	; Note that the XON is then discarded
	;
	mov	WORD PTR _comm_info2+XOFFRECVD,0	; Set flag to allow transmit
	jmp	ciloop

cinopace:

	;
	; Received a character which is not a pacing character:  Put it into
	; our circular read queue at the write pointer into that queue, and
	; increment that pointer. 
	;
	mov	bx,WORD PTR _comm_info2+WRTRECVBUF
	mov	[bx],al				; Save it in circular buffer
	inc	bx				; If ptr + 1 > end of buffer
	cmp	bx,WORD PTR _comm_info2+ENDRECVBUF
	jbe	ciincptr
	mov	ax,WORD PTR _comm_info2+BEGRECVBUF	; set to beginning
	mov	WORD PTR _comm_info2+WRTRECVBUF,ax
	jmp	ciloop

ciincptr:
	mov	WORD PTR _comm_info2+WRTRECVBUF,bx	; else increment
	jmp	ciloop

cinotrecv:
	cmp	ax,3				; is it a line status int?
	jne	cinotline
	mov	ax,WORD PTR _comm_info2+COMMBASE
	add	ax,5				; LSR_PORT
	mov	dx,ax
	in	al,dx				; get line status
	mov	BYTE PTR _comm_info2+LINESTATUS,al

	test	ax,16				; is there break? (LSR_BREAK)
	je	ciline1
	mov	WORD PTR _comm_info2+BREAKRECVD,1	; set received break
	inc	WORD PTR _comm_info2+LINKSTATS+12	; inc # of breaks

ciline1:
	test	ax,8				; framing error? (LSR_FRAMING)
	je	ciline2
	mov	WORD PTR _comm_info2+ERRORRECVD,1
	inc	WORD PTR _comm_info2+LINKSTATS+2 

ciline2:
	test	ax,2				; Overrun err? (LSR_OVERRUN)
	je	ciline3
	mov	WORD PTR _comm_info2+ERRORRECVD,1
	inc	WORD PTR _comm_info2+LINKSTATS+4

ciline3:
	test	ax,4				; Parity err? (LSR_PARITY)
	je	ciline4
	mov	WORD PTR _comm_info2+ERRORRECVD,1
	inc	WORD PTR _comm_info2+LINKSTATS+6

ciline4:
	jmp	ciloop

cinotline:					; must be mdm status int
	mov	ax,WORD PTR _comm_info2+COMMBASE
	add	ax,6				; MSR_PORT
	mov	dx,ax
	in	al,dx				; Get modem status
	mov	BYTE PTR _comm_info2+MDMSTATUS,al	; Save it.
	jmp	ciloop

ciend:
	mov	dx,PIC0				; clear interrupt from PIC
	mov	ax,020H
	out	dx,al

	pop	es
	pop	ds				; restore registers
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	iret	

_com2_interrupt	ENDP




;************************************************************************
;* VOID xmt_one_char()
;*
;*     Transmits one character from the transmit queue, and adjusts all
;*     necessary pointers and counters.  The transmit queue is actually
;*     an array of pointers and an array of corresponding lengths.  The
;*     pointers are to buflet chains which contain the data.  For a full
;*     description of this queue and it's workings, see the IOCOMM design
;*     specification.
;*
;* Notes: While this routine does follow MSC calling requirements, and
;*     would work if called directly, instead of by the interrupt handler,
;*     this practice is not recommended.
;*
;* Returns:  none
;*
;************************************************************************
	PUBLIC	_xmt2_one_char

_xmt2_one_char	PROC NEAR

	push	bp					; setup C sequence
	mov	bp,sp
	push	di					; save other regs
	push	si

	;
	; If we must send a Pace off or Pace on character, do so
	; and finish up.
	;
	cmp	WORD PTR _comm_info2+XMTXOFF,0
	je	xoc3

	cmp	WORD PTR _comm_info2+XMTXOFF,1		; = SEND_PACEON
	jne	xoc1

	mov	al,BYTE PTR _comm_info2+STARTPACE	; Send pace off
	mov	bx,2
	jmp	xoc2

xoc1:
	cmp	WORD PTR _comm_info2+XMTXOFF,3		; = SEND_PACEOFF
	jne	xoc3
	mov	al,BYTE PTR _comm_info2+ENDPACE		; Send pace on
	xor	bx,bx

xoc2:
	mov	dx,WORD PTR _comm_info2+COMMBASE
	out	dx,al
	mov	WORD PTR _comm_info2+XMTXOFF,bx
	jmp	xocend

xoc3:
	;
	; If we have received an pace off, turn off transmit.
	; (comm_info2.xoffrecvd is set by Character mode link)
	;
	cmp	WORD PTR _comm_info2+XOFFRECVD,0
	je	xoc4
	jmp	xmtdeactivate

xoc4:
	;
	; If we are checking Clear to Send (CTS), and CTS is not
	; set, turn off transmit.  Later, we may add a status flag
	; here to warn Link.
	;
	cmp	WORD PTR _comm_info2+CTSCHECKING,0
	je	xoc5
	test	BYTE PTR _comm_info2+MDMSTATUS,16
	jne	xoc5
	jmp	xmtdeactivate

xoc5:
	cmp	WORD PTR _comm_info2+XMTLEN,0
	jne	xoc6

	cmp	WORD PTR _comm_info2+XMTLEN+2,0
	jne	xoc5a
	jmp	xmtdeactivate

xoc5a:
	mov	ax,WORD PTR _comm_info2
	mov	WORD PTR _comm_info2+XMTPTR+(XMTBUFSIZE*2),ax
	mov	WORD PTR _comm_info2+XMTLEN+(XMTBUFSIZE*2),0

	mov	cx,XMTBUFSIZE
	mov	si,OFFSET _comm_info2
	mov	di,si
	add	di,XMTLEN

xocloop:
	mov	ax,[di+2]
	mov	[di],ax
	mov	ax,[si+2]
	mov	[si],ax
	add	si,2
	add	di,2
	loop	xocloop

	mov	ax,WORD PTR _comm_info2
	mov	xmtbufptr,ax
	add	ax,4
	add	ax,WORD PTR _comm_info2+XMTIDX
	mov	xmt_point,ax
	mov	ax,DATABUFSIZ
	sub	ax,WORD PTR _comm_info2+XMTIDX
	mov	left_in_buflet,ax

xoc6:
	cmp	left_in_buflet,0
	jne	xoc8
	
	mov	cx,0
	mov	bx,xmtbufptr
	cmp	bx,0
	jne	xoc7
	mov	ax,WORD PTR _comm_info2
	mov	cx,WORD PTR _comm_info2+XMTIDX
	jmp	xoc7a
xoc7:
	mov	ax,[bx]
xoc7a:
	mov	xmtbufptr,ax
	add	ax,4
	add	ax,cx
	mov	xmt_point,ax
	mov	ax,DATABUFSIZ
	sub	ax,cx
	mov	left_in_buflet,ax

xoc8:
	mov	bx,xmt_point
	mov	al,[bx]
	mov	dx,WORD PTR _comm_info2+COMMBASE
	out	dx,al

	inc	xmt_point
	dec	left_in_buflet
	dec	WORD PTR _comm_info2+XMTLEN
	jne	xocend
	mov	xmtbufptr,0
	mov	left_in_buflet,0
	jmp	xocend

xmtdeactivate:
	mov	WORD PTR _comm_info2+XMTACTIVE,0

xocend:
	pop	si
	pop	di
	mov	sp,bp
	pop	bp
	ret	

_xmt2_one_char	ENDP
_TEXT	ENDS
END

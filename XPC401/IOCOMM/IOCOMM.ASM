;************************************************************************
;* IOCOMM.ASM - Communications interrupt handler routines.
;* Copywrite (C) 1987, Tymnet MDNSC
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
;* REVISION HISTORY:
;*
;*   Date    Version      By       Purpose of Revision
;* --------  ------- ------------  ----------------------------------------
;* 03/04/87   4.00    S. Bennett    Initial Draft
;*
;************************************************************************


	TITLE   iocomm

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
; (Partially produced via MSC v4.0.
; I have no idea why _comm_info was declared as BYTE)
;
EXTRN	_comm_info:BYTE
EXTRN   _xmt_intcnt:WORD
;
; Diagnostic references
;
;EXTRN	_par1:WORD				; XXX
;EXTRN	_par3:WORD				; XXX

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
;* VOID comm_interrupt()
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

	PUBLIC	_comm_interrupt
_comm_interrupt	PROC FAR
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
	mov	ax,WORD PTR _comm_info+COMMBASE
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
	call	_xmt_one_char			; Otherwise, transmit a char
	jmp	ciloop

cinotxmt:
	cmp	ax,2				; Is it a receive interrupt?
	je	cirecv				; Yes...
	jmp	cinotrecv			; No...

cirecv:
	mov	dx,WORD PTR _comm_info+COMMBASE
	in	al,dx				; Otherwise, Get received char

	cmp	WORD PTR _comm_info+PACEACTIVE,0
	je	cinopace			; If no pacing, skip

	cmp	al,BYTE PTR _comm_info+STARTPACE
	jne	cipace2				; If not XOFF, then check XON
;	inc	_par1				; XXX

	;
	; Start pacing:  We have received an XOFF  (or whatever alternate
	; pacing character we may have be told to watch for...) and must turn
	; off the transmitter.  Easy.
	;
	; Note that the XOFF character is discarded
	;
	mov	WORD PTR _comm_info+XOFFRECVD,1	; Turn off transmitter.
	jmp	ciloop				; continue the loop

cipace2:
	cmp	al,BYTE PTR _comm_info+ENDPACE	; If incoming character is not
	jne	cinopace			; end pacing (XON), then skip
;	inc	_par3				; XXX

	;
	; Stop pacing: Here we have received an XON (or whatever...) and have
	; to restart the transmitter.  We can't do it directly, but we turn
	; off the xoffrecvd flag, which lets Link kick the machinery again.
	;
	; Note that the XON is then discarded
	;
	mov	WORD PTR _comm_info+XOFFRECVD,0	; Set flag to allow transmit
	jmp	ciloop

cinopace:

	;
	; Received a character which is not a pacing character:  Put it into
	; our circular read queue at the write pointer into that queue, and
	; increment that pointer. 
	;
	mov	bx,WORD PTR _comm_info+WRTRECVBUF
	mov	[bx],al				; Save it in circular buffer
	inc	bx				; If ptr + 1 > end of buffer
	cmp	bx,WORD PTR _comm_info+ENDRECVBUF
	jbe	ciincptr
	mov	ax,WORD PTR _comm_info+BEGRECVBUF	; set to beginning
	mov	WORD PTR _comm_info+WRTRECVBUF,ax
	jmp	ciloop

ciincptr:
	mov	WORD PTR _comm_info+WRTRECVBUF,bx	; else increment ptr
	jmp	ciloop

cinotrecv:
	cmp	ax,3				; is it line status interrupt?
	jne	cinotline
	mov	ax,WORD PTR _comm_info+COMMBASE
	add	ax,5				; address for LSR_PORT
	mov	dx,ax
	in	al,dx				; get line status
	mov	BYTE PTR _comm_info+LINESTATUS,al

	test	ax,16				; is there break? (LSR_BREAK)
	je	ciline1
	mov	WORD PTR _comm_info+BREAKRECVD,1	; set received break
	inc	WORD PTR _comm_info+LINKSTATS+12	; inc # of breaks

ciline1:
	test	ax,8				; framing error? (LSR_FRAMING)
	je	ciline2
	mov	WORD PTR _comm_info+ERRORRECVD,1	; set error received
	inc	WORD PTR _comm_info+LINKSTATS+2 	; inc # framing errs

ciline2:
	test	ax,2				; Overrun err? (LSR_OVERRUN)
	je	ciline3
	mov	WORD PTR _comm_info+ERRORRECVD,1	; set error received
	inc	WORD PTR _comm_info+LINKSTATS+4		; inc # overrun errs

ciline3:
	test	ax,4				; Parity err? (LSR_PARITY)
	je	ciline4
	mov	WORD PTR _comm_info+ERRORRECVD,1	; set error received
	inc	WORD PTR _comm_info+LINKSTATS+6		; inc # parity errs

ciline4:
	jmp	ciloop

cinotline:					; must be mdm status int
	call	mdm_status
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

_comm_interrupt	ENDP




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
	PUBLIC	_xmt_one_char

_xmt_one_char	PROC NEAR

	push	bp					; setup C sequence
	mov	bp,sp
	push	di					; save other regs
	push	si
	
	; 
	; Reset the number of timer ticks since the last transmit interrupt.
	;
	mov	ax,0
	mov	_xmt_intcnt,ax
	
	;
	; If we must send a Pace off or Pace on character, do so
	; and finish up.
	;
	cmp	WORD PTR _comm_info+XMTXOFF,0		; if NO_PACE_OUT
	je	xoc3					;    then skip pacing

	cmp	WORD PTR _comm_info+XMTXOFF,1		; if not SEND_STARTPACE
	jne	xoc1					;    skip

	mov	al,BYTE PTR _comm_info+STARTPACE	; else send startpace
	mov	bx,2					;    & set state to
	jmp	xoc2					;    SENT_STARTPACE

xoc1:
	cmp	WORD PTR _comm_info+XMTXOFF,3		; if not SEND_ENDPACE
	jne	xoc3					;    Skip pacing
	mov	al,BYTE PTR _comm_info+ENDPACE		; Then send endpace
	xor	bx,bx					;    & set state to
							;    NO_PACE_OUT

xoc2:
	mov	dx,WORD PTR _comm_info+COMMBASE         ; Send the pace char
	out	dx,al
	mov	WORD PTR _comm_info+XMTXOFF,bx		; Set the new state
	jmp	xocend					; clean up & return

xoc3:
	;
	; If we have received a start pacing character, turn off transmit.
	;
	cmp	WORD PTR _comm_info+XOFFRECVD,0		; If not paced
	je	xoc4					;    skip to next test
	jmp	xmtdeactivate				; else turn off xmit

xoc4:
	;
	; If we are checking Clear to Send (CTS), and CTS is not
	; set, turn off transmit.  Later, we may add a status flag
	; here to warn Link.
	;
	cmp	WORD PTR _comm_info+CTSCHECKING,0	; If not checking CTS
	je	xoc5					;    Skip to next
	call	mdm_status
	test	BYTE PTR _comm_info+MDMSTATUS,16	; If CTS is set
	jne	xoc5					;    skip to next
	jmp	xmtdeactivate				; else turn off xmit

xoc5:
	;
	; Here we handle the transmit buflet chain queue.  If the first
	; entry in the queue is empty, then we are finished transmitting the
	; current buflet chain.  (If not empty, we *might* be finished, but
	; that is handled farther below.)  If both the first and second
	; entries in this queue are empty (ie. have a zero XMTLEN)
	; then the entire queue is empty and we are done transmitting, so
	; we turn off the transmitter.
	;
	cmp	WORD PTR _comm_info+XMTLEN,0		; If first entry not
	jne	xoc6					;   empty, skip

	cmp	WORD PTR _comm_info+XMTLEN+2,0
	jne	xoc5a
	jmp	xmtdeactivate

xoc5a:
	;
	; At this point, the first entry in the queue is empty, but there
	; is at least one more chain in the queue.  So we must shift
	; everything in the queue forward by one space.  We also rotate the
	; pointer to the original first chain (which was empty) to the last
	; position, where Link can later access it for disposal if needed.
	; This is accomplished by a hidden space in the queue beyond the end
	; as shown in the diagram below:
	;
	;     +-------------------------------------------------+
	;     |                                                 |
	;     V                                                 |
	;  +------+    +------+                 +------+    +------+
	;  |      |--->|      |----/  ...   /-->|      |--->|      |
	;  +------+    +------+                 +------+    +------+
	;   N + 1          N                       2           1
	;
	; ...for a queue with N entries.  Note that the movement from 1 to
	; N+1 is done first, then entries 2 through N+1 are shifted one space
	; down.
	;

	;
	; Move first entry to space beyond last entry.
	;
	mov	ax,WORD PTR _comm_info+XMTPTR
	mov	WORD PTR _comm_info+XMTPTR+(XMTBUFSIZE*2),ax
	mov	WORD PTR _comm_info+XMTLEN+(XMTBUFSIZE*2),0
	mov	ax,WORD PTR _comm_info+XMTFLG
	mov	WORD PTR _comm_info+XMTFLG+(XMTBUFSIZE*2),ax

	;
	; Set up a loop based on number of entries and perform the shift
	;
	mov	cx,XMTBUFSIZE				; # of entries
	mov	si,OFFSET _comm_info			; Start of pointers

xocloop:
	mov	ax,[si+XMTPTR+2]			; Get ptr[i+1]
	mov	[si+XMTPTR],ax				; Set ptr[i]=ptr[i+1]
	mov	ax,[si+XMTLEN+2]			; Get len[i+1]
	mov	[si+XMTLEN],ax				; Set len[i]=len[i+1]
	mov	ax,[si+XMTFLG+2]			; Get flg[i+1]
	mov	[si+XMTFLG],ax				; Set flg[i]=flg[i+1]
	add	si,2					; increment "i"
							; (actually the start
							;  pointers)
	loop	xocloop					; Loop CX times.

	;
	; Finally we have to assign our local pointers and counters to the
	; new buflet chain to transmit.  This is now the first entry in the
	; queue.
	;
	mov	ax,WORD PTR _comm_info+XMTPTR	; Set current buflet to first
	mov	xmtbufptr,ax			;   buflet in the chain.
	add	ax,4				; Set current byte transmit
	add	ax,WORD PTR _comm_info+XMTIDX	;   pointer to data area of
	mov	xmt_point,ax			;   xmtbufptr plus xmtidx.
	mov	ax,DATABUFSIZ			; Set amount of bytes left in
	sub	ax,WORD PTR _comm_info+XMTIDX	;   the buflet to the buflet
	mov	left_in_buflet,ax		;   size minue xmtidx.

xoc6:
	;
	; Now that the transmit buflet chain queue is okay, or was okay to
	; begin with, we check the amount left in our current buflet.
	; If zero, and the buflet chain pointer is zero, then we assign the
	; pointers and counters to the first chain in the queue.
	; If the chain pointer is not zero, we get the next buflet in the
	; chain.
	;
	cmp	left_in_buflet,0
	jne	xoc8
	
	mov	cx,0
	mov	bx,xmtbufptr
	cmp	bx,0
	jne	xoc7
	mov	ax,WORD PTR _comm_info+XMTPTR
	mov	cx,WORD PTR _comm_info+XMTIDX
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
	;
	; Here we transmit the actual character, increment the byte pointer,
	; decrement the number of bytes left in the buflet and the buflet
	; chain, and if the chain is empty, we zero out the buflet pointer and
	; the number left in the buflet.
	;
	mov	bx,xmt_point
	mov	al,[bx]
	mov	dx,WORD PTR _comm_info+COMMBASE
	out	dx,al

	inc	xmt_point
	dec	left_in_buflet
	dec	WORD PTR _comm_info+XMTLEN
	jne	xocend
	mov	xmtbufptr,0
	mov	left_in_buflet,0
	jmp	xocend

xmtdeactivate:
	;
	; This effectively turns off the transmitter.  Transmit interrupts
	; will not come in because we didn't transmit anything.
	;
	mov	WORD PTR _comm_info+XMTACTIVE,0

xocend:
	pop	si
	pop	di
	mov	sp,bp
	pop	bp
	ret	

_xmt_one_char	ENDP


;************************************************************************
;* LOCAL VOID mdm_status()
;*
;*    This function reads the modem status
;*
;* Returns:  none
;*
;************************************************************************
mdm_status	PROC	NEAR
	push	ax
	push	dx
	mov	ax,WORD PTR _comm_info+COMMBASE
	add	ax,6				; address for MSR_PORT
	mov	dx,ax
	in	al,dx				; Get modem status
	mov	BYTE PTR _comm_info+MDMSTATUS,al	; Save it.
	pop	dx
	pop	ax
	ret
mdm_status	ENDP

;************************************************************************
;* VOID clear_xmt()
;*
;*    This function clears out the local pointers used by the interrupt
;*    level transmit functions.  It should be called whenever the 
;*    transmit pointers/lengths are cleared out.
;*
;* Returns:  none
;*
;************************************************************************

	PUBLIC	_clear_xmt
_clear_xmt	PROC	NEAR
	mov	left_in_buflet,0
	mov	xmtbufptr,0
	ret
_clear_xmt	ENDP

_TEXT	ENDS
END

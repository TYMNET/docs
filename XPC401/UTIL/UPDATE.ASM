;************************************************************************
;* UPDATE.ASM - ASM routines for Set Update and Set Interrupt
;* Copyright (C) 1987, Tymnet MDNSC
;* All Rights Reserved
;*
;* SUMMARY:
;*
;*    This module contains several short utility routines written in
;* assembler to perform certain functions which cannot be written in C
;* or using the Microsoft C 4.0 library.  These functions are used
;* to perform the actual update or interrupt call when a requested event
;* occurs.
;*
;*     
;* REVISION HISTORY:
;*
;*   Date    Version      By       Purpose of Revision
;* --------  ------- ------------  ----------------------------------------
;* 03/05/87   4.00    S. Bennett    Initial Draft
;*
;************************************************************************
	TITLE	update

;
; Microsoft C 4.0 segment & group usage
;
	.287
_TEXT	SEGMENT  BYTE PUBLIC 'CODE'
_TEXT	ENDS
_DATA	SEGMENT  WORD PUBLIC 'DATA'
_DATA	ENDS
CONST	SEGMENT  WORD PUBLIC 'CONST'
CONST	ENDS
_BSS	SEGMENT  WORD PUBLIC 'BSS'
_BSS	ENDS

DGROUP	GROUP	CONST,	_BSS,	_DATA
	ASSUME	CS: _TEXT, DS: DGROUP, SS: DGROUP, ES: DGROUP

_DATA	SEGMENT

jmp_addr	dw	0,0		; Place to put jump vector

_DATA	ENDS


_TEXT	SEGMENT

;************************************************************************
;* VOID store_word(seg, off, val)
;*    UWORD seg;
;*    UWORD off;
;*    UWORD val;
;*
;*    This function stores the value given to the address seg:off.
;*
;* Returns: None
;*
;************************************************************************

	public	_store_word
_store_word	proc    near
	push	bp
	mov	bp,sp
	push	es
	push	di
	mov	es,[bp+4]
	mov	di,[bp+6]
	mov	ax,[bp+8]
	mov	es:[di],ax
	pop	di
	pop	es
	mov	sp,bp
	pop	bp
	ret
_store_word	endp

;************************************************************************
;* VOID exec_interrupt(seg, off)
;*    UWORD seg;
;*    UWORD off;
;*
;*    This function calls the function at address seg:off via a
;*    "faked" software interrupt
;*
;* Returns: None
;*
;************************************************************************

	public	_exec_interrupt
_exec_interrupt	proc    near
	push	bp
	mov	bp,sp
	push	ds				; Save some regs
	push	es
	push	si
	push	di
	push	bp
	mov	ax,[bp+4]			; move address to jmp_addr
	mov	jmp_addr+2,ax
	mov	ax,[bp+6]
	mov	jmp_addr,ax
	pushf					; Push flags for fake INT
	pop	ax				; ...Load into AX for manip.
	push	ax				; ...And put back on stack
	and	ax,0FCFFH			; AND off the TF and IF
	push	ax				; ...And load back into
	popf					; ...The flags register
	call	DWORD PTR jmp_addr		; Call the INT routine
	pop	bp				; Restore the regs
	pop	di
	pop	si
	pop	es
	pop	ds
	mov	sp,bp
	pop	bp
	ret
_exec_interrupt	endp

_TEXT	ENDS
	END


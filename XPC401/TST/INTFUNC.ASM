;************************************************************************
;* intfunc.asm - Service Interrupt
;* Copyright (C) 1987, Tymnet MDNSC
;* All Rights Reserved
;*
;* SUMMARY:
;*    Services interrupt called by application set interrupt function.
;*
;* REVISION HISTORY:
;*
;*   Date    Version  By    Purpose of Revision
;* --------  ------- -----  ---------------------------------------------
;* 03/04/87   4.00    KS    Initial Draft
;*
;************************************************************************/
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

EXTRN	_do_int:NEAR

_TEXT	SEGMENT

;************************************************************************
;*
;* Returns: None
;*
;************************************************************************

	public _intfunc
_intfunc	proc	near
	push	bp
	push	ds
	push	es
	push	si
	push	di
	push	ax
	push	bx
	push	cx
	push	dx
	mov	ax,DGROUP
	push	ax
	push	ax
	pop	ds
	pop	es
	call	_do_int
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	di
	pop	si
	pop	es
	pop	ds
	pop	bp
	iret
_intfunc	endp

_TEXT	ENDS
	END


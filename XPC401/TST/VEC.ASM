;************************************************************************
;* vec.c - Interrupt Vector Manipulation
;* Copyright (C) 1987, Tymnet MDNSC
;* All Rights Reserved
;*
;* SUMMARY:
;*    vec.c reads the specified interrupt vector.
;*
;* REVISION HISTORY:
;*
;*   Date    Version  By    Purpose of Revision
;* --------  ------- -----  ---------------------------------------------
;* 03/04/87   4.00    KS    Initial Draft
;*
;*************************************************************************/
	TITLE   vec

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
	ASSUME  CS: _TEXT, DS: DGROUP, SS: DGROUP, ES: DGROUP

_TEXT      SEGMENT

	PUBLIC	_get_intvec
_get_intvec	PROC NEAR
	push	bp			; C entry sequence
	mov	bp,sp

	push	es			; Save ES, SI, & DI to be safe
	push	si
	push	di

	mov	ah,035H			; Set up for DOS call 35H (get vector)
	mov	al,BYTE PTR [bp+4]	; Put vector in AL
	int	021H			; Make DOS call (return is in ES:BX)

	mov	ax,bx			; move offset into AX for safety
	mov	bx,[bp+6]		; Get address for storing segment
	mov	WORD PTR [bx],es	; Store segment value
	mov	bx,[bp+8]		; Get address for storing offset
	mov	WORD PTR [bx],ax	; Store offset value

	pop	di			; Restore ES, SI, & DI
	pop	si
	pop	es

	mov	sp,bp			; C exit sequence
	pop	bp
	ret	
_get_intvec	ENDP

_TEXT	ENDS
	END


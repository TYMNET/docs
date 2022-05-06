;************************************************************************
;* MOVPARAM.ASM - Move driver call parameters
;* Copyright (C) 1987, Tymnet MDNSC
;* All Rights Reserved
;*
;* SUMMARY:
;*
;*    This module contains the function necessary to move data to and from
;* the application via the addresses and segment specified in the parameter
;* block passed by the application to the driver.  There are currently no
;* checks for illegal addresses or offsets, as there is no mechanism for
;* specifying that such has occurred.  Perhaps a debugging option could
;* be added?
;*     
;* REVISION HISTORY:
;*
;*   Date    Version      By       Purpose of Revision
;* --------  ------- ------------  ----------------------------------------
;* 03/04/87   4.00    S. Bennett    Initial Draft
;*
;************************************************************************
	TITLE   movparam

;
; Standard Microsoft C 4.0 segment & group definitions
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
; External (static) variables
;
EXTRN	_param_block:WORD


_TEXT      SEGMENT


;************************************************************************
;* VOID mov_param(addr, len, param, offset, direction)
;*    UBYTE *addr;		/* address in driver of transfer buffer */
;*    UWORD len;		/* Number of bytes to transfer */
;*    UWORD param;		/* Parameter number (1-4) for transfer */
;*    UWORD offset;		/* Offset from address in given param */
;*    BOOL direction;		/* 1 (YES) if appl --> driver, else 0 */
;*
;*    This function moves a block of *len* bytes either from the driver to
;*    the application, or vice versa.  The address for the start of the
;*    transfer is given in *addr* for the driver, and uses the driver's
;*    Data Segment (DS).  The address for the application is taken from
;*    the parameter specified in the parameter block, and is further modified
;*    by adding *offset* to it.  
;*
;* Notes: No error checking is done.  Caveat programmitor.
;*
;* Returns:  none
;*
;************************************************************************
	PUBLIC	_mov_param
_mov_param	PROC NEAR

	push	bp
	mov	bp,sp
	push	ds
	push	es
	push	si
	push	di
;
;	WORD PTR [bp+4]			addr
;	WORD PTR [bp+6]			len
;	WORD PTR [bp+8]			param
;	WORD PTR [bp+10]		offset
;	WORD PTR [bp+12]		direction
;
	mov	ax,WORD PTR [bp+12]		; get direction flag
	or	ax,ax				; if zero, then moving data
	jz	movtoapp			;   from driver to application,

						; Otherwise, moving from
						; application to driver...
	
	mov	bx,WORD PTR [bp+8]		; Get parameter number
	shl	bx,1				; Get word offset into the
						;   parameter block.
	mov	si,WORD PTR _param_block[bx+8]	; Get source address
	add	si,WORD PTR [bp+10]		; Add offset to address
	mov	di,WORD PTR [bp+4]		; Get destination address
	mov	cx,WORD PTR [bp+6]		; Get length in bytes
	push	ds				; Get destination segment
	pop	es
	mov	ds,WORD PTR _param_block	; Get source segment
	jmp	short moveit			; Move and return
	
movtoapp:
	mov	bx,WORD PTR [bp+8]		; Get parameter number
	shl	bx,1				; Get word offset into the
						; parameter block
	mov	di,WORD PTR _param_block[bx+8]	; Get destination address
	add	di,WORD PTR [bp+10]		; Add offset to it
	mov	si,WORD PTR [bp+4]		; Get source address
	mov	cx,WORD PTR [bp+6]		; Get length in bytes
	mov	es,WORD PTR _param_block	; Get destination segment

moveit:
	cld
	rep	movsb				; Move the bytes
	pop	di
	pop	si
	pop	es
	pop	ds
	mov	sp,bp
	pop	bp
	ret	

_mov_param	ENDP
_TEXT	ENDS
END

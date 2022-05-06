;************************************************************************
;* APPINT.ASM - Application Interrupt handler
;* Copyright (C) 1987, Tymnet MDNSC
;* All Rights Reserved
;*
;* SUMMARY:
;* 
;*    This module contains the routine necessary to process incoming
;*    function call requests coming via the INT 0x7A interrupt vector.
;*     
;* REVISION HISTORY:
;*
;*   Date    Version      By       Purpose of Revision
;* --------  ------- ------------  ----------------------------------------
;* 03/04/87   4.00    S. Bennett    Initial Draft
;*
;************************************************************************
	TITLE   appint

;
; Microsoft C 4.0 segment/group usage
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

;
; External functions
;
EXTRN	_app_interface:NEAR
EXTRN   _xpc_unload:NEAR

;
; External variables
;
EXTRN   _app_ss:WORD
EXTRN	_app_sp:WORD
EXTRN   _param_block:WORD
EXTRN   __psp:WORD
EXTRN	_in_application:WORD
EXTRN	_asave_ss:WORD
EXTRN	_asave_sp:WORD


_TEXT      SEGMENT



;************************************************************************
;* VOID appint() 
;*
;*    This function is called by the software interrupt 0x7A (or whatever
;*    alternate vector may have been chosen.) with a pointer to a parameter
;*    block to perform one of the functions as listed in the Driver
;*    Interface Specification.  The format of this block is given in that
;*    document.  The pointer is passed in ES:BX.  If ES is 0, then there is
;*    no parameter block and the request is rather to begin the unload
;*    process, which consists of calling xpc_unload().
;*
;* Notes: While this routine provides it's own stack for the driver's use,
;*    it necessarily requires some of the application's stack.  It uses
;*    18 bytes of the application's stack in normal operation, and a bit
;*    more for an unload operation.  This is in addition to the 6 bytes used
;*    by the software interrupt itself.
;*
;*    This routine also makes use of a software locking mechanism to prevent
;*    the driver from being called while it is processing another function
;*    call.
;*
;* Returns:  none normally.
;*           BX contains the Program Segment Prefix (PSP) during an unload
;*
;************************************************************************

EXTRN   _xpc_ds:WORD			; Import data segment so we can use it

	PUBLIC	_xpc_appint
_xpc_appint	PROC FAR
	push	ax			; Push all the registers...
	push	cx
	push	dx
	push	si
	push	di
	push	ds
	push	bp
	push	es
	
	mov	ds,cs:_xpc_ds		; Load with our data segment
	
	push	es			; If ES is NULL, then this is
	pop	ax			; a request to unload the driver
	cmp	ax,0
	jne	appint2
	push	ds			; Set ES = DS to make C compiler happy
	pop	es
	call	_xpc_unload		; Call cleanup routine for unload
	mov	bx,[__psp]		; Return Program Segment Prefix in BX
	jmp	doneapp

appint2:	
	cli				; Turn off ints while checking...
	cmp	_in_application,0	; Driver interface already called?
	jz	setupapp
	;
	; eventually here we need to put some kind of return code
	; stating that the driver interface cannot be called if it
	; is already processing a call
	;
	sti
	jmp	doneapp

setupapp:
	push	bx
	mov	_in_application,1	; Set flag to prevent reentry
	mov	_asave_ss,SS		; Save stack segment
	mov	_asave_sp,SP		; Save stack pointer
	mov	SS,WORD PTR _app_ss	; Set local stack segment
	mov	SP,WORD PTR _app_sp	; Set local stack pointer
	sti				; Safe to turn interrupts back on
	
	;
	; Below we copy the parameter block to our own static buffer
	; to simplify and speed up access.  The address of the param
	; block is in ES:BX.  The address of the static buffer is
	; DS:_param_block.  Since MOVSW requires the source to
	; be DS:SI and the destination to be ES:DI, we must swap segment
	; registers.
	;
	push	ds			; Swap segments for MOVSW
	push	es
	pop	ds
	pop	es
	mov	si,bx			; Set up source address
	mov	di,OFFSET _param_block	; Set destination address
	mov	cx,9			; # of words - soon may be 11
	cld				; make sure we move forward
	rep	movsw			; copy the parameter block
	
	mov	ds,cs:_xpc_ds		; Reload the data segment

	mov	es,WORD PTR _param_block
	mov	bx,WORD PTR _param_block+6
	push	es:[bx]			; Push the channel number
	mov	bx,WORD PTR _param_block+4
	push	es:[bx]			; Push the device number
	mov	bx,WORD PTR _param_block+2
	push	es:[bx]			; Push the function number

	push	ds			; Set ES = DS to make C compiler happy
	pop	es
	
	call	_app_interface		; Call the application interface
	add	sp,6			; Pop params off stack

	mov	es,WORD PTR _param_block
	mov	bx,WORD PTR _param_block+8
	mov	es:[bx],ax		; Save status

	cli				; Ints off while restoring stack
	mov	ss,_asave_ss		; Restore stack segment
	mov	sp,_asave_sp		; Restore stack pointer
	mov	_in_application,0	; Allow reentrancy
	sti

	pop	bx			; Done here to allow for unload...
doneapp:	
	pop	es			; restore all registers
	pop	bp
	pop	ds
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	ax
	iret	

_xpc_appint	ENDP
_TEXT	ENDS
END

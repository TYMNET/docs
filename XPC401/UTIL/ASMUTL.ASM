;************************************************************************
;* ASMUTL.ASM - Assembly language utilities for the X.PC Driver
;* Copyright (C) 1987, Tymnet MDNSC
;* All Rights Reserved
;*
;* SUMMARY:
;*
;*    This module contains several short utility routines written in
;* assembler to perform certain functions which cannot be written in C
;* or using the Microsoft C 4.0 library.  These functions are mainly used
;* at startup time to set up the driver, interrupt vectors, and data areas
;* used by the driver, as well as during driver operation to handle interrupt
;* initialization and I/O port access.
;*
;*     
;* REVISION HISTORY:
;*
;*   Date    Version      By       Purpose of Revision
;* --------  ------- ------------  ----------------------------------------
;* 03/04/87   4.00    S. Bennett    Initial Draft
;*
;************************************************************************
	TITLE   asmutl

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


;
; External variables
;
EXTRN	_xpc_cs:WORD
EXTRN	_nbr_disables:WORD

;
; External functions
;
EXTRN	__myalloc:NEAR


_TEXT      SEGMENT

;************************************************************************
;* UWORD save_ds() 
;*
;*    This function stores the data segment of the driver in a global
;*    storage area in the *code* segment, so that both timer and comm
;*    interrupt service routines can find the data segment for their
;*    use.  It also saves the code segment in an area in the data
;*    segment for use by various interrupt vector initialization
;*    routines.
;*
;* Returns:  DS
;*
;************************************************************************

	PUBLIC _xpc_ds
_xpc_ds	DW	0			; Location to save DS for use
					; by interrupt routines.

	PUBLIC	_save_ds
_save_ds	PROC NEAR
	push	bp			; C entry sequence
	mov	bp,sp

	mov	ax,ds			; put DS into a variable in code
	mov	CS:_xpc_ds,ax		; segment so interrupts can read it.

	mov	ax,cs			; Put code segment where the
	mov	_xpc_cs,ax		; driver itself can read it.

	mov	ax,ds			; return DS

	mov	sp,bp			; C exit sequence
	pop	bp
	ret	
_save_ds	ENDP



;************************************************************************
;* VOID int_disable()
;*
;*    Disables hardware interrupts.  This is necessary when a non
;*    interrupt driven section of the driver wishes to modify a variable
;*    which could potentially be modified by an interrupt driven section
;*    of the program.  This routine and it's companion, int_enable(), are
;*    designed to allow nesting:  That is to say that if three calls to
;*    int_disable() are made, three calls to int_enable() must be made
;*    before interrupts are re-enabled.
;*
;* Notes:  Use this routine with caution.  Leaving interrupts off for any
;*    significant length of time (ie. more than a few milliseconds)
;*    could result in loss of character or timer interrupts.  This is
;*    Not Very Good, and should be avoided if possible.
;*
;* Returns:  none
;*
;************************************************************************

	PUBLIC	_int_disable
_int_disable	PROC NEAR
	push	bp			; C Entry sequence
	mov	bp,sp

	cli				; Turn off the interrupts
	inc	_nbr_disables		; Increment nesting count

	mov	sp,bp			; C exit sequence
	pop	bp
	ret	
_int_disable	ENDP


;************************************************************************
;* VOID int_enable() 
;*
;*    Re-enables interrupts after a int_disable() call.  See notes on
;*    int_disable() for general usage and warnings.
;*
;* Returns:  none
;*
;************************************************************************

	PUBLIC	_int_enable
_int_enable	PROC NEAR
	push	bp			; C entry sequence
	mov	bp,sp

	dec	_nbr_disables		; Decrement nesting count
	jne	iedone			; If nesting count is not zero, skip
	sti				; Otherwise, enable interrupts

iedone:
	mov	sp,bp			; C exit sequence
	pop	bp
	ret	
_int_enable	ENDP



;************************************************************************
;* UBYTE in_port(portaddr)
;*    UWORD portaddr;		/* I/O Port address for input */
;*
;*    Reads the I/O port at the given port address and returns the byte
;*    thus acquired.  
;*
;* Returns:  Byte read from port
;*
;************************************************************************
	PUBLIC	_in_port
_in_port	PROC NEAR
	push	bp			; C Entry Sequence
	mov	bp,sp

	mov	dx,[bp+4]		; move Port number into DS
	xor	ax,ax			; Clear AH
	in	al,dx			; Read port into AL and return

	mov	sp,bp			; C Exit sequence
	pop	bp
	ret	
_in_port	ENDP



;************************************************************************
;* VOID out_port(portaddr, value)
;*    UWORD portaddr;		/* I/O port address for output */
;*    UBYTE value;		/* Value for output */
;*
;*    Writes the value given to the I/O port at portaddr.
;*
;* Returns:  none
;*
;************************************************************************
	PUBLIC	_out_port
_out_port	PROC NEAR
	push	bp			; C entry sequence
	mov	bp,sp

	mov	al,[bp+6]		; Get byte to output
	mov	dx,[bp+4]		; Get port address where it goes
	out	dx,al			; output byte to given port address

	mov	sp,bp			; C exit sequence
	pop	bp
	ret	
_out_port	ENDP


;************************************************************************
;* VOID get_intvec(vec, pcs, pip)
;*    UWORD vec;		/* interrupt vector number */
;*    UWORD *pcs;		/* pointer to storage for code segment */
;*    UWORD *pip;		/* pointer to storage for instruction ptr */
;*
;*    Reads (via a call to DOS) the interrupt vector specified, and stores
;*    the resulting address in the locations specified.
;*
;* Returns:  none
;*
;************************************************************************

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


;************************************************************************
;* VOID set_intvec(vec, cs, ip)
;*    UWORD vec;		/* interrupt vector number */
;*    UWORD cs;			/* New code segment */
;*    UWORD ip;			/* New instruction pointer */
;*
;*    Sets the specified interrupt vector to the code segment and
;*    instruction pointer specified.  This uses the DOS call for setting
;*    vectors.
;*
;* Returns:  none
;*
;************************************************************************

	PUBLIC	_set_intvec
_set_intvec	PROC NEAR
	push	bp			; C entry sequence
	mov	bp,sp

	mov	ah,025H			; Set up for DOS call 25H (Set Vector)
	mov	al,BYTE PTR [bp+4]	; Get vector number
	mov	dx,WORD PTR [bp+8]	; Get offset
	push	ds			; Save DS for use right after call
	mov	ds,WORD PTR [bp+6]	; Get segment into DS
	int	021H			; Call DOS

	pop	ds			; Restore DS

	mov	sp,bp			; C exit sequence
	pop	bp
	ret	
_set_intvec	ENDP


;************************************************************************
;* VOID get_sp(pss, psp)
;*    UWORD *pss;		/* pointer to storage for stack segment */
;*    UWORD *psp;		/* pointer to storage for stack pointer */
;*
;*    This routine reads the stack pointer and segment and stores them
;*    in the locations provided.  This is used to determine where
;*    the stack should be set upon application calls to the driver.
;*
;* Returns:  none
;*
;************************************************************************

	PUBLIC	_get_sp

_get_sp	PROC	NEAR
	push	bp			; C entry sequence
	mov	bp,sp

	mov	bx,WORD PTR [bp+4]	; Get address for storing SS
	mov	WORD PTR [bx],ss	; Store SS
	mov	bx,WORD PTR [bp+6]	; Get address for storing SP
	mov	WORD PTR [bx],sp	; Store SP

	mov	sp,bp			; C exit sequence
	pop	bp
	ret
_get_sp	ENDP


;************************************************************************
;* VOID xpc_alloc(length)
;*    UWORD length;		/* Size of the area required */
;*
;*    This routine calls a special Microsoft C function to allocate
;*    memory by increasing the size of the memory partition containing
;*    the driver.  It is used to allocate required buffer space at load
;*    time.  If there is not enough memory, the routine exits the program.
;*
;* Notes:  Do NOT use any other alloc routine with this driver.  Use this
;*    routine ONLY at load time, and then before anything involving
;*    interrupts.  The routine will exit (not all that gracefully) if not
;*    enough memory is available, and once load time is done, memory is
;*    NEVER available.  Be warned!
;*
;* Returns:  none
;*
;************************************************************************

	PUBLIC	_xpc_alloc
_xpc_alloc	PROC NEAR
	push	bp			; C entry sequence
	mov	bp,sp
	push	si
	push	di

	mov	ax,[bp+4]		; Length (in bytes) of alloc
	mov	di,02010		; Error message number if not enough
	call	__myalloc		; Microsoft C low level alloc
	mov	ax,bp			; Move address into ax for return

	pop	di			; C exit sequence
	pop	si
	pop	bp
	ret	
_xpc_alloc	ENDP

_TEXT	ENDS
	END


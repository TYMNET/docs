;************************************************************************
;* TIMEINT.ASM - Timer interrupt handling routines.
;* Copyright (C) 1987, Tymnet MDNSC
;* All Rights Reserved
;*
;* SUMMARY:
;*     
;*     Contains the interrupt routines required to process timer
;*     interrupts, as well as the routines needed to initialize these
;*     interrupts and clear them.  
;*
;* REVISION HISTORY:
;*
;*   Date    Version      By       Purpose of Revision
;* --------  ------- ------------  ----------------------------------------
;* 03/04/87   4.00    S. Bennett    Initial Draft
;*
;************************************************************************

	TITLE   timeint

;
; The following is the timer stack size.  If the size must change for any
; reason, you must also change the define XPC_STACK_LEN in xpc.h
;
XPCSTACKLEN EQU 1000

;
; Stuff to make C and link happy
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
; External variables and functions
;
EXTRN   _dec_sixths:NEAR		; Decrements 1/6 second timers
EXTRN	_do_link:NEAR			; Does link processing
EXTRN	_do_timers:NEAR			; Does timeout processing
EXTRN	_get_intvec:NEAR		; gets an interrupt vector
EXTRN	_set_intvec:NEAR		; stores an interrupt vector
EXTRN	_xpc_cs:WORD			; Code segment for the driver
EXTRN   _xpc_stack:WORD			; Timer interrupt stack pointer
EXTRN	_xpc_timer_active:WORD		; Are X.PC timers active?

;
; Local (static) variables
;
_BSS      SEGMENT
ts1     DD      00H			; Space to save old timer int vector
in_link	DW	00H			; Software lock on timer int
save_ss DW	00H			; Area to save SS
save_sp DW	00H			; Area to save SP
time_enabled DW 00H			; Timer enabled flag
_BSS      ENDS


_TEXT      SEGMENT

;
; The following external is in the code segment so that it can be accessed
; by timer2_int(), which loads it into DS so that the rest of timer interrupt
; processing can access our data segment.
;
EXTRN	_xpc_ds:WORD			; Driver's data segment

;
; The following static variables MUST be in the code segment so that
; timer_int() can access them, since timer_int() does not have access
; to the driver's data segment.
;
save_ax DW	00H			; Area to save AX
ts2	DD	00H			; Jump vector for interrupt chain


;************************************************************************
;* VOID timer2_int()
;*
;*     This function is the main timer interrupt processor, called when
;*     returning from all other timer processing (DOS, Int 1C, and all
;*     who preempted these before we did.) via the sneaky trick of
;*     placing a fake interrupt call on the stack, with the return address
;*     pointing to the beginning of this routine.
;*
;*     The routine saves the registers and decrements 1/6th second timers.
;*     If a software lock is set, it then restores the registers and
;*     returns.  Otherwise, it sets the lock, reassigns the stack to our
;*     own location, since the interrupted program may not have had much
;*     stack available; and calls timeout and link level processing.  After
;*     timeout and link processing are complete, it restores the stack,
;*     clears the software lock, restores the registers, and returns.
;*     
;* Notes: This is NOT a C callable function.  For that matter, it should
;*     not be directly called by ANY function, but called only in the
;*     manner described above.  Further note that timeout processing
;*     (in do_timers()) and link level processing (in do_link()) run with
;*     interrupts ENABLED.  Timer interrupts will be blocked here.  Comm
;*     interrupts should be provided for, where necessary.
;*
;* Returns: None.
;*
;************************************************************************
	PUBLIC	_timer2_int

_timer2_int	PROC FAR
	
	;
	; At this entry point, all that should remain on the stack is the
	; original interrupt call to timer_int().
	;
	cli				; Turn off interrupts
	push	ax			; Push all regs save SS and SP
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	ds
	push	es
	push	bp

	mov	ds,cs:_xpc_ds		; Load in the X.PC data segment
					; from the code segment.

	call	_dec_sixths		; Decrement 1/6th second timers
	cmp	in_link,0		; Is the software lock set?
	jne	end_timeint		; If so, jump to end of timer

	mov	in_link,1		; Set the software lock

	mov	save_ss,ss		; Save the stack segment (SS)
	mov	save_sp,sp		; Save the stack pointer (SP)
	mov	ss,cs:_xpc_ds		; Set stack segment to X.PC DS
	lea	sp,_xpc_stack+XPCSTACKLEN ; Set stack pointer to timer stack

	;
	; At this point, it is safe to reenable interrupts, as the stack
	; and software lock have been set.  Most probably we'll be
	; interrupted immediately, but this should no longer be a problem.
	;
	sti				; Enable interrupts
	call	_do_timers		; Do timeout processing
	call	_do_link		; Do link level processing
	
	;
	; to restore everything, interrupts must once again be off.
	;
	cli				; Disable interrupts
	mov	ss,save_ss		; Restore Stack Segment
	mov	sp,save_sp		; Restore Stack pointer
	mov	in_link,0		; Clear software lock

end_timeint:
	pop	bp			; pop all registers in reverse
	pop	es			; order of their original stacking.
	pop	ds
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	sti				; Done.  Enable interrupts
	
	;
	; Left on the stack now is only the original interrupt call which
	; called timer_int().  We now return to our interrupted program.
	;
	iret

_timer2_int	ENDP



;************************************************************************
;* VOID timer_int()
;*
;*     This function is the initial timer interrupt function called at
;*     a hardware timer interrupt.  It's sole function is to set up a
;*     fake interrupt on the stack so that when the BIOS interrupt handler
;*     returns, it will end up in timer2_int(), instead of the originally
;*     interrupted program.  It then directly jumps to the address which
;*     the original timer interrupt vector had been pointing to. (Which
;*     should be the BIOS, or another program which has preempted the
;*     BIOS)
;*
;*     This function simply passes the interrupt on to the original timer
;*     interrupt vector if the flag "xpc_timer_active" is false (ie., 0).
;*     This is for support for the "-T" flag for programs which use the
;*     0x8 timer vector.
;*
;* Notes: Again, this is NOT a C Callable function.  Do not call directly.
;*     Interrupts are disabled at this point due to the automatic
;*     disabling of interrupts upon receipt of a hardware interrupt.
;*
;* Returns:  <list all possible returns and their meanings, one to a line>
;*
;************************************************************************
	PUBLIC	_timer_int

_timer_int	PROC FAR
	;
	; First, we check the xpc_timer_active flag to see if we need
	; to do anything other than jump to the next vector in the chain.
	;
	push	ax				; save used registers
	push	ds
	
	mov	ds,cs:_xpc_ds			; Get the data segment
	mov	ax,_xpc_timer_active		; Get the flag
	cmp	ax,0				; If the flag is not zero,
	jne	ti1				;    then set up fake INT...
	
	pop	ds				; else restore used registers
	pop	ax
	
	jmp	short ti2			; and call the next vector.
	
ti1:
	pop	ds				; restore stack to the
	pop	ax				;   original setup...

	;
	; At this point, the only thing on the stack is the flags and
	; return address for the hardware interrupt.  It looks like this:
	;
	;	SP -> IP of return address
	;	      CS of return address
	;             FLAGS
	;
	pushf				; Push flags for fake interrupt
	push	cs			; Push return CS for timer2_int
	mov	save_ax,ax		; Save AX temporarily
	mov	ax,OFFSET _timer2_int	; Load AX with address of timer2_int
	push	ax			; Push address of timer2_int
	
	;
	; At this point, the fake interrupt is on the stack,
	; and looks like this:
	;
	;       SP -> IP for timer2_int
	;             CS for timer2_int
	;             FLAGS
	;             IP of return address
	;             CS of return address
	;             FLAGS
	;
	; Now, when the original interrupt routine executes its IRET, it
	; will return to timer2_int(), instead of the interrupted program.
	;
	mov	ax,save_ax		; Restore AX

ti2:
	jmp	dword ptr ts2		; Jump to original interrupt routine
	iret				; This instruction never reached

_timer_int	ENDP



;************************************************************************
;* VOID enable_timer()
;*
;*     Initializes timers by setting the interrupt vector.  Called during
;*     device reset.
;*
;* Returns: none
;*
;************************************************************************
	PUBLIC	_enable_timer

_enable_timer	PROC NEAR
	push	bp			; C subroutine initialization
	mov	bp,sp

	mov	ax,time_enabled		; If already enabled, return
	cmp	ax,0
	jne	et_end

	mov	ax,OFFSET DGROUP:ts1	; Get address to store old IP from INT
	push	ax
	add	ax,2			; Address to store old CS
	push	ax
	mov	ax,8			; Interrupt vector 0x8
	push	ax
	call	_get_intvec		; Store old vector in ts1
	add	sp,6

	mov	ax,WORD PTR ts1		; Copy vector to ts2 in the X.PC
	mov	WORD PTR cs:ts2,ax	; code segment, so that timer_int
	mov	ax,WORD PTR ts1+2	; can use it to keep the chain intact.
	mov	WORD PTR cs:ts2+2,ax
	mov	ax,OFFSET _timer_int	; Push address of timer_int()
	push	ax
	push	_xpc_cs			; And it's code segment
	mov	ax,8			; And the interrupt vector (0x8)
	push	ax
	call	_set_intvec		; Set the interrupt vector.

	mov	time_enabled,1		; Set timer enabled flag

et_end:
	mov	sp,bp			; End of C function.
	pop	bp
	ret	

_enable_timer	ENDP



;************************************************************************
;* VOID disable_timer()
;*
;*     Restores timer interrupts to their original configuration after
;*     a call to time_init().
;*
;* Returns: none.
;*
;************************************************************************
	PUBLIC	_disable_timer

_disable_timer	PROC NEAR
	push	bp			; C Subroutine initialization
	mov	bp,sp

	mov	ax,time_enabled		; If timers not enabled, return
	cmp	ax,0
	je	dt_end

	push	WORD PTR ts1		; push address of saved vector's IP
	mov	bx,OFFSET DGROUP:ts1	; Calculate address of saved vector's
	add	bx,2			; code segment, and push it.
	push	WORD PTR [bx]
	mov	ax,8			; Push interrupt vector (0x8)
	push	ax
	call	_set_intvec		; Restore old interrupt vector

	mov	time_enabled,0		; Clear timer enabled flag

dt_end:
	mov	sp,bp			; Return from C function
	pop	bp
	ret	

_disable_timer	ENDP
_TEXT	ENDS
END

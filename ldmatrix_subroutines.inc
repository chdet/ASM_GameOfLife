/*
 * ldmatrix_subroutines.inc
 *
 *  Created: 04-05-18 22:52:15
 *   Author: Charles
 */ 

; Used registers: R2,R3,R16, R17, R18, R19, R20, Y

Timer0OverflowInterrupt:
	PUSH R20
	PUSH R19
	PUSH R18
	PUSH R17
	PUSH R16
	PUSH R3
	PUSH R2
	LDS R0, SREG
	PUSH R0


	RCALL Print_board				; Print one row

	LDI R16,SCRSTART				; Reset timer 0 counter
	OUT TCNT0, R16

	POP R0
	STS SREG,R0
	POP R2
	POP R3
	POP R16
	POP R17
	POP R18
	POP R19
	POP R20
	RETI

.def COORD_X = R2
Print_board:
	SBI PORTB,4						; Trick to avoid waiting a lot for the capacitors to discharge inside the interrupt

	LDI R16, MATRIX_COL/8			; Get byte count of matrix columns
	MOV R3, R16
	CLR COORD_X
	LDI R16,5						; 80 columns, 8 bit per data register
	ColumnLoop1:					; Loading one 5 bytes row
		LD R17,Y+					; Indirect load from (Y) with increment
		INC COORD_X
		CP COORD_X, R3				; Out-of-bounds handling
		BRNE PC+3
			CLR COORD_X
			ADIW Y, 2*PADDING
		LDI R18,8
		DataLoop1:					; Loading one byte
			CBI PORTB,3
			SBRC R17,7				; Copy data to shift register by bit check + logical shift left
			SBI PORTB,3
			CBI PORTB,5
			SBI PORTB,5
			LSL R17
			DEC R18
			BRNE DataLoop1
		DEC R16
		BRNE ColumnLoop1
	ADIW Y,6*(5+2*PADDING)						; Line 2 to Line 8 ---> +6*(5 bytes) = 30 bytes
	CLR COORD_X
	LDI R16,5
	ColumnLoop2:
		LD R17,Y+
		INC COORD_X
		CP COORD_X, R3				; Out-of-bounds handling
		BRNE PC+3
			CLR COORD_X
			ADIW Y, 2*PADDING
		LDI R18,8
		DataLoop2:
			CBI PORTB,3
			SBRC R17,7
			SBI PORTB,3
			CBI PORTB,5
			SBI PORTB,5
			LSL R17
			DEC R18
			BRNE DataLoop2
		DEC R16
		BRNE ColumnLoop2
	SBIW Y, 7*(5+2*PADDING)						; Line 9 to Line 2 ---> -7*(5 bytes) = -35 bytes

	MOV R17,ACTIVE_ROW						; Copy current active row in R17
	LDI R18,8
	RowLoop:						; Loading current row to shift register
		CBI PORTB,3
		SBRC R17,7
		SBI PORTB,3
		CBI PORTB,5
		SBI PORTB,5
		LSL R17
		DEC R18
		BRNE RowLoop

	CBI PORTB,4
	SBI PORTB,4						; Rising edge on PB4 copies shift register to latch

	LDI R16,50						; Wait to leave a bit more time for the led capacitors to empty
	WaitLoop21:						; Postwait avoids glitches when output is enabled
		NOP
		DEC R16
		BRNE WaitLoop21
	CBI PORTB,4						; PB4 at 0 enable output

	LSR ACTIVE_ROW					; Shift current active row
	BRNE PC+2
	LDI ACTIVE_ROW,0b01000000

	DEC ROWS_REMAINING				; Size of data block is known, used to find end-of-line			
	BRNE PC+2						; Check if data memory reached end of line
	RCALL ResetDataAdr
	RET

ResetDataAdr:
	; Reset the data address
	LDI R19,high(board+matrix_start) 
	LDI R20,low(board+matrix_start)
	MOV YH, R19					; Program memory adress MSB
	MOV YL, R20					; LSB
	LDI ROWS_REMAINING,7
	RET

.undef COORD_X

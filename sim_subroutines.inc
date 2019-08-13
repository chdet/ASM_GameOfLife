/*
 * sim_subroutines.inc
 *
 *  Created: 09-05-18 17:22:45
 *   Author: Charles
 */ 
 ; Used registers: R0,R2,R3,R4,R5,R6,R7,R8,R16,R17,R18,R19, X, Y


.def COORD_X = R2
.def COORD_Y = R3
.def NEIGHBOURS = R19
.def BITMASK = R4
.def IDX = R7
Timer1OverflowInterrupt:
	T1_PUSHREGS						; Push the registers that will be used, which is quite a few
	BSET 7							; Re-enable global interrupt


	SBI PINC,2						; Toggle LED2

	LDI ZL, low(board)
	LDI ZH, high(board)
	CLR COORD_X						; Used to avoid compare operations with addresses, which use words (less straightforward)
	CLR COORD_Y
	CLR NEIGHBOURS					; NEIGHBOURS register is used to count the number of live neighbours of a cell
	CLR IDX							; The index is used to locate the correct byte when copying to boardcpy
	LDI R16, 0b10000000
	MOV BITMASK, R16


	LDI R16,BOARD_SIZE				; Iterate through the entire board, not only the visible space
	MOV R5,R16
	ColumnLoop3:
		LD R18,Z					; Indirect load from (Z)
		LDI R16,8
		MOV R6,R16
		BCLR 7						; Disable global interrupt to avoid glitch with led refresh
		DataLoop3:					; Loading one byte
			SBRC R18,7				; bit check
			RJMP PC+3
				RCALL Check_dead_cell
				RJMP PC+2
			RCALL Check_live_cell
			LSL R18					; logical shift left
			INC COORD_X
			MOV R16, COORD_X
			CPI R16, BOARD_COL		; Check if new row
			BRNE PC+3
				INC COORD_Y
				CLR COORD_X
			LSR BITMASK
			CLR NEIGHBOURS
			DEC R6
			BRNE DataLoop3
		BSET 7							; Re-enable global interrupt
		ADIW Z, 1						; Increment Z
		INC IDX
		LDI R16, 0b10000000
		MOV BITMASK, R16				; Reset bitmask
		DEC R5
		BRNE ColumnLoop3


	LDI ZL, low(board)
	LDI ZH, high(board)
	LDI XL, low(boardcpy)
	LDI XH, high(boardcpy)

	Boardcpy_to_Board:
	LD R18,X+
	ST Z+, R18
	CPI ZL, low(board_eol)
	BRNE Boardcpy_to_Board
	CPI ZH, high(board_eol)
	BRNE Boardcpy_to_Board

	;Update cursor_byte	
	X2CB
	LD R16, X
	STXI cur_byte, R16

	
	LDI R16,high(TIMERSTART)				; Reset Timer 1 counter
	LDI R17,low(TIMERSTART)
	STS TCNT1H,R16
	STS TCNT1L,R17
	T1_POPREGS
	RETI

Check_dead_cell:
	RCALL TestYplus
	RCALL TestYminus
	RCALL TestXplus
	RCALL TestXminus
	RCALL TestXplusYminus
	RCALL TestXminusYplus
	RCALL TestXYplus				; This section can be optimized by doing checks every 3 tests
	RCALL TestXYminus
	CPI NEIGHBOURS, 3
	BRNE stop_dc_chk
			; Set cell to 1 in board copy
			LDI XL, low(boardcpy)
			LDI XH, high(boardcpy)
			ADD XL,IDX
			CLR R0
			ADC XH,R0				; Add with potential carry of previous operation
			LD R16, X
			OR R16, BITMASK
			ST X, R16
			RET
	stop_dc_chk: 
	; Set cell to 0 in board copy
	LDI XL, low(boardcpy)
	LDI XH, high(boardcpy)
	ADD XL,IDX
	CLR R0
	ADC XH,R0						; Add with potential carry of previous operation
	LD R16, X
	MOV R8, BITMASK
	LDI R17, 0xFF
	EOR R8, R17						; XOR(R8,0xFF) = NOT(R8)
	AND R16, R8
	ST X, R16
	RET

Check_live_cell:
	RCALL TestYplus
	RCALL TestYminus
	RCALL TestXplus
	RCALL TestXminus
	RCALL TestXplusYminus
	RCALL TestXminusYplus
	RCALL TestXYplus
	RCALL TestXYminus
	CPI NEIGHBOURS, 2
	BRLO stop_lc_chk
		CPI NEIGHBOURS, 4			; Zero counts as positive so testing for 3 neighbours with BRPL is CPI 4
		BRPL stop_lc_chk
			; Set cell to 1 in board copy
			LDI XL, low(boardcpy)
			LDI XH, high(boardcpy)
			ADD XL,IDX
			CLR R0
			ADC XH,R0				; Add with potential carry of previous operation
			LD R16, X
			OR R16, BITMASK
			ST X, R16
			RET
	stop_lc_chk:
	; Set cell to 0 in board copy
	LDI XL, low(boardcpy)
	LDI XH, high(boardcpy)
	ADD XL,IDX
	CLR R0
	ADC XH,R0						; Add with potential carry of previous operation
	LD R16, X
	MOV R8, BITMASK
	LDI R17, 0xFF
	EOR R8, R17						; XOR(R8,0xFF) = NOT(R8)
	AND R16, R8
	ST X, R16
	RET

TestYminus:
	MOV R16, COORD_Y
	SUBI R16, 1
	BRPL positive_Yminus
		RET							; Negative Y indicates cell is outside board <-> dead cell
	positive_Yminus:
	MOV XL, ZL
	MOV XH, ZH
	SBIW X, (5+2*PADDING)
	LD R16, X
	AND R16, BITMASK
	BRNE liveYminus
		RET
	liveYminus: INC NEIGHBOURS
	RET

TestYplus:
	MOV R16, COORD_Y
	INC R16
	CPI R16, BOARD_ROWS
	BRNE bounded_Yplus
		RET							; Y+ > BOARD_ROWS indicates cell is outside board <-> dead cell
	bounded_Yplus:
	MOV XL, ZL
	MOV XH, ZH
	ADIW X, (5+2*PADDING)
	LD R16, X
	AND R16, BITMASK
	BRNE liveYplus
		RET
	liveYplus: INC NEIGHBOURS
	RET

TestXminus:
	MOV R16, COORD_X
	SUBI R16, 1
	BRPL positive_Xminus
		RET							; Negative X indicates cell is outside board <-> dead cell
	positive_Xminus:
	MOV XL, ZL
	MOV XH, ZH
	MOV R16, BITMASK
	LSL R16
	BRNE same_byte1
		SBIW X, 1
		LD R17, X
		ANDI R17, 0b00000001
		BRNE liveXminus
			RET
	same_byte1:
	LD R17, X
	AND R17, R16
	BRNE liveXminus
		RET
	liveXminus: INC NEIGHBOURS
	RET

TestXplus:
	MOV R16, COORD_X
	INC R16
	CPI R16, BOARD_COL
	BRNE bounded_Xplus
		RET							; X+ > BOARD_COL indicates cell is outside board <-> dead cell
	bounded_Xplus:
	MOV XL, ZL
	MOV XH, ZH
	MOV R16, BITMASK
	LSR R16
	BRNE same_byte2
		ADIW X, 1
		LD R17, X
		ANDI R17, 0b10000000
		BRNE liveXplus
			RET
	same_byte2:
	LD R17, X
	AND R17, R16
	BRNE liveXplus
		RET
	liveXplus: INC NEIGHBOURS
	RET

TestXYminus:
	MOV R16, COORD_X
	MOV R17, COORD_Y 
	SUBI R16, 1
	BRPL PC+2
		RET							; Negative X indicates cell is outside board <-> dead cell
	SUBI R17, 1
	BRPL positive_XYminus
		RET							; Negative Y indicates cell is outside board <-> dead cell
	positive_XYminus:
	MOV XL, ZL
	MOV XH, ZH
	SBIW X, (5+2*PADDING)
	MOV R16, BITMASK
	LSL R16
	BRNE same_Xbyte
		SBIW X, 1
		LD R17, X
		ANDI R17, 0b00000001
		BRNE liveXminus
			RET
	same_Xbyte:
	LD R17, X
	AND R17, R16
	BRNE liveXYminus
		RET
	liveXYminus: INC NEIGHBOURS
	RET

TestXYplus:
	MOV R16, COORD_X
	MOV R17, COORD_Y 
	INC R17
	CPI R17, BOARD_ROWS
	BRNE PC+2
		RET							; Y+ > BOARD_ROWS indicates cell is outside board <-> dead cell
	INC R16
	CPI R16,BOARD_COL
	BRNE bounded_XYplus
		RET							; X+ > BOARD_COL indicates cell is outside board <-> dead cell
	bounded_XYplus:
	MOV XL, ZL
	MOV XH, ZH
	ADIW X, (5+2*PADDING)
	MOV R16, BITMASK
	LSR R16
	BRNE same_Xbyte4
		ADIW X, 1
		LD R17, X
		ANDI R17, 0b10000000
		BRNE liveXYplus
			RET
	same_Xbyte4:
	LD R17, X
	AND R17, R16
	BRNE liveXYplus
		RET
	liveXYplus: INC NEIGHBOURS
	RET

TestXplusYminus:
	MOV R16, COORD_X
	MOV R17, COORD_Y 
	INC R16
	CPI R16, BOARD_COL
	BRNE PC+2
		RET							; X+ > BOARD_COL indicates cell is outside board <-> dead cell
	SUBI R17, 1
	BRPL bounded_XplusYminus
		RET							; Negative Y indicates cell is outside board <-> dead cell
	bounded_XplusYminus:
	MOV XL, ZL
	MOV XH, ZH
	SBIW X, (5+2*PADDING)
	MOV R16, BITMASK
	LSR R16
	BRNE same_Xbyte2
		ADIW X, 1
		LD R17, X
		ANDI R17, 0b10000000
		BRNE liveXplusYminus
			RET
	same_Xbyte2:
	LD R17, X
	AND R17, R16
	BRNE liveXplusYminus
		RET
	liveXplusYminus: INC NEIGHBOURS
	RET

TestXminusYplus:
	MOV R16, COORD_X
	MOV R17, COORD_Y 
	INC R17
	CPI R17, BOARD_ROWS
	BRNE PC+2
		RET							; Y+ > BOARD_ROWS indicates cell is outside board <-> dead cell
	SUBI R16, 1
	BRPL bounded_XminusYplus
		RET							; Negative X indicates cell is outside board <-> dead cell
	bounded_XminusYplus:
	MOV XL, ZL
	MOV XH, ZH
	ADIW X, (5+2*PADDING)
	MOV R16, BITMASK
	LSL R16
	BRNE same_Xbyte3
		SBIW X, 1
		LD R17, X
		ANDI R17, 0b00000001
		BRNE liveXminusYplus
			RET
	same_Xbyte3:
	LD R17, X
	AND R17, R16
	BRNE liveXminusYplus
		RET
	liveXminusYplus: INC NEIGHBOURS
	RET

.undef COORD_X
.undef COORD_Y
.undef NEIGHBOURS
.undef BITMASK
.undef IDX
/*
 * kb_subroutines.inc
 *
 *  Created: 04-05-18 22:36:26
 *   Author: Charles
 */ 

Poll_kb:
	 ;Neutralize inactive rows
	SBI PORTD,6
	SBI PORTD,5
	SBI PORTD,4

	CBR STATE,HELD_BIT			; Clear HELB_DBIT to allow button hold test

	; Send a 0 to keyboard row => Active
	CBI PORTD,7
	NOP
	SBIS PIND,0
	NOP								; Buttons F->7
	SBIS PIND,1
	RCALL Load_squad
	SBIS PIND,2
	RCALL Load_gun
	SBIS PIND,3
	RCALL Load_pulsar
	SBI PORTD,7

	CBI PORTD,6
	NOP
	SBIS PIND,0
	NOP								; Buttons E->4
	SBIS PIND,1
	NOP
	SBIS PIND,2
	RCALL Up
	SBIS PIND,3
	NOP
	SBI PORTD,6

	CBI PORTD,5
	NOP
	SBIS PIND,0
	NOP								; Buttons D->1
	SBIS PIND,1
	RCALL Right
	SBIS PIND,2
	RCALL Set_cell
	SBIS PIND,3
	RCALL Left
	SBI PORTD,5

	CBI PORTD,4
	NOP
	SBIS PIND,0
	NOP								; Buttons C->A
	SBIS PIND,1
	NOP
	SBIS PIND,2
	RCALL Down
	SBIS PIND,3
	RCALL Clear
	SBI PORTD,4

	SBRS STATE,HELD_BIT_IDX		; Check if a button is being held
	CBR STATE,PRESS_BIT			; Clear the PRESS_BIT, the button can now be pressed again
	RET

Up:
	SBR STATE,HELD_BIT 		; Indicate the button is being held
	SBRS STATE,PRESS_BIT_IDX
		RJMP PC+2
		RET						; Return if the T-flag is set
	SBR STATE,PRESS_BIT		; Set the Press-flag, indicating a button was pressed

	;Out-of-bounds handling
	LDXI R18, y_cursor
	CPI R18, MATRIX_ROWS-1
	BRLO go_up
		; Entered if the cursor is out of bounds
		CLR R18
		ST X, R18									; Set the new y cursor coordinate
		MOV R16, CUR_BYTE_IDX
		SUBI R16, (MATRIX_ROWS-1)*(5+2*PADDING)		; Wrap around the visible board space
		LDXI R17, cur_byte							
		X2CB
		ST X, R17									; Restore the previous cursor byte
		MOV CUR_BYTE_IDX, R16						; Set the new cursor byte index
		SBIW X, MATRIX_ROWS/2*(5+2*PADDING)			; Move the pointer position
		SBIW X, MATRIX_ROWS/2*(5+2*PADDING-1)		; In two steps as one step is out of range for SBIW
		LD R17,X									; Load new cursor byte
		MOV R16, R17
		OR R16, CUR_BITMASK			; Sets the value at current bit position to 1
		ST X, R16
		STXI cur_byte, R17							; Set new cursor byte copy
		RET
	; What follows is essentially the same as the indented block but for in-bounds operation
	go_up:
	INC R18
	ST X, R18
	MOV R16, CUR_BYTE_IDX
	SUBI R16, -(5+2*PADDING)
	LDXI R17, cur_byte			; Load cursor byte to R17
	X2CB						; Set X register to cursor byte position
	ST X, R17
	MOV CUR_BYTE_IDX, R16
	ADIW X, (5+2*PADDING)
	LD R17,X
	MOV R16, R17
	OR R16, CUR_BITMASK			; Sets the value at current bit position to 1
	ST X, R16
	STXI cur_byte, R17
	RET

Down:
	SBR STATE,HELD_BIT 		; Indicate the button is being held
	SBRS STATE,PRESS_BIT_IDX
		RJMP PC+2
		RET						; Return if the Press-flag is set
	SBR STATE,PRESS_BIT		; Set the Press-flag, indicating a button was pressed

	;Out-of-bounds handling
	LDXI R18, y_cursor
	DEC R18
	BRPL go_down
		LDI R18, MATRIX_ROWS-1
		ST X, R18
		MOV R16, CUR_BYTE_IDX
		SUBI R16, -(MATRIX_ROWS-1)*(5+2*PADDING)
		LDXI R17, cur_byte
		X2CB
		ST X, R17
		MOV CUR_BYTE_IDX, R16
		ADIW X, MATRIX_ROWS/2*(5+2*PADDING)
		ADIW X, MATRIX_ROWS/2*(5+2*PADDING-1)
		LD R17,X
		MOV R16, R17
		OR R16, CUR_BITMASK			; Sets the value at current bit position to 1
		ST X, R16
		STXI cur_byte, R17
		RET

	go_down:
	ST X, R18
	MOV R16, CUR_BYTE_IDX
	SUBI R16, (5+2*PADDING)
	LDXI R17, cur_byte
	X2CB
	ST X, R17
	MOV CUR_BYTE_IDX, R16
	SBIW X, (5+2*PADDING)
	LD R17,X
	MOV R16, R17
	OR R16, CUR_BITMASK			; Sets the value at current bit position to 1
	ST X, R16
	STXI cur_byte, R17
	RET

Left:
	SBR STATE,HELD_BIT 		; Indicate the button is being held
	SBRS STATE,PRESS_BIT_IDX
		RJMP PC+2
		RET
	SBR STATE,PRESS_BIT
	LDXI R18, x_cursor
	INC R18
	LDXI R16, cur_byte
	X2CB						; Set X register to cursor byte position
	LSR CUR_BITMASK				; Shift the bitmask to the new cursor position
	BRNE update_board
		; Entered if the cursor is in a new byte
		ST X+, R16				; Restore the previous cursor byte and increment X
		INC CUR_BYTE_IDX
		CPI R18, MATRIX_COL		; Out-of-bounds handling
		BRLO go_left
			CLR R18
			SUBI CUR_BYTE_IDX, 5
			SBIW X, 5
		go_left:
		LD R16, X				; Load the new cursor byte
		STXI cur_byte, R16		; Set the new cursor byte copy
		X2CB
		LDI CUR_BITMASK, 0b10000000
	update_board:
	OR R16, CUR_BITMASK			; Sets the value at current bit position to 1
	ST X, R16
	STXI x_cursor,R18			; Set the new x cursor coordinate
	RET

Right:
	SBR STATE,HELD_BIT 		; Indicate the button is being held
	SBRS STATE,PRESS_BIT_IDX
		RJMP PC+2
		RET	
	SBR STATE,PRESS_BIT
	LDXI R18, x_cursor
	DEC R18
	LDXI R16, cur_byte
	X2CB						; Set X register to cursor byte position
	LSL CUR_BITMASK
	BRNE update_board2
		ST X, R16
		SBIW X, 1
		DEC CUR_BYTE_IDX
		CPI R18, 0
		BRPL go_right
			LDI R18, MATRIX_COL-1
			SUBI CUR_BYTE_IDX, -5
			ADIW X, 5
		go_right:
		LD R16, X
		STXI cur_byte, R16
		X2CB
		LDI CUR_BITMASK, 0b00000001
	update_board2:
	OR R16, CUR_BITMASK			; Sets the value at current bit position to 1
	ST X, R16
	STXI x_cursor,R18
	RET

Set_cell:
	SBR STATE,HELD_BIT 		; Indicate the button is being held
	SBRS STATE,PRESS_BIT_IDX
		RJMP PC+2
		RET
	SBR STATE,PRESS_BIT	
	LDXI R18, cur_byte
	MOV R16, R18
	AND R16, CUR_BITMASK
	BREQ set_bit
	unset_bit:
	MOV R16, CUR_BITMASK
	LDI R17, 0xFF
	EOR R16, R17				; XOR(R16,0xFF) = NOT(R16)
	AND R18, R16
	STXI cur_byte, R18
	RET
	set_bit:	
	OR R18, CUR_BITMASK
	STXI cur_byte, R18
	RET

Load_pulsar:
	RCALL Clear_copy
	PUSH ZL
	PUSH ZH
	LDI XL, low(board)
	LDI XH, high(board)
	LDI ZL, low(pulsar<<1)
	LDI ZH, high(pulsar<<1)

	pulsarcpy:
	LPM R16, Z+
	ST X+, R16
	CPI XL, low(board_eol)
	BRNE pulsarcpy
	CPI XH, high(board_eol)
	BRNE pulsarcpy

	POP ZH
	POP ZL
	RET

Load_gun:
	RCALL Clear_copy
	PUSH ZL
	PUSH ZH
	LDI XL, low(board)
	LDI XH, high(board)
	LDI ZL, low(Gun<<1)
	LDI ZH, high(Gun<<1)

	guncpy:
	LPM R16, Z+
	ST X+, R16
	CPI XL, low(board_eol)
	BRNE guncpy
	CPI XH, high(board_eol)
	BRNE guncpy

	POP ZH
	POP ZL
	RET

Load_squad:
	RCALL Clear_copy
	PUSH ZL
	PUSH ZH
	LDI XL, low(board)
	LDI XH, high(board)
	LDI ZL, low(Squad<<1)
	LDI ZH, high(Squad<<1)

	squadcpy:
	LPM R16, Z+
	ST X+, R16
	CPI XL, low(board_eol)
	BRNE squadcpy
	CPI XH, high(board_eol)
	BRNE squadcpy

	POP ZH
	POP ZL
	RET

Clear:
	; Reinitialize SRAM-memory to 0 in board space
	LDI XH, high(board)
	LDI XL, low(board)
	CLR R0
	ClearLoop: ST X+,R0
	CPI XL,low(board_eol)
	BRNE ClearLoop
	CPI XH,high(board_eol)
	BRNE ClearLoop

	RCALL Clear_copy

	STXI cur_byte,R0

	RET

Clear_copy:
	; Reinitialize SRAM-memory to 0 in board_copy space
	LDI XH, high(boardcpy)
	LDI XL, low(boardcpy)
	CLR R0
	ClearLoop2: ST X+,R0
	CPI XL,low(boardcpy_eol)
	BRNE ClearLoop2
	CPI XH,high(boardcpy_eol)
	BRNE ClearLoop2
	RET

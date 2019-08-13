;
; GameOfLife.asm
;
; Created: 30/04/2018 15:35:18
; Author : Charles
;
; Registers R16,R17,R18,R19,R20 are open for calculation
; Registers R21,R22,R23,R24,R25 are used for control logic

; Naming convention: - register names are UPPERCASE
;					 - memory and flow labels are lowercase
;					 - subroutines are Capitalized
;					 - loops are CamelCase

.INCLUDE "m328pdef.inc"			; Load addresses of (I/O) registers
.INCLUDE "gofdef.inc"


.dseg
; Preallocate the board data-memory space at label "board"
board: .byte 70
cur_byte: .byte 1
.cseg

.equ board_eol = board+70

.ORG 0x0000
RJMP init						; First instruction that is executed by the microcontroller

;======================================================================
;								INIT
;======================================================================
init:

CLT								; Clear the T-Flag to enable keyboard operation

; Configure input pin PD0
CBI DDRD,0						; Pin PD0 is an input
SBI PORTD,0						; Enable the pull-up resistor

; Configure input pin PD0
CBI DDRD,1						; Pin PD1 is an input
SBI PORTD,1						; Enable the pull-up resistor

; Configure input pin PD0
CBI DDRD,2						; Pin PD2 is an input
SBI PORTD,2						; Enable the pull-up resistor

; Configure input pin PD0
CBI DDRD,3						; Pin PD3 is an input
SBI PORTD,3						; Enable the pull-up resistor

; Configure output pin PC2
SBI DDRD,7						; Pin PD7 is an output
SBI PORTD,7						; Keyboard row at 1 => Neutral

; Configure output pin PC2
SBI DDRD,6						; Pin PD6 is an output
SBI PORTD,6						; Keyboard row at 1 => Neutral

; Configure output pin PC2
SBI DDRD,5						; Pin PD5 is an output
SBI PORTD,5						; Keyboard row at 1 => Neutral

; Configure output pin PC2
SBI DDRD,4						; Pin PD4 is an output
SBI PORTD,4						; Keyboard row at 1 => Neutral

; Configure output pin PB3 (SBI)
SBI DDRB,3						; Pin PB3 is an output
CBI PORTB,3						; Output GND => SBI at 0
; Configure output pin PB4 (Shift Reg output & latch control)
SBI DDRB,4						; Pin PB4 is an output
CBI PORTB,4						; Output GND => Output OFF
; Configure output pin PB5 (Shift Reg Clk)
SBI DDRB,5						; Pin PB5 is an output
CBI PORTB,5						; Output GND => Clock at 0

LDI ACTIVE_ROW,0b01000000		; Stores the current active row, MSB is the inactive pin
LDI ROWS_REMAINING, 7			; Number of LED_MATRIX char_array's
LDI CUR_BYTE_IDX, 0
LDI CUR_BITMASK, 0b10000000		; Led_matrix origin is lower-right corner (upper-left upside down)
LDI R16,0b00000000
STXI cur_byte, R16

; Initialize SRAM-memory to 0 in board space
LDI R19,high(board) 
LDI R20,low(board)
MOV ZH, R19
MOV ZL, R20
CLR R0
rst: ST Z+,R0
CPI ZL,low(board_eol)
BRNE rst

; Reset the data address
MOV ZH, R19						; Data memory adress MSB
MOV ZL, R20						; LSB
; Initialize cursor
LDI R16,0b10000000
ST Z,R16


;======================================================================
;								MAIN
;======================================================================
main:
;============================================
;			LED MATRIX MANAGEMENT
;============================================

LDI R16,5						; 80 columns, 8 bit per data register
ColumnLoop1:					; Loading one 5 bytes row
	LD R17,Z+					; Indirect load from (Z) with increment
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
ADIW Z,30						; Line 2 to Line 8 ---> +6*(5 bytes) = 30 bytes
LDI R16,5
ColumnLoop2:
	LD R17,Z+
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
SBIW Z,35						; Line 9 to Line 2 ---> -7*(5 bytes) = -35 bytes

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

LDI R16,0xFF					; Prewait allows the activated row to stay longer -> brightness control 
WaitLoop11:
	NOP
	LDI R17,0x06
	WaitLoop12:
		NOP
		DEC R17
		BRNE WaitLoop12
	DEC R16
	BRNE WaitLoop11
CBI PORTB,4
SBI PORTB,4						; Rising edge on PB4 copies shift register to latch
LDI R16,0xFF					; Wait at least 100 us (16Mhz master Clk --> at least 1600 cycles)
WaitLoop21:						; Postwait avoids glitches when output is enabled
	NOP
	LDI R17,0x06
	WaitLoop22:
		NOP
		DEC R17
		BRNE WaitLoop22
	DEC R16
	BRNE WaitLoop21
CBI PORTB,4						; PB4 at 0 enable output

LSR ACTIVE_ROW					; Shift current active row
BRNE PC+2
LDI ACTIVE_ROW,0b01000000

DEC ROWS_REMAINING				; Size of data block is known, used to find end-of-line			
BRNE PC+2						; Check if data memory reached end of line
RCALL ResetDataAdr

;============================================
;			KEYBOARD MANAGEMENT
;============================================

;Neutralize inactive rows
SBI PORTD,6
SBI PORTD,5
SBI PORTD,4

CLR R18								; Clear R18 to allow button hold test

; Send a 0 to keyboard row => Active
CBI PORTD,7
NOP
SBIS PIND,0
NOP								; Buttons F->7
SBIS PIND,1
NOP
SBIS PIND,2
NOP
SBIS PIND,3
NOP
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
NOP
SBI PORTD,4

CPI R18,0xFF						; Check if a button is being held
BREQ PC+2
CLT								; Clear the T-Flag, the button can now be pressed again


RJMP main						; Create an infinite loop

;======================================================================
;							SUBROUTINES
;======================================================================


ResetDataAdr:
	; Reset the data address
	LDI R19,high(board) 
	LDI R20,low(board)
	MOV ZH, R19					; Program memory adress MSB
	MOV ZL, R20					; LSB
	LDI ROWS_REMAINING,7
	RET

Up:
	BRTC PC+3
		SER R18					; Indicate the button is being held
		RET						; Return if the T-flag is set
	SET							; Set the Test-flag, indicating a button was pressed
	MOV R16, CUR_BYTE_IDX
	SUBI R16, -5
	CPI R16, 70
	BRLO PC+2
		RET
	LDXI R17, cur_byte
	CB2X
	ST X, R17
	MOV CUR_BYTE_IDX, R16
	SUBI XL, -5
	LD R17,X
	MOV R16, R17
	OR R16, CUR_BITMASK			; Sets the value at current bit position to 1
	ST X, R16
	RET

Down:
	BRTC PC+3
		SER R18					; Indicate the button is being held
		RET						; Return if the T-flag is set
	SET							; Set the Test-flag, indicating a button was pressed
	MOV R16, CUR_BYTE_IDX
	SUBI R16, 5
	BRPL PC+2
		RET
	LDI XH, high(board)
	LDI XL, low(board)
	ADD XL, CUR_BYTE_IDX
	ST X, CUR_BYTE
	MOV CUR_BYTE_IDX, R16
	SUBI XL, 5
	LD CUR_BYTE,X
	MOV R16, CUR_BYTE
	OR R16, CUR_BITMASK			; Sets the value at current bit position to 1
	ST X, R16
	RET

Left:
	BRTS PC+3
		SER R18					; Indicate the button is being held
		RET						; Return if the T-flag is set
	SET							; Set the Test-flag, indicating a button was pressed
	LDI XH, high(board)
	LDI XL, low(board)
	ADD XL, CUR_BYTE_IDX	; X contains the cursor byte adress
	LSR CUR_BITMASK
	BRNE update_board
		ST X, CUR_BYTE
		INC XL
		INC CUR_BYTE_IDX
		CPI CUR_BYTE_IDX,70
		BRLO PC+3				; Overflow handling
			LDI CUR_BYTE_IDX, 0
			LDI XL, low(board)
		LD CUR_BYTE, X
		LDI CUR_BITMASK, 0b10000000
	update_board:
	MOV R16, CUR_BYTE
	OR R16, CUR_BITMASK			; Sets the value at current bit position to 1
	ST X, R16
	RET

Right:
	BRTC PC+3
		SER R18					; Indicate the button is being held
		RET						; Return if the T-flag is set
	SET							; Set the Test-flag, indicating a button was pressed
	LDI XH, high(board)
	LDI XL, low(board)
	ADD XL, CUR_BYTE_IDX	; X contains the cursor byte adress
	LSL CUR_BITMASK
	BRNE update_board2
		ST X, CUR_BYTE
		DEC XL
		DEC CUR_BYTE_IDX
		BRPL PC+4				; Overflow handling
			LDI CUR_BYTE_IDX, 69
			LDI XL, low(board_eol)
			DEC XL
		LD CUR_BYTE, X
		LDI CUR_BITMASK, 0b00000001
	update_board2:
	MOV R16, CUR_BYTE
	OR R16, CUR_BITMASK			; Sets the value at current bit position to 1
	ST X, R16
	RET

Set_cell:
	BRTC PC+3
		SER R18					; Indicate the button is being held
		RET						; Return if the T-flag is set
	SET							; Set the Test-flag, indicating a button was pressed
	MOV R16, CUR_BYTE
	AND R16, CUR_BITMASK
	BREQ bit_set
	bit_unset:
	MOV R16, CUR_BITMASK
	LDI R17, 0xFF
	EOR R16, R17				; XOR(R16,0xFF) = NOT(R16)
	AND CUR_BYTE, R16
	RET
	bit_set:	
	OR CUR_BYTE, CUR_BITMASK
	RET

/*board: .db 0b10000000,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
*/
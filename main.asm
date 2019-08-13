;
; GameOfLife.asm
;
; Created: 30/04/2018 15:35:18
; Author : Charles Detemmerman 
;
; Registers R0 -> R20 are open for calculation
; Registers R21,R22,R23,R24,R25 are used for control logic
; <!> The pointer Y is only used by the screen refresh loop, it cannot be modified anywhere, even if pushed </!>
; This is voluntary as the refresh loop is entered extremely often and has to remember where the last memory position was

; Naming convention: - register names and constants are UPPERCASE
;					 - memory and flow labels are lowercase
;					 - subroutines are Capitalized
;					 - loops are CamelCase
; Always followed except when it's not
; Pretty > Functional
; Best enjoyed in 6pt Comic Sans
; 100% Gluten Free Code

.INCLUDE "m328pdef.inc"				; Load addresses of (I/O) registers
.INCLUDE "goldef.inc"


.dseg
; Preallocate the board data-memory space at label "board"
board: .byte BOARD_SIZE
boardcpy: .byte BOARD_SIZE
cur_byte: .byte 1					; Contains the active copy of the byte at the cursor
x_cursor: .byte 1					; Contains the X coordinate of the cursor, simplifies out of bounds detection when padding > 0
y_cursor: .byte 1
.cseg

.equ board_eol = board+BOARD_SIZE
.equ boardcpy_eol = boardcpy+BOARD_SIZE
.equ matrix_start = PADDING*(BOARD_COL + 1)			; Matrix start at (BOARD_COL*(8*PADDING) + 8)/8



.ORG 0x0000
RJMP init							; First instruction that is executed by the microcontroller
.ORG 0x001A							; See Datasheet: 12.4 Interrupt Vectors in ATmega328 and ATmega328P
RJMP Timer1OverflowInterrupt
.ORG 0x0020
RJMP Timer0OverflowInterrupt


;======================================================================
;								INIT
;======================================================================
init:

; Configure input pin PB0 (Switch)
CBI DDRB,0							; Pin PB0 is an input
SBI PORTB,0							; Enable the pull-up resistor

; Configure output pin PC2
SBI DDRC,2							; Pin PC2 is an output
SBI PORTC,2							; Output Vcc => LED1 is turned off!

; Configure input pin PD0
CBI DDRD,0							; Pin PD0 is an input
SBI PORTD,0							; Enable the pull-up resistor

; Configure input pin PD0
CBI DDRD,1							; Pin PD1 is an input
SBI PORTD,1							; Enable the pull-up resistor

; Configure input pin PD0
CBI DDRD,2							; Pin PD2 is an input
SBI PORTD,2							; Enable the pull-up resistor

; Configure input pin PD0
CBI DDRD,3							; Pin PD3 is an input
SBI PORTD,3							; Enable the pull-up resistor

; Configure output pin PC2
SBI DDRD,7							; Pin PD7 is an output
SBI PORTD,7							; Keyboard row at 1 => Neutral

; Configure output pin PC2
SBI DDRD,6							; Pin PD6 is an output
SBI PORTD,6							; Keyboard row at 1 => Neutral

; Configure output pin PC2
SBI DDRD,5							; Pin PD5 is an output
SBI PORTD,5							; Keyboard row at 1 => Neutral

; Configure output pin PC2
SBI DDRD,4							; Pin PD4 is an output
SBI PORTD,4							; Keyboard row at 1 => Neutral

; Configure output pin PB3 (SBI)
SBI DDRB,3							; Pin PB3 is an output
CBI PORTB,3							; Output GND => SBI at 0
; Configure output pin PB4 (Shift Reg output & latch control)
SBI DDRB,4							; Pin PB4 is an output
CBI PORTB,4							; Output GND => Output OFF
; Configure output pin PB5 (Shift Reg Clk)
SBI DDRB,5							; Pin PB5 is an output
CBI PORTB,5							; Output GND => Clock at 0

; Timer setup

LDI R16,0b00000100					; Set timer0 (8 bits) prescaler to 64 [00000011] or 256 [00000100]
OUT TCCR0B,R16
LDI R16,SCRSTART
OUT TCNT0, R16

LDI R16,0b00000100					; Set timer1 (16 bits) prescaler to 256 [00000100]
STS TCCR1B,R16
LDI R16,high(TIMERSTART)
LDI R17,low(TIMERSTART)	
STS TCNT1H,R16
STS TCNT1L,R17

; Control registers setup
LDI ACTIVE_ROW,0b01000000			; Stores the current active row index, MSB is the inactive pin
LDI ROWS_REMAINING, SHFTREG_ROWS	; Number of LED_MATRIX char_array's
LDI CUR_BYTE_IDX, matrix_start
LDI CUR_BITMASK, 0b10000000			; Led_matrix origin is lower-right corner (upper-left upside down)
CLR STATE
LDI R16,0b00000000
STXI cur_byte, R16
CLR R0
STXI x_cursor, R0
STXI y_cursor, R0

; Initialize SRAM-memory to 0 in board space
LDI ZH, high(board)
LDI ZL, low(board)
CLR R0
rst: ST Z+,R0
CPI ZL,low(board_eol)
BRNE rst

LDI ZH, high(boardcpy)
LDI ZL, low(boardcpy)
CLR R0
rst2: ST Z+,R0
CPI ZL,low(boardcpy_eol)
BRNE rst2

RCALL Load_GOL							; Load the start screen into game memory


; Set the data address at start of led matrix
LDI YH, high(board+matrix_start)		; Data memory adress MSB
LDI YL, low(board+matrix_start)			; LSB
; Initialize cursor
LDI R16,0b10000000
ST Y,R16

LDI R16,0b10000000					; Enable global interrupt
OUT SREG,R16
LDI R16,0b00000001					; Enable timer0 interrupt
STS TIMSK0,R16
LDI R16,0b00000000					; Disable timer1 interrupt
STS TIMSK1,R16

;======================================================================
;								MAIN
;======================================================================
main:

; LED MATRIX MANAGEMENT
/*RCALL Print_board*/			; Now done in interrupt

LDI R16,0xFF					; Wait to avoid button bounce glitches when refreshing screen in interrupt
WaitLoop31:
	NOP
	LDI R17,40
	WaitLoop32:
		NOP
		DEC R17
		BRNE WaitLoop32
	DEC R16
	BRNE WaitLoop31

IN R0,PINB							; Get value of PINB
BST R0,0							; Copy PB2 (bit 0 of PINB) to the T flag

									; The switch is high if the T flag is set
BRTS run_mode						; Branch if the T flag is set

edit_mode:
; KEYBOARD MANAGEMENT
LDI R16,0b00000000					; Disable timer1 interrupt
STS TIMSK1,R16
RCALL Poll_kb				
RJMP main


run_mode:
LDS R16,TIMSK1
BST R16,0
BRTS main						; Branch if the T flag is set
LDXI R16, cur_byte
X2CB
ST X,R16
LDI R16,0b00000001					; Enable timer1 interrupt
STS TIMSK1,R16
RJMP main							; Create an infinite loop


;======================================================================
;							SUBROUTINES
;======================================================================
; Strange error can happen where assembler can't detect the subroutine at build, just cut and paste the include a few times it should work
.INCLUDE "sim_subroutines.inc"
.INCLUDE "kb_subroutines.inc"
.INCLUDE "ldmatrix_subroutines.inc"

Load_GOL:
	LDI XL, low(board)
	LDI XH, high(board)
	LDI ZL, low(GOL<<1)
	LDI ZH, high(GOL<<1)

	golcpy:
	LPM R16, Z+
	ST X+, R16
	CPI XL, low(board_eol)
	BRNE golcpy
	CPI XH, high(board_eol)
	BRNE golcpy
	RET

;======================================================================
;							DATA
;======================================================================

Pulsar: .db 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,227,128,0,0,0,0,0,0,0,0,0,0,0,2,20,32,0,0,0,0,2,20,32,0,0,0,0,2,20,32,0,0,0,0,0,227,128,0,0,0,0,0,0,0,0,0,0,0,0,227,128,0,0,0,0,2,20,32,0,0,0,0,2,20,32,0,0,0,0,2,20,32,0,0,0,0,0,0,0,0,0,0,0,0,227,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
Gun: .db 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,192,0,0,0,0,0,2,32,0,0,0,0,4,4,16,0,0,0,0,5,13,16,12,0,0,0,0,196,16,12,0,0,48,0,194,32,0,0,0,48,0,192,192,0,0,0,0,5,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
GOL: .db 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,60,47,190,1,124,0,0,4,34,2,1,68,0,0,28,226,2,7,68,0,0,4,34,2,1,68,0,0,61,239,130,15,124,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,245,81,124,0,0,0,0,21,81,68,0,0,0,0,117,95,116,0,0,0,0,21,81,4,0,0,0,1,247,223,124,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
Squad: .db 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14,56,227,142,0,0,0,8,32,130,8,0,0,0,4,16,65,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14,56,227,142,0,0,0,8,32,130,8,0,0,0,4,16,65,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14,56,227,142,0,0,0,8,32,130,8,0,0,0,4,16,65,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

/*
 * goldef.inc
 *
 *  Created: 04-05-18 17:50:54
 *   Author: Charles
 */ 

.def CUR_BYTE_IDX = R23				; Contains the byte index of the cursor in board 
.def CUR_BITMASK = R24				; Contains the bitmask of the cursor in byte
.def STATE = R25					; Contains program state flags
.def ACTIVE_ROW = R21
.def ROWS_REMAINING = R22			; Contains the number of rows to display before restarting at the first

; Timing of the simulation
/*.equ TIMERSTART = 3036			; 1 Hz with 256 prescaler*/
.equ TIMERSTART = 34086				; 2 Hz with 256 prescaler

; Timing of the screen refresh
/*.equ SCRSTART = 6;					; 1kHz with 64 prescaler*/
.equ SCRSTART = 131;				; 500Hz with 256 prescaler


.equ MATRIX_ROWS = 14
.equ MATRIX_COL = 40
.equ MATRIX_SIZE = MATRIX_ROWS*MATRIX_COL/8
.equ SHFTREG_ROWS = MATRIX_ROWS/2	; Shift_register rows = board_rows/2

; Sets the extra space around the matrix to allow interaction near the borders
.equ PADDING = 1							; in bytes
.equ BOARD_ROWS = MATRIX_ROWS + 16*PADDING	; in bits
.equ BOARD_COL = MATRIX_COL + 16*PADDING
.equ BOARD_SIZE = BOARD_ROWS*BOARD_COL/8	; in bytes

; State flags
.equ HELD_BIT = 0b10000000
.equ HELD_BIT_IDX = 7
.equ PRESS_BIT = 0b01000000
.equ PRESS_BIT_IDX = 6


.macro LDXI
; Load immediate data memory adress with X

LDI XL, low(@1)
LDI XH, high(@1)
LD @0,X
.endmacro
 
.macro STXI
; Store to immediate data memory adress with X
LDI XL, low(@0)
LDI XH, high(@0)
ST X,@1
.endmacro

.macro X2CB
; Sets the X pointer to the cursor position on the board (NB: not to the local copy)

LDI XH, high(board)
LDI XL, low(board)
ADD XL, CUR_BYTE_IDX
.endmacro

.macro T1_PUSHREGS
; Push the registers used by the Timer 1 interrupt

PUSH ZH
PUSH ZL
PUSH XH
PUSH XL
PUSH R0
PUSH R2
PUSH R3
PUSH R4
PUSH R5
PUSH R6
PUSH R7
PUSH R8
PUSH R16
PUSH R17
PUSH R18
PUSH R19
LDS R0, SREG
PUSH R0
.endmacro

.macro T1_POPREGS
POP R0
STS SREG, R0
POP R19
POP R18
POP R17
POP R16
POP R8
POP R7
POP R6
POP R5
POP R4
POP R3
POP R2
POP R0
POP XL
POP XH
POP ZL
POP ZH
.endmacro
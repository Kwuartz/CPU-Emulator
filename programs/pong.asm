; ============================================================
; PONG - 64x32 framebuffer
; Framebuffer base: 4096, size: 2048 (64x32)
; Symbols: 0=space, 1=| (paddle), 2=O (ball)
; Register layout:
;   R0 = ball X        (starts 32)
;   R1 = ball Y        (starts 16)
;   R2 = ball DX       (starts 1)
;   R3 = ball DY       (starts 1)
;   R4 = left paddle Y (starts 14)
;   R5 = right paddle Y(starts 14)
;   R6 = scratch/address
;   R7 = scratch
;   R8 = loop counter
;   R9 = temp
; Paddle height = 5, bounces at X=1 and X=62
; ============================================================

; --- INIT ---
MOV R0, #32
MOV R1, #16
MOV R2, #1
MOV R3, #1
MOV R4, #14
MOV R5, #14

; ============================================================
; MAIN LOOP
; ============================================================
mainloop:

; --- CLEAR FRAMEBUFFER ---
; R8 = 0..2047, write 0 to address 4096+R8
MOV R8, #0
MOV R9, #0
clearloop:
ADD R6, R8, #4096
STR R9, R6
ADD R8, R8, #1
CMP R8, #2048
BLT clearloop

; ============================================================
; DRAW LEFT PADDLE (column 0)
; address = 4096 + row*64
; ============================================================
MOV R6, #0
MOV R9, #1
drawleft:
ADD R7, R4, R6
MUL R7, R7, #64
ADD R7, R7, #4096
STR R9, R7
ADD R6, R6, #1
CMP R6, #5
BLT drawleft

; ============================================================
; DRAW RIGHT PADDLE (column 63)
; address = 4096 + row*64 + 63
; ============================================================
MOV R6, #0
MOV R9, #1
drawright:
ADD R7, R5, R6
MUL R7, R7, #64
ADD R7, R7, #4096
ADD R7, R7, #63
STR R9, R7
ADD R6, R6, #1
CMP R6, #5
BLT drawright

; ============================================================
; DRAW BALL
; address = 4096 + R1*64 + R0
; ============================================================
MUL R6, R1, #64
ADD R6, R6, R0
ADD R6, R6, #4096
MOV R9, #2
STR R9, R6

; ============================================================
; FLUSH + WAIT
; ============================================================
FLUSH
WAIT #50

; ============================================================
; UPDATE BALL POSITION
; ============================================================
ADD R0, R0, R2
ADD R1, R1, R3

; ============================================================
; BOUNCE OFF TOP/BOTTOM WALLS
; ============================================================
CMP R1, #0
BEQ bouncetop
CMP R1, #31
BEQ bouncebottom
B afterwallbounce

bouncetop:
CMP R3, #0
BGT afterwallbounce
MOV R3, #1
B afterwallbounce

bouncebottom:
CMP R3, #0
BLT afterwallbounce
MOV R3, #-1

afterwallbounce:

; ============================================================
; BOUNCE OFF LEFT PADDLE (ball X=1, paddle in col 0)
; ============================================================
CMP R0, #1
BNE afterleftbounce
CMP R2, #0
BGT afterleftbounce
CMP R1, R4
BLT afterleftbounce
ADD R9, R4, #4
CMP R1, R9
BGT afterleftbounce
MOV R2, #1

afterleftbounce:

; ============================================================
; BOUNCE OFF RIGHT PADDLE (ball X=62, paddle in col 63)
; ============================================================
CMP R0, #62
BNE afterrightbounce
CMP R2, #0
BLT afterrightbounce
CMP R1, R5
BLT afterrightbounce
ADD R9, R5, #4
CMP R1, R9
BGT afterrightbounce
MOV R2, #-1

afterrightbounce:

; ============================================================
; RESET BALL if it escapes left or right
; ============================================================
CMP R0, #0
BEQ resetball
CMP R0, #63
BEQ resetball
B afterreset

resetball:
MOV R0, #32
MOV R1, #16
MOV R2, #1
MOV R3, #1

afterreset:

; ============================================================
; AI - LEFT PADDLE tracks ball when DX < 0 (ball coming left)
; Clamp to 0..27 (so paddle rows 0..31 fit)
; ============================================================
CMP R2, #0
BGT skipleftai
SUB R4, R1, #2
CMP R4, #0
BGT leftclampok
MOV R4, #0
leftclampok:
CMP R4, #27
BLT skipleftai
MOV R4, #27

skipleftai:

; ============================================================
; AI - RIGHT PADDLE tracks ball when DX > 0 (ball coming right)
; Clamp to 0..27
; ============================================================
CMP R2, #0
BLT skiprightai
SUB R5, R1, #2
CMP R5, #0
BGT rightclampok
MOV R5, #0
rightclampok:
CMP R5, #27
BLT skiprightai
MOV R5, #27

skiprightai:

B mainloop
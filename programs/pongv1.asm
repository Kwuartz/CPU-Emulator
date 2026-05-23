; ============================================================
; PONG - 100x30 framebuffer
; Framebuffer base: 1024, size: 3000 (100x30)
; Symbols: 0=space, 1=| (paddle), 2=O (ball)
; Register layout:
;   R0 = ball X        (starts 50)
;   R1 = ball Y        (starts 15)
;   R2 = ball DX       (starts 1)
;   R3 = ball DY       (starts 1)
;   R4 = left paddle Y (starts 13)
;   R5 = right paddle Y(starts 13)
;   R6 = scratch/address
;   R7 = scratch
;   R8 = loop counter
;   R9 = temp
; Paddle height = 5, bounces at X=1 and X=98
; ============================================================

; --- INIT ---
MOV R0, #50
MOV R1, #15
MOV R2, #1
MOV R3, #1
MOV R4, #13
MOV R5, #13

; ============================================================
; MAIN LOOP
; ============================================================
mainloop:

; --- CLEAR FRAMEBUFFER ---
; R8 = 0..2999, write 0 to address 1024+R8
MOV R8, #0
MOV R9, #0
clearloop:
ADD R6, R8, #1024
STR R9, R6
ADD R8, R8, #1
CMP R8, #3000
BLT clearloop

; ============================================================
; DRAW LEFT PADDLE (column 0)
; rows R4 to R4+4, address = 1024 + row*100
; ============================================================
MOV R6, #0
MOV R9, #1
drawleft:
ADD R7, R4, R6
MUL R7, R7, #100
ADD R7, R7, #1024
STR R9, R7
ADD R6, R6, #1
CMP R6, #5
BLT drawleft

; ============================================================
; DRAW RIGHT PADDLE (column 99)
; address = 1024 + row*100 + 99
; ============================================================
MOV R6, #0
MOV R9, #1
drawright:
ADD R7, R5, R6
MUL R7, R7, #100
ADD R7, R7, #1024
ADD R7, R7, #99
STR R9, R7
ADD R6, R6, #1
CMP R6, #5
BLT drawright

; ============================================================
; DRAW BALL
; address = 1024 + R1*100 + R0
; ============================================================
MUL R6, R1, #100
ADD R6, R6, R0
ADD R6, R6, #1024
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
CMP R1, #29
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
; BOUNCE OFF RIGHT PADDLE (ball X=98, paddle in col 99)
; ============================================================
CMP R0, #98
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
CMP R0, #99
BEQ resetball
B afterreset

resetball:
MOV R0, #50
MOV R1, #15
MOV R2, #1
MOV R3, #1

afterreset:

; ============================================================
; AI - LEFT PADDLE tracks ball when DX < 0 (ball coming left)
; Target paddleY = ballY - 2 (centre paddle on ball)
; Clamp to 0..25 (so paddle rows 0..29 fit)
; ============================================================
CMP R2, #0
BGT skipleftai
SUB R4, R1, #2
CMP R4, #0
BGT leftclampok
MOV R4, #0
leftclampok:
CMP R4, #25
BLT skipleftai
MOV R4, #25

skipleftai:

; ============================================================
; AI - RIGHT PADDLE tracks ball when DX > 0 (ball coming right)
; ============================================================
CMP R2, #0
BLT skiprightai
SUB R5, R1, #2
CMP R5, #0
BGT rightclampok
MOV R5, #0
rightclampok:
CMP R5, #25
BLT skiprightai
MOV R5, #25

skiprightai:

B mainloop
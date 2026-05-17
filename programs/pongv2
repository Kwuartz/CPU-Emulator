; PONG 100x30 | FB base:1024 | ball=1px | AI=1step
; R0=ballX R1=ballY R2=DX R3=DY
; R4=leftPaddleMidY R5=rightPaddleMidY (paddle height=7, zones: top2=steep, mid3=flat, bot2=steep)
; R10=lScore R11=rScore
; Paddle zones (relative to mid):
;   hit row mid-3 or mid-2 => DY=-2 (steep up)
;   hit row mid-1,mid,mid+1 => DY unchanged (flat)
;   hit row mid+2 or mid+3 => DY=2 (steep down)

MOV R0, #49
MOV R1, #14
MOV R2, #1
MOV R3, #1
MOV R4, #15
MOV R5, #15
MOV R10, #0
MOV R11, #0

mainloop:

; CLEAR
MOV R8, #0
MOV R9, #0
clearloop:
ADD R6, R8, #1024
STR R9, R6
ADD R8, R8, #1
CMP R8, #3000
BLT clearloop

; SCORE
ADD R6, R10, #3
MOV R7, #1034
STR R6, R7
ADD R6, R11, #3
MOV R7, #1113
STR R6, R7

; LEFT PADDLE col0, rows mid-3..mid+3 (7 tall)
SUB R8, R4, #3
MOV R6, #0
MOV R9, #1
drawleft:
ADD R7, R8, R6
MUL R7, R7, #100
ADD R7, R7, #1024
STR R9, R7
ADD R6, R6, #1
CMP R6, #7
BLT drawleft

; RIGHT PADDLE col99, rows mid-3..mid+3
SUB R8, R5, #3
MOV R6, #0
MOV R9, #1
drawright:
ADD R7, R8, R6
MUL R7, R7, #100
ADD R7, R7, #1024
ADD R7, R7, #99
STR R9, R7
ADD R6, R6, #1
CMP R6, #7
BLT drawright

; BALL single pixel
MOV R9, #2
MUL R6, R1, #100
ADD R6, R6, #1024
ADD R6, R6, R0
STR R9, R6

FLUSH
WAIT #16

; MOVE
ADD R0, R0, R2
ADD R1, R1, R3

; WALL BOUNCE Y (clamp 0..29)
CMP R1, #0
BLT bouncetop
CMP R1, #29
BGT bouncebottom
B afterwall
bouncetop:
MOV R1, #0
MOV R3, #1
B afterwall
bouncebottom:
MOV R1, #29
MOV R3, #-1
afterwall:

; LEFT PADDLE BOUNCE
; ball moving left, X<=1, Y in [mid-3..mid+3]
CMP R2, #0
BGT afterleft
CMP R0, #1
BGT afterleft
SUB R7, R4, #3
CMP R1, R7
BLT afterleft
ADD R7, R4, #3
CMP R1, R7
BGT afterleft
; HIT - check zone for DY
MOV R0, #2
MOV R2, #1
; top zone: ballY <= mid-2 => DY=-2
SUB R7, R4, #2
CMP R1, R7
BGT leftzonecheck2
MOV R3, #-2
B afterleft
; bottom zone: ballY >= mid+2 => DY=2
leftzonecheck2:
ADD R7, R4, #2
CMP R1, R7
BLT afterleft
MOV R3, #2
afterleft:

; RIGHT PADDLE BOUNCE
; ball moving right, X>=98, Y in [mid-3..mid+3]
CMP R2, #0
BLT afterright
CMP R0, #98
BLT afterright
SUB R7, R5, #3
CMP R1, R7
BLT afterright
ADD R7, R5, #3
CMP R1, R7
BGT afterright
; HIT
MOV R0, #96
MOV R2, #-1
; top zone: ballY <= mid-2 => DY=-2
SUB R7, R5, #2
CMP R1, R7
BGT rightzonecheck2
MOV R3, #-2
B afterright
; bottom zone: ballY >= mid+2 => DY=2
rightzonecheck2:
ADD R7, R5, #2
CMP R1, R7
BLT afterright
MOV R3, #2
afterright:

; SCORE + RESET
CMP R0, #0
BLT rightscore
CMP R0, #99
BGT leftscore
B afterreset

rightscore:
ADD R11, R11, #1
CMP R11, #10
BLT doreset
MOV R11, #0
B doreset

leftscore:
ADD R10, R10, #1
CMP R10, #10
BLT doreset
MOV R10, #0

doreset:
MOV R0, #49
MOV R1, #14
MOV R2, #1
MOV R3, #1
afterreset:

; LEFT AI (DX<0, max 1 step, tracks ballY)
CMP R2, #0
BGT skipleft
CMP R4, R1
BEQ skipleft
BGT leftup
ADD R4, R4, #1
B leftclamp
leftup:
SUB R4, R4, #1
leftclamp:
CMP R4, #3
BGT leftok
MOV R4, #3
leftok:
CMP R4, #26
BLT skipleft
MOV R4, #26
skipleft:

; RIGHT AI (DX>0, max 1 step, tracks ballY)
CMP R2, #0
BLT skipright
CMP R5, R1
BEQ skipright
BGT rightup
ADD R5, R5, #1
B rightclamp
rightup:
SUB R5, R5, #1
rightclamp:
CMP R5, #3
BGT rightok
MOV R5, #3
rightok:
CMP R5, #26
BLT skipright
MOV R5, #26
skipright:

B mainloop
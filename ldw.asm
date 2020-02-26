	.ORIG x3000
	LEA     R0, B
	LDW	R1, R0, #-1
	LDW     R2, R0, #0
	LDW     R3, R0, #1
	HALT

A       .FILL   xDEFA
B       .FILL   xCEDC
C       .FILL   XCAB0			
	.END
	.ORIG x3000
	BRNP	DONE
A	AND R0, R0, #0
	ADD R0, R0, #1		; 1 is in bit 0
	ADD R0, R0, R0		; 1 is in bit 1
	ADD R0, R0, R0		; 1 is in bit 2
	ADD R0, R0, R0		; 1 is in bit 3
	ADD R0, R0, R0		; 1 is in bit 4
	ADD R0, R0, R0		; 1 is in bit 5
	ADD R0, R0, R0		; 1 is in bit 6
	ADD R0, R0, R0		; 1 is in bit 7
	ADD R0, R0, R0		; 1 is in bit 8
	BRP B
	HALT
B	ADD R0, R0, R0		; 1 is in bit 9
	ADD R0, R0, R0		; 1 is in bit 10
	ADD R0, R0, R0		; 1 is in bit 11
	ADD R0, R0, R0		; 1 is in bit 12
	ADD R0, R0, R0		; 1 is in bit 13
	ADD R0, R0, R0		; 1 is in bit 14
	ADD R0, R0, R0		; 1 is in bit 15
	BRN C
	HALT
C	BRZP	DONE
	ADD R2, R0, #0
	BRNZP DONE
	HALT
DONE	ADD R3, R0, #0
	HALT
	.END



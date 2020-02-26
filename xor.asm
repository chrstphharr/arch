       .ORIG x3000
       AND R0, R0, #0
       AND R1, R1, #0
       ADD R0, R0, #5
       ADD R1, R1, #2
       XOR R3, R0, R1
       XOR R4, R0, #15
       HALT 
        .END	

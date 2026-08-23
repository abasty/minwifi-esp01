10 INPUT "A: ", A: INPUT "B: ", B: X = A: Y = B
20 WHILE Y <> 0: T = X % Y: X = Y: Y = T: WEND
30 PRINT "PGCD = "; X
40 PRINT "PPCM = "; A * B / X

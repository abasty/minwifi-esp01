1 CLS 
10 DIM a$(10,25)
20 LET a$(1)="Alain Basty"
30 LET a$(2,1)="x"
35 PRINT LEN a$(1)
36 LET a$(1,LEN(a$(1)))="y"
50 PRINT a$(1);"."
60 PRINT a$(2);"."
70 PRINT a$(3);"."
100 LET s$="Alain Basty"
110 LET s$(1)="x"
120 LET s$(LEN(s$))="."
150 PRINT s$
200 LET z$=CHR$ 0
210 PRINT LEN z$
220 LET z$=z$+z$+z$
230 PRINT LEN z$
240 FOR i=1 TO LEN z$
250 PRINT CODE z$(i)
260 NEXT i

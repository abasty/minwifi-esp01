5 lk$=""
10 k$=INKEY$ 
20 IF k$="" THEN 10
30 IF k$=lk$ THEN 10
40 PRINT CODE k$
50 lk$=k$
60 GOTO 10

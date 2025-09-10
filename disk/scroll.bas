10 p=50
20 output c$
30 cat
40 output stop
50 cls cursor 0
60 ?c$
100 SCROLL UP
110 FOR i=1 TO p
120 NEXT i
130 SCROLL DOWN
140 FOR i=1 TO p
150 NEXT i
160 GOTO 100

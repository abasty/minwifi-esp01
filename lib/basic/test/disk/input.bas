1 cls

10 a=1
20 c$="toto"

40 ? "Entre 'a' [" STR$(a) "]: ";
50 INPUT a
60 ? "a: ";a
70 IF a<>0 THEN 40

100 ? "Entre 'c$' [" c$ "] ou 'fin' : ";
110 INPUT c$
120 ? "c$: \"" c$ "\""
130 IF c$<>"fin" THEN 100

200 GOTO 40

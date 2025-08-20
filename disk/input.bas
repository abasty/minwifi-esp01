5 a=1
10 a$="a=["+STR$(a)+"]"
20 PRINT a$;
30 INPUT ": ",a
35 PRINT "a=";a
40 IF a<>0 THEN 10
100 INPUT "chaine: ",c$
110 PRINT c$
120 IF c$<>"" THEN 100
130 GOTO 10

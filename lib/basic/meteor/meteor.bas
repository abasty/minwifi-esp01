10 LOAD "meteor.vdt.var"
20 PRINT menu$;
30 INPUT "Choix: ",c
31 c=INT c
40 IF c<1 THEN LET c=1
50 IF c>=5 THEN LET c=5
90 GOSUB c*100
99 GOTO 20
100 PRINT jeu$;cursor 0;
120 goto 1000
200 PRINT records$;
210 IF INKEY$ ="" THEN 210
220 RETURN
300 PRINT config$;
310 IF INKEY$ ="" THEN 310
320 RETURN
400 PRINT credits$;
410 IF INKEY$ ="" THEN 410
420 RETURN
500 CLS
510 END
999 REM "jeu"
1000 sx=20
1100 SCROLL DOWN
1105 gosub 5000
1110 k$=INKEY$
1120 IF k$="a" AND sx>1 THEN LET sx=sx-1
1130 IF k$="e" AND sx<37 THEN LET sx=sx+1
1140 AT 24,sx;"HOH"
1150 if k$="x" then return
1199 PAUSE 100
1200 GOTO 1100
5000 r=int(rnd*100)
5010 e$ = "."
5020 if r >= 35 then let e$ = "+"
5021 if r >= 55 then let e$ = "*"
5022 if r >= 65 then let e$ = "o"
5023 if r >= 75 then let e$ = "'"
5030 ec = int(rnd*40) + 1
5040 at 1,ec;e$
5050 return

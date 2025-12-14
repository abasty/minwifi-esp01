10 PRINT CLS ;LINE0 ;CLEOL
20 name$=GET 250,"name"
100 AT 2,1;PAPER 1;" ";CLEOL
110 PRINT AT 2,10;SIZE 1;"Bienvenue sur BASTOS !";SIZE 0
120 if name$ <> "" then 200
130 ? at 4,1;"Procédons à la configuration de ton\r\nsystème"
140 input "\x0aTon nom: ", n$
150 PUT 250,"name",n$
160 run
200 at 10,14;"Mon Très Cher"
205 name$ = name$(1 to 20)
210 c = 21 - len name$
220 at 12,c;size 2;name$;size 0
230 at 23,1

100 FAST :MODE 1:CLS :CURSOR 0:INS CHAR 1
105 w=40:n=0:fin=0
110 WHILE NOT fin
115 n=n+1
120 AT 1,1;DEL CHAR :IF n%3=0 THEN PRINT AT 1,w;"*";
125 AT 24,1:IF n%3=0 THEN PRINT "*"; ELSE PRINT " ";
130 PAUSE 50
135 IF INKEY$ <>"" THEN LET fin=1
140 WEND
145 INS CHAR 0;CURSOR 1
200 LABEL "ScrollGauche"
210 CLS :CURSOR 0:SCROLL 0
220 DIM ciel$(10,40)
230 ciel$(1)="                                        "
240 ciel$(2)="        .------.                        "
250 ciel$(3)="      -(        )-        .--.          "
260 ciel$(4)="    -(            )-    -(    )-        "
270 ciel$(5)="   (________________)  -(      )-       "
280 ciel$(6)="                       (__________)     "
290 ciel$(7)="                                        "
300 ciel$(8)="          .--.                          "
310 ciel$(9)="        -(    )-    .------.            "
320 ciel$(10)="       (______)   -(        )-          "
330 n=0:fin=0:w=40:sl$=DEL CHAR:sl$=sl$+"\x08"
340 for i=1 to 10:? ciel$(i);:next
350 WHILE NOT fin
360 n=n+1:AT 1,1
370 FOR r=1 TO 10
380 DEL CHAR;"\x0a\x08";ciel$(r,(n-1)%w+1)
390 NEXT
400 PAUSE 100
410 IF INKEY$ <>"" THEN LET fin=1
420 WEND
430 INS CHAR 1
440 FOR c=1 to 40
450 AT 1,1
460 FOR l=1 TO 10:PRINT " \r\n";:NEXT
470 PAUSE 100:NEXT
480 INS CHAR 0
490 CURSOR 1

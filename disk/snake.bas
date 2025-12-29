10 FAST
50 CURSOR 0;CLS;ORIGIN 2,5;AT 3,0;PAPER 1;REP$ 32," "
51 AT 3,12;"ZXSnake"
52 AT 5,11;"a - Up"
53 AT 6,11;"q - Down"
54 AT 7,11;"o - Left"
55 AT 8,11;"p - Right"
56 AT 10,3;"You have to pick up every"
57 AT 11,3;"fruit on the screen while"
58 AT 12,3;"growing up more and more!"
59 AT 14,4; FLASH 1; "Press any key to start";FLASH 0
60 j$=INKEY$
63 PAUSE 50
65 IF j$="" THEN 60

REM "UDG Definition des caractAeres si minitel 2 / Magic Club"
71 s$=""
72 f$="@"
73 ss$="+"

REM "Variable declaration (maybe declare as char array)"
76 DIM p(23,34)
77 DIM x(23,34)
78 DIM y(23,34)

REM "Variable definition"
110 headx=11
120 heady=5
130 tailx=5
140 taily=5
150 orientationx=1
160 orientationy=0
REM "Clear arrays p, x and y"
170 FOR c=1 TO 23
175 FOR f=1 TO 34
180 p(c,f)=0
181 x(c,f)=0
182 y(c,f)=0
190 NEXT f
191 NEXT c
200 score=0
210 eaten=0
220 maxx=33
230 maxy=22
240 minx=0
250 miny=0

REM "Screen Initialization"
1020 AT -1,-1;PAPER 1;REP$ 34," "
1024 FOR l=0 TO 20
1025 AT l,-1;INK 1;"\x7f";REP$ 32," ";"\x7f"
1026 NEXT l
1027 AT 21,-1;PAPER 1;"  SCORE : 0";REP$ 23," "

1030 FOR c=minx TO maxx
1040 p(miny+1,c+1)=4
1050 p(maxy+1,c+1)=4
1060 NEXT c
1070 FOR f=miny TO maxy
1080 p(f+1,minx+1)=4
1090 p(f+1,maxx+1)=4
1100 NEXT f
1500 GOSUB 8000

REM "Draw snake in its initial position"
REM "Draw the body"
2010 FOR c=tailx TO headx-1
2020 PRINT AT taily,c;ss$
2025 p(taily+2,c+2)=3
2026 x(taily+2,c+2)=1
2027 y(taily+2,c+2)=0
2030 NEXT c
REM "Draw the Head"
2045 s$="\x19\x2e"
2050 AT heady,headx;s$
2055 p(heady+2,headx+2)=2
2056 x(heady+2,headx+2)=1
2057 y(heady+2,headx+2)=0

REM "Update snake position"
REM "Change the orientation if needed"
3015 x(heady+2,headx+2)=orientationx
3020 y(heady+2,headx+2)=orientationy
REM "Erase previous head"
3030 AT heady,headx;ss$
3035 p(heady+2,headx+2)=3
3040 headx=headx+orientationx
3045 heady=heady+orientationy
3050 IF p(heady+2,headx+2)>1 THEN 9900
3051 IF p(heady+2,headx+2)<>1 THEN 3060
3052 score=score+10
3053 AT 21,8;PAPER 1;" ";score
3054 eaten=1
3055 GOSUB 8000
REM "Print the new head"
3060 AT heady,headx;s$
3065 p(heady+2,headx+2)=2
3070 IF eaten=0 THEN GOSUB 8100
3080 eaten=0

REM "Read the keyboard"
3210 a$=INKEY$
3300 IF a$<>"o" THEN 3400
3310 orientationx=-1
3320 orientationy=0
3330 s$="\x19\x2c"
3399 GOTO 7000
3400 IF a$<>"p" THEN 3500
3410 orientationx=1
3420 orientationy=0
3430 s$="\x19\x2e"
3499 GOTO 7000
3500 IF a$<>"a" THEN 3600
3510 orientationx=0
3520 orientationy=-1
3530 s$="\x19\x2d"
3599 GOTO 7000
3600 IF a$<>"q" THEN 3700
3610 orientationx=0
3620 orientationy=1
3630 s$="\x19\x2f"
3699 GOTO 7000
REM "No key"
7000 PAUSE 150
7998 GOTO 3000

REM "Fruit placement"
8010 fruitx=INT(RND*30)+1
8020 fruity=INT(RND*20)+1
8030 IF p(fruity+2,fruitx+2)=0 THEN 8050
8040 GOTO 8010
8050 AT fruity,fruitx;INK 2;f$
8060 p(fruity+2,fruitx+2)=1
8070 RETURN

REM "Erase tail"
8110 AT taily,tailx;" "
8120 newtailx=tailx+x(taily+2,tailx+2)
8130 newtaily=taily+y(taily+2,tailx+2)
8140 p(taily+2,tailx+2)=0
8150 x(taily+2,tailx+2)=0
8160 y(taily+2,tailx+2)=0
8170 tailx=newtailx
8180 taily=newtaily
8190 RETURN

REM "Game over"
9910 AT 10,11;INK 2;SIZE 1;" GAME OVER ";SIZE 0
9930 AT 12,8;FLASH 1;" Press space key ";FLASH 0
9940 j$=INKEY$
9945 PAUSE 50
9950 IF j$=" " THEN 50
9960 GOTO 9940

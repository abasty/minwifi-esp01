REM "Bootstrap"
100 FAST
102 CURSOR 0
105 SCROLL 1
110 GOSUB 10000
115 DIM hof$(9,16)
116 GOSUB 6200
120 rc$=GET(199,"h1")(TO 5)
125 IF rc$="" THEN LET rc$="00000"
130 vn$="H"+(INVERSE 1)+"O"+(INVERSE 0)+"H"
140 vi$="/T\\"

REM "Menu principal"
150 PRINT menu$;
160 c$=INKEY$
162 PAUSE 50
164 IF c$="" THEN 160
165 IF c$(1)>="a" AND c$(1)<="z" THEN LET c$(1)=CHR$(CODE c$(1)-32)
170 IF c$="J" THEN GOSUB 300
172 IF c$="R" THEN GOSUB 400
174 IF c$="C" THEN GOSUB 600
176 IF c$="Q" THEN GOSUB 700
210 GOTO 150

REM "Jeu"
300 CURSOR 0
305 PRINT jeu$;
310 GOTO 900

REM "Hall of Fame"
400 PRINT records$;
405 GOSUB 6000
410 IF INKEY$ ="" THEN 410
420 RETURN

REM "Credits"
600 PRINT credits$;
610 IF INKEY$ ="" THEN 610
620 RETURN
700 CLS
710 END

REM "Jeu init"
REM "sc$: score, sx: X vaisseau, b: Compteur 1/10s"
REM "scr$: Screen / collision, top: n° 1ère ligne"
REM "col: à 1 si collision, l24: ligne 24 dans scr$"
REM "v: Vies"
900 sc$="00000"
902 sx=20
904 l24=23
906 DIM scr$(24,40)
908 DIM sm$(4,4)
910 FOR i=1 TO 24
912 scr$(i)=""
914 NEXT i
916 b=0
918 top=0
920 col=0
922 v=2
924 AT 0,28;"V:";REP$ 3,"°"
926 v$=vn$
928 immu=0
930 fin=0

REM "Jeu boucle"
REM "---- Scroll down et ajuste top, l24 et scr$"
1000 CURSOR 0
1005 SCROLL DOWN
1010 l24=(l24+23)%24
1020 top=(top+23)%24
REM "---- Test de collision"
1030 IF immu>0 THEN LET immu=immu-1
1040 IF immu>0 THEN 1110
1050 col=scr$(l24+1,sx,sx+2)<>"   "
1060 IF col=0 THEN 1110
REM "---- COLLISION"
1070 GOSUB 4200
1080 col=0
1090 IF fin THEN RETURN
REM "---- Affiche vaisseau"
1110 GOSUB 2500
REM "---- Star field"
1120 GOSUB 2000
REM "---- Nouveau meteor"
1130 GOSUB 5000
REM "---- Score"
1140 GOSUB 4000
1150 AT 0,7;sc$
1160 IF sc$>rc$ THEN LET rc$=sc$
1170 AT 0,21;rc$;"\n"
1180 k$=INKEY$
1190 IF k$="a" AND sx>1 THEN LET sx=sx-1
1200 IF k$="e" AND sx<37 THEN LET sx=sx+1
REM "---- Efface tir si t=1"
1210 IF t=0 THEN 1240
1220 GOSUB 3500
1230 GOTO 1260
REM "---- Tir si espace"
1240 IF immu=0 AND k$=" " THEN GOSUB 3000
REM "---- Fin jeu si x"
1250 IF k$="x" THEN RETURN
1260 PAUSE 150
1270 GOTO 1000

REM "Nouvelle etoile"
2000 r=INT(RND*100)
2010 e$="."
2020 IF r>=35 THEN LET e$="+"²
2030 IF r>=55 THEN LET e$="*"
2040 IF r>=65 THEN LET e$="o"
2050 IF r>=75 THEN LET e$="'"
2060 ec=INT(RND*40)+1
2070 AT 1,ec;e$
2080 RETURN

REM "Affichage vaisseau"
2500 IF immu>0 THEN LET v$=vi$
2510 IF immu=0 THEN LET v$=vn$
2520 AT 24,sx;v$
2530 RETURN

REM "Tir"
REM "Chercher la ligne / case d'impact en remontant"
REM "li: ligne impact"
3000 sxl=sx+1
3010 imp=0
3020 FOR i=1 TO 23
3030 li=(top+i-1)%24+1
3040 IF scr$(li,sxl)<>" " THEN LET imp=i
3050 NEXT i
3060 lf=1
3070 IF imp<>0 THEN LET lf=imp
3080 FOR l=23 TO lf STEP -2
3090 AT l,sxl;"|"
3100 NEXT l
3110 t=1
3200 IF imp=0 THEN RETURN
REM "Score+100"
3210 i=4
3220 GOSUB 4030
REM "Effacer ligne d'impact et au dessus (si pas 0)"
3230 AT imp,sxl-1;". +"
3240 li=(top+imp-1)%24+1
3250 scr$(li,sxl-1,sxl+1)="   "
3260 IF imp<=1 THEN RETURN
3270 AT imp-1,sxl-1;" . "
3280 li=(li-1+23)%24+1
3290 scr$(li,sxl-1,sxl+1)="   "
3300 RETURN

REM "Tir efface"
3500 lt=1
3520 FOR l=22 TO lf+1 STEP -2
3530 AT l,sxl;" "
3540 NEXT l
3560 t=0
3570 RETURN

REM "Calcule nouveau score"
4000 b=(b+1)%10
4010 IF b<>0 THEN RETURN
4020 i=5
4030 IF sc$(i)<"9" THEN 4070
4040 sc$(i)="0"
4050 i=i-1
4060 GOTO 4030
4070 sc$(i)=CHR$((CODE sc$(i))+1)
4080 RETURN

REM "Explosion vaisseau"
4200 scr$(l24+1,sx,sx+2)="   "
4210 p=150
4220 AT 24,sx-1;"-***-"
4230 PAUSE p
4240 AT 24,sx-1;" -*- "
4250 PAUSE p
4260 AT 24,sx;" - "
4270 PAUSE p
4280 AT 24,sx+1;"."
4290 PAUSE p
4300 AT 24,sx+1;" "
4310 IF v<=0 THEN 4500
4320 v=v-1
4330 immu=24
4340 AT 0,28;"V:";REP$ (v+1),"°";" \n"
4350 PAUSE 2000
4360 RETURN

REM "Game over"
4500 fin=1
4505 AT 0,28;"V:---\n"
4510 AT 13,12;INK 2;SIZE 1;"    GAME OVER    ";SIZE 0;
4520 AT 14,12;REP$ 17," ";
4530 AT 15,12;FLASH 1;     "  Press 'x' key ";FLASH 0;
4540 j$=INKEY$
4550 PAUSE 50
4560 IF j$<>"x" THEN 4540
4600 GOSUB 6600
4605 IF pos=0 THEN 400
4610 GOSUB 6800
4620 GOSUB 6400
4630 GOTO 400

REM "Affiche meteor"
REM "sm$: Sprite meteor, cml: Current meteor line"
REM "lm: largeur meteor, hm: hauteur meteor"
REM "cm: colonne meteor"
5000 IF cml<>0 THEN 5030
REM "---- cml==0: Calcule un nouveau meteor"
5010 GOSUB 5500
REM "---- cml==0: Colonne nouveau meteor"
5020 cm=INT(RND*(41-lm))+1
REM "---- Affiche la ligne courante du meteor"
5030 AT 1,cm;sm$(hm-cml)
REM "---- MAJ scr$ pour test collision"
5040 scr$(top+1)=""
5050 scr$(top+1,cm TO cm+lm-1)="XXXXX"
REM "---- Ligne suivante dans le sprite meteor"
5060 cml=(cml+1)%hm
5070 RETURN

REM "Calcule nouveau meteor"
5500 sm$(1)=G1+"xt"
5510 sm$(2)=G1+"+'"
5530 hm=2
5540 lm=2
5550 RETURN

REM "Hall of fame"
6000 DIM p(2)
6010 p(1)=4
6020 p(2)=1
6070 FOR z=1 TO 9
6100 l$=hof$(z)
6110 t=INDEX l$,":",1
6120 IF t>0 THEN LET l$(t)=" "
6130 AT z*2+3,1;SIZE 1;PAPER p(z%2+1);" ";z;CLEOL;" ";l$
6140 NEXT z
6150 RETURN

REM "Charge HOF dans hof$"
6200 FOR i=1 TO 9
6210 hof$(i)=GET(199,"h"+CHR$(48+i))
6220 IF hof$(i,1)=" " THEN LET hof$(i)="000000:---"
6230 j=INDEX hof$(i),":",1
6240 IF j<=0 THEN LET hof$(i)="000000:---"
6250 NEXT i
6260 RETURN

REM "Sauvegarde hof$"
6400 FOR i=1 TO 9
6410 PUT 199,"h"+CHR$(48+i),hof$(i)
6420 NEXT i
6430 RETURN

REM "Insère score courant (sc$+'0') dans hof$ si > à un des 9 premiers"
6600 hs$=sc$+"0"
6610 pos=0
6620 FOR i=1 TO 9
6630 IF pos=0 AND hs$>hof$(i,1 TO 6) THEN LET pos=i
6640 NEXT i

6650 IF pos=0 THEN RETURN
6670 FOR j=9 TO pos+1 STEP -1
6680 hof$(j, 1 TO 16)=hof$(j-1, 1 TO 16)
6690 NEXT j
6700 hof$(pos)=hs$+":???"
6710 RETURN

REM "Demande nom pour HOF"
6800 AT 16,12;"Nom (max 9 car.): ";CURSOR 1
6810 INPUT "",nm$
6820 IF LEN nm$>9 THEN LET nm$=nm$(1 TO 9)
6830 hof$(pos)=hof$(pos,1 TO 7)+nm$
6840 RETURN

6999 RETURN

REM "Debug STOP"
9000 AT 0,37;"STOP\n"
9010 IF INKEY$="" THEN 9010
9020 RETURN

REM "Définition statique des écrans"
REM "----------------------------------------------"
10000 OUTPUT bottom$
10010 AT 24,10;REP$ 10,"<";REP$ 10,">"

REM "----------------------------------------------"
10030 OUTPUT menu$
10040 LINE0;CLEOL;CLS;"\n";PAPER 1;" ";CLEOL;bottom$
10050 AT 2,13;SIZE 1;"<<< METEOR >>>";SIZE 0
10060 AT 6,3;"Déplacez-vous et évitez les météores"
10070 AT 7,3;"Tirez dessus pour marquer des points"
10080 AT 9,10;INVERSE 1;"[a]";INVERSE 0;"=Gauche    ";INVERSE 1;"[e]";INVERSE 0;"=Droite"
10090 AT 10,9;INVERSE 1;"[espace]";INVERSE 0;"=Tir    ";INVERSE 1;"[x]";INVERSE 0;"=Fin"
10100 AT 13,16;INVERSE 1;SIZE 2;"J";SIZE 0;INVERSE 0;"ouer"
10110 AT 14,16;INVERSE 1;SIZE 2;"R";SIZE 0;INVERSE 0;"ecords"
10120 AT 15,16;INVERSE 1;SIZE 2;"C";SIZE 0;INVERSE 0;"rédits"
10130 AT 16,16;INVERSE 1;SIZE 2;"Q";SIZE 0;INVERSE 0;"uitter"
10135 AT 18,16;"Choix: .\x08"
10140 CURSOR 1

REM "----------------------------------------------"
11000 OUTPUT jeu$
11010 AT 0,1;CLEOL;"SCORE:000000 RECORD:000000";CLS

REM "----------------------------------------------"
11020 OUTPUT records$
11030 LINE0;CLEOL;CURSOR 0
11040 CLS;"\n";PAPER 1;" ";CLEOL;bottom$
11050 AT 2,13;SIZE 1;"<<< RECORDS >>>"

REM "----------------------------------------------"
11070 OUTPUT credits$
11130 LINE0;CLEOL;CURSOR 0
11140 CLS;"\n";PAPER 1;" ";CLEOL;bottom$
11150 AT 2,13;SIZE 1;"<<< CREDITS >>>"
11160 AT 5,20-6;SIZE 3;INVERSE 1;"BASTOS";SIZE 0;INVERSE 0
11170 AT 7,15;"Inspiré par"
11180 AT 8,9;"Sinclair ZX81 / Spectrum"
11190 AT 9,14;"Oric 1 / Atmos"
11200 AT 10,15;"Amstrad CPC"
11220 AT 14,20-6;SIZE 3;FLASH 1;"METEOR";SIZE 0;FLASH 0
11230 AT 16,2;"Inspiré par";UNDERLINE 1;" METEOR";UNDERLINE 0;" : Premier jeu en"
11240 AT 17,1;"assembleur Z80 sur ZX81. Sur Minitel,"
11250 AT 18,1;"pas de mémoire écran, juste une prise"
11260 AT 19,8;"4800(M1B)/9600(M2) bps."

REM "----------------------------------------------"
11270 OUTPUT STOP
11280 RETURN

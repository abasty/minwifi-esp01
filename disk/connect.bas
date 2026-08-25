1000 MODE 1
1010 GOSUB "ChoixWifi"
1020 LABEL "ChoixService":b=0:nc=0:suiv=0
1030 LABEL "AfficheChoix":GOSUB "ListeServices"
1040 LABEL "SaisieChoix":CURSOR 0;AT 23,1;INK 2;"quitter → ";INK 6;INVERSE 1;SIZE 2;"Q"
1050 AT 24,1;"connect → ";SIZE 2;"#"
1060 LABEL "LitTouche":k$=INKEY$:IF k$="" THEN PAUSE 50:GOTO "LitTouche"
1070 IF k$="Q" OR k$="q" THEN "Fin"
1080 IF k$="\x06" THEN "ChoixService"
1090 IF k$="\x05" THEN "Retour"
1100 IF k$<>"\x04" THEN "TraiteChoix"
1110 IF suiv=0 THEN "SaisieChoix"
1120 b=b+5
1130 GOTO "AfficheChoix"
1140 LABEL "Retour":IF b=0 THEN "SaisieChoix"
1150 b=b-5
1160 GOTO "AfficheChoix"
1170 LABEL "TraiteChoix":c=CODE k$-48
1180 IF c<=0 OR c>nc THEN "SaisieChoix"
1190 GOSUB 2000+(b+c-1)*10
1200 IF urn$="" THEN "SaisieChoix"
1210 PRINT AT 0,1;"Connecting to ";srv$
1220 MODE smode:MINITEL srv$,urn$:MODE 1
1230 GOTO "AfficheChoix"
1240 LABEL "ListeServices":CURSOR 0;LINE0;CLEOL;CLS;SCROLL 0;"LISTE DES SERVICES\r\n";INK 4;REP$ 40,"`"
1250 FOR i=1 TO 4:AT i*4+2,5;INK 4;REP$ 36,"`";"\n":NEXT
1260 PRINT "\n\n";REP$ 40,"`";
1270 nc=0
1280 FOR i=1 TO 5
1290 GOSUB 2000+(b+i-1)*10
1300 IF srv$="" THEN 1350
1310 AT i*4-1,3;i;" ";srv$(1 TO 36)
1320 AT i*4,5;INK 6;desc$(1 TO 36)
1330 AT i*4+1,5;INK 2;urn$(1 TO 36)
1340 nc=nc+1
1350 NEXT
1360 GOSUB 2000+(b+5)*10
1370 suiv=0
1380 IF srv$="" THEN 1410
1390 suiv=1
1400 AT 23,17;INK 2;"page suivante →";UNDERLINE 1;" ";INK 6;INVERSE 1;" SUITE  "
1410 AT 24,17;INK 2;"première page → ";INK 6;INVERSE 1;"SOMMAIRE"
1420 RETURN
1999 REM "Services"
2000 srv$="minipavi"
2001 urn$="tcp:go.minipavi.fr:516"
2002 desc$="Kiosque Minipavi"
2003 smode=0
2009 RETURN
2010 srv$="3611"
2011 urn$="ws:3611.re:80:/ws"
2012 desc$="Annuaire électronique"
2013 smode=0
2019 RETURN
2020 srv$="3615"
2021 urn$="ws:3615co.de:80:/ws"
2022 desc$="Kiosque 3615"
2023 smode=0
2029 RETURN
2030 srv$="hacker"
2031 urn$="ws:mntl.joher.com:2018:/?echo"
2032 desc$="Pour geeks rétro, site original"
2033 smode=0
2039 RETURN
2040 srv$="retrocampus"
2041 urn$="tcp:bbs.retrocampus.com:1651"
2042 desc$="Passerelle BBS"
2043 smode=0
2049 RETURN
2050 srv$="zboub"
2051 urn$="tcp:abasty-retro.fr:1967"
2052 desc$="Portage serveur monovoie 90's"
2053 smode=0
2059 RETURN
2060 srv$="telehack"
2061 urn$="tcp:telehack.com:23"
2062 desc$="Serveur Telnet"
2063 smode=2
2069 RETURN
2070 srv$="galaxy"
2071 urn$="ws:galaxy.microtel.fr:50124"
2072 desc$="Serveur des années 90"
2073 smode=0
2079 RETURN
2990 srv$=""
2991 urn$=""
2992 desc$=""
2993 smode=0
2998 RETURN
3000 LABEL "ChoixWifi":OUTPUT ws$
3010 WIFI STATUS
3020 OUTPUT STOP
3030 ws=INDEX ws$,"Not connected"
3040 IF ws=0 THEN RETURN
3050 LABEL "ScanWifi":CLS:WIFI SCAN
3060 INPUT "N° du réseau Wi-Fi : .\b",wn
3070 IF wn<1 OR wn>10 THEN "ScanWifi"
3080 IF ssid$(wn,1)=" " THEN "ScanWifi"
3090 WIFI wn
3100 GOTO "ChoixWifi"
3110 LABEL "Fin":MODE 1:CLS:END

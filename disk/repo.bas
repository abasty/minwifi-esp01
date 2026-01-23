10 REM "Choix Wi-Fi"
20 GOSUB 3000

99 REM "Choix service"
100 b=0
101 nc=0
102 suiv=0
105 GOSUB 1000
110 AT 24,1;"Choix : .\x08";CURSOR 1
120 INPUT c$
130 k=VKEY
140 IF k=6 THEN 100
150 IF k<>4 THEN 200
160 IF suiv=0 THEN 110
170 b=b+5
180 GOTO 105
200 c=CODE c$-48
220 IF c<=0 OR c>nc THEN 110
230 GOSUB 2000+(b+c-1)*10
240 IF urn$="" THEN 110
250 PRINT AT 0,1;"Connecting to ";srv$
260 MINITEL urn$
300 GOTO 100

999 REM "Affiche page services"
1000 CURSOR 0;LINE0 ;CLEOL ;CLS ;SCROLL 0;"LISTE DES SERVICES\r\n";INK 4;REP$ 40,"`"
1010 FOR i=1 TO 4
1020 AT i*4+2,5;INK 4;REP$ 36,"`";"\n"
1040 NEXT i
1050 PRINT "\n\n";REP$ 40,"`";
1100 nc=0
1110 FOR i=1 TO 5
1120 GOSUB 2000+(b+i-1)*10
1125 IF srv$="" THEN 1160
1130 AT i*4-1,3;i;" ";srv$(1 TO 36)
1140 AT i*4,5;INK 6;desc$(1 TO 36)
1150 AT i*4+1,5;INK 2;urn$(1 TO 36)
1155 nc=nc+1
1160 NEXT i
1165 GOSUB 2000+(b+5)*10
1166 suiv=0
1170 IF srv$="" THEN 1190
1175 suiv=1
1180 AT 23,17;INK 2;"page suivante \x19\x2e";UNDERLINE 1; " ";INK 6;INVERSE 1;" SUITE  "
1190 AT 24,17;INK 2;"première page \x19\x2e ";INK 6;INVERSE 1;"SOMMAIRE"
1200 RETURN

1999 REM "Services"
2000 srv$="minipavi"
2001 urn$="tcp:go.minipavi.fr:516"
2002 desc$="Kiosque Minipavi"
2009 RETURN
2010 srv$="3611"
2011 urn$="ws:3611.re:80:/ws"
2012 desc$="Annuaire électronique"
2019 RETURN
2020 srv$="3615"
2021 urn$="ws:3615co.de:80:/ws"
2022 desc$="Kiosque 3615"
2029 RETURN
2030 srv$="hacker"
2031 urn$="ws:mntl.joher.com:2018:/?echo"
2032 desc$="Pour geeks rétro, site original"
2039 RETURN
2040 srv$="retrocampus"
2041 urn$="tcp:bbs.retrocampus.com:1651"
2042 desc$="Passerelle BBS"
2049 RETURN
2050 srv$="zboub"
2051 urn$="tcp:abasty-retro.fr:1967"
2052 desc$="Portage serveur monovoie 90's"
2059 RETURN
2060 srv$="telehack"
2061 urn$="tcp:telehack.com:23"
2062 desc$="Serveur Telnet"
2069 RETURN
2990 srv$=""
2991 urn$=""
2992 desc$=""
2998 RETURN

2999 REM "Choix Wi-Fi"
3000 OUTPUT ws$
3010 WIFI STATUS
3015 OUTPUT STOP
3020 ws=INDEX ws$,"Not connected"
3030 IF ws=0 THEN RETURN
3040 CLS
3050 WIFI SCAN
3060 INPUT "N° du réseau Wi-Fi : .\x08",wn
3070 IF wn<1 OR wn>10 THEN 3040
3080 IF ssid$(wn, 1)=" " THEN 3040
3090 WIFI wn
3095 GOTO 3000

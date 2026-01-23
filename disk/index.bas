10 a$=GET 255
100 deb=1
110 fin=INDEX a$,"\r\n",deb
120 IF fin<=0 THEN 200
130 s$=a$(deb,fin-1)
135 PRINT "> ";s$
140 deb=fin+2
150 GOTO 110

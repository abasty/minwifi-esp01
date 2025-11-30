10 INPUT "Pr\x19Benom: ",a$
20 INPUT "Nom   : ",b$
30 ia$=CHR$(CODE a$&223)
40 ib$=CHR$(CODE b$&223)
50 a$(1)=ia$
60 b$(1)=ib$
100 PRINT "Nom complet: ";a$;" ";b$
120 PRINT "Initiales  : ";ia$;ib$

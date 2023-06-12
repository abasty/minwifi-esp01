#!/bin/bash

netcat esp-minitel.local 23 <<EOF
10 INPUT "Prénom: ",a\$
20 INPUT "Nom: ",b\$
30 LET ia\$=CHR\$(CODE(a\$)|32-32)
40 LET ib\$=CHR\$(CODE(b\$)|32-32)
50 LET a\$=ia\$+a\$(2 TO )
60 LET b\$=ib\$+b\$(2 TO )
100 PRINT "Nom complet: ";a\$,b\$
120 PRINT "Initiales: ";ia\$;ib\$
EOF

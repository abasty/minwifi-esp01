#!/bin/bash

netcat esp-minitel.local 23 <<EOF
10 let a=10
20 let b=20
50 print a+b,a-b,a*b,a/b,a%b
100 let pi4=pi/4
110 let sqr2=sqr 2
150 print "sin(pi/4)=";sin pi4, "sqr(2)/2=";sqr2/2
EOF

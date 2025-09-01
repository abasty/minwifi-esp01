output menu$
fast
line0;cleol
cls;"\n";paper 1;" ";cleol
size 1;"*** METEOR ***"
?at 4,1;"1.";underline 1;" Jouer";underline 0;" "
?"2. Records"
?"3. Configuration"
?"4. Credits"
?"5. Quitter"
?

output credits$
line0;cleol
cls;"\n";paper 1;" ";cleol
at 2,13;size 1;"*** CREDITS ***"
?at 5,1;size 3;inverse 1;"BASTOS"
?at 7,1;"Inspire par :"
?
?"* Sinclair ZX81 / Spectrum"
?"* Oric 1 / Atmos"
?"* Amstrad CPC"
?
?at 14,1;size 3;flash 1;"METEOR";
?at 16,1;"Inspire par";underline 1;" METEOR";underline 0;" : Premier jeu en"
?"assembleur Z80 sur ZX81. Sur Minitel,"
rem "pas de memoire ecran, juste une ligne"



output stop
save"meteor.vdt.var"
run"meteor"

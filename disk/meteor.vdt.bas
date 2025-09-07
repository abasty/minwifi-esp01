rem "----------------------------------------------"
output bottom$
at 24,10;rep$ 10,"<";rep$ 10,">"
rem "----------------------------------------------"
output menu$
fast
line0;cleol
cls;"\n";paper 1;" ";cleol;bottom$
at 2,13;size 1;"<<< METEOR >>>"
?at 4,1;"1.";underline 1;" Jouer";underline 0;" "
?"2. Records"
?"3. Configuration"
?"4. Crédits"
?"5. Quitter"
?
rem "----------------------------------------------"
output jeu$
at 0,5;cleol;"SCORE: 000000   RECORD: 000000";cls
at 20,1;paper 1;" ";cleol
at 20,37;size 1;"JEU"
at 23,1;"<Appuie sur une touche>"
rem "----------------------------------------------"
output records$
line0;cleol;
cls;"\n";paper 1;" ";cleol;bottom$
at 2,13;size 1;"<<< RECORDS >>>"
at 4,1;"<Appuie sur une touche>"
rem "----------------------------------------------"
output config$
line0;cleol
cls;"\n";paper 1;" ";cleol;bottom$
at 2,10;size 1;"<<< CONFIGURATION >>>"
at 4,1;"<Appuie sur une touche>"
rem "----------------------------------------------"
output credits$
line0;cleol
cls;"\n";paper 1;" ";cleol;bottom$
at 2,13;size 1;"<<< CREDITS >>>"
?at 5,20-6;size 3;inverse 1;"BASTOS"
?at 7,2;"Inspiré par :"
?" - Sinclair ZX81 / Spectrum"
?" - Oric 1 / Atmos"
?" - Amstrad CPC"
?
?at 14,20-6;size 3;flash 1;"METEOR";
?at 16,1;" Inspiré par";underline 1;" METEOR";underline 0;" : Premier jeu en"
?" assembleur Z80 sur ZX81. Sur Minitel,"
?" pas de mémoire écran, juste une prise"
?" 4800(M1B)/9600(M2) bps."
rem "----------------------------------------------"
output stop
save"meteor.vdt.var"
run"meteor"

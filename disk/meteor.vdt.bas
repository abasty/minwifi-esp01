output menu$
fast
line0;cleol
cls;"\n";paper 1;" ";cleol
at 2,13;size 1;"*** METEOR ***"
?at 4,1;"1.";underline 1;" Jouer";underline 0;" "
?"2. Records"
?"3. Configuration"
?"4. Credits"
?"5. Quitter"
?

output jeu$
at 0,5;cleol;"SCORE: 000000   RECORD: 000000";cls
at 20,1;paper 1;" ";cleol
at 20,37;size 1;"JEU"
at 23,1;"<Appuie sur une touche>"

output records$
line0;cleol
cls;"\n";paper 1;" ";cleol
at 2,13;size 1;"*** RECORDS ***"
at 4,1;"<Appuie sur une touche>"

output config$
line0;cleol
cls;"\n";paper 1;" ";cleol
at 2,10;size 1;"*** CONFIGURATION ***"
at 4,1;"<Appuie sur une touche>"

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
?"pas de memoire ecran, juste une prise"
?"4800 (1B) / 9600 (2) bps."

output stop
save"meteor.vdt.var"
run"meteor"

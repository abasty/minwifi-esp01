#ifndef TTY_VT100_H
#define TTY_VT100_H

#define CODE_SEQUENCE_MAX_SIZE 32

#define DEL "\x08 \x08"

#define CLS "\x1B[2J\x1B[H"
#define CLEOL "\x1B[K"

#define CON "\x1B[?25h"
#define COFF "\x1B[?25l"

#define G0 ""
#define G1 ""
#define G2 ""
#define SS2 (0x19)

#define CUR "\x1B[%d;%dH"
#define CUR_DELTA_V 0
#define CUR_DELTA_H 0
#define LINE0 ""

#define INV "\x1B[7m"
#define NORMAL "\x1B[m"
#define BLINK "\x1B[5m"

#define INK "\x1B[%dm"
#define INK1 "\x1B[\x1Fm"
#define INK7 "\x1B[\x25m"
#define INK_DELTA 30

#define PAPER "\x1B[%dm"
#define PAPER_DELTA 40

#define P_ROULEAU_ON ""
#define P_ROULEAU_OFF ""
#define P_CLAVIER_MINUSCULE ""
#define P_CLAVIER_ETENDU ""

#define P_ACK_OFF_PRISE ""
#define P_LOCAL_ECHO_OFF ""

#define P_40_COLS "\x1B[?3l"
#define P_80_COLS "\x1B[?3h"

// Mode 80 colonnes - codes VT100 standards
#define MODE80_CLS "\x1B[2J\x1B[H"
#define MODE80_CLEOL "\x1B[K"
#define MODE80_CON "\x1B[?25h"
#define MODE80_COFF "\x1B[?25l"
#define MODE80_CUR "\x1B[%d;%dH"
#define MODE80_CUR_DELTA_V 0
#define MODE80_CUR_DELTA_H 0
#define MODE80_CUD1 "\x1B[1B"

#define MODE80_BOLD "\x1B[1m"
#define MODE80_BOLD_OFF "\x1B[22m"
#define MODE80_INV "\x1B[7m"
#define MODE80_INV_OFF "\x1B[27m"
#define MODE80_NORMAL "\x1B[m"
#define MODE80_BLINK "\x1B[5m"
#define MODE80_BLINK_OFF "\x1B[25m"
#define MODE80_UNDERLINE "\x1B[4m"
#define MODE80_UNDERLINE_OFF "\x1B[24m"
#define MODE80_ROULEAU_ON "\x1B[?4l"
#define MODE80_ROULEAU_OFF "\x1B[<4h"

#endif // TTY_VT100_H

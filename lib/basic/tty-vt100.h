#ifndef TTY_VT100_H
#define TTY_VT100_H

#define CODE_SEQUENCE_MAX_SIZE 32

#define CLS "\x1B[2J\x1B[H"

#define CON "\x1B[?25h"
#define COFF "\x1B[?25l"

#define CUR "\x1B[%d;%dH"
#define CUR_DELTA_V 0
#define CUR_DELTA_H 0

#define INK "\x1B[%dm"
#define INK_DELTA 30

#define PAPER "\x1B[%dm"
#define PAPER_DELTA 40

#endif // TTY_VT100_H

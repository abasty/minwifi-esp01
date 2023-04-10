#ifndef __BASIC_H__
#define __BASIC_H__

#define KEYWORD_END_TAG (0b10000000)
#define TOKEN_KEYWORD ((uint8_t) 0b10000000)
#define TOKEN_INTEGER ((uint8_t) 0b00000001)
#define TOKEN_STRING  ((uint8_t) 0b00000010)

typedef struct {
    char *read_ptr;
    char *write_ptr;
} t_line;

int tokenize(t_line *line);

#endif // __BASIC_H__

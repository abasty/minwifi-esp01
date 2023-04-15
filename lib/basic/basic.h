#ifndef __BASIC_H__
#define __BASIC_H__

#include <stdint.h>

#define TOKEN_KEYWORD           ((uint8_t) 0b10000000)
#define KEYWORD_END_TAG         TOKEN_KEYWORD

#define TOKEN_INTEGER_TYPE_MASK ((uint8_t) 0b11000000)
#define TOKEN_INTEGER_BITS_MASK ((uint8_t) 0b01110000)
#define TOKEN_INTEGER           ((uint8_t) 0b01000000)
#define TOKEN_INTEGER_4         ((uint8_t) 0b01000000)
#define TOKEN_INTEGER_8         ((uint8_t) 0b01100000)
#define TOKEN_INTEGER_16        ((uint8_t) 0b01010000)

#define TOKEN_STRING            ((uint8_t) 0b00100000)


typedef struct {
    uint8_t *start;
    uint8_t *read_ptr;
    uint8_t *write_ptr;
} t_tokenizer_state;

int tokenize(t_tokenizer_state *state, char *line);
uint8_t token_get_next(t_tokenizer_state *state);
uint16_t token_integer_get_value(t_tokenizer_state *state);
char* token_string_get_value(t_tokenizer_state *state);

#endif // __BASIC_H__

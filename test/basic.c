#include <stdint.h>
#include <stdio.h>

const uint8_t KEYWORD_MAX_LENGHT = 16;

#define KEYWORD_END_TAG (0b10000000)

#include "keywords.c"

uint8_t keyword_get_id(const char *keyword_char, char *word)
{
    unsigned char searched_word[KEYWORD_MAX_LENGHT + 1];

    char *current_searched_char = searched_word; // Should be done in place (word) until separator (all char < ' ')
    int n = KEYWORD_MAX_LENGHT;
    while (*word != 0 && n > 0)
    {
        *current_searched_char++ = *word >= 'a' && *word <= 'z' ? *word - 32 : *word;
        word++;
        n--;
    }
    *current_searched_char-- = 0;
    *current_searched_char |= KEYWORD_END_TAG;

    if (n == 0 && *word != 0) {
        return -1;
    }

    int index = 0;
    while (*keyword_char != 0)
    {
        current_searched_char = searched_word;
        while (*current_searched_char == *keyword_char)
        {
            if ((*current_searched_char & KEYWORD_END_TAG) != 0)
            {
                return index;
            }
            current_searched_char++;
            keyword_char++;
        }
        index++;
        while (*keyword_char && (*keyword_char & KEYWORD_END_TAG) == 0)
        {
            keyword_char++;
        }
        if (*keyword_char)
        {
            keyword_char++;
        }
    }
    return -1;
}

// TODO: pase a string, split to spaces, and give pointer to words, then
// tokenerize : interger, string, keyword
// Ou plutôt un truc qui fait "nextToken, nextTokenAsString"

void keyword_print_all(const char *keyword_char)
{
    while (*keyword_char)
    {
        putchar(*keyword_char & ~KEYWORD_END_TAG);
        if ((*keyword_char & KEYWORD_END_TAG) != 0)
        {
            putchar('\n');
        }
        keyword_char++;
    }
}


int main()
{
    keyword_print_all(keywords);
    printf("cLeAr: %d\n", keyword_get_id(keywords, "cLeAr"));
    printf("catS: %d\n", keyword_get_id(keywords, "catS"));
    printf("free: %d\n", keyword_get_id(keywords, "free"));
    printf("FREEz: %d\n", keyword_get_id(keywords, "FREEz"));
}

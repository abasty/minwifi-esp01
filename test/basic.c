#include <stdint.h>
#include <stdio.h>

const uint8_t KEYWORD_MAX_LENGHT = 16;
const char *keywords = "FREeCATsCLEAr";

int getKeywordIndex(const char *keywords, char *word)
{
    char searchedWord[KEYWORD_MAX_LENGHT];

    char *current = searchedWord;
    while (*word)
    {
        *current++ = *word >= 'a' && *word <= 'z' ? *word - 32 : *word;
        word++;
    }
    *current-- = 0;
    *current += 32;

    int index = 0;
    while (*keywords != 0)
    {
        current = searchedWord;
        while (*current == *keywords)
        {
            if (*current >= 'a' && *current <= 'z')
            {
                return index;
            }
            current++;
            keywords++;
        }
        index++;
        while (*keywords && (*keywords <= 'a' || *keywords >= 'z'))
        {
            keywords++;
        }
        if (*keywords)
        {
            keywords++;
        }
    }
    return -1;
}

int main()
{
    printf("cLeAr: %d\n", getKeywordIndex(keywords, "cLeAr"));
    printf("catS: %d\n", getKeywordIndex(keywords, "catS"));
    printf("free: %d\n", getKeywordIndex(keywords, "free"));
    printf("FREEz: %d\n", getKeywordIndex(keywords, "FREEz"));
}

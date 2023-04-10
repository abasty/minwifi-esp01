#include <stdint.h>
#include <stdio.h>

const uint8_t KEYWORD_MAX_LENGHT = 16;
const char *keywords = "FREeCATsCLEArRESEtCONFIgCONNECt";

const int KEYWORD_FREE = 0;
const int KEYWORD_CATS = 1;
const int KEYWORD_CLEAR = 2;
const int KEYWORD_RESET = 3;
const int KEYWORD_CONFIG = 4;
const int KEYWORD_CONNECT = 5;

int getKeywordIndex(const char *keywords, char *word)
{
    char searchedWord[KEYWORD_MAX_LENGHT + 1];

    char *current = searchedWord;
    int n = KEYWORD_MAX_LENGHT;
    while (*word && n > 0)
    {
        *current++ = *word >= 'a' && *word <= 'z' ? *word - 32 : *word;
        word++;
        n--;
    }
    *current-- = 0;
    *current += 32;

    if (n == 0 && *word) {
        return -1;
    }

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

// TODO: pase a string, split to spaces, and give pointer to words, then
// tokenerize : interger, string, keyword

int main()
{
    printf("cLeAr: %d\n", getKeywordIndex(keywords, "cLeAr"));
    printf("catS: %d\n", getKeywordIndex(keywords, "catS"));
    printf("free: %d\n", getKeywordIndex(keywords, "free"));
    printf("FREEz: %d\n", getKeywordIndex(keywords, "FREEz"));
}

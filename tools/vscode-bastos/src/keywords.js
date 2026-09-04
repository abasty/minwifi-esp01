// Master keyword list for BASTOS, decoded from lib/basic/keywords.c-static
// (kept in sync by hand — see that file and keywords.h for the source of
// truth). Includes every reserved word the real tokenizer recognizes:
// control-flow, commands, functions, and word operators.
const KEYWORDS = [
  "ABS", "ACS", "ASN", "ATN", "LABEL", "CHR$", "CODE", "COS", "EXP", "INT",
  "LN", "PI", "RND", "SGN", "SIN", "SQR", "TAN", "CLEAR", "NEW", "CLS",
  "CAT", "ERASE", "RESET", "LOAD", "SAVE", "LIST", "LET", "PRINT", "INPUT",
  "INKEY$", "RUN", "TO", "STR$", "AT", "INK", "PAPER", "AND", "OR", "NOT",
  "IF", "GOTO", "GOSUB", "RETURN", "FOR", "NEXT", "STEP", "STOP", "CONT",
  "THEN", "LEN", "REM", "CURSOR", "FAST", "SLOW", "DIM", "FREE", "RAND",
  "PAUSE", "TAB", "SCROLL", "PLOT", "UNPLOT", "TEST", "MODE", "WIFI",
  "START", "PEEK", "POKE", "USR", "VAL", "BASTOS", "SCAN", "STATUS", "FTP",
  "MINITEL", "GET", "PUT", "DB", "DOWN", "UP", "DEBUG", "OUTPUT", "BEEP",
  "CLEOL", "LINE0", "FLASH", "INVERSE", "UNDERLINE", "REP$", "ECHO",
  "SIZE", "END", "EDIT", "DEL", "INS", "LINE", "CHAR", "FAST2", "LL", "G0",
  "G1", "ORIGIN", "VKEY", "INDEX", "FILE", "ELSE", "WHILE", "WEND", "MD",
  "CD", "RD", "MOVE",
];

const KEYWORD_SET = new Set(KEYWORDS);

module.exports = { KEYWORDS, KEYWORD_SET };

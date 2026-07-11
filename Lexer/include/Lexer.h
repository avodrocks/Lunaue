#ifndef LEXER_H
#define LEXER_H

// Includes

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Local Includes

#include "../../Token/include/Token.h"

// Constants

extern const char* ReservedKeywords[];
extern const char AlphanumericCharacters[];
extern const char AlphabeticalCharacters[];
extern const char Whitespaces[];
extern const char DigitCharacters[];
extern const char HexadecimalCharacters[];
extern const char BinaryCharacters[];
extern const char ValidEscapeSequences[];

// Structs

typedef struct {
	size_t SizeTokenList;
	size_t CapTokenList;
	TokenStruct* TokenList;
	size_t Pos;
	uint32_t Line;
	uint32_t LinePos;
	size_t SizeSource;
	char* Source;
} LexerStruct;

// Functions

LexerStruct* NewLexer(size_t SizeSource, char* Source);
void FreeTokenList(LexerStruct* self);
int LexCurrent(LexerStruct* self);
char PeekLexer(LexerStruct* self, int n);
char AdvanceLexer(LexerStruct* self);
int AppendToken(LexerStruct* self, TokenStruct* tk);
bool IsAlpha(char C);
bool IsAlNum(char C);
bool IsWhitespace(char C);
bool IsDigit(char C);
bool IsBinary(char C);
bool IsHex(char C);
bool IsValidEscapeSequence(char C);
bool IsReserved(char* Keyword);

#endif

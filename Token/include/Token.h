#ifndef TOKEN_H
#define TOKEN_H

// Includes

#include <stddef.h>

// Constants

extern const char* TokenTypeIds[];

// Token Types

typedef enum {
	TK_EOF,
	TK_STRING,
	TK_NUMBER,
	TK_SYMBOL,
	TK_RESERVED,
	TK_IDENTIFIER,
} TokenType;

// Token Struct

typedef struct {
	char* Source;
	size_t SizeSource;
	TokenType TokenType;
} TokenStruct;

// Functions

TokenStruct* NewTokenStruct(char* Source, TokenType Type);
void PrintTokenStruct(TokenStruct* self);
int FreeTokenStruct(TokenStruct* self);

#endif

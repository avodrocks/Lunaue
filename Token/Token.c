// Library Includes

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>

// Local Includes

#include "include/Token.h"

// Constants

const char* TokenTypeIds[] = {
	"TK_EOF",
	"TK_STRING",
	"TK_NUMBER",
	"TK_SYMBOL",
	"TK_RESERVED",
	"TK_IDENTIFIER",
};

// Implementations

TokenStruct* NewTokenStruct(char* Source, TokenType Type) {
	TokenStruct* NewToken = malloc(sizeof(TokenStruct));

	if (NewToken == NULL) {
		printf("[Lunaue]: Failed to allocate memory for token.");
		free(NewToken);

		NewToken = NULL;

		return NULL;
	};

	NewToken->TokenType = Type;
	NewToken->SizeSource = strlen(Source) + 1;
	NewToken->Source = Source;

	return NewToken;
}

void PrintTokenStruct(TokenStruct* self) {
	const char* TokenTypeId = TokenTypeIds[self->TokenType];
	const char* Source = self->Source;

	freopen(NULL, "w", stdout);
	printf("[Lunaue]: %s:::%s\n", TokenTypeId, Source);
}

int FreeTokenStruct(TokenStruct* self) {
	free(self->Source);

	self->Source = NULL;

	free(self);

	self = NULL;

	return 0;
}

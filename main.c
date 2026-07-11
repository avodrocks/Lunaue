#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

#include "Lexer/include/Lexer.h"
#include "Token/include/Token.h"

int main() {
	setlocale(LC_ALL, "");

	LexerStruct* L = NewLexer(50, "[[hiii]][==[ [[hiii]==] ] == ]");
	int Success = LexCurrent(L);

	if (Success == -1) {
		printf("[Lunaue]: An error occured! Aborting program...\n");
		abort(); // fuh no
	};

	for (size_t i = 0; i < L->SizeTokenList; i++) {
		PrintTokenStruct(&L->TokenList[i]);
	};

	printf("[Lunaue]: Finished Running\n");

	return 0;
}

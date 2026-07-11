// Includes

#include <ctype.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Local Includes

#include "./include/Lexer.h"
#include "../Token/include/Token.h"

// Constants

const char* ReservedKeywords[] = {
	"if", "else", "elseif", "then",
	"for", "in", "while", "do",
	"repeat", "until", "break", "continue",
	"end", "function", "local", "return",
	"true", "false", "nil", "not",
	"and", "or", "goto", "self",
	"type", "typeof", "export",
};
const char AlphanumericCharacters[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
	'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
	'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
	'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
	's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	'0', '1', '2', '3', '4',
	'5', '6', '7', '8', '9',
};
const char AlphabeticalCharacters[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
	'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
	'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
	'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
	's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
};
const char Whitespaces[] = {
	' ', '\t', '\n', '\r',
};
const char DigitCharacters[] = {
	'0', '1', '2', '3', '4',
	'5', '6', '7', '8', '9',
};
const char HexadecimalCharacters[] = {
	'A', 'B', 'C', 'D', 'E', 'F',
	'a', 'b', 'c', 'd', 'e', 'f',
	'0', '1', '2', '3', '4',
	'5', '6', '7', '8', '9',
};
const char BinaryCharacters[] = {
	'0', '1',
};
const char ValidEscapeSequences[] = {
	'n', 'r', 't', 'v',
	'b', 'a', '\\', '"',
	'\'',

	'\n', '\r', '\t', '\v',
	'\b', '\a',
};

// Local Helpers

static int EnsureCapacity(char** Buffer, size_t Index, size_t* Cap) {
	if (Index >= *Cap) {
		size_t NewCap = (*Cap) * 2;

		if (NewCap <= Index) {
			NewCap = Index + 1;
		};

		char* NewBuffer = realloc(*Buffer, NewCap);

		if (!NewBuffer) {
			return -1;
		};

		*Buffer = NewBuffer;
		*Cap = NewCap;
	};

	return 0;
}

static uint8_t CodePointToUTF8(uint32_t cp, uint8_t *utf8_str) {
	if (cp <= 0x7F) {
		utf8_str[0] = (uint8_t)cp;

		return 1;
	} else if (cp <= 0x7FF) {
		utf8_str[0] = (uint8_t)((cp >> 6) | 0xC0);
		utf8_str[1] = (uint8_t)((cp & 0x3F) | 0x80);

		return 2;
	} else if (cp <= 0xFFFF) {
		utf8_str[0] = (uint8_t)((cp >> 12) | 0xE0);
		utf8_str[1] = (uint8_t)(((cp >> 6) & 0x3F) | 0x80);
		utf8_str[2] = (uint8_t)((cp & 0x3F) | 0x80);

		return 3;
	} else if (cp <= 0x10FFFF) {
		utf8_str[0] = (uint8_t)((cp >> 18) | 0xF0);
		utf8_str[1] = (uint8_t)(((cp >> 12) & 0x3F) | 0x80);
		utf8_str[2] = (uint8_t)(((cp >> 6) & 0x3F) | 0x80);
		utf8_str[3] = (uint8_t)((cp & 0x3F) | 0x80);

		return 4;
	}

	return 0;
}

static char* DupSymbol(const char* Symbol) {
	size_t Len = strlen(Symbol);
	char* Dup = malloc(Len + 1);

	if (!Dup) {
		return NULL;
	};

	memcpy(Dup, Symbol, Len + 1);

	return Dup;
}

// Implementations

LexerStruct* NewLexer(size_t SizeSource, char* Source) {
	LexerStruct* NewLexerStruct = malloc(sizeof(LexerStruct));

	if (NewLexerStruct == NULL) {
		printf("[Lunaue]: Failed to allocate memory for lexer struct.");
		free(NewLexerStruct);

		return NULL;
	}

	NewLexerStruct->CapTokenList = 2;
	NewLexerStruct->SizeTokenList = 0;
	NewLexerStruct->TokenList = malloc(sizeof(TokenStruct) * NewLexerStruct->CapTokenList);

	if (NewLexerStruct->TokenList == NULL) {
		printf("[Lunaue]: Failed to allocate memory for lexer token struct list");
		free(NewLexerStruct->TokenList);

		NewLexerStruct->TokenList = NULL;

		free(NewLexerStruct);

		NewLexerStruct = NULL;

		return NULL;
	}

	NewLexerStruct->SizeSource = SizeSource;
	NewLexerStruct->Source = Source;
	NewLexerStruct->Pos = 0;
	NewLexerStruct->Line = 1;
	NewLexerStruct->LinePos = 0;

	return NewLexerStruct;
}

void FreeTokenList(LexerStruct* self) {
	for (size_t i = 0; i < self->SizeTokenList; i++) {
		FreeTokenStruct(&self->TokenList[i]);
	};

	free(self->TokenList);

	self->TokenList = NULL;
	self->SizeTokenList = 0;
	self->CapTokenList = 0;
}

int LexCurrent(LexerStruct* self) {
	while (1) {
		char Current = PeekLexer(self, 0);

		if (IsAlpha(Current) || Current == '_') {
			size_t CapKeyword = 2;
			size_t SizeKeyword = 0;
			char* Keyword = malloc(sizeof(char) * CapKeyword);

			if (!Keyword) {
				printf("[Lunaue]: Failed to allocate string\n");
				free(Keyword);

				Keyword = NULL;

				return -1;
			};

			Keyword[SizeKeyword] = AdvanceLexer(self);
			SizeKeyword++;

			while (IsAlNum(PeekLexer(self, 0)) || PeekLexer(self, 0) == '_') {
				if (EnsureCapacity(&Keyword, SizeKeyword, &CapKeyword) != 0) {
					printf("[Lunaue]: Failed to reallocate string\n");
					free(Keyword);

					Keyword = NULL;

					return -1;
				};

				Keyword[SizeKeyword] = AdvanceLexer(self);
				SizeKeyword++;
			};

			if (EnsureCapacity(&Keyword, SizeKeyword, &CapKeyword) != 0) {
				printf("[Lunaue]: Failed to reallocate string\n");
				free(Keyword);

				Keyword = NULL;

				return -1;
			};

			Keyword[SizeKeyword] = '\0';

			if (IsReserved(Keyword)) {
				TokenStruct* Tk = NewTokenStruct(Keyword, TK_RESERVED);

				if (!Tk) {
					printf("[Lunaue]: Failed to make new token, returned NULL\n");

					return -1;
				};

				AppendToken(self, Tk);

				continue;
			};

			TokenStruct* Tk = NewTokenStruct(Keyword, TK_IDENTIFIER);

			if (!Tk) {
				printf("[Lunaue]: Failed to make new token, returned NULL\n");

				return -1;
			};

			AppendToken(self, Tk);

			continue;
		};

		if (IsWhitespace(Current)) {
			AdvanceLexer(self);

			continue;
		};

		if (IsDigit(Current)) {
			size_t CapNum = 2;
			size_t SizeNum = 0;
			char* Number = malloc(sizeof(char) * CapNum);

			if (!Number) {
				printf("[Lunaue]: Failed to allocate string\n");
				free(Number);

				Number = NULL;

				return -1;
			};

			Number[SizeNum] = AdvanceLexer(self);
			SizeNum++;

			if (Number[0] == '0') {
				if (tolower(PeekLexer(self, 0)) == 'x') {
					if (EnsureCapacity(&Number, SizeNum, &CapNum) != 0) {
						printf("[Lunaue]: Failed to reallocate string\n");
						free(Number);

						Number = NULL;

						return -1;
					};

					Number[SizeNum] = AdvanceLexer(self);
					SizeNum++;

					while (IsHex(PeekLexer(self, 0))) {
						if (EnsureCapacity(&Number, SizeNum, &CapNum) != 0) {
							printf("[Lunaue]: Failed to reallocate string\n");
							free(Number);

							Number = NULL;

							return -1;
						};

						Number[SizeNum] = AdvanceLexer(self);
						SizeNum++;
					};

					if (PeekLexer(self, 0) == '.') {
						AdvanceLexer(self);

						if (EnsureCapacity(&Number, SizeNum, &CapNum) != 0) {
							printf("[Lunaue]: Failed to reallocate string\n");
							free(Number);

							Number = NULL;

							return -1;
						};

						Number[SizeNum] = '.';
						SizeNum++;

						while (IsHex(PeekLexer(self, 0))) {
							if (EnsureCapacity(&Number, SizeNum, &CapNum) != 0) {
								printf("[Lunaue]: Failed to reallocate string\n");
								free(Number);

								Number = NULL;

								return -1;
							};

							Number[SizeNum] = AdvanceLexer(self);
							SizeNum++;
						};
					};

					if (EnsureCapacity(&Number, SizeNum, &CapNum) != 0) {
						printf("[Lunaue]: Failed to reallocate string\n");
						free(Number);

						Number = NULL;

						return -1;
					};

					Number[SizeNum] = '\0';

					TokenStruct* Tk = NewTokenStruct(Number, TK_NUMBER);

					if (!Tk) {
						printf("[Lunaue]: Failed to make new token, returned NULL\n");

						return -1;
					};

					AppendToken(self, Tk);

					continue;
				};

				if (tolower(PeekLexer(self, 0)) == 'b') {
					if (EnsureCapacity(&Number, SizeNum, &CapNum) != 0) {
						printf("[Lunaue]: Failed to reallocate string\n");
						free(Number);

						Number = NULL;

						return -1;
					};

					Number[SizeNum] = AdvanceLexer(self);
					SizeNum++;

					while (IsBinary(PeekLexer(self, 0))) {
						if (EnsureCapacity(&Number, SizeNum, &CapNum) != 0) {
							printf("[Lunaue]: Failed to reallocate string\n");
							free(Number);

							Number = NULL;

							return -1;
						};

						Number[SizeNum] = AdvanceLexer(self);
						SizeNum++;
					};

					if (EnsureCapacity(&Number, SizeNum, &CapNum) != 0) {
						printf("[Lunaue]: Failed to reallocate string\n");
						free(Number);

						Number = NULL;

						return -1;
					};

					Number[SizeNum] = '\0';

					TokenStruct* Tk = NewTokenStruct(Number, TK_NUMBER);

					if (!Tk) {
						printf("[Lunaue]: Failed to make new token, returned NULL\n");

						return -1;
					};

					AppendToken(self, Tk);

					continue;
				};
			};

			while (IsDigit(PeekLexer(self, 0))) {
				if (EnsureCapacity(&Number, SizeNum, &CapNum) != 0) {
					printf("[Lunaue]: Failed to reallocate string\n");
					free(Number);

					Number = NULL;

					return -1;
				};

				Number[SizeNum] = AdvanceLexer(self);
				SizeNum++;
			};

			if (PeekLexer(self, 0) == '.') {
				AdvanceLexer(self);

				if (EnsureCapacity(&Number, SizeNum, &CapNum) != 0) {
					printf("[Lunaue]: Failed to reallocate string\n");
					free(Number);

					Number = NULL;

					return -1;
				};

				Number[SizeNum] = '.';
				SizeNum++;

				while (IsDigit(PeekLexer(self, 0))) {
					if (EnsureCapacity(&Number, SizeNum, &CapNum) != 0) {
						printf("[Lunaue]: Failed to reallocate string\n");
						free(Number);

						Number = NULL;

						return -1;
					};

					Number[SizeNum] = AdvanceLexer(self);
					SizeNum++;
				};
			};

			if (tolower(PeekLexer(self, 0)) == 'e') {
				char ExponentDecl = AdvanceLexer(self);
				bool IsNegative = false;

				if (PeekLexer(self, 0) == '-') {
					AdvanceLexer(self);

					IsNegative = true;
				};

				if (EnsureCapacity(&Number, SizeNum, &CapNum) != 0) {
					printf("[Lunaue]: Failed to reallocate string\n");
					free(Number);

					Number = NULL;

					return -1;
				};

				Number[SizeNum] = ExponentDecl;
				SizeNum++;

				if (IsNegative) {
					if (EnsureCapacity(&Number, SizeNum, &CapNum) != 0) {
						printf("[Lunaue]: Failed to reallocate string\n");
						free(Number);

						Number = NULL;

						return -1;
					};

					Number[SizeNum] = '-';
					SizeNum++;
				};

				while (IsDigit(PeekLexer(self, 0))) {
					if (EnsureCapacity(&Number, SizeNum, &CapNum) != 0) {
						printf("[Lunaue]: Failed to reallocate string\n");
						free(Number);

						Number = NULL;

						return -1;
					};

					Number[SizeNum] = AdvanceLexer(self);
					SizeNum++;
				};
			};

			if (EnsureCapacity(&Number, SizeNum, &CapNum) != 0) {
				printf("[Lunaue]: Failed to reallocate string\n");
				free(Number);

				Number = NULL;

				return -1;
			};

			Number[SizeNum] = '\0';

			TokenStruct* Tk = NewTokenStruct(Number, TK_NUMBER);

			if (!Tk) {
				printf("[Lunaue]: Failed to make new token, returned NULL\n");

				return -1;
			};

			AppendToken(self, Tk);

			continue;
		};

		if (Current == '\"' || Current == '\'' || Current == '`') {
			char Opener = AdvanceLexer(self);
			size_t CapStringContents = 8;
			size_t SizeStringContents = 0;
			char* StringContents = malloc(sizeof(char) * CapStringContents);

			if (!StringContents) {
				printf("[Lunaue]: Failed to allocate memory for string token.\n");

				return -1;
			};

			while ((PeekLexer(self, 0) != Opener) && (PeekLexer(self, 0) != '\n') && (PeekLexer(self, 0) != '\0')) {
				if (EnsureCapacity(&StringContents, SizeStringContents, &CapStringContents) != 0) {
					printf("[Lunaue]: Failed to reallocate memory for string token.\n");
					free(StringContents);

					StringContents = NULL;

					return -1;
				};

				char C = AdvanceLexer(self);

				if (C == '\\') {
					if (IsValidEscapeSequence(PeekLexer(self, 0))) {
						char EscSeq = AdvanceLexer(self);

						switch (EscSeq) {
							case 'n':
							case '\n':
								C = '\n';

								break;
							case 'r':
							case '\r':
								C = '\r';

								break;
							case 't':
							case '\t':
								C = '\t';

								break;
							case 'v':
							case '\v':
								C = '\v';

								break;
							case 'b':
							case '\b':
								C = '\b';

								break;
							case 'a':
							case '\a':
								C = '\a';

								break;
							case '\\':
								C = '\\';

								break;
							case '"':
								C = '"';

								break;
							case '\'':
								C = '\'';

								break;
							default:
								printf("[Lunaue]: WARNING - Weird unhandled escape sequence: %c\n", EscSeq);

								break;
						};
					} else if (IsDigit(PeekLexer(self, 0))) {
						char* Seq = malloc(4);
						uint8_t Size = 0;

						if (!Seq) {
							printf("[Lunaue]: Failed to allocate memory for ASCII escape sequence\n");
							free(Seq);

							Seq = NULL;

							free(StringContents);

							return -1;
						};

						Seq[Size] = AdvanceLexer(self);
						Size++;

						if (IsDigit(PeekLexer(self, 0))) {
							Seq[Size] = AdvanceLexer(self);
							Size++;

							if (IsDigit(PeekLexer(self, 0))) {
								Seq[Size] = AdvanceLexer(self);
								Size++;
							};
						};

						Seq[Size] = '\0';

						int Value = atoi(Seq);

						if (Value > 255) {
							printf("[Lunaue]: WARNING - decimal escape sequence \\%s out of range, truncating\n", Seq);

							Value = 255;
						};

						C = (char)Value;

						free(Seq);
						Seq = NULL;
					} else if (tolower(PeekLexer(self, 0)) == 'x') {
						AdvanceLexer(self);

						char* Seq = malloc(3);
						uint8_t Size = 0;

						if (!Seq) {
							printf("[Lunaue]: Failed to allocate memory for hex escape sequence\n");
							free(Seq);

							Seq = NULL;

							free(StringContents);

							return -1;
						};

						if (IsHex(PeekLexer(self, 0))) {
							Seq[Size] = AdvanceLexer(self);
							Size++;

							if (IsHex(PeekLexer(self, 0))) {
								Seq[Size] = AdvanceLexer(self);
								Size++;
							};
						};

						Seq[Size] = '\0';
						C = (char)strtol(Seq, NULL, 16);

						free(Seq);
						Seq = NULL;
					} else if (tolower(PeekLexer(self, 0)) == 'u') {
						AdvanceLexer(self);

						if (PeekLexer(self, 0) != '{') {
							printf("[Lunaue]: Unicode escape sequence init failure\n");

							free(StringContents);

							return -1;
						};

						AdvanceLexer(self);

						char* Seq = malloc(8);
						uint8_t Size = 0;

						if (!Seq) {
							printf("[Lunaue]: Failed to allocate memory for unicode escape sequence\n");
							free(Seq);

							Seq = NULL;

							free(StringContents);

							return -1;
						};

						while (Size < 7 && IsHex(PeekLexer(self, 0)) && PeekLexer(self, 0) != '}') {
							Seq[Size] = AdvanceLexer(self);
							Size++;
						}

						if (PeekLexer(self, 0) != '}') {
							printf("[Lunaue]: Unclosed unicode escape sequence\n");

							free(Seq);
							free(StringContents);

							return -1;
						};

						Seq[Size] = '\0';

						AdvanceLexer(self);

						uint8_t Buffer[4];
						uint8_t EncodedLen = CodePointToUTF8((uint32_t)strtoul(Seq, NULL, 16), Buffer);

						free(Seq);
						Seq = NULL;

						if (EncodedLen == 0) {
							printf("[Lunaue]: Invalid unicode code point in escape sequence\n");

							free(StringContents);

							return -1;
						};

						if (EnsureCapacity(&StringContents, SizeStringContents + EncodedLen - 1, &CapStringContents) != 0) {
							printf("[Lunaue]: Failed to reallocate memory for string token.\n");
							free(StringContents);

							StringContents = NULL;

							return -1;
						};

						for (int i = 0; i < EncodedLen; i++) {
							StringContents[SizeStringContents] = Buffer[i];
							SizeStringContents++;
						};

						continue;
					};
				};

				StringContents[SizeStringContents] = C;
				SizeStringContents++;
			};

			if (PeekLexer(self, 0) != Opener) {
				printf("[Lunaue]: Failed to lex, expected string to be closed.\n");

				free(StringContents);

				return -1;
			};

			AdvanceLexer(self);

			if (EnsureCapacity(&StringContents, SizeStringContents, &CapStringContents) != 0) {
				printf("[Lunaue]: Failed to reallocate memory for string token.\n");
				free(StringContents);

				StringContents = NULL;

				return -1;
			};

			StringContents[SizeStringContents] = '\0';

			TokenStruct* Tk = NewTokenStruct(StringContents, TK_STRING);

			if (!Tk) {
				printf("[Lunaue]: Failed to append token, token is NULL!\n");

				return -1;
			};

			AppendToken(self, Tk);

			continue;
		};

		if (Current == '[') {
			AdvanceLexer(self);

			size_t EqCount = 0;

			while (PeekLexer(self, 0) == '=') {
				AdvanceLexer(self);

				EqCount++;
			};

			if (PeekLexer(self, 0) != '[' && EqCount == 0) {
				TokenStruct* Tk = NewTokenStruct("[", TK_SYMBOL);

				if (!Tk) {
					printf("[Lunaue]: Failed to make token for symbol '['\n");

					return -1;
				};

				AppendToken(self, Tk);

				continue;
			};

			if (PeekLexer(self, 0) != '[' && EqCount > 0) {
				printf("[Lunaue]: Expected '[' to open long data\n");

				return -1;
			};

			if (PeekLexer(self, 0) == '[') {
				AdvanceLexer(self);

				size_t SizeStringData = 0;
				size_t CapStringData = 4;
				char* StringData = malloc(CapStringData);

				if (!StringData) {
					printf("[Lunaue]: Failed to allocate memory long data data\n");
					free(StringData);

					StringData = NULL;

					return -1;
				};

				while (true) {
					char Next = AdvanceLexer(self);

					if (Next == '\0') {
						printf("[Lunaue]: Unterminated long string\n");

						free(StringData);

						StringData = NULL;

						return -1;
					};

					if (Next == ']') {
						if (EqCount == 0 && PeekLexer(self, 0) == ']') {
							AdvanceLexer(self);

							break;
						} else {
							size_t MatchedEq = 0;

							while (MatchedEq < EqCount && PeekLexer(self, 0) == '=') {
								AdvanceLexer(self);

								MatchedEq++;
							};

							if (MatchedEq == EqCount && PeekLexer(self, 0) == ']') {
								AdvanceLexer(self);

								break;
							} else {
								if (EnsureCapacity(&StringData, SizeStringData, &CapStringData) != 0) {
									printf("[Lunaue]: Failed to reallocate long data data\n");
									free(StringData);

									StringData = NULL;

									return -1;
								};

								StringData[SizeStringData] = Next;
								SizeStringData++;

								for (size_t i = 0; i < MatchedEq; i++) {
									if (EnsureCapacity(&StringData, SizeStringData, &CapStringData) != 0) {
										printf("[Lunaue]: Failed to reallocate long data data\n");
										free(StringData);

										StringData = NULL;

										return -1;
									};

									StringData[SizeStringData] = '=';
									SizeStringData++;
								};
							};
						};
					} else {
						if (EnsureCapacity(&StringData, SizeStringData, &CapStringData) != 0) {
							printf("[Lunaue]: Failed to reallocate long data data\n");
							free(StringData);

							StringData = NULL;

							return -1;
						};

						StringData[SizeStringData] = Next;
						SizeStringData++;
					};
				};

				if (EnsureCapacity(&StringData, SizeStringData, &CapStringData) != 0) {
					printf("[Lunaue]: Failed to reallocate long data data\n");
					free(StringData);

					StringData = NULL;

					return -1;
				};

				StringData[SizeStringData] = '\0';

				TokenStruct* Tk = NewTokenStruct(StringData, TK_STRING);

				if (!Tk) {
					printf("[Lunaue]: Failed to allocate memory for long data token\n");

					return -1;
				};

				AppendToken(self, Tk);

				continue;
			};

			continue;
		};

		if (Current == '-') {
			char Symbol = AdvanceLexer(self);

			if (PeekLexer(self, 0) == '=') {
				char SymbolTotal[3] = {Symbol, AdvanceLexer(self), '\0'};
				char* SymbolText = DupSymbol(SymbolTotal);

				if (!SymbolText) {
					printf("[Lunaue]: Failed to allocate memory for symbol '-='\n");

					return -1;
				};

				TokenStruct* Tk = NewTokenStruct(SymbolText, TK_SYMBOL);

				if (!Tk) {
					printf("[Lunaue]: Failed to allocate token for symbol '-='\n");

					return -1;
				};

				AppendToken(self, Tk);

				continue;
			};

			if (PeekLexer(self, 0) == '-') {
				AdvanceLexer(self);

				if (PeekLexer(self, 0) == '[') {
					AdvanceLexer(self);

					// Must be && here - with || this was always true
					// (a char can't fail both comparisons at once),
					// so long comments were never detected and always
					// fell through as line comments.
					if (PeekLexer(self, 0) != '[' && PeekLexer(self, 0) != '=') {
						while (PeekLexer(self, 0) != '\0' && PeekLexer(self, 0) != '\n') {
							AdvanceLexer(self);
						};

						continue;
					};

					size_t EqCount = 0;

					while (PeekLexer(self, 0) == '=') {
						AdvanceLexer(self);

						EqCount++;
					};

					if (PeekLexer(self, 0) != '[' && EqCount == 0) {
						while (PeekLexer(self, 0) != '\0' && PeekLexer(self, 0) != '\n') {
							AdvanceLexer(self);
						};

						continue;
					};

					if (PeekLexer(self, 0) != '[' && EqCount > 0) {
						while (PeekLexer(self, 0) != '\0' && PeekLexer(self, 0) != '\n') {
							AdvanceLexer(self);
						};

						continue;
					};

					if (PeekLexer(self, 0) == '[') {
						AdvanceLexer(self);

						while (true) {
							char Next = AdvanceLexer(self);

							if (Next == '\0') {
								printf("[Lunaue]: Unterminated long comment\n");

								return -1;
							};

							if (Next == ']') {
								if (EqCount == 0 && PeekLexer(self, 0) == ']') {
									AdvanceLexer(self);

									break;
								} else {
									size_t MatchedEq = 0;

									while (MatchedEq < EqCount && PeekLexer(self, 0) == '=') {
										AdvanceLexer(self);

										MatchedEq++;
									};

									if (MatchedEq == EqCount && PeekLexer(self, 0) == ']') {
										AdvanceLexer(self);

										break;
									};
								};
							};
						};

						continue;
					};

					continue;
				} else {
					while (PeekLexer(self, 0) != '\0' && PeekLexer(self, 0) != '\n') {
						AdvanceLexer(self);
					};

					continue;
				};
			};

			char SymbolTotal[2] = {Symbol, '\0'};
			char* SymbolText = DupSymbol(SymbolTotal);

			if (!SymbolText) {
				printf("[Lunaue]: Failed to allocate memory for symbol '-'\n");

				return -1;
			};

			TokenStruct* Tk = NewTokenStruct(SymbolText, TK_SYMBOL);

			if (!Tk) {
				printf("[Lunaue]: Failed to allocate token for symbol '-'\n");

				return -1;
			};

			AppendToken(self, Tk);

			continue;
		};

		if (Current == '+') {
			char Symbol = AdvanceLexer(self);

			if (PeekLexer(self, 0) == '=') {
				char NSymbol[3] = {Symbol, AdvanceLexer(self), '\0'};
				char* SymbolText = DupSymbol(NSymbol);

				if (!SymbolText) {
					printf("[Lunaue]: Failed to allocate memory for symbol '+='\n");

					return -1;
				};

				TokenStruct* Tk = NewTokenStruct(SymbolText, TK_SYMBOL);

				if (!Tk) {
					printf("[Lunaue]: Failed to make token for symbol '+='\n");

					return -1;
				};

				AppendToken(self, Tk);

				continue;
			};

			char NSymbol[2] = {Symbol, '\0'};
			char* SymbolText = DupSymbol(NSymbol);

			if (!SymbolText) {
				printf("[Lunaue]: Failed to allocate memory for symbol '+'\n");

				return -1;
			};

			TokenStruct* Tk = NewTokenStruct(SymbolText, TK_SYMBOL);

			if (!Tk) {
				printf("[Lunaue]: Failed to make token for symbol '+'\n");

				return -1;
			};

			AppendToken(self, Tk);

			continue;
		};

		if (Current == '*') {
			char Symbol = AdvanceLexer(self);

			if (PeekLexer(self, 0) == '=') {
				char NSymbol[3] = {Symbol, AdvanceLexer(self), '\0'};
				char* SymbolText = DupSymbol(NSymbol);

				if (!SymbolText) {
					printf("[Lunaue]: Failed to allocate memory for symbol '*='\n");

					return -1;
				};

				TokenStruct* Tk = NewTokenStruct(SymbolText, TK_SYMBOL);

				if (!Tk) {
					printf("[Lunaue]: Failed to make token for symbol '*='\n");

					return -1;
				};

				AppendToken(self, Tk);

				continue;
			};

			char NSymbol[2] = {Symbol, '\0'};
			char* SymbolText = DupSymbol(NSymbol);

			if (!SymbolText) {
				printf("[Lunaue]: Failed to allocate memory for symbol '*'\n");

				return -1;
			};

			TokenStruct* Tk = NewTokenStruct(SymbolText, TK_SYMBOL);

			if (!Tk) {
				printf("[Lunaue]: Failed to make token for symbol '*'\n");

				return -1;
			};

			AppendToken(self, Tk);

			continue;
		};

		if (Current == '%') {
			char Symbol = AdvanceLexer(self);

			if (PeekLexer(self, 0) == '=') {
				char NSymbol[3] = {Symbol, AdvanceLexer(self), '\0'};
				char* SymbolText = DupSymbol(NSymbol);

				if (!SymbolText) {
					printf("[Lunaue]: Failed to allocate memory for symbol '%%='\n");

					return -1;
				};

				TokenStruct* Tk = NewTokenStruct(SymbolText, TK_SYMBOL);

				if (!Tk) {
					printf("[Lunaue]: Failed to make token for symbol '%%='\n");

					return -1;
				};

				AppendToken(self, Tk);

				continue;
			};

			char NSymbol[2] = {Symbol, '\0'};
			char* SymbolText = DupSymbol(NSymbol);

			if (!SymbolText) {
				printf("[Lunaue]: Failed to allocate memory for symbol '%%'\n");

				return -1;
			};

			TokenStruct* Tk = NewTokenStruct(SymbolText, TK_SYMBOL);

			if (!Tk) {
				printf("[Lunaue]: Failed to make token for symbol '%%'\n");

				return -1;
			};

			AppendToken(self, Tk);

			continue;
		};

		if (Current == '^') {
			char Symbol = AdvanceLexer(self);

			if (PeekLexer(self, 0) == '=') {
				char NSymbol[3] = {Symbol, AdvanceLexer(self), '\0'};
				char* SymbolText = DupSymbol(NSymbol);

				if (!SymbolText) {
					printf("[Lunaue]: Failed to allocate memory for symbol '^='\n");

					return -1;
				};

				TokenStruct* Tk = NewTokenStruct(SymbolText, TK_SYMBOL);

				if (!Tk) {
					printf("[Lunaue]: Failed to make token for symbol '^='\n");

					return -1;
				};

				AppendToken(self, Tk);

				continue;
			};

			char NSymbol[2] = {Symbol, '\0'};
			char* SymbolText = DupSymbol(NSymbol);

			if (!SymbolText) {
				printf("[Lunaue]: Failed to allocate memory for symbol '^'\n");

				return -1;
			};

			TokenStruct* Tk = NewTokenStruct(SymbolText, TK_SYMBOL);

			if (!Tk) {
				printf("[Lunaue]: Failed to make token for symbol '^'\n");

				return -1;
			};

			AppendToken(self, Tk);

			continue;
		};

		if (Current == '/') {
			char Symbol = AdvanceLexer(self);

			if (PeekLexer(self, 0) == '=' || PeekLexer(self, 0) == '/') {
				char NSymbol[3] = {Symbol, AdvanceLexer(self), '\0'};
				char* SymbolText = DupSymbol(NSymbol);

				if (!SymbolText) {
					printf("[Lunaue]: Failed to allocate memory for symbol '/=' or '//'\n");

					return -1;
				};

				TokenStruct* Tk = NewTokenStruct(SymbolText, TK_SYMBOL);

				if (!Tk) {
					printf("[Lunaue]: Failed to make token for symbol '/=' or '//'\n");

					return -1;
				};

				AppendToken(self, Tk);

				continue;
			};

			char NSymbol[2] = {Symbol, '\0'};
			char* SymbolText = DupSymbol(NSymbol);

			if (!SymbolText) {
				printf("[Lunaue]: Failed to allocate memory for symbol '/'\n");

				return -1;
			};

			TokenStruct* Tk = NewTokenStruct(SymbolText, TK_SYMBOL);

			if (!Tk) {
				printf("[Lunaue]: Failed to make token for symbol '/'\n");

				return -1;
			};

			AppendToken(self, Tk);

			continue;
		};

		if (Current == '.') {
			char Symbol = AdvanceLexer(self);

			if (PeekLexer(self, 0) == '.') {
				char Symbol2 = AdvanceLexer(self);

				if (PeekLexer(self, 0) == '=' || PeekLexer(self, 0) == '.') {
					char NSymbol[4] = {Symbol, Symbol2, AdvanceLexer(self), '\0'};
					char* SymbolText = DupSymbol(NSymbol);

					if (!SymbolText) {
						printf("[Lunaue]: Failed to allocate memory for symbol '...' or '..='\n");

						return -1;
					};

					TokenStruct* Tk = NewTokenStruct(SymbolText, TK_SYMBOL);

					if (!Tk) {
						printf("[Lunaue]: Failed to make token for symbol\n"); // LOL

						return -1;
					};

					AppendToken(self, Tk);

					continue;
				};

				char NSymbol[3] = {Symbol, Symbol2, '\0'};
				char* SymbolText = DupSymbol(NSymbol);

				if (!SymbolText) {
					printf("[Lunaue]: Failed to allocate memory for symbol '..'\n");

					return -1;
				};

				TokenStruct* Tk = NewTokenStruct(SymbolText, TK_SYMBOL);

				if (!Tk) {
					printf("[Lunaue]: Failed to make token for symbol '..'\n");

					return -1;
				};

				AppendToken(self, Tk);

				continue;
			};

			char NSymbol[2] = {Symbol, '\0'};
			char* SymbolText = DupSymbol(NSymbol);

			if (!SymbolText) {
				printf("[Lunaue]: Failed to allocate memory for symbol '.'\n");

				return -1;
			};

			TokenStruct* Tk = NewTokenStruct(SymbolText, TK_SYMBOL);

			if (!Tk) {
				printf("[Lunaue]: Failed to make token for symbol '.'\n");

				return -1;
			};

			AppendToken(self, Tk);

			continue;
		};

		if (Current == '~' && PeekLexer(self, 1) == '=') {
			char FirstChar = AdvanceLexer(self);
			char SecondChar = AdvanceLexer(self);
			char NSymbol[3] = {FirstChar, SecondChar, '\0'};
			char* SymbolText = DupSymbol(NSymbol);

			if (!SymbolText) {
				printf("[Lunaue]: Failed to allocate memory for symbol '~='\n");

				return -1;
			};

			TokenStruct* Tk = NewTokenStruct(SymbolText, TK_SYMBOL);

			if (!Tk) {
				printf("[Lunaue]: Failed to make token for symbol '~='\n");

				return -1;
			};

			AppendToken(self, Tk);

			continue;
		};

		if ((Current == '=') || (Current == '<') || (Current == '>')) {
			char Symbol = AdvanceLexer(self);

			if (PeekLexer(self, 0) == '=') {
				char NSymbol[3] = {Symbol, AdvanceLexer(self), '\0'};
				char* SymbolText = DupSymbol(NSymbol);

				if (!SymbolText) {
					printf("[Lunaue]: Failed to allocate memory for symbol\n");

					return -1;
				};

				TokenStruct* Tk = NewTokenStruct(SymbolText, TK_SYMBOL);

				if (!Tk) {
					printf("[Lunaue]: Failed to make token for symbol\n");

					return -1;
				};

				AppendToken(self, Tk);

				continue;
			};

			char NSymbol[2] = {Symbol, '\0'};
			char* SymbolText = DupSymbol(NSymbol);

			if (!SymbolText) {
				printf("[Lunaue]: Failed to allocate memory for symbol\n");

				return -1;
			};

			TokenStruct* Tk = NewTokenStruct(SymbolText, TK_SYMBOL);

			if (!Tk) {
				printf("[Lunaue]: Failed to make token for symbol\n");

				return -1;
			};

			AppendToken(self, Tk);

			continue;
		};

		if (Current == ':') {
			char Symbol = AdvanceLexer(self);

			if (PeekLexer(self, 0) == ':') {
				char NSymbol[3] = {Symbol, AdvanceLexer(self), '\0'};
				char* SymbolText = DupSymbol(NSymbol);

				if (!SymbolText) {
					printf("[Lunaue]: Failed to allocate memory for symbol '::'\n");

					return -1;
				};

				TokenStruct* Tk = NewTokenStruct(SymbolText, TK_SYMBOL);

				if (!Tk) {
					printf("[Lunaue]: Failed to make token for symbol '::'\n");

					return -1;
				};

				AppendToken(self, Tk);

				continue;
			};

			char NSymbol[2] = {Symbol, '\0'};
			char* SymbolText = DupSymbol(NSymbol);

			if (!SymbolText) {
				printf("[Lunaue]: Failed to allocate memory for symbol ':'\n");

				return -1;
			};

			TokenStruct* Tk = NewTokenStruct(SymbolText, TK_SYMBOL);

			if (!Tk) {
				printf("[Lunaue]: Failed to make token for symbol ':'\n");

				return -1;
			};

			AppendToken(self, Tk);

			continue;
		};

		if ((Current == '#') || (Current == '(') || (Current == ')') || (Current == ']') || (Current == '{') || (Current == '}') || (Current == ';') || (Current == ',')) {
			char NSymbol[2] = {AdvanceLexer(self), '\0'};
			char* SymbolText = DupSymbol(NSymbol);

			if (!SymbolText) {
				printf("[Lunaue]: Failed to allocate memory for symbol\n");

				return -1;
			};

			TokenStruct* Tk = NewTokenStruct(SymbolText, TK_SYMBOL);

			if (!Tk) {
				printf("[Lunaue]: Failed to make token for symbol\n");

				return -1;
			};

			AppendToken(self, Tk);

			continue;
		};

		if (Current == '\0') {
			printf("[Lunaue]: Finished lexing\n");

			TokenStruct* EofTk = NewTokenStruct("__EOF__", TK_EOF);

			if (!EofTk) {
				printf("[Lunaue]: Failed to make EOF token\n");

				return -1;
			};

			AppendToken(self, EofTk);

			return 0;
		};

		printf("[Lunaue]: Unsupported Character Found: %.1s\n", &Current);

		return -1;
	};
}

char PeekLexer(LexerStruct* self, int n) {
	if ((self->Pos + n) >= self->SizeSource) {
		return '\0';
	};

	return self->Source[self->Pos + n];
}

char AdvanceLexer(LexerStruct* self) {
	char CurrentChar = PeekLexer(self, 0);

	self->Pos++;
	self->LinePos++;

	if (CurrentChar == '\n') {
		self->LinePos = 0;
		self->Line++;
	};

	return CurrentChar;
}

int AppendToken(LexerStruct* self, TokenStruct* tk) {
	if (self->SizeTokenList >= self->CapTokenList) {
		int InitCap = self->CapTokenList * 2;
		TokenStruct* ReallocTokenList = realloc(self->TokenList, sizeof(TokenStruct) * InitCap);

		if (!ReallocTokenList) {
			printf("[Lunaue]: Failed to reallocate memory for lexer token list");
			free(self->TokenList);

			self->TokenList = NULL;

			free(tk);

			return 1;
		}

		self->TokenList = ReallocTokenList;
		self->CapTokenList = InitCap;
	};

	self->TokenList[self->SizeTokenList] = *tk;
	self->SizeTokenList++;

	free(tk);

	return 0;
}

bool IsAlpha(char C) {
	for (int i = 0; i < (int)(sizeof(AlphabeticalCharacters) / sizeof(AlphabeticalCharacters[0])); i++) {
		if (AlphabeticalCharacters[i] == C) {
			return true;
		};
	};

	return false;
}

bool IsAlNum(char C) {
	for (int i = 0; i < (int)(sizeof(AlphanumericCharacters) / sizeof(AlphanumericCharacters[0])); i++) {
		if (AlphanumericCharacters[i] == C) {
			return true;
		};
	};

	return false;
}

bool IsWhitespace(char C) {
	for (int i = 0; i < (int)(sizeof(Whitespaces) / sizeof(Whitespaces[0])); i++) {
		if (Whitespaces[i] == C) {
			return true;
		};
	};

	return false;
}

bool IsDigit(char C) {
	for (int i = 0; i < (int)(sizeof(DigitCharacters) / sizeof(DigitCharacters[0])); i++) {
		if (DigitCharacters[i] == C) {
			return true;
		};
	};

	return false;
}

bool IsBinary(char C) {
	for (int i = 0; i < (int)(sizeof(BinaryCharacters) / sizeof(BinaryCharacters[0])); i++) {
		if (BinaryCharacters[i] == C) {
			return true;
		};
	};

	return false;
}

bool IsHex(char C) {
	for (int i = 0; i < (int)(sizeof(HexadecimalCharacters) / sizeof(HexadecimalCharacters[0])); i++) {
		if (HexadecimalCharacters[i] == C) {
			return true;
		};
	};

	return false;
}

bool IsValidEscapeSequence(char C) {
	for (int i = 0; i < (int)(sizeof(ValidEscapeSequences) / sizeof(ValidEscapeSequences[0])); i++) {
		if (ValidEscapeSequences[i] == C) {
			return true;
		};
	};

	return false;
}

bool IsReserved(char* Keyword) {
	for (int i = 0; i < (int)(sizeof(ReservedKeywords) / sizeof(ReservedKeywords[0])); i++) {
		if (strcmp(ReservedKeywords[i], Keyword) == 0) {
			return true;
		};
	};

	return false;
}

cmake_minimum_required(VERSION 3.10)
project(MyCProject LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_executable(
	adam

	main.c
	Lexer/Lexer.c
	Token/Token.c
)

#pragma once
#include "stdafx.h"

enum chartype
{
	ct_symbol = 64,			// Any symbol > 127, a-z, A-Z, 0-9, _
	ct_start_symbol = 128	// Any symbol > 127, a-z, A-Z, _, :
};

const unsigned char chartype_table[256] =
{
	0,   0,   0,   0,   0,   0,   0,   0,      0,   0,   0,   0,   0,   0,   0,   0,   // 0-15
	0,   0,   0,   0,   0,   0,   0,   0,      0,   0,   0,   0,   0,   0,   0,   0,   // 16-31
	0,   0,   6,   0,   0,   0,   0,   0,      0,   0,   0,   0,   0,   0,   0,   0,   // 32-47
	64,  64,  64,  64,  64,  64,  64,  64,     64,  64,  0,   0,   0,   0,   0,   0,   // 48-63
	0,   192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192, // 64-79
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 0,   0,   0,   0,   192, // 80-95
	0,   192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192, // 96-111
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 0, 0, 0, 0, 0,           // 112-127

	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192, // 128+
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192
};

static inline bool isDigit(char data)
{
	return (unsigned char)(data - '0') < 10;
}

enum LexemeType
{
	lex_none,
	lex_number, lex_string, lex_quotedstring, // *(0-9) *(a-z,A-Z,_) "*any"
	lex_semiquote, lex_escape, lex_point, lex_comma, // " ' \ .
	lex_inc, lex_dec, // ++ --
	lex_add, lex_sub, lex_mul, lex_div, lex_mod, lex_pow, lex_less, lex_lequal, lex_shl, lex_greater, lex_gequal, lex_shr, lex_equal, lex_nequal, // + - * / % ** < <= << > >= >> == !=
	lex_set, lex_addset, lex_subset, lex_mulset, lex_divset, lex_powset, // = += -= *= /= **=
	lex_bitand, lex_bitor, lex_bitxor, lex_logand, lex_logor, lex_logxor, // & | ^ and or xor
	lex_bitnot, lex_lognot,	// ~ !
	lex_oparen, lex_cparen, lex_obracket, lex_cbracket, lex_ofigure, lex_cfigure, // ( ) [ ] { }
	lex_questionmark, lex_colon, lex_semicolon, // ? : ;
	lex_if, lex_else, lex_for, lex_while, lex_do, lex_switch, lex_case,	// if else for while switch case
	lex_break, lex_continue, lex_return, // break continue return
	lex_const, lex_ref, lex_auto, lex_class, lex_noalign, lex_align, lex_typeof, lex_sizeof, // const ref auto class noalign align typeof sizeof
};

static const int lexemLength[] =
{
	0,
	0, 0, 0,
	1, 1, 1, 1, // " ' \ . ,
	2, 2, // ++ --
	1, 1, 1, 1, 1, 2, 1, 2, 2, 1, 2, 2, 2, 2, // + - * / % ** < <= << > >= >> == !=
	1, 2, 2, 2, 2, 3, // = += -= *= /= **=
	1, 1, 1, 3, 2, 3, // & | ^ and or xor
	1, 1,	// ~ !
	1, 1, 1, 1, 1, 1, // ( ) [ ] { }
	1, 1, 1, // ? : ;
	2, 4, 3, 5, 2, 6, 4,	// if else for while switch case
	5, 8, 6, // break continue return
	5, 3, 4, 5, 7, 5, 6, 6, // const ref auto class noalign align typeof sizeof
};

struct Lexeme
{
	LexemeType type;
	const char *pos;
	unsigned int length;
};

class Lexer
{
public:
	void	Lexify(const char* code);
	Lexeme*	GetStreamStart();

private:
	FastVector<Lexeme>	lexems;
};

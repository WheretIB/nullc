#pragma once

#include "stdafx.h"

#include "Allocator.h"
#include "Array.h"

enum chartype
{
	ct_symbol = 64,			// Any symbol > 127, a-z, A-Z, 0-9, _
	ct_start_symbol = 128	// Any symbol > 127, a-z, A-Z, _, :
};

static const unsigned char chartype_table[256] =
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
	lex_semiquotedchar, lex_point, lex_comma, // ' .
	lex_inc, lex_dec, // ++ --
	lex_add, lex_sub, lex_mul, lex_div, lex_mod, lex_pow, lex_less, lex_lequal, lex_shl, lex_greater, lex_gequal, lex_shr, lex_equal, lex_nequal, // + - * / % ** < <= << > >= >> == !=
	lex_bitand, lex_bitor, lex_bitxor, lex_logand, lex_logor, lex_logxor, // & | ^ && || ^^
	lex_in,
	lex_set, lex_addset, lex_subset, lex_mulset, lex_divset, lex_powset, lex_modset, lex_shlset, lex_shrset, lex_andset, lex_orset, lex_xorset, // = += -= *= /= **= %= <<= >>= &= |= ^=
	lex_bitnot, lex_lognot,	// ~ !
	lex_oparen, lex_cparen, lex_obracket, lex_cbracket, lex_ofigure, lex_cfigure, // ( ) [ ] { }
	lex_questionmark, lex_colon, lex_semicolon, // ? : ;
	lex_if, lex_else, lex_for, lex_while, lex_do, lex_switch, lex_case,	lex_default, // if else for while switch case default
	lex_break, lex_continue, lex_return, // break continue return
	lex_ref, lex_auto, lex_class, lex_noalign, lex_align, // ref auto class noalign align
	lex_typeof, lex_sizeof, lex_new, lex_operator, lex_typedef, lex_import, lex_nullptr, // typeof sizeof new operator typedef import in nullptr
	lex_coroutine, lex_yield,	// coroutine yield
	lex_at,	// @
	lex_generic, lex_const, lex_true, lex_false, lex_enum, lex_namespace, lex_extendable, lex_with // generic const true false enum namespace extendable with
};

struct Lexeme
{
	LexemeType type;
	const char *pos;
	unsigned length;

	// Zero-based values
	unsigned line;
	unsigned column;
};

class Lexer
{
public:
	Lexer(Allocator *allocator);

	void			Clear(unsigned count);
	void			Lexify(const char* code);
	void			Append(Lexeme* stream, unsigned count);

	Lexeme*			GetStreamStart();
	unsigned int	GetStreamSize();

private:
	SmallArray<Lexeme, 32>	lexems;

	unsigned line;
};

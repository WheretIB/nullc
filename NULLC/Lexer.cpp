#include "Lexer.h"

Lexer::Lexer(Allocator *allocator): lexems(allocator)
{
}

void Lexer::Clear(unsigned count)
{
	if(count)
		lexems.shrink(count);
	else
		lexems.clear();
}

void Lexer::Lexify(const char* code)
{
	lexems.reserve(2048);

	const char *lineStart = code;
	line = 0;

	LexemeType lType = lex_none;
	int lLength = 1;

	while(*code)
	{
		switch(*code)
		{
		case '\r':
			code++;

			if(*code == '\n')
				code++;

			lineStart = code;
			line++;

			continue;
		case '\n':
			code++;

			lineStart = code;
			line++;

			continue;
		case ' ':
		case '\t':
			code++;
			while((unsigned char)(code[0] - 1) < ' ' && *code != '\r' && *code != '\n')
				code++;
			continue;
		case '\"':
			lType = lex_quotedstring;
			{
				const char *pos = code;
				pos++;

				while(*pos && *pos != '\"')
				{
					if(pos[0] == '\\' && pos[1])
					{
						pos += 2;
					}
					else if(pos[0] == '\r')
					{
						pos++;

						if(*pos == '\n')
							pos++;

						lineStart = pos;
						line++;
					}
					else if(pos[0] == '\n')
					{
						pos++;

						lineStart = pos;
						line++;
					}
					else
					{
						pos += 1;
					}
				}

				if(*pos)
					pos++;

				lLength = (int)(pos - code);
			}
			break;
		case '\'':
			lType = lex_semiquotedchar;
			{
				const char *pos = code;
				pos++;

				while(*pos && *pos != '\'')
				{
					if(pos[0] == '\\' && pos[1])
					{
						pos += 2;
					}
					else if(pos[0] == '\r')
					{
						pos++;

						if(*pos == '\n')
							pos++;

						lineStart = pos;
						line++;
					}
					else if(pos[0] == '\n')
					{
						pos++;

						lineStart = pos;
						line++;
					}
					else
					{
						pos += 1;
					}
				}

				if(*pos)
					pos++;

				lLength = (int)(pos - code);
			}
			break;
		case '.':
			lType = lex_point;
			break;
		case ',':
			lType = lex_comma;
			break;
		case '+':
			lType = lex_add;
			if(code[1] == '=')
			{
				lType = lex_addset;
				lLength = 2;
			}else if(code[1] == '+'){
				lType = lex_inc;
				lLength = 2;
			}
			break;
		case '-':
			lType = lex_sub;
			if(code[1] == '=')
			{
				lType = lex_subset;
				lLength = 2;
			}else if(code[1] == '-'){
				lType = lex_dec;
				lLength = 2;
			}
			break;
		case '*':
			lType = lex_mul;
			if(code[1] == '=')
			{
				lType = lex_mulset;
				lLength = 2;
			}else if(code[1] == '*'){
				lType = lex_pow;
				lLength = 2;
				if(code[2] == '=')
				{
					lType = lex_powset;
					lLength = 3;
				}
			}
			break;
		case '/':
			if(code[1] == '=')
			{
				lType = lex_divset;
				lLength = 2;
			}else if(code[1] == '/'){
				while(code[0] != '\n' && code[0] != '\0')
					code++;
				continue;
			}else if(code[1] == '*'){
				code += 2;
				unsigned depth = 1;
				while(*code && depth)
				{
					if(code[0] == '*' && code[1] == '/')
					{
						code += 2;
						depth--;
					}
					else if(code[0] == '/' && code[1] == '*')
					{
						code += 2;
						depth++;
					}
					else if(code[0] == '\"')
					{
						code++;

						while(code[0] && code[0] != '\"')
						{
							if(code[0] == '\\' && code[1])
							{
								code += 2;
							}
							else if(code[0] == '\r')
							{
								code++;

								if(*code == '\n')
									code++;

								lineStart = code;
								line++;
							}
							else if(code[0] == '\n')
							{
								code++;

								lineStart = code;
								line++;
							}
							else
							{
								code += 1;
							}
						}

						if(code[0] == '\"')
							code++;
					}
					else if(code[0] == '\r')
					{
						code++;

						if(*code == '\n')
							code++;

						lineStart = code;
						line++;
					}
					else if(code[0] == '\n')
					{
						code++;

						lineStart = code;
						line++;
					}
					else
					{
						code++;
					}
				}
				continue;
			}else{
				lType = lex_div;
			}
			break;
		case '%':
			lType = lex_mod;
			if(code[1] == '=')
			{
				lType = lex_modset;
				lLength = 2;
			}
			break;
		case '<':
			lType = lex_less;
			if(code[1] == '=')
			{
				lType = lex_lequal;
				lLength = 2;
			}else if(code[1] == '<'){
				lType = lex_shl;
				lLength = 2;
				if(code[2] == '=')
				{
					lType = lex_shlset;
					lLength = 3;
				}
			}
			break;
		case '>':
			lType = lex_greater;
			if(code[1] == '=')
			{
				lType = lex_gequal;
				lLength = 2;
			}else if(code[1] == '>'){
				lType = lex_shr;
				lLength = 2;
				if(code[2] == '=')
				{
					lType = lex_shrset;
					lLength = 3;
				}
			}
			break;
		case '=':
			lType = lex_set;
			if(code[1] == '=')
			{
				lType = lex_equal;
				lLength = 2;
			}
			break;
		case '!':
			lType = lex_lognot;
			if(code[1] == '=')
			{
				lType = lex_nequal;
				lLength = 2;
			}
			break;
		case '@':
			lType = lex_at;
			break;
		case '~':
			lType = lex_bitnot;
			break;
		case '&':
			lType = lex_bitand;
			if(code[1] == '&')
			{
				lType = lex_logand;
				lLength = 2;
			}else if(code[1] == '='){
				lType = lex_andset;
				lLength = 2;
			}
			break;
		case '|':
			lType = lex_bitor;
			if(code[1] == '|')
			{
				lType = lex_logor;
				lLength = 2;
			}else if(code[1] == '='){
				lType = lex_orset;
				lLength = 2;
			}
			break;
		case '^':
			lType = lex_bitxor;
			if(code[1] == '^')
			{
				lType = lex_logxor;
				lLength = 2;
			}else if(code[1] == '='){
				lType = lex_xorset;
				lLength = 2;
			}
			break;
		case '(':
			lType = lex_oparen;
			break;
		case ')':
			lType = lex_cparen;
			break;
		case '[':
			lType = lex_obracket;
			break;
		case ']':
			lType = lex_cbracket;
			break;
		case '{':
			lType = lex_ofigure;
			break;
		case '}':
			lType = lex_cfigure;
			break;
		case '?':
			lType = lex_questionmark;
			break;
		case ':':
			lType = lex_colon;
			break;
		case ';':
			lType = lex_semicolon;
			break;
		default:
			if(isDigit(*code))
			{
				lType = lex_number;

				const char *pos = code;
				if(pos[0] == '0' && pos[1] == 'x')
				{
					pos += 2;
					while(isDigit(*pos) || ((*pos & ~0x20) >= 'A' && (*pos & ~0x20) <= 'F'))
						pos++;
				}else{
					while(isDigit(*pos))
						pos++;
				}
				if(*pos == '.')
					pos++;
				while(isDigit(*pos))
					pos++;
				if(*pos == 'e')
				{
					pos++;
					if(*pos == '-')
						pos++;
				}
				while(isDigit(*pos))
					pos++;
				lLength = (int)(pos - code);
			}else if(chartype_table[(unsigned char)*code] & ct_start_symbol){
				const char *pos = code;
				while(chartype_table[(unsigned char)*pos] & ct_symbol)
					pos++;
				lLength = (int)(pos - code);

				if(!(chartype_table[(unsigned char)*pos] & ct_symbol))
				{
					switch(lLength)
					{
					case 2:
						if(memcmp(code, "if", 2) == 0)
							lType = lex_if;
						else if(memcmp(code, "in", 2) == 0)
							lType = lex_in;
						else if(memcmp(code, "do", 2) == 0)
							lType = lex_do;
						break;
					case 3:
						if(memcmp(code, "for", 3) == 0)
							lType = lex_for;
						else if(memcmp(code, "ref", 3) == 0)
							lType = lex_ref;
						else if(memcmp(code, "new", 3) == 0)
							lType = lex_new;
						break;
					case 4:
						if(memcmp(code, "case", 4) == 0)
							lType = lex_case;
						else if(memcmp(code, "else", 4) == 0)
							lType = lex_else;
						else if(memcmp(code, "auto", 4) == 0)
							lType = lex_auto;
						else if(memcmp(code, "true", 4) == 0)
							lType = lex_true;
						else if(memcmp(code, "enum", 4) == 0)
							lType = lex_enum;
						else if(memcmp(code, "with", 4) == 0)
							lType = lex_with;
						break;
					case 5:
						if(memcmp(code, "while", 5) == 0)
							lType = lex_while;
						else if(memcmp(code, "break", 5) == 0)
							lType = lex_break;
						else if(memcmp(code, "class", 5) == 0)
							lType = lex_class;
						else if(memcmp(code, "align", 5) == 0)
							lType = lex_align;
						else if(memcmp(code, "yield", 5) == 0)
							lType = lex_yield;
						else if(memcmp(code, "const", 5) == 0)
							lType = lex_const;
						else if(memcmp(code, "false", 5) == 0)
							lType = lex_false;
						break;
					case 6:
						if(memcmp(code, "switch", 6) == 0)
							lType = lex_switch;
						else if(memcmp(code, "return", 6) == 0)
							lType = lex_return;
						else if(memcmp(code, "typeof", 6) == 0)
							lType = lex_typeof;
						else if(memcmp(code, "sizeof", 6) == 0)
							lType = lex_sizeof;
						else if(memcmp(code, "import", 6) == 0)
							lType = lex_import;
						break;
					case 7:
						if(memcmp(code, "noalign", 7) == 0)
							lType = lex_noalign;
						else if(memcmp(code, "default", 7) == 0)
							lType = lex_default;
						else if(memcmp(code, "typedef", 7) == 0)
							lType = lex_typedef;
						else if(memcmp(code, "nullptr", 7) == 0)
							lType = lex_nullptr;
						else if(memcmp(code, "generic", 7) == 0)
							lType = lex_generic;
						break;
					case 8:
						if(memcmp(code, "continue", 8) == 0)
							lType = lex_continue;
						else if(memcmp(code, "operator", 8) == 0)
							lType = lex_operator;
						break;
					case 9:
						if(memcmp(code, "coroutine", 9) == 0)
							lType = lex_coroutine;
						else if(memcmp(code, "namespace", 9) == 0)
							lType = lex_namespace;
						break;
					case 10:
						if(memcmp(code, "extendable", 10) == 0)
							lType = lex_extendable;
						break;
					}
				}

				if(lType == lex_none)
					lType = lex_identifier;
			}
		}
		Lexeme lex;

		lex.type = lType;
		lex.length = lLength;

		lex.pos = code;

		lex.line = line;
		lex.column = unsigned(code - lineStart);

		lexems.push_back(lex);

		code += lLength;
		lType = lex_none;
		lLength = 1;
	}

	Lexeme lex;

	lex.type = lex_none;
	lex.length = 1;

	lex.pos = code;

	lex.line = line;
	lex.column = unsigned(code - lineStart);

	lexems.push_back(lex);
}

void Lexer::Append(Lexeme* stream, unsigned count)
{
	unsigned oldSize = lexems.size();
	lexems.resize(lexems.size() + count);
	memcpy(&lexems.data[oldSize], stream, count * sizeof(Lexeme));
}

Lexeme* Lexer::GetStreamStart()
{
	return &lexems[0];
}

unsigned int Lexer::GetStreamSize()
{
	return lexems.size();
}

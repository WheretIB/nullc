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

	LexemeType lType = LexemeType::lex_none;
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
			lType = LexemeType::lex_quotedstring;
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
			lType = LexemeType::lex_semiquotedchar;
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
                        if(isDigit(code[1])) goto do_lex_number;
			else lType = LexemeType::lex_point;
			break;
		case ',':
			lType = LexemeType::lex_comma;
			break;
		case '+':
			lType = LexemeType::lex_add;
			if(code[1] == '=')
			{
				lType = LexemeType::lex_addset;
				lLength = 2;
			}else if(code[1] == '+'){
				lType = LexemeType::lex_inc;
				lLength = 2;
			}
			break;
		case '-':
			lType = LexemeType::lex_sub;
			if(code[1] == '=')
			{
				lType = LexemeType::lex_subset;
				lLength = 2;
			}else if(code[1] == '-'){
				lType = LexemeType::lex_dec;
				lLength = 2;
			}
			break;
		case '*':
			lType = LexemeType::lex_mul;
			if(code[1] == '=')
			{
				lType = LexemeType::lex_mulset;
				lLength = 2;
			}else if(code[1] == '*'){
				lType = LexemeType::lex_pow;
				lLength = 2;
				if(code[2] == '=')
				{
					lType = LexemeType::lex_powset;
					lLength = 3;
				}
			}
			break;
		case '/':
			if(code[1] == '=')
			{
				lType = LexemeType::lex_divset;
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
				lType = LexemeType::lex_div;
			}
			break;
		case '%':
			lType = LexemeType::lex_mod;
			if(code[1] == '=')
			{
				lType = LexemeType::lex_modset;
				lLength = 2;
			}
			break;
		case '<':
			lType = LexemeType::lex_less;
			if(code[1] == '=')
			{
				lType = LexemeType::lex_lequal;
				lLength = 2;
			}else if(code[1] == '<'){
				lType = LexemeType::lex_shl;
				lLength = 2;
				if(code[2] == '=')
				{
					lType = LexemeType::lex_shlset;
					lLength = 3;
				}
			}
			break;
		case '>':
			lType = LexemeType::lex_greater;
			if(code[1] == '=')
			{
				lType = LexemeType::lex_gequal;
				lLength = 2;
			}else if(code[1] == '>'){
				lType = LexemeType::lex_shr;
				lLength = 2;
				if(code[2] == '=')
				{
					lType = LexemeType::lex_shrset;
					lLength = 3;
				}
			}
			break;
		case '=':
			lType = LexemeType::lex_set;
			if(code[1] == '=')
			{
				lType = LexemeType::lex_equal;
				lLength = 2;
			}
			break;
		case '!':
			lType = LexemeType::lex_lognot;
			if(code[1] == '=')
			{
				lType = LexemeType::lex_nequal;
				lLength = 2;
			}
			break;
		case '@':
			lType = LexemeType::lex_at;
			break;
		case '~':
			lType = LexemeType::lex_bitnot;
			break;
		case '&':
			lType = LexemeType::lex_bitand;
			if(code[1] == '&')
			{
				lType = LexemeType::lex_logand;
				lLength = 2;
			}else if(code[1] == '='){
				lType = LexemeType::lex_andset;
				lLength = 2;
			}
			break;
		case '|':
			lType = LexemeType::lex_bitor;
			if(code[1] == '|')
			{
				lType = LexemeType::lex_logor;
				lLength = 2;
			}else if(code[1] == '='){
				lType = LexemeType::lex_orset;
				lLength = 2;
			}
			break;
		case '^':
			lType = LexemeType::lex_bitxor;
			if(code[1] == '^')
			{
				lType = LexemeType::lex_logxor;
				lLength = 2;
			}else if(code[1] == '='){
				lType = LexemeType::lex_xorset;
				lLength = 2;
			}
			break;
		case '(':
			lType = LexemeType::lex_oparen;
			break;
		case ')':
			lType = LexemeType::lex_cparen;
			break;
		case '[':
			lType = LexemeType::lex_obracket;
			break;
		case ']':
			lType = LexemeType::lex_cbracket;
			break;
		case '{':
			lType = LexemeType::lex_ofigure;
			break;
		case '}':
			lType = LexemeType::lex_cfigure;
			break;
		case '?':
			lType = LexemeType::lex_questionmark;
			break;
		case ':':
                        if(code[1] == ':')
			{
				lType = LexemeType::lex_dblcolon;
				lLength = 2;
			}
                        else lType = LexemeType::lex_colon;
			break;
		case ';':
			lType = LexemeType::lex_semicolon;
			break;
		default:
			if(isDigit(*code))
			{
do_lex_number:
				lType = LexemeType::lex_number;

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
				if(*pos == 'e' || *pos == 'E')
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
							lType = LexemeType::lex_if;
						else if(memcmp(code, "in", 2) == 0)
							lType = LexemeType::lex_in;
						else if(memcmp(code, "do", 2) == 0)
							lType = LexemeType::lex_do;
						break;
					case 3:
						if(memcmp(code, "for", 3) == 0)
							lType = LexemeType::lex_for;
						else if(memcmp(code, "ref", 3) == 0)
							lType = LexemeType::lex_ref;
						else if(memcmp(code, "new", 3) == 0)
							lType = LexemeType::lex_new;
						break;
					case 4:
						if(memcmp(code, "case", 4) == 0)
							lType = LexemeType::lex_case;
						else if(memcmp(code, "else", 4) == 0)
							lType = LexemeType::lex_else;
						else if(memcmp(code, "auto", 4) == 0)
							lType = LexemeType::lex_auto;
						else if(memcmp(code, "true", 4) == 0)
							lType = LexemeType::lex_true;
						else if(memcmp(code, "enum", 4) == 0)
							lType = LexemeType::lex_enum;
						else if(memcmp(code, "with", 4) == 0)
							lType = LexemeType::lex_with;
						else if(memcmp(code, "goto", 4) == 0)
							lType = LexemeType::lex_goto;
						break;
					case 5:
						if(memcmp(code, "while", 5) == 0)
							lType = LexemeType::lex_while;
						else if(memcmp(code, "break", 5) == 0)
							lType = LexemeType::lex_break;
						else if(memcmp(code, "class", 5) == 0)
							lType = LexemeType::lex_class;
						else if(memcmp(code, "align", 5) == 0)
							lType = LexemeType::lex_align;
						else if(memcmp(code, "yield", 5) == 0)
							lType = LexemeType::lex_yield;
						else if(memcmp(code, "const", 5) == 0)
							lType = LexemeType::lex_const;
						else if(memcmp(code, "false", 5) == 0)
							lType = LexemeType::lex_false;
						break;
					case 6:
						if(memcmp(code, "switch", 6) == 0)
							lType = LexemeType::lex_switch;
						else if(memcmp(code, "return", 6) == 0)
							lType = LexemeType::lex_return;
						else if(memcmp(code, "typeof", 6) == 0)
							lType = LexemeType::lex_typeof;
						else if(memcmp(code, "sizeof", 6) == 0)
							lType = LexemeType::lex_sizeof;
						else if(memcmp(code, "import", 6) == 0)
							lType = LexemeType::lex_import;
						else if(memcmp(code, "struct", 6) == 0)
							lType = LexemeType::lex_struct;
						else if(memcmp(code, "static", 6) == 0)
							lType = LexemeType::lex_static;
						else if(memcmp(code, "public", 6) == 0)
							lType = LexemeType::lex_public;
						break;
					case 7:
						if(memcmp(code, "noalign", 7) == 0)
							lType = LexemeType::lex_noalign;
						else if(memcmp(code, "default", 7) == 0)
							lType = LexemeType::lex_default;
						else if(memcmp(code, "typedef", 7) == 0)
							lType = LexemeType::lex_typedef;
						else if(memcmp(code, "nullptr", 7) == 0)
							lType = LexemeType::lex_nullptr;
						else if(memcmp(code, "generic", 7) == 0)
							lType = LexemeType::lex_generic;
						else if(memcmp(code, "private", 7) == 0)
							lType = LexemeType::lex_private;
						break;
					case 8:
						if(memcmp(code, "continue", 8) == 0)
							lType = LexemeType::lex_continue;
						else if(memcmp(code, "operator", 8) == 0)
							lType = LexemeType::lex_operator;
						else if(memcmp(code, "template", 8) == 0)
							lType = LexemeType::lex_template;
						break;
					case 9:
						if(memcmp(code, "coroutine", 9) == 0)
							lType = LexemeType::lex_coroutine;
						else if(memcmp(code, "namespace", 9) == 0)
							lType = LexemeType::lex_namespace;
						else if(memcmp(code, "protected", 9) == 0)
							lType = LexemeType::lex_protected;
						break;
					case 10:
						if(memcmp(code, "extendable", 10) == 0)
							lType = LexemeType::lex_extendable;
						break;
					}
				}

				if(lType == LexemeType::lex_none)
					lType = LexemeType::lex_identifier;
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
		lType = LexemeType::lex_none;
		lLength = 1;
	}

	Lexeme lex;

	lex.type = LexemeType::lex_none;
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

#include "Lexer.h"

void	Lexer::Lexify(const char* code)
{
	lexems.clear();

	LexemeType lType = lex_none;
	int lLength = 1;

	while(*code)
	{
		switch(*code)
		{
		case ' ':
		case '\r':
		case '\n':
		case '\t':
			code++;
			while((unsigned char)(code[0] - 1) < ' ')
				code++;
			continue;
		case '\"':
			lType = lex_quotedstring;
			{
				const char *pos = code;
				pos++;
				while(!(*pos == '\"' && pos[-1] != '\\'))
					pos++;
				pos++;
				lLength = (int)(pos - code);
			}
			break;
		case '\'':
			lType = lex_semiquote;
			break;
		case '\\':
			lType = lex_escape;
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
				while(!(code[0] == '*' && code[1] == '/') && code[0] != '\0')
					code++;
				if(*code)
					code += 2;
				continue;
			}else{
				lType = lex_div;
			}
			break;
		case '%':
			lType = lex_mod;
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
		case '~':
			lType = lex_bitnot;
			break;
		case '&':
			lType = lex_bitand;
			if(code[1] == '&')
			{
				lType = lex_logand;
				lLength = 2;
			}
			break;
		case '|':
			lType = lex_bitor;
			if(code[1] == '|')
			{
				lType = lex_logor;
				lLength = 2;
			}
			break;
		case '^':
			lType = lex_bitxor;
			if(code[1] == '^')
			{
				lType = lex_logxor;
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
					pos++;
				while(isDigit(*pos))
					pos++;
				lLength = (int)(pos - code);
			}else if(chartype_table[*code] & ct_start_symbol){
				const char *pos = code;
				while(chartype_table[*pos] & ct_symbol)
					pos++;
				lLength = (int)(pos-code);

				if(!(chartype_table[*pos] & ct_symbol))
				{
					switch(lLength)
					{
					case 2:
						if(memcmp(code, "if", 2) == 0)
							lType = lex_if;
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
						break;
					case 5:
						if(memcmp(code, "while", 5) == 0)
							lType = lex_while;
						else if(memcmp(code, "break", 5) == 0)
							lType = lex_break;
						else if(memcmp(code, "const", 5) == 0)
							lType = lex_const;
						else if(memcmp(code, "class", 5) == 0)
							lType = lex_class;
						else if(memcmp(code, "align", 5) == 0)
							lType = lex_align;
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
						break;
					case 7:
						if(memcmp(code, "noalign", 7) == 0)
							lType = lex_noalign;
						break;
					case 8:
						if(memcmp(code, "continue", 8) == 0)
							lType = lex_continue;
						break;
					}
				}
	
				if(lType == lex_none)
					lType = lex_string;
				if(!(chartype_table[*code] & ct_start_symbol))
				{
					lType = lex_none;
					break;
				}
			}
		}
		Lexeme lex;
		lex.type = lType;
		lex.length = lLength;
		lex.pos = code;
		lexems.push_back(lex);

		code += lLength;
		lType = lex_none;
		lLength = 1;
	}
	Lexeme lex;
	lex.type = lex_none;
	lex.length = 1;
	lex.pos = code;
	lexems.push_back(lex);
}

Lexeme*	Lexer::GetStreamStart()
{
	return &lexems[0];
}
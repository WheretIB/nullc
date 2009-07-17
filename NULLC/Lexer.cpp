#include "Lexer.h"

// 	lex_none,
// 	lex_number, lex_string,



void	SkipSpace(const char** str)
{
	for(;;)
	{
		while((unsigned char)((*str)[0] - 1) < ' ')
			(*str)++;
		if((*str)[0] == '/'){
			if((*str)[1] == '/')
			{
				while((*str)[0] != '\n' && (*str)[0] != '\0')
					(*str)++;
			}else if((*str)[1] == '*'){
				while(!((*str)[0] == '*' && (*str)[1] == '/') && (*str)[0] != '\0')
					(*str)++;
				(*str) += 2;
			}else{
				break;
			}
		}else{
			break;
		}
	}
}

void	Lexer::Lexify(const char* code)
{
	lexems.clear();

	while(*code)
	{
		SkipSpace(&code);
		if(!*code)
			break;

		LexemType lType = lex_none;
		int lLength = 0;

		switch(*code)
		{
		case '\"':
			lType = lex_quote;
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
				lType = lex_addset;
			break;
		case '-':
			lType = lex_sub;
			if(code[1] == '=')
				lType = lex_subset;
			break;
		case '*':
			lType = lex_mul;
			if(code[1] == '=')
			{
				lType = lex_mulset;
			}else if(code[1] == '*'){
				lType = lex_pow;
				if(code[2] == '=')
					lType = lex_powset;
			}
			break;
		case '/':
			lType = lex_div;
			if(code[1] == '=')
				lType = lex_divset;
			break;
		case '%':
			lType = lex_mod;
			break;
		case '<':
			lType = lex_less;
			if(code[1] == '=')
				lType = lex_lequal;
			break;
		case '>':
			lType = lex_greater;
			if(code[1] == '=')
				lType = lex_gequal;
			break;
		case '=':
			lType = lex_set;
			if(code[1] == '=')
				lType = lex_equal;
			break;
		case '!':
			lType = lex_lognot;
			if(code[1] == '=')
				lType = lex_nequal;
			break;
		case '~':
			lType = lex_bitnot;
			break;
		case '&':
			lType = lex_bitand;
			break;
		case '|':
			lType = lex_bitor;
			break;
		case '^':
			lType = lex_bitxor;
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
					while(isDigit(*pos) || ((*pos & 0x20) >= 'A' && (*pos & 0x20) <= 'F'))
					pos++;
				}else{
					while(isDigit(*pos))
						pos++;
				}
				/*if(*pos == 'b' || *pos == 'l' || *pos == 'f')
					pos++;
				if(*pos == '.')
					pos++;
				while(isDigit(*pos))
					pos++;
				if(*pos == 'e')
					pos++;
				while(isDigit(*pos))
					pos++;*/
				lLength = (int)(pos - code);
			}else if(chartype_table[*code] & ct_start_symbol){
				const char *pos = code;
				while(chartype_table[*pos++] & ct_symbol)
					lLength++;

				if(!(chartype_table[code[lLength]] & ct_symbol))
				{
					if(lLength == 3 && memcmp(code, "and", 3) == 0)
						lType = lex_logand;
					else if(lLength == 2 && memcmp(code, "or", 2) == 0)
						lType = lex_logor;
					else if(lLength == 3 && memcmp(code, "xor", 3) == 0)
						lType = lex_logxor;
					else if(lLength == 2 && memcmp(code, "if", 2) == 0)
						lType = lex_if;
					else if(lLength == 4 && memcmp(code, "else", 4) == 0)
						lType = lex_else;
					else if(lLength == 3 && memcmp(code, "for", 3) == 0)
						lType = lex_for;
					else if(lLength == 5 && memcmp(code, "while", 5) == 0)
						lType = lex_while;
					else if(lLength == 2 && memcmp(code, "do", 2) == 0)
						lType = lex_do;
					else if(lLength == 6 && memcmp(code, "switch", 6) == 0)
						lType = lex_switch;
					else if(lLength == 4 && memcmp(code, "case", 4) == 0)
						lType = lex_case;
					else if(lLength == 5 && memcmp(code, "break", 5) == 0)
						lType = lex_break;
					else if(lLength == 8 && memcmp(code, "continue", 8) == 0)
						lType = lex_continue;
					else if(lLength == 6 && memcmp(code, "return", 6) == 0)
						lType = lex_return;
					else if(lLength == 6 && memcmp(code, "typeof", 6) == 0)
						lType = lex_typeof;
					else if(lLength == 6 && memcmp(code, "sizeof", 6) == 0)
						lType = lex_sizeof;
				}
	
				if(lType == lex_none)
					lType = lex_string;
				if(!(chartype_table[*code] & ct_start_symbol))
				{
					lType = lex_none;
					lLength = 1;
					break;
				}
			}else{
				lLength = 1;	// unknown lexeme, let the parser handle
			}
		}
		if(!lLength)
			lLength = lexemLength[lType];

		Lexem lex;
		lex.type = lType;
		lex.length = lLength;
		lex.pos = code;
		lexems.push_back(lex);

		code += lLength;
	}
	Lexem lex;
	lex.type = lex_none;
	lex.length = 1;
	lex.pos = code;
	lexems.push_back(lex);
}

Lexem*	Lexer::GetStreamStart()
{
	return &lexems[0];
}
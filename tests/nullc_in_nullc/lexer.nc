import std.vector;
import stringutil;

enum chartype
{
	ct_symbol = 64,			// Any symbol > 127, a-z, A-Z, 0-9, _
	ct_start_symbol = 128	// Any symbol > 127, a-z, A-Z, _, :
}

short[256] chartype_table =
{
	0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,   // 0-15
	0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,   // 16-31
	0,   0,   6,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,   // 32-47
	64,  64,  64,  64,  64,  64,  64,  64,	 64,  64,  0,   0,   0,   0,   0,   0,   // 48-63
	0,   192, 192, 192, 192, 192, 192, 192,	192, 192, 192, 192, 192, 192, 192, 192, // 64-79
	192, 192, 192, 192, 192, 192, 192, 192,	192, 192, 192, 0,   0,   0,   0,   192, // 80-95
	0,   192, 192, 192, 192, 192, 192, 192,	192, 192, 192, 192, 192, 192, 192, 192, // 96-111
	192, 192, 192, 192, 192, 192, 192, 192,	192, 192, 192, 0, 0, 0, 0, 0,		   // 112-127

	192, 192, 192, 192, 192, 192, 192, 192,	192, 192, 192, 192, 192, 192, 192, 192, // 128+
	192, 192, 192, 192, 192, 192, 192, 192,	192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192,	192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192,	192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192,	192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192,	192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192,	192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192,	192, 192, 192, 192, 192, 192, 192, 192
};
/*
block comment
*/

bool isDigit(char data)
{
	return (data - '0') >= 0 && (data - '0') <= 9;
}

enum LexemeType
{
	lex_none,
	lex_number, lex_identifier, lex_quotedstring, // *(0-9) *(a-z,A-Z,_) "*any"
	lex_semiquotedchar, lex_point, lex_comma, // ' .
	lex_inc, lex_dec, // ++ --
	lex_add, lex_sub, lex_mul, lex_div, lex_mod, lex_pow, lex_less, lex_lequal, lex_shl, lex_greater, lex_gequal, lex_shr, lex_equal, lex_nequal, // + - * / % ** < <= << > >= >> == !=
	lex_bitand, lex_bitor, lex_bitxor, lex_logand, lex_logor, lex_logxor, // & | ^ && || ^^
	lex_in,
	lex_set, lex_addset, lex_subset, lex_mulset, lex_divset, lex_powset, lex_modset, lex_shlset, lex_shrset, lex_andset, lex_orset, lex_xorset, // = += -= *= /= **= %= <<= >>= &= |= ^=
	lex_bitnot, lex_lognot,	// ~ !
	lex_oparen, lex_cparen, lex_obracket, lex_cbracket, lex_ofigure, lex_cfigure, // ( ) [ ] { }
	lex_questionmark, lex_colon, lex_semicolon, lex_dblcolon, // ? : ; ::
	lex_if, lex_else, lex_for, lex_while, lex_do, lex_switch, lex_case,	lex_default, // if else for while switch case default
	lex_break, lex_continue, lex_return, // break continue return
	lex_ref, lex_auto, lex_class, lex_noalign, lex_align, // ref auto class noalign align
	lex_typeof, lex_sizeof, lex_new, lex_operator, lex_typedef, lex_import, lex_nullptr, // typeof sizeof new operator typedef import in nullptr
	lex_coroutine, lex_yield,	// coroutine yield
	lex_at,	// @
	lex_generic, lex_const, lex_true, lex_false, lex_enum, lex_namespace, lex_extendable, lex_with // generic const true false enum namespace extendable with
}

class Lexeme
{
	LexemeType type;
	int pos;
	int length;
}

class Lexer
{
	char[] code;

	vector<Lexeme> lexems;
}

class LexemeRef
{
	void LexemeRef(Lexer ref owner, int index)
	{
		this.owner = owner;
		this.index = index;

		assert(index < owner.lexems.size());
	}

	Lexer ref owner;

	int index;

	LexemeType type{ get{ return owner.lexems[index].type; }};
	StringRef pos{ get{ return StringRef(owner.code, owner.lexems[index].pos); }};
	int length{ get{ return owner.lexems[index].length; }};

	void advance()
	{
		index++;
	}
}

bool operator>(LexemeRef lhs, LexemeRef rhs)
{
	assert(lhs.owner == rhs.owner);

	return lhs.index > rhs.index;
}

bool operator==(LexemeRef lhs, LexemeRef rhs)
{
	assert(lhs.owner == rhs.owner);

	return lhs.index == rhs.index;
}

bool operator!=(LexemeRef lhs, LexemeRef rhs)
{
	assert(lhs.owner == rhs.owner);

	return lhs.index != rhs.index;
}

LexemeRef operator-(LexemeRef lhs, int rhs)
{
	assert(lhs.index >= rhs);

	return LexemeRef(lhs.owner, lhs.index - rhs);
}

LexemeRef operator+(LexemeRef lhs, int rhs)
{
	return LexemeRef(lhs.owner, lhs.index + rhs);
}

int operator-(LexemeRef lhs, LexemeRef rhs)
{
	assert(lhs.owner == rhs.owner);

	return lhs.index - rhs.index;
}

bool bool(LexemeRef lexeme)
{
	return lexeme.owner != nullptr;
}

void Lexer::Clear(int count)
{
	lexems.resize(count);
}

void Lexer::Lexify(char[] code)
{
	this.code = code;

	lexems.reserve(2048);

	LexemeType lType = LexemeType.lex_none;
	int lLength = 1;

	int curr = 0;

	void parserNumber() {
				lType = LexemeType.lex_number;

				int pos = curr;
				if(code[pos] == '0' && code[pos + 1] == 'x')
				{
					pos += 2;
					while(isDigit(code[pos]) || ((code[pos] & ~0x20) >= 'A' && (code[pos] & ~0x20) <= 'F'))
						pos++;
				}else{
					while(isDigit(code[pos]))
						pos++;
				}
				if(code[pos] == '.')
					pos++;
				while(isDigit(code[pos]))
					pos++;
				if(code[pos] == 'e' || code[pos] == 'E')
				{
					pos++;
					if(code[pos] == '-')
						pos++;
				}
				while(isDigit(code[pos]))
					pos++;
				lLength = (pos - curr);
	}

	while(curr < code.size)
	{
		switch(code[curr])
		{
		case ' ':
		case '\r':
		case '\n':
		case '\t':
			curr++;
			while(as_unsigned(code[curr] - 1) < ' ')
				curr++;
			continue;
		case '\"':
			lType = LexemeType.lex_quotedstring;
			{
				int pos = curr;
				pos++;
				while(code[pos] && code[pos] != '\"')
					pos += (code[pos] == '\\' && code[pos + 1]) ? 2 : 1;
				if(code[pos])
					pos++;
				lLength = pos - curr;
			}
			break;
		case '\'':
			lType = LexemeType.lex_semiquotedchar;
			{
				int pos = curr;
				pos++;
				while(code[pos] && code[pos] != '\'')
					pos += (code[pos] == '\\' && code[pos + 1]) ? 2 : 1;
				if(code[pos])
					pos++;
				lLength = pos - curr;
			}
			break;
		case '.':
			if(isDigit(code[1])) parserNumber();
			else lType = LexemeType.lex_point;
			break;
		case ',':
			lType = LexemeType.lex_comma;
			break;
		case '+':
			lType = LexemeType.lex_add;
			if(code[curr + 1] == '=')
			{
				lType = LexemeType.lex_addset;
				lLength = 2;
			}else if(code[curr + 1] == '+'){
				lType = LexemeType.lex_inc;
				lLength = 2;
			}
			break;
		case '-':
			lType = LexemeType.lex_sub;
			if(code[curr + 1] == '=')
			{
				lType = LexemeType.lex_subset;
				lLength = 2;
			}else if(code[curr + 1] == '-'){
				lType = LexemeType.lex_dec;
				lLength = 2;
			}
			break;
		case '*':
			lType = LexemeType.lex_mul;
			if(code[curr + 1] == '=')
			{
				lType = LexemeType.lex_mulset;
				lLength = 2;
			}else if(code[curr + 1] == '*'){
				lType = LexemeType.lex_pow;
				lLength = 2;
				if(code[curr + 2] == '=')
				{
					lType = LexemeType.lex_powset;
					lLength = 3;
				}
			}
			break;
		case '/':
			if(code[curr + 1] == '=')
			{
				lType = LexemeType.lex_divset;
				lLength = 2;
			}else if(code[curr + 1] == '/'){
				while(code[curr] != '\n' && code[curr] != '\0')
					curr++;
				continue;
			}else if(code[curr + 1] == '*'){
				curr += 2;
				int depth = 1;
				while(code[curr] && depth)
				{
					if(code[curr] == '*' && code[curr + 1] == '/')
					{
						curr += 2;
						depth--;
					}
					else if(code[curr] == '/' && code[curr + 1] == '*')
					{
						curr += 2;
						depth++;
					}
					else if(code[curr] == '\"')
					{
						curr++;

						while(code[curr] && code[curr] != '\"')
						{
							if(code[curr] == '\\' && code[curr + 1])
								curr += 2;
							else
								curr += 1;
						}

						if(code[curr] == '\"')
							curr++;
					}
					else
					{
						curr++;
					}
				}
				continue;
			}else{
				lType = LexemeType.lex_div;
			}
			break;
		case '%':
			lType = LexemeType.lex_mod;
			if(code[curr + 1] == '=')
			{
				lType = LexemeType.lex_modset;
				lLength = 2;
			}
			break;
		case '<':
			lType = LexemeType.lex_less;
			if(code[curr + 1] == '=')
			{
				lType = LexemeType.lex_lequal;
				lLength = 2;
			}else if(code[curr + 1] == '<'){
				lType = LexemeType.lex_shl;
				lLength = 2;
				if(code[curr + 2] == '=')
				{
					lType = LexemeType.lex_shlset;
					lLength = 3;
				}
			}
			break;
		case '>':
			lType = LexemeType.lex_greater;
			if(code[curr + 1] == '=')
			{
				lType = LexemeType.lex_gequal;
				lLength = 2;
			}else if(code[curr + 1] == '>'){
				lType = LexemeType.lex_shr;
				lLength = 2;
				if(code[curr + 2] == '=')
				{
					lType = LexemeType.lex_shrset;
					lLength = 3;
				}
			}
			break;
		case '=':
			lType = LexemeType.lex_set;
			if(code[curr + 1] == '=')
			{
				lType = LexemeType.lex_equal;
				lLength = 2;
			}
			break;
		case '!':
			lType = LexemeType.lex_lognot;
			if(code[curr + 1] == '=')
			{
				lType = LexemeType.lex_nequal;
				lLength = 2;
			}
			break;
		case '@':
			lType = LexemeType.lex_at;
			break;
		case '~':
			lType = LexemeType.lex_bitnot;
			break;
		case '&':
			lType = LexemeType.lex_bitand;
			if(code[curr + 1] == '&')
			{
				lType = LexemeType.lex_logand;
				lLength = 2;
			}else if(code[curr + 1] == '='){
				lType = LexemeType.lex_andset;
				lLength = 2;
			}
			break;
		case '|':
			lType = LexemeType.lex_bitor;
			if(code[curr + 1] == '|')
			{
				lType = LexemeType.lex_logor;
				lLength = 2;
			}else if(code[curr + 1] == '='){
				lType = LexemeType.lex_orset;
				lLength = 2;
			}
			break;
		case '^':
			lType = LexemeType.lex_bitxor;
			if(code[curr + 1] == '^')
			{
				lType = LexemeType.lex_logxor;
				lLength = 2;
			}else if(code[curr + 1] == '='){
				lType = LexemeType.lex_xorset;
				lLength = 2;
			}
			break;
		case '(':
			lType = LexemeType.lex_oparen;
			break;
		case ')':
			lType = LexemeType.lex_cparen;
			break;
		case '[':
			lType = LexemeType.lex_obracket;
			break;
		case ']':
			lType = LexemeType.lex_cbracket;
			break;
		case '{':
			lType = LexemeType.lex_ofigure;
			break;
		case '}':
			lType = LexemeType.lex_cfigure;
			break;
		case '?':
			lType = LexemeType.lex_questionmark;
			break;
		case ':':
			if(code[curr + 1] == ':')
			{
				lType = LexemeType.lex_dblcolon;
				lLength = 2;
			} else lType = LexemeType.lex_colon;
			break;
		case ';':
			lType = LexemeType.lex_semicolon;
			break;
		default:
			if(isDigit(code[curr]))
			{
				parserNumber();
			}else if(chartype_table[as_unsigned(code[curr])] & int(chartype.ct_start_symbol)){
				int pos = curr;
				while(chartype_table[as_unsigned(code[pos])] & int(chartype.ct_symbol))
					pos++;
				lLength = (pos - curr);

				if(!(chartype_table[as_unsigned(code[pos])] & int(chartype.ct_symbol)))
				{
					switch(lLength)
					{
					case 2:
						if(memcmp(code, curr, "if", 2) == 0)
							lType = LexemeType.lex_if;
						else if(memcmp(code, curr, "in", 2) == 0)
							lType = LexemeType.lex_in;
						else if(memcmp(code, curr, "do", 2) == 0)
							lType = LexemeType.lex_do;
						break;
					case 3:
						if(memcmp(code, curr, "for", 3) == 0)
							lType = LexemeType.lex_for;
						else if(memcmp(code, curr, "ref", 3) == 0)
							lType = LexemeType.lex_ref;
						else if(memcmp(code, curr, "new", 3) == 0)
							lType = LexemeType.lex_new;
						break;
					case 4:
						if(memcmp(code, curr, "case", 4) == 0)
							lType = LexemeType.lex_case;
						else if(memcmp(code, curr, "else", 4) == 0)
							lType = LexemeType.lex_else;
						else if(memcmp(code, curr, "auto", 4) == 0)
							lType = LexemeType.lex_auto;
						else if(memcmp(code, curr, "true", 4) == 0)
							lType = LexemeType.lex_true;
						else if(memcmp(code, curr, "enum", 4) == 0)
							lType = LexemeType.lex_enum;
						else if(memcmp(code, curr, "with", 4) == 0)
							lType = LexemeType.lex_with;
						break;
					case 5:
						if(memcmp(code, curr, "while", 5) == 0)
							lType = LexemeType.lex_while;
						else if(memcmp(code, curr, "break", 5) == 0)
							lType = LexemeType.lex_break;
						else if(memcmp(code, curr, "class", 5) == 0)
							lType = LexemeType.lex_class;
						else if(memcmp(code, curr, "align", 5) == 0)
							lType = LexemeType.lex_align;
						else if(memcmp(code, curr, "yield", 5) == 0)
							lType = LexemeType.lex_yield;
						else if(memcmp(code, curr, "const", 5) == 0)
							lType = LexemeType.lex_const;
						else if(memcmp(code, curr, "false", 5) == 0)
							lType = LexemeType.lex_false;
						break;
					case 6:
						if(memcmp(code, curr, "switch", 6) == 0)
							lType = LexemeType.lex_switch;
						else if(memcmp(code, curr, "return", 6) == 0)
							lType = LexemeType.lex_return;
						else if(memcmp(code, curr, "typeof", 6) == 0)
							lType = LexemeType.lex_typeof;
						else if(memcmp(code, curr, "sizeof", 6) == 0)
							lType = LexemeType.lex_sizeof;
						else if(memcmp(code, curr, "import", 6) == 0)
							lType = LexemeType.lex_import;
						break;
					case 7:
						if(memcmp(code, curr, "noalign", 7) == 0)
							lType = LexemeType.lex_noalign;
						else if(memcmp(code, curr, "default", 7) == 0)
							lType = LexemeType.lex_default;
						else if(memcmp(code, curr, "typedef", 7) == 0)
							lType = LexemeType.lex_typedef;
						else if(memcmp(code, curr, "nullptr", 7) == 0)
							lType = LexemeType.lex_nullptr;
						else if(memcmp(code, curr, "generic", 7) == 0)
							lType = LexemeType.lex_generic;
						break;
					case 8:
						if(memcmp(code, curr, "continue", 8) == 0)
							lType = LexemeType.lex_continue;
						else if(memcmp(code, curr, "operator", 8) == 0)
							lType = LexemeType.lex_operator;
						break;
					case 9:
						if(memcmp(code, curr, "coroutine", 9) == 0)
							lType = LexemeType.lex_coroutine;
						else if(memcmp(code, curr, "namespace", 9) == 0)
							lType = LexemeType.lex_namespace;
						break;
					case 10:
						if(memcmp(code, curr, "extendable", 10) == 0)
							lType = LexemeType.lex_extendable;
						break;
					}
				}

				if(lType == LexemeType.lex_none)
					lType = LexemeType.lex_identifier;
			}
		}
		Lexeme lex;
		lex.type = lType;
		lex.length = lLength;
		lex.pos = curr;
		lexems.push_back(lex);

		curr += lLength;
		lType = LexemeType.lex_none;
		lLength = 1;
	}
	Lexeme lex;
	lex.type = LexemeType.lex_none;
	lex.length = 1;
	lex.pos = curr;
	lexems.push_back(lex);
}

void Lexer::Append(Lexeme[] stream)
{
	for(i in stream)
		lexems.push_back(i);
}
/*
auto f = auto(int a, int b)
{
	return a * b;
};

auto apply(int a, int b, int ref(int, int) f)
{
	return auto(){ return f(a, b); };
}

coroutine int coro(int start)
{
	int a = 1;
	yield start + a;
	a++;
	yield start + a;
	a++;
	return start + a;
}

int k = f(4, 5);

int ref l = &k;
int ref ref l2 = &l;

auto arr = { coro(2), coro(2), coro(2) };

auto f2 = apply(6, 7, <a, b>{ a + b; });
int k2 = f2();

Lexer test;

test.Clear(0);

test.Lexify("return 2 * 3");

return k;
*/
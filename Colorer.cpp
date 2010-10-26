#include "stdafx.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Colorer.h"

#include "NULLC/nullc_debug.h"
#include "NULLC/StrAlgo.h" // for GetStringHash
#include "NULLC/Lexer.h" // for chartype_table

#include "SupSpi/SupSpi.h"
using namespace supspi;

class ColorCodeCallback
{
public:
	ColorCodeCallback()
	{
		colorer = NULL;
		style = COLOR_CODE;
	}
	ColorCodeCallback(Colorer* col, int nStyle)
	{
		colorer = col;
		style = nStyle;
	}

	void operator()(const char* start, const char* end)
	{
		colorer->ColorCode(style, start, end);
	}
	void operator()(int style, const char* start, const char* end)
	{
		colorer->ColorCode(style, start, end);
	}
private:
	Colorer *colorer;
	int style;
};

namespace ColorerGrammar
{
	std::string	logStr;
	const char *codeStart;

	std::vector<unsigned>	typeInfo;

	// Callbacks
	ColorCodeCallback ColorRWord, ColorVar, ColorVarDef, ColorFunc, ColorText, ColorChar, ColorReal, ColorInt, ColorBold, ColorErr, ColorComment;
	ColorCodeCallback ColorCode;

	//Error log
	ostringstream logStream;
	std::string	lastError;

	class LogError
	{
	public:
		LogError(): err(NULL){ }
		LogError(const char* str): err(str){ }

		void operator() (char const* s, char const* e)
		{
			(void)e;	// C4100
			assert(err);
			logStream << err << "\r\n";

			const char *begin = s;
			while((begin >= codeStart) && (*begin != '\n') && (*begin != '\r'))
				begin--;
			if(begin < s)
				begin++;

			const char *end = s;
			while((*end != '\r') && (*end != '\n') && (*end != 0))
				end++;

			if((end-begin) < 2048)
			{
				char line[2048];
				for(int k = 0; k < (end-begin); k++)
				{
					if(begin[k] < 0x20)
						line[k] = ' ';
					else
						line[k] = begin[k];
				}
				line[end-begin] = 0;
				logStream << "  at \"" << line << '\"';
			}else{
				logStream << "  at \"" << std::string(begin,end) << '\"';
			}
			logStream << "\r\n      ";
			for(unsigned int i = 0; i < (unsigned int)(s - begin); i++)
				logStream << ' ';
			logStream << "^\r\n";
		}
	private:
		const char* err;
	};

	class TypeNameP: public BaseP
	{
	public:
		TypeNameP(Rule a): m_a(a.getPtr()){ }
		virtual ~TypeNameP(){ }

		virtual bool	Parse(char** str, SpaceRule space)
		{
			if(space)
				space(str);
			char* curr = *str;
			m_a->Parse(str, NULL);
			if(curr == *str)
				return false;
			unsigned hash = GetStringHash(curr, *str);
			for(unsigned int i = 0; i < typeInfo.size(); i++)
				if(typeInfo[i] == hash)
					return true;
			return false;
		}
	protected:
		Rule m_a;
	};
	Rule	typenameP(Rule a){ return Rule(new TypeNameP(a)); }

	class IdentifierP: public BaseP
	{
	public:
		IdentifierP(){ }
		virtual ~IdentifierP(){ }

		virtual bool	Parse(char** str, SpaceRule space)
		{
			if(space)
				space(str);
			if(!(chartype_table[(unsigned char)**str] & ct_start_symbol))
				return false;

			char *pos = *str;
			while(chartype_table[(unsigned char)*pos] & ct_symbol)
				pos++;
			*str = pos;
			return true;
		}
	protected:
	};
	Rule	id_P(){ return Rule(new IdentifierP()); }
#define idP id_P()

	class LexemeP: public BaseP
	{
	public:
		LexemeP(const char *n){ lex = n; length = (unsigned)strlen(lex); }
		virtual ~LexemeP(){ }

		virtual bool	Parse(char** str, SpaceRule space)
		{
			if(space)
				space(str);
			if(memcmp(*str, lex, length) != 0)
				return false;
			if(chartype_table[*(*str + length)] & ct_symbol)
				return false;
			(*str) += length;
			if(space)
				space(str);
			return true;
		}
	protected:
		const char *lex;
		unsigned	length;
	};
	Rule	strWP(char* str){ return Rule(new LexemeP(str)); }

	class QuotedStrP: public BaseP
	{
	public:
		QuotedStrP(){ }
		virtual ~QuotedStrP(){ }

		virtual bool	Parse(char** str, SpaceRule space)
		{
			if(space)
				space(str);

			char *pos = *str;
			if(*pos++ != '\"')
				return false;
			ColorText(pos - 1, pos);
			while(*pos && *pos != '\"')
			{
				if(pos[0] == '\\' && pos[1])
				{
					ColorReal(pos, pos + 2);
					pos += 2;
				}else{
					ColorChar(pos, pos + 1);
					pos++;
				}
			}
			if(*pos)
				pos++;
			ColorText(pos - 1, pos);
			*str = pos;
			return true;
		}
	protected:
	};
	Rule	quotedStrP(){ return Rule(new QuotedStrP()); }

	void	ParseSpace(char** str)
	{
		char *old = *str;
		for(;;)
		{
			while((unsigned char)((*str)[0] - 1) < ' ')
				(*str)++;
			if((*str)[0] == '/'){
				if((*str)[1] == '/')
				{
					while((*str)[0] != '\n' && (*str)[0] != '\0')
						(*str)++;
					ColorComment(COLOR_COMMENT, old, *str);
				}else if((*str)[1] == '*'){
					(*str) += 2;
					while(!((*str)[0] == '*' && (*str)[1] == '/') && (*str)[0] != '\0')
						(*str)++;
					if((*str)[0])
						(*str) += 2;
					ColorComment(COLOR_COMMENT, old, *str);
				}else{
					break;
				}
			}else{
				break;
			}
		}
	}

	std::string importPath;
	void ImportStart(char const* s, char const* e)
	{
		importPath.assign(s, e);
	}
	void ImportSeparator(char const* s, char const* e)
	{
		(void)s; (void)e;	// C4100
		importPath.append(1, '/');
	}
	void ImportName(char const* s, char const* e)
	{
		importPath.append(s, e);
	}
	void ImportEnd(char const* s, char const* e)
	{
		(void)s; (void)e;	// C4100
		importPath.append(".nc");
		ByteCode *code = (ByteCode*)nullcGetModule(importPath.c_str());
		if(!code)
		{
			logStream << "ERROR: can't find module '" << importPath << "'\r\n";
		}else{
			ExternTypeInfo *tInfo = FindFirstType(code);
			const char *symbols = (char*)(code) + code->offsetToSymbols;
			for(unsigned int i = 0; i < code->typeCount; i++, tInfo++)
			{
				if(tInfo->subCat != ExternTypeInfo::CAT_CLASS)
					continue;
				bool found = false;
				for(unsigned int n = 0; n < typeInfo.size() && !found; n++)
				{
					if(tInfo->nameHash == typeInfo[n])
						found = true;
				}
				if(found)
					continue;
				typeInfo.push_back(GetStringHash(symbols + tInfo->offsetToName));
			}
		}
	}

	void StartType(char const* s, char const* e)
	{
		typeInfo.push_back(GetStringHash(s, e));
	}

	void OnError(char const* s, char const* e)
	{
		if(s == e)
			e++;
		if(logStr.length() != 0)
			ColorCode(COLOR_ERR, s, e);
		logStr = "";
	}

	void LogStrAndInfo(char const* s, char const* e)
	{
		logStream << logStr << " " << std::string(s, e) << "\r\n";
	}
	void LogStr(char const* s, char const* e)
	{
		(void)s; (void)e;	// C4100
		logStream << logStr << "\r\n";
	}

	class Grammar
	{
	public:
		void	InitGrammar()
		{
			symb		=	graphP - alnumP - chP(')');
			symb2		=	graphP - alphaP;
			typeName	=	idP - strP("return");

			arrayDef	=
				(
					chP('[')[ColorText] >>
					(term4_9 | epsP) >>
					(chP(']')[ColorText] | epsP[LogError("ERROR: closing ']' not found in array definition")]) >>
					!arrayDef
				);
			typePostExpr =
				!(
					chP('<')[ColorText] >>
					(typeExpr | strP("generic")[ColorRWord] | (chP('@')[ColorText] >> idP[ColorRWord])) >>
					*(chP(',') >> (typeExpr | strP("generic")[ColorRWord] | (chP('@')[ColorText] >> idP[ColorRWord]))) >>
					chP('>')[ColorText]
				) >>
				*(
					(strWP("ref")[ColorRWord] >> !(chP('(')[ColorText] >> !typeExpr >> *(chP(',')[ColorText] >> typeExpr) >> chP(')')[ColorText])) |
					arrayDef
				);
			typeofPostExpr =
				chP('.')[ColorText] >>
				(
					(
						strP("argument")[ColorRWord] >>
						((chP('.')[ColorText] >> (strP("first")[ColorRWord] | strP("last")[ColorRWord] | strP("size")[ColorRWord])) | (chP('[')[ColorText] >> intP[ColorReal] >> chP(']')[ColorText]))
					) |
					strP("return")[ColorRWord] |
					strP("target")[ColorRWord] |
					strP("isArray")[ColorRWord] |
					strP("isFunction")[ColorRWord] |
					strP("isReference")[ColorRWord] |
					strP("arraySize")[ColorRWord] |
					idP[ColorVar]
				);
			typeExpr	=
				(
					(strWP("auto") | typenameP(typeName))[ColorRWord] |
					(
						strP("typeof")[ColorRWord] >>
						(chP('(')[ColorText] | epsP[LogError("ERROR: '(' not found after 'typeof'")]) >>
						(term5 | epsP[LogError("ERROR: expression not found in 'typeof' statement")]) >>
						(chP(')')[ColorText] | epsP[LogError("ERROR: ')' not found after 'typeof' statement")]) >>
						*typeofPostExpr
					)
				) >>
				typePostExpr >>
				*typeofPostExpr;

			classdef	=
				((strP("align")[ColorRWord] >> '(' >> intP[ColorReal] >> ')') | (strP("noalign")[ColorRWord] | epsP)) >>
				strP("class")[ColorRWord] >>
				(idP[StartType][ColorRWord] | epsP[LogError("ERROR: class name expected")]) >>
				!(chP('<')[ColorText] >> idP[ColorRWord][StartType] >> *(chP(',') >> idP[ColorRWord][StartType]) >> chP('>')[ColorText]) >>
				(chP('{') | epsP[LogError("ERROR: '{' not found after class name")])[ColorText] >>
				*(
					typeDef |
					funcdef |
					(
						(typeExpr | (idP[ColorErr][LogError("ERROR: unknown type name")] >> typePostExpr)) >>
						(
							(
								(idP - typenameP(idP))[ColorVarDef] >>
								chP('{')[ColorText] >>
								(strP("get")[ColorRWord] | epsP[LogError("ERROR: 'get' is expected after '{'")]) >>
								(block | epsP[LogError("ERROR: function body expected after 'get'")]) >>
								!(
									strP("set")[ColorRWord] >>
									!(
										chP('(')[ColorText] >>
										((idP - typenameP(idP))[ColorVarDef] | epsP[LogError("ERROR: r-value name not found")]) >>
										(chP(')')[ColorText] | epsP[LogError("ERROR: ')' is expected after r-value name")])
									) >> 
									(block | epsP[LogError("ERROR: function body expected after 'set'")])
								) >>
								(chP('}')[ColorText] | epsP[LogError("ERROR: '}' is expected after property")])
							) | (
								((idP - typenameP(idP))[ColorVarDef] | epsP[LogError("ERROR: variable name not found after type")]) >>
								*(
									chP(',')[ColorText] >>
									((idP - typenameP(idP))[ColorVarDef] | epsP[LogError("ERROR: variable name not found after ','")])
								) 
							)
						)>>
						(chP(';') | epsP[LogError("ERROR: ';' expected after variable list")])[ColorText]
					)
				) >>
				(chP('}') | epsP[LogError("ERROR: '}' not found after class definition")])[ColorText];

			funccall	=	(typeExpr | idP)[ColorFunc] >> fcallpart;

			fcallpart	=
				chP('(')[ColorBold] >>
				!(
					term5 >>
					*(
						chP(',')[ColorText] >> 
						(term5 | epsP[LogError("ERROR: unexpected symbol after ','")])
					)[OnError]
				) >>
				(chP(')')[ColorBold] | epsP[LogError("ERROR: ')' not found after function call")]);
			funcvars	=
				!(
					(typeExpr | (strP("generic")[ColorRWord] >> !strP("ref")[ColorRWord]) | (idP[ColorErr] >> epsP[LogError("ERROR: function argument type expected after '('")])) >>
					((idP - typenameP(idP))[ColorVar] | epsP[LogError("ERROR: variable name expected after type")]) >>
					!(chP('=')[ColorText] >> term4_9)
				) >>
				*(
					chP(',')[ColorText] >>
					(
						!((typeExpr | (strP("generic")[ColorRWord] >> !strP("ref")[ColorRWord]))) >>
						((idP - typenameP(idP))[ColorVar] | epsP[LogError("ERROR: parameter name expected after ','")]) >>
						!(chP('=')[ColorText] >> term4_9)
					)
				)[OnError];
			oneLexOperator = strP("**") | strP("<=") | strP(">=") | strP("!=") | strP("==") | strP("<<") | strP(">>") | strP("&&") | strP("||") | strP("^^") |
				strP("+=") | strP("-=") | strP("*=") | strP("/=") | strP("**=") |
				chP('+') | chP('-') | chP('*') | chP('/') | chP('%') | chP('!') | chP('~') | chP('<') | chP('>') | chP('&') | chP('|') | chP('^') | chP('=');

			funcdef		=
				!strWP("coroutine")[ColorRWord] >>
				(
					(strWP("auto")[ColorRWord] >> chP('(')[ColorBold]) |
					(typeExpr >>
						(
							(
								strP("operator")[ColorRWord] >>
								(
									oneLexOperator |
									(chP('[') >> chP(']')) | (chP('(') >> chP(')')) |
									chP('=')
								)[ColorText] >>
								chP('(')[ColorBold]
							) |
							(
								typenameP(typeName)[ColorRWord] >>
								(
									chP(':')[ColorBold] |
									chP('.')[ColorBold] |
									(chP('<')[ColorText] >> typeExpr >> *(chP(',') >> typeExpr) >> chP('>')[ColorText] >> chP(':')[ColorBold])
								) >>
								(idP[ColorFunc] | epsP[LogError("ERROR: function name expected after ':'")]) >>
								chP('(')[ColorBold]
							) |
							(idP[ColorFunc] >> chP('(')[ColorBold])
						)
					)
				) >>
				(
					(*(symb | digitP))[ColorErr] >>
					funcvars
				) >>
				(chP(')')[ColorBold] | epsP[LogError("ERROR: ')' expected after function parameter list")]) >>
				!(strP("where")[ColorRWord] >> term4_9) >>
				(
					chP(';')[ColorBold] |
					(
						chP('{')[ColorBold] >>
						(code | epsP[LogError("ERROR: function body not found")]) >>
						(chP('}')[ColorBold] | epsP[LogError("ERROR: '}' not found after function body")])
					) |
					epsP[LogError("ERROR: unexpected symbol after function header")]
				);

			postExpr	=	(chP('[')[ColorText] >> !(term5 >> *(chP(',')[ColorText] >> term5)) >> chP(']')[ColorText]) |	(chP('.')[ColorText] >>	((idP[ColorFunc] >> fcallpart) | idP[ColorVar])) | fcallpart;
			appval		=	(idP - (strP("case") | strP("default")))[ColorVar] >> ~chP('(') >> *postExpr;
			addvarp		=
				(
				idP[ColorVarDef] >> 
				!chP('[')[LogError("ERROR: unexpected '[', array size must be specified after typename")]
				) >>
				((chP('=')[ColorText] >> (term5 | epsP[LogError("ERROR: expression not found after '='")])) | epsP);
			vardefsub	=	addvarp >> *(chP(',')[ColorText] >> vardefsub);
			vardef		=
				((strP("align")[ColorRWord] >> chP('(')[ColorText] >> intP[ColorReal] >> chP(')')[ColorText]) | (strP("noalign")[ColorRWord] | epsP)) >>
				(typeExpr - (typeExpr >> chP('('))) >> 
				(vardefsub | epsP[LogError("ERROR: variable definition after typename is incorrect")]);

			ifExpr			=
				!chP('@')[ColorText] >> strWP("if")[ColorRWord] >>
				(
					('(' | epsP[LogError("ERROR: '(' not found after 'if'")])[ColorText] >>
					(term5 | epsP[LogError("ERROR: condition not found in 'if' statement")]) >>
					(')' | epsP[LogError("ERROR: ')' not found after 'if' condition")])[ColorText]
				) >>
				(expr | epsP[LogError("ERROR: expression not found after 'if' statement")]) >>
				(
					(strP("else")[ColorRWord] >> (expr | epsP[LogError("ERROR: expression not found after 'else'")])) |
					epsP
				);

			forExpr			=
				strWP("for")[ColorRWord] >>
				('(' | epsP[LogError("ERROR: '(' not found after 'for'")])[ColorText] >>
				(
					(!typeExpr >> idP[ColorVarDef] >> strWP("in")[ColorRWord] >> term5 >> *(chP(',')[ColorText] >> !typeExpr >> idP[ColorVarDef] >> strWP("in")[ColorRWord] >> term5)) |
					(
						(block | vardef | term5 | epsP ) >>
						(';' | epsP[LogError("ERROR: ';' not found after initializer in 'for'")])[ColorText] >>
						(term5 | epsP[LogError("ERROR: condition not found in 'for' statement")]) >>
						(';' | epsP[LogError("ERROR: ';' not found after condition in 'for'")])[ColorText] >>
						(block | term5 | epsP)
					)
				) >>
				(')' | epsP[LogError("ERROR: ')' not found after 'for' statement")])[ColorText] >>
				(expr | epsP[LogError("ERROR: expression not found after 'for' statement")]);

			whileExpr		=
				strWP("while")[ColorRWord] >>
				(
					('(' | epsP[LogError("ERROR: '(' not found after 'while'")])[ColorText] >>
					(term5 | epsP[LogError("ERROR: condition not found in 'while' statement")]) >>
					(')' | epsP[LogError("ERROR: ')' not found after 'while' condition")])[ColorText]
				) >>
				(expr | epsP[LogError("ERROR: expression not found after 'while' statement")]);

			dowhileExpr		=
				strWP("do")[ColorRWord] >>
				(expr | epsP[LogError("ERROR: expression or block not found after 'do'")]) >>
				(strP("while")[ColorRWord] | epsP[LogError("ERROR: 'while' not found after body of 'do'")]) >>
				('(' | epsP[LogError("ERROR: '(' not found after 'while'")])[ColorText] >>
				(term5 | epsP[LogError("ERROR: condition not found in 'while' statement")]) >>
				(')' | epsP[LogError("ERROR: ')' not found after 'while' condition")])[ColorText] >>
				(';' | epsP[LogError("ERROR: ';' expected after 'do...while' statement")])[ColorText];

			switchExpr		=
				strWP("switch")[ColorRWord] >>
				(
					('(' | epsP[LogError("ERROR: '(' not found after 'switch'")])[ColorText] >>
					(term5 | epsP[LogError("ERROR: condition not found in 'switch' statement")]) >>
					(')' | epsP[LogError("ERROR: ')' not found after 'switch' condition")])[ColorText]
				) >>
				('{' | epsP[LogError("ERROR: '{' expected")])[ColorBold] >> 
				*(
					strWP("case")[ColorRWord] >>
					(term5 | epsP[LogError("ERROR: case condition expected")]) >>
					(':' | epsP[LogError("ERROR: ':' not found after case condition")])[ColorText] >>
					*expr
				) >>
				!(
					strWP("default")[ColorRWord] >>
					(':' | epsP[LogError("ERROR: ':' not found after default")])[ColorText] >>
					*expr
				) >>
				('}' | epsP[LogError("ERROR: '}' expected")])[ColorBold];

			typeDef			=	strWP("typedef")[ColorRWord] >>
				(typeExpr | epsP[LogError("ERROR: typename expected after typedef")]) >>
				(
					(typeExpr >> epsP[LogError("ERROR: there is already a type or an alias with the same name")]) |
					idP[StartType][ColorRWord] |
					epsP[LogError("ERROR: alias name expected after typename in typedef expression")]
				) >>
				(chP(';') | epsP[LogError("ERROR: ';' expected after typedef")]);

			returnExpr		=	(strWP("return") | strWP("yield"))[ColorRWord] >> (term5 | epsP) >> (+(';' >> epsP)[ColorBold] | epsP[LogError("ERROR: return statement must be followed by ';'")]);
			breakExpr		=	strWP("break")[ColorRWord] >> (term4_9 | epsP) >> (+chP(';')[ColorBold] | epsP[LogError("ERROR: break statement must be followed by ';'")]);
			continueExpr	=	strWP("continue")[ColorRWord] >> (term4_9 | epsP) >> (+chP(';')[ColorBold] | epsP[LogError("ERROR: continue statement must be followed by ';'")]);

			group		=	chP('(')[ColorText] >> term5 >> chP(')')[ColorText];
			term1		=
				funcdef |
				(
					chP('<')[ColorBold] >> (!typeExpr >> idP[ColorVar]) >> *(chP(',')[ColorText] >> !typeExpr >> idP[ColorVar]) >> chP('>')[ColorBold] >>
					(
						chP('{')[ColorBold] >>
						code >>
						(chP('}')[ColorBold] | epsP[LogError("ERROR: '}' not found after function body")])
					)
				) |
				strWP("nullptr")[ColorRWord] |
				(strWP("sizeof")[ColorRWord] >> chP('(')[ColorText] >> (typeExpr | term5) >> chP(')')[ColorText]) |
				(chP('&')[ColorText] >> appval) |
				((strP("--") | strP("++"))[ColorText] >> appval) | 
				(+chP('-')[ColorText] >> term1) | (+chP('+')[ColorText] >> term1) | ((chP('!') | '~')[ColorText] >> term1) |
				(chP('@') >> oneLexOperator[ColorFunc]) |
				(!chP('@')[ColorText] >> (quotedStrP() >> *postExpr)) |
				lexemeD[strP("0x") >> +(digitP | chP('a') | chP('b') | chP('c') | chP('d') | chP('e') | chP('f') | chP('A') | chP('B') | chP('C') | chP('D') | chP('E') | chP('F'))][ColorReal] |
				longestD[(intP >> (chP('l') | chP('b') | epsP)) | (realP >> (chP('f') | epsP))][ColorReal] |
				lexemeD[chP('\'')[ColorText] >> ((chP('\\') >> anycharP)[ColorReal] | anycharP[ColorChar]) >> chP('\'')[ColorText]] |
				(chP('{')[ColorText] >> ((forExpr >> code) | (term5 >> *(chP(',')[ColorText] >> term5))) >> chP('}')[ColorText] >> *postExpr) |
				(strWP("new")[ColorRWord] >> typeExpr >> !((chP('[')[ColorText] >> term4_9 >> chP(']')[ColorText]) | fcallpart)) |
				(group >> *postExpr) |
				(funccall >> *postExpr) |
				(typeExpr) |
				(((+chP('*')[ColorText] >> term1) | appval) >> (strP("++")[ColorText] | strP("--")[ColorText] | (chP('.')[ColorText] >> funccall >> *postExpr) | epsP));
			term2	=	term1 >> *((strP("**") - strP("**="))[ColorText] >> (term1 | epsP[LogError("ERROR: expression not found after operator **")]));
			term3	=	term2 >> *(((chP('*') - strP("*=")) | (chP('/') - strP("/=")) | chP('%'))[ColorText] >> (term2 | epsP[LogError("ERROR: expression not found after operator")]));
			term4	=	term3 >> *(((chP('+') - strP("+=")) | (chP('-') - strP("-=")))[ColorText] >> (term3 | epsP[LogError("ERROR: expression not found after operator")]));
			term4_1	=	term4 >> *((strP("<<") | strP(">>"))[ColorText] >> (term4 | epsP[LogError("ERROR: expression not found after operator")]));
			term4_2	=	term4_1 >> *((strP("<=") | strP(">=") | chP('<') | chP('>'))[ColorText] >> (term4_1 | epsP[LogError("ERROR: expression not found after operator")]));
			term4_4	=	term4_2 >> *((strP("==") | strP("!="))[ColorText] >> (term4_2 | epsP[LogError("ERROR: expression not found after operator")]));
			term4_6	=	term4_4 >> *((strP("&&") | strP("||") | strP("^^") | chP('&') | chP('|') | chP('^'))[ColorText] >> (term4_4 | epsP[LogError("ERROR: expression not found after operator")]));
			term4_9	=	term4_6 >> 
				!(
					chP('?')[ColorText] >>
					(term5 | epsP[LogError("ERROR: expression not found after operator ?")]) >>
					(chP(':')[ColorText] | epsP[LogError("ERROR: ':' not found in conditional statement")]) >>
					(term5 | epsP[LogError("ERROR: expression not found after ':' in conditional statement")])
				);
			term5	=	(
				term4_9 >>
				!(
					(chP('=')[ColorText] >> term5) |
					(
						(strP("+=") | strP("-=") | strP("*=") | strP("/=") | strP("**="))[ColorText] >>
						(term5 | epsP[LogError("ERROR: expression not found after assignment")]))
					)
				);

			block	=	chP('{')[ColorBold] >> (code | epsP) >> (chP('}')[ColorBold] | epsP[LogError("ERROR: } not found after block")]);
			expr	=	*chP(';')[ColorText] >> (classdef | block | (vardef >> (';' | epsP[LogError("ERROR: ; not found after variable definition")])[ColorText]) |
				breakExpr | continueExpr | ifExpr | forExpr | whileExpr | dowhileExpr | switchExpr | typeDef | returnExpr |
				(term5 >> (+chP(';')[ColorText] | epsP[LogError("ERROR: ; not found after expression")])));
			code	=	*(
							strP("import")[ColorRWord] >>
							((+alphaP)[ColorVar][ImportStart] | epsP[LogError("module name or folder expected")]) >>
							*(
								chP('.')[ColorText][ImportSeparator] >>
								((+alphaP)[ColorVar][ImportName] | epsP[LogError("module name or folder expected")])
							) >>
							(chP(';')[ColorText][ImportEnd] | epsP[LogError("ERROR: ';' expected after import")])
						) >>
						*(funcdef | expr | (+alnumP)[LogError("ERROR: unexpected symbol")]);
		}
		void DeInitGrammar()
		{
			DeleteParsers();
		}

		// Parsing rules
		Rule expr, block, funcdef, breakExpr, continueExpr, ifExpr, forExpr, returnExpr, vardef, vardefsub, whileExpr, dowhileExpr, switchExpr, typeDef;
		Rule term5, term4_9, term4_6, term4_4, term4_2, term4_1, term4, term3, term2, term1, group, funccall, fcallpart, funcvars;
		Rule appval, symb, symb2, addvarp, typeExpr, classdef, arrayDef, typeName, postExpr, oneLexOperator, typePostExpr, typeofPostExpr;
		// Main rule
		Rule code;
	};
};

Colorer::Colorer()
{
	syntax = new ColorerGrammar::Grammar();

	ColorerGrammar::ColorRWord	= ColorCodeCallback(this, COLOR_RWORD);
	ColorerGrammar::ColorVar	= ColorCodeCallback(this, COLOR_VAR);
	ColorerGrammar::ColorVarDef	= ColorCodeCallback(this, COLOR_VARDEF);
	ColorerGrammar::ColorFunc	= ColorCodeCallback(this, COLOR_FUNC);
	ColorerGrammar::ColorText	= ColorCodeCallback(this, COLOR_TEXT);
	ColorerGrammar::ColorBold	= ColorCodeCallback(this, COLOR_BOLD);
	ColorerGrammar::ColorChar	= ColorCodeCallback(this, COLOR_CHAR);
	ColorerGrammar::ColorReal	= ColorCodeCallback(this, COLOR_REAL);
	ColorerGrammar::ColorInt	= ColorCodeCallback(this, COLOR_INT);
	ColorerGrammar::ColorErr	= ColorCodeCallback(this, COLOR_ERR);
	ColorerGrammar::ColorComment= ColorCodeCallback(this, COLOR_COMMENT);
	

	ColorerGrammar::ColorCode	= ColorCodeCallback(this, COLOR_CODE);

	syntax->InitGrammar();
}
Colorer::~Colorer()
{
	syntax->DeInitGrammar();
	delete syntax;
}

void (*ColorFunc)(HWND, unsigned int, unsigned int, int);

bool Colorer::ColorText(HWND wnd, char *text, void (*ColFunc)(HWND, unsigned int, unsigned int, int))
{
	richEdit = wnd;
	ColorFunc = ColFunc;

	lastError = "";

	ColorerGrammar::typeInfo.clear();

	ColorerGrammar::typeInfo.push_back(GetStringHash("void"));
	ColorerGrammar::typeInfo.push_back(GetStringHash("char"));
	ColorerGrammar::typeInfo.push_back(GetStringHash("short"));
	ColorerGrammar::typeInfo.push_back(GetStringHash("int"));
	ColorerGrammar::typeInfo.push_back(GetStringHash("long"));
	ColorerGrammar::typeInfo.push_back(GetStringHash("float"));
	ColorerGrammar::typeInfo.push_back(GetStringHash("double"));
	ColorerGrammar::typeInfo.push_back(GetStringHash("typeid"));
	ColorerGrammar::typeInfo.push_back(GetStringHash("const_string"));

	ColorerGrammar::logStream.str("");

	ColorerGrammar::codeStart = text;

	errUnderline = false;
	//ColorCode(COLOR_ERR, text, text+strlen(text));

	ParseResult pRes = Parse(syntax->code, text, ColorerGrammar::ParseSpace);
	if(pRes == PARSE_ABORTED)
	{
		lastError = ColorerGrammar::lastError;
		return false;
	}
	if(pRes != PARSE_OK)
		ColorerGrammar::logStream << "ERROR: syntax error, unable to continue";

	if(ColorerGrammar::logStream.str().length() != 0)
	{
		lastError = ColorerGrammar::logStream.str();
		return false;
	}
	return true;
}

std::string Colorer::GetError()
{
	return lastError;
}

void Colorer::ColorCode(int style, const char* start, const char* end)
{
	unsigned int cpMin = (unsigned int)(start - ColorerGrammar::codeStart);
	unsigned int cpMax = (unsigned int)(end - ColorerGrammar::codeStart);

	if(errUnderline)
		ColorFunc(richEdit, cpMin, cpMax, COLOR_ERR);
	else
		ColorFunc(richEdit, cpMin, cpMax, style);
}

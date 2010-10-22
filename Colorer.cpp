#include "stdafx.h"

#include "SupSpi/SupSpi.h"
using namespace supspi;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "richedit.h"
#include <windowsx.h>

#include "Colorer.h"

#include "NULLC/nullc_debug.h"
#include "NULLC/StrAlgo.h"

#include <set>

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
	// Temporary variables
	unsigned int	varSize, varTop;
	bool	currValConst;
	std::string	logStr;

	const char *codeStart;

	std::vector<std::string>	typeInfo;
	std::vector<unsigned int>	callArgCount;

	std::set<InplaceStr>	variables;

	void MarkVar(char const* s, char const* e)
	{
		(void)s; (void)e;
		//printf("%.*s\r\n", int(e-s), s);
	}

	// Callbacks
	ColorCodeCallback ColorRWord, ColorVar, ColorVarDef, ColorFunc, ColorText, ColorChar, ColorReal, ColorInt, ColorBold, ColorErr, ColorComment;
	ColorCodeCallback ColorCode;

	//Error log
	ostringstream logStream;

	std::string	lastError;

	std::string tempStr;
	void SetTempStr(char const* s, char const* e)
	{
		tempStr.assign(s, e);
	}

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
			for(unsigned int i = 0; i < (unsigned int)(s-begin); i++)
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
			std::string type(curr, *str);
			for(unsigned int i = 0; i < typeInfo.size(); i++)
				if(typeInfo[i] == type)
					return true;
			return false;
		}
	protected:
		Rule m_a;
	};
	Rule	typenameP(Rule a){ return Rule(new TypeNameP(a)); }
	Rule	strWP(char* str){ return (lexemeD[strP(str) >> (epsP - (alnumP | '_'))]); }

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

	class Grammar
	{
	public:
		void	InitGrammar()
		{
			constExpr		=	epsP[AssignVar<bool>(currValConst, false)] >>
				!strP("const")[ColorRWord][AssignVar<bool>(currValConst, true)];
			symb		=	graphP - alnumP - chP(')');
			symb2		=	graphP - alphaP;
			varname		=	lexemeD[(alphaP | '_') >> *(alnumP | '_')];
			typeName	=	varname - strP("return");

			arrayDef	=
				(
					chP('[')[ColorText] >>
					(term4_9 | epsP) >>
					(chP(']')[ColorText] | epsP[LogError("ERROR: closing ']' not found in array definition")]) >>
					!arrayDef
				);
			typeExpr	=
				(
					(strWP("auto") | typenameP(typeName))[ColorRWord] |
					(
						strP("typeof")[ColorRWord] >>
						(chP('(')[ColorText] | epsP[LogError("ERROR: '(' not found after 'typeof'")]) >>
						(term5 | epsP[LogError("ERROR: expression not found in 'typeof' statement")]) >>
						(chP(')')[ColorText] | epsP[LogError("ERROR: ')' not found after 'typeof' statement")]) >>
						!(
							chP('.')[ColorText] >>
							(
								(
									strP("argument")[ColorRWord] >>
									((chP('.')[ColorText] >> (strP("first")[ColorRWord] | strP("last")[ColorRWord])) | (chP('[')[ColorText] >> intP[ColorReal] >> chP(']')[ColorText]))
								) |
								strP("return")[ColorRWord] |
								strP("target")[ColorRWord]
							)
						)
					)
				) >>
				*(
					(strWP("ref")[ColorRWord] >> !(chP('(')[ColorText] >> !typeExpr >> *(chP(',')[ColorText] >> typeExpr) >> chP(')')[ColorText])) |
					arrayDef
				);

			classdef	=
				((strP("align")[ColorRWord] >> '(' >> intP[ColorReal] >> ')') | (strP("noalign")[ColorRWord] | epsP)) >>
				strP("class")[ColorRWord] >>
				(varname[StartType][ColorRWord] | epsP[LogError("ERROR: class name expected")]) >>
				(chP('{') | epsP[LogError("ERROR: '{' not found after class name")])[ColorText][BlockBegin] >>
				*(
					typeDef |
					funcdef |
					(
						typeExpr >>
						(
							(
								(varname - typenameP(varname))[ColorVarDef] >>
								chP('{')[ColorText] >>
								(strP("get")[ColorRWord] | epsP[LogError("ERROR: 'get' is expected after '{'")]) >>
								(block | epsP[LogError("ERROR: function body expected after 'get'")]) >>
								!(
									strP("set")[ColorRWord] >>
									!(
										chP('(')[ColorText] >>
										((varname - typenameP(varname))[ColorVarDef] | epsP[LogError("ERROR: r-value name not found")]) >>
										(chP(')')[ColorText] | epsP[LogError("ERROR: ')' is expected after r-value name")])
									) >> 
									(block | epsP[LogError("ERROR: function body expected after 'set'")])
								) >>
								(chP('}')[ColorText] | epsP[LogError("ERROR: '}' is expected after property")])
							) | (
								((varname - typenameP(varname))[ColorVarDef][AddVar] | epsP[LogError("ERROR: variable name not found after type")]) >>
								*(
									chP(',')[ColorText] >>
									((varname - typenameP(varname))[ColorVarDef][AddVar] | epsP[LogError("ERROR: variable name not found after ','")])
								) 
							)
						)>>
						(chP(';') | epsP[LogError("ERROR: ';' expected after variable list")])[ColorText]
					)
				) >>
				(chP('}') | epsP[LogError("ERROR: '}' not found after class definition")])[ColorText][BlockEnd];

			funccall	=	(typeExpr | varname)[ColorFunc] >> fcallpart;

			fcallpart	=
				strP("(")[ColorBold][PushBackVal<std::vector<unsigned int>, unsigned int>(callArgCount, 0)] >>
				!(
					term5[ArrBackInc<std::vector<unsigned int> >(callArgCount)] >>
					*(
						strP(",")[ColorText] >> 
						(term5[ArrBackInc<std::vector<unsigned int> >(callArgCount)] | epsP[LogError("ERROR: unexpected symbol after ','")])
					)[OnError]
				) >>
				(strP(")")[ColorBold] | epsP[LogError("ERROR: ')' not found after function call")]);
			funcvars	=
				!(
					(typeExpr | (strP("generic")[ColorRWord] >> !strP("ref")[ColorRWord])) >>
					constExpr >>
					((varname - typenameP(varname))[ColorVar][AddVar][ArrBackInc<std::vector<unsigned int> >(callArgCount)] | epsP[LogError("ERROR: variable name expected after type")]) >>
					!(chP('=')[ColorText] >> term4_9)
				) >>
				*(
					strP(",")[ColorText] >>
					(
						!((typeExpr | (strP("generic")[ColorRWord] >> !strP("ref")[ColorRWord])) >> constExpr) >>
						((varname - typenameP(varname))[ColorVar][AddVar][ArrBackInc<std::vector<unsigned int> >(callArgCount)] | epsP[LogError("ERROR: parameter name expected after ','")]) >>
						!(chP('=')[ColorText] >> term4_9)
					)
				)[OnError];
			funcdef		=
				!strWP("coroutine")[ColorRWord] >>
				(
					(strWP("auto")[ColorRWord] >> chP('(')[ColorBold]) |
					(typeExpr >>
						(
							(
								strP("operator")[ColorRWord] >>
								(
									strP("**") | strP("<=") | strP(">=") | strP("!=") | strP("==") | strP("<<") | strP(">>") | strP("&&") | strP("||") | strP("^^") | strP("()") |
									strP("+=") | strP("-=") | strP("*=") | strP("/=") | strP("**=") |
									chP('+') | chP('-') | chP('*') | chP('/') | chP('%') | chP('!') | chP('~') | chP('<') | chP('>') | chP('&') | chP('|') | chP('^') |
									(chP('[') >> chP(']')) | (chP('(') >> chP(')')) |
									chP('=')
								)[ColorText] >>
								chP('(')[ColorBold]
							) |
							(typenameP(typeName)[ColorRWord] >> (chP(':') | chP('.'))[ColorBold] >> (varname[ColorFunc] | epsP[LogError("ERROR: function name expected after ':'")]) >> chP('(')[ColorBold]) |
							(varname[ColorFunc] >> chP('(')[ColorBold])
						)[SetTempStr]
					)
				)[PushBackVal<std::vector<unsigned int>, unsigned int>(callArgCount, 0)][FuncAdd][BlockBegin] >>
				(
					(*(symb | digitP))[ColorErr] >>
					funcvars
				) >>
				(chP(')')[ColorBold][FuncEnd] | epsP[LogError("ERROR: ')' expected after function parameter list")]) >>
				(
					chP(';')[ColorBold] |
					(
						chP('{')[ColorBold] >>
						(code | epsP[LogError("ERROR: function body not found")]) >>
						(chP('}')[ColorBold][BlockEnd] | epsP[LogError("ERROR: '}' not found after function body")])
					) |
					epsP[LogError("ERROR: unexpected symbol after function header")]
				);

			postExpr	=	(chP('[')[ColorText] >> term5 >> chP(']')[ColorText]) |	(chP('.')[ColorText] >>	((varname[ColorFunc] >> fcallpart) | varname[ColorVar])) | fcallpart;
			appval		=	(varname - (strP("case") | strP("default")))[ColorVar][MarkVar] >> ~chP('(') >> *postExpr;
			addvarp		=
				(
				varname[ColorVarDef] >> epsP[AssignVar<unsigned int>(varSize,1)] >> 
				!chP('[')[LogError("ERROR: unexpected '[', array size must be specified after typename")]
				)[AddVar] >>
				((chP('=')[ColorText] >> (term5 | epsP[LogError("ERROR: expression not found after '='")])) | epsP);
			vardefsub	=	addvarp >> *(chP(',')[ColorText] >> vardefsub);
			vardef		=
				((strP("align")[ColorRWord] >> '(' >> intP[ColorReal] >> ')') | (strP("noalign")[ColorRWord] | epsP)) >>
				(typeExpr - (typeExpr >> chP('('))) >> 
				(
					(
						constExpr >>
						(vardefsub | epsP[LogError("ERROR: variable definition after typename is incorrect")])
					)
				);

			ifExpr			=
				strWP("if")[ColorRWord] >>
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
					(!typeExpr >> varname[ColorVarDef] >> strWP("in")[ColorRWord] >> term5 >> *(chP(',')[ColorText] >> !typeExpr >> varname[ColorVarDef] >> strWP("in")[ColorRWord] >> term5)) |
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
					varname[StartType][ColorRWord] |
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
					chP('<')[ColorBold] >> (!typeExpr >> varname[ColorVar]) >> *(chP(',')[ColorText] >> !typeExpr >> varname[ColorVar]) >> chP('>')[ColorBold] >>
					(
						chP('{')[ColorBold] >>
						code >>
						(chP('}')[ColorBold] | epsP[LogError("ERROR: '}' not found after function body")])
					)
				) |
				strWP("nullptr")[ColorRWord] |
				(strWP("sizeof")[ColorRWord] >> chP('(')[ColorText] >> (typeExpr | term5) >> chP(')')[ColorText]) |
				(chP('&')[ColorText] >> appval) |
				((strP("--") | strP("++"))[ColorText] >> appval[GetVar]) | 
				(+chP('-')[ColorText] >> term1) | (+chP('+')[ColorText] >> term1) | ((chP('!') | '~')[ColorText] >> term1) |
				(!chP('@')[ColorText] >> chP('\"')[ColorText] >> *((strP("\\\"") | strP("\\r") | strP("\\n") | strP("\\\'") | strP("\\\\") | strP("\\t") | strP("\\0"))[ColorReal] | (anycharP[ColorChar] - chP('\"'))) >> chP('\"')[ColorText] >> *postExpr) |
				lexemeD[strP("0x") >> +(digitP | chP('a') | chP('b') | chP('c') | chP('d') | chP('e') | chP('f') | chP('A') | chP('B') | chP('C') | chP('D') | chP('E') | chP('F'))][ColorReal] |
				longestD[(intP >> (chP('l') | chP('b') | epsP)) | (realP >> (chP('f') | epsP))][ColorReal] |
				lexemeD[chP('\'')[ColorText] >> ((chP('\\') >> anycharP)[ColorReal] | anycharP[ColorChar]) >> chP('\'')[ColorText]] |
				(chP('{')[ColorText] >> ((forExpr >> code) | (term5 >> *(chP(',')[ColorText] >> term5))) >> chP('}')[ColorText] >> *postExpr) |
				(strWP("new")[ColorRWord] >> typeExpr >> !((chP('[')[ColorText] >> term4_9 >> chP(']')[ColorText]) | fcallpart)) |
				(group >> *postExpr) |
				(funccall[FuncCall] >> *postExpr) |
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

			block	=	chP('{')[ColorBold][BlockBegin] >> (code | epsP) >> (chP('}')[ColorBold][BlockEnd] | epsP[LogError("ERROR: } not found after block")]);
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
		Rule appval, varname, symb, symb2, constExpr, addvarp, typeExpr, classdef, arrayDef, typeName, postExpr;
		// Main rule
		Rule code;
	};

	bool CheckIfDeclared(const std::string& str)
	{
		if(str == "if" || str == "else" || str == "for" || str == "while" || str == "return" || str=="switch" || str=="case")
		{
			logStream << "ERROR: the name '" << str << "' is reserved\r\n";
			return true;
		}
		return false;
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
					if(tInfo->nameHash == GetStringHash(typeInfo[n].c_str()))
						found = true;
				}
				if(found)
					continue;
				typeInfo.push_back(symbols + tInfo->offsetToName);
			}
		}
	}

	void AddVar(char const* s, char const* e)
	{
		(void)s; (void)e;	// C4100
	}

	void SetVar(char const* s, char const* e)
	{
		(void)s; (void)e;	// C4100
	}

	void GetVar(char const* s, char const* e)
	{
		(void)s; (void)e;	// C4100
	}

	void FuncAdd(char const* s, char const* e)
	{
		(void)s; (void)e;	// C4100
	}

	void FuncEnd(char const* s, char const* e)
	{
		(void)s; (void)e;	// C4100
	}

	void FuncCall(char const* s, char const* e)
	{
		(void)s; (void)e;	// C4100
	}

	void StartType(char const* s, char const* e)
	{
		typeInfo.push_back(std::string(s, e));
	}

	void OnError(char const* s, char const* e)
	{
		if(s == e)
			e++;
		if(logStr.length() != 0)
			ColorCode(COLOR_ERR, s, e);
		logStr = "";
	}
	void BlockBegin(char const* s, char const* e)
	{
		(void)s; (void)e;	// C4100
	}
	void BlockEnd(char const* s, char const* e)
	{
		(void)s; (void)e;	// C4100
	}

	void LogStrAndInfo(char const* s, char const* e)
	{
		logStream << logStr << " " << std::string(s,e) << "\r\n";
	}
	void LogStr(char const* s, char const* e)
	{
		(void)s; (void)e;	// C4100
		logStream << logStr << "\r\n";
	}
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

	ColorerGrammar::typeInfo.push_back("void");
	ColorerGrammar::typeInfo.push_back("char");
	ColorerGrammar::typeInfo.push_back("short");
	ColorerGrammar::typeInfo.push_back("int");
	ColorerGrammar::typeInfo.push_back("long");
	ColorerGrammar::typeInfo.push_back("float");
	ColorerGrammar::typeInfo.push_back("double");
	ColorerGrammar::typeInfo.push_back("typeid");
	ColorerGrammar::typeInfo.push_back("const_string");

	ColorerGrammar::callArgCount.clear();
	ColorerGrammar::varSize = 1;
	ColorerGrammar::varTop = 3;

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
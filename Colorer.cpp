#include "stdafx.h"

#include "NULLC/SupSpi/SupSpi.h"
using namespace supspi;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "richedit.h"
#include <windowsx.h>

#include "NULLC/ParseClass.h"
#include "Colorer.h"

class ColorCodeCallback
{
public:
	ColorCodeCallback()
	{
		colorer = NULL;
		colRed = colGreen = colBlue = 0;
		styleBold = styleItalic = styleUnderlined = 0;
	}
	ColorCodeCallback(Colorer* col, int red, int green, int blue, int bold, int ital, int under)
	{
		colorer = col;
		colRed = red;
		colGreen = green;
		colBlue = blue;
		styleBold = bold;
		styleItalic = ital;
		styleUnderlined = under;
	}

	void operator()(const char* start, const char* end)
	{
		colorer->ColorCode(colRed, colGreen, colBlue, styleBold, styleItalic, styleUnderlined, start, end);
	}
	void operator()(int red, int green, int blue, int bold, int ital, int under, const char* start, const char* end)
	{
		colorer->ColorCode(red, green, blue, bold, ital, under, start, end);
	}
private:
	Colorer *colorer;
	int colRed, colGreen, colBlue;
	int styleBold, styleItalic, styleUnderlined;
};

namespace ColorerGrammar
{
	// Temporary variables
	unsigned int	varSize, varTop;
	bool	currValConst;
	std::string	logStr;

	const char *codeStart;

	std::vector<FunctionInfo*>	funcs;
	std::vector<std::string>	typeInfo;
	std::vector<VariableInfo>	varInfo;
	std::vector<VarTopInfo>		varInfoTop;
	std::vector<unsigned int>	callArgCount;

	// Callbacks
	ColorCodeCallback ColorRWord, ColorVar, ColorVarDef, ColorFunc, ColorText, ColorReal, ColorInt, ColorBold, ColorErr, ColorComment;
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
			while((begin > codeStart) && (*begin != '\n') && (*begin != '\r'))
				begin--;
			begin++;

			const char *end = s;
			while((*end != '\r') && (*end != '\n') && (*end != 0))
				end++;

			logStream << "  at \"" << std::string(begin,end) << '\"';
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

		virtual bool	Parse(char** str, BaseP* space)
		{
			SkipSpaces(str, space);
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
	class MySpaceP: public BaseP
	{
	public:
		MySpaceP(){ }
		virtual ~MySpaceP(){ }

		virtual bool	Parse(char** str, BaseP* space)
		{
			for(;;)
			{
				while((unsigned char)((*str)[0] - 1) < ' ')
					(*str)++;
				if((*str)[0] == '/'){
					if((*str)[1] == '/')
					{
						char *start = *str;
						while((*str)[0] != '\n' && (*str)[0] != '\0')
							(*str)++;
						ColorComment(start, *str);
					}else if((*str)[1] == '*'){
						char *start = *str;
						while(!((*str)[0] == '*' && (*str)[1] == '/') && (*str)[0] != '\0')
							(*str)++;
						(*str) += 2;
						ColorComment(start, *str);
					}else{
						break;
					}
				}else{
					break;
				}
			}
			return true;
		}
	protected:
	};
	Rule	myspaceP(){ return Rule(new MySpaceP()); }
	Rule	strWP(char* str){ return (lexemeD[strP(str) >> (epsP - alnumP)]); }

	class Grammar
	{
	public:
		void	InitGrammar()
		{
			constExpr		=	epsP[AssignVar<bool>(currValConst, false)] >>
				!strP("const")[ColorRWord][AssignVar<bool>(currValConst, true)];
			symb		=	graphP - alnumP - chP(')');
			symb2		=	graphP - alphaP;
			varname		=	lexemeD[alphaP >> *(alnumP | '_')];
			typeName	=	varname - strP("return") ;

			arrayDef	=
				(
					chP('[')[ColorText] >>
					(term4_9 | epsP) >>
					(chP(']')[ColorText] | epsP[LogError("ERROR: closing ']' not found in array definition")]) >>
					!arrayDef
				);
			typeExpr	=
				(
					(strP("auto") | typenameP(typeName))[ColorRWord] |
					(
						strP("typeof")[ColorRWord] >>
						(chP('(')[ColorText] | epsP[LogError("ERROR: '(' not found after 'typeof'")]) >>
						(term5 | epsP[LogError("ERROR: expression not found in 'typeof' statement")]) >>
						(chP(')')[ColorText] | epsP[LogError("ERROR: ')' not found after 'typeof' statement")])
					)
				) >> *(lexemeD[strP("ref") >> (~alnumP | nothingP)][ColorRWord] | arrayDef);

			classdef	=
				strP("class")[ColorRWord] >>
				(varname[StartType][ColorRWord] | epsP[LogError("ERROR: class name expected")]) >>
				(chP('{') | epsP[LogError("ERROR: '{' not found after class name")])[ColorText][BlockBegin] >>
				*(
					funcdef |
					(
						typeExpr >>
						((varname - typenameP(varname))[ColorVarDef][AddVar] | epsP[LogError("ERROR: variable name not found after type")]) >>
						*(
							chP(',')[ColorText] >>
							((varname - typenameP(varname))[ColorVarDef][AddVar] | epsP[LogError("ERROR: variable name not found after ','")])
						) >>
						(chP(';') | epsP[LogError("ERROR: ';' expected after variable list")])[ColorText]
					)
				) >>
				(chP('}') | epsP[LogError("ERROR: '}' not found after class definition")])[ColorText][BlockEnd][AddType];

			funccall	=	varname[ColorFunc] >> 
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
					typeExpr >>
					constExpr >>
					((varname - typenameP(varname))[ColorVar][AddVar][ArrBackInc<std::vector<unsigned int> >(callArgCount)] | epsP[LogError("ERROR: variable name expected after type")])
				) >>
				*(
					strP(",")[ColorText] >>
					(
						typeExpr >>
						constExpr >>
						((varname - typenameP(varname))[ColorVar][AddVar][ArrBackInc<std::vector<unsigned int> >(callArgCount)] | epsP[LogError("ERROR: parameter name expected after ','")])
					)
				)[OnError];
			funcdef		=
				typeExpr >>
				(varname - typenameP(varname))[ColorFunc][SetTempStr] >>
				chP('(')[ColorBold][PushBackVal<std::vector<unsigned int>, unsigned int>(callArgCount, 0)][FuncAdd][BlockBegin] >>
				(
					(*(symb | digitP))[ColorErr] >>
					funcvars
				) >>
				(chP(')')[ColorBold][FuncEnd] | epsP[LogError("ERROR: ')' expected after function parameter list")]) >>
				(chP('{')[ColorBold] | epsP[LogError("ERROR: '{' not found after function header")]) >>
				(code | epsP[LogError("ERROR: function body not found")]) >>
				(chP('}')[ColorBold][BlockEnd] | epsP[LogError("ERROR: '}' not found after function body")]);

			appval		=
				(
					(varname - strP("case"))[ColorVar] >> ~chP('(') >>
					*(
						chP('[')[ColorText] >> 
						term5 >> 
						chP(']')[ColorText]
					) >>
					*(
						chP('.')[ColorText] >>
						varname[ColorVar] >> (~chP('(') | nothingP) >>
						*(
							chP('[')[ColorText] >>
							term5 >>
							chP(']')[ColorText]
						)
					)
				);
			addvarp		=
				(
				varname[ColorVarDef] >> epsP[AssignVar<unsigned int>(varSize,1)] >> 
				!(chP('[')[ColorText] >> term4_9 >> chP(']')[ColorText])
				)[AddVar] >>
				((chP('=')[ColorText] >> (term5 | epsP[LogError("ERROR: expression not found after '='")])) | epsP);
			vardefsub	=	addvarp >> *(chP(',')[ColorText] >> vardefsub);
			vardef		=
				typeExpr >>
				constExpr >>
				vardefsub;

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
					(
						chP('{')[ColorBold] >>
						(code | epsP) >>
						(chP('}')[ColorBold] | epsP[LogError("ERROR: '}' not found after '{'")])
					) |
					vardef |
					term5 |
					epsP
				) >>
				(';' | epsP[LogError("ERROR: ';' not found after initializer in 'for'")])[ColorText] >>
				(term5 | epsP[LogError("ERROR: condition not found in 'for' statement")]) >>
				(';' | epsP[LogError("ERROR: ';' not found after condition in 'for'")])[ColorText] >>
				(block | term5 | epsP) >>
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

			switchExpr		=	strWP("switch")[ColorRWord] >> ('(' >> epsP)[ColorText] >> term5 >> (')' >> epsP)[ColorText] >> ('{' >> epsP)[ColorBold] >> 
				(strWP("case")[ColorRWord] >> term5 >> (':' >> epsP)[ColorText] >> expr >> *expr) >>
				*(strWP("case")[ColorRWord] >> term5 >> (':' >> epsP)[ColorText] >> expr >> *expr) >>
				('}' >> epsP)[ColorBold];
			returnExpr		=	strWP("return")[ColorRWord] >> (term5 | epsP) >> (+(';' >> epsP)[ColorBold] | epsP[LogError("ERROR: return must be followed by ';'")]);
			breakExpr		=	strWP("break")[ColorRWord] >> (+chP(';')[ColorBold] | epsP[LogError("ERROR: break must be followed by ';'")]);
			continueExpr		=	strWP("continue")[ColorRWord] >> (+chP(';')[ColorBold] | epsP[LogError("ERROR: continue must be followed by ';'")]);

			group		=	chP('(')[ColorText] >> term5 >> chP(')')[ColorText];
			term1		=
				funcdef |
				(strP("sizeof")[ColorRWord] >> chP('(')[ColorText] >> (typeExpr | term5) >> chP(')')[ColorText]) |
				(chP('&')[ColorText] >> appval) |
				((strP("--") | strP("++"))[ColorText] >> appval[GetVar]) | 
				(+chP('-')[ColorText] >> term1) | (+chP('+')[ColorText] >> term1) | ((chP('!') | '~')[ColorText] >> term1) |
				(chP('\"')[ColorText] >> *((strP("\\\"") | strP("\\r") | strP("\\n") | strP("\\\'") | strP("\\\\") | strP("\\t") | strP("\\0"))[ColorReal] | (anycharP[ColorVar] - chP('\"'))) >> chP('\"')[ColorText]) |
				lexemeD[strP("0x") >> +(digitP | chP('a') | chP('b') | chP('c') | chP('d') | chP('e') | chP('f') | chP('A') | chP('B') | chP('C') | chP('D') | chP('E') | chP('F'))][ColorReal] |
				longestD[(intP >> (chP('l') | epsP)) | (realP >> (chP('f') | epsP))][ColorReal] |
				(chP('\'')[ColorText] >> ((chP('\\') >> anycharP)[ColorReal] | anycharP[ColorVar]) >> chP('\'')[ColorText]) |
				(chP('{')[ColorText] >> term5 >> *(chP(',')[ColorText] >> term5) >> chP('}')[ColorText]) |
				group | funccall[FuncCall] |
				(!chP('*')[ColorText] >> appval[GetVar] >> (strP("++") | strP("--") | ('.' >> funccall) | epsP)[ColorText]);
			term2	=	term1 >> *(strP("**")[ColorText] >> (term1 | epsP[LogError("ERROR: expression not found after operator **")]));
			term3	=	term2 >> *((chP('*') | chP('/') | chP('%'))[ColorText] >> (term2 | epsP[LogError("ERROR: expression not found after operator")]));
			term4	=	term3 >> *((chP('+') | chP('-'))[ColorText] >> (term3 | epsP[LogError("ERROR: expression not found after operator")]));
			term4_1	=	term4 >> *((strP("<<") | strP(">>"))[ColorText] >> (term4 | epsP[LogError("ERROR: expression not found after operator")]));
			term4_2	=	term4_1 >> *((strP("<=") | strP(">=") | chP('<') | chP('>'))[ColorText] >> (term4_1 | epsP[LogError("ERROR: expression not found after operator")]));
			term4_4	=	term4_2 >> *((strP("==") | strP("!="))[ColorText] >> (term4_2 | epsP[LogError("ERROR: expression not found after operator")]));
			term4_6	=	term4_4 >> *((strP("&") | strP("|") | strP("^") | strP("and") | strP("or") | strP("xor"))[ColorText] >> (term4_4 | epsP[LogError("ERROR: expression not found after operator")]));
			term4_9	=	term4_6 >> 
				!(
					chP('?')[ColorText] >>
					(term5 | epsP[LogError("ERROR: expression not found after operator ?")]) >>
					(chP(':')[ColorText] | epsP[LogError("ERROR: ':' not found in conditional statement")]) >>
					(term5 | epsP[LogError("ERROR: expression not found after ':' in conditional statement")])
				);
			term5	=	(
				!chP('*')[ColorText] >>
				appval[SetVar] >>
				(
					(chP('=')[ColorText] >> term5) |
					(
						(strP("+=") | strP("-=") | strP("*=") | strP("/=") | strP("**="))[ColorText] >>
						(term5 | epsP[LogError("ERROR: expression not found after assignment")]))
					)
				) |
				term4_9;

			block	=	chP('{')[ColorBold][BlockBegin] >> code >> chP('}')[ColorBold][BlockEnd];
			expr	=	*chP(';')[ColorText] >> (classdef | block | (vardef >> (';' >> epsP)[ColorText]) | breakExpr | continueExpr | ifExpr | forExpr | whileExpr | dowhileExpr | switchExpr | returnExpr | (term5 >> +(';' >> epsP)[ColorText]));
			code	=	*(funcdef | expr);

			mySpaceP = myspaceP();
		}
		void DeInitGrammar()
		{
			DeleteParsers();
		}

		// Parsing rules
		Rule expr, block, funcdef, breakExpr, continueExpr, ifExpr, forExpr, returnExpr, vardef, vardefsub, whileExpr, dowhileExpr, switchExpr;
		Rule term5, term4_9, term4_6, term4_4, term4_2, term4_1, term4, term3, term2, term1, group, funccall, funcvars;
		Rule appval, varname, symb, symb2, constExpr, addvarp, typeExpr, classdef, arrayDef, typeName;
		// Main rule and space parsers
		Rule code;
		Rule mySpaceP;
	};

	bool CheckIfDeclared(const std::string& str, bool forFunction = false)
	{
		if(str == "if" || str == "else" || str == "for" || str == "while" || str == "return" || str=="switch" || str=="case")
		{
			logStream << "ERROR: The name '" << str << "' is reserved" << "\r\n";
			return true;
		}
		if(!forFunction)
		{
			for(unsigned int i = 0; i < funcs.size(); i++)
			{
				if(funcs[i]->name == str)
				{
					logStream << "ERROR: Name '" << str << "' is already taken for a function" << "\r\n";
					return true;
				}
			}
		}
		return false;
	}
	void AddVar(char const* s, char const* e)
	{
		(void)e;	// C4100
		const char* st=s;
		while(isalnum(*st) || *st == '_')
			st++;
		string vName = std::string(s, st);

		for(unsigned int i = varInfoTop.back().activeVarCnt; i < varInfo.size(); i++)
		{
			if(varInfo[i].name == vName)
			{
				ColorCode(255, 0, 0, 0, 0, 1, s, st);
				logStream << "ERROR: Name '" << vName << "' is already taken for a variable in current scope\r\n";
				return;
			}
		}
		if(CheckIfDeclared(vName))
		{
			ColorCode(255, 0, 0, 0, 0, 1, s, st);
			return;
		}

		varInfo.push_back(VariableInfo(vName, 0, NULL, currValConst));
		varSize = 1;
	}

	void SetVar(char const* s, char const* e)
	{
		(void)e;	// C4100
		const char* st=s;
		while(isalnum(*st) || *st == '_')
			st++;
		string vName = std::string(s, st);

		int i = (int)varInfo.size()-1;
		while(i >= 0 && varInfo[i].name != vName)
			i--;
		if(i == -1)
		{
			ColorCode(255, 0, 0, 0, 0, 1, s, st);
			//logStream << "ERROR: variable '" << vName << "' is not defined\r\n";
			return;
		}
		if(varInfo[i].isConst)
		{
			ColorCode(255, 0, 0, 0, 0, 1, s, st);
			logStream << "ERROR: cannot change constant parameter '" << vName << "'\r\n";
			return;
		}
	}

	void GetVar(char const* s, char const* e)
	{
		(void)e;	// C4100
		const char* st=s;
		while(isalnum(*st) || *st == '_')
			st++;
		string vName = std::string(s, st);

		int i = (int)varInfo.size()-1;
		while(i >= 0 && varInfo[i].name != vName)
			i--;
		if(i == -1)
		{
			i = (int)funcs.size()-1;
			while(i >= 0 && funcs[i]->name != vName)
				i--;
		}
		if(i == -1)
		{
			ColorCode(255,0,0,0,0,1,s,st);
			logStream << "ERROR: variable '" << vName << "' is not defined\r\n";
			return;
		}
	}

	void FuncAdd(char const* s, char const* e)
	{
		string vName = tempStr;
		for(unsigned int i = varInfoTop.back().activeVarCnt; i < varInfo.size(); i++)
			if(varInfo[i].name == vName)
			{
				ColorCode(255, 0, 0, 0, 0, 1, s, e);
				logStream << "ERROR: Name '" << vName << "' is already taken for a variable in current scope\r\n";
				return;
			}
			if(CheckIfDeclared(vName))
			{
				ColorCode(255, 0, 0, 0, 0, 1, s, e);
				return;
			}
			funcs.push_back(new FunctionInfo());
			funcs.back()->name = vName;
	}

	void FuncEnd(char const* s, char const* e)
	{
		(void)s; (void)e;	// C4100
		if(funcs.empty())
			return;

		funcs.back()->params.clear();
		for(unsigned int i = 0; i < callArgCount.back(); i++)
			funcs.back()->params.push_back(VariableInfo("param", 0, NULL));
		callArgCount.pop_back();
	}

	void FuncCall(char const* s, char const* e)
	{
		const char* st=s;
		while(isalnum(*st) || *st == '_')
			st++;
		string fname = std::string(s, st);

		//Find function
		bool foundFunction = false;
		bool funcPtr = false;
		int i = (int)funcs.size()-1;
		while(true)
		{
			while(i >= 0 && funcs[i]->name != fname)
				i--;
			if(i == -1)
			{
				i = (int)varInfo.size()-1;
				while(i >= 0 && varInfo[i].name != fname)
					i--;
				if(i != -1)
					funcPtr = true;
			}
			if(i == -1)
			{
				if(!foundFunction)
				{
					ColorCode(255,0,0,0,0,1,s,st);
					logStream << "ERROR: function '" << fname << "' is undefined\r\n";
				}else{
					ColorCode(255,0,0,0,0,1,s,e);
					logStream << "ERROR: none of the functions '" << fname << "' takes " << (unsigned int)callArgCount.back() << " arguments\r\n";
				}
				break;
			}
			foundFunction = true;
			if(funcPtr || funcs[i]->params.size() == callArgCount.back())
				break;
			i--;
		}
		callArgCount.pop_back();
	}

	std::string newType;
	void StartType(char const* s, char const* e)
	{
		newType = std::string(s, e);
	}
	void AddType(char const* s, char const* e)
	{
		(void)s; (void)e;	// C4100
		typeInfo.push_back(newType);
	}

	void OnError(char const* s, char const* e)
	{
		if(s == e)
			e++;
		if(logStr.length() != 0)
			ColorCode(255,0,0,0,0,1,s,e);
		logStr = "";
	}
	void BlockBegin(char const* s, char const* e)
	{
		(void)s; (void)e;	// C4100
		varInfoTop.push_back(VarTopInfo((unsigned int)varInfo.size(), varTop));
	}
	void BlockEnd(char const* s, char const* e)
	{
		(void)s; (void)e;	// C4100
		while(varInfo.size() > varInfoTop.back().activeVarCnt)
		{
			varTop--;// -= varInfo.back().count;
			varInfo.pop_back();
		}
		varInfoTop.pop_back();
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

Colorer::Colorer(HWND rich): richEdit(rich)
{
	syntax = new ColorerGrammar::Grammar();

	strBuf = new char[400000];//Should be enough

	ColorerGrammar::ColorRWord	= ColorCodeCallback(this, 0,0,255,0,0,0);
	ColorerGrammar::ColorVar	= ColorCodeCallback(this, 128,128,128,0,0,0);
	ColorerGrammar::ColorVarDef	= ColorCodeCallback(this, 50,50,50,0,0,0);
	ColorerGrammar::ColorFunc	= ColorCodeCallback(this, 136,0,0,0,0,0);
	ColorerGrammar::ColorText	= ColorCodeCallback(this, 0,0,0,0,0,0);
	ColorerGrammar::ColorBold	= ColorCodeCallback(this, 0,0,0,1,0,0);
	ColorerGrammar::ColorReal	= ColorCodeCallback(this, 0,150,0,0,0,0);
	ColorerGrammar::ColorInt	= ColorCodeCallback(this, 0,150,0,0,1,0);
	ColorerGrammar::ColorErr	= ColorCodeCallback(this, 255,0,0,0,0,1);
	ColorerGrammar::ColorComment= ColorCodeCallback(this, 255,0,255,0,0,0);

	ColorerGrammar::ColorCode	= ColorCodeCallback(this, 0, 0, 0, 0, 0, 0);

	syntax->InitGrammar();
}
Colorer::~Colorer()
{
	delete[] strBuf;

	syntax->DeInitGrammar();
	delete syntax;
}

bool Colorer::ColorText()
{
	lastError = "";
	ColorerGrammar::varInfoTop.clear();
	ColorerGrammar::varInfo.clear();

	unsigned int oldFuncCount = 0;
	ColorerGrammar::typeInfo.clear();

	ColorerGrammar::typeInfo.push_back("void");
	ColorerGrammar::typeInfo.push_back("char");
	ColorerGrammar::typeInfo.push_back("short");
	ColorerGrammar::typeInfo.push_back("int");
	ColorerGrammar::typeInfo.push_back("long");
	ColorerGrammar::typeInfo.push_back("float");
	ColorerGrammar::typeInfo.push_back("double");
	ColorerGrammar::typeInfo.push_back("float2");
	ColorerGrammar::typeInfo.push_back("float3");
	ColorerGrammar::typeInfo.push_back("float4");
	ColorerGrammar::typeInfo.push_back("float4x4");

	ColorerGrammar::varInfoTop.push_back(VarTopInfo(0,0));

	ColorerGrammar::callArgCount.clear();
	ColorerGrammar::varSize = 1;
	ColorerGrammar::varTop = 3;

	ColorerGrammar::logStream.str("");

	ColorerGrammar::codeStart = strBuf;

	memset(strBuf, 0, GetWindowTextLength(richEdit)+5);
	GetWindowText(richEdit, strBuf, 400000);

	errUnderline = false;
	ColorCode(255,0,0,0,0,0, strBuf, strBuf+strlen(strBuf));

	ParseResult pRes = Parse(syntax->code, strBuf, syntax->mySpaceP);
	if(pRes == PARSE_ABORTED)
	{
		lastError = ColorerGrammar::lastError;
		return false;
	}
	if(pRes != PARSE_OK)
	{
		lastError = "Syntax error";
		return false;
	}

	for(unsigned int i = oldFuncCount; i < ColorerGrammar::funcs.size(); i++)
		delete ColorerGrammar::funcs[i];
	ColorerGrammar::funcs.clear();

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

void Colorer::ColorCode(int red, int green, int blue, int bold, int ital, int under, const char* start, const char* end)
{
	if(errUnderline)
	{
		red=255;
		green=blue=0;
		bold=ital=0;
		under=1;
		errUnderline = 0;
	}
	//logstr << richEdit << " " << red << " " << green << " " << blue << " " << std::string(start, end) << "\r\n";
	CHARFORMAT2 cf;
	ZeroMemory(&cf, sizeof(CHARFORMAT2));
	cf.cbSize = sizeof(CHARFORMAT2);
	cf.dwMask = CFM_BOLD | CFM_COLOR | CFM_FACE | CFM_ITALIC | CFM_UNDERLINE;
	cf.dwEffects = CFE_BOLD * bold | CFE_ITALIC * ital | CFE_UNDERLINE * under;
	cf.crTextColor = RGB(red,green,blue);
	cf.bCharSet = ANSI_CHARSET;
	cf.bPitchAndFamily = DEFAULT_PITCH;
	cf.bUnderlineType = CFU_UNDERLINEDOUBLE;
	
	strcpy(cf.szFaceName, "Courier New");
	Edit_SetSel(richEdit, start-strBuf, end-strBuf);
	SendMessage(richEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}
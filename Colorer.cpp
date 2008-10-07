#include "stdafx.h"
#include "SupSpi.h"
using namespace supspi;
#include "richedit.h"
#include <windowsx.h>

#include "ParseClass.h"
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
	// Parsing rules
	Rule expr, block, funcdef, breakExpr, ifExpr, forExpr, returnExpr, vardef, vardefsub, whileExpr, dowhileExpr, switchExpr;
	Rule term5, term4_9, term4_6, term4_4, term4_2, term4_1, term4, term3, term2, term1, group, funccall, funcvars;
	Rule appval, varname, comment, symb, symb2, constExpr, addvarp, typeExpr;
	// Main rule and space parsers
	Rule code;
	Rule mySpaceP;

	// Temporary variables
	UINT	varSize, varTop;
	bool	currValConst;
	std::string	logStr;

	std::vector<FunctionInfo>	funcs;
	std::vector<std::string>	typeInfo;
	std::vector<VariableInfo>	varInfo;
	std::vector<VarTopInfo>		varInfoTop;
	std::vector<UINT>			callArgCount;

	// Callbacks
	ColorCodeCallback ColorRWord, ColorVar, ColorVarDef, ColorFunc, ColorText, ColorReal, ColorInt, ColorBold, ColorErr, ColorComment;
	ColorCodeCallback ColorCode;

	//Error log
	ostringstream logStream;

	std::string tempStr;
	void SetTempStr(char const* s, char const* e)
	{
		tempStr.assign(s, e);
	}

	class TypeNameP: public BaseP
	{
	public:
		TypeNameP(Rule a){ m_a.set(a); }
		virtual ~TypeNameP(){}

		virtual bool	Parse(char** str, shared_ptr<BaseP> space)
		{
			SkipSpaces(str, space);
			char* curr = *str;
			m_a->Parse(str, shared_ptr<BaseP>((BaseP*)NULL));
			if(curr == *str)
				return false;
			std::string type(curr, *str);
			for(UINT i = 0; i < typeInfo.size(); i++)
				if(typeInfo[i] == type)
					return true;
			return false;
		}
	protected:
		Rule m_a;
	};
	Rule	typenameP(Rule a){ return Rule(shared_ptr<BaseP>(new TypeNameP(a))); }

	Rule	strWP(char* str){ return (lexemeD[strP(str) >> (epsP - alnumP)]); }
	void	InitGrammar()
	{
		constExpr		=	epsP[AssignVar<bool>(currValConst, false)] >>
			!strP("const")[ColorRWord][AssignVar<bool>(currValConst, true)];
		symb		=	graphP - alnumP - chP(')');
		symb2		=	graphP - alphaP;
		varname		=	lexemeD[alphaP >> *alnumP];

		typeExpr	=	typenameP(varname)[ColorRWord];

		funccall	=	varname[ColorFunc] >> 
			strP("(")[ColorBold][PushBackVal<std::vector<UINT>, UINT>(callArgCount, 0)] >>
			!(
			term5[ArrBackInc<std::vector<UINT> >(callArgCount)] >>
			*(
			strP(",")[ColorText] >> 
			(term5[ArrBackInc<std::vector<UINT> >(callArgCount)] | epsP[AssignVar<string>(logStr, "ERROR: unexpected symbol after ','")][LogStr])
			)[OnError]
			) >>
			strP(")")[ColorBold];
		funcvars	=	!(typeExpr >> constExpr >> !strP("ref")[ColorRWord] >> varname[ColorVar][AddVar][ArrBackInc<std::vector<UINT> >(callArgCount)]) >>
			*(
			strP(",")[ColorText] >>
			(typeExpr >> constExpr >> !strP("ref")[ColorRWord] >> varname[ColorVar][AddVar][ArrBackInc<std::vector<UINT> >(callArgCount)] | epsP[AssignVar<string>(logStr, "ERROR: parameter name expected after ','")][LogStr])
			)[OnError];
		funcdef		=
			typeExpr >>
			varname[ColorFunc][SetTempStr] >>
			chP('(')[ColorBold][PushBackVal<std::vector<UINT>, UINT>(callArgCount, 0)][FuncAdd][BlockBegin] >>
			((*(symb | digitP))[ColorErr] >> funcvars) >>
			chP(')')[ColorBold][FuncEnd] >>
			chP('{')[ColorBold] >>
			code >>
			chP('}')[ColorBold][BlockEnd];

		appval		=
			(
				(varname - strP("case"))[ColorVar] >> ~chP('(') >>
				!(
					chP('[')[ColorText] >> 
					term5 >> 
					chP(']')[ColorText]
				) >>
				*(
					(chP('.') | strP("->"))[ColorText] >>
					varname[ColorVar] >>
					!(
						chP('[')[ColorText] >>
						term5 >>
						chP(']')[ColorText]
					)
				)
			);
		addvarp		=
			(
			varname[ColorVarDef] >> epsP[AssignVar<UINT>(varSize,1)] >> 
			!(chP('[')[ColorText] >> intP[StrToInt(varSize)][ColorInt] >> chP(']')[ColorText])
			)[AddVar] >>
			((chP('=')[ColorText] >> term5) | epsP);
		vardefsub	=	*strP("ref")[ColorRWord] >> addvarp >> *(chP(',')[ColorText] >> vardefsub);
		vardef		=
			typeExpr >>
			constExpr >>
			vardefsub;

		ifExpr			=	strWP("if")[ColorRWord] >> (('(' >> epsP)[ColorText] >> term5 >> (')' >> epsP)[ColorText]) >> expr >> ((strP("else")[ColorRWord] >> expr) | epsP);
		forExpr			=	strWP("for")[ColorRWord] >> ('(' >> epsP)[ColorText] >> (vardef | term5 | block) >> (';' >> epsP)[ColorText] >> term5 >> (';' >> epsP)[ColorText] >> (term5 | block) >> (')' >> epsP)[ColorText] >> expr;
		whileExpr		=	strWP("while")[ColorRWord] >> (('(' >> epsP)[ColorText] >> term5 >> (')' >> epsP)[ColorText]) >> expr;
		dowhileExpr		=	strWP("do")[ColorRWord] >> expr >> strP("while")[ColorRWord] >> ('(' >> epsP)[ColorText] >> term5 >> (')' >> epsP)[ColorText] >> (';' >> epsP)[ColorText];
		switchExpr		=	strWP("switch")[ColorRWord] >> ('(' >> epsP)[ColorText] >> term5 >> (')' >> epsP)[ColorText] >> ('{' >> epsP)[ColorBold] >> 
			(strWP("case")[ColorRWord] >> term5 >> (':' >> epsP)[ColorText] >> expr >> *expr) >>
			*(strWP("case")[ColorRWord] >> term5 >> (':' >> epsP)[ColorText] >> expr >> *expr) >>
			('}' >> epsP)[ColorBold];
		returnExpr		=	strWP("return")[ColorRWord] >> term5 >> +(';' >> epsP)[ColorBold];
		breakExpr		=	strWP("break")[ColorRWord] >> +(';' >> epsP)[ColorBold];

		group		=	chP('(')[ColorText] >> term5 >> chP(')')[ColorText];
		term1		=
			(chP('&')[ColorText] >> appval) |
			((strP("--") | strP("++"))[ColorText] >> appval[GetVar]) | 
			(+chP('-')[ColorText] >> term1) | (+chP('+')[ColorText] >> term1) | ((chP('!') | '~')[ColorText] >> term1) |
			longestD[(intP >> (chP('l') | epsP)) | (realP >> (chP('f') | epsP))][ColorReal] |
			group | funccall[FuncCall] |
			(!chP('*')[ColorText] >> appval[GetVar] >> strP("++")[ColorText]) |
			(!chP('*')[ColorText] >> appval[GetVar] >> strP("--")[ColorText]) |
			!chP('*')[ColorText] >> appval[GetVar];
		term2	=	term1 >> *(strP("**")[ColorText] >> term1);
		term3	=	term2 >> *((chP('*') | chP('/') | chP('%'))[ColorText] >> term2);
		term4	=	term3 >> *((chP('+') | chP('-'))[ColorText] >> term3);
		term4_1	=	term4 >> *((strP("<<") | strP(">>"))[ColorText] >> term4);
		term4_2	=	term4_1 >> *((strP("<=") | strP(">=") | chP('<') | chP('>'))[ColorText] >> term4_1);
		term4_4	=	term4_2 >> *((strP("==") | strP("!="))[ColorText] >> term4_2);
		term4_6	=	term4_4 >> *((strP("&") | strP("|") | strP("^") | strP("and") | strP("or") | strP("xor"))[ColorText] >> term4_4);
		term4_9	=	term4_6 >> !(chP('?')[ColorText] >> term5 >> chP(':')[ColorText] >> term5);
		term5	=	(!chP('*')[ColorText] >> appval[SetVar] >> (strP("=") | strP("+=") | strP("-=") | strP("*=") | strP("/=") | strP("^="))[ColorText] >> term5) | term4_9;

		block	=	chP('{')[ColorBold][BlockBegin] >> code >> chP('}')[ColorBold][BlockEnd];
		expr	=	*chP(';')[ColorText] >> ((vardef >> (';' >> epsP)[ColorText]) | breakExpr | ifExpr | forExpr | whileExpr | dowhileExpr | switchExpr | returnExpr | (term5 >> +(';' >> epsP)[ColorText]) | block);
		code	=	*(funcdef | expr);

		mySpaceP = spaceP | ((strP("//") >> *(anycharP - eolP)) | (strP("/*") >> *(anycharP - strP("*/")) >> strP("*/")))[ColorComment];
	}

	void CheckIfDeclared(const std::string& str, bool forFunction = false)
	{
		if(str == "if" || str == "else" || str == "for" || str == "while" || str == "var" || str == "func" || str == "return" || str=="switch" || str=="case")
			throw std::string("ERROR: The name '" + str + "' is reserved");
		if(!forFunction)
			for(UINT i = 0; i < funcs.size(); i++)
				if(funcs[i].name == str)
					throw std::string("ERROR: Name '" + str + "' is already taken for a function");
	}
	void AddVar(char const* s, char const* e)
	{
		const char* st=s;
		while(isalnum(*st))
			st++;
		string vName = std::string(s, st);

		for(UINT i = varInfoTop.back().activeVarCnt; i < varInfo.size(); i++){
			if(varInfo[i].name == vName){
				ColorCode(255,0,0,0,0,1,s,st);
				logStream << "ERROR: Name '" << vName << "' is already taken for a variable in current scope\r\n";
				return;
			}
		}
		try{
			CheckIfDeclared(vName);
		}catch(const std::string& str){
			ColorCode(255,0,0,0,0,1,s,st);
			logStream << str << "\r\n";
			return;
		}

		varInfo.push_back(VariableInfo(vName, 0, NULL, varSize, currValConst));
		varSize = 1;
	}

	void SetVar(char const* s, char const* e)
	{
		const char* st=s;
		while(isalnum(*st))
			st++;
		string vName = std::string(s, st);
		size_t braceInd = std::string(s, e).find('[');

		int i = (int)varInfo.size()-1;
		while(i >= 0 && varInfo[i].name != vName)
			i--;
		if(i == -1)
		{
			ColorCode(255,0,0,0,0,1,s,st);
			logStream << "ERROR: variable '" << vName << "' is not defined\r\n";
			return;
		}
		if(varInfo[i].isConst)
		{
			ColorCode(255,0,0,0,0,1,s,st);
			logStream << "ERROR: cannot change constant parameter '" << vName << "'\r\n";
			return;
		}
		/*if((braceInd == -1) && varInfo[i].count != 1)
		{
			ColorCode(255,0,0,0,0,1,s,e);
			logStream << "ERROR: variable '" << vName << "' is an array, but no index specified\r\n";
			return;
		}*/
	}

	void GetVar(char const* s, char const* e)
	{
		const char* st=s;
		while(isalnum(*st))
			st++;
		string vName = std::string(s, st);
		size_t braceInd = std::string(s, e).find('[');

		int i = (int)varInfo.size()-1;
		while(i >= 0 && varInfo[i].name != vName)
			i--;
		if(i == -1)
		{
			ColorCode(255,0,0,0,0,1,s,st);
			logStream << "ERROR: variable '" << vName << "' is not defined\r\n";
			return;
		}
		/*if((braceInd != -1) && varInfo[i].count == 1)
		{
			ColorCode(255,0,0,0,0,1,s,e);
			logStream << "ERROR: variable '" << vName << "' is not array\r\n";
			return;
		}
		if((braceInd == -1) && varInfo[i].count != 1)
		{
			ColorCode(255,0,0,0,0,1,s,e);
			logStream << "ERROR: variable '" << vName << "' is an array, but no index specified\r\n";
			return;
		}*/
	}

	void FuncAdd(char const* s, char const* e)
	{
		string vName = tempStr;
		for(UINT i = varInfoTop.back().activeVarCnt; i < varInfo.size(); i++)
			if(varInfo[i].name == vName)
			{
				ColorCode(255,0,0,0,0,1,s,e);
				logStream << "ERROR: Name '" << vName << "' is already taken for a variable in current scope\r\n";
				return;
			}
			try{
				CheckIfDeclared(vName, true);
			}catch(const std::string& str){
				ColorCode(255,0,0,0,0,1,s,e);
				logStream << str << "\r\n";
				return;
			}
			funcs.push_back(FunctionInfo());
			funcs.back().name = vName;
	}

	void FuncEnd(char const* s, char const* e)
	{
		funcs.back().params.clear();
		for(UINT i = 0; i < callArgCount.back(); i++)
			funcs.back().params.push_back(VariableInfo("param", 0, NULL));
		callArgCount.pop_back();
	}

	void FuncCall(char const* s, char const* e)
	{
		const char* st=s;
		while(isalnum(*st))
			st++;
		string fname = std::string(s, st);

		//Find standard function
		if(fname == "cos" || fname == "sin" || fname == "tan" || fname == "ctg" || fname == "ceil" || fname == "floor" || 
			fname == "sqrt" || fname == "clock")
		{
			if(fname == "clock" && callArgCount.back() != 0)
			{
				ColorCode(255,0,0,0,0,1,s,e);
				logStream << "ERROR: function '" << fname << "' takes no arguments\r\n";
				return;
			}
			if(fname != "clock" && callArgCount.back() != 1)
			{
				ColorCode(255,0,0,0,0,1,s,e);
				logStream << "ERROR: function '" << fname << "' takes one argument\r\n";
				return;
			}
		}else{	//Find user-defined function
			bool foundFunction = false;
			int i = (int)funcs.size()-1;
			while(true)
			{
				while(i >= 0 && funcs[i].name != fname)
					i--;
				if(i == -1)
				{
					if(!foundFunction)
					{
						ColorCode(255,0,0,0,0,1,s,st);
						logStream << "ERROR: function '" << fname << "' is undefined\r\n";
					}else{
						ColorCode(255,0,0,0,0,1,s,e);
						logStream << "ERROR: none of the functions '" << fname << "' takes " << (UINT)callArgCount.back() << " arguments\r\n";
					}
					break;
				}
				foundFunction = true;
				if(funcs[i].params.size() == callArgCount.back())
					break;
				i--;
			}
		}
		callArgCount.pop_back();
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
		varInfoTop.push_back(VarTopInfo((UINT)varInfo.size(), varTop));
	}
	void BlockEnd(char const* s, char const* e)
	{
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
		logStream << logStr << "\r\n";
	}
};

Colorer::Colorer(HWND rich): richEdit(rich)
{
	strBuf = new char[400000];//Should be enough
}
Colorer::~Colorer()
{
}

void Colorer::InitParser()
{
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

	ColorerGrammar::InitGrammar();
}
void Colorer::ColorText()
{
	ColorerGrammar::varInfoTop.clear();
	ColorerGrammar::varInfo.clear();
	ColorerGrammar::funcs.clear();
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

	ColorerGrammar::varInfo.push_back(VariableInfo("ERROR", 0, typeDouble));
	ColorerGrammar::varInfo.push_back(VariableInfo("pi", 1, typeDouble));
	ColorerGrammar::varInfo.push_back(VariableInfo("e", 2, typeDouble));

	ColorerGrammar::varInfoTop.push_back(VarTopInfo(0,0));

	ColorerGrammar::callArgCount.clear();
	ColorerGrammar::varSize = 1;
	ColorerGrammar::varTop = 3;

	ColorerGrammar::logStream.str("");

	memset(strBuf, 0, GetWindowTextLength(richEdit)+5);
	GetWindowText(richEdit, strBuf, 400000);

	errUnderline = false;
	ColorCode(255,0,0,0,0,0, strBuf, strBuf+strlen(strBuf));

	if(!Parse(ColorerGrammar::code, strBuf, ColorerGrammar::mySpaceP))
		throw std::string("Syntax error");
	//char* ptr = m_buf;
	//if(!m_data->code->Parse(&ptr, m_data->mySpaceP.getParser()))
	//	throw std::string("Syntax error");
	if(ColorerGrammar::logStream.str().length() != 0)
		throw ColorerGrammar::logStream.str();
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
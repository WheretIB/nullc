#include "stdafx.h"
using boost::bind;
#include "SupSpi.h"
using namespace supspi;
#include "richedit.h"
#include <windowsx.h>

#include "ParseClass.h"
#include "Colorer.h"

struct ColorerGrammar
{
	Rule	strWP(char* str){ return (lexemeD[strP(str) >> (epsP - alnumP)]); }
	void	InitGrammar()
	{
		AddVar	=	bind(&ColorerGrammar::ColAddVar, this, _1, _2);
		AddRef	=	bind(&ColorerGrammar::ColAddRef, this, _1, _2);
		SetVar	=	bind(&ColorerGrammar::ColSetVar, this, _1, _2);
		GetVar	=	bind(&ColorerGrammar::ColGetVar, this, _1, _2);
		FuncAdd	=	bind(&ColorerGrammar::ColFuncAdd, this, _1, _2);
		FuncEnd	=	bind(&ColorerGrammar::ColFuncEnd, this, _1, _2);
		FuncCall=	bind(&ColorerGrammar::ColFuncCall, this, _1, _2);
		ColorOnErr=	bind(&ColorerGrammar::ColOnErr, this, _1, _2);
		BlockBegin=	bind(&ColorerGrammar::ColBlockBegin, this, _1, _2);
		BlockEnd=	bind(&ColorerGrammar::ColBlockEnd, this, _1, _2);

		col_type		=	(strP("float4") | strP("float3") | strP("float2") | strP("double") | strP("long") | strP("float") | strP("int") | strP("short") | strP("char") | epsP)[ColorRWord];
		col_const		=	epsP[AssignVar<bool>(currValConst, false)] >>
			!strP("const")[ColorRWord][AssignVar<bool>(currValConst, true)];
		col_symb		=	graphP - alnumP;
		col_symb2		=	graphP - alphaP;
		col_varname		=	lexemeD[alphaP >> *alnumP];

		col_funccall	=	col_varname[ColorFunc] >> 
			strP("(")[ColorBold][PushBackVal<std::vector<UINT>, UINT>(callArgCount, 0)] >>
			!(
			col_term5[ArrBackInc<std::vector<UINT> >(callArgCount)] >>
			*(
			strP(",")[ColorText] >> 
			(col_term5[ArrBackInc<std::vector<UINT> >(callArgCount)] | epsP[AssignVar<string>(logStr, "ERROR: unexpected symbol after ','")][PLog])
			)[ColorOnErr]
			) >>
			strP(")")[ColorBold];
		col_funcvars	=	!(col_type >> col_const >> col_varname[ColorVar][AddVar][ArrBackInc<std::vector<UINT> >(callArgCount)]) >>
			*(
			strP(",")[ColorText] >>
			(col_type >> col_const >> col_varname[ColorVar][AddVar][ArrBackInc<std::vector<UINT> >(callArgCount)] | epsP[AssignVar<string>(logStr, "ERROR: parameter name expected after ','")][PLog])
			)[ColorOnErr];
		col_funcdef		=
			strP("func")[ColorRWord][PushBackVal<std::vector<UINT>, UINT>(callArgCount, 0)] >> 
			col_type >>
			(col_varname[ColorFunc][FuncAdd][BlockBegin] >>
			(('(' >> epsP)[ColorBold] | epsP[AssignVar<string>(logStr, "ERROR: function name must be followed by '('")][PLog]))[ColorOnErr] >>
			((*(col_symb | digitP))[ColorErr] >> col_funcvars) >>
			chP(')')[ColorBold][FuncEnd] >>
			chP('{')[ColorBold] >>
			col_code >>
			chP('}')[ColorBold][BlockEnd];

		col_appval		=	(col_varname[ColorVar] >> 
			!(
			chP('[')[ColorText] >> 
			col_term5 >> 
			chP(']')[ColorText]
			));
		col_addvarp		=
			(
			col_varname[ColorVarDef] >> epsP[AssignVar<UINT>(varSize,1)] >> 
			!(chP('[')[ColorText] >> intP[StrToInt(varSize)][ColorInt] >> chP(']')[ColorText])
			)[AddVar] >>
			((chP('=')[ColorText] >> col_term5) | epsP);
		col_addrefp		=
			(
			col_varname[ColorVarDef] >> epsP[AssignVar<UINT>(varSize,1)] >>
			chP('=')[ColorText] >> 
			col_varname[ColorVar] >>
			!(chP('[')[ColorText] >> intP[StrToInt(varSize)][ColorInt] >> chP(']')[ColorText])
			)[AddRef];
		col_vardef		=
			col_type >>
			col_const >>
			((strP("ref")[ColorRWord] >> col_addrefp) | col_addvarp) >>
			*(chP(',')[ColorText] >> col_vardef);

		col_if			=	strWP("if")[ColorRWord] >> (('(' >> epsP)[ColorText] >> col_term5 >> (')' >> epsP)[ColorText]) >> col_expr >> ((strP("else")[ColorRWord] >> col_expr) | epsP);
		col_for			=	strWP("for")[ColorRWord] >> ('(' >> epsP)[ColorText] >> ((strP("var")[ColorRWord] >> col_vardef) | col_term5 | col_block) >> (';' >> epsP)[ColorText] >> col_term5 >> (';' >> epsP)[ColorText] >> (col_term5 | col_block) >> (')' >> epsP)[ColorText] >> col_expr;
		col_while		=	strWP("while")[ColorRWord] >> (('(' >> epsP)[ColorText] >> col_term5 >> (')' >> epsP)[ColorText]) >> col_expr;
		col_dowhile		=	strWP("do")[ColorRWord] >> col_expr >> strP("while")[ColorRWord] >> ('(' >> epsP)[ColorText] >> col_term5 >> (')' >> epsP)[ColorText] >> (';' >> epsP)[ColorText];
		col_switch		=	strWP("switch")[ColorRWord] >> ('(' >> epsP)[ColorText] >> col_term5 >> (')' >> epsP)[ColorText] >> ('{' >> epsP)[ColorBold] >> 
			(strWP("case")[ColorRWord] >> col_term5 >> (':' >> epsP)[ColorText] >> col_expr >> *col_expr) >>
			*(strWP("case")[ColorRWord] >> col_term5 >> (':' >> epsP)[ColorText] >> col_expr >> *col_expr) >>
			('}' >> epsP)[ColorBold];
		col_return		=	strWP("return")[ColorRWord] >> col_term5 >> +(';' >> epsP)[ColorBold];
		col_break		=	strWP("break")[ColorRWord] >> +(';' >> epsP)[ColorBold];

		col_group		=	chP('(')[ColorText] >> col_term5 >> chP(')')[ColorText];
		col_term1		=	((strP("--") | strP("++"))[ColorText] >> col_appval[GetVar]) | 
			(+chP('-')[ColorText] >> col_term1) | (+chP('+')[ColorText] >> col_term1) | ((chP('!') | '~')[ColorText] >> col_term1) |
			//(realP)[ColorReal] |
			longestD[(intP >> (chP('l') | epsP)) | (realP >> (chP('f') | epsP))][ColorReal] |
			col_group | col_funccall[FuncCall] |
			(col_appval >> strP("++")[ColorText])[GetVar] |
			(col_appval >> strP("--")[ColorText])[GetVar] |
			col_appval[GetVar];
		col_term2	=	col_term1 >> *(strP("**")[ColorText] >> col_term1);
		col_term3	=	col_term2 >> *((chP('*') | chP('/') | chP('%'))[ColorText] >> col_term2);
		col_term4	=	col_term3 >> *((chP('+') | chP('-'))[ColorText] >> col_term3);
		col_term4_1	=	col_term4 >> *((strP("<<") | strP(">>"))[ColorText] >> col_term4);
		col_term4_2	=	col_term4_1 >> *((strP("<=") | strP(">=") | chP('<') | chP('>'))[ColorText] >> col_term4_1);
		col_term4_4	=	col_term4_2 >> *((strP("==") | strP("!="))[ColorText] >> col_term4_2);
		col_term4_6	=	col_term4_4 >> *((strP("&") | strP("|") | strP("^") | strP("and") | strP("or") | strP("xor"))[ColorText] >> col_term4_4);
		col_term4_9	=	col_term4_6 >> !(chP('?')[ColorText] >> col_term5 >> chP(':')[ColorText] >> col_term5);
		col_term5	=	(col_appval >> (strP("=") | strP("+=") | strP("-=") | strP("*=") | strP("/=") | strP("^="))[ColorText] >> col_term5)[SetVar] | col_term4_9;

		col_block	=	chP('{')[ColorBold][BlockBegin] >> col_code >> chP('}')[ColorBold][BlockEnd];
		col_expr	=	*chP(';')[ColorText] >> ((strWP("var")[ColorRWord] >> col_vardef >> (';' >> epsP)[ColorText]) | col_break | col_if | col_for | col_while | col_dowhile | col_switch | col_return | (col_term5 >> +(';' >> epsP)[ColorText]) | col_block);
		col_code	=	*(col_funcdef | col_expr);

		mySpaceP = spaceP | ((strP("//") >> *(anycharP - eolP)) | (strP("/*") >> *(anycharP - strP("*/")) >> strP("*/")))[ColorComment];
	}

	void CheckIfDeclared(const std::string& str)
	{
		if(str == "if" || str == "else" || str == "for" || str == "while" || str == "var" || str == "func" || str == "return" || str=="switch" || str=="case")
			throw std::string("ERROR: The name '" + str + "' is reserved");
		for(UINT i = 0; i < funcs.size(); i++)
			if(funcs[i].name == str)
				throw std::string("ERROR: Name '" + str + "' is already taken for a function");
	}
	void ColAddVar(char const* s, char const* e)
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

		if(varSize > 128000){
			ColorCode(255,0,0,0,0,1,st,e);
			logStream << "ERROR: variable '" << vName << "' has to big length (>128000)\r\n";
			return;
		}
		varInfo.push_back(VariableInfo(vName, 0, NULL, varSize, currValConst));
		varSize = 1;
	}

	void ColAddRef(char const* s, char const* e)
	{
		const char *st=s, *sb, *sc;
		while(isalnum(*st))
			st++;
		sb = st;
		while(!isalnum(*sb))
			sb++;
		sc = sb;
		while(isalnum(*sc))
			sc++;
		string vRefName = std::string(s, st);
		string vVarName = std::string(sb, sc);

		for(UINT i = varInfoTop.back().activeVarCnt; i < varInfo.size(); i++){
			if(varInfo[i].name == vRefName){
				ColorCode(255,0,0,0,0,1,s,st);
				logStream << "ERROR: Name '" << vRefName << "' is already taken for a variable in current scope\r\n";
				return;
			}
		}
		try{
			CheckIfDeclared(vRefName);
		}catch(const std::string& str){
			ColorCode(255,0,0,0,0,1,s,st);
			logStream << str << "\r\n";
			return;
		}

		int index = (int)varInfo.size()-1;
		while(index >= 0 && varInfo[i].name != vVarName)
			index--;
		if(index == -1){
			ColorCode(255,0,0,0,0,1,sb,sc);
			logStream << "ERROR: variable '" << vVarName << "' is not defined\r\n";
			return;
		}
		if(!currValConst && varInfo[index].isConst)
		{
			ColorCode(255,0,0,0,0,1,s,sc);
			logStream << "ERROR: cannot remove constant flag of variable '" << vVarName << "'. Use 'const ref'\r\n";
			return;
		}
		varInfo.push_back(VariableInfo(vRefName, 0, 0, currValConst, true));
	}

	void ColSetVar(char const* s, char const* e)
	{
		const char* st=s;
		while(isalnum(*st))
			st++;
		string vName = std::string(s, st);
		size_t braceInd = std::string(s, e).find('[');

		int i = (int)varInfo.size()-1;
		while(i >= 0 && varInfo[i].name != vName)
			i--;
		if(i == -1){
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
		if((braceInd == -1) && varInfo[i].count != 1)
		{
			ColorCode(255,0,0,0,0,1,s,e);
			logStream << "ERROR: variable '" << vName << "' is an array, but no index specified\r\n";
			return;
		}
	}

	void ColGetVar(char const* s, char const* e)
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
		if((braceInd != -1) && varInfo[i].count == 1)
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
		}
	}

	void ColFuncAdd(char const* s, char const* e)
	{
		string vName = std::string(s, e);
		for(UINT i = varInfoTop.back().activeVarCnt; i < varInfo.size(); i++)
			if(varInfo[i].name == vName)
			{
				ColorCode(255,0,0,0,0,1,s,e);
				logStream << "ERROR: Name '" << vName << "' is already taken for a variable in current scope\r\n";
				return;
			}
			try{
				CheckIfDeclared(vName);
			}catch(const std::string& str){
				ColorCode(255,0,0,0,0,1,s,e);
				logStream << str << "\r\n";
				return;
			}
			funcs.push_back(FunctionInfo());
			funcs.back().name = vName;
	}

	void ColFuncEnd(char const* s, char const* e)
	{
		funcs.back().params.clear();
		for(UINT i = 0; i < callArgCount.back(); i++)
			funcs.back().params.push_back(VariableInfo("param", 0, NULL));
		callArgCount.pop_back();
	}

	void ColFuncCall(char const* s, char const* e)
	{
		const char* st=s;
		while(isalnum(*st))
			st++;
		string fname = std::string(s, st);

		//Find standard function
		if(fname == "cos" || fname == "sin" || fname == "tan" || fname == "ctg" || fname == "ceil" || fname == "floor" || 
			fname == "sqrt" || fname == "clock")
		{
			if(fname == "clock" && callArgCount.back() != 0){
				ColorCode(255,0,0,0,0,1,s,e);
				logStream << "ERROR: function '" << fname << "' takes no arguments\r\n";
				return;
			}
			if(fname != "clock" && callArgCount.back() != 1){
				ColorCode(255,0,0,0,0,1,s,e);
				logStream << "ERROR: function '" << fname << "' takes one argument\r\n";
				return;
			}
		}else{	//Find user-defined function
			int i = (int)funcs.size()-1;
			while(i >= 0 && funcs[i].name != fname)
				i--;
			if(i == -1){
				ColorCode(255,0,0,0,0,1,s,st);
				logStream << "ERROR: function '" << fname << "' is undefined\r\n";
				return;
			}

			if(funcs[i].params.size() != callArgCount.back()){
				ColorCode(255,0,0,0,0,1,s,e);
				logStream << "ERROR: function '" << fname << "' takes " << (UINT)funcs[i].params.size() << " argument(s), not " << callArgCount.back() << "\r\n";
				return;
			}
		}
		callArgCount.pop_back();
	}

	void ColOnErr(char const* s, char const* e)
	{
		if(s == e)
			e++;
		if(logStr.length() != 0)
			ColorCode(255,0,0,0,0,1,s,e);
		logStr = "";
	}
	void ColBlockBegin(char const* s, char const* e)
	{
		varInfoTop.push_back(VarTopInfo((UINT)varInfo.size(), varTop));
	}
	void ColBlockEnd(char const* s, char const* e)
	{
		while(varInfo.size() > varInfoTop.back().activeVarCnt)
		{
			varTop -= varInfo.back().count;
			varInfo.pop_back();
		}
		varInfoTop.pop_back();
	}

	//Parsing rules
	Rule col_expr, col_block, col_funcdef, col_break, col_if, col_for, col_return, col_vardef, col_while, col_dowhile, col_switch;
	Rule col_term5, col_term4_9, col_term4_6, col_term4_4, col_term4_2, col_term4_1, col_term4, col_term3, col_term2, col_term1, col_group, col_funccall, col_funcvars;
	Rule col_appval, col_varname, col_comment, col_symb, col_symb2, col_const, col_addvarp, col_addrefp, col_type;
	//Main rule and space parsers
	Rule col_code;
	Rule mySpaceP;

	//Callbacks
	boost::function<void (char const*, char const*)> ColorRWord, ColorVar, ColorVarDef, ColorFunc, ColorText, ColorReal, ColorInt, ColorBold, ColorErr, ColorComment;
	boost::function<void (char const*, char const*)> PLog, PLogS, BlockBegin, BlockEnd;
	boost::function<void (char const*, char const*)> AddVar, AddRef, SetVar, GetVar, FuncAdd, FuncEnd, FuncCall, ColorOnErr;
	boost::function<void (int, int, int, int, int, int, const char*, const char*)> ColorCode;
	//Error log
	ostringstream logStream;

	//Temporary variables
	UINT	varSize, varTop;
	bool	currValConst;
	std::string	logStr;

	std::vector<FunctionInfo>	funcs;
	std::vector<std::string>	strs;
	std::vector<VariableInfo>	varInfo;
	std::vector<VarTopInfo>		varInfoTop;
	std::vector<UINT>			callArgCount;
};

Colorer::Colorer(HWND rich): m_richEdit(rich)
{
	m_data = NULL;
	m_buf = new char[400000];//Should be enough
}
Colorer::~Colorer()
{
	delete m_data;
}

void Colorer::InitParser()
{
	m_data = new ColorerGrammar();

	m_data->ColorRWord	= bind(&Colorer::ColorCode, this, 0,0,255,0,0,0, _1, _2);
	m_data->ColorVar	= bind(&Colorer::ColorCode, this, 128,128,128,0,0,0, _1, _2);
	m_data->ColorVarDef	= bind(&Colorer::ColorCode, this, 50,50,50,0,0,0, _1, _2);
	m_data->ColorFunc	= bind(&Colorer::ColorCode, this, 136,0,0,0,0,0, _1, _2);
	m_data->ColorText	= bind(&Colorer::ColorCode, this, 0,0,0,0,0,0, _1, _2);
	m_data->ColorBold	= bind(&Colorer::ColorCode, this, 0,0,0,1,0,0, _1, _2);
	m_data->ColorReal	= bind(&Colorer::ColorCode, this, 0,150,0,0,0,0, _1, _2);
	m_data->ColorInt	= bind(&Colorer::ColorCode, this, 0,150,0,0,1,0, _1, _2);
	m_data->ColorErr	= bind(&Colorer::ColorCode, this, 255,0,0,0,0,1, _1, _2);
	m_data->ColorComment= bind(&Colorer::ColorCode, this, 255,0,255,0,0,0, _1, _2);

	m_data->ColorCode	= bind(&Colorer::ColorCode, this, _1,_2,_3,_4,_5,_6, _7, _8);

	m_data->PLog = bind(&Colorer::LogTempStr, this, 0, _1, _2);
	m_data->PLogS = bind(&Colorer::LogTempStr, this, 1, _1, _2);

	m_data->InitGrammar();
}
void Colorer::ColorText()
{
	m_data->varInfoTop.clear();
	m_data->varInfo.clear();
	m_data->funcs.clear();

	m_data->varInfo.push_back(VariableInfo("ERROR", 0, typeDouble));
	m_data->varInfo.push_back(VariableInfo("pi", 1, typeDouble));
	m_data->varInfo.push_back(VariableInfo("e", 2, typeDouble));

	m_data->varInfoTop.push_back(VarTopInfo(0,0));

	m_data->callArgCount.clear();
	m_data->varSize = 1;
	m_data->varTop = 3;

	m_data->logStream.str("");

	memset(m_buf, 0, GetWindowTextLength(m_richEdit)+5);
	GetWindowText(m_richEdit, m_buf, 400000);

	m_errUnderline = false;
	ColorCode(255,0,0,0,0,0, m_buf, m_buf+strlen(m_buf));

	if(!Parse(m_data->col_code, m_buf, m_data->mySpaceP))
		throw std::string("Syntax error");
	//char* ptr = m_buf;
	//if(!m_data->col_code->Parse(&ptr, m_data->mySpaceP.getParser()))
	//	throw std::string("Syntax error");
	if(m_data->logStream.str().length() != 0)
		throw m_data->logStream.str();
}

void Colorer::ColorCode(int red, int green, int blue, int bold, int ital, int under, const char* start, const char* end)
{
	if(m_errUnderline)
	{
		red=255;
		green=blue=0;
		bold=ital=0;
		under=1;
		m_errUnderline = 0;
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
	Edit_SetSel(m_richEdit, start-m_buf, end-m_buf);
	SendMessage(m_richEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

void Colorer::LogTempStr(int str, char const* s, char const* e)
{
	if(str)
		m_data->logStream << m_data->logStr << " " << std::string(s,e) << "\r\n";
	else
		m_data->logStream << m_data->logStr << "\r\n";
}
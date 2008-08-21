#include "stdafx.h"
#include "SupSpi.h"
using namespace supspi;

#include "ParseCommand.h"
#include "ParseClass.h"
#include "Compiler.h"

//////////////////////////////////////////////////////////////////////////
//						Code gen ops
//////////////////////////////////////////////////////////////////////////
//Code information
std::vector<FunctionInfo*>	funcs;
std::vector<VariableInfo>	varInfo;
std::vector<VarTopInfo>		varInfoTop;
std::vector<UINT>			undComandIndex;

std::vector<TypeInfo*>		typeInfo;
TypeInfo*	typeVoid = NULL;
TypeInfo*	typeChar = NULL;
TypeInfo*	typeShort = NULL;
TypeInfo*	typeInt = NULL;
TypeInfo*	typeFloat = NULL;
TypeInfo*	typeLong = NULL;
TypeInfo*	typeDouble = NULL;

TypeInfo*	currType = NULL;
std::vector<TypeInfo*>	currTypes;

std::vector<shared_ptr<NodeZeroOP> >	nodeList;

//Temp variables
UINT negCount, varDefined, varSize, varHaveIndex, needCopy, varTop;
bool currValConst;
std::vector<std::string>	strs;
std::vector<UINT>			callArgCount;
std::vector<TypeInfo*>		retTypeStack;

void checkIfDeclared(const std::string& str)
{
	if(str == "if" || str == "else" || str == "for" || str == "while" || str == "var" || str == "func" || str == "return" || str=="switch" || str=="case")
		throw std::string("ERROR: The name '" + str + "' is reserved");
	for(UINT i = 0; i < funcs.size(); i++)
		if(funcs[i]->name == str)
			throw std::string("ERROR: Name '" + str + "' is already taken for a function");
}

void blockBegin(char const* s, char const* e)
{
	varInfoTop.push_back(VarTopInfo((UINT)varInfo.size(), varTop));
}
void blockEnd(char const* s, char const* e)
{
	while(varInfo.size() > varInfoTop.back().activeVarCnt)
	{ 
		varTop -= varInfo.back().count*varInfo.back().varType->size;
		varInfo.pop_back();
	}
	varInfoTop.pop_back();
}

void removeTop(char const* s, char const* e)
{
	nodeList.pop_back();
}
/*
void popBackInIndexed(char const* s, char const* e)
{
	size_t braceInd = strs.back().find('[');
	if(braceInd != -1)
		nodeList.pop_back();
}*/

template<CmdID cmd> void createTwoAndCmd(char const* s, char const* e)
{
	addTwoAndCmpNode(cmd);
}

typedef void (*ParseFuncPtr)(char const* s, char const* e);

static ParseFuncPtr addCmd(CmdID cmd)
{
	if(cmd == cmdAdd) return &createTwoAndCmd<cmdAdd>;
	if(cmd == cmdSub) return &createTwoAndCmd<cmdSub>;
	if(cmd == cmdMul) return &createTwoAndCmd<cmdMul>;
	if(cmd == cmdDiv) return &createTwoAndCmd<cmdDiv>;
	if(cmd == cmdPow) return &createTwoAndCmd<cmdPow>;
	if(cmd == cmdLess) return &createTwoAndCmd<cmdLess>;
	if(cmd == cmdGreater) return &createTwoAndCmd<cmdGreater>;
	if(cmd == cmdLEqual) return &createTwoAndCmd<cmdLEqual>;
	if(cmd == cmdGEqual) return &createTwoAndCmd<cmdGEqual>;
	if(cmd == cmdEqual) return &createTwoAndCmd<cmdEqual>;
	if(cmd == cmdNEqual) return &createTwoAndCmd<cmdNEqual>;
	if(cmd == cmdShl) return &createTwoAndCmd<cmdShl>;
	if(cmd == cmdShr) return &createTwoAndCmd<cmdShr>;
	if(cmd == cmdMod) return &createTwoAndCmd<cmdMod>;
	if(cmd == cmdBitAnd) return &createTwoAndCmd<cmdBitAnd>;
	if(cmd == cmdBitOr) return &createTwoAndCmd<cmdBitOr>;
	if(cmd == cmdBitXor) return &createTwoAndCmd<cmdBitXor>;
	if(cmd == cmdLogAnd) return &createTwoAndCmd<cmdLogAnd>;
	if(cmd == cmdLogOr) return &createTwoAndCmd<cmdLogOr>;
	if(cmd == cmdLogXor) return &createTwoAndCmd<cmdLogXor>;
	throw std::string("ERROR: addCmd call with unknown command");
	return &createTwoAndCmd<cmdReturn>;
}

void addZeroNode(char const* s, char const* e){ nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeZeroOP())); }

long long atoll(const char* str)
{
	int len = 0;
	while(isdigit(str[len++]));
	int len2 = len -= 1;
	long long res = 0;
	while(len)
		res = res * 10L + (long long)(str[len2-len--] - '0');
	return res;
}
template<typename T>	void addNumberNode(char const*s, char const*e);
template<>	void addNumberNode<int>(char const*s, char const*e){ nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(atoi(s), typeInt))); };
template<>	void addNumberNode<float>(char const*s, char const*e){ nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<float>((float)atof(s), typeFloat))); };
template<>	void addNumberNode<long long>(char const*s, char const*e){ nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<long long>(atoll(s), typeLong))); };
template<>	void addNumberNode<double>(char const*s, char const*e){ nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<double>(atof(s), typeDouble))); };

void addPopNode(char const* s, char const* e){
	if((*(nodeList.end()-1))->GetNodeType() == typeNodeNumber){
		nodeList.pop_back();
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeZeroOP()));
	}else if((*(nodeList.end()-1))->GetNodeType() == typeNodePreValOp){
		static_cast<NodePreValOp*>(nodeList.back().get())->SetOptimised(true);
	}else{
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodePopOp()));
	}
}
void addNegNode(char const* s, char const* e){
	if(negCount % 2 == 0)
		return;
	if((*(nodeList.end()-1))->GetNodeType() == typeNodeNumber){
		TypeInfo *aType = (*(nodeList.end()-1))->GetTypeInfo();
		NodeZeroOP* zOP = (nodeList.end()-1)->get();
		shared_ptr<NodeZeroOP > Rd;
		if(aType == typeDouble)
		{
			Rd.reset(new NodeNumber<double>(-static_cast<NodeNumber<double>* >(zOP)->GetVal(), zOP->GetTypeInfo()));
		}else if(aType == typeFloat){
			Rd.reset(new NodeNumber<float>(-static_cast<NodeNumber<float>* >(zOP)->GetVal(), zOP->GetTypeInfo()));
		}else if(aType == typeLong){
			Rd.reset(new NodeNumber<long long>(-static_cast<NodeNumber<long long>* >(zOP)->GetVal(), zOP->GetTypeInfo()));
		}else if(aType == typeInt){
			Rd.reset(new NodeNumber<int>(-static_cast<NodeNumber<int>* >(zOP)->GetVal(), zOP->GetTypeInfo()));
		}else{
			throw std::string("addBitNotNode() ERROR: unknown type ") + aType->name;
		}
		nodeList.pop_back();
		nodeList.push_back(Rd);
	}else{
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeUnaryOp(cmdNeg)));
	}
	negCount = 0;
}
void addLogNotNode(char const* s, char const* e){
	if((*(nodeList.end()-1))->GetNodeType() == typeNodeNumber){
		TypeInfo *aType = (*(nodeList.end()-1))->GetTypeInfo();
		NodeZeroOP* zOP = (nodeList.end()-1)->get();
		shared_ptr<NodeZeroOP > Rd;
		if(aType == typeDouble)
		{
			Rd.reset(new NodeNumber<double>(static_cast<NodeNumber<double>* >(zOP)->GetLogNotVal(), zOP->GetTypeInfo()));
		}else if(aType == typeFloat){
			Rd.reset(new NodeNumber<float>(static_cast<NodeNumber<float>* >(zOP)->GetLogNotVal(), zOP->GetTypeInfo()));
		}else if(aType == typeLong){
			Rd.reset(new NodeNumber<long long>(static_cast<NodeNumber<long long>* >(zOP)->GetLogNotVal(), zOP->GetTypeInfo()));
		}else if(aType == typeInt){
			Rd.reset(new NodeNumber<int>(static_cast<NodeNumber<int>* >(zOP)->GetLogNotVal(), zOP->GetTypeInfo()));
		}else{
			throw std::string("addBitNotNode() ERROR: unknown type ") + aType->name;
		}
		nodeList.pop_back();
		nodeList.push_back(Rd);
	}else{
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeUnaryOp(cmdLogNot)));
	}
}
void addBitNotNode(char const* s, char const* e){
	if((*(nodeList.end()-1))->GetNodeType() == typeNodeNumber){
		TypeInfo *aType = (*(nodeList.end()-1))->GetTypeInfo();
		NodeZeroOP* zOP = (nodeList.end()-1)->get();
		shared_ptr<NodeZeroOP > Rd;
		if(aType == typeDouble)
		{
			throw std::string("ERROR: bitwise NOT cannot be used on floating point numbers");
		}else if(aType == typeFloat){
			throw std::string("ERROR: bitwise NOT cannot be used on floating point numbers");
		}else if(aType == typeLong){
			Rd.reset(new NodeNumber<long long>(static_cast<NodeNumber<long long>* >(zOP)->GetBitNotVal(), zOP->GetTypeInfo()));
		}else if(aType == typeInt){
			Rd.reset(new NodeNumber<int>(static_cast<NodeNumber<int>* >(zOP)->GetBitNotVal(), zOP->GetTypeInfo()));
		}else{
			throw std::string("addBitNotNode() ERROR: unknown type ") + aType->name;
		}
		nodeList.pop_back();
		nodeList.push_back(Rd);
	}else{
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeUnaryOp(cmdBitNot)));
	}
}

template<typename T>
T optDoOperation(CmdID cmd, T a, T b, bool swap = false)
{
	if(swap)
		std::swap(a, b);
	if(cmd == cmdAdd) return a + b;
	if(cmd == cmdSub) return a - b;
	if(cmd == cmdMul) return a * b;
	if(cmd == cmdDiv) return a / b;
	if(cmd == cmdPow) return (T)pow((double)a, (double)b);
	if(cmd == cmdLess) return a < b;
	if(cmd == cmdGreater) return a > b;
	if(cmd == cmdGEqual) return a >= b;
	if(cmd == cmdLEqual) return a <= b;
	if(cmd == cmdEqual) return a == b;
	if(cmd == cmdNEqual) return a != b;
	return optDoSpecial(cmd, a, b);
}
template<typename T>	T	optDoSpecial(CmdID cmd, T a, T b)
{
	throw std::string("ERROR: optDoSpecial call with unknown type");
}
template<>				int	optDoSpecial<>(CmdID cmd, int a, int b)
{
	if(cmd == cmdShl) return a << b;
	if(cmd == cmdShr) return a >> b;
	if(cmd == cmdMod) return a % b;
	if(cmd == cmdBitAnd) return a & b;
	if(cmd == cmdBitXor) return a ^ b;
	if(cmd == cmdBitOr) return a | b;
	if(cmd == cmdLogAnd) return a && b;
	if(cmd == cmdLogXor) return !!a ^ !!b;
	if(cmd == cmdLogOr) return a || b;
	throw std::string("ERROR: optDoSpecial<int> call with unknown command");
}
template<>				long long	optDoSpecial<>(CmdID cmd, long long a, long long b)
{
	if(cmd == cmdShl) return a << b;
	if(cmd == cmdShr) return a >> b;
	if(cmd == cmdMod) return a % b;
	if(cmd == cmdBitAnd) return a & b;
	if(cmd == cmdBitXor) return a ^ b;
	if(cmd == cmdBitOr) return a | b;
	if(cmd == cmdLogAnd) return a && b;
	if(cmd == cmdLogXor) return !!a ^ !!b;
	if(cmd == cmdLogOr) return a || b;
	throw std::string("ERROR: optDoSpecial<long long> call with unknown command");
}
template<>				double	optDoSpecial<>(CmdID cmd, double a, double b)
{
	if(cmd == cmdShl)
		throw std::string("ERROR: optDoSpecial<double> call with << operation is illegal");
	if(cmd == cmdShr)
		throw std::string("ERROR: optDoSpecial<double> call with >> operation is illegal");
	if(cmd == cmdMod) return fmod(a,b);
	if(cmd >= cmdBitAnd && cmd <= cmdBitXor)
		throw std::string("ERROR: optDoSpecial<double> call with binary operation is illegal");
	if(cmd == cmdLogAnd) return (int)a && (int)b;
	if(cmd == cmdLogXor) return !!(int)a ^ !!(int)b;
	if(cmd == cmdLogOr) return (int)a || (int)b;
	throw std::string("ERROR: optDoSpecial<double> call with unknown command");
}

void addTwoAndCmpNode(CmdID id){

	if((*(nodeList.end()-1))->GetNodeType() == typeNodeNumber && (*(nodeList.end()-2))->GetNodeType() == typeNodeNumber){
		//If we have operation between two known numbers, we can optimize code by calculating the result in place

		TypeInfo *aType, *bType;
		aType = (*(nodeList.end()-2))->GetTypeInfo();
		bType = (*(nodeList.end()-1))->GetTypeInfo();

		UINT shA = 2, shB = 1;	//Shift's to operand A and B in array
		//Swap operands, to reduce number of combinations
		if((aType == typeFloat || aType == typeLong || aType == typeInt) && bType == typeDouble)
			std::swap(shA, shB);
		if((aType == typeLong || aType == typeInt) && bType == typeFloat)
			std::swap(shA, shB);
		if(aType == typeInt && bType == typeLong)
			std::swap(shA, shB);

		bool swapOper = shA != 2;

		aType = (*(nodeList.end()-shA))->GetTypeInfo();
		bType = (*(nodeList.end()-shB))->GetTypeInfo();
		if(aType == typeDouble)
		{
			NodeNumber<double> *Ad = static_cast<NodeNumber<double>* >((nodeList.end()-shA)->get());
			shared_ptr<NodeNumber<double> > Rd;
			if(bType == typeDouble)
			{
				NodeNumber<double> *Bd = static_cast<NodeNumber<double>* >((nodeList.end()-shB)->get());
				Rd.reset(new NodeNumber<double>(optDoOperation<double>(id, Ad->GetVal(), Bd->GetVal()), typeDouble));
			}else if(bType == typeFloat){
				NodeNumber<float> *Bd = static_cast<NodeNumber<float>* >((nodeList.end()-shB)->get());
				Rd.reset(new NodeNumber<double>(optDoOperation<double>(id, Ad->GetVal(), (double)Bd->GetVal(), swapOper), typeDouble));
			}else if(bType == typeLong){
				NodeNumber<long long> *Bd = static_cast<NodeNumber<long long>* >((nodeList.end()-shB)->get());
				Rd.reset(new NodeNumber<double>(optDoOperation<double>(id, Ad->GetVal(), (double)Bd->GetVal(), swapOper), typeDouble));
			}else if(bType == typeInt){
				NodeNumber<int> *Bd = static_cast<NodeNumber<int>* >((nodeList.end()-shB)->get());
				Rd.reset(new NodeNumber<double>(optDoOperation<double>(id, Ad->GetVal(), (double)Bd->GetVal(), swapOper), typeDouble));
			}
			nodeList.pop_back(); nodeList.pop_back();
			nodeList.push_back(Rd);
		}else if(aType == typeFloat){
			NodeNumber<float> *Ad = static_cast<NodeNumber<float>* >((nodeList.end()-shA)->get());
			shared_ptr<NodeNumber<float> > Rd;
			if(bType == typeFloat){
				NodeNumber<float> *Bd = static_cast<NodeNumber<float>* >((nodeList.end()-shB)->get());
				Rd.reset(new NodeNumber<float>(optDoOperation<float>(id, Ad->GetVal(), Bd->GetVal()), typeFloat));
			}else if(bType == typeLong){
				NodeNumber<long long> *Bd = static_cast<NodeNumber<long long>* >((nodeList.end()-shB)->get());
				Rd.reset(new NodeNumber<float>(optDoOperation<float>(id, Ad->GetVal(), (float)Bd->GetVal(), swapOper), typeFloat));
			}else if(bType == typeInt){
				NodeNumber<int> *Bd = static_cast<NodeNumber<int>* >((nodeList.end()-shB)->get());
				Rd.reset(new NodeNumber<float>(optDoOperation<float>(id, Ad->GetVal(), (float)Bd->GetVal(), swapOper), typeFloat));
			}
			nodeList.pop_back(); nodeList.pop_back();
			nodeList.push_back(Rd);
		}else if(aType == typeLong){
			NodeNumber<long long> *Ad = static_cast<NodeNumber<long long>* >((nodeList.end()-shA)->get());
			shared_ptr<NodeNumber<long long> > Rd;
			if(bType == typeLong){
				NodeNumber<long long> *Bd = static_cast<NodeNumber<long long>* >((nodeList.end()-shB)->get());
				Rd.reset(new NodeNumber<long long>(optDoOperation<long long>(id, Ad->GetVal(), Bd->GetVal()), typeLong));
			}else if(bType == typeInt){
				NodeNumber<int> *Bd = static_cast<NodeNumber<int>* >((nodeList.end()-shB)->get());
				Rd.reset(new NodeNumber<long long>(optDoOperation<long long>(id, Ad->GetVal(), (long long)Bd->GetVal(), swapOper), typeLong));
			}
			nodeList.pop_back(); nodeList.pop_back();
			nodeList.push_back(Rd);
		}else if(aType == typeInt){
			NodeNumber<int> *Ad = static_cast<NodeNumber<int>* >((nodeList.end()-shA)->get());
			shared_ptr<NodeNumber<int> > Rd;
			//bType is also int!
			NodeNumber<int> *Bd = static_cast<NodeNumber<int>* >((nodeList.end()-shB)->get());
			Rd.reset(new NodeNumber<int>(optDoOperation<int>(id, Ad->GetVal(), Bd->GetVal()), typeInt));
			nodeList.pop_back(); nodeList.pop_back();
			nodeList.push_back(Rd);
		}
	}else{
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeTwoAndCmdOp(id)));
	}
}
void addReturnNode(char const* s, char const* e)
{
	//ColorCode(0, 0, 255, s, s+6);
	int t = (int)varInfoTop.size();
	int c = 0;
	if(funcs.size() != 0)
		while(t > (int)funcs.back()->vTopSize){
			c++;
			t--;
		}
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeReturnOp(c, retTypeStack.back())));
}

void addBreakNode(char const* s, char const* e)
{
	//ColorCode(0, 0, 255, s, s+5);
	if(undComandIndex.empty())
		throw std::string("ERROR: break used outside loop statements");
	int t = (int)varInfoTop.size();
	int c = 0;
	while(t > (int)undComandIndex.back()){
		c++;
		t--;
	}
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeBreakOp(c)));
}

//Finds TypeInfo in a typeInfo list by name
void selType(char const* s, char const* e)
{
	string vType = std::string(s,e);
	for(UINT i = 0; i < typeInfo.size(); i++)
	{
		if(typeInfo[i]->name == vType)
		{
			currType = typeInfo[i];
			return;
		}
	}
	throw std::string("ERROR: Variable type '" + vType + "' is unknown\r\n");
}

void addVar(char const* s, char const* e)
{
	string vName = *(strs.end()-2);
	size_t braceInd = strs.back().find('[');

	for(UINT i = varInfoTop.back().activeVarCnt; i < varInfo.size(); i++)
		if(varInfo[i].name == vName)
			throw std::string("ERROR: Name '" + vName + "' is already taken for a variable in current scope\r\n");
	checkIfDeclared(vName);
	//infoln4("do_addvar ", vName, " ", varTop-varInfoTop.back().t);
	if(varSize > 128000)
		throw std::string("ERROR: variable '" + vName + "' has to big length (>128000)");
	varInfo.push_back(VariableInfo(vName, varTop, currType, varSize, currValConst));
	varDefined += varSize-1;
	varTop += varSize*currType->size;
	varSize = 1;
}

void addRefVar(char const* s, char const* e)
{
	string vRefName = *(strs.end()-2);
	string vVarName = *(strs.end()-1);
	strs.pop_back();
	strs.pop_back();

	int i = (int)varInfo.size()-1;
	while(i >= 0 && varInfo[i].name != vVarName)
		i--;
	if(i == -1)
		throw std::string("ERROR: variable '" + vVarName + " is not defined");

	varInfo.push_back(VariableInfo(vRefName, varInfo[i].pos+varSize, currType, 1, currValConst, true));
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeZeroOP()));
}

void addVarDefNode(char const* s, char const* e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarDef(varDefined*currType->size, strs.back())));
	varDefined = 0;
}

void pushType(char const* s, char const* e)
{
	currTypes.push_back(currType);
}

void popType(char const* s, char const* e)
{
	currTypes.pop_back();
}

bool pushedShiftAddrNode = false;
bool pushedShiftAddr = false;

void popTypeAndAddrNode(char const* s, char const* e)
{
	currTypes.pop_back();
	if(pushedShiftAddr || pushedShiftAddrNode)
		nodeList.pop_back();
}

void getType(char const* s, char const* e)
{
	int i = (int)varInfo.size()-1;
	string vName = strs.back();
	while(i >= 0 && varInfo[i].name != vName)
		i--;
	if(i == -1)
		throw std::string("ERROR: variable '" + strs.back() + "' is not defined [set]");
	currTypes.push_back(varInfo[i].varType);
	pushedShiftAddr = false;
	pushedShiftAddrNode = false;
}

void getMember(char const* s, char const* e)
{
	string vName = std::string(s, e);
	currType = currTypes.back();

	int i = (int)currType->memberData.size()-1;
	while(i >= 0 && currType->memberData[i].name != vName)
		i--;
	if(i == -1)
		throw std::string("ERROR: variable '" + vName + "' is not a member of '" + currType->name + "' [set]");
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(currType->memberData[i].offset, typeInt)));
	if(pushedShiftAddrNode | pushedShiftAddr)
		addTwoAndCmpNode(cmdAdd);
	pushedShiftAddrNode = false;
	pushedShiftAddr = true;
	currTypes.back() = currType->memberData[i].type;
}

void addShiftAddrNode(char const* s, char const* e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodePushShift(currTypes.back()->size)));
	pushedShiftAddrNode = true;
}

void addSetNode(char const* s, char const* e)
{
	int i = (int)varInfo.size()-1;
	string vName = *(strs.end()-2);
	size_t braceInd = strs.back().find('[');
	size_t compoundType = strs.back().find('.');

	while(i >= 0 && varInfo[i].name != vName)
		i--;
	if(i == -1)
		throw std::string("ERROR: variable '" + strs.back() + "' is not defined [set]");
	if(!currValConst && varInfo[i].isConst)
		throw std::string("ERROR: cannot change constant parameter '" + strs.back() + "' ");
	if(braceInd != -1 && varInfo[i].count == 1)
		throw std::string("ERROR: variable '" + strs.back() + "' is not an array");
	if(braceInd == -1 && varInfo[i].count > 1)
		throw std::string("ERROR: variable '" + strs.back() + "' is an array, but no index specified");

	bool aabsadr = ((varInfoTop.size() > 1) && (varInfo[i].pos < varInfoTop[1].varStackSize)) || varInfoTop.back().varStackSize == 0;
	int ashift = aabsadr ? 0 : varInfoTop.back().varStackSize;

	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarSet(varInfo[i], currTypes.back(), varInfo[i].pos-ashift, compoundType != -1, varDefined != 0 && braceInd != -1, aabsadr)));
	currTypes.pop_back();

	currValConst = false;
	varDefined = 0;
}

void addGetNode(char const* s, char const* e)
{
	int i = (int)varInfo.size()-1;
	string vName = *(strs.end()-2);
	size_t braceInd = strs.back().find('[');
	size_t compoundType = strs.back().find('.');

	while(i >= 0 && varInfo[i].name != vName)
		i--;
	if(i == -1)
		throw std::string("ERROR: variable '" + strs.back() + "' is not defined [get]");
	if(braceInd != -1 && varInfo[i].count == 1)
		throw std::string("ERROR: variable '" + strs.back() + "' is not an array");
	if(braceInd == -1 && varInfo[i].count > 1)
		throw std::string("ERROR: variable '" + strs.back() + "' is an array, but no index specified");

	if(((varInfoTop.size() > 1) && (varInfo[i].pos < varInfoTop[1].varStackSize)) || varInfoTop.back().varStackSize == 0)
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarGet(varInfo[i], currTypes.back(), varInfo[i].pos, compoundType != -1, true)));
	else
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarGet(varInfo[i], currTypes.back(), varInfo[i].pos-(int)(varInfoTop.back().varStackSize), compoundType != -1, false)));
	
	currTypes.pop_back();
}

void addSetAndOpNode(CmdID cmd)
{
	int i = (int)varInfo.size()-1;
	string vName = *(strs.end()-2);
	size_t braceInd = strs.back().find('[');
	size_t compoundType = strs.back().find('.');

	while(i >= 0 && varInfo[i].name != vName)
		i--;
	if(i == -1)
		throw std::string("ERROR: variable " + strs.back() + " is not defined");
	if(!currValConst && varInfo[i].isConst)
		throw std::string("ERROR: cannot change constant parameter '" + strs.back() + "' ");
	if(braceInd == -1 && varInfo[i].count > 1)
		throw std::string("ERROR: variable '" + strs.back() + "' is an array, but no index specified");

	bool aabsadr = ((varInfoTop.size() > 1) && (varInfo[i].pos < varInfoTop[1].varStackSize)) || varInfoTop.back().varStackSize == 0;
	int ashift = aabsadr ? 0 : varInfoTop.back().varStackSize;

	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarSetAndOp(varInfo[i], currTypes.back(), varInfo[i].pos-ashift, compoundType != -1, aabsadr, cmd)));

	//nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarSetAndOp(varInfo[i].varType, varInfo[i].pos-varInfoTop.back().varStackSize, vName, braceInd != -1, varInfo[i].count, cmd)));
	varDefined = 0;
}
void addAddSetNode(char const* s, char const* e){ addSetAndOpNode(cmdAdd); }
void addSubSetNode(char const* s, char const* e){ addSetAndOpNode(cmdSub); }
void addMulSetNode(char const* s, char const* e){ addSetAndOpNode(cmdMul); }
void addDivSetNode(char const* s, char const* e){ addSetAndOpNode(cmdDiv); }
void addPowSetNode(char const* s, char const* e){ addSetAndOpNode(cmdPow); }

void addPreOpNode(CmdID cmd, bool pre)
{
	int i = (int)varInfo.size()-1;
	string vName = *(strs.end()-2);
	size_t braceInd = strs.back().find('[');
	size_t compoundType = strs.back().find('.');

	while(i >= 0 && varInfo[i].name != vName)
		i--;
	if(i == -1)
		throw std::string("ERROR: variable '" + strs.back() + "' is not defined [set]");
	if(!currValConst && varInfo[i].isConst)
		throw std::string("ERROR: cannot change constant parameter '" + strs.back() + "' ");
	if(braceInd == -1 && varInfo[i].count > 1)
		throw std::string("ERROR: variable '" + strs.back() + "' is an array, but no index specified");

	bool aabsadr = ((varInfoTop.size() > 1) && (varInfo[i].pos < varInfoTop[1].varStackSize)) || varInfoTop.back().varStackSize == 0;
	int ashift = aabsadr ? 0 : varInfoTop.back().varStackSize;

	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodePreValOp(varInfo[i], currTypes.back(), varInfo[i].pos-ashift, compoundType != -1, aabsadr, cmd, pre)));
	currTypes.pop_back();
}
void addPreDecNode(char const* s, char const* e){ addPreOpNode(cmdDec, true); }
void addPreIncNode(char const* s, char const* e){ addPreOpNode(cmdInc, true); }
void addPostDecNode(char const* s, char const* e){ addPreOpNode(cmdDec, false); }
void addPostIncNode(char const* s, char const* e){ addPreOpNode(cmdInc, false); }

void addOneExprNode(char const* s, char const* e){ nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeExpression())); }
void addTwoExprNode(char const* s, char const* e){ nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeTwoExpression())); }
void addBlockNode(char const* s, char const* e){ nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeBlock())); }

void funcAdd(char const* s, char const* e)
{
	//infoln3(__FUNCTION__, " adding function ", strs.back());
	for(UINT i = varInfoTop.back().activeVarCnt; i < varInfo.size(); i++)
		if(varInfo[i].name == strs.back())
			throw std::string("ERROR: Name '" + strs.back() + "' is already taken for a variable in current scope\r\n");
	checkIfDeclared(strs.back());
	funcs.push_back(new FunctionInfo());
	funcs.back()->name = strs.back();
	funcs.back()->vTopSize = (UINT)varInfoTop.size();
	retTypeStack.push_back(currType);
	funcs.back()->retType = currType;
	//strs.pop_back();
	//funcs.back().address = cmds.GetCurrPos()+sizeof(CmdID)+sizeof(UINT);
}
void funcParam(char const* s, char const* e)
{
	//infoln3(__FUNCTION__, " adding variable ", strs.back());
	funcs.back()->params.push_back(VariableInfo(strs.back(), 0, currType, 1, currValConst));
	strs.pop_back();
}
void funcStart(char const* s, char const* e)
{
	//infoln1(__FUNCTION__);
	varInfoTop.push_back(VarTopInfo((UINT)varInfo.size(), varTop));
	bool two = false;
	for(int i = (int)funcs.back()->params.size()-1; i >= 0; i--)
	{
		strs.push_back(funcs.back()->params[i].name);
		strs.push_back(funcs.back()->params[i].name);
		currValConst = funcs.back()->params[i].isConst;
		currType = funcs.back()->params[i].varType;
		addVar(0,0);

		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeZeroOP(currType)));

		pushType(0,0);
		addSetNode(0,0);
		//nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarSet(varInfo[n].pos, varInfo[n].name, false, 1)));
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodePopOp()));
		if(two)
			nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeTwoExpression()));
		two = true;
		strs.pop_back();
		strs.pop_back();
	}
}
void funcEnd(char const* s, char const* e)
{
	int i = (int)funcs.size()-1;
	while(i >= 0 && funcs[i]->name != strs.back())
		i--;

	while(varInfo.size() > varInfoTop.back().activeVarCnt)
	{
		varTop -= varInfo.back().count*varInfo.back().varType->size;//sizeof(double);
		varInfo.pop_back();
	}
	varInfoTop.pop_back();
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeTwoExpression()));
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeBlock()));
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeFuncDef(i)));
	strs.pop_back();
	retTypeStack.pop_back();
}


void addFuncPushParamNode(char const* s, char const* e)
{
	string fname = strs.back();
	if(fname == "cos" || fname == "sin" || fname == "tan" || fname == "ctg" || fname == "ceil" || fname == "floor" || 
		fname == "sqrt" || fname == "clock")
	{
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeFuncParam(typeDouble)));
	}else{
		int i = (int)funcs.size()-1;
		while(i >= 0 && funcs[i]->name != fname)
			i--;
		if(i == -1)
			throw std::string("ERROR: function " + fname + " is undefined");
		VariableInfo* vinfo = &funcs[i]->params[callArgCount.back()-1];
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeFuncParam(vinfo->varType)));
	}
}
void addFuncCallNode(char const* s, char const* e)
{
	string fname = strs.back();
	strs.pop_back();
	//infoln3(__FUNCTION__, " ", fname); 
	char strCnt[32];
	//Find standard function
	if(fname == "cos" || fname == "sin" || fname == "tan" || fname == "ctg" || fname == "ceil" || fname == "floor" || 
		fname == "sqrt" || fname == "clock")
	{
		if(fname == "clock" && callArgCount.back() != 0)
			throw std::string("ERROR: function " + fname + " takes no argumets\r\n");
		if(fname != "clock" && callArgCount.back() != 1)
			throw std::string("ERROR: function " + fname + " takes one argument\r\n");
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeFuncCall(fname, -1, callArgCount.back(), (fname == "clock" ? typeInt : typeDouble))));
	}else{	//Find user-defined function
		int i = (int)funcs.size()-1;
		while(i >= 0 && funcs[i]->name != fname)
			i--;
		if(i == -1)
			throw std::string("ERROR: function " + fname + " is undefined");
		if(funcs[i]->params.size() != callArgCount.back())
			throw std::string("ERROR: function ") + fname + std::string(" takes ") + _itoa((int)funcs[i]->params.size(), strCnt, 10) + std::string(" argument(s)\r\n");

		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeFuncCall(fname, i, callArgCount.back(), funcs[i]->retType)));
	}
	callArgCount.pop_back();
}

void addIfNode(char const* s, char const* e){ nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeIfElseExpr(false))); }
void addIfElseNode(char const* s, char const* e){ nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeIfElseExpr(true))); }
void addIfElseTermNode(char const* s, char const* e)
{
	TypeInfo* typeA = nodeList[nodeList.size()-1]->GetTypeInfo();
	TypeInfo* typeB = nodeList[nodeList.size()-2]->GetTypeInfo();
	if(typeA != typeB)
		throw std::string("ERROR: trinary operator ?: \r\n result types are not equal (" + typeB->name + " : " + typeA->name + ")");
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeIfElseExpr(true, true)));
}

void saveVarTop(char const* s, char const* e){ undComandIndex.push_back((UINT)varInfoTop.size()); }
void addForNode(char const* s, char const* e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeForExpr()));
	undComandIndex.pop_back();
}
void addWhileNode(char const* s, char const* e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeWhileExpr()));
	undComandIndex.pop_back();
}
void addDoWhileNode(char const* s, char const* e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeDoWhileExpr()));
	undComandIndex.pop_back();
}

void preSwitchNode(char const* s, char const* e)
{
	undComandIndex.push_back((UINT)varInfoTop.size());
	varInfoTop.push_back(VarTopInfo((UINT)varInfo.size(), varTop));
}
void addCaseNode(char const* s, char const* e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeCaseExpr()));
}
void addSwitchNode(char const* s, char const* e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeSwitchExpr()));
	undComandIndex.pop_back();
	while(varInfo.size() > varInfoTop.back().activeVarCnt)
	{
		varTop -= varInfo.back().count;
		varInfo.pop_back();
	}
	varInfoTop.pop_back();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace CompilerGrammar
{
	// Если при парсинге обнаруживается синтаксическая ошибка
	// В errStr помещается сообщение об ошибке и вызывается функция
	// ParseAbort, которая останавливает парсинг, создавая исключительную ситуацию
	std::string errStr;
	void ParseAbort(char const*, char const*){ throw errStr; }

	// Функции, кладушие и убирающие строки со стека строк
	// Стек строк может использоваться для удобного получения элемента более сложной грамматики
	// Например для правила использования переменной a[i], можно поместить "a" в стек,
	// потому что в функцию передаётся "a[i]" целиком
	void ParseStrPush(char const *s, char const *e){ strs.push_back(string(s,e)); }
	void ParseStrPop(char const *s, char const *e){ strs.pop_back(); }

	// Callbacks
	typedef void (*parserCallback)(char const*, char const*);
	parserCallback addInt, addFloat, addLong, addDouble;
	parserCallback strPush, strPop, pAbort;

	// Parser rules
	Rule group, term5, term4_9, term4_8, term4_85, term4_7, term4_75, term4_6, term4_65, term4_4, term4_2, term4_1, term4, term3, term2, term1, expression;
	Rule varname, funccall, funcdef, funcvars, block, vardef, vardefsub, applyval, ifexpr, whileexpr, forexpr, retexpr;
	Rule doexpr, breakexpr, switchexpr, isconst, addvarp, addrefp, seltype;

	Rule code, mySpaceP;

	void InitGrammar()
	{
		pAbort	=	CompilerGrammar::ParseAbort;
		strPush	=	CompilerGrammar::ParseStrPush;
		strPop	=	CompilerGrammar::ParseStrPop;

		addInt		=	addNumberNode<int>;
		addFloat	=	addNumberNode<float>;
		addLong		=	addNumberNode<long long>;
		addDouble	=	addNumberNode<double>;

		seltype		=	varname[selType];

		isconst		=	epsP[AssignVar<bool>(currValConst,false)] >> !strP("const")[AssignVar<bool>(currValConst,true)];
		varname		=	lexemeD[alphaP >> *alnumP];

		funccall	=	varname[strPush] >> 
			('(' | (epsP[strPop] >> nothingP)) >>
			epsP[PushBackVal<std::vector<UINT>, UINT>(callArgCount, 0)] >> 
			!(
			term5[ArrBackInc<std::vector<UINT> >(callArgCount)][addFuncPushParamNode] >>
			*(',' >> term5[ArrBackInc<std::vector<UINT> >(callArgCount)][addFuncPushParamNode])[addTwoExprNode]
			) >>
			(')' | epsP[AssignVar<string>(errStr, "ERROR: ')' not found after function call")][pAbort]);
		funcvars	=	!(seltype >> isconst >> !strP("ref") >> varname[strPush][funcParam]) >> *(',' >> seltype >> isconst >> !strP("ref") >> varname[strPush][funcParam]);
		funcdef		=	strP("func") >> seltype >> varname[strPush][funcAdd] >> '(' >>  funcvars[funcStart] >> chP(')') >> chP('{') >> code[funcEnd] >> chP('}');

		applyval	=
			(
				(varname - strP("case"))[strPush] >> (~chP('(') | (epsP[strPop] >> nothingP)) >> epsP[getType] >>
				!('[' >> term5 >> ']')[addShiftAddrNode] >>
				*(
					'.' >>
					(varname - strP("case"))[getMember] >>
					!('[' >> term5 >> ']')[addShiftAddrNode][addCmd(cmdAdd)]
				)
			)[strPush];
		addvarp		=
			(
			(varname[strPush][pushType] >>
			epsP[AssignVar<UINT>(varSize,1)] >>
			!('[' >> intP[StrToInt(varSize)] >> ']'))
			)[strPush][addVar][IncVar<UINT>(varDefined)] >>
			(('=' >> term5)[addSetNode][addPopNode] | epsP[addVarDefNode][popType])[strPop][strPop];
		addrefp		=
			(
			varname[strPush] >>
			chP('=') >>
			epsP[AssignVar<UINT>(varSize,1)] >>
			varname[strPush] >>
			!('[' >> intP[StrToInt(varSize)] >> ']')
			)[addRefVar];
		vardefsub	=
			((strP("ref") >> addrefp) | addvarp) >>
			*(',' >> vardefsub)[addTwoExprNode];
		vardef		=
			seltype >>
			isconst >>
			vardefsub;

		ifexpr		=	strP("if") >> ('(' >> term5 >> ')') >> expression >> ((strP("else") >> expression)[addIfElseNode] | epsP[addIfNode]);
		forexpr		=	strP("for")[saveVarTop] >> '(' >> ((strP("var") >> vardef) | term5[addPopNode] | block) >> ';' >> term5 >> ';' >> (term5[addPopNode] | block) >> ')' >> expression[addForNode];
		whileexpr	=
			strP("while")[saveVarTop] >>
			(
			('(' | epsP[AssignVar<string>(errStr, "ERROR: '(' not found after 'while'")][pAbort]) >>
			(term5 | epsP[AssignVar<string>(errStr, "ERROR: expression expected after 'while('")][pAbort]) >>
			(')' | epsP[AssignVar<string>(errStr, "ERROR: closing ')' not found after expression in 'while' statement")][pAbort])
			) >>
			(expression[addWhileNode] | epsP[AssignVar<string>(errStr, "ERROR: expression expected after 'while(...)'")][pAbort]);
		doexpr		=	
			strP("do")[saveVarTop] >> 
			(expression | epsP[AssignVar<string>(errStr, "ERROR: expression expected after 'do'")][pAbort]) >> 
			(strP("while") | epsP[AssignVar<string>(errStr, "ERROR: 'while' expected after 'do' statement")][pAbort]) >>
			(
			('(' | epsP[AssignVar<string>(errStr, "ERROR: '(' not found after 'while'")][pAbort]) >> 
			(term5 | epsP[AssignVar<string>(errStr, "ERROR: expression not found after 'while('")][pAbort]) >> 
			(')' | epsP[AssignVar<string>(errStr, "ERROR: closing ')' not found after expression in 'while' statement")][pAbort])
			)[addDoWhileNode] >> 
			(';' | epsP[AssignVar<string>(errStr, "ERROR: while(...) should be followed by ';'")][pAbort]);
		switchexpr	=
			strP("switch")[preSwitchNode] >>
			('(' | epsP[AssignVar<string>(errStr, "ERROR: '(' not found after 'switch'")][pAbort]) >>
			(term5 | epsP[AssignVar<string>(errStr, "ERROR: expression not found after 'switch('")][pAbort]) >>
			(')' | epsP[AssignVar<string>(errStr, "ERROR: closing ')' not found after expression in 'switch' statement")][pAbort]) >>
			('{' | epsP[AssignVar<string>(errStr, "ERROR: '{' not found after 'switch(...)'")][pAbort]) >>
			(strP("case") >> term5 >> ':' >> expression >> *expression[addTwoExprNode])[addCaseNode] >>
			*(strP("case") >> term5 >> ':' >> expression >> *expression[addTwoExprNode])[addCaseNode][addTwoExprNode] >>
			//(strP("case") >> term5 >> ':' >> code)[addCaseNode] >>
			('}' | epsP[AssignVar<string>(errStr, "ERROR: '}' not found after 'switch' statement")][pAbort])[addSwitchNode];

		retexpr		=	(strP("return") >> term5 >> +chP(';'))[addReturnNode];
		breakexpr	=	(
			strP("break") >>
			(+chP(';') | epsP[AssignVar<string>(errStr, "ERROR: break must be followed by ';'")][pAbort])
			)[addBreakNode];

		group		=	'(' >> term5 >> ')';
		term1		=	
			(strP("--") >> applyval)[addPreDecNode][strPop][strPop] | 
			(strP("++") >> applyval)[addPreIncNode][strPop][strPop] |
			(+(chP('-')[IncVar<UINT>(negCount)]) >> term1)[addNegNode] | (+chP('+') >> term1) | ('!' >> term1)[addLogNotNode] | ('~' >> term1)[addBitNotNode] |
			longestD[((intP >> chP('l'))[addLong] | (intP[addInt])) | ((realP >> chP('f'))[addFloat] | (realP[addDouble]))] |
			group |
			funccall[addFuncCallNode] |
			applyval >>
			(
				strP("++")[addPostIncNode] |
				strP("--")[addPostDecNode] |
				epsP[addGetNode]
			)[strPop][strPop];
		term2		=	term1 >> *((strP("**") >> term1)[addCmd(cmdPow)]);
		term3		=	term2 >> *(('*' >> term2)[addCmd(cmdMul)] | ('/' >> term2)[addCmd(cmdDiv)] | ('%' >> term2)[addCmd(cmdMod)]);
		term4		=	term3 >> *(('+' >> term3)[addCmd(cmdAdd)] | ('-' >> term3)[addCmd(cmdSub)]);
		term4_1		=	term4 >> *((strP("<<") >> term4)[addCmd(cmdShl)] | (strP(">>") >> term4)[addCmd(cmdShr)]);
		term4_2		=	term4_1 >> *(('<' >> term4_1)[addCmd(cmdLess)] | ('>' >> term4_1)[addCmd(cmdGreater)] | (strP("<=") >> term4_1)[addCmd(cmdLEqual)] | (strP(">=") >> term4_1)[addCmd(cmdGEqual)]);
		term4_4		=	term4_2 >> *((strP("==") >> term4_2)[addCmd(cmdEqual)] | (strP("!=") >> term4_2)[addCmd(cmdNEqual)]);
		term4_6		=	term4_4 >> *(strP("&") >> term4_4)[addCmd(cmdBitAnd)];
		term4_65	=	term4_6 >> *(strP("^") >> term4_6)[addCmd(cmdBitXor)];
		term4_7		=	term4_65 >> *(strP("|") >> term4_65)[addCmd(cmdBitOr)];
		term4_75	=	term4_7 >> *(strP("and") >> term4_7)[addCmd(cmdLogAnd)];
		term4_8		=	term4_75 >> *(strP("xor") >> term4_75)[addCmd(cmdLogXor)];
		term4_85	=	term4_8 >> *(strP("or") >> term4_8)[addCmd(cmdLogOr)];
		term4_9		=	term4_85 >> !('?' >> term5 >> ':' >> term5)[addIfElseTermNode];
		term5		=	(
			applyval >> (
			(strP("=") >> term5)[addSetNode] |
			(strP("+=") >> term5)[addAddSetNode] |
			(strP("-=") >> term5)[addSubSetNode] |
			(strP("*=") >> term5)[addMulSetNode] |
			(strP("/=") >> term5)[addDivSetNode] |
			(strP("^=") >> term5)[addPowSetNode] |
			(epsP[strPop][strPop][popTypeAndAddrNode] >> nothingP))
			)[strPop][strPop] |
			term4_9;

		block		=	chP('{')[blockBegin] >> code >> chP('}')[blockEnd];
		expression	=	*chP(';') >> ((strP("var") >> vardef >> +chP(';')) | breakexpr | ifexpr | forexpr | whileexpr | doexpr | switchexpr | retexpr | (term5 >> +chP(';'))[addPopNode] | block[addBlockNode]);
		code		=	((funcdef | expression) >> (code[addTwoExprNode] | epsP[addOneExprNode]));
	
		mySpaceP = spaceP | ((strP("//") >> *(anycharP - eolP)) | (strP("/*") >> *(anycharP - strP("*/")) >> strP("*/")));
	}
};

Compiler::Compiler(CommandList* cmds)
{
	cmdList = cmds;

	TypeInfo* info;
	info = new TypeInfo();
	info->name = "void";
	info->size = 0;
	info->type = TypeInfo::POD_VOID;
	typeVoid = info;
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "double";
	info->size = 8;
	info->type = TypeInfo::POD_DOUBLE;
	typeDouble = info;
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "float";
	info->size = 4;
	info->type = TypeInfo::POD_FLOAT;
	typeFloat = info;
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "long";
	info->size = 8;
	info->type = TypeInfo::POD_LONG;
	typeLong = info;
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "int";
	info->size = 4;
	info->type = TypeInfo::POD_INT;
	typeInt = info;
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "short";
	info->size = 2;
	info->type = TypeInfo::POD_SHORT;
	typeShort = info;
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "char";
	info->size = 1;
	info->type = TypeInfo::POD_CHAR;
	typeChar = info;
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "float2";
	info->type = TypeInfo::NOT_POD;
	info->AddMember("x", typeFloat);
	info->AddMember("y", typeFloat);
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "float3";
	info->type = TypeInfo::NOT_POD;
	info->AddMember("x", typeFloat);
	info->AddMember("y", typeFloat);
	info->AddMember("z", typeFloat);
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "float4";
	info->type = TypeInfo::NOT_POD;
	info->AddMember("x", typeFloat);
	info->AddMember("y", typeFloat);
	info->AddMember("z", typeFloat);
	info->AddMember("w", typeFloat);
	typeInfo.push_back(info);

	TypeInfo *typeFloat4 = info;

	info = new TypeInfo();
	info->name = "float4x4";
	info->type = TypeInfo::NOT_POD;
	info->AddMember("row1", typeFloat4);
	info->AddMember("row2", typeFloat4);
	info->AddMember("row3", typeFloat4);
	info->AddMember("row4", typeFloat4);
	typeInfo.push_back(info);

	CompilerGrammar::InitGrammar();

}
Compiler::~Compiler()
{
}

bool Compiler::Compile(string str)
{
	varInfoTop.clear();
	varInfo.clear();
	funcs.clear();

	callArgCount.clear();
	retTypeStack.clear();

	varDefined = 0;
	negCount = 0;
	varSize = 1;
	varTop = 24;
	needCopy = 0;

	varInfo.push_back(VariableInfo("ERROR", 0, typeDouble, 1, true));
	varInfo.push_back(VariableInfo("pi", 8, typeDouble, 1, true));
	varInfo.push_back(VariableInfo("e", 16, typeDouble, 1, true));
	varInfoTop.push_back(VarTopInfo(0,0));

	retTypeStack.push_back(NULL);	//global return can return anything

	logAST.str("");
	cmdList->Clear();

	SetCommandList(cmdList);
	SetFunctionList(&funcs);
	SetLogStream(&logAST);
	SetNodeList(&nodeList);

	if(getList()->size() != 0){
		getList()->pop_back();
	}

	ofstream m_FileStream("code.txt", std::ios::binary);
	m_FileStream << str;
	m_FileStream.flush();

	char* ptr = (char*)str.c_str();

	ofstream m_TempStream("time.txt", std::ios::binary);

	UINT t = GetTickCount();
	if(!Parse(CompilerGrammar::code, ptr, CompilerGrammar::mySpaceP))
		return false;
	UINT tem = GetTickCount()-t;
	m_TempStream << "Parsing and AST tree gen. time: " << tem << "ms\r\n";
	
	t = GetTickCount();
	if(getList()->back())
		getList()->back()->Compile();
	tem = GetTickCount()-t;
	m_TempStream << "Compile time: " << tem << "ms\r\n";

	m_TempStream.flush();

	ostringstream		graphlog;
	ofstream graphFile("graph.txt", std::ios::binary);
	if(getList()->back())
		getList()->back()->LogToStream(graphlog);
	graphFile << graphlog.str();
	graphFile.close();
	return true;
}

void Compiler::GenListing()
{
	UINT pos = 0, pos2 = 0;
	CmdID	cmd;
	//double	val;
	char	name[512];
	UINT	valind;
	logASM.str("");

	char* typeInfoS[] = { "int", "long", "float", "double" };
	char* typeInfoD[] = { "char", "short", "int", "long", "float", "double" };
	UINT typeSizeS[] = { 4, 8, 4, 8 };
	UINT typeSizeD[] = { 1, 2, 4, 8, 4, 8 };
	CmdFlag cFlag;
	OperFlag oFlag;
	while(cmdList->GetData(pos, cmd))
	{
		pos2 = pos;
		pos += 2;
		switch(cmd)
		{
		case cmdCallStd:
			{
				size_t len;
				cmdList->GetData(pos, len);
				pos += sizeof(size_t);
				if(len >= 511)
					break;
				cmdList->GetData(pos, name, len);
				pos += (UINT)len;
				name[len] = 0;
				logASM << dec << showbase << pos2 << dec << " CALLS " << name << ";";
			}
			break;
		case cmdPushVTop:
			logASM << dec << showbase << pos2 << dec << " PUSHT;";
			break;
		case cmdPopVTop:
			logASM << dec << showbase << pos2 << dec << " POPT;";
			break;
		case cmdCall:
			cmdList->GetData(pos, &valind, sizeof(UINT));
			pos += sizeof(UINT);
			logASM << dec << showbase << pos2 << " CALL " << valind << dec << ";";
			break;
		case cmdReturn:
			logASM << dec << showbase << pos2 << " RET " << dec << ";";
			break;
		case cmdPushV:
			{
				int valind;
				cmdList->GetData(pos, &valind, sizeof(int));
				pos += sizeof(int);
				logASM << dec << showbase << pos2 << " PUSHV " << valind << dec << ";";
			}
			break;
		case cmdNop:
			logASM << dec << showbase << pos2 << dec << " NOP;";
			break;
		case cmdCTI:
			logASM << dec << showbase << pos2 << dec << " CTI addr*";
			cmdList->GetUINT(pos, valind);
			pos += sizeof(UINT);
			logASM << valind;
			break;
		case cmdPush:
			{
				cmdList->GetUSHORT(pos, cFlag);
				pos += 2;
				logASM << pos2 << " PUSH ";
				logASM << typeInfoS[cFlag&0x00000003] << "<-";
				logASM << typeInfoD[(cFlag>>2)&0x00000007];

				asmStackType st = flagStackType(cFlag);
				asmDataType dt = flagDataType(cFlag);
				UINT	DWords[2];
				USHORT sdata;
				UCHAR cdata;
				int valind;
				if(flagNoAddr(cFlag)){
					if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG){
						cmdList->GetUINT(pos, DWords[0]); pos += 4;
						cmdList->GetUINT(pos, DWords[1]); pos += 4;
					}
					if(dt == DTYPE_FLOAT || dt == DTYPE_INT){ cmdList->GetUINT(pos, DWords[0]); pos += 4; }
					if(dt == DTYPE_SHORT){ cmdList->GetUSHORT(pos, sdata); pos += 2; DWords[0] = sdata; }
					if(dt == DTYPE_CHAR){ cmdList->GetUCHAR(pos, cdata); pos += 1; DWords[0] = cdata; }

					if(dt == DTYPE_DOUBLE)
						logASM << " (" << *((double*)(&DWords[0])) << ')';
					if(dt == DTYPE_LONG)
						logASM << " (" << *((long*)(&DWords[0])) << ')';
					if(dt == DTYPE_FLOAT)
						logASM << " (" << *((float*)(&DWords[0])) << dec << ')';
					if(dt == DTYPE_INT)
						logASM << " (" << *((int*)(&DWords[0])) << dec << ')';
					if(dt == DTYPE_SHORT)
						logASM << " (" << *((short*)(&DWords[0])) << dec << ')';
					if(dt == DTYPE_CHAR)
						logASM << " (" << *((char*)(&DWords[0])) << ')';
				}else{
					logASM << " PTR[";
					if(flagAddrStk(cFlag))
					{
						logASM << "stack";
						if(flagAddrRel(cFlag))
							logASM << "+top";
					}else{
						if(flagAddrRel(cFlag) || flagAddrAbs(cFlag))
						{
							cmdList->GetINT(pos, valind);
							pos += 4;
						}
						logASM << valind;
						if(flagAddrRel(cFlag))
							logASM << "+top";
					}
					if(flagShiftStk(cFlag))
						logASM << "+shift(stack)";
					if(flagShiftOn(cFlag))
					{
						cmdList->GetINT(pos, valind);
						pos += 4;
						logASM << "+" << valind;
					}
					logASM << "] ";
					if(flagSizeStk(cFlag))
						logASM << "size(stack)";
					if(flagSizeOn(cFlag))
					{
						cmdList->GetINT(pos, valind);
						pos += 4;
						logASM << "size(" << valind << ")";
					}
				}
			}
			
			break;
		case cmdMov:
			{
				cmdList->GetUSHORT(pos, cFlag);
				pos += 2;
				logASM << pos2 << " MOV ";
				logASM << typeInfoS[cFlag&0x00000003] << "->";
				logASM << typeInfoD[(cFlag>>2)&0x00000007] << " PTR[";
				asmStackType st = flagStackType(cFlag);
				asmDataType dt = flagDataType(cFlag);
				UINT	highDW = 0, lowDW = 0;
				int valind;

				if(flagAddrStk(cFlag))
				{
					logASM << "stack";
					if(flagAddrRel(cFlag))
						logASM << "+top";
					logASM << "]";
				}else{
					if(flagAddrRel(cFlag) || flagAddrAbs(cFlag))
					{
						cmdList->GetINT(pos, valind);
						pos += 4;
					}
					logASM << valind;
					if(flagAddrRel(cFlag))
						logASM << "+top";

					if(flagShiftStk(cFlag))
						logASM << "+shift(stack)";

					if(flagShiftOn(cFlag))
					{
						cmdList->GetINT(pos, valind);
						pos += 4;
						logASM << "+" << valind;
					}
					logASM << "] ";
					if(flagSizeStk(cFlag))
						logASM << "size(stack)";
					if(flagSizeOn(cFlag))
					{
						cmdList->GetINT(pos, valind);
						pos += 4;
						logASM << "size: " << valind;
					}
				}
			}
			break;
		case cmdPop:
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			logASM << pos2 << " POP ";
			logASM << typeInfoS[cFlag&0x00000003];
			break;
		case cmdRTOI:
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			logASM << pos2 << " RTOI ";
			logASM << typeInfoS[cFlag&0x00000003] << "->" << typeInfoD[(cFlag>>2)&0x00000007];
			break;
		case cmdITOR:
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			logASM << pos2 << " ITOR ";
			logASM << typeInfoS[cFlag&0x00000003] << "->" << typeInfoD[(cFlag>>2)&0x00000007];
			break;
		case cmdITOL:
			logASM << pos2 << " ITOL";
			break;
		case cmdLTOI:
			logASM << pos2 << " LTOI";
			break;
		case cmdSwap:
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			logASM << pos2 << " SWAP ";
			logASM << typeInfoS[cFlag&0x00000003] << "<->";
			logASM << typeInfoD[(cFlag>>2)&0x00000007];
			break;
		case cmdCopy:
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			logASM << pos2 << " COPY ";
			switch(oFlag)
			{
			case OTYPE_DOUBLE:
				logASM << " double;";
				break;
			case OTYPE_LONG:
				logASM << " long;";
				break;
			case OTYPE_INT:
				logASM << " int;";
				break;
			}
			break;
		case cmdJmp:
			cmdList->GetUINT(pos, valind);
			pos += 4;
			logASM << pos2 << " JMP " << valind;
			break;
		case cmdJmpZ:
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			cmdList->GetUINT(pos, valind);
			pos += 4;
			logASM << pos2 << " JMPZ";
			switch(oFlag)
			{
			case OTYPE_DOUBLE:
				logASM << " double";
				break;
			case OTYPE_LONG:
				logASM << " long";
				break;
			case OTYPE_INT:
				logASM << " int";
				break;
			}
			logASM << ' ' << valind << ';';
			break;
		case cmdJmpNZ:
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			cmdList->GetUINT(pos, valind);
			pos += 4;
			logASM << pos2 << " JMPNZ";
			switch(oFlag)
			{
			case OTYPE_DOUBLE:
				logASM << " double";
				break;
			case OTYPE_LONG:
				logASM << " long";
				break;
			case OTYPE_INT:
				logASM << " int";
				break;
			}
			logASM << ' ' << valind << ';';
			break;
		}
		if(cmd >= cmdAdd && cmd <= cmdLogXor)
		{
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			logASM << pos2 << ' ';
			switch(cmd)
			{
			case cmdAdd:
				logASM << "ADD";
				break;
			case cmdSub:
				logASM << "SUB";
				break;
			case cmdMul:
				logASM << "MUL";
				break;
			case cmdDiv:
				logASM << "DIV";
				break;
			case cmdPow:
				logASM << "POW";
				break;
			case cmdMod:
				logASM << "MOD";
				break;
			case cmdLess:
				logASM << "LES";
				break;
			case cmdGreater:
				logASM << "GRT";
				break;
			case cmdLEqual:
				logASM << "LEQL";
				break;
			case cmdGEqual:
				logASM << "GEQL";
				break;
			case cmdEqual:
				logASM << "EQL";
				break;
			case cmdNEqual:
				logASM << "NEQL";
				break;
			case cmdShl:
				logASM << "SHL";
				if(oFlag == OTYPE_DOUBLE)// || oFlag == OTYPE_FLOAT)
					throw string("Invalid operation: SHL used on float");
				break;
			case cmdShr:
				logASM << "SHR";
				if(oFlag == OTYPE_DOUBLE)// || oFlag == OTYPE_FLOAT)
					throw string("Invalid operation: SHR used on float");
				break;
			case cmdBitAnd:
				logASM << "BAND";
				if(oFlag == OTYPE_DOUBLE)// || oFlag == OTYPE_FLOAT)
					throw string("Invalid operation: BAND used on float");
				break;
			case cmdBitOr:
				logASM << "BOR";
				if(oFlag == OTYPE_DOUBLE)// || oFlag == OTYPE_FLOAT)
					throw string("Invalid operation: BOR used on float");
				break;
			case cmdBitXor:
				logASM << "BXOR";
				if(oFlag == OTYPE_DOUBLE)// || oFlag == OTYPE_FLOAT)
					throw string("Invalid operation: BXOR used on float");
				break;
			case cmdLogAnd:
				logASM << "LAND";
				break;
			case cmdLogOr:
				logASM << "LOR";
				break;
			case cmdLogXor:
				logASM << "LXOR";
				break;
			}
			switch(oFlag)
			{
			case OTYPE_DOUBLE:
				logASM << " double;";
				break;
			case OTYPE_LONG:
				logASM << " long;";
				break;
			case OTYPE_INT:
				logASM << " int;";
				break;
			default:
				logASM << "ERROR: OperFlag expected after instruction";
			}
		}
		if(cmd >= cmdNeg && cmd <= cmdLogNot)
		{
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			logASM << pos2 << ' ';
			switch(cmd)
			{
			case cmdNeg:
				logASM << "NEG";
				break;
			case cmdInc:
				logASM << "INC";
				break;
			case cmdDec:
				logASM << "DEC";
				break;
			case cmdBitNot:
				logASM << "BNOT";
				if(oFlag == OTYPE_DOUBLE)// || oFlag == OTYPE_FLOAT)
					throw string("Invalid operation: BNOT used on float");
				break;
			case cmdLogNot:
				logASM << "LNOT;";
				break;
			}
			switch(oFlag)
			{
			case OTYPE_DOUBLE:
				logASM << " double;";
				break;
			case OTYPE_LONG:
				logASM << " long;";
				break;
			case OTYPE_INT:
				logASM << " int;";
				break;
			default:
				logASM << "ERROR: OperFlag expected after ";
			}
		}
		if(cmd >= cmdIncAt && cmd <= cmdDecAt)
		{
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			if(cmd == cmdIncAt)
				logASM << pos2 << " INCAT ";
			if(cmd == cmdDecAt)
				logASM << pos2 << " DECAT ";
			logASM << typeInfoD[(cFlag>>2)&0x00000007] << " PTR[";
			asmStackType st = flagStackType(cFlag);
			asmDataType dt = flagDataType(cFlag);
			UINT	highDW = 0, lowDW = 0;
			int valind;

			if(flagAddrStk(cFlag)){
				logASM << "stack";
				if(flagAddrRel(cFlag))
					logASM << "+top";
				logASM << "]";
			}else{
				if(flagAddrRel(cFlag) || flagAddrAbs(cFlag)){
					cmdList->GetINT(pos, valind);
					pos += 4;
				}
				logASM << valind;
				if(flagAddrRel(cFlag))
					logASM << "+top";

				if(flagShiftStk(cFlag)){
					logASM << "+shift";
				}
				if(flagShiftOn(cFlag)){
					cmdList->GetINT(pos, valind);
					pos += 4;
					logASM << "+" << valind;
				}
				logASM << "] ";
				if(flagSizeStk(cFlag)){
					logASM << "size: stack";
				}
				if(flagSizeOn(cFlag)){
					cmdList->GetINT(pos, valind);
					pos += 4;
					logASM << "size: " << valind;
				}
			}
		}
		logASM << "\r\n";
	}

	ofstream m_FileStream("asm.txt", std::ios::binary);
	m_FileStream << logASM.str();
	m_FileStream.flush();
}

string Compiler::GetListing()
{
	return logASM.str();
}

string Compiler::GetLog()
{
	return logAST.str();
}

std::vector<VariableInfo>* Compiler::GetVariableInfo()
{
	return &varInfo;
}

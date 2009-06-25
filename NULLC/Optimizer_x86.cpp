#pragma warning(disable: 4786)    // Надо-ли? Но, не нравиццо.

#include "stdafx.h"
#include "Optimizer_x86.h"

std::vector<std::string> Strings;

struct Command_def
{
	char* Name;
	int	  Hash;
	int   Size;
};

struct Argument_def
{
	char*			Name;
	Argument::Type	Hash;
	int				Size;
};

// Check if type is a general register (eax, ebx, ecx, edx)
static bool isGenReg[] = { false, false, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false };
static char* argTypeToStr[] = { NULL, NULL, "eax", "ebx", "ecx", "edx", "edi", "esi", "esp", "ebp", "ax", "al", "bx", "bl", NULL, NULL, NULL };
static Argument_def Argument_Table[] = {
		"eax", Argument::eax, 3,
		"ebx", Argument::ebx, 3,
		"ecx", Argument::ecx, 3,
		"edx", Argument::edx, 3,
		"edi", Argument::edi, 3,
		"esi", Argument::esi, 3,
		"esp", Argument::esp, 3,
		"ebp", Argument::ebp, 3,
		"ax", Argument::ax, 2,
		"al", Argument::al, 2,
		"bx", Argument::bx, 2,
		"bl", Argument::bl, 2,
};

std::vector<Command> Commands;

Command_def Commands_table[] = {

	"none",		o_none,	sizeof("none"),
	"mov",		o_mov,	sizeof("mov"),
	"movsx",	o_movsx,sizeof("movsx"),
	"push",		o_push,	sizeof("push"),
	"pop",		o_pop,	sizeof("pop"),
	"lea",		o_lea,	sizeof("lea"),
	"xchg",		o_xchg,	sizeof("xchg"),
	"cdq",		o_cdq,	sizeof("cdq"),
	"rep movsd",o_rep_movsd,sizeof("rep movsd"),

	"jmp",		o_jmp,	sizeof("jmp"),
	"ja",		o_ja,	sizeof("ja"),
	"jae",		o_jae,	sizeof("jae"),
	"jb",		o_jb,	sizeof("jb"),
	"jbe",		o_jbe,	sizeof("jbe"),
	"jc",		o_jc,	sizeof("jc"),
	"je",		o_je,	sizeof("je"),
	"jz",		o_jz,	sizeof("jz"),
	"jg",		o_jg,	sizeof("jg"),
	"jl",		o_jl,	sizeof("jl"),
	"jne",		o_jne,	sizeof("jne"),
	"jnp",		o_jnp,	sizeof("jnp"),
	"jnz",		o_jnz,	sizeof("jnz"),
	"jp",		o_jp,	sizeof("jp"),	     // Now its not full list, if you will add commands, calculate correct value in IsJump function
	"call",		o_call,	sizeof("call"),
	"ret",		o_ret,	sizeof("ret"),

	"fld",		o_fld,		sizeof("fld"),
	"fild",		o_fild,		sizeof("fild"),
	"fistp",	o_fistp,	sizeof("fistp"),
	"fst",		o_fst,		sizeof("fst"),
	"fstp",		o_fstp,		sizeof("fstp"),
	"fnstsw",	o_fnstsw,	sizeof("fnstsw"),
	"fstcw",	o_fstcw,	sizeof("fstcw"),
	"fldcw",	o_fldcw,	sizeof("fldcw"),

	"neg",		o_neg,	sizeof("neg"),
	"add",		o_add,	sizeof("add"),
	"adc",		o_adc,	sizeof("adc"),
	"sub",		o_sub,	sizeof("sub"),
	"sbb",		o_sbb,	sizeof("sbb"),
	"imul",		o_imul,	sizeof("imul"),
	"idiv",		o_idiv,	sizeof("idiv"),
	"shl",		o_shl,	sizeof("shl"),
	"sal",		o_sal,	sizeof("sal"),
	"sar",		o_sar,	sizeof("sar"),
	"not",		o_not,	sizeof("not"),
	"and",		o_and,	sizeof("and"),
	"or",		o_or,	sizeof("or"),
	"xor",		o_xor,	sizeof("xor"),
	"cmp",		o_cmp,	sizeof("cmp"),
	"test",		o_test,	sizeof("test"),

	"setl",		o_setl,		sizeof("setl"),
	"setg",		o_setg,		sizeof("setg"),
	"setle",	o_setle,	sizeof("setle"),
	"setge",	o_setge,	sizeof("setge"),
	"sete",		o_sete,		sizeof("sete"),
	"setne",	o_setne,	sizeof("setne"),
	"setz",		o_setz,		sizeof("setz"),
	"setnz",	o_setnz,	sizeof("setnz"),

	"fadd",		o_fadd,		sizeof("fadd"),
	"faddp",	o_faddp,	sizeof("faddp"),
	"fmul",		o_fmul,		sizeof("fmul"),
	"fmulp",	o_fmulp,	sizeof("fmulp"),
	"fsub",		o_fsub,		sizeof("fsub"),
	"fsubr",	o_fsubr,	sizeof("fsubr"),
	"fsubp",	o_fsubp,	sizeof("fsubp"),
	"fsubrp",	o_fsubrp,	sizeof("fsubrp"),
	"fdiv",		o_fdiv,		sizeof("fdiv"),
	"fdivr",	o_fdivr,	sizeof("fdivr"),
	"fdivrp",	o_fdivrp,	sizeof("fdivrp"),
	"fchs",		o_fchs,		sizeof("fchs"),
	"fprem",	o_fprem,	sizeof("fprem"),
	"fcomp",	o_fcomp,	sizeof("fcomp"),
	"fldz",		o_fldz,		sizeof("fldz"),
	"fld1",		o_fld1,		sizeof("fld1"),
	"fsincos",	o_fsincos,	sizeof("fsincos"),
	"fptan",	o_fptan,	sizeof("fptan"),
	"fsqrt",	o_fsqrt,	sizeof("fsqrt"),
	"frndint",	o_frndint,	sizeof("frndint"),

	"int",		o_int,		sizeof("int"),
	"dd",		o_dd,		sizeof("dd"),
	"label:",	o_label,	sizeof("label:"),
	"other",	o_other,	sizeof("other"),
};

const int Commands_table_size = sizeof(Commands_table) / sizeof(Command_def);

// Функция определяет параметры аргумента по строке
void ClassifyArgument(Argument& arg, const char* str)
{
	bool flag = false;
	arg.num = 0;
	arg.ptrSize = Argument::snone;
	arg.fpArg = 0;

	if(str == NULL || *str == 0)
	{
		arg.type = Argument::none;
		arg.size = 0;
		flag = true;
	}else if(*str >= '0' && *str <= '9'){
		arg.type = Argument::number;
		arg.num = atoi(str);
		arg.size = (strchr(str, ',') ? (char)(strchr(str, ',') - str) : (char)strlen(str));
		flag = true;
	}else if(*str == '[' || memcmp(str, "byte", 4) == 0 || memcmp(str, "word", 4) == 0 || memcmp(str, "dword", 5) == 0 || memcmp(str, "qword", 5) == 0){
		arg.type = Argument::ptr;
		if(*str == '[')
			arg.ptrSize = Argument::dword;
		else if(memcmp(str, "byte", 4) == 0)
			arg.ptrSize = Argument::byte;
		else if(memcmp(str, "word", 4) == 0)
			arg.ptrSize = Argument::word;
		else if(memcmp(str, "dword", 4) == 0)
			arg.ptrSize = Argument::dword;
		else if(memcmp(str, "qword", 4) == 0)
			arg.ptrSize = Argument::qword;

		const char* ptrArgs = strchr(str, '[');
		assert(ptrArgs != NULL);
		ptrArgs++;
		arg.ptrReg[0] = Argument::none;
		arg.ptrReg[1] = Argument::none;
		arg.ptrNum = 0;
		arg.ptrMult = 1;
		for(int n = 0; n < 3 && ptrArgs; n++)
		{
			if(*ptrArgs >= '0' && *ptrArgs <= '9')
			{
				arg.ptrNum = atoi(ptrArgs);
			}else{
				assert(n < 2);
				for(int i = 0; i < 12; i++)
				{
					if(memcmp(ptrArgs, Argument_Table[i].Name, Argument_Table[i].Size) == 0)
					{
						arg.ptrReg[n] = Argument_Table[i].Hash;
						break;
					}
				}
			}
			if(const char *ptrMult = strchr(ptrArgs, '*'))
				arg.ptrMult = atoi(ptrMult);

			ptrArgs = strchr(ptrArgs, '+');
			if(ptrArgs)
				while(!isalnum(*ptrArgs))
					ptrArgs++;
			if(ptrArgs > strchr(str, ']'))
				ptrArgs = NULL;
		}
		
		if(strchr(str, ']') != 0)
		{
			arg.size = char(strchr(str, ']') + 1 - str);
		}else{
			const char *temp;
			temp = strchr(str, 0);

			while(*temp == ' ' || *temp == '\t')
				temp = temp - 1;

			arg.size = (char)(temp + 1 - str);
		}
		flag = true;
	}else if(str[0] == 's' && str[1] == 't'){
		arg.fpArg = isdigit(str[2]) ? str[2]-'0' : 0;
	}else{
		for(int i = 0; i < 12; i++)
		{
			if(memcmp(str, Argument_Table[i].Name, Argument_Table[i].Size) == 0)
			{
				arg.type = Argument_Table[i].Hash;
				arg.size = (char)Argument_Table[i].Size;
				flag = true;
				break;
			}
		}
	}
	if(flag == false)
	{
		if(strchr(str, ',') == NULL && strlen(str) > 4)
		{
			arg.type = Argument::label;
			arg.size = (strchr(str, ',') ? (char)(strchr(str, ',') - str) : (char)strlen(str));
		}else{
			arg.type = Argument::reg;
			arg.size = (strchr(str, ',') ? (char)(strchr(str, ',') - str) : (char)strlen(str));
		}
	}
}

// Функция определяет параметры команды и аргументов по строке
void ClassifyInstruction(Command& cmd, const char *strRep)
{
	const char* temp = strRep;

	// By default, we don't know, what command this is
	cmd.Name = o_other;
	// Compare it to all known commands
	for(int b = 0; b < Commands_table_size; b++)
	{
		if(strncmp(Commands_table[b].Name, temp, Commands_table[b].Size - 1) == 0 && !isalpha(*(temp+Commands_table[b].Size - 1)))
		{
			cmd.Name = (Command_Hash)Commands_table[b].Hash;
			break;
		}
	}

	if(strchr(temp, ':'))
		cmd.Name = o_label;

	// Find the first argument
	temp = strchr(temp, ' ');
	while(temp && *temp == ' ')
		temp++;
	// Find out, if there is an argument, and what kind of argument it is
	ClassifyArgument(cmd.argA, temp);
	// If argument was valid, save offset to argument start position
	if(cmd.argA.type != Argument::none)
		cmd.argA.begin = char(temp - strRep);
	// And try to find second argument
	if(temp && cmd.argA.type != Argument::none)
	{
		temp += cmd.argA.size;
		while(*temp && *temp == ' ' || *temp == ',')
			temp++;
		// Find out, if there is an argument, and what kind of argument it is
		ClassifyArgument(cmd.argB, temp);
		// If argument was valid, save offset to argument start position
		if(cmd.argB.type != Argument::none)
			cmd.argB.begin = char(temp - strRep);
	}
}

std::vector<Command>* Optimizer_x86::HashListing(const char* pListing, int strSize)
{
	// Create text without comments, empty lines and other trash
	UINT originalSize = strSize;
	char *clearText = new char[originalSize+1];
	char *currPos = clearText;
	for(UINT i = 0; i < originalSize; i++)
	{
		// Skip everything before command name or comment
		while(!((pListing[i] >= 'a' && pListing[i] <= 'z') || pListing[i] == ';'))
			i++;
		// Skip comment text
		if(pListing[i] == ';')
			while(pListing[i] != '\n')
				i++;
		// Copy text, until it is over by comment or line break
		while(pListing[i] != '\n' && pListing[i] != ';')
		{
			*currPos = (pListing[i] == '\t' ? ' ' : pListing[i]);
			i++;
			currPos++;
		}
		// If it was ended with an comment, add line break
		if(pListing[i] == ';')
		{
			if(*(currPos-1) == ' ')
			{
				*(currPos-1) = '\n';
			}else{
				*currPos = '\n';
				currPos++;
			}
			i--;
		}
		// If it was ended with caret return, replace it with line break
		if(*(currPos-1) == '\r')
			*(currPos-1) = '\n';
	}
	*currPos = 0;

	Strings.clear();
	Commands.clear();

	HashListing(clearText);

	delete[] clearText;

	return &Commands;
}

std::vector<std::string>* Optimizer_x86::Optimize()
{
	OptimizePushPop();
	OptimizePushPop();

	// Strings contain the optimized code
	return &Strings;
}

bool CheckDependencies(int start, int end, Argument::Type dependency, bool checkESPChange, bool checkFlowControl)
{
	for(int i = start; i <= end; i++)
	{
		if(checkESPChange && strstr(Strings[i].c_str(), "esp") || Commands[i].Name == o_push || Commands[i].Name == o_pop)
			return true;
		if(checkFlowControl && Commands[i].Name >= o_jmp && Commands[i].Name <= o_call)
			return true;
		if(checkFlowControl && Commands[i].Name == o_label)
			return true;
		if(Commands[i].argA.type == dependency || Commands[i].argB.type == dependency || (argTypeToStr[dependency] && strstr(Strings[i].c_str()+Commands[i].argA.begin, argTypeToStr[dependency])))
			return true;
	}
	return false;
}

void Optimizer_x86::OptimizePushPop()
{
	int optimize_count = 0;

	for(UINT i = 0; i < Commands.size(); i++)
	{
		// Optimizations for "push num ... pop [location]" and "push register ... pop location"
		if(Commands[i].Name == o_pop && Commands[i].argA.type == Argument::ptr)
		{
			// Search up to find "push num" or "push reg"
			int pushIndex = i-1;
			while(Commands[pushIndex].Name != o_push && pushIndex > int(i)-10 && pushIndex > 0)
				pushIndex--;
			if(Commands[pushIndex].Name == o_push && (Commands[pushIndex].argA.type == Argument::number || isGenReg[Commands[pushIndex].argA.type]) &&
				!CheckDependencies(pushIndex+1, i-1, (Commands[pushIndex].argA.type == Argument::number ? Argument::label : Commands[pushIndex].argA.type), true, true))
			{
				Strings[i].replace(0, 3, "mov");
				Strings[i] += ", " + std::string(Strings[pushIndex].c_str()+Commands[pushIndex].argA.begin, Commands[pushIndex].argA.size);
				Strings[pushIndex] = "";

				// Update instruction information
				ClassifyInstruction(Commands[i], Strings[i].c_str());
				ClassifyInstruction(Commands[pushIndex], Strings[pushIndex].c_str());

				++optimize_count;
			}
		}
		// Optimizations for "push num ... pop reg", "push [location] ... pop reg" and "push regA ... pop regB"
		if(Commands[i].Name == o_pop && isGenReg[Commands[i].argA.type])
		{
			// Search up to find "push" command
			int pushIndex = i-1;
			while(Commands[pushIndex].Name != o_push && pushIndex > int(i)-10 && pushIndex > 0)
				pushIndex--;
			// For first two cases
			if(Commands[pushIndex].Name == o_push && (Commands[pushIndex].argA.type == Argument::number || (Commands[pushIndex].argA.type == Argument::ptr && i-pushIndex<=2)) &&
				!CheckDependencies(pushIndex+1, i-1, Argument::label, true, true))
			{
				Strings[i].replace(0, 3, "mov");
				Strings[i] += ", " + std::string(Strings[pushIndex].c_str()+Commands[pushIndex].argA.begin, Commands[pushIndex].argA.size);
				Strings[pushIndex] = "";

				// Update instruction information
				ClassifyInstruction(Commands[i], Strings[i].c_str());
				ClassifyInstruction(Commands[pushIndex], Strings[pushIndex].c_str());

				++optimize_count;
			}
			// For the third case
			if(Commands[pushIndex].Name == o_push && isGenReg[Commands[pushIndex].argA.type] &&
				!CheckDependencies(pushIndex+1, i-1, Commands[pushIndex].argA.type, true, true))
			{
				if(Commands[i].argA.type == Commands[pushIndex].argA.type)
				{
					Strings[i] = "";
					Strings[pushIndex] = "";
				}else{
					Strings[i].replace(0, 3, "mov");
					Strings[i] += ", " + std::string(Strings[pushIndex].c_str()+Commands[pushIndex].argA.begin, Commands[pushIndex].argA.size);
					Strings[pushIndex] = "";
				}

				// Update instruction information
				ClassifyInstruction(Commands[i], Strings[i].c_str());
				ClassifyInstruction(Commands[pushIndex], Strings[pushIndex].c_str());

				++optimize_count;
			}
		}
		if(Commands[i].Name == o_fld && strstr(Strings[i].c_str(), "qword [esp]"))
		{
			// push dword [a+4], push dword[a], fld [esp]
			if(Commands[i-1].Name == o_push && Commands[i-1].argA.type == Argument::ptr && Commands[i-2].Name == o_push && Commands[i-2].argA.type == Argument::ptr)
			{
				Strings[i] = "fld qword " + std::string(Strings[i-1].c_str()+Commands[i-1].argA.begin+6, Commands[i-1].argA.size-6);
				Strings[i-1] = "";
				Strings[i-2] = "sub esp, 8";

				// Update instruction information
				ClassifyInstruction(Commands[i], Strings[i].c_str());
				ClassifyInstruction(Commands[i-1], Strings[i-1].c_str());
				ClassifyInstruction(Commands[i-2], Strings[i-2].c_str());

				++optimize_count;
			// push dword [a+4], push dword [a], fld ... [], fld dword [esp]
			}else if(Commands[i-2].Name == o_push && Commands[i-2].argA.type == Argument::ptr && Commands[i-3].Name == o_push && Commands[i-3].argA.type == Argument::ptr)
			{
				Strings[i] = "fld qword " + std::string(Strings[i-2].c_str()+Commands[i-2].argA.begin+6, Commands[i-2].argA.size-6);
				if(Strings[i] == Strings[i-1])
					Strings[i] = "fld st0";

				Strings[i-2] = "";
				Strings[i-3] = "sub esp, 8";

				// Update instruction information
				ClassifyInstruction(Commands[i], Strings[i].c_str());
				ClassifyInstruction(Commands[i-2], Strings[i-2].c_str());
				ClassifyInstruction(Commands[i-3], Strings[i-3].c_str());

				++optimize_count;
			// push num, push num, fld qword [esp]
			}else if(Commands[i-1].Name == o_push && Commands[i-1].argA.type == Argument::number && Commands[i-2].Name == o_push && Commands[i-2].argA.type == Argument::number)
			{
				if(atoi(Strings[i-1].c_str() + Commands[i-1].argA.begin) == 0 && atoi(Strings[i-2].c_str() + Commands[i-2].argA.begin) == 0)
					Strings[i] = "fldz";
				else if(atoi(Strings[i-1].c_str() + Commands[i-1].argA.begin) == 0 && atoi(Strings[i-2].c_str() + Commands[i-2].argA.begin) == 0x3ff00000)
					Strings[i] = "fld1";
				else
					continue;

				Strings[i-1] = "";
				Strings[i-2] = "sub esp, 8";

				// Update instruction information
				ClassifyInstruction(Commands[i], Strings[i].c_str());
				ClassifyInstruction(Commands[i-1], Strings[i-1].c_str());
				ClassifyInstruction(Commands[i-2], Strings[i-2].c_str());

				++optimize_count;
			}
		}
		if(Commands[i].Name == o_fld && (strstr(Strings[i].c_str(), "qword") || strstr(Strings[i].c_str(), "st")))
		{
			// fld qword [esp], faddp
			// fld stN, faddp
			if(Commands[i+1].Name == o_faddp)
			{
				Strings[i+1] = "fadd " + std::string(Strings[i].c_str()+Commands[i].argA.begin, Commands[i].argA.size);
				if(strstr(Strings[i].c_str(), "st"))
					Strings[i+1] += ", " + std::string(Strings[i].c_str()+Commands[i].argA.begin, Commands[i].argA.size);
				Strings[i] = "";

				// Update instruction information
				ClassifyInstruction(Commands[i], Strings[i].c_str());
				ClassifyInstruction(Commands[i+1], Strings[i+1].c_str());

				++optimize_count;
			}
			// fld qword [esp], fmulp
			// fld stN, fmulp
			if(Commands[i+1].Name == o_fmulp)
			{
				Strings[i+1] = "fmul " + std::string(Strings[i].c_str()+Commands[i].argA.begin, Commands[i].argA.size);
				if(strstr(Strings[i].c_str(), "st"))
					Strings[i+1] += ", " + std::string(Strings[i].c_str()+Commands[i].argA.begin, Commands[i].argA.size);
				Strings[i] = "";

				// Update instruction information
				ClassifyInstruction(Commands[i], Strings[i].c_str());
				ClassifyInstruction(Commands[i+1], Strings[i+1].c_str());

				++optimize_count;
			}
			// fld qword [esp], fsubrp
			// fld stN, fsubrp
			if(Commands[i+1].Name == o_fsubrp)
			{
				Strings[i+1] = "fsubr " + std::string(Strings[i].c_str()+Commands[i].argA.begin, Commands[i].argA.size);
				if(strstr(Strings[i].c_str(), "st"))
					Strings[i+1] += ", " + std::string(Strings[i].c_str()+Commands[i].argA.begin, Commands[i].argA.size);
				Strings[i] = "";

				// Update instruction information
				ClassifyInstruction(Commands[i], Strings[i].c_str());
				ClassifyInstruction(Commands[i+1], Strings[i+1].c_str());

				++optimize_count;
			}
			// fld qword [esp], fdivrp
			// fld stN, fdivrp
			if(Commands[i+1].Name == o_fdivrp)
			{
				Strings[i+1] = "fdivr " + std::string(Strings[i].c_str()+Commands[i].argA.begin, Commands[i].argA.size);
				if(strstr(Strings[i].c_str(), "st"))
					Strings[i+1] += ", " + std::string(Strings[i].c_str()+Commands[i].argA.begin, Commands[i].argA.size);
				Strings[i] = "";

				// Update instruction information
				ClassifyInstruction(Commands[i], Strings[i].c_str());
				ClassifyInstruction(Commands[i+1], Strings[i+1].c_str());

				++optimize_count;
			}
		}
		if(Commands[i].Name == o_fstp && strstr(Strings[i].c_str(), "qword [esp]"))
		{
			// fstp qword [esp], fld qword [esp]
			if(Commands[i+1].Name == o_fld && strstr(Strings[i+1].c_str(), "qword [esp]"))
			{
				Strings[i] = "";
				Strings[i+1] = "";

				// Update instruction information
				ClassifyInstruction(Commands[i], Strings[i].c_str());
				ClassifyInstruction(Commands[i+1], Strings[i+1].c_str());

				++optimize_count;
			// fstp qword [esp], pop dword [a], pop dword [a+4]
			}else if(Commands[i+1].Name == o_pop && Commands[i+1].argA.type == Argument::ptr && Commands[i+2].Name == o_pop && Commands[i+2].argA.type == Argument::ptr){
				Strings[i] = "fstp qword " + std::string(Strings[i+1].c_str()+Commands[i+1].argA.begin+6, Commands[i+1].argA.size-6);
				Strings[i+1] = "";
				Strings[i+2] = "add esp, 8";

				// Update instruction information
				ClassifyInstruction(Commands[i], Strings[i].c_str());
				ClassifyInstruction(Commands[i+1], Strings[i+1].c_str());
				ClassifyInstruction(Commands[i+2], Strings[i+2].c_str());

				++optimize_count;
			}
		}
		if(Commands[i].Name == o_add && Commands[i].argA.type == Argument::reg && Commands[i].argB.type == Argument::number)
		{
			if(Commands[i+1].Name == o_sub && Commands[i+1].argA.type == Commands[i].argA.type && strcmp(Strings[i].c_str()+Commands[i].argB.begin, Strings[i+1].c_str()+Commands[i+1].argB.begin) == 0)
			{
				Strings[i] = "";
				Strings[i+1] = "";

				// Update instruction information
				ClassifyInstruction(Commands[i], Strings[i].c_str());
				ClassifyInstruction(Commands[i+1], Strings[i+1].c_str());

				++optimize_count;
			}
		}
	}
}

void Optimizer_x86::HashListing(const char* pListing)
{
	// Divide code into small strings (for every line)
	const char* pString = pListing, *endString;
	while((endString = strchr(pString, '\n')) != NULL)
	{	
		Strings.push_back(std::string(pString, endString));
		pString = endString + 1;
	}
	Commands.resize(Strings.size());

	// Classify instruction
	for(UINT n = 0; n < Strings.size(); n++)
	{
		Commands[n].strName = &Strings[n];
		ClassifyInstruction(Commands[n], Strings[n].c_str());
	}
}

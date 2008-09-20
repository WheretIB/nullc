#pragma warning(disable: 4786)    // ����-��? ��, �� ��������.

#include "stdafx.h"
#include "Optimizer_x86.h"

std::vector<std::string> Strings;

struct Command_def
{
	char* Name;
	int	  Hash;
	int   Size;
};

enum Command_Hash
{
	none,
	push,
	pop,
	jmp,
	ja,
	jae,
	jb,
	jbe,
	jc,
	je,
	jz,
	jg,
	jl,
	jne,
	jnp,
	jnz,
	jp,
	call,
	fld,
	fstp,
	add,
	sub,
	other,
};

// Check if type is a general register (eax, ebx, ecx, edx)
static bool isGenReg[] = { false, false, true, true, true, true, true, true, false, false, false };

struct Argument
{
	// Argument type
	enum Type{ none, number, eax, ebx, ecx, edx, edi, esi, reg, ptr, label };

	char	begin, size;
	Type	type;
};

struct Command
{
	Command_Hash Name;
	std::string* strName;	// pointer to command in text form
	Argument	argA, argB, argC;
};

std::vector<Command> Commands;

Command_def Commands_table[] = {

	"none"	,  0, sizeof("none"),
	"push"	,  1, sizeof("push"),
	"pop"	,  2, sizeof("pop"),
	"jmp"	,  3, sizeof("jmp"),
	"ja"	,  4, sizeof("ja"),
	"jae"	,  5, sizeof("jae"),
	"jb"	,  6, sizeof("jb"),
	"jbe"	,  7, sizeof("jbe"),
	"jc"	,  8, sizeof("jc"),
	"je"	,  9, sizeof("je"),
	"jz"	, 10, sizeof("jz"),
	"jg"	, 11, sizeof("jg"),
	"jl"	, 12, sizeof("jl"),
	"jne"	, 13, sizeof("jne"),
	"jnp"	, 14, sizeof("jnp"),
	"jnz"	, 15, sizeof("jnz"),
	"jp"	, 16, sizeof("jp"),	     // Now its not full list, if you will add commands, calculate correct value in IsJump function
	"call"	, 17, sizeof("call"),
	"fld"	, 18, sizeof("fld"),
	"fstp"	, 19, sizeof("fstp"),
	"add"	, 20, sizeof("add"),
	"sub"	, 21, sizeof("sub"),
	"other"	, 22, sizeof("other"),
};

const int Commands_table_size = sizeof(Commands_table) / sizeof(Command_def);

// ������� ���������� ��������� ��������� �� ������
void ClassifyArgument(Argument& arg, const char* str)
{
	if(str == NULL || *str == 0)
	{
		arg.type = Argument::none;
		arg.size = 0;
	}else if(*str >= '0' && *str <= '9'){
		arg.type = Argument::number;
		arg.size = (strchr(str, ',') ? (char)(strchr(str, ',') - str) : (char)strlen(str));
	}else if(*str == '[' || memcmp(str, "byte", 4) == 0 || memcmp(str, "word", 4) == 0 || memcmp(str, "dword", 5) == 0 || memcmp(str, "qword", 5) == 0){
		arg.type = Argument::ptr;
		arg.size = char(strchr(str, ']') + 1 - str);
	}else if(memcmp(str, "eax", 3) == 0){
		arg.type = Argument::eax;
		arg.size = 3;
	}else if(memcmp(str, "ebx", 3) == 0){
		arg.type = Argument::ebx;
		arg.size = 3;
	}else if(memcmp(str, "ecx", 3) == 0){
		arg.type = Argument::ecx;
		arg.size = 3;
	}else if(memcmp(str, "edx", 3) == 0){
		arg.type = Argument::edx;
		arg.size = 3;
	}else if(memcmp(str, "edi", 3) == 0){
		arg.type = Argument::edi;
		arg.size = 3;
	}else if(memcmp(str, "esi", 3) == 0){
		arg.type = Argument::esi;
		arg.size = 3;
	}else if(strchr(str, ',') == NULL && strlen(str) > 4){
		arg.type = Argument::label;
		arg.size = (strchr(str, ',') ? (char)(strchr(str, ',') - str) : (char)strlen(str));
	}else{
		arg.type = Argument::reg;
		arg.size = (strchr(str, ',') ? (char)(strchr(str, ',') - str) : (char)strlen(str));
	}
}

// ������� ���������� ��������� ������� � ���������� �� ������
void ClassifyInstruction(Command& cmd, const char *strRep)
{
	const char* temp = strRep;

	// By default, we don't know, what command this is
	cmd.Name = other;
	// Compare it to all known commands
	for(int b = 0; b < Commands_table_size; b++)
	{
		if(strncmp(Commands_table[b].Name, temp, Commands_table[b].Size - 1) == 0 && !isalpha(*(temp+Commands_table[b].Size - 1)))
		{
			cmd.Name = (Command_Hash)Commands_table[b].Hash;
			break;
		}
	}

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

std::vector<std::string>* Optimizer_x86::Optimize(const char* pListing, int strSize)
{
	// Create text without comments, empty lines and other trash
	UINT originalSize = strSize;
	char *clearText = new char[originalSize];
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

	Strings.clear();
	Commands.clear();

	HashListing(clearText);
	OptimizePushPop();

	delete[] clearText;
	// Strings contain the optimized code
	return &Strings;
}

void Optimizer_x86::OptimizePushPop()
{
	int optimize_count = 0;

	for(UINT i = 0; i < Commands.size(); i++)
	{
		if(Commands[i].Name == pop && isGenReg[Commands[i].argA.type])
		{
			int n = i;
			bool flag = true;

			// Find push command
			while(Commands[n].Name != push && n > 0)
				n = n - 1;

			// Check, if the register value is being pushed to stack
			if(!isGenReg[Commands[n].argA.type])
				continue;	// Can't optimize, skip

			for(UINT m = n + 1; m < i; m++)
			{
				if(Commands[m].Name == call || IsJump(Commands[m].Name) == true)
					flag = false;

				// There are a few other situations, when the optimization is unavailable
				if(Commands[m].Name != none)
				{
					//char text[32] = "";

					// If someone works with stack pointer
					if(strstr(Strings[m].c_str(), "esp") != 0)
						flag = false;

					// If there is a label between them
					if(strchr(Strings[m].c_str(), ':') != 0)
						flag = false;

					// Or if someone modifies register that was pushed on stack
					if(Commands[n].argA.type == Commands[m].argA.type)
						flag = false;
				}
			}
			if(!flag)
				continue;	// Can't optimize, skip

			// If the register is the same
			if(Commands[n].argA.type == Commands[i].argA.type)
			{
				// Remove the pop instruction
				Strings[i].clear();
				ClassifyInstruction(Commands[i], Strings[i].c_str());
			}else{
				// Change pop to mov
				Strings[i].replace(0, 3, "mov");
				Strings[i] += ", ";
				Strings[i] += std::string(Strings[n].c_str() + Commands[n].argA.begin, Commands[n].argA.size);
				
				ClassifyInstruction(Commands[i], Strings[i].c_str());
			}
			// Remove the push instruction
			Strings[n].clear();
			ClassifyInstruction(Commands[n], Strings[n].c_str());

			++optimize_count;
		}
		if(Commands[i].Name == push && (Commands[i].argA.type == Argument::number || isGenReg[Commands[i].argA.type]))
		{
			// push num, pop dword [] or
			// push reg, pop dword []
			if(Commands[i+1].Name == pop && Commands[i+1].argA.type == Argument::ptr)
			{
				Strings[i+1].replace(0, 3, "mov");
				Strings[i+1] += ", " + std::string(Strings[i].c_str()+Commands[i].argA.begin, Commands[i].argA.size);
				Strings[i] = "";

				// Update instruction information
				ClassifyInstruction(Commands[i], Strings[i].c_str());
				ClassifyInstruction(Commands[i+1], Strings[i+1].c_str());

				++optimize_count;
			}
			// push num, pop reg
			if(Commands[i].argA.type == Argument::number && Commands[i+1].Name == pop && isGenReg[Commands[i+1].argA.type])
			{
				Strings[i+1].replace(0, 3, "mov");
				Strings[i+1] += ", " + std::string(Strings[i].c_str()+Commands[i].argA.begin, Commands[i].argA.size);
				Strings[i] = "";

				// Update instruction information
				ClassifyInstruction(Commands[i], Strings[i].c_str());
				ClassifyInstruction(Commands[i+1], Strings[i+1].c_str());

				++optimize_count;
			}
		}
		if(Commands[i].Name == push && Commands[i].argA.type == Argument::ptr && strstr(Strings[i].c_str()+Commands[i].argA.begin, "dword"))
		{
			// push dword, pop reg
			if(Commands[i+1].Name == pop && isGenReg[Commands[i+1].argA.type])
			{
				Strings[i+1].replace(0, 3, "mov");
				Strings[i+1] += ", " + std::string(Strings[i].c_str()+Commands[i].argA.begin, Commands[i].argA.size);
				Strings[i] = "";

				// Update instruction information
				ClassifyInstruction(Commands[i], Strings[i].c_str());
				ClassifyInstruction(Commands[i+1], Strings[i+1].c_str());
				++optimize_count;
			}
		}
		if(Commands[i].Name == fld && strstr(Strings[i].c_str(), "qword [esp]"))
		{
			// push dword [a+4], push dword[a], fld [esp]
			if(Commands[i-1].Name == push && Commands[i-1].argA.type == Argument::ptr && Commands[i-2].Name == push && Commands[i-2].argA.type == Argument::ptr)
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
			}else if(Commands[i-2].Name == push && Commands[i-2].argA.type == Argument::ptr && Commands[i-3].Name == push && Commands[i-3].argA.type == Argument::ptr)
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
			}else if(Commands[i-1].Name == push && Commands[i-1].argA.type == Argument::number && Commands[i-2].Name == push && Commands[i-2].argA.type == Argument::number)
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
		if(Commands[i].Name == fstp && strstr(Strings[i].c_str(), "qword [esp]"))
		{
			// fstp qword [esp], fld qword [esp]
			if(Commands[i+1].Name == fld && strstr(Strings[i+1].c_str(), "qword [esp]"))
			{
				Strings[i] = "";
				Strings[i+1] = "";

				// Update instruction information
				ClassifyInstruction(Commands[i], Strings[i].c_str());
				ClassifyInstruction(Commands[i+1], Strings[i+1].c_str());

				++optimize_count;
			// fstp qword [esp], pop dword [a], pop dword [a+4]
			}else if(Commands[i+1].Name == pop && Commands[i+1].argA.type == Argument::ptr && Commands[i+2].Name == pop && Commands[i+2].argA.type == Argument::ptr){
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
		if(Commands[i].Name == add && Commands[i].argA.type == Argument::reg && Commands[i].argB.type == Argument::number)
		{
			if(Commands[i+1].Name == sub && Commands[i+1].argA.type == Commands[i].argA.type && strcmp(Strings[i].c_str()+Commands[i].argB.begin, Strings[i+1].c_str()+Commands[i+1].argB.begin) == 0)
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

	for(UINT m = 0; m < Strings.size(); m++)
	{
		char text[256] = "";

		if(Strings[m].size() != 0)
		{
			//strncpy(text, Strings[m].c_str(), Strings[m].size() + 1);
			//cout << text << endl;
		}
	}
	//cout << "Optimize : " << optimize_count << endl;
}

bool Optimizer_x86::IsJump(int Command_Name)
{
	for(int i = 3; i < 17; i++)
	{
		if(Command_Name == Commands_table[i].Hash)
			return true;
	}

	return false;
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

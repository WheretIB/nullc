#pragma warning(disable: 4786)    // Надо-ли? Но, не нравиццо.

#include "stdafx.h"
#include "Optimizer_x86.h"


std::vector <std::string> Strings;
std::vector <int>   Sizes;

struct Command_def{
	char* Name;
	int	  Hash;
	int   Size;
};

enum Command_Hash{
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
	other,
};

class Command
{
public:
	Command_Hash Name;
	int    pName;
	int    arg1;        // Относительно Strings[i].begin(), не оставим по старому, не всё же по ходу надо делать так
	int	   size1;
	char*  arg2;
	int	   size2;
	char*  arg3;
	int    size3;
};

std::vector<Command> Commands;


Command_def Commands_table[] = {
	
	"none" ,  0, sizeof("none"),
	"push" ,  1, sizeof("push"),
	"pop"  ,  2, sizeof("pop"),
	"jmp"  ,  3, sizeof("jmp"),
	"ja"   ,  4, sizeof("ja"),
	"jae"  ,  5, sizeof("jae"),
	"jb"   ,  6, sizeof("jb"),
	"jbe"  ,  7, sizeof("jbe"),
	"jc"   ,  8, sizeof("jc"),
	"je"   ,  9, sizeof("je"),
	"jz"   , 10, sizeof("jz"),
	"jg"   , 11, sizeof("jg"),
	"jl"   , 12, sizeof("jl"),
	"jne"  , 13, sizeof("jne"),
	"jnp"  , 14, sizeof("jnp"),
	"jnz"  , 15, sizeof("jnz"),
	"jp"   , 16, sizeof("jp"),	     // Now its not full list, if you will add commands, calculate correct value in IsJump function
	"call" , 17, sizeof("call"),
	"other", 18, sizeof("other"),
};

const int Commands_table_size = sizeof(Commands_table) / sizeof(Command_def);

std::vector<std::string>* Optimizer_x86::Optimize(const char* pListing)
{
	HashListing(pListing);
	OptimizePushPop();

	// Strings contain the optimized code
	return &Strings;
}

void Optimizer_x86::OptimizePushPop()
{
	int optimize_count = 0;

	for(int i = 0; i < Commands.size(); i++)
	{
		if(Commands[i].Name == pop)
		{
			if(Commands[i].size1 == 3 && IsRegister((char*)(Strings[i].c_str() + Commands[i].arg1)) == true)
			{
				int n = i;
				bool flag = true;

				while(Commands[n].Name != push && n > 0)
				{
					n = n - 1;
				}

				if(IsRegister((char*)(Strings[n].c_str() + Commands[n].arg1)) == false)
					flag = false;

				for(int m = n + 1; m < i; m++)
				{
					if(Commands[m].Name == call || IsJump(Commands[m].Name) == true)
					{
						flag = false;
					}

					if(Commands[m].Name != none)
					{
						char text[32] = "";

						if(Strnstr((char*)Strings[m].c_str(), "esp", Strings[m].size()) != 0)
							flag = false;

						if(Strnstr((char*)Strings[m].c_str(), ":", Strings[m].size()) != 0)
							flag = false;

						strncpy(text, (char*)(Strings[n].c_str() + Commands[n].arg1), Commands[n].size1);
						if(Strnstr((char*)(Strings[m].c_str() + Commands[m].arg1), text, Commands[m].size1) != 0)
							flag = false;
					}

				}

				if(flag == true)
				{
					int bu = (int)(Strings[i].begin() - Strings[n + 1].begin());
					char* bu1 = (char*)Strings[n].c_str() + Commands[n].arg1;
					char* bu2 = (char*)Strings[i].c_str() + Commands[i].arg1;

					//strncpy(Strings[i].begin() + Commands[i].arg1, Strings[n].c_str() + Commands[n].arg1, 3);
					Strings[n][0] = ';';
					//strncpy(Strings[n].begin(), ";", 1);
					strncpy((char*)Strings[i].c_str() + Commands[i].pName, "mov", 3);

					Strings[i].insert(Commands[i].arg1 + Commands[i].size1, ",    ", 5);
					
					strncpy((char*)Strings[i].c_str() + Commands[i].arg1 + Commands[i].size1 + 2, Strings[n].c_str() +
																									Commands[n].arg1, 3);

					++optimize_count;

				}
			}

		}
	}

	for(int m = 0; m < Strings.size(); m++)
	{
		char text[256] = "";

		if(Strings[m].size() != 0)
		{
			/*if(Commands[m].Name == none)
				cout << "none" << endl;

			if(Commands[m].Name == push)
				cout << "push" << endl;

			if(Commands[m].Name == pop)
				cout << "pop" << endl;

			if(Commands[m].Name == other)
				cout << "other" << endl;*/

			strncpy(text, Strings[m].c_str(), Strings[m].size() + 1);
			//cout << text << endl;
		}
	//cout << "Size : " << Sizes[m] << endl;
	}

	//cout << "Optimize : " << optimize_count << endl;
}

bool Optimizer_x86::IsRegister(const char* text)
{
	if(strncmp(text, "eax", 3) != 0 && strncmp(text, "ebx", 3) != 0 && strncmp(text, "ecx", 3) !=0 &&
		strncmp(text, "edx", 3) != 0 && strncmp(text, "edi", 3) != 0 && strncmp(text, "esi", 3) != 0)
	{
		return false;
	}

	return true;
}

char* Optimizer_x86::Strnstr(char* text, char* subtext, int size)
{
	char temp;
	char* pointer;

	temp = text[size];
	text[size] = 0;

	pointer = strstr(text, subtext);

	text[size] = temp;

	return pointer;
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
	const char* pString = pListing;
	int Str_size = 0;
	//std::string text;
	char temp_text[256] = "";			// Ща с гпрсом не поглядеть справку для append штобы что-нить вроде append(string, size);

	//Command Command_1;

	while(strchr(pString, '\n') != NULL)
	{	
		memset(temp_text, NULL, sizeof(temp_text));
		strncpy(temp_text, pString, (int)(strchr(pString, '\n') - pString));
		//text.append(temp_text);

		Strings.push_back(std::string(temp_text));

		//text.erase(text.begin(), text.end());

		//Commands.push_back(Command_1);								// Resize?
		pString = strchr(pString, '\n') + 1;
	}
	Commands.resize(Strings.size());	// Yep! Resize!

	for(int n = 0; n < Strings.size(); n++)
	{
		char* temp;
		int   size = 0;
		int   command_size = 0;

		temp = (char*)Strings[n].c_str();
		while((*temp == ' ' || *temp == '\t' ) && *temp != ';' && size < Strings[n].size())
		{
			++temp;
			++size;
		}

		while(*(temp + command_size) != ' ' && *(temp + command_size) != '\t')
		{
			++command_size;
		}

		size = 0;

		if(*temp != ';')
		{
			/*while(*((char*)((int)temp + size)) != ' ' && *((char*)((int)temp + size)) != '\t')
			{
				++size;
			}*/

			for(int b = 0; b < Commands_table_size; b++)
			{
				if(1)//command_size == Commands_table[b].Size - 1)
				{
					if(strncmp(Commands_table[b].Name, temp, Commands_table[b].Size - 1) == 0)
					{
						Commands[n].Name = (Command_Hash)Commands_table[b].Hash;
						size = 1;
						break;
					}
				}
			}

			if(size == 0)
				Commands[n].Name = other;

			Commands[n].pName = temp - Strings[n].c_str();

		}
		else
		{
			Commands[n].Name = none;
		}

		for(int i = 0; i < 2; i++)					//Cycle? Ага.
		{
			while(*temp != ' ' && *temp != '\t')
			{
				++temp;
			}

			while(*temp == ' ' || *temp == '\t')
			{
				++temp;
			}

			if(strncmp(temp, "byte", 4) != 0 && strncmp(temp, "word", 4) != 0 && strncmp(temp, "dword", 5) != 0 &&
				strncmp(temp, "ptr", 3) != 0)
			{
				break;
			}
		}

		Commands[n].arg1 = temp - Strings[n].c_str();

		size = 0;

		while(*temp != ' ' && *temp != ',' && *temp != '\t' && *temp != ';' && *temp != '\n')
		{
			++temp;
			++size;
		}
		
		Commands[n].size1 = size;
	}
}

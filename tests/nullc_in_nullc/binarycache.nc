import std.string;
import std.memory;
import bytecode;
import lexer;

int GetStringHash(string str)
{
    return GetStringHash(str.data, 0, str.length());
}

namespace BinaryCache
{
	void Initialize();
	void Terminate();

	void		PutBytecode(string path, ByteCode ref bytecode, Lexeme[] lexemes);
	void		PutLexemes(string path, Lexeme[] lexemes);

	ByteCode ref	GetBytecode(string path);

	// Find module bytecode in any of the import paths or by name alone
	ByteCode ref	FindBytecode(string moduleName, bool addExtension);

	Lexeme[]		GetLexems(string path);

	// Find module source lexems in any of the import paths or by name alone
	Lexeme[]		FindLexems(string moduleName, bool addExtension);

	void		RemoveBytecode(string path);
	string ref	EnumerateModules(int id);

	void		LastBytecode(ByteCode ref bytecode);

	void		ClearImportPaths();
	void		AddImportPath(string path);
	void		RemoveImportPath(string path);
	bool		HasImportPath(string path);
	string ref	EnumImportPath(int pos);

	class CodeDescriptor
	{
		string name;
		int nameHash;
		ByteCode ref binary;
		Lexeme[] lexemes;
	}

    vector<CodeDescriptor> cache;
	vector<string> importPaths;

	int	lastReserved = 0;
	ByteCode ref lastBytecode;
	int	lastHash = GetStringHash("__last.nc");

    void ClearImportPaths()
    {
        importPaths.clear();
    }

    void AddImportPath(string path)
    {
        for(int i = 0; i < importPaths.size(); i++)
        {
            if(importPaths[i] == path)
                return;
        }

        importPaths.push_back(path);
    }

    void RemoveImportPath(string path)
    {
        for(int i = 0; i < importPaths.size(); i++)
        {
            if(importPaths[i] == path)
            {
                importPaths[i] = importPaths.back();
                importPaths.pop_back();
                return;
            }
        }
    }

    bool HasImportPath(string path)
    {
        for(int i = 0; i < importPaths.size(); i++)
        {
            if(importPaths[i] == path)
                return true;
        }

        return false;
    }

    string ref EnumImportPath(int pos)
    {
        return pos < importPaths.size() ? &importPaths[pos] : nullptr;
    }

    void Initialize()
    {
        lastReserved = 0;
        lastBytecode = nullptr;
    }

    void Terminate()
    {
        ClearImportPaths();

        importPaths.clear();

        cache.clear();

        lastBytecode = nullptr;
    }

    void PutBytecode(string path, ByteCode ref bytecode, Lexeme[] lexStart)
    {
        int hash = GetStringHash(path);

        if(hash == lastHash)
            return;

        int i = 0;
        for(; i < cache.size(); i++)
        {
            if(hash == cache[i].nameHash)
                break;
        }
        assert(i == cache.size());

        CodeDescriptor desc;
        desc.name = path;
        desc.nameHash = hash;
        desc.binary = bytecode;

        if(lexStart)
        {
            desc.lexemes = new Lexeme[lexStart.size];

            for(i in lexStart, j in desc.lexemes)
                j = i;
        }
        else
        {
            desc.lexemes = nullptr;
        }
        cache.push_back(desc);
    }

    void PutLexemes(string path, Lexeme[] lexStart)
    {
        int hash = GetStringHash(path);

        if(hash == lastHash)
            return;

        for(int i = 0; i < cache.size(); i++)
        {
            CodeDescriptor ref desc = &cache[i];

            if(hash == cache[i].nameHash)
            {
                assert(cache[i].lexemes == nullptr);

                desc.lexemes = new Lexeme[lexStart.size];

                for(i in lexStart, j in desc.lexemes)
                    j = i;

                return;
            }
        }

        assert(false, "module not found");
    }

    ByteCode ref GetBytecode(string path)
    {
        int hash = GetStringHash(path);

        if(hash == lastHash)
            return lastBytecode;

        int i = 0;
        for(; i < cache.size(); i++)
        {
            if(hash == cache[i].nameHash)
                break;
        }
        if(i != cache.size())
            return cache[i].binary;

        return nullptr;
    }

    ByteCode ref FindBytecode(string moduleName, bool addExtension)
    {
        if(!addExtension)
        {
            int hash = GetStringHash(moduleName);

            if(hash == lastHash)
                return lastBytecode;
        }

        if(addExtension)
            assert(moduleName.find(".nc") == -1);
        else
            assert(moduleName.find(".nc") != -1);

        int modulePathPos = 0;
        string ref modulePath = EnumImportPath(modulePathPos++);
        while(modulePath)
        {
            string path = *modulePath + moduleName;

            if(addExtension)
            {
                for(int i = modulePath.length(), e = path.length(); i != e; i++)
                {
                    if(path[i] == '.')
                        path[i] = '/';
                }

                path = path + ".nc";
            }

            if(ByteCode ref bytecode = GetBytecode(path))
                return bytecode;

            modulePath = EnumImportPath(modulePathPos++);
        }

        return nullptr;
    }

    Lexeme[] GetLexems(string path)
    {
        int hash = GetStringHash(path);

        for(int i = 0; i < cache.size(); i++)
        {
            if(hash == cache[i].nameHash)
            {
                return cache[i].lexemes;
            }
        }
        return nullptr;
    }

    Lexeme[] FindLexems(string moduleName, bool addExtension)
    {
        if(addExtension)
            assert(moduleName.find(".nc") == -1);
        else
            assert(moduleName.find(".nc") != -1);

        int modulePathPos = 0;
        string ref modulePath = EnumImportPath(modulePathPos++);
        while(modulePath)
        {
            string path = *modulePath + moduleName;

            if(addExtension)
            {
                for(int i = modulePath.length(), e = path.length(); i != e; i++)
                {
                    if(path[i] == '.')
                        path[i] = '/';
                }

                path = path + ".nc";
            }

            if(Lexeme[] lexems = GetLexems(path))
                return lexems;

            modulePath = EnumImportPath(modulePathPos++);
        }

        return nullptr;
    }

    void RemoveBytecode(string path)
    {
        int hash = GetStringHash(path);
        int i = 0;
        for(; i < cache.size(); i++)
        {
            if(hash == cache[i].nameHash)
                break;
        }
        if(i == cache.size())
            return;

        cache[i] = *cache.back();
        cache.pop_back();
    }

    string ref EnumerateModules(int id)
    {
        if(id >= cache.size())
            return nullptr;

        return &cache[id].name;
    }

    void LastBytecode(ByteCode ref bytecode)
    {
        lastBytecode = bytecode;
    }
}

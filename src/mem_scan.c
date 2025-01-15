#include "mem_scan.h"

char* TO_CHAR(wchar_t* string)
{
    size_t len      = wcslen(string) + 1;
    char*  c_string = new char[len];
    size_t numCharsRead;
    wcstombs_s(&numCharsRead, c_string, len, string, _TRUNCATE);
    return c_string;
}

PEB* GetPEB()
{
#ifdef _WIN64
    PEB* peb = (PEB*)__readgsqword(0x60);

#else
    PEB* peb = (PEB*)__readfsdword(0x30);
#endif

    return peb;
}

LDR_DATA_TABLE_ENTRY* GetLDREntry(char* moduleName)
{
    LDR_DATA_TABLE_ENTRY* ldr = nullptr;
    PEB* peb = GetPEB();
    LIST_ENTRY head = peb->Ldr->InMemoryOrderModuleList;
    LIST_ENTRY curr = head;

    while (curr.Flink != head.Blink)
    {
        LDR_DATA_TABLE_ENTRY* mod = (LDR_DATA_TABLE_ENTRY*)
            CONTAINING_RECORD(curr.Flink, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        if (mod->FullDllName.Buffer)
        {
            char* cName = TO_CHAR(mod->BaseDllName.Buffer);

            if (_stricmp(cName, moduleName) == 0)
            {
                ldr = mod;
                break;
            }
            delete[] cName;
        }
        curr = *curr.Flink;
    }
    return ldr;
}

char* FindPatternBasic(char* pattern, char* mask, char* begin, intptr_t size)
{
    intptr_t patternLen = strlen(mask);

    for (int i = 0; i < size; i++)
    {
        bool found = true;
        for (int j = 0; j < patternLen; j++)
        {
            if (mask[j] != '?' && pattern[j] != *(char*)((intptr_t)begin + i + j))
            {
                found = false;
                break;
            }
        }
        if (found)
        {
            return (begin + i);
        }
    }
    return nullptr;
}

char* FindPattern(char* pattern, char* mask, char* begin, intptr_t size)
{
    char*                    match { nullptr };
    MEMORY_BASIC_INFORMATION mbi {};

    for (char* curr = begin; curr < begin + size; curr += mbi.RegionSize)
    {
        if (!VirtualQuery(curr, &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT
            || mbi.Protect == PAGE_NOACCESS)
            continue;

        match = FindPatternBasic(pattern, mask, curr, mbi.RegionSize);

        if (match != nullptr)
        {
            break;
        }
    }
    return match;
}


char* FindPatternIn(char* pattern, char* mask, char* moduleName)
{
    LDR_DATA_TABLE_ENTRY* ldr = GetLDREntry(modName);
    
    char* match = FindPattern(pattern, mask, (char*)ldr->DllBase, ldr->SizeOfImage);

    return match;
}

#include "hooks.h"
#include "jvmti_unlock.h"

uintptr_t get_potential_capabilities_ptr;
uintptr_t get_current_capabilities;

jvmtiCapabilities* g_oCapabilitiesPtr;

bool Initialize(jvmtiEnv* jvmti)
{
    MH_Initialize();

    /*Address of signature = jvm.dll + 0x0025D460
    "\x48\x89\x5C\x24\x00\x56\x48\x83\xEC\x00\x33\xC0", "xxxx?xxxx?xx"
    "48 89 5C 24 ? 56 48 83 EC ? 33 C0"*/
    get_potential_capabilities_ptr = (uintptr_t)(FindPatternIn(
        (char*)("\x48\x89\x5C\x24\x00\x56\x48\x83\xEC\x00\x33\xC0"),
        (char*)("xxxx?xxxx?xx"),
        "jvm.dll"));

    /*Address of signature = jvm.dll + 0x001FABC0
    "\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x74\x00\x48\xFF\xE0", "xxx????xxxx?xxx"
    "48 8B 05 ? ? ? ? 48 85 C0 74 ? 48 FF E0"*/
    get_current_capabilities = (uintptr_t)(FindPatternIn(
        (char*)("\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x74\x00\x48\xFF\xE0"),
        (char*)("xxx????xxxx?xxx"),
        "jvm.dll"));

    if (get_current_capabilities)
    {
        g_oCapabilitiesPtr = ((f_DirectGetCapabilities)get_current_capabilities)(jvmti);
    }

    if (!get_potential_capabilities_ptr && !get_current_capabilities)
    {
        return false;
    }


    return true;
}

f_GetPotentialCapabilities oGetPotentialCapabilities;
f_DirectGetCapabilities    oGetCapabilities;

/* The number of capabilities in the jvmtiCapabilities struct(not including the padding fields)
 * this number may (probably will) deviate from java version to java version. some capabilities
 * may have been added. AFAIK none have been removed, therefore this should work for all versions,
 * except for newly added capabilities.*/

#define JVMTI_INTERNAL_CAPABILITY_COUNT 41
static const jint CAPA_SIZE = (JVMTI_INTERNAL_CAPABILITY_COUNT + 7) / 8;

void enable_all_capabilities(jvmtiCapabilities* capa)
{
    // The compiler optimizes the unsigned integers in the jvmtiCapabilities struct to
    // single bits, so we need to set each *bit* to 1

    // This actually took me longer to figure out than I'd like to admit. I was trying to memset
    // the whole struct to 1, but the compiler optimizes the struct to single bits, so I was
    // essentially only setting every 8th field to 1 (and corrupting some memory).

    // we just don't give a flying f about the current or prohibited capabilities and
    // just allow everything
    for (int i = 0; i < JVMTI_INTERNAL_CAPABILITY_COUNT; i++)
    {
        int byte_index = i / 8; // Index of the byte in the array
        int bit_index  = i % 8; // The specific bit within that byte

        // Set the specific bit to 1
        ((char*)capa)[byte_index] |= (1 << bit_index);
    }
}

bool HookFunction(
    LPVOID      func_ptr,
    void*       detour,
    void**      original_func_ptr,
    const char* function_name)
{
    const MH_STATUS hookStatus = MH_CreateHook(func_ptr, detour, original_func_ptr);

    if (hookStatus != MH_OK)
    {
        printf(
            "[HookLibraryNative] Failed to hook %s: %s",
            function_name,
            MH_StatusToString(hookStatus));
        return false;
    }

    if (MH_EnableHook(func_ptr) != MH_OK)
    {
        return false;
    }

    return true;
}


static std::atomic<bool>  g_AllCapabilitiesInitialized;
static jvmtiCapabilities* g_AllCapabilities;
static std::mutex         g_CapabilitiesMutex;

jvmtiCapabilities* DetourGetCapabilities(jvmtiEnv* jvmti)
{
    if (!g_AllCapabilitiesInitialized.load())
    {
        std::lock_guard<std::mutex> lock(g_CapabilitiesMutex);
        if (!g_AllCapabilitiesInitialized.load())
        {
            jvmtiCapabilities* capabilities = (jvmtiCapabilities*)malloc(sizeof(jvmtiCapabilities));

            if (!capabilities)
            {
                // eh, this is fatal... we'll try not to crash the process, no guarantee tho :D
                printf("[HookLibraryNative] Failed to allocate memory for capabilities in detour! "
                       "This is fatal, trying to recover...");
                MH_DisableHook(MH_ALL_HOOKS);
                return g_oCapabilitiesPtr;
            }

            g_AllCapabilities = capabilities;
            g_AllCapabilitiesInitialized.store(true);
        }
    }

    enable_all_capabilities(g_AllCapabilities);
    return g_AllCapabilities; // Return the pointer to the capabilities object
}

/**
 Address of signature = jvm.dll + 0x00329ED1
"\x41\x8B\xD6\x48\x8B\xCF\xE8\x00\x00\x00\x00\x48\x8D\x4C\x24", "xxxxxxx????xxxx"
"41 8B D6 48 8B CF E8 ? ? ? ? 48 8D 4C 24"
 */
bool PatchSetEventNotifications(jvmtiEnv* jvmti)
{

    // XOR EAX, EAX; NOP x 9, basically removes a check and
    // allows the function to continue without checking if the
    // capabilities are set. We need to fix the register though,
    // so we XOR out EAX
    static const char* shell_code = "\x31\xC0\x90\x90\x90\x90\x90\x90\x90\x90\x90";
    const char* capability_check  = "\x41\x8B\xD6\x48\x8B\xCF\xE8\x00\x00\x00\x00\x48\x8D\x4C\x24";
    const char* pattern_mask      = "xxxxxxx????xxxx";

    char* addr_to_patch = FindPatternIn(
        const_cast<char*>(capability_check),
        const_cast<char*>(pattern_mask),
        "jvm.dll");

    if (!addr_to_patch)
    {
        return false;
    }

    DWORD dwOldProtect;
    VirtualProtect(addr_to_patch, strlen(pattern_mask), PAGE_EXECUTE_READWRITE, &dwOldProtect);
    memcpy(addr_to_patch, shell_code, strlen(shell_code));
    VirtualProtect(addr_to_patch, strlen(pattern_mask), dwOldProtect, &dwOldProtect);

    return true;
}

bool HookGetCapabilities(jvmtiEnv* jvmti)
{
    if (!get_current_capabilities)
    {
        return false;
    }

    LPVOID func_ptr = (LPVOID)get_current_capabilities;

    if (!HookFunction(
            func_ptr,
            &DetourGetCapabilities,
            reinterpret_cast<void**>(&oGetCapabilities),
            "GetCapabilities"))
    {
        return false;
    }

    return true;
}

/*Address of signature = jvm.dll + 0x0019DAA9
"\x0F\x85\x00\x00\x00\x00\xFF\xC0", "xx????xx"
"0F 85 ? ? ? ? FF C0" 6*/
bool PatchAddCapabilities(jvmtiEnv* jvmti)
{
    static const char* shell_code           = "\x90\x90\x90\x90\x90\x90";
    const char*        add_capability_check = "\x0F\x85\x00\x00\x00\x00\xFF\xC0";
    const char*        pattern_mask         = "xx????xx";

    char* addr_to_patch = FindPatternIn(
        const_cast<char*>(add_capability_check),
        const_cast<char*>(pattern_mask),
        "jvm.dll");

    if (!addr_to_patch)
    {
        return false;
    }

    DWORD dwOldProtect;
    VirtualProtect(addr_to_patch, strlen(pattern_mask), PAGE_EXECUTE_READWRITE, &dwOldProtect);
    memcpy(addr_to_patch, shell_code, strlen(shell_code));
    VirtualProtect(addr_to_patch, strlen(pattern_mask), dwOldProtect, &dwOldProtect);

    return true;
}

void Uninstall()
{
    free(g_AllCapabilities);
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}

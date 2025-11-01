#include <windows.h>
#include <string>

static FARPROC GetOriginalFunction(const char* name, const char* func) {
    std::string original_name = std::string(name) + ".original.dll";
    HMODULE h = GetModuleHandleA(original_name.c_str());
    if (!h) {
        printf("Failed to get handle for %s\n", original_name.c_str());
        return nullptr;
    }
    return GetProcAddress(h, func);
}

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved
) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

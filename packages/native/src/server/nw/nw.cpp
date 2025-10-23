#include <cstdio>
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

extern "C" {
    // ?InternalFieldCount@Object@v8@@QEBAHXZ
    __declspec(dllexport) int forward_InternalFieldCount_Object_v8__QEAAHXZ(void* platform) {
        printf("TODO: forward_InternalFieldCount_Object_v8__QEAAHXZ\n");
        return 1;
    }
    // ?SetTLSPlatform@v8@@YAXPEAVPlatform@1@@Z
    __declspec(dllexport) void forward_SetTLSPlatform_v8__YAXPEAVPlatform_1__Z(void* platform) {
        printf("TODO: forward_SetTLSPlatform_v8__YAXPEAVPlatform_1__Z\n");
    }
    // ?Free@Allocator@ArrayBuffer@v8@@UEAAXPEAX_KW4AllocationMode@123@@Z
    __declspec(dllexport) void forward_Free_Allocator_ArrayBuffer_v8__UEAAXPEAX_KW4AllocationMode_123__Z() {
        printf("TODO: forward_Free_Allocator_ArrayBuffer_v8__UEAAXPEAX_KW4AllocationMode_123__Z\n");
    }
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

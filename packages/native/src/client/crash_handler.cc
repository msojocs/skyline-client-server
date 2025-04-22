#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#include <spdlog/spdlog.h>
#include <fstream>
#include <ctime>
#include <string>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <tlhelp32.h>

#pragma comment(lib, "dbghelp.lib")

namespace CrashHandler {
    // 崩溃日志目录
    const std::string CRASH_LOG_DIR = "crash_logs";
    
    // 获取当前时间字符串
    std::string getCurrentTimeString() {
        auto now = std::time(nullptr);
        auto tm = std::localtime(&now);
        std::stringstream ss;
        ss << std::put_time(tm, "%Y%m%d_%H%M%S");
        return ss.str();
    }

    // 创建崩溃日志目录
    void createCrashLogDirectory() {
        if (!std::filesystem::exists(CRASH_LOG_DIR)) {
            std::filesystem::create_directory(CRASH_LOG_DIR);
        }
    }

    // 获取模块信息
    std::string getModuleInfo(HMODULE hModule) {
        char moduleName[MAX_PATH];
        if (GetModuleFileNameA(hModule, moduleName, MAX_PATH)) {
            return moduleName;
        }
        return "Unknown Module";
    }

    // 获取异常信息
    std::string getExceptionInfo(PEXCEPTION_POINTERS pExceptionInfo) {
        std::stringstream ss;
        ss << "Exception Code: 0x" << std::hex << pExceptionInfo->ExceptionRecord->ExceptionCode << std::endl;
        ss << "Exception Address: 0x" << std::hex << pExceptionInfo->ExceptionRecord->ExceptionAddress << std::endl;
        ss << "Exception Flags: 0x" << std::hex << pExceptionInfo->ExceptionRecord->ExceptionFlags << std::endl;
        return ss.str();
    }

    // 获取调用堆栈
    std::string getStackTrace(PEXCEPTION_POINTERS pExceptionInfo) {
        std::stringstream ss;
        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();
        
        // 初始化符号表
        SymInitialize(process, NULL, TRUE);
        
        // 设置符号选项
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
        
        // 获取上下文
        CONTEXT context = *pExceptionInfo->ContextRecord;
        
        // 初始化STACKFRAME
        STACKFRAME64 stackFrame;
        memset(&stackFrame, 0, sizeof(STACKFRAME64));
        
        // 设置初始栈帧
        stackFrame.AddrPC.Offset = context.Rip;
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = context.Rbp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = context.Rsp;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        
        // 遍历调用堆栈
        while (StackWalk64(
            IMAGE_FILE_MACHINE_AMD64,
            process,
            thread,
            &stackFrame,
            &context,
            NULL,
            SymFunctionTableAccess64,
            SymGetModuleBase64,
            NULL)) {
            
            // 获取符号信息
            char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
            PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)symbolBuffer;
            pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            pSymbol->MaxNameLen = MAX_SYM_NAME;
            
            DWORD64 displacement = 0;
            if (SymFromAddr(process, stackFrame.AddrPC.Offset, &displacement, pSymbol)) {
                ss << "0x" << std::hex << stackFrame.AddrPC.Offset << " " << pSymbol->Name;
                
                // 获取行号信息
                IMAGEHLP_LINE64 line;
                line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
                DWORD displacement = 0;
                if (SymGetLineFromAddr64(process, stackFrame.AddrPC.Offset, &displacement, &line)) {
                    ss << " (" << line.FileName << ":" << line.LineNumber << ")";
                }
                ss << std::endl;
            }
        }
        
        SymCleanup(process);
        return ss.str();
    }

    // 异常处理函数
    LONG WINAPI ExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo) {
        // 创建崩溃日志目录
        createCrashLogDirectory();
        
        // 生成日志文件名
        std::string logFileName = CRASH_LOG_DIR + "/crash_" + getCurrentTimeString() + ".log";
        
        // 打开日志文件
        std::ofstream logFile(logFileName);
        if (!logFile.is_open()) {
            return EXCEPTION_CONTINUE_SEARCH;
        }
        
        // 写入崩溃信息
        logFile << "=== Crash Report ===" << std::endl;
        logFile << "Time: " << getCurrentTimeString() << std::endl;
        logFile << "Process ID: " << GetCurrentProcessId() << std::endl;
        logFile << "Thread ID: " << GetCurrentThreadId() << std::endl;
        logFile << std::endl;
        
        // 写入异常信息
        logFile << "=== Exception Information ===" << std::endl;
        logFile << getExceptionInfo(pExceptionInfo) << std::endl;
        
        // 写入调用堆栈
        logFile << "=== Call Stack ===" << std::endl;
        logFile << getStackTrace(pExceptionInfo) << std::endl;
        
        // 写入模块信息
        logFile << "=== Loaded Modules ===" << std::endl;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
        if (snapshot != INVALID_HANDLE_VALUE) {
            MODULEENTRY32 moduleEntry;
            moduleEntry.dwSize = sizeof(MODULEENTRY32);
            if (Module32First(snapshot, &moduleEntry)) {
                do {
                    logFile << "Module: " << moduleEntry.szModule << " (Base: 0x" 
                           << std::hex << (DWORD64)moduleEntry.modBaseAddr 
                           << ", Size: " << std::dec << moduleEntry.modBaseSize 
                           << ")" << std::endl;
                } while (Module32Next(snapshot, &moduleEntry));
            }
            CloseHandle(snapshot);
        }
        
        logFile.close();
        
        // 记录到spdlog
        spdlog::error("Application crashed. Crash report saved to: {}", logFileName);
        
        return EXCEPTION_CONTINUE_SEARCH;
    }

    // 初始化崩溃处理
    void init() {
        // 设置未处理异常过滤器
        SetUnhandledExceptionFilter(ExceptionHandler);
        
        // 设置结构化异常处理
        _set_se_translator([](unsigned int code, PEXCEPTION_POINTERS pExceptionInfo) {
            throw std::runtime_error("Structured Exception");
        });
        
        spdlog::info("Crash handler initialized");
    }
} 
#endif
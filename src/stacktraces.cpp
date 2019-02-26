// Copyright (c) 2014-2018 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "stacktraces.h"
#include "tinyformat.h"
#include "random.h"
#include "util.h"

#include "dash-config.h"

#include <mutex>
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>

#include <cxxabi.h>

#if WIN32
#include <windows.h>
#include <dbghelp.h>
#else
#include <execinfo.h>
#include <unistd.h>
#include <signal.h>
#endif

#if !WIN32
#include <dlfcn.h>
#endif

#if __APPLE__
#include <mach-o/dyld.h>
#endif

#ifdef ENABLE_STACKTRACES
#include <backtrace.h>
#endif
#include <libgen.h> // for basename()
#include <string.h>

static void PrintCrashInfo(const std::string& s)
{
    LogPrintf("%s", s);
    fprintf(stderr, "%s", s.c_str());
    fflush(stderr);
}

std::string DemangleSymbol(const std::string& name)
{
#if __GNUC__ || __clang__
    int status = -4; // some arbitrary value to eliminate the compiler warning
    char* str = abi::__cxa_demangle(name.c_str(), nullptr, nullptr, &status);
    if (status != 0) {
        free(str);
        return name;
    }
    std::string ret = str;
    free(str);
    return ret;
#else
    // TODO other platforms/compilers
    return name;
#endif
}

// set to true when the abort signal should not handled
// this is the case when the terminate handler or an assert already handled the exception
static std::atomic<bool> skipAbortSignal(false);

#ifdef ENABLE_STACKTRACES
ssize_t GetExeFileNameImpl(char* buf, size_t bufSize)
{
#if WIN32
    std::vector<TCHAR> tmp(bufSize);
    DWORD len = GetModuleFileName(NULL, tmp.data(), bufSize);
    if (len >= bufSize) {
        return len;
    }
    for (size_t i = 0; i < len; i++) {
        buf[i] = (char)tmp[i];
    }
    return len;
#elif __APPLE__
    uint32_t bufSize2 = (uint32_t)bufSize;
    if (_NSGetExecutablePath(buf, &bufSize2) != 0) {
        // it's not entirely clear if the value returned by _NSGetExecutablePath includes the null character
        return bufSize2 + 1;
    }
    // TODO do we have to call realpath()? The path returned by _NSGetExecutablePath may include ., .. symbolic links
    return strlen(buf);
#else
    ssize_t len = readlink("/proc/self/exe", buf, bufSize - 1);
    if (len == -1) {
        return -1;
    }
    return len;
#endif
}

std::string GetExeFileName()
{
    std::vector<char> buf(1024);
    while (true) {
        ssize_t len = GetExeFileNameImpl(buf.data(), buf.size());
        if (len < 0) {
            return "";
        }
        if (len < buf.size()) {
            return std::string(buf.begin(), buf.begin() + len);
        }
        buf.resize(buf.size() * 2);
    }
}

static void my_backtrace_error_callback (void *data, const char *msg,
                                  int errnum)
{
    PrintCrashInfo(strprintf("libbacktrace error: %d - %s\n", errnum, msg));
}

static backtrace_state* GetLibBacktraceState()
{
    static std::string exeFileName = GetExeFileName();
    static const char* exeFileNamePtr = exeFileName.empty() ? nullptr : exeFileName.c_str();
    static backtrace_state* st = backtrace_create_state(exeFileNamePtr, 1, my_backtrace_error_callback, NULL);
    return st;
}

#if WIN32
// PC addresses returned by StackWalk64 are in the real mapped space, while libbacktrace expects them to be in the
// default mapped space starting at 0x400000. This method converts the address.
// TODO this is probably the same reason libbacktrace is not able to gather the stacktrace on Windows (returns pointers like 0x1 or 0xfffffff)
// If they ever fix this problem, we might end up converting to invalid addresses here
static uintptr_t ConvertAddress(uintptr_t addr)
{
    MEMORY_BASIC_INFORMATION mbi;

    if (!VirtualQuery((PVOID)addr, &mbi, sizeof(mbi)))
        return 0;

    uintptr_t hMod = (uintptr_t)mbi.AllocationBase;
    uintptr_t offset = addr - hMod;
    return 0x400000 + offset;
}

static __attribute__((noinline)) std::vector<uintptr_t> GetStackFrames(size_t skip, size_t max_frames, const CONTEXT* pContext = nullptr)
{
    // We can't use libbacktrace for stack unwinding on Windows as it returns invalid addresses (like 0x1 or 0xffffffff)
    static BOOL symInitialized = SymInitialize(GetCurrentProcess(), NULL, TRUE);

    // dbghelp is not thread safe
    static std::mutex m;
    std::lock_guard<std::mutex> l(m);

    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    CONTEXT context;
    if (!pContext) {
        memset(&context, 0, sizeof(CONTEXT));
        context.ContextFlags = CONTEXT_FULL;
        RtlCaptureContext(&context);
    } else {
        memcpy(&context, pContext, sizeof(CONTEXT));
    }

    DWORD image;
    STACKFRAME64 stackframe;
    ZeroMemory(&stackframe, sizeof(STACKFRAME64));

#ifdef __i386__
    image = IMAGE_FILE_MACHINE_I386;
    stackframe.AddrPC.Offset = context.Eip;
    stackframe.AddrPC.Mode = AddrModeFlat;
    stackframe.AddrFrame.Offset = context.Ebp;
    stackframe.AddrFrame.Mode = AddrModeFlat;
    stackframe.AddrStack.Offset = context.Esp;
    stackframe.AddrStack.Mode = AddrModeFlat;
#elif __x86_64__
    image = IMAGE_FILE_MACHINE_AMD64;
    stackframe.AddrPC.Offset = context.Rip;
    stackframe.AddrPC.Mode = AddrModeFlat;
    stackframe.AddrFrame.Offset = context.Rbp;
    stackframe.AddrFrame.Mode = AddrModeFlat;
    stackframe.AddrStack.Offset = context.Rsp;
    stackframe.AddrStack.Mode = AddrModeFlat;
    if (!pContext) {
        skip++; // skip this method
    }
#else
#error unsupported architecture
#endif

    std::vector<uintptr_t> ret;

    size_t i = 0;
    while (ret.size() < max_frames) {
        BOOL result = StackWalk64(
                image, process, thread,
                &stackframe, &context, NULL,
                SymFunctionTableAccess64, SymGetModuleBase64, NULL);

        if (!result) {
            break;
        }
        if (i >= skip) {
            uintptr_t pc = ConvertAddress(stackframe.AddrPC.Offset);
            if (pc == 0) {
                pc = stackframe.AddrPC.Offset;
            }
            ret.emplace_back(pc);
        }
        i++;
    }

    return ret;
}
#else
static int my_backtrace_simple_callback(void *data, uintptr_t pc)
{
    auto v = (std::vector<uintptr_t>*)data;
    v->emplace_back(pc);
    if (v->size() >= 128) {
        // abort
        return 1;
    }
    return 0;
}

static __attribute__((noinline)) std::vector<uintptr_t> GetStackFrames(size_t skip, size_t max_frames)
{
    // FYI, this is not using libbacktrace, but "backtrace()" from <execinfo.h>
    std::vector<void*> buf(max_frames);
    int count = backtrace(buf.data(), (int)buf.size());
    if (count == 0) {
        return {};
    }
    buf.resize((size_t)count);

    std::vector<uintptr_t> ret;
    ret.reserve(count);
    for (size_t i = skip + 1; i < buf.size(); i++) {
        ret.emplace_back((uintptr_t) buf[i]);
    }
    return ret;
}
#endif

struct stackframe_info {
    uintptr_t pc;
    std::string filename;
    int lineno;
    std::string function;
};

static int my_backtrace_full_callback (void *data, uintptr_t pc, const char *filename, int lineno, const char *function)
{
    auto sis = (std::vector<stackframe_info>*)data;
    stackframe_info si;
    si.pc = pc;
    si.lineno = lineno;
    if (filename) {
        si.filename = filename;
    }
    if (function) {
        si.function = DemangleSymbol(function);
    }
    sis->emplace_back(si);
    if (sis->size() >= 128) {
        // abort
        return 1;
    }
    if (si.function == "mainCRTStartup" ||
        si.function == "pthread_create_wrapper" ||
        si.function == "__tmainCRTStartup") {
        // on Windows, stack frames are unwinded into invalid PCs after entry points
        // this doesn't catch all cases unfortunately
        return 1;
    }
    return 0;
}

static std::vector<stackframe_info> GetStackFrameInfos(const std::vector<uintptr_t>& stackframes)
{
    std::vector<stackframe_info> infos;
    infos.reserve(stackframes.size());

    for (size_t i = 0; i < stackframes.size(); i++) {
        if (backtrace_pcinfo(GetLibBacktraceState(), stackframes[i], my_backtrace_full_callback, my_backtrace_error_callback, &infos)) {
            break;
        }
    }

    return infos;
}

static std::string GetStackFrameInfosStr(const std::vector<stackframe_info>& sis, size_t spaces = 2)
{
    if (sis.empty()) {
        return "\n";
    }

    std::string sp;
    for (size_t i = 0; i < spaces; i++) {
        sp += " ";
    }

    std::vector<std::string> lstrs;
    lstrs.reserve(sis.size());

    for (size_t i = 0; i < sis.size(); i++) {
        auto& si = sis[i];

        std::string lstr;
        if (!si.filename.empty()) {
            std::vector<char> vecFilename(si.filename.size() + 1, '\0');
            strcpy(vecFilename.data(), si.filename.c_str());
            lstr += basename(vecFilename.data());
        } else {
            lstr += "<unknown-file>";
        }
        if (si.lineno != 0) {
            lstr += strprintf(":%d", si.lineno);
        }

        lstrs.emplace_back(lstr);
    }

    // get max "filename:line" length so we can better format it
    size_t lstrlen = std::max_element(lstrs.begin(), lstrs.end(), [](const std::string& a, const std::string& b) { return a.size() < b.size(); })->size();

    std::string fmtStr = strprintf("%%2d#: (0x%%08X) %%-%ds - %%s\n", lstrlen);

    std::string s;
    for (size_t i = 0; i < sis.size(); i++) {
        auto& si = sis[i];

        auto& lstr = lstrs[i];

        std::string fstr;
        if (!si.function.empty()) {
            fstr = si.function;
        } else {
            fstr = "???";
        }

        std::string s2 = strprintf(fmtStr, i, si.pc, lstr, fstr);

        s += sp;
        s += s2;
    }
    return s;
}

static std::mutex g_stacktraces_mutex;
static std::map<void*, std::shared_ptr<std::vector<uintptr_t>>> g_stacktraces;

#if STACKTRACE_WRAPPED_CXX_ABI
// These come in through -Wl,-wrap
// It only works on GCC
extern "C" void* __real___cxa_allocate_exception(size_t thrown_size);
extern "C" void __real___cxa_free_exception(void * thrown_exception);
#if __clang__
#error not supported on WIN32 (no dlsym support)
#elif WIN32
extern "C" void __real__assert(const char *assertion, const char *file, unsigned int line);
extern "C" void __real__wassert(const wchar_t *assertion, const wchar_t *file, unsigned int line);
#else
extern "C" void __real___assert_fail(const char *assertion, const char *file, unsigned int line, const char *function);
#endif
#else
// Clang does not support -Wl,-wrap, so we must use dlsym
// This is ok because at the same time Clang only supports dynamic linking to libc/libc++
extern "C" void* __real___cxa_allocate_exception(size_t thrown_size)
{
    static auto f = (void*(*)(size_t))dlsym(RTLD_NEXT, "__cxa_allocate_exception");
    return f(thrown_size);
}
extern "C" void __real___cxa_free_exception(void * thrown_exception)
{
    static auto f = (void(*)(void*))dlsym(RTLD_NEXT, "__cxa_free_exception");
    return f(thrown_exception);
}
#if __clang__
extern "C" void __attribute__((noreturn)) __real___assert_rtn(const char *function, const char *file, int line, const char *assertion)
{
    static auto f = (void(__attribute__((noreturn)) *) (const char*, const char*, int, const char*))dlsym(RTLD_NEXT, "__assert_rtn");
    f(function, file, line, assertion);
}
#elif WIN32
#error not supported on WIN32 (no dlsym support)
#else
extern "C" void __real___assert_fail(const char *assertion, const char *file, unsigned int line, const char *function)
{
    static auto f = (void(*)(const char*, const char*, unsigned int, const char*))dlsym(RTLD_NEXT, "__assert_fail");
    f(assertion, file, line, function);
}
#endif
#endif

#if STACKTRACE_WRAPPED_CXX_ABI
#define WRAPPED_NAME(x) __wrap_##x
#else
#define WRAPPED_NAME(x) x
#endif

extern "C" void* __attribute__((noinline)) WRAPPED_NAME(__cxa_allocate_exception)(size_t thrown_size)
{
    // grab the current stack trace and store it in the global exception->stacktrace map
    auto localSt = GetStackFrames(1, 16);

    // WARNING keep this as it is and don't try to optimize it (no std::move, no std::make_shared, ...)
    // trying to optimize this will cause the optimizer to move the GetStackFrames() call deep into the stl libs
    std::shared_ptr<std::vector<uintptr_t>> st(new std::vector<uintptr_t>(localSt));

    void* p = __real___cxa_allocate_exception(thrown_size);

    std::lock_guard<std::mutex> l(g_stacktraces_mutex);
    g_stacktraces.emplace(p, st);
    return p;
}

extern "C" void __attribute__((noinline)) WRAPPED_NAME(__cxa_free_exception)(void * thrown_exception)
{
    __real___cxa_free_exception(thrown_exception);

    std::lock_guard<std::mutex> l(g_stacktraces_mutex);
    g_stacktraces.erase(thrown_exception);
}

#if __clang__
extern "C" void __attribute__((noinline)) WRAPPED_NAME(__assert_rtn)(const char *function, const char *file, int line, const char *assertion)
{
    auto st = GetCurrentStacktraceStr(1);
    PrintCrashInfo(strprintf("#### assertion failed: %s ####\n%s", assertion, st));
    skipAbortSignal = true;
    __real___assert_rtn(function, file, line, assertion);
}
#elif WIN32
extern "C" void __attribute__((noinline)) WRAPPED_NAME(_assert)(const char *assertion, const char *file, unsigned int line)
{
    auto st = GetCurrentStacktraceStr(1);
    PrintCrashInfo(strprintf("#### assertion failed: %s ####\n%s", assertion, st));
    skipAbortSignal = true;
    __real__assert(assertion, file, line);
}
extern "C" void __attribute__((noinline)) WRAPPED_NAME(_wassert)(const wchar_t *assertion, const wchar_t *file, unsigned int line)
{
    auto st = GetCurrentStacktraceStr(1);
    PrintCrashInfo(strprintf("#### assertion failed: %s ####\n%s", std::string(assertion, assertion + wcslen(assertion)), st));
    skipAbortSignal = true;
    __real__wassert(assertion, file, line);
}
#else
extern "C" void __attribute__((noinline)) WRAPPED_NAME(__assert_fail)(const char *assertion, const char *file, unsigned int line, const char *function)
{
    auto st = GetCurrentStacktraceStr(1);
    PrintCrashInfo(strprintf("#### assertion failed: %s ####\n%s", assertion, st));
    skipAbortSignal = true;
    __real___assert_fail(assertion, file, line, function);
}
#endif

static std::shared_ptr<std::vector<uintptr_t>> GetExceptionStacktrace(const std::exception_ptr& e)
{
    void* p = *(void**)&e;

    std::lock_guard<std::mutex> l(g_stacktraces_mutex);
    auto it = g_stacktraces.find(p);
    if (it == g_stacktraces.end()) {
        return nullptr;
    }
    return it->second;
}
#endif //ENABLE_STACKTRACES

std::string GetExceptionStacktraceStr(const std::exception_ptr& e)
{
#ifdef ENABLE_STACKTRACES
    auto stackframes = GetExceptionStacktrace(e);
    if (stackframes && !stackframes->empty()) {
        auto infos = GetStackFrameInfos(*stackframes);
        return GetStackFrameInfosStr(infos);
    }
#endif
    return "<no stacktrace>\n";
}

std::string __attribute__((noinline)) GetCurrentStacktraceStr(size_t skip, size_t max_depth)
{
#ifdef ENABLE_STACKTRACES
    auto stackframes = GetStackFrames(skip + 1, max_depth); // +1 to skip current method
    auto infos = GetStackFrameInfos(stackframes);
    return GetStackFrameInfosStr(infos);
#else
    return "<no stacktrace>\n";
#endif
}

std::string GetPrettyExceptionStr(const std::exception_ptr& e)
{
    if (!e) {
        return "<no exception>\n";
    }

    std::string type;
    std::string what;

    try {
        // rethrow and catch the exception as there is no other way to reliably cast to the real type (not possible with RTTI)
        std::rethrow_exception(e);
    } catch (const std::exception& e) {
        type = abi::__cxa_current_exception_type()->name();
        what = GetExceptionWhat(e);
    } catch (const std::string& e) {
        type = abi::__cxa_current_exception_type()->name();
        what = GetExceptionWhat(e);
    } catch (const char* e) {
        type = abi::__cxa_current_exception_type()->name();
        what = GetExceptionWhat(e);
    } catch (int e) {
        type = abi::__cxa_current_exception_type()->name();
        what = GetExceptionWhat(e);
    } catch (...) {
        type = abi::__cxa_current_exception_type()->name();
        what = "<unknown>";
    }

    if (type.empty()) {
        type = "<unknown>";
    } else {
        type = DemangleSymbol(type);
    }

    std::string s = strprintf("Exception: type=%s, what=\"%s\"\n", type, what);

#if ENABLE_STACKTRACES
    s += GetExceptionStacktraceStr(e);
#endif

    return s;
}

static void terminate_handler()
{
    auto exc = std::current_exception();

    std::string s, s2;
    s += "#### std::terminate() called, aborting ####\n";

    if (exc) {
        s += "#### UNCAUGHT EXCEPTION ####\n";
        s2 = GetPrettyExceptionStr(exc);
    } else {
        s += "#### UNKNOWN REASON ####\n";
#ifdef ENABLE_STACKTRACES
        s2 = GetCurrentStacktraceStr(0);
#else
        s2 = "\n";
#endif
    }

    PrintCrashInfo(strprintf("%s%s", s, s2));

    skipAbortSignal = true;
    std::abort();
}

void RegisterPrettyTerminateHander()
{
    std::set_terminate(terminate_handler);
}

#ifdef ENABLE_STACKTRACES
#if !WIN32
static void HandlePosixSignal(int s)
{
    if (s == SIGABRT && skipAbortSignal) {
        return;
    }
    std::string st = GetCurrentStacktraceStr(0);
    const char* name = strsignal(s);
    if (!name) {
        name = "UNKNOWN";
    }
    PrintCrashInfo(strprintf("#### signal %s ####\n%s", name, st));

    // avoid a signal loop
    skipAbortSignal = true;
    std::abort();
}

#else
static void DoHandleWindowsException(EXCEPTION_POINTERS * ExceptionInfo)
{
    std::string excType;
    switch(ExceptionInfo->ExceptionRecord->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION: excType = "EXCEPTION_ACCESS_VIOLATION"; break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: excType = "EXCEPTION_ARRAY_BOUNDS_EXCEEDED"; break;
    case EXCEPTION_BREAKPOINT: excType = "EXCEPTION_BREAKPOINT"; break;
    case EXCEPTION_DATATYPE_MISALIGNMENT: excType = "EXCEPTION_DATATYPE_MISALIGNMENT"; break;
    case EXCEPTION_FLT_DENORMAL_OPERAND: excType = "EXCEPTION_FLT_DENORMAL_OPERAND"; break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO: excType = "EXCEPTION_FLT_DIVIDE_BY_ZERO"; break;
    case EXCEPTION_FLT_INEXACT_RESULT: excType = "EXCEPTION_FLT_INEXACT_RESULT"; break;
    case EXCEPTION_FLT_INVALID_OPERATION: excType = "EXCEPTION_FLT_INVALID_OPERATION"; break;
    case EXCEPTION_FLT_OVERFLOW: excType = "EXCEPTION_FLT_OVERFLOW"; break;
    case EXCEPTION_FLT_STACK_CHECK: excType = "EXCEPTION_FLT_STACK_CHECK"; break;
    case EXCEPTION_FLT_UNDERFLOW: excType = "EXCEPTION_FLT_UNDERFLOW"; break;
    case EXCEPTION_ILLEGAL_INSTRUCTION: excType = "EXCEPTION_ILLEGAL_INSTRUCTION"; break;
    case EXCEPTION_IN_PAGE_ERROR: excType = "EXCEPTION_IN_PAGE_ERROR"; break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO: excType = "EXCEPTION_INT_DIVIDE_BY_ZERO"; break;
    case EXCEPTION_INT_OVERFLOW: excType = "EXCEPTION_INT_OVERFLOW"; break;
    case EXCEPTION_INVALID_DISPOSITION: excType = "EXCEPTION_INVALID_DISPOSITION"; break;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION: excType = "EXCEPTION_NONCONTINUABLE_EXCEPTION"; break;
    case EXCEPTION_PRIV_INSTRUCTION: excType = "EXCEPTION_PRIV_INSTRUCTION"; break;
    case EXCEPTION_SINGLE_STEP: excType = "EXCEPTION_SINGLE_STEP"; break;
    case EXCEPTION_STACK_OVERFLOW: excType = "EXCEPTION_STACK_OVERFLOW"; break;
    default: excType = "UNKNOWN"; break;
    }

    auto stackframes = GetStackFrames(0, 16, ExceptionInfo->ContextRecord);
    auto infos = GetStackFrameInfos(stackframes);
    auto infosStr = GetStackFrameInfosStr(infos);

    PrintCrashInfo(strprintf("#### Windows Exception %s ####\n%s", excType, infosStr));
}

LONG WINAPI HandleWindowsException(EXCEPTION_POINTERS * ExceptionInfo)
{
    if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW) {
        // We can't directly do the exception handling in this thread anymore as we need stack space for this
        // So let's do the handling in another thread
        // On Wine, the exception handler is not called at all
        std::thread([&]() {
            DoHandleWindowsException(ExceptionInfo);
        }).join();
    } else {
        DoHandleWindowsException(ExceptionInfo);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif
#endif // ENABLE_STACKTRACES

void RegisterPrettySignalHandlers()
{
#if ENABLE_STACKTRACES
#if WIN32
    SetUnhandledExceptionFilter(HandleWindowsException);
#else
    const std::vector<int> posix_signals = {
            // Signals for which the default action is "Core".
            SIGABRT,    // Abort signal from abort(3)
            SIGBUS,     // Bus error (bad memory access)
            SIGFPE,     // Floating point exception
            SIGILL,     // Illegal Instruction
            SIGIOT,     // IOT trap. A synonym for SIGABRT
            SIGQUIT,    // Quit from keyboard
            SIGSEGV,    // Invalid memory reference
            SIGSYS,     // Bad argument to routine (SVr4)
            SIGTRAP,    // Trace/breakpoint trap
            SIGXCPU,    // CPU time limit exceeded (4.2BSD)
            SIGXFSZ,    // File size limit exceeded (4.2BSD)
#if __APPLE__
            SIGEMT,     // emulation instruction executed
#endif
    };

    for (auto s : posix_signals) {
        struct sigaction sa_segv;
        sa_segv.sa_handler = HandlePosixSignal;
        sigemptyset(&sa_segv.sa_mask);
        sa_segv.sa_flags = 0;
        sigaction(s, &sa_segv, NULL);
    }
#endif
#endif
}

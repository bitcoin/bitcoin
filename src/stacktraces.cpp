// Copyright (c) 2014-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/dash-config.h>
#endif // HAVE_CONFIG_H

#include <stacktraces.h>
#include <fs.h>
#include <logging.h>
#include <random.h>
#include <streams.h>
#include <utilstrencodings.h>

#include <mutex>
#include <map>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>

#if WIN32
#include <windows.h>
#include <dbghelp.h>
#else
#ifdef ENABLE_STACKTRACES
#include <execinfo.h>
#endif
#include <unistd.h>
#include <signal.h>
#endif

#if !WIN32
#include <dlfcn.h>
#if !__APPLE__
#include <link.h>
#endif
#endif

#if __APPLE__
#include <mach-o/dyld.h>
#include <mach/mach_init.h>
#include <sys/sysctl.h>
#include <mach/mach_vm.h>
#endif

#ifdef ENABLE_STACKTRACES
#include <backtrace.h>
#endif

#include <string.h>

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

static ssize_t GetExeFileNameImpl(char* buf, size_t bufSize)
{
#if WIN32
    std::vector<TCHAR> tmp(bufSize);
    DWORD len = GetModuleFileName(nullptr, tmp.data(), bufSize);
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

static std::string GetExeFileName()
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

static std::string g_exeFileName = GetExeFileName();
static std::string g_exeFileBaseName = fs::path(g_exeFileName).filename().string();

#ifdef ENABLE_STACKTRACES
static void my_backtrace_error_callback (void *data, const char *msg,
                                  int errnum)
{
}

static backtrace_state* GetLibBacktraceState()
{
#if WIN32
    // libbacktrace is not able to handle the DWARF debuglink in the .exe
    // but luckily we can just specify the .dbg file here as it's a valid PE/XCOFF file
    static std::string debugFileName = g_exeFileName + ".dbg";
    static const char* exeFileNamePtr = fs::exists(debugFileName) ? debugFileName.c_str() : g_exeFileName.c_str();
#else
    static const char* exeFileNamePtr = g_exeFileName.empty() ? nullptr : g_exeFileName.c_str();
#endif
    static backtrace_state* st = backtrace_create_state(exeFileNamePtr, 1, my_backtrace_error_callback, nullptr);
    return st;
}
#endif // ENABLE_STACKTRACES

#if WIN32
static uint64_t GetBaseAddress()
{
    return 0;
}

// PC addresses returned by StackWalk64 are in the real mapped space, while libbacktrace expects them to be in the
// default mapped space starting at 0x400000. This method converts the address.
// TODO this is probably the same reason libbacktrace is not able to gather the stacktrace on Windows (returns pointers like 0x1 or 0xfffffff)
// If they ever fix this problem, we might end up converting to invalid addresses here
static uint64_t ConvertAddress(uint64_t addr)
{
    MEMORY_BASIC_INFORMATION mbi;

    if (!VirtualQuery((PVOID)addr, &mbi, sizeof(mbi)))
        return 0;

    uint64_t hMod = (uint64_t)mbi.AllocationBase;
    uint64_t offset = addr - hMod;
    return 0x400000 + offset;
}

static __attribute__((noinline)) std::vector<uint64_t> GetStackFrames(size_t skip, size_t max_frames, const CONTEXT* pContext = nullptr)
{
#ifdef ENABLE_STACKTRACES
    // We can't use libbacktrace for stack unwinding on Windows as it returns invalid addresses (like 0x1 or 0xffffffff)
    static BOOL symInitialized = SymInitialize(GetCurrentProcess(), nullptr, TRUE);

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

    std::vector<uint64_t> ret;

    size_t i = 0;
    while (ret.size() < max_frames) {
        BOOL result = StackWalk64(
                image, process, thread,
                &stackframe, &context, nullptr,
                SymFunctionTableAccess64, SymGetModuleBase64, nullptr);

        if (!result) {
            break;
        }
        if (i >= skip) {
            uint64_t pc = ConvertAddress(stackframe.AddrPC.Offset);
            if (pc == 0) {
                pc = stackframe.AddrPC.Offset;
            }
            ret.emplace_back(pc);
        }
        i++;
    }

    return ret;
#else
    return {};
#endif // ENABLE_STACKTRACES
}
#else

#if __APPLE__
static uint64_t GetBaseAddress()
{
    mach_port_name_t target_task;
    vm_map_offset_t vmoffset;
    vm_map_size_t vmsize;
    uint32_t nesting_depth = 0;
    struct vm_region_submap_info_64 vbr;
    mach_msg_type_number_t vbrcount = 16;
    kern_return_t kr;

    kr = task_for_pid(mach_task_self(), getpid(), &target_task);
    if (kr != KERN_SUCCESS) {
        return 0;
    }

    kr = mach_vm_region_recurse(target_task, &vmoffset, &vmsize, &nesting_depth, (vm_region_recurse_info_t)&vbr, &vbrcount);
    if (kr != KERN_SUCCESS) {
        return 0;
    }

    return vmoffset;
}
#else
static int dl_iterate_callback(struct dl_phdr_info* info, size_t s, void* data)
{
    uint64_t* p = (uint64_t*)data;
    if (info->dlpi_name == nullptr || info->dlpi_name[0] == '\0') {
        *p = info->dlpi_addr;
    }
    return 0;
}

static uint64_t GetBaseAddress()
{
    uint64_t basePtr = 0;
    dl_iterate_phdr(dl_iterate_callback, &basePtr);
    return basePtr;
}
#endif

static __attribute__((noinline)) std::vector<uint64_t> GetStackFrames(size_t skip, size_t max_frames)
{
#ifdef ENABLE_STACKTRACES
    // FYI, this is not using libbacktrace, but "backtrace()" from <execinfo.h>
    std::vector<void*> buf(max_frames);
    int count = backtrace(buf.data(), (int)buf.size());
    if (count == 0) {
        return {};
    }
    buf.resize((size_t)count);

    std::vector<uint64_t> ret;
    ret.reserve(count);
    for (size_t i = skip + 1; i < buf.size(); i++) {
        ret.emplace_back((uint64_t) buf[i]);
    }
    return ret;
#else
    return {};
#endif // ENABLE_STACKTRACES
}
#endif

struct stackframe_info {
    uint64_t pc{0};
    std::string filename;
    int lineno{-1};
    std::string function;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(pc);
        READWRITE(filename);
        READWRITE(lineno);
        READWRITE(function);
    }
};

#ifdef ENABLE_STACKTRACES
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

static std::vector<stackframe_info> GetStackFrameInfos(const std::vector<uint64_t>& stackframes)
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
#else
static std::vector<stackframe_info> GetStackFrameInfos(const std::vector<uint64_t>& stackframes)
{
    return {};
}
#endif // ENABLE_STACKTRACES

struct crash_info_header
{
    std::string magic;
    uint16_t version;
    std::string exeFileName;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(magic);
        READWRITE(version);
        READWRITE(exeFileName);
    }
};

struct crash_info
{
    std::string crashDescription;
    std::vector<uint64_t> stackframes;
    std::vector<stackframe_info> stackframeInfos;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(crashDescription);
        READWRITE(stackframes);
        READWRITE(stackframeInfos);
    }

    void ConvertAddresses(int64_t offset)
    {
        for (auto& sf : stackframes) {
            sf += offset;
        }
        for (auto& sfi : stackframeInfos) {
            sfi.pc += offset;
        }
    }
};

static std::string GetCrashInfoStrNoDebugInfo(crash_info ci)
{
    static uint64_t basePtr = GetBaseAddress();

    CDataStream ds(SER_DISK, 0);

    crash_info_header hdr;
    hdr.magic = "DashCrashInfo";
    hdr.version = 1;
    hdr.exeFileName = g_exeFileBaseName;
    ds << hdr;

    ci.ConvertAddresses(-(int64_t)basePtr);
    ds << ci;

    auto ciStr = EncodeBase32((const unsigned char*)ds.data(), ds.size());
    std::string s = ci.crashDescription + "\n";
    s += strprintf("No debug information available for stacktrace. You should add debug information and then run:\n"
                   "%s -printcrashinfo=%s\n", g_exeFileBaseName, ciStr);
    return s;
}

static std::string GetCrashInfoStr(const crash_info& ci, size_t spaces = 2);

std::string GetCrashInfoStrFromSerializedStr(const std::string& ciStr)
{
    static uint64_t basePtr = GetBaseAddress();

    bool dataInvalid = false;
    auto buf = DecodeBase32(ciStr.c_str(), &dataInvalid);
    if (buf.empty() || dataInvalid) {
        return "Error while deserializing crash info";
    }

    CDataStream ds(buf, SER_DISK, 0);

    crash_info_header hdr;
    try {
        ds >> hdr;
    } catch (...) {
        return "Error while deserializing crash info header";
    }

    if (hdr.magic != "DashCrashInfo") {
        return "Invalid magic string";
    }
    if (hdr.version != 1) {
        return "Unsupported version";
    }
    if (hdr.exeFileName != g_exeFileBaseName) {
        return "Crash info is not for this executable";
    }

    crash_info ci;
    try {
        ds >> ci;
    } catch (...) {
        return "Error while deserializing crash info";
    }

    ci.ConvertAddresses(basePtr);

    if (ci.stackframeInfos.empty()) {
        std::vector<uint64_t> stackframes(ci.stackframes.begin(), ci.stackframes.end());
        ci.stackframeInfos = GetStackFrameInfos(stackframes);
    }

    return GetCrashInfoStr(ci);
}

static std::string GetCrashInfoStr(const crash_info& ci, size_t spaces)
{
    if (ci.stackframeInfos.empty()) {
        return GetCrashInfoStrNoDebugInfo(ci);
    }

    std::string sp;
    for (size_t i = 0; i < spaces; i++) {
        sp += " ";
    }

    std::vector<std::string> lstrs;
    lstrs.reserve(ci.stackframeInfos.size());

    for (size_t i = 0; i < ci.stackframeInfos.size(); i++) {
        auto& si = ci.stackframeInfos[i];

        std::string lstr;
        if (!si.filename.empty()) {
            lstr += fs::path(si.filename).filename().string();
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

    std::string s = ci.crashDescription + "\n";
    for (size_t i = 0; i < ci.stackframeInfos.size(); i++) {
        auto& si = ci.stackframeInfos[i];

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

static void PrintCrashInfo(const crash_info& ci)
{
    auto str = GetCrashInfoStr(ci);
    LogPrintf("%s", str); /* Continued */
    fprintf(stderr, "%s", str.c_str());
    fflush(stderr);
}

#ifdef ENABLE_CRASH_HOOKS
static std::mutex g_stacktraces_mutex;
static std::map<void*, std::shared_ptr<std::vector<uint64_t>>> g_stacktraces;

#if CRASH_HOOKS_WRAPPED_CXX_ABI
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

#if CRASH_HOOKS_WRAPPED_CXX_ABI
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
    std::shared_ptr<std::vector<uint64_t>> st(new std::vector<uint64_t>(localSt));

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

static __attribute__((noinline)) crash_info GetCrashInfoFromAssertion(const char* assertion, const char* file, int line, const char* function)
{
    crash_info ci;
    ci.stackframes = GetStackFrames(1, 16);
    ci.crashDescription = "Assertion failure:";
    if (assertion) {
        ci.crashDescription += strprintf("\n  assertion: %s", assertion);
    }
    if (file) {
        ci.crashDescription += strprintf("\n  file: %s, line: %d", file, line);
    }
    if (function) {
        ci.crashDescription += strprintf("\n  function: %s", function);
    }
    ci.stackframeInfos = GetStackFrameInfos(ci.stackframes);
    return ci;
}

#if __clang__
extern "C" void __attribute__((noinline)) WRAPPED_NAME(__assert_rtn)(const char *function, const char *file, int line, const char *assertion)
{
    auto ci = GetCrashInfoFromAssertion(assertion, file, line, function);
    PrintCrashInfo(ci);
    skipAbortSignal = true;
    __real___assert_rtn(function, file, line, assertion);
}
#elif WIN32
extern "C" void __attribute__((noinline)) WRAPPED_NAME(_assert)(const char *assertion, const char *file, unsigned int line)
{
    auto ci = GetCrashInfoFromAssertion(assertion, file, line, nullptr);
    PrintCrashInfo(ci);
    skipAbortSignal = true;
    __real__assert(assertion, file, line);
}
extern "C" void __attribute__((noinline)) WRAPPED_NAME(_wassert)(const wchar_t *assertion, const wchar_t *file, unsigned int line)
{
    auto ci = GetCrashInfoFromAssertion(
            assertion ?  std::string(assertion, assertion + wcslen(assertion)).c_str() : nullptr,
            file ? std::string(file, file + wcslen(file)).c_str() : nullptr,
            line, nullptr);
    PrintCrashInfo(ci);
    skipAbortSignal = true;
    __real__wassert(assertion, file, line);
}
#else
extern "C" void __attribute__((noinline)) WRAPPED_NAME(__assert_fail)(const char *assertion, const char *file, unsigned int line, const char *function)
{
    auto ci = GetCrashInfoFromAssertion(assertion, file, line, function);
    PrintCrashInfo(ci);
    skipAbortSignal = true;
    __real___assert_fail(assertion, file, line, function);
}
#endif
#endif //ENABLE_CRASH_HOOKS

static std::shared_ptr<std::vector<uint64_t>> GetExceptionStacktrace(const std::exception_ptr& e)
{
#ifdef ENABLE_CRASH_HOOKS
    void* p = *(void**)&e;

    std::lock_guard<std::mutex> l(g_stacktraces_mutex);
    auto it = g_stacktraces.find(p);
    if (it == g_stacktraces.end()) {
        return nullptr;
    }
    return it->second;
#else
    return {};
#endif
}

crash_info GetCrashInfoFromException(const std::exception_ptr& e)
{
    crash_info ci;
    ci.crashDescription = "Exception: ";

    if (!e) {
        ci.crashDescription += "<null>";
        return ci;
    }

    std::string type;
    std::string what;

    auto getExceptionType = [&]() -> std::string {
        auto type = abi::__cxa_current_exception_type();
        if (type && (strlen(type->name()) > 0)) {
            return DemangleSymbol(type->name());
        }
        return "<unknown>";
    };

    try {
        // rethrow and catch the exception as there is no other way to reliably cast to the real type (not possible with RTTI)
        std::rethrow_exception(e);
    } catch (const std::exception& e) {
        type = getExceptionType();
        what = GetExceptionWhat(e);
    } catch (const std::string& e) {
        type = getExceptionType();
        what = GetExceptionWhat(e);
    } catch (const char* e) {
        type = getExceptionType();
        what = GetExceptionWhat(e);
    } catch (int e) {
        type = getExceptionType();
        what = GetExceptionWhat(e);
    } catch (...) {
        type = getExceptionType();
        what = "<unknown>";
    }

    ci.crashDescription += strprintf("type=%s, what=\"%s\"", type, what);

    auto stackframes = GetExceptionStacktrace(e);
    if (stackframes) {
        ci.stackframes = *stackframes;
        ci.stackframeInfos = GetStackFrameInfos(ci.stackframes);
    }

    return ci;
}

std::string GetPrettyExceptionStr(const std::exception_ptr& e)
{
    return GetCrashInfoStr(GetCrashInfoFromException(e));
}

static void terminate_handler()
{
    auto exc = std::current_exception();

    crash_info ci;
    ci.crashDescription = "std::terminate() called, aborting";

    if (exc) {
        auto ci2 = GetCrashInfoFromException(exc);
        ci.crashDescription = strprintf("std::terminate() called due to unhandled exception\n%s", ci2.crashDescription);
        ci.stackframes = std::move(ci2.stackframes);
        ci.stackframeInfos = std::move(ci2.stackframeInfos);
    } else {
        ci.crashDescription = "std::terminate() called due unknown reason";
        ci.stackframes = GetStackFrames(0, 16);
    }

    ci.stackframeInfos = GetStackFrameInfos(ci.stackframes);

    PrintCrashInfo(ci);

    skipAbortSignal = true;
    std::abort();
}

void RegisterPrettyTerminateHander()
{
    std::set_terminate(terminate_handler);
}

#if !WIN32
static void HandlePosixSignal(int s)
{
    if (s == SIGABRT && skipAbortSignal) {
        return;
    }

    const char* name = strsignal(s);
    if (!name) {
        name = "UNKNOWN";
    }

    crash_info ci;
    ci.crashDescription = strprintf("Posix Signal: %s", name);
    ci.stackframes = GetStackFrames(0, 16);
    ci.stackframeInfos = GetStackFrameInfos(ci.stackframes);
    PrintCrashInfo(ci);

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

    crash_info ci;
    ci.crashDescription = strprintf("Windows Exception: %s", excType);
    ci.stackframes = GetStackFrames(0, 16, ExceptionInfo->ContextRecord);
    ci.stackframeInfos = GetStackFrameInfos(ci.stackframes);

    PrintCrashInfo(ci);
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

void RegisterPrettySignalHandlers()
{
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
        sigaction(s, &sa_segv, nullptr);
    }
#endif
}

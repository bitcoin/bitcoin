// This file contains source that originates from:
// http://code.google.com/p/leveldbwin/source/browse/trunk/win32_impl_src/env_win32.h
// http://code.google.com/p/leveldbwin/source/browse/trunk/win32_impl_src/port_win32.cc
// Those files dont' have any explict license headers but the 
// project (http://code.google.com/p/leveldbwin/) lists the 'New BSD License'
// as the license.
#if defined(LEVELDB_PLATFORM_WINDOWS)
#include <map>


#include "leveldb/env.h"

#include "port/port.h"
#include "leveldb/slice.h"
#include "util/logging.h"

#include <shlwapi.h>
#include <process.h>
#include <cstring>
#include <stdio.h>
#include <errno.h>
#include <io.h>
#include <algorithm>

#ifdef max
#undef max
#endif

#ifndef va_copy
#define va_copy(d,s) ((d) = (s))
#endif

#if defined DeleteFile
#undef DeleteFile
#endif

//Declarations
namespace leveldb
{

namespace Win32
{

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)

std::string GetCurrentDir();
std::wstring GetCurrentDirW();

static const std::string CurrentDir = GetCurrentDir();
static const std::wstring CurrentDirW = GetCurrentDirW();

std::string& ModifyPath(std::string& path);
std::wstring& ModifyPath(std::wstring& path);

std::string GetLastErrSz();
std::wstring GetLastErrSzW();

size_t GetPageSize();

typedef void (*ScheduleProc)(void*) ;

struct WorkItemWrapper
{
    WorkItemWrapper(ScheduleProc proc_,void* content_);
    ScheduleProc proc;
    void* pContent;
};

DWORD WINAPI WorkItemWrapperProc(LPVOID pContent);

class Win32SequentialFile : public SequentialFile
{
public:
    friend class Win32Env;
    virtual ~Win32SequentialFile();
    virtual Status Read(size_t n, Slice* result, char* scratch);
    virtual Status Skip(uint64_t n);
    BOOL isEnable();
private:
    BOOL _Init();
    void _CleanUp();
    Win32SequentialFile(const std::string& fname);
    std::string _filename;
    ::HANDLE _hFile;
    DISALLOW_COPY_AND_ASSIGN(Win32SequentialFile);
};

class Win32RandomAccessFile : public RandomAccessFile
{
public:
    friend class Win32Env;
    virtual ~Win32RandomAccessFile();
    virtual Status Read(uint64_t offset, size_t n, Slice* result,char* scratch) const;
    BOOL isEnable();
private:
    BOOL _Init(LPCWSTR path);
    void _CleanUp();
    Win32RandomAccessFile(const std::string& fname);
    HANDLE _hFile;
    const std::string _filename;
    DISALLOW_COPY_AND_ASSIGN(Win32RandomAccessFile);
};

class Win32MapFile : public WritableFile
{
public:
    Win32MapFile(const std::string& fname);

    ~Win32MapFile();
    virtual Status Append(const Slice& data);
    virtual Status Close();
    virtual Status Flush();
    virtual Status Sync();
    BOOL isEnable();
private:
    std::string _filename;
    HANDLE _hFile;
    size_t _page_size;
    size_t _map_size;       // How much extra memory to map at a time
    char* _base;            // The mapped region
    HANDLE _base_handle;	
    char* _limit;           // Limit of the mapped region
    char* _dst;             // Where to write next  (in range [base_,limit_])
    char* _last_sync;       // Where have we synced up to
    uint64_t _file_offset;  // Offset of base_ in file
    //LARGE_INTEGER file_offset_;
    // Have we done an munmap of unsynced data?
    bool _pending_sync;

    // Roundup x to a multiple of y
    static size_t _Roundup(size_t x, size_t y);
    size_t _TruncateToPageBoundary(size_t s);
    bool _UnmapCurrentRegion();
    bool _MapNewRegion();
    DISALLOW_COPY_AND_ASSIGN(Win32MapFile);
    BOOL _Init(LPCWSTR Path);
};

class Win32FileLock : public FileLock
{
public:
    friend class Win32Env;
    virtual ~Win32FileLock();
    BOOL isEnable();
private:
    BOOL _Init(LPCWSTR path);
    void _CleanUp();
    Win32FileLock(const std::string& fname);
    HANDLE _hFile;
    std::string _filename;
    DISALLOW_COPY_AND_ASSIGN(Win32FileLock);
};

class Win32Logger : public Logger
{
public: 
    friend class Win32Env;
    virtual ~Win32Logger();
    virtual void Logv(const char* format, va_list ap);
private:
    explicit Win32Logger(WritableFile* pFile);
    WritableFile* _pFileProxy;
    DISALLOW_COPY_AND_ASSIGN(Win32Logger);
};

class Win32Env : public Env
{
public:
    Win32Env();
    virtual ~Win32Env();
    virtual Status NewSequentialFile(const std::string& fname,
        SequentialFile** result);

    virtual Status NewRandomAccessFile(const std::string& fname,
        RandomAccessFile** result);
    virtual Status NewWritableFile(const std::string& fname,
        WritableFile** result);

    virtual bool FileExists(const std::string& fname);

    virtual Status GetChildren(const std::string& dir,
        std::vector<std::string>* result);

    virtual Status DeleteFile(const std::string& fname);

    virtual Status CreateDir(const std::string& dirname);

    virtual Status DeleteDir(const std::string& dirname);

    virtual Status GetFileSize(const std::string& fname, uint64_t* file_size);

    virtual Status RenameFile(const std::string& src,
        const std::string& target);

    virtual Status LockFile(const std::string& fname, FileLock** lock);

    virtual Status UnlockFile(FileLock* lock);

    virtual void Schedule(
        void (*function)(void* arg),
        void* arg);

    virtual void StartThread(void (*function)(void* arg), void* arg);

    virtual Status GetTestDirectory(std::string* path);

    //virtual void Logv(WritableFile* log, const char* format, va_list ap);

    virtual Status NewLogger(const std::string& fname, Logger** result);

    virtual uint64_t NowMicros();

    virtual void SleepForMicroseconds(int micros);
};

void ToWidePath(const std::string& value, std::wstring& target) {
	wchar_t buffer[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, value.c_str(), -1, buffer, MAX_PATH);
	target = buffer;
}

void ToNarrowPath(const std::wstring& value, std::string& target) {
	char buffer[MAX_PATH];
	WideCharToMultiByte(CP_ACP, 0, value.c_str(), -1, buffer, MAX_PATH, NULL, NULL);
	target = buffer;
}

std::string GetCurrentDir()
{
    CHAR path[MAX_PATH];
    ::GetModuleFileNameA(::GetModuleHandleA(NULL),path,MAX_PATH);
    *strrchr(path,'\\') = 0;
    return std::string(path);
}

std::wstring GetCurrentDirW()
{
    WCHAR path[MAX_PATH];
    ::GetModuleFileNameW(::GetModuleHandleW(NULL),path,MAX_PATH);
    *wcsrchr(path,L'\\') = 0;
    return std::wstring(path);
}

std::string& ModifyPath(std::string& path)
{
    if(path[0] == '/' || path[0] == '\\'){
        path = CurrentDir + path;
    }
    std::replace(path.begin(),path.end(),'/','\\');

    return path;
}

std::wstring& ModifyPath(std::wstring& path)
{
    if(path[0] == L'/' || path[0] == L'\\'){
        path = CurrentDirW + path;
    }
    std::replace(path.begin(),path.end(),L'/',L'\\');
    return path;
}

std::string GetLastErrSz()
{
    LPWSTR lpMsgBuf;
    FormatMessageW( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        0, // Default language
        (LPWSTR) &lpMsgBuf,
        0,
        NULL 
        );
    std::string Err;
	ToNarrowPath(lpMsgBuf, Err); 
    LocalFree( lpMsgBuf );
    return Err;
}

std::wstring GetLastErrSzW()
{
    LPVOID lpMsgBuf;
    FormatMessageW( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        0, // Default language
        (LPWSTR) &lpMsgBuf,
        0,
        NULL 
        );
    std::wstring Err = (LPCWSTR)lpMsgBuf;
    LocalFree(lpMsgBuf);
    return Err;
}

WorkItemWrapper::WorkItemWrapper( ScheduleProc proc_,void* content_ ) :
    proc(proc_),pContent(content_)
{

}

DWORD WINAPI WorkItemWrapperProc(LPVOID pContent)
{
    WorkItemWrapper* item = static_cast<WorkItemWrapper*>(pContent);
    ScheduleProc TempProc = item->proc;
    void* arg = item->pContent;
    delete item;
    TempProc(arg);
    return 0;
}

size_t GetPageSize()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return std::max(si.dwPageSize,si.dwAllocationGranularity);
}

const size_t g_PageSize = GetPageSize();


Win32SequentialFile::Win32SequentialFile( const std::string& fname ) :
    _filename(fname),_hFile(NULL)
{
    _Init();
}

Win32SequentialFile::~Win32SequentialFile()
{
    _CleanUp();
}

Status Win32SequentialFile::Read( size_t n, Slice* result, char* scratch )
{
    Status sRet;
    DWORD hasRead = 0;
    if(_hFile && ReadFile(_hFile,scratch,n,&hasRead,NULL) ){
        *result = Slice(scratch,hasRead);
    } else {
        sRet = Status::IOError(_filename, Win32::GetLastErrSz() );
    }
    return sRet;
}

Status Win32SequentialFile::Skip( uint64_t n )
{
    Status sRet;
    LARGE_INTEGER Move,NowPointer;
    Move.QuadPart = n;
    if(!SetFilePointerEx(_hFile,Move,&NowPointer,FILE_CURRENT)){
        sRet = Status::IOError(_filename,Win32::GetLastErrSz());
    }
    return sRet;
}

BOOL Win32SequentialFile::isEnable()
{
    return _hFile ? TRUE : FALSE;
}

BOOL Win32SequentialFile::_Init()
{
	std::wstring path;
	ToWidePath(_filename, path);
	_hFile = CreateFileW(path.c_str(),
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);
    return _hFile ? TRUE : FALSE;
}

void Win32SequentialFile::_CleanUp()
{
    if(_hFile){
        CloseHandle(_hFile);
        _hFile = NULL;
    }
}

Win32RandomAccessFile::Win32RandomAccessFile( const std::string& fname ) :
    _filename(fname),_hFile(NULL)
{
	std::wstring path;
	ToWidePath(fname, path);
    _Init( path.c_str() );
}

Win32RandomAccessFile::~Win32RandomAccessFile()
{
    _CleanUp();
}

Status Win32RandomAccessFile::Read(uint64_t offset,size_t n,Slice* result,char* scratch) const
{
    Status sRet;
    OVERLAPPED ol = {0};
    ZeroMemory(&ol,sizeof(ol));
    ol.Offset = (DWORD)offset;
    ol.OffsetHigh = (DWORD)(offset >> 32);
    DWORD hasRead = 0;
    if(!ReadFile(_hFile,scratch,n,&hasRead,&ol))
        sRet = Status::IOError(_filename,Win32::GetLastErrSz());
    else
        *result = Slice(scratch,hasRead);
    return sRet;
}

BOOL Win32RandomAccessFile::_Init( LPCWSTR path )
{
    BOOL bRet = FALSE;
    if(!_hFile)
        _hFile = ::CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,NULL);
    if(!_hFile || _hFile == INVALID_HANDLE_VALUE )
        _hFile = NULL;
    else
        bRet = TRUE;
    return bRet;
}

BOOL Win32RandomAccessFile::isEnable()
{
    return _hFile ? TRUE : FALSE;
}

void Win32RandomAccessFile::_CleanUp()
{
    if(_hFile){
        ::CloseHandle(_hFile);
        _hFile = NULL;
    }
}

size_t Win32MapFile::_Roundup( size_t x, size_t y )
{
    return ((x + y - 1) / y) * y;
}

size_t Win32MapFile::_TruncateToPageBoundary( size_t s )
{
    s -= (s & (_page_size - 1));
    assert((s % _page_size) == 0);
    return s;
}

bool Win32MapFile::_UnmapCurrentRegion()
{
    bool result = true;
    if (_base != NULL) {
        if (_last_sync < _limit) {
            // Defer syncing this data until next Sync() call, if any
            _pending_sync = true;
        }
        if (!UnmapViewOfFile(_base) || !CloseHandle(_base_handle))
            result = false;
        _file_offset += _limit - _base;
        _base = NULL;
        _base_handle = NULL;
        _limit = NULL;
        _last_sync = NULL;
        _dst = NULL;
        // Increase the amount we map the next time, but capped at 1MB
        if (_map_size < (1<<20)) {
            _map_size *= 2;
        }
    }
    return result;
}

bool Win32MapFile::_MapNewRegion()
{
    assert(_base == NULL);
    //LONG newSizeHigh = (LONG)((file_offset_ + map_size_) >> 32);
    //LONG newSizeLow = (LONG)((file_offset_ + map_size_) & 0xFFFFFFFF);
    DWORD off_hi = (DWORD)(_file_offset >> 32);
    DWORD off_lo = (DWORD)(_file_offset & 0xFFFFFFFF);
    LARGE_INTEGER newSize;
    newSize.QuadPart = _file_offset + _map_size;
    SetFilePointerEx(_hFile, newSize, NULL, FILE_BEGIN);
    SetEndOfFile(_hFile);

    _base_handle = CreateFileMappingA(
        _hFile,
        NULL,
        PAGE_READWRITE,
        0,
        0,
        0);
    if (_base_handle != NULL) {
        _base = (char*) MapViewOfFile(_base_handle,
            FILE_MAP_ALL_ACCESS,
            off_hi,
            off_lo,
            _map_size);
        if (_base != NULL) {
            _limit = _base + _map_size;
            _dst = _base;
            _last_sync = _base;
            return true;
        }
    }
    return false;
}

Win32MapFile::Win32MapFile( const std::string& fname) :
    _filename(fname),
    _hFile(NULL),
    _page_size(Win32::g_PageSize),
    _map_size(_Roundup(65536, Win32::g_PageSize)),
    _base(NULL),
    _base_handle(NULL),
    _limit(NULL),
    _dst(NULL),
    _last_sync(NULL),
    _file_offset(0),
    _pending_sync(false)
{
	std::wstring path;
	ToWidePath(fname, path);
    _Init(path.c_str());
    assert((Win32::g_PageSize & (Win32::g_PageSize - 1)) == 0);
}

Status Win32MapFile::Append( const Slice& data )
{
    const char* src = data.data();
    size_t left = data.size();
    Status s;
    while (left > 0) {
        assert(_base <= _dst);
        assert(_dst <= _limit);
        size_t avail = _limit - _dst;
        if (avail == 0) {
            if (!_UnmapCurrentRegion() ||
                !_MapNewRegion()) {
                    return Status::IOError("WinMmapFile.Append::UnmapCurrentRegion or MapNewRegion: ", Win32::GetLastErrSz());
            }
        }
        size_t n = (left <= avail) ? left : avail;
        memcpy(_dst, src, n);
        _dst += n;
        src += n;
        left -= n;
    }
    return s;
}

Status Win32MapFile::Close()
{
    Status s;
    size_t unused = _limit - _dst;
    if (!_UnmapCurrentRegion()) {
        s = Status::IOError("WinMmapFile.Close::UnmapCurrentRegion: ",Win32::GetLastErrSz());
    } else if (unused > 0) {
        // Trim the extra space at the end of the file
        LARGE_INTEGER newSize;
        newSize.QuadPart = _file_offset - unused;
        if (!SetFilePointerEx(_hFile, newSize, NULL, FILE_BEGIN)) {
            s = Status::IOError("WinMmapFile.Close::SetFilePointer: ",Win32::GetLastErrSz());
        } else 
            SetEndOfFile(_hFile);
    }
    if (!CloseHandle(_hFile)) {
        if (s.ok()) {
            s = Status::IOError("WinMmapFile.Close::CloseHandle: ", Win32::GetLastErrSz());
        }
    }
    _hFile = INVALID_HANDLE_VALUE;
    _base = NULL;
    _base_handle = NULL;
    _limit = NULL;

    return s;
}

Status Win32MapFile::Sync()
{
    Status s;
    if (_pending_sync) {
        // Some unmapped data was not synced
        _pending_sync = false;
        if (!FlushFileBuffers(_hFile)) {
            s = Status::IOError("WinMmapFile.Sync::FlushFileBuffers: ",Win32::GetLastErrSz());
        }
    }
    if (_dst > _last_sync) {
        // Find the beginnings of the pages that contain the first and last
        // bytes to be synced.
        size_t p1 = _TruncateToPageBoundary(_last_sync - _base);
        size_t p2 = _TruncateToPageBoundary(_dst - _base - 1);
        _last_sync = _dst;
        if (!FlushViewOfFile(_base + p1, p2 - p1 + _page_size)) {
            s = Status::IOError("WinMmapFile.Sync::FlushViewOfFile: ",Win32::GetLastErrSz());
        }
    }
    return s;
}

Status Win32MapFile::Flush()
{
    return Status::OK();
}

Win32MapFile::~Win32MapFile()
{
    if (_hFile != INVALID_HANDLE_VALUE) { 
        Win32MapFile::Close();
    }
}

BOOL Win32MapFile::_Init( LPCWSTR Path )
{
    DWORD Flag = PathFileExistsW(Path) ? OPEN_EXISTING : CREATE_ALWAYS;
    _hFile = CreateFileW(Path,
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ|FILE_SHARE_DELETE|FILE_SHARE_WRITE,
                         NULL,
                         Flag,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);
    if(!_hFile || _hFile == INVALID_HANDLE_VALUE)
        return FALSE;
    else
        return TRUE;
}

BOOL Win32MapFile::isEnable()
{
    return _hFile ? TRUE : FALSE;
}

Win32FileLock::Win32FileLock( const std::string& fname ) :
    _hFile(NULL),_filename(fname)
{
	std::wstring path;
	ToWidePath(fname, path);
	_Init(path.c_str());
}

Win32FileLock::~Win32FileLock()
{
    _CleanUp();
}

BOOL Win32FileLock::_Init( LPCWSTR path )
{
    BOOL bRet = FALSE;
    if(!_hFile)
        _hFile = ::CreateFileW(path,0,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(!_hFile || _hFile == INVALID_HANDLE_VALUE ){
        _hFile = NULL;
    }
    else
        bRet = TRUE;
    return bRet;
}

void Win32FileLock::_CleanUp()
{
    ::CloseHandle(_hFile);
    _hFile = NULL;
}

BOOL Win32FileLock::isEnable()
{
    return _hFile ? TRUE : FALSE;
}

Win32Logger::Win32Logger(WritableFile* pFile) : _pFileProxy(pFile)
{
    assert(_pFileProxy);
}

Win32Logger::~Win32Logger()
{
    if(_pFileProxy)
        delete _pFileProxy;
}

void Win32Logger::Logv( const char* format, va_list ap )
{
    uint64_t thread_id = ::GetCurrentThreadId();

    // We try twice: the first time with a fixed-size stack allocated buffer,
    // and the second time with a much larger dynamically allocated buffer.
    char buffer[500];
    for (int iter = 0; iter < 2; iter++) {
        char* base;
        int bufsize;
        if (iter == 0) {
            bufsize = sizeof(buffer);
            base = buffer;
        } else {
            bufsize = 30000;
            base = new char[bufsize];
        }
        char* p = base;
        char* limit = base + bufsize;

        SYSTEMTIME st;
        GetLocalTime(&st);
        p += snprintf(p, limit - p,
            "%04d/%02d/%02d-%02d:%02d:%02d.%06d %llx ",
            int(st.wYear),
            int(st.wMonth),
            int(st.wDay),
            int(st.wHour),
            int(st.wMinute),
            int(st.wMinute),
            int(st.wMilliseconds),
            static_cast<long long unsigned int>(thread_id));

        // Print the message
        if (p < limit) {
            va_list backup_ap;
            va_copy(backup_ap, ap);
            p += vsnprintf(p, limit - p, format, backup_ap);
            va_end(backup_ap);
        }

        // Truncate to available space if necessary
        if (p >= limit) {
            if (iter == 0) {
                continue;       // Try again with larger buffer
            } else {
                p = limit - 1;
            }
        }

        // Add newline if necessary
        if (p == base || p[-1] != '\n') {
            *p++ = '\n';
        }

        assert(p <= limit);
        DWORD hasWritten = 0;
        if(_pFileProxy){
            _pFileProxy->Append(Slice(base, p - base));
            _pFileProxy->Flush();
        }
        if (base != buffer) {
            delete[] base;
        }
        break;
    }
}

bool Win32Env::FileExists(const std::string& fname)
{
	std::string path = fname;
    std::wstring wpath;
	ToWidePath(ModifyPath(path), wpath);
    return ::PathFileExistsW(wpath.c_str()) ? true : false;
}

Status Win32Env::GetChildren(const std::string& dir, std::vector<std::string>* result)
{
    Status sRet;
    ::WIN32_FIND_DATAW wfd;
    std::string path = dir;
    ModifyPath(path);
    path += "\\*.*";
	std::wstring wpath;
	ToWidePath(path, wpath);

	::HANDLE hFind = ::FindFirstFileW(wpath.c_str() ,&wfd);
    if(hFind && hFind != INVALID_HANDLE_VALUE){
        BOOL hasNext = TRUE;
        std::string child;
        while(hasNext){
            ToNarrowPath(wfd.cFileName, child); 
            if(child != ".." && child != ".")  {
                result->push_back(child);
            }
            hasNext = ::FindNextFileW(hFind,&wfd);
        }
        ::FindClose(hFind);
    }
    else
        sRet = Status::IOError(dir,"Could not get children.");
    return sRet;
}

void Win32Env::SleepForMicroseconds( int micros )
{
    ::Sleep((micros + 999) /1000);
}


Status Win32Env::DeleteFile( const std::string& fname )
{
    Status sRet;
    std::string path = fname;
    std::wstring wpath;
	ToWidePath(ModifyPath(path), wpath);

    if(!::DeleteFileW(wpath.c_str())) {
        sRet = Status::IOError(path, "Could not delete file.");
    }
    return sRet;
}

Status Win32Env::GetFileSize( const std::string& fname, uint64_t* file_size )
{
    Status sRet;
    std::string path = fname;
    std::wstring wpath;
	ToWidePath(ModifyPath(path), wpath);

    HANDLE file = ::CreateFileW(wpath.c_str(),
        GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    LARGE_INTEGER li;
    if(::GetFileSizeEx(file,&li)){
        *file_size = (uint64_t)li.QuadPart;
    }else
        sRet = Status::IOError(path,"Could not get the file size.");
    CloseHandle(file);
    return sRet;
}

Status Win32Env::RenameFile( const std::string& src, const std::string& target )
{
    Status sRet;
    std::string src_path = src;
    std::wstring wsrc_path;
	ToWidePath(ModifyPath(src_path), wsrc_path);
	std::string target_path = target;
    std::wstring wtarget_path;
	ToWidePath(ModifyPath(target_path), wtarget_path);

    if(!MoveFileW(wsrc_path.c_str(), wtarget_path.c_str() ) ){
        DWORD err = GetLastError();
        if(err == 0x000000b7){
            if(!::DeleteFileW(wtarget_path.c_str() ) )
                sRet = Status::IOError(src, "Could not rename file.");
			else if(!::MoveFileW(wsrc_path.c_str(),
                                 wtarget_path.c_str() ) )
                sRet = Status::IOError(src, "Could not rename file.");    
        }
    }
    return sRet;
}

Status Win32Env::LockFile( const std::string& fname, FileLock** lock )
{
    Status sRet;
    std::string path = fname;
    ModifyPath(path);
    Win32FileLock* _lock = new Win32FileLock(path);
    if(!_lock->isEnable()){
        delete _lock;
        *lock = NULL;
        sRet = Status::IOError(path, "Could not lock file.");
    }
    else
        *lock = _lock;
    return sRet;
}

Status Win32Env::UnlockFile( FileLock* lock )
{
    Status sRet;
    delete lock;
    return sRet;
}

void Win32Env::Schedule( void (*function)(void* arg), void* arg )
{
    QueueUserWorkItem(Win32::WorkItemWrapperProc,
                      new Win32::WorkItemWrapper(function,arg),
                      WT_EXECUTEDEFAULT);
}

void Win32Env::StartThread( void (*function)(void* arg), void* arg )
{
    ::_beginthread(function,0,arg);
}

Status Win32Env::GetTestDirectory( std::string* path )
{
    Status sRet;
    WCHAR TempPath[MAX_PATH];
    ::GetTempPathW(MAX_PATH,TempPath);
	ToNarrowPath(TempPath, *path);
    path->append("leveldb\\test\\");
    ModifyPath(*path);
    return sRet;
}

uint64_t Win32Env::NowMicros()
{
#ifndef USE_VISTA_API
#define GetTickCount64 GetTickCount
#endif
    return (uint64_t)(GetTickCount64()*1000);
}

static Status CreateDirInner( const std::string& dirname )
{
    Status sRet;
#ifdef _MSC_VER
    DWORD attr = ::GetFileAttributesA(dirname.c_str());
#else
    DWORD attr = ::GetFileAttributes(dirname.c_str());
#endif

    if (attr == INVALID_FILE_ATTRIBUTES) { // doesn't exist:
      std::size_t slash = dirname.find_last_of("\\");
      if (slash != std::string::npos){
	sRet = CreateDirInner(dirname.substr(0, slash));
	if (!sRet.ok()) return sRet;
      }
#ifdef _MSC_VER
      BOOL result = ::CreateDirectoryA(dirname.c_str(), NULL);
#else
      BOOL result = ::CreateDirectory(dirname.c_str(), NULL);
#endif
      if (result == FALSE) {
	sRet = Status::IOError(dirname, "Could not create directory.");
	return sRet;
      }
    }
    return sRet;
}

Status Win32Env::CreateDir( const std::string& dirname )
{
    std::string path = dirname;
    if(path[path.length() - 1] != '\\'){
        path += '\\';
    }
    ModifyPath(path);

    return CreateDirInner(path);
}

Status Win32Env::DeleteDir( const std::string& dirname )
{
    Status sRet;
    std::wstring path;
	ToWidePath(dirname, path);
    ModifyPath(path);
    if(!::RemoveDirectoryW( path.c_str() ) ){
        sRet = Status::IOError(dirname, "Could not delete directory.");
    }
    return sRet;
}

Status Win32Env::NewSequentialFile( const std::string& fname, SequentialFile** result )
{
    Status sRet;
    std::string path = fname;
    ModifyPath(path);
    Win32SequentialFile* pFile = new Win32SequentialFile(path);
    if(pFile->isEnable()){
        *result = pFile;
    }else {
        delete pFile;
        sRet = Status::IOError(path, Win32::GetLastErrSz());
    }
    return sRet;
}

Status Win32Env::NewRandomAccessFile( const std::string& fname, RandomAccessFile** result )
{
    Status sRet;
    std::string path = fname;
    Win32RandomAccessFile* pFile = new Win32RandomAccessFile(ModifyPath(path));
    if(!pFile->isEnable()){
        delete pFile;
        *result = NULL;
        sRet = Status::IOError(path, Win32::GetLastErrSz());
    }else
        *result = pFile;
    return sRet;
}

Status Win32Env::NewLogger( const std::string& fname, Logger** result )
{
    Status sRet;
    std::string path = fname;
    Win32MapFile* pMapFile = new Win32MapFile(ModifyPath(path));
    if(!pMapFile->isEnable()){
        delete pMapFile;
        *result = NULL;
        sRet = Status::IOError(path,"could not create a logger.");
    }else
        *result = new Win32Logger(pMapFile);
    return sRet;
}

Status Win32Env::NewWritableFile( const std::string& fname, WritableFile** result )
{
    Status sRet;
    std::string path = fname;
    Win32MapFile* pFile = new Win32MapFile(ModifyPath(path));
    if(!pFile->isEnable()){
        *result = NULL;
        sRet = Status::IOError(fname,Win32::GetLastErrSz());
    }else
        *result = pFile;
    return sRet;
}

Win32Env::Win32Env()
{

}

Win32Env::~Win32Env()
{

}


}  // Win32 namespace

static port::OnceType once = LEVELDB_ONCE_INIT;
static Env* default_env;
static void InitDefaultEnv() { default_env = new Win32::Win32Env(); }

Env* Env::Default() {
  port::InitOnce(&once, InitDefaultEnv);
  return default_env;
}

}  // namespace leveldb

#endif // defined(LEVELDB_PLATFORM_WINDOWS)

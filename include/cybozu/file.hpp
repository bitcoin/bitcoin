#pragma once
/**
	@file
	@brief file class and operations

	@author MITSUNARI Shigeo(@herumi)
	@remark mingw requires -lshlwapi option
*/

#include <assert.h>
#include <sys/stat.h> // for stat
#include <cybozu/exception.hpp>
#include <vector>
#include <ios>
#ifdef _WIN32
	#include <shlwapi.h>
	#include <io.h>
	#include <fcntl.h>
	#include <shlobj.h>
	#include <direct.h>
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>
	#ifdef _MSC_VER
		#pragma comment(lib, "shlwapi.lib")
		#pragma comment(lib, "shell32.lib")
		#pragma comment(lib, "User32.lib")
	#endif
#else
	#include <stdio.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <fcntl.h>
	#include <dirent.h>
#endif
#ifdef __APPLE__
	#include <mach-o/dyld.h>
#endif

namespace cybozu {

class File {
	std::string name_; // used for only errmsg
#ifdef _WIN32
	typedef HANDLE handleType;
#else
	typedef int handleType;
	enum {
		INVALID_HANDLE_VALUE = -1
	};
#endif
	handleType hdl_;
	bool doClose_;
	bool isReadOnly_;
	File(const File&);
	void operator=(const File&);
#ifdef _WIN32
	void openSub()
	{
	}
#endif
	void verifyMode(std::ios::openmode mode)
	{
		doClose_ = true;
		bool isCorrectMode = true;
		if (!!(mode & std::ios::in) == !!(mode & std::ios::out)) {
			isCorrectMode = false;
		} else {
			if (mode & std::ios::in) {
				isReadOnly_ = true;
				if ((mode & std::ios::app) || (mode & std::ios::trunc)) isCorrectMode = false;
			} else {
				isReadOnly_ = false;
				if ((mode & std::ios::app) && (mode & std::ios::trunc)) isCorrectMode = false;
			}
		}
		if (!isCorrectMode) {
			throw cybozu::Exception("File:open:bad mode") << name_ << mode;
		}
	}
#ifdef _WIN32
	HANDLE createFile(const std::string& name, DWORD access, DWORD share, DWORD disposition)
	{
		return  ::CreateFileA(name.c_str(), access, share, NULL, disposition, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	HANDLE createFile(const std::wstring& name, DWORD access, DWORD share, DWORD disposition)
	{
		return  ::CreateFileW(name.c_str(), access, share, NULL, disposition, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	template<class T>
	void setHandle(const T& name, std::ios::openmode mode)
	{
		DWORD access = GENERIC_READ;
		DWORD disposition = OPEN_EXISTING;
		DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE;
		if (mode & std::ios::out) {
			access = GENERIC_WRITE;
			disposition = CREATE_ALWAYS;
			if (mode & std::ios::app) {
				disposition = OPEN_ALWAYS;
			}
		}
		hdl_ = createFile(name, access, share, disposition);
	}
#else
	void setHandle(const std::string& name, std::ios::openmode mode)
	{
		int flags = O_RDONLY; // | O_NOATIME; /* can't use on NFS */
		mode_t access = 0644;
		if (mode & std::ios::out) {
			flags = O_WRONLY | O_CREAT;
			if (mode & std::ios::app) {
				flags |= O_APPEND;
			} else {
				flags |= O_TRUNC;
			}
		}
		hdl_ = ::open(name.c_str(), flags, access);
	}
#endif
	template<class T>
	void openSub(const T& name, std::ios::openmode mode)
	{
		if (isOpen()) throw cybozu::Exception("File:open:alread opened") << name_;
		verifyMode(mode);
		setHandle(name, mode);
		if (isOpen()) {
			if (mode & std::ios::app) {
				seek(getSize(), std::ios::beg);
			}
			return;
		}
		throw cybozu::Exception("File:open") << name_ << cybozu::ErrorNo() << static_cast<int>(mode);
	}
public:
	File()
		: hdl_(INVALID_HANDLE_VALUE)
		, doClose_(true)
		, isReadOnly_(false)
	{
	}
	File(const std::string& name, std::ios::openmode mode)
		: hdl_(INVALID_HANDLE_VALUE)
		, doClose_(true)
		, isReadOnly_(false)
	{
		open(name, mode);
	}
	/*
		construct with file handle
		@param hdl [in] file handle
	*/
	explicit File(handleType hdl)
		: hdl_(hdl)
		, doClose_(false)
		, isReadOnly_(false)

	{
	}
	~File() CYBOZU_NOEXCEPT
	{
		if (!doClose_) return;
		try {
			sync();
			close();
		} catch (std::exception& e) {
			fprintf(stderr, "File:dstr:%s\n", e.what());
		} catch (...) {
			fprintf(stderr, "File:dstr:unknown\n");
		}
	}
	bool isOpen() const CYBOZU_NOEXCEPT { return hdl_ != INVALID_HANDLE_VALUE; }
	/**
		support mode
		always binary mode
		ios::in : read only
		ios::out : write only(+truncate)
		ios::out + ios::app(append)
	*/
	void open(const std::string& name, std::ios::openmode mode)
	{
		name_ = name;
		openSub(name, mode);
	}
	void openW(const std::string& name)
	{
		open(name, std::ios::out | std::ios::binary | std::ios::trunc);
	}
	void openR(const std::string& name)
	{
		open(name, std::ios::in | std::ios::binary);
	}
#ifdef _WIN32
	void open(const std::wstring& name, std::ios::openmode mode)
	{
		name_ = cybozu::exception::wstr2str(name);
		openSub(name, mode);
	}
	File(const std::wstring& name, std::ios::openmode mode)
		: hdl_(INVALID_HANDLE_VALUE)
	{
		open(name, mode);
	}
	void openW(const std::wstring& name)
	{
		open(name, std::ios::out | std::ios::binary | std::ios::trunc);
	}
	void openR(const std::wstring& name)
	{
		open(name, std::ios::in | std::ios::binary);
	}
#endif
	void close()
	{
		if (!isOpen()) return;
#ifdef _WIN32
		bool isOK = ::CloseHandle(hdl_) != 0;
#else
		bool isOK = ::close(hdl_) == 0;
#endif
		hdl_ = INVALID_HANDLE_VALUE;
		if (isOK) return;
		throw cybozu::Exception("File:close") << name_ << cybozu::ErrorNo();
	}
	/*
		sync
		@param doFullSync [in] call sync(for only Linux)
	*/
	void sync(bool doFullSync = false)
	{
		cybozu::disable_warning_unused_variable(doFullSync);
		if (!isOpen()) return;
		if (isReadOnly_) return;
#ifdef _WIN32
		/* fail if isReadOnly_ */
		if (!::FlushFileBuffers(hdl_)) goto ERR_EXIT;
#elif defined(__linux__) || defined(__CYGWIN__)
		if (doFullSync) {
			if (::fsync(hdl_)) goto ERR_EXIT;
		} else {
			if (::fdatasync(hdl_)) goto ERR_EXIT;
		}
#else
		if (::fcntl(hdl_, F_FULLFSYNC)) goto ERR_EXIT;
#endif
		return;
	ERR_EXIT:
		throw cybozu::Exception("File:sync") << name_ << cybozu::ErrorNo();
	}
	void write(const void *buf, size_t bufSize)
	{
		const char *p = static_cast<const char *>(buf);
		while (bufSize > 0) {
			uint32_t size = static_cast<uint32_t>(std::min<size_t>(0x7fffffff, bufSize));
#ifdef _WIN32
			DWORD writeSize;
			if (!::WriteFile(hdl_, p, size, &writeSize, NULL)) goto ERR_EXIT;
#else
			ssize_t writeSize = ::write(hdl_, p, size);
			if (writeSize < 0) {
				if (errno == EINTR) continue;
				goto ERR_EXIT;
			}
#endif
			p += writeSize;
			bufSize -= writeSize;
		}
		return;
	ERR_EXIT:
		throw cybozu::Exception("File:write") << name_ << cybozu::ErrorNo();
	}
	size_t readSome(void *buf, size_t bufSize)
	{
		uint32_t size = static_cast<uint32_t>(std::min<size_t>(0x7fffffff, bufSize));
#ifdef _WIN32
		DWORD readSize;
		if (!::ReadFile(hdl_, buf, size, &readSize, NULL)) goto ERR_EXIT;
#else
	RETRY:
		ssize_t readSize = ::read(hdl_, buf, size);
		if (readSize < 0) {
			if (errno == EINTR) goto RETRY;
			goto ERR_EXIT;
		}
#endif
		return readSize;
	ERR_EXIT:
		throw cybozu::Exception("File:read") << name_ << cybozu::ErrorNo();
	}
	void read(void *buf, size_t bufSize)
	{
		char *p = static_cast<char *>(buf);
		while (bufSize > 0) {
			size_t readSize = readSome(p, bufSize);
			p += readSize;
			bufSize -= readSize;
		}
	}
	void seek(int64_t pos, std::ios::seekdir dir)
	{
#ifdef _WIN32
		LARGE_INTEGER largePos;
		largePos.QuadPart = pos;
		DWORD posMode = FILE_BEGIN;
		switch (dir) {
		case std::ios::beg:
			posMode = FILE_BEGIN;
			break;
		case std::ios::cur:
		default:
			posMode = FILE_CURRENT;
			break;
		case std::ios::end:
			posMode = FILE_END;
			break;
		}
		bool isOK = SetFilePointerEx(hdl_, largePos, NULL, posMode) != 0;
#else
		int whence;
		switch (dir) {
		case std::ios::beg:
			whence = SEEK_SET;
			break;
		case std::ios::cur:
		default:
			whence = SEEK_CUR;
			break;
		case std::ios::end:
			whence = SEEK_END;
			break;
		}
		bool isOK = lseek(hdl_, pos, whence) >= 0;
#endif
		if (isOK) return;
		throw cybozu::Exception("File:seek") << name_ << cybozu::ErrorNo() << pos << static_cast<int>(dir);
	}
	uint64_t getSize() const
	{
		uint64_t fileSize;
#ifdef _WIN32
		LARGE_INTEGER size;
		bool isOK = GetFileSizeEx(hdl_, &size) != 0;
		fileSize = size.QuadPart;
#else
		struct stat stat;
		bool isOK = fstat(hdl_, &stat) == 0;
		fileSize = stat.st_size;
#endif
		if (isOK) return fileSize;
		throw cybozu::Exception("File:getSize") << name_ << cybozu::ErrorNo();
	}
};

/*
	name has extension
*/
inline bool HasExtension(const std::string& name, const std::string& extension)
{
	const size_t extensionSize = extension.size();
	if (extensionSize == 0) return true;
	const size_t nameSize = name.size();
	if (nameSize < extensionSize + 1) return false;
	const char *p = &name[nameSize - extensionSize - 1];
	return *p == '.' && memcmp(p + 1, &extension[0], extensionSize) == 0;
}
/*
	split name as basename.suffix
*/
inline std::string GetBaseName(const std::string& name, std::string *suffix = 0)
{
	size_t pos = name.find_last_of('.');
	if (pos == std::string::npos) {
		if (suffix) suffix->clear();
		return name;
	}
	if (suffix) {
		*suffix = name.substr(pos + 1);
	}
	return name.substr(0, pos);
}

/**
	replace \ with /
*/
inline void ReplaceBackSlash(std::string& str)
{
	for (size_t i = 0, n = str.size(); i < n; i++) {
		if (str[i] == '\\') str[i] = '/';
	}
}

/**
	get exe path and baseNamme
	@note file name is the form "xxx.exe" then baseName = xxx
*/
inline std::string GetExePath(std::string *baseName = 0)
{
	std::string path;
	path.resize(4096);
#ifdef _WIN32
	if (!GetModuleFileNameA(NULL, &path[0], static_cast<int>(path.size()) - 2)) {
		return "";
	}
	PathRemoveExtensionA(&path[0]);
	if (baseName) {
		*baseName = PathFindFileNameA(&path[0]);
	}
	if (::PathRemoveFileSpecA(&path[0])) {
		::PathAddBackslashA(&path[0]);
		path[0] = static_cast<char>(tolower(path[0]));
		path.resize(strlen(&path[0]));
		ReplaceBackSlash(path);
	}
#else
#if defined(__APPLE__)
	uint32_t size = (uint32_t)path.size();
	if (_NSGetExecutablePath(&path[0], &size) != 0) {
		return "";
	}
	path.resize(strlen(&path[0]));
#else
	int ret = readlink("/proc/self/exe", &path[0], path.size() - 2);
	if (ret < 0) return "";
	path.resize(ret);
#endif
	size_t pos = path.find_last_of('/');
	if (pos != std::string::npos) {
		if (baseName) {
			const std::string name = path.substr(pos + 1);
			std::string suffix;
			std::string base = GetBaseName(name, &suffix);
			if (suffix == "exe") {
				*baseName = base;
			} else {
				*baseName = name;
			}
		}
		path.resize(pos + 1);
	}
#endif
	return path;
}

/**
	get file size
*/
inline uint64_t GetFileSize(const std::string& name)
{
#ifdef _WIN32
	struct __stat64 buf;
	bool isOK = _stat64(name.c_str(), &buf) == 0;
#else
	struct stat buf;
	bool isOK = stat(name.c_str(), &buf) == 0;
#endif
	if (isOK) return buf.st_size;
	throw cybozu::Exception("GetFileSize") << name << cybozu::ErrorNo();
}

/**
	verify whether path exists or not
*/
inline bool DoesFileExist(const std::string& path)
{
	if (path.empty()) return false;
	std::string p = path;
	char c = p[p.size() - 1];
	if (c == '/' || c == '\\') {
		p.resize(p.size() - 1);
	}
#ifdef _WIN32
	struct _stat buf;
	return _stat(p.c_str(), &buf) == 0;
#else
	struct stat buf;
	return stat(p.c_str(), &buf) == 0;
#endif
}

inline void RenameFile(const std::string& from, const std::string& to)
{
	if (DoesFileExist(to)) {
		throw cybozu::Exception("RenameFile:file already exist") << from << to;
	}
#ifdef _WIN32
	bool isOK = ::MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH) != 0;
#else
	bool isOK = ::rename(from.c_str(), to.c_str()) == 0;
#endif
	if (!isOK) {
		throw cybozu::Exception("RenameFile") << from << to << cybozu::ErrorNo();
	}
}

/**
	remove file
*/
inline void RemoveFile(const std::string& name)
{
#ifdef _WIN32
	bool isOK = DeleteFileA(name.c_str()) != 0;
#else
	bool isOK = unlink(name.c_str()) == 0;
#endif
	if (!isOK) {
		throw cybozu::Exception("RemoveFile") << name << cybozu::ErrorNo();
	}
}

/*
	remark of isFile()
	not directory on Windows
	not contain symbolic link on Linux
*/
struct FileInfo {
	std::string name;
	uint32_t attr; // dwFileAttributes for Windows, d_type for Linux
#ifdef _WIN32
	bool isUnknown() const { return attr == 0; }
	bool isDirectory() const { verify(); return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0; }
	bool isFile() const { verify(); return !isDirectory(); }
#else
	bool isUnknown() const { return attr == DT_UNKNOWN; }
	bool isDirectory() const { verify(); return attr == DT_DIR; }
	bool isFile() const { verify(); return attr == DT_REG; }
#endif
	FileInfo() : attr(0) {}
	FileInfo(const std::string& name, uint32_t attr) : name(name), attr(attr) {}
	void verify() const
	{
		if (isUnknown()) throw cybozu::Exception("FileInfo:unknown attr") << name;
	}
};

typedef std::vector<FileInfo> FileList;


namespace file_local {

inline void filterAndPush(FileList& list, const FileInfo& fi, const std::string& extension, bool cond(const std::string&, const std::string&))
{
	if (fi.name == "." || fi.name == "..") {
		return;
	}
	if (cond(fi.name, extension)) {
		list.push_back(fi);
	}
}

} // cybozu::file_local

/**
	get file name in dir
	@param list [out] FileList
	@param dir [in] directory
	@param extension [in] select files(including directory) having extension such as "cpp" ; select all if suffix is empty
	@param cond [in] filter function (select if cond(targetFile, suffix) is true)
	@note "." and ".." are excluded
*/
inline bool GetFileList(FileList &list, const std::string& dir, const std::string& extension = "", bool (*cond)(const std::string&, const std::string&) = cybozu::HasExtension)
{
#ifdef _WIN32
	std::string path = dir + "/*";
	WIN32_FIND_DATAA fd;
	struct Handle {
		Handle(HANDLE hdl)
			: hdl_(hdl)
		{
		}
		~Handle()
		{
			if (hdl_ != INVALID_HANDLE_VALUE) {
				FindClose(hdl_);
			}
		}
		HANDLE hdl_;
	};
	Handle hdl(FindFirstFileA(path.c_str(), &fd));
	if (hdl.hdl_ == INVALID_HANDLE_VALUE) {
		return false;
	}
	do {
		FileInfo fi(fd.cFileName, fd.dwFileAttributes);
		file_local::filterAndPush(list, fi, extension, cond);
	} while (FindNextFileA(hdl.hdl_, &fd) != 0);
	return true;
#else
	struct Handle {
		DIR *dir_;
		Handle(DIR *dir)
			: dir_(dir)
		{
			if (dir_ == 0) {
				perror("opendir");
			}
		}
		~Handle()
		{
			if (dir_) {
				if (::closedir(dir_)) {
					perror("closedir");
				}
			}
		}
		bool isValid() const { return dir_ != 0; }
	};
	Handle hdl(::opendir(dir.c_str()));
	if (!hdl.isValid()) return false;
	for (;;) {
		struct dirent *dp = ::readdir(hdl.dir_);
		if (dp == 0) return true;
		FileInfo fi(dp->d_name, (uint8_t)dp->d_type);
		file_local::filterAndPush(list, fi, extension, cond);
	}
#endif
}

inline FileList GetFileList(const std::string& dir, const std::string& extension = "", bool (*cond)(const std::string&, const std::string&) = cybozu::HasExtension)
{
	FileList fl;
	if (GetFileList(fl, dir, extension, cond)) return fl;
	throw cybozu::Exception("cybozu:GetFileList") << dir << cybozu::ErrorNo();
}

} // cybozu

// Copyright (c) 2018 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

// Prevent Windows headers from defining min/max macros and instead
// use STL.
#ifndef NOMINMAX
#define NOMINMAX
#endif  // ifndef NOMINMAX
#include <windows.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

#include "leveldb/env.h"
#include "leveldb/slice.h"
#include "port/port.h"
#include "port/thread_annotations.h"
#include "util/env_windows_test_helper.h"
#include "util/logging.h"
#include "util/mutexlock.h"
#include "util/windows_logger.h"

#if defined(DeleteFile)
#undef DeleteFile
#endif  // defined(DeleteFile)

namespace leveldb {

namespace {

constexpr const size_t kWritableFileBufferSize = 65536;

// Up to 1000 mmaps for 64-bit binaries; none for 32-bit.
constexpr int kDefaultMmapLimit = (sizeof(void*) >= 8) ? 1000 : 0;

// Can be set by by EnvWindowsTestHelper::SetReadOnlyMMapLimit().
int g_mmap_limit = kDefaultMmapLimit;

std::string GetWindowsErrorMessage(DWORD error_code) {
  std::string message;
  char* error_text = nullptr;
  // Use MBCS version of FormatMessage to match return value.
  size_t error_text_size = ::FormatMessageA(
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<char*>(&error_text), 0, nullptr);
  if (!error_text) {
    return message;
  }
  message.assign(error_text, error_text_size);
  ::LocalFree(error_text);
  return message;
}

Status WindowsError(const std::string& context, DWORD error_code) {
  if (error_code == ERROR_FILE_NOT_FOUND || error_code == ERROR_PATH_NOT_FOUND)
    return Status::NotFound(context, GetWindowsErrorMessage(error_code));
  return Status::IOError(context, GetWindowsErrorMessage(error_code));
}

class ScopedHandle {
 public:
  ScopedHandle(HANDLE handle) : handle_(handle) {}
  ScopedHandle(const ScopedHandle&) = delete;
  ScopedHandle(ScopedHandle&& other) noexcept : handle_(other.Release()) {}
  ~ScopedHandle() { Close(); }

  ScopedHandle& operator=(const ScopedHandle&) = delete;

  ScopedHandle& operator=(ScopedHandle&& rhs) noexcept {
    if (this != &rhs) handle_ = rhs.Release();
    return *this;
  }

  bool Close() {
    if (!is_valid()) {
      return true;
    }
    HANDLE h = handle_;
    handle_ = INVALID_HANDLE_VALUE;
    return ::CloseHandle(h);
  }

  bool is_valid() const {
    return handle_ != INVALID_HANDLE_VALUE && handle_ != nullptr;
  }

  HANDLE get() const { return handle_; }

  HANDLE Release() {
    HANDLE h = handle_;
    handle_ = INVALID_HANDLE_VALUE;
    return h;
  }

 private:
  HANDLE handle_;
};

// Helper class to limit resource usage to avoid exhaustion.
// Currently used to limit read-only file descriptors and mmap file usage
// so that we do not run out of file descriptors or virtual memory, or run into
// kernel performance problems for very large databases.
class Limiter {
 public:
  // Limit maximum number of resources to |max_acquires|.
  Limiter(int max_acquires) : acquires_allowed_(max_acquires) {}

  Limiter(const Limiter&) = delete;
  Limiter operator=(const Limiter&) = delete;

  // If another resource is available, acquire it and return true.
  // Else return false.
  bool Acquire() {
    int old_acquires_allowed =
        acquires_allowed_.fetch_sub(1, std::memory_order_relaxed);

    if (old_acquires_allowed > 0) return true;

    acquires_allowed_.fetch_add(1, std::memory_order_relaxed);
    return false;
  }

  // Release a resource acquired by a previous call to Acquire() that returned
  // true.
  void Release() { acquires_allowed_.fetch_add(1, std::memory_order_relaxed); }

 private:
  // The number of available resources.
  //
  // This is a counter and is not tied to the invariants of any other class, so
  // it can be operated on safely using std::memory_order_relaxed.
  std::atomic<int> acquires_allowed_;
};

class WindowsSequentialFile : public SequentialFile {
 public:
  WindowsSequentialFile(std::string filename, ScopedHandle handle)
      : handle_(std::move(handle)), filename_(std::move(filename)) {}
  ~WindowsSequentialFile() override {}

  Status Read(size_t n, Slice* result, char* scratch) override {
    DWORD bytes_read;
    // DWORD is 32-bit, but size_t could technically be larger. However leveldb
    // files are limited to leveldb::Options::max_file_size which is clamped to
    // 1<<30 or 1 GiB.
    assert(n <= std::numeric_limits<DWORD>::max());
    if (!::ReadFile(handle_.get(), scratch, static_cast<DWORD>(n), &bytes_read,
                    nullptr)) {
      return WindowsError(filename_, ::GetLastError());
    }

    *result = Slice(scratch, bytes_read);
    return Status::OK();
  }

  Status Skip(uint64_t n) override {
    LARGE_INTEGER distance;
    distance.QuadPart = n;
    if (!::SetFilePointerEx(handle_.get(), distance, nullptr, FILE_CURRENT)) {
      return WindowsError(filename_, ::GetLastError());
    }
    return Status::OK();
  }

  std::string GetName() const override { return filename_; }

 private:
  const ScopedHandle handle_;
  const std::string filename_;
};

class WindowsRandomAccessFile : public RandomAccessFile {
 public:
  WindowsRandomAccessFile(std::string filename, ScopedHandle handle)
      : handle_(std::move(handle)), filename_(std::move(filename)) {}

  ~WindowsRandomAccessFile() override = default;

  Status Read(uint64_t offset, size_t n, Slice* result,
              char* scratch) const override {
    DWORD bytes_read = 0;
    OVERLAPPED overlapped = {};

    overlapped.OffsetHigh = static_cast<DWORD>(offset >> 32);
    overlapped.Offset = static_cast<DWORD>(offset);
    if (!::ReadFile(handle_.get(), scratch, static_cast<DWORD>(n), &bytes_read,
                    &overlapped)) {
      DWORD error_code = ::GetLastError();
      if (error_code != ERROR_HANDLE_EOF) {
        *result = Slice(scratch, 0);
        return Status::IOError(filename_, GetWindowsErrorMessage(error_code));
      }
    }

    *result = Slice(scratch, bytes_read);
    return Status::OK();
  }

  std::string GetName() const override { return filename_; }

 private:
  const ScopedHandle handle_;
  const std::string filename_;
};

class WindowsMmapReadableFile : public RandomAccessFile {
 public:
  // base[0,length-1] contains the mmapped contents of the file.
  WindowsMmapReadableFile(std::string filename, char* mmap_base, size_t length,
                          Limiter* mmap_limiter)
      : mmap_base_(mmap_base),
        length_(length),
        mmap_limiter_(mmap_limiter),
        filename_(std::move(filename)) {}

  ~WindowsMmapReadableFile() override {
    ::UnmapViewOfFile(mmap_base_);
    mmap_limiter_->Release();
  }

  Status Read(uint64_t offset, size_t n, Slice* result,
              char* scratch) const override {
    if (offset + n > length_) {
      *result = Slice();
      return WindowsError(filename_, ERROR_INVALID_PARAMETER);
    }

    *result = Slice(mmap_base_ + offset, n);
    return Status::OK();
  }

  std::string GetName() const override { return filename_; }

 private:
  char* const mmap_base_;
  const size_t length_;
  Limiter* const mmap_limiter_;
  const std::string filename_;
};

class WindowsWritableFile : public WritableFile {
 public:
  WindowsWritableFile(std::string filename, ScopedHandle handle)
      : pos_(0), handle_(std::move(handle)), filename_(std::move(filename)) {}

  ~WindowsWritableFile() override = default;

  Status Append(const Slice& data) override {
    size_t write_size = data.size();
    const char* write_data = data.data();

    // Fit as much as possible into buffer.
    size_t copy_size = std::min(write_size, kWritableFileBufferSize - pos_);
    std::memcpy(buf_ + pos_, write_data, copy_size);
    write_data += copy_size;
    write_size -= copy_size;
    pos_ += copy_size;
    if (write_size == 0) {
      return Status::OK();
    }

    // Can't fit in buffer, so need to do at least one write.
    Status status = FlushBuffer();
    if (!status.ok()) {
      return status;
    }

    // Small writes go to buffer, large writes are written directly.
    if (write_size < kWritableFileBufferSize) {
      std::memcpy(buf_, write_data, write_size);
      pos_ = write_size;
      return Status::OK();
    }
    return WriteUnbuffered(write_data, write_size);
  }

  Status Close() override {
    Status status = FlushBuffer();
    if (!handle_.Close() && status.ok()) {
      status = WindowsError(filename_, ::GetLastError());
    }
    return status;
  }

  Status Flush() override { return FlushBuffer(); }

  Status Sync() override {
    // On Windows no need to sync parent directory. Its metadata will be updated
    // via the creation of the new file, without an explicit sync.

    Status status = FlushBuffer();
    if (!status.ok()) {
      return status;
    }

    if (!::FlushFileBuffers(handle_.get())) {
      return Status::IOError(filename_,
                             GetWindowsErrorMessage(::GetLastError()));
    }
    return Status::OK();
  }

  std::string GetName() const override { return filename_; }

 private:
  Status FlushBuffer() {
    Status status = WriteUnbuffered(buf_, pos_);
    pos_ = 0;
    return status;
  }

  Status WriteUnbuffered(const char* data, size_t size) {
    DWORD bytes_written;
    if (!::WriteFile(handle_.get(), data, static_cast<DWORD>(size),
                     &bytes_written, nullptr)) {
      return Status::IOError(filename_,
                             GetWindowsErrorMessage(::GetLastError()));
    }
    return Status::OK();
  }

  // buf_[0, pos_-1] contains data to be written to handle_.
  char buf_[kWritableFileBufferSize];
  size_t pos_;

  ScopedHandle handle_;
  const std::string filename_;
};

// Lock or unlock the entire file as specified by |lock|. Returns true
// when successful, false upon failure. Caller should call ::GetLastError()
// to determine cause of failure
bool LockOrUnlock(HANDLE handle, bool lock) {
  if (lock) {
    return ::LockFile(handle,
                      /*dwFileOffsetLow=*/0, /*dwFileOffsetHigh=*/0,
                      /*nNumberOfBytesToLockLow=*/MAXDWORD,
                      /*nNumberOfBytesToLockHigh=*/MAXDWORD);
  } else {
    return ::UnlockFile(handle,
                        /*dwFileOffsetLow=*/0, /*dwFileOffsetHigh=*/0,
                        /*nNumberOfBytesToLockLow=*/MAXDWORD,
                        /*nNumberOfBytesToLockHigh=*/MAXDWORD);
  }
}

class WindowsFileLock : public FileLock {
 public:
  WindowsFileLock(ScopedHandle handle, std::string filename)
      : handle_(std::move(handle)), filename_(std::move(filename)) {}

  const ScopedHandle& handle() const { return handle_; }
  const std::string& filename() const { return filename_; }

 private:
  const ScopedHandle handle_;
  const std::string filename_;
};

class WindowsEnv : public Env {
 public:
  WindowsEnv();
  ~WindowsEnv() override {
    static const char msg[] =
        "WindowsEnv singleton destroyed. Unsupported behavior!\n";
    std::fwrite(msg, 1, sizeof(msg), stderr);
    std::abort();
  }

  Status NewSequentialFile(const std::string& filename,
                           SequentialFile** result) override {
    *result = nullptr;
    DWORD desired_access = GENERIC_READ;
    DWORD share_mode = FILE_SHARE_READ;
    auto wFilename = toUtf16(filename);
    ScopedHandle handle = ::CreateFileW(
        wFilename.c_str(), desired_access, share_mode,
        /*lpSecurityAttributes=*/nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
        /*hTemplateFile=*/nullptr);
    if (!handle.is_valid()) {
      return WindowsError(filename, ::GetLastError());
    }

    *result = new WindowsSequentialFile(filename, std::move(handle));
    return Status::OK();
  }

  Status NewRandomAccessFile(const std::string& filename,
                             RandomAccessFile** result) override {
    *result = nullptr;
    DWORD desired_access = GENERIC_READ;
    DWORD share_mode = FILE_SHARE_READ;
    auto wFilename = toUtf16(filename);
    ScopedHandle handle =
        ::CreateFileW(wFilename.c_str(), desired_access, share_mode,
                      /*lpSecurityAttributes=*/nullptr, OPEN_EXISTING,
                      FILE_ATTRIBUTE_READONLY,
                      /*hTemplateFile=*/nullptr);
    if (!handle.is_valid()) {
      return WindowsError(filename, ::GetLastError());
    }
    if (!mmap_limiter_.Acquire()) {
      *result = new WindowsRandomAccessFile(filename, std::move(handle));
      return Status::OK();
    }

    LARGE_INTEGER file_size;
    Status status;
    if (!::GetFileSizeEx(handle.get(), &file_size)) {
      mmap_limiter_.Release();
      return WindowsError(filename, ::GetLastError());
    }

    ScopedHandle mapping =
        ::CreateFileMappingW(handle.get(),
                             /*security attributes=*/nullptr, PAGE_READONLY,
                             /*dwMaximumSizeHigh=*/0,
                             /*dwMaximumSizeLow=*/0,
                             /*lpName=*/nullptr);
    if (mapping.is_valid()) {
      void* mmap_base = ::MapViewOfFile(mapping.get(), FILE_MAP_READ,
                                        /*dwFileOffsetHigh=*/0,
                                        /*dwFileOffsetLow=*/0,
                                        /*dwNumberOfBytesToMap=*/0);
      if (mmap_base) {
        *result = new WindowsMmapReadableFile(
            filename, reinterpret_cast<char*>(mmap_base),
            static_cast<size_t>(file_size.QuadPart), &mmap_limiter_);
        return Status::OK();
      }
    }
    mmap_limiter_.Release();
    return WindowsError(filename, ::GetLastError());
  }

  Status NewWritableFile(const std::string& filename,
                         WritableFile** result) override {
    DWORD desired_access = GENERIC_WRITE;
    DWORD share_mode = 0;  // Exclusive access.
    auto wFilename = toUtf16(filename);
    ScopedHandle handle = ::CreateFileW(
        wFilename.c_str(), desired_access, share_mode,
        /*lpSecurityAttributes=*/nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
        /*hTemplateFile=*/nullptr);
    if (!handle.is_valid()) {
      *result = nullptr;
      return WindowsError(filename, ::GetLastError());
    }

    *result = new WindowsWritableFile(filename, std::move(handle));
    return Status::OK();
  }

  Status NewAppendableFile(const std::string& filename,
                           WritableFile** result) override {
    DWORD desired_access = FILE_APPEND_DATA;
    DWORD share_mode = 0;  // Exclusive access.
    auto wFilename = toUtf16(filename);
    ScopedHandle handle = ::CreateFileW(
        wFilename.c_str(), desired_access, share_mode,
        /*lpSecurityAttributes=*/nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
        /*hTemplateFile=*/nullptr);
    if (!handle.is_valid()) {
      *result = nullptr;
      return WindowsError(filename, ::GetLastError());
    }

    *result = new WindowsWritableFile(filename, std::move(handle));
    return Status::OK();
  }

  bool FileExists(const std::string& filename) override {
    auto wFilename = toUtf16(filename);
    return GetFileAttributesW(wFilename.c_str()) != INVALID_FILE_ATTRIBUTES;
  }

  Status GetChildren(const std::string& directory_path,
                     std::vector<std::string>* result) override {
    const std::string find_pattern = directory_path + "\\*";
    WIN32_FIND_DATAW find_data;
    auto wFind_pattern = toUtf16(find_pattern);
    HANDLE dir_handle = ::FindFirstFileW(wFind_pattern.c_str(), &find_data);
    if (dir_handle == INVALID_HANDLE_VALUE) {
      DWORD last_error = ::GetLastError();
      if (last_error == ERROR_FILE_NOT_FOUND) {
        return Status::OK();
      }
      return WindowsError(directory_path, last_error);
    }
    do {
      char base_name[_MAX_FNAME];
      char ext[_MAX_EXT];

      auto find_data_filename = toUtf8(find_data.cFileName);
      if (!_splitpath_s(find_data_filename.c_str(), nullptr, 0, nullptr, 0,
                        base_name, ARRAYSIZE(base_name), ext, ARRAYSIZE(ext))) {
        result->emplace_back(std::string(base_name) + ext);
      }
    } while (::FindNextFileW(dir_handle, &find_data));
    DWORD last_error = ::GetLastError();
    ::FindClose(dir_handle);
    if (last_error != ERROR_NO_MORE_FILES) {
      return WindowsError(directory_path, last_error);
    }
    return Status::OK();
  }

  Status DeleteFile(const std::string& filename) override {
    auto wFilename = toUtf16(filename);
    if (!::DeleteFileW(wFilename.c_str())) {
      return WindowsError(filename, ::GetLastError());
    }
    return Status::OK();
  }

  Status CreateDir(const std::string& dirname) override {
    auto wDirname = toUtf16(dirname);
    if (!::CreateDirectoryW(wDirname.c_str(), nullptr)) {
      return WindowsError(dirname, ::GetLastError());
    }
    return Status::OK();
  }

  Status DeleteDir(const std::string& dirname) override {
    auto wDirname = toUtf16(dirname);
    if (!::RemoveDirectoryW(wDirname.c_str())) {
      return WindowsError(dirname, ::GetLastError());
    }
    return Status::OK();
  }

  Status GetFileSize(const std::string& filename, uint64_t* size) override {
    WIN32_FILE_ATTRIBUTE_DATA file_attributes;
    auto wFilename = toUtf16(filename);
    if (!::GetFileAttributesExW(wFilename.c_str(), GetFileExInfoStandard,
                                &file_attributes)) {
      return WindowsError(filename, ::GetLastError());
    }
    ULARGE_INTEGER file_size;
    file_size.HighPart = file_attributes.nFileSizeHigh;
    file_size.LowPart = file_attributes.nFileSizeLow;
    *size = file_size.QuadPart;
    return Status::OK();
  }

  Status RenameFile(const std::string& from, const std::string& to) override {
    // Try a simple move first. It will only succeed when |to| doesn't already
    // exist.
    auto wFrom = toUtf16(from);
    auto wTo = toUtf16(to);
    if (::MoveFileW(wFrom.c_str(), wTo.c_str())) {
      return Status::OK();
    }
    DWORD move_error = ::GetLastError();

    // Try the full-blown replace if the move fails, as ReplaceFile will only
    // succeed when |to| does exist. When writing to a network share, we may not
    // be able to change the ACLs. Ignore ACL errors then
    // (REPLACEFILE_IGNORE_MERGE_ERRORS).
    if (::ReplaceFileW(wTo.c_str(), wFrom.c_str(), /*lpBackupFileName=*/nullptr,
                       REPLACEFILE_IGNORE_MERGE_ERRORS,
                       /*lpExclude=*/nullptr, /*lpReserved=*/nullptr)) {
      return Status::OK();
    }
    DWORD replace_error = ::GetLastError();
    // In the case of FILE_ERROR_NOT_FOUND from ReplaceFile, it is likely that
    // |to| does not exist. In this case, the more relevant error comes from the
    // call to MoveFile.
    if (replace_error == ERROR_FILE_NOT_FOUND ||
        replace_error == ERROR_PATH_NOT_FOUND) {
      return WindowsError(from, move_error);
    } else {
      return WindowsError(from, replace_error);
    }
  }

  Status LockFile(const std::string& filename, FileLock** lock) override {
    *lock = nullptr;
    Status result;
    auto wFilename = toUtf16(filename);
    ScopedHandle handle = ::CreateFileW(
        wFilename.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
        /*lpSecurityAttributes=*/nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (!handle.is_valid()) {
      result = WindowsError(filename, ::GetLastError());
    } else if (!LockOrUnlock(handle.get(), true)) {
      result = WindowsError("lock " + filename, ::GetLastError());
    } else {
      *lock = new WindowsFileLock(std::move(handle), filename);
    }
    return result;
  }

  Status UnlockFile(FileLock* lock) override {
    WindowsFileLock* windows_file_lock =
        reinterpret_cast<WindowsFileLock*>(lock);
    if (!LockOrUnlock(windows_file_lock->handle().get(), false)) {
      return WindowsError("unlock " + windows_file_lock->filename(),
                          ::GetLastError());
    }
    delete windows_file_lock;
    return Status::OK();
  }

  void Schedule(void (*background_work_function)(void* background_work_arg),
                void* background_work_arg) override;

  void StartThread(void (*thread_main)(void* thread_main_arg),
                   void* thread_main_arg) override {
    std::thread new_thread(thread_main, thread_main_arg);
    new_thread.detach();
  }

  Status GetTestDirectory(std::string* result) override {
    const char* env = getenv("TEST_TMPDIR");
    if (env && env[0] != '\0') {
      *result = env;
      return Status::OK();
    }

    wchar_t wtmp_path[MAX_PATH];
    if (!GetTempPathW(ARRAYSIZE(wtmp_path), wtmp_path)) {
      return WindowsError("GetTempPath", ::GetLastError());
    }
    std::string tmp_path = toUtf8(std::wstring(wtmp_path));
    std::stringstream ss;
    ss << tmp_path << "leveldbtest-" << std::this_thread::get_id();
    *result = ss.str();

    // Directory may already exist
    CreateDir(*result);
    return Status::OK();
  }

  Status NewLogger(const std::string& filename, Logger** result) override {
    auto wFilename = toUtf16(filename);
    std::FILE* fp = _wfopen(wFilename.c_str(), L"w");
    if (fp == nullptr) {
      *result = nullptr;
      return WindowsError(filename, ::GetLastError());
    } else {
      *result = new WindowsLogger(fp);
      return Status::OK();
    }
  }

  uint64_t NowMicros() override {
    // GetSystemTimeAsFileTime typically has a resolution of 10-20 msec.
    // TODO(cmumford): Switch to GetSystemTimePreciseAsFileTime which is
    // available in Windows 8 and later.
    FILETIME ft;
    ::GetSystemTimeAsFileTime(&ft);
    // Each tick represents a 100-nanosecond intervals since January 1, 1601
    // (UTC).
    uint64_t num_ticks =
        (static_cast<uint64_t>(ft.dwHighDateTime) << 32) + ft.dwLowDateTime;
    return num_ticks / 10;
  }

  void SleepForMicroseconds(int micros) override {
    std::this_thread::sleep_for(std::chrono::microseconds(micros));
  }

 private:
  void BackgroundThreadMain();

  static void BackgroundThreadEntryPoint(WindowsEnv* env) {
    env->BackgroundThreadMain();
  }

  // Stores the work item data in a Schedule() call.
  //
  // Instances are constructed on the thread calling Schedule() and used on the
  // background thread.
  //
  // This structure is thread-safe because it is immutable.
  struct BackgroundWorkItem {
    explicit BackgroundWorkItem(void (*function)(void* arg), void* arg)
        : function(function), arg(arg) {}

    void (*const function)(void*);
    void* const arg;
  };

  port::Mutex background_work_mutex_;
  port::CondVar background_work_cv_ GUARDED_BY(background_work_mutex_);
  bool started_background_thread_ GUARDED_BY(background_work_mutex_);

  std::queue<BackgroundWorkItem> background_work_queue_
      GUARDED_BY(background_work_mutex_);

  Limiter mmap_limiter_;  // Thread-safe.

  // Converts a Windows wide multi-byte UTF-16 string to a UTF-8 string.
  // See http://utf8everywhere.org/#windows
  std::string toUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(
        CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0],
                        size_needed, NULL, NULL);
    return strTo;
  }

  // Converts a UTF-8 string to a Windows UTF-16 multi-byte wide character
  // string.
  // See http://utf8everywhere.org/#windows
  std::wstring toUtf16(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed =
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring strTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &strTo[0],
                        size_needed);
    return strTo;
  }
};

// Return the maximum number of concurrent mmaps.
int MaxMmaps() { return g_mmap_limit; }

WindowsEnv::WindowsEnv()
    : background_work_cv_(&background_work_mutex_),
      started_background_thread_(false),
      mmap_limiter_(MaxMmaps()) {}

void WindowsEnv::Schedule(
    void (*background_work_function)(void* background_work_arg),
    void* background_work_arg) {
  background_work_mutex_.Lock();

  // Start the background thread, if we haven't done so already.
  if (!started_background_thread_) {
    started_background_thread_ = true;
    std::thread background_thread(WindowsEnv::BackgroundThreadEntryPoint, this);
    background_thread.detach();
  }

  // If the queue is empty, the background thread may be waiting for work.
  if (background_work_queue_.empty()) {
    background_work_cv_.Signal();
  }

  background_work_queue_.emplace(background_work_function, background_work_arg);
  background_work_mutex_.Unlock();
}

void WindowsEnv::BackgroundThreadMain() {
  while (true) {
    background_work_mutex_.Lock();

    // Wait until there is work to be done.
    while (background_work_queue_.empty()) {
      background_work_cv_.Wait();
    }

    assert(!background_work_queue_.empty());
    auto background_work_function = background_work_queue_.front().function;
    void* background_work_arg = background_work_queue_.front().arg;
    background_work_queue_.pop();

    background_work_mutex_.Unlock();
    background_work_function(background_work_arg);
  }
}

// Wraps an Env instance whose destructor is never created.
//
// Intended usage:
//   using PlatformSingletonEnv = SingletonEnv<PlatformEnv>;
//   void ConfigurePosixEnv(int param) {
//     PlatformSingletonEnv::AssertEnvNotInitialized();
//     // set global configuration flags.
//   }
//   Env* Env::Default() {
//     static PlatformSingletonEnv default_env;
//     return default_env.env();
//   }
template <typename EnvType>
class SingletonEnv {
 public:
  SingletonEnv() {
#if !defined(NDEBUG)
    env_initialized_.store(true, std::memory_order_relaxed);
#endif  // !defined(NDEBUG)
    static_assert(sizeof(env_storage_) >= sizeof(EnvType),
                  "env_storage_ will not fit the Env");
    static_assert(std::is_standard_layout_v<SingletonEnv<EnvType>>);
    static_assert(
        offsetof(SingletonEnv<EnvType>, env_storage_) % alignof(EnvType) == 0,
        "env_storage_ does not meet the Env's alignment needs");
    static_assert(alignof(SingletonEnv<EnvType>) % alignof(EnvType) == 0,
                  "env_storage_ does not meet the Env's alignment needs");
    new (env_storage_) EnvType();
  }
  ~SingletonEnv() = default;

  SingletonEnv(const SingletonEnv&) = delete;
  SingletonEnv& operator=(const SingletonEnv&) = delete;

  Env* env() { return reinterpret_cast<Env*>(&env_storage_); }

  static void AssertEnvNotInitialized() {
#if !defined(NDEBUG)
    assert(!env_initialized_.load(std::memory_order_relaxed));
#endif  // !defined(NDEBUG)
  }

 private:
  alignas(EnvType) char env_storage_[sizeof(EnvType)];
#if !defined(NDEBUG)
  static std::atomic<bool> env_initialized_;
#endif  // !defined(NDEBUG)
};

#if !defined(NDEBUG)
template <typename EnvType>
std::atomic<bool> SingletonEnv<EnvType>::env_initialized_;
#endif  // !defined(NDEBUG)

using WindowsDefaultEnv = SingletonEnv<WindowsEnv>;

}  // namespace

void EnvWindowsTestHelper::SetReadOnlyMMapLimit(int limit) {
  WindowsDefaultEnv::AssertEnvNotInitialized();
  g_mmap_limit = limit;
}

Env* Env::Default() {
  static WindowsDefaultEnv env_container;
  return env_container.env();
}

}  // namespace leveldb

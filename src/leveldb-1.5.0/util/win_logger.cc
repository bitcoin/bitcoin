// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "util/win_logger.h"

#include <windows.h>

namespace leveldb {

void WinLogger::Logv(const char* format, va_list ap) {
  const uint64_t thread_id = static_cast<uint64_t>(::GetCurrentThreadId());

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

    // GetSystemTime returns UTC time, we want local time!
    ::GetLocalTime(&st);

#ifdef _MSC_VER
    p += _snprintf_s(p, limit - p, _TRUNCATE,
      "%04d/%02d/%02d-%02d:%02d:%02d.%03d %llx ",
      st.wYear,
      st.wMonth,
      st.wDay,
      st.wHour,
      st.wMinute,
      st.wSecond,
      st.wMilliseconds,
      static_cast<long long unsigned int>(thread_id));
#else
#ifdef __MINGW32__
    p += snprintf(p, limit - p,
      "%04d/%02d/%02d-%02d:%02d:%02d.%03d %llx ",
      st.wYear,
      st.wMonth,
      st.wDay,
      st.wHour,
      st.wMinute,
      st.wSecond,
      st.wMilliseconds,
      static_cast<long long unsigned int>(thread_id));
#else
#error Unable to detect Windows compiler (neither _MSC_VER nor __MINGW32__ are set)
#endif
#endif

    // Print the message
    if (p < limit) {
      va_list backup_ap = ap;
      p += vsnprintf(p, limit - p, format, backup_ap);
      va_end(backup_ap);
    }

    // Truncate to available space if necessary
    if (p >= limit) {
      if (iter == 0) {
        continue; // Try again with larger buffer
      } else {
        p = limit - 1;
      }
    }

    // Add newline if necessary
    if (p == base || p[-1] != '\n') {
      *p++ = '\n';
    }

    assert(p <= limit);
    fwrite(base, 1, p - base, file_);
    fflush(file_);
    if (base != buffer) {
      delete[] base;
    }
    break;
  }
}

}
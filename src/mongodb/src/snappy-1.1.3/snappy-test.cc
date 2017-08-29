// Copyright 2011 Google Inc. All Rights Reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Various stubs for the unit tests for the open-source version of Snappy.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include "snappy-test.h"

#include <algorithm>

DEFINE_bool(run_microbenchmarks, true,
            "Run microbenchmarks before doing anything else.");

namespace snappy {

string ReadTestDataFile(const string& base, size_t size_limit) {
  string contents;
  const char* srcdir = getenv("srcdir");  // This is set by Automake.
  string prefix;
  if (srcdir) {
    prefix = string(srcdir) + "/";
  }
  file::GetContents(prefix + "testdata/" + base, &contents, file::Defaults()
      ).CheckSuccess();
  if (size_limit > 0) {
    contents = contents.substr(0, size_limit);
  }
  return contents;
}

string ReadTestDataFile(const string& base) {
  return ReadTestDataFile(base, 0);
}

string StringPrintf(const char* format, ...) {
  char buf[4096];
  va_list ap;
  va_start(ap, format);
  vsnprintf(buf, sizeof(buf), format, ap);
  va_end(ap);
  return buf;
}

bool benchmark_running = false;
int64 benchmark_real_time_us = 0;
int64 benchmark_cpu_time_us = 0;
string *benchmark_label = NULL;
int64 benchmark_bytes_processed = 0;

void ResetBenchmarkTiming() {
  benchmark_real_time_us = 0;
  benchmark_cpu_time_us = 0;
}

#ifdef WIN32
LARGE_INTEGER benchmark_start_real;
FILETIME benchmark_start_cpu;
#else  // WIN32
struct timeval benchmark_start_real;
struct rusage benchmark_start_cpu;
#endif  // WIN32

void StartBenchmarkTiming() {
#ifdef WIN32
  QueryPerformanceCounter(&benchmark_start_real);
  FILETIME dummy;
  CHECK(GetProcessTimes(
      GetCurrentProcess(), &dummy, &dummy, &dummy, &benchmark_start_cpu));
#else
  gettimeofday(&benchmark_start_real, NULL);
  if (getrusage(RUSAGE_SELF, &benchmark_start_cpu) == -1) {
    perror("getrusage(RUSAGE_SELF)");
    exit(1);
  }
#endif
  benchmark_running = true;
}

void StopBenchmarkTiming() {
  if (!benchmark_running) {
    return;
  }

#ifdef WIN32
  LARGE_INTEGER benchmark_stop_real;
  LARGE_INTEGER benchmark_frequency;
  QueryPerformanceCounter(&benchmark_stop_real);
  QueryPerformanceFrequency(&benchmark_frequency);

  double elapsed_real = static_cast<double>(
      benchmark_stop_real.QuadPart - benchmark_start_real.QuadPart) /
      benchmark_frequency.QuadPart;
  benchmark_real_time_us += elapsed_real * 1e6 + 0.5;

  FILETIME benchmark_stop_cpu, dummy;
  CHECK(GetProcessTimes(
      GetCurrentProcess(), &dummy, &dummy, &dummy, &benchmark_stop_cpu));

  ULARGE_INTEGER start_ulargeint;
  start_ulargeint.LowPart = benchmark_start_cpu.dwLowDateTime;
  start_ulargeint.HighPart = benchmark_start_cpu.dwHighDateTime;

  ULARGE_INTEGER stop_ulargeint;
  stop_ulargeint.LowPart = benchmark_stop_cpu.dwLowDateTime;
  stop_ulargeint.HighPart = benchmark_stop_cpu.dwHighDateTime;

  benchmark_cpu_time_us +=
      (stop_ulargeint.QuadPart - start_ulargeint.QuadPart + 5) / 10;
#else  // WIN32
  struct timeval benchmark_stop_real;
  gettimeofday(&benchmark_stop_real, NULL);
  benchmark_real_time_us +=
      1000000 * (benchmark_stop_real.tv_sec - benchmark_start_real.tv_sec);
  benchmark_real_time_us +=
      (benchmark_stop_real.tv_usec - benchmark_start_real.tv_usec);

  struct rusage benchmark_stop_cpu;
  if (getrusage(RUSAGE_SELF, &benchmark_stop_cpu) == -1) {
    perror("getrusage(RUSAGE_SELF)");
    exit(1);
  }
  benchmark_cpu_time_us += 1000000 * (benchmark_stop_cpu.ru_utime.tv_sec -
                                      benchmark_start_cpu.ru_utime.tv_sec);
  benchmark_cpu_time_us += (benchmark_stop_cpu.ru_utime.tv_usec -
                            benchmark_start_cpu.ru_utime.tv_usec);
#endif  // WIN32

  benchmark_running = false;
}

void SetBenchmarkLabel(const string& str) {
  if (benchmark_label) {
    delete benchmark_label;
  }
  benchmark_label = new string(str);
}

void SetBenchmarkBytesProcessed(int64 bytes) {
  benchmark_bytes_processed = bytes;
}

struct BenchmarkRun {
  int64 real_time_us;
  int64 cpu_time_us;
};

struct BenchmarkCompareCPUTime {
  bool operator() (const BenchmarkRun& a, const BenchmarkRun& b) const {
    return a.cpu_time_us < b.cpu_time_us;
  }
};

void Benchmark::Run() {
  for (int test_case_num = start_; test_case_num <= stop_; ++test_case_num) {
    // Run a few iterations first to find out approximately how fast
    // the benchmark is.
    const int kCalibrateIterations = 100;
    ResetBenchmarkTiming();
    StartBenchmarkTiming();
    (*function_)(kCalibrateIterations, test_case_num);
    StopBenchmarkTiming();

    // Let each test case run for about 200ms, but at least as many
    // as we used to calibrate.
    // Run five times and pick the median.
    const int kNumRuns = 5;
    const int kMedianPos = kNumRuns / 2;
    int num_iterations = 0;
    if (benchmark_real_time_us > 0) {
      num_iterations = 200000 * kCalibrateIterations / benchmark_real_time_us;
    }
    num_iterations = max(num_iterations, kCalibrateIterations);
    BenchmarkRun benchmark_runs[kNumRuns];

    for (int run = 0; run < kNumRuns; ++run) {
      ResetBenchmarkTiming();
      StartBenchmarkTiming();
      (*function_)(num_iterations, test_case_num);
      StopBenchmarkTiming();

      benchmark_runs[run].real_time_us = benchmark_real_time_us;
      benchmark_runs[run].cpu_time_us = benchmark_cpu_time_us;
    }

    string heading = StringPrintf("%s/%d", name_.c_str(), test_case_num);
    string human_readable_speed;

    nth_element(benchmark_runs,
                benchmark_runs + kMedianPos,
                benchmark_runs + kNumRuns,
                BenchmarkCompareCPUTime());
    int64 real_time_us = benchmark_runs[kMedianPos].real_time_us;
    int64 cpu_time_us = benchmark_runs[kMedianPos].cpu_time_us;
    if (cpu_time_us <= 0) {
      human_readable_speed = "?";
    } else {
      int64 bytes_per_second =
          benchmark_bytes_processed * 1000000 / cpu_time_us;
      if (bytes_per_second < 1024) {
        human_readable_speed = StringPrintf("%dB/s", bytes_per_second);
      } else if (bytes_per_second < 1024 * 1024) {
        human_readable_speed = StringPrintf(
            "%.1fkB/s", bytes_per_second / 1024.0f);
      } else if (bytes_per_second < 1024 * 1024 * 1024) {
        human_readable_speed = StringPrintf(
            "%.1fMB/s", bytes_per_second / (1024.0f * 1024.0f));
      } else {
        human_readable_speed = StringPrintf(
            "%.1fGB/s", bytes_per_second / (1024.0f * 1024.0f * 1024.0f));
      }
    }

    fprintf(stderr,
#ifdef WIN32
            "%-18s %10I64d %10I64d %10d %s  %s\n",
#else
            "%-18s %10lld %10lld %10d %s  %s\n",
#endif
            heading.c_str(),
            static_cast<long long>(real_time_us * 1000 / num_iterations),
            static_cast<long long>(cpu_time_us * 1000 / num_iterations),
            num_iterations,
            human_readable_speed.c_str(),
            benchmark_label->c_str());
  }
}

#ifdef HAVE_LIBZ

ZLib::ZLib()
    : comp_init_(false),
      uncomp_init_(false) {
  Reinit();
}

ZLib::~ZLib() {
  if (comp_init_)   { deflateEnd(&comp_stream_); }
  if (uncomp_init_) { inflateEnd(&uncomp_stream_); }
}

void ZLib::Reinit() {
  compression_level_ = Z_DEFAULT_COMPRESSION;
  window_bits_ = MAX_WBITS;
  mem_level_ =  8;  // DEF_MEM_LEVEL
  if (comp_init_) {
    deflateEnd(&comp_stream_);
    comp_init_ = false;
  }
  if (uncomp_init_) {
    inflateEnd(&uncomp_stream_);
    uncomp_init_ = false;
  }
  first_chunk_ = true;
}

void ZLib::Reset() {
  first_chunk_ = true;
}

// --------- COMPRESS MODE

// Initialization method to be called if we hit an error while
// compressing. On hitting an error, call this method before returning
// the error.
void ZLib::CompressErrorInit() {
  deflateEnd(&comp_stream_);
  comp_init_ = false;
  Reset();
}

int ZLib::DeflateInit() {
  return deflateInit2(&comp_stream_,
                      compression_level_,
                      Z_DEFLATED,
                      window_bits_,
                      mem_level_,
                      Z_DEFAULT_STRATEGY);
}

int ZLib::CompressInit(Bytef *dest, uLongf *destLen,
                       const Bytef *source, uLong *sourceLen) {
  int err;

  comp_stream_.next_in = (Bytef*)source;
  comp_stream_.avail_in = (uInt)*sourceLen;
  if ((uLong)comp_stream_.avail_in != *sourceLen) return Z_BUF_ERROR;
  comp_stream_.next_out = dest;
  comp_stream_.avail_out = (uInt)*destLen;
  if ((uLong)comp_stream_.avail_out != *destLen) return Z_BUF_ERROR;

  if ( !first_chunk_ )   // only need to set up stream the first time through
    return Z_OK;

  if (comp_init_) {      // we've already initted it
    err = deflateReset(&comp_stream_);
    if (err != Z_OK) {
      LOG(WARNING) << "ERROR: Can't reset compress object; creating a new one";
      deflateEnd(&comp_stream_);
      comp_init_ = false;
    }
  }
  if (!comp_init_) {     // first use
    comp_stream_.zalloc = (alloc_func)0;
    comp_stream_.zfree = (free_func)0;
    comp_stream_.opaque = (voidpf)0;
    err = DeflateInit();
    if (err != Z_OK) return err;
    comp_init_ = true;
  }
  return Z_OK;
}

// In a perfect world we'd always have the full buffer to compress
// when the time came, and we could just call Compress().  Alas, we
// want to do chunked compression on our webserver.  In this
// application, we compress the header, send it off, then compress the
// results, send them off, then compress the footer.  Thus we need to
// use the chunked compression features of zlib.
int ZLib::CompressAtMostOrAll(Bytef *dest, uLongf *destLen,
                              const Bytef *source, uLong *sourceLen,
                              int flush_mode) {   // Z_FULL_FLUSH or Z_FINISH
  int err;

  if ( (err=CompressInit(dest, destLen, source, sourceLen)) != Z_OK )
    return err;

  // This is used to figure out how many bytes we wrote *this chunk*
  int compressed_size = comp_stream_.total_out;

  // Some setup happens only for the first chunk we compress in a run
  if ( first_chunk_ ) {
    first_chunk_ = false;
  }

  // flush_mode is Z_FINISH for all mode, Z_SYNC_FLUSH for incremental
  // compression.
  err = deflate(&comp_stream_, flush_mode);

  *sourceLen = comp_stream_.avail_in;

  if ((err == Z_STREAM_END || err == Z_OK)
      && comp_stream_.avail_in == 0
      && comp_stream_.avail_out != 0 ) {
    // we processed everything ok and the output buffer was large enough.
    ;
  } else if (err == Z_STREAM_END && comp_stream_.avail_in > 0) {
    return Z_BUF_ERROR;                            // should never happen
  } else if (err != Z_OK && err != Z_STREAM_END && err != Z_BUF_ERROR) {
    // an error happened
    CompressErrorInit();
    return err;
  } else if (comp_stream_.avail_out == 0) {     // not enough space
    err = Z_BUF_ERROR;
  }

  assert(err == Z_OK || err == Z_STREAM_END || err == Z_BUF_ERROR);
  if (err == Z_STREAM_END)
    err = Z_OK;

  // update the crc and other metadata
  compressed_size = comp_stream_.total_out - compressed_size;  // delta
  *destLen = compressed_size;

  return err;
}

int ZLib::CompressChunkOrAll(Bytef *dest, uLongf *destLen,
                             const Bytef *source, uLong sourceLen,
                             int flush_mode) {   // Z_FULL_FLUSH or Z_FINISH
  const int ret =
    CompressAtMostOrAll(dest, destLen, source, &sourceLen, flush_mode);
  if (ret == Z_BUF_ERROR)
    CompressErrorInit();
  return ret;
}

// This routine only initializes the compression stream once.  Thereafter, it
// just does a deflateReset on the stream, which should be faster.
int ZLib::Compress(Bytef *dest, uLongf *destLen,
                   const Bytef *source, uLong sourceLen) {
  int err;
  if ( (err=CompressChunkOrAll(dest, destLen, source, sourceLen,
                               Z_FINISH)) != Z_OK )
    return err;
  Reset();         // reset for next call to Compress

  return Z_OK;
}


// --------- UNCOMPRESS MODE

int ZLib::InflateInit() {
  return inflateInit2(&uncomp_stream_, MAX_WBITS);
}

// Initialization method to be called if we hit an error while
// uncompressing. On hitting an error, call this method before
// returning the error.
void ZLib::UncompressErrorInit() {
  inflateEnd(&uncomp_stream_);
  uncomp_init_ = false;
  Reset();
}

int ZLib::UncompressInit(Bytef *dest, uLongf *destLen,
                         const Bytef *source, uLong *sourceLen) {
  int err;

  uncomp_stream_.next_in = (Bytef*)source;
  uncomp_stream_.avail_in = (uInt)*sourceLen;
  // Check for source > 64K on 16-bit machine:
  if ((uLong)uncomp_stream_.avail_in != *sourceLen) return Z_BUF_ERROR;

  uncomp_stream_.next_out = dest;
  uncomp_stream_.avail_out = (uInt)*destLen;
  if ((uLong)uncomp_stream_.avail_out != *destLen) return Z_BUF_ERROR;

  if ( !first_chunk_ )   // only need to set up stream the first time through
    return Z_OK;

  if (uncomp_init_) {    // we've already initted it
    err = inflateReset(&uncomp_stream_);
    if (err != Z_OK) {
      LOG(WARNING)
        << "ERROR: Can't reset uncompress object; creating a new one";
      UncompressErrorInit();
    }
  }
  if (!uncomp_init_) {
    uncomp_stream_.zalloc = (alloc_func)0;
    uncomp_stream_.zfree = (free_func)0;
    uncomp_stream_.opaque = (voidpf)0;
    err = InflateInit();
    if (err != Z_OK) return err;
    uncomp_init_ = true;
  }
  return Z_OK;
}

// If you compressed your data a chunk at a time, with CompressChunk,
// you can uncompress it a chunk at a time with UncompressChunk.
// Only difference bewteen chunked and unchunked uncompression
// is the flush mode we use: Z_SYNC_FLUSH (chunked) or Z_FINISH (unchunked).
int ZLib::UncompressAtMostOrAll(Bytef *dest, uLongf *destLen,
                                const Bytef *source, uLong *sourceLen,
                                int flush_mode) {  // Z_SYNC_FLUSH or Z_FINISH
  int err = Z_OK;

  if ( (err=UncompressInit(dest, destLen, source, sourceLen)) != Z_OK ) {
    LOG(WARNING) << "UncompressInit: Error: " << err << " SourceLen: "
                 << *sourceLen;
    return err;
  }

  // This is used to figure out how many output bytes we wrote *this chunk*:
  const uLong old_total_out = uncomp_stream_.total_out;

  // This is used to figure out how many input bytes we read *this chunk*:
  const uLong old_total_in = uncomp_stream_.total_in;

  // Some setup happens only for the first chunk we compress in a run
  if ( first_chunk_ ) {
    first_chunk_ = false;                          // so we don't do this again

    // For the first chunk *only* (to avoid infinite troubles), we let
    // there be no actual data to uncompress.  This sometimes triggers
    // when the input is only the gzip header, say.
    if ( *sourceLen == 0 ) {
      *destLen = 0;
      return Z_OK;
    }
  }

  // We'll uncompress as much as we can.  If we end OK great, otherwise
  // if we get an error that seems to be the gzip footer, we store the
  // gzip footer and return OK, otherwise we return the error.

  // flush_mode is Z_SYNC_FLUSH for chunked mode, Z_FINISH for all mode.
  err = inflate(&uncomp_stream_, flush_mode);

  // Figure out how many bytes of the input zlib slurped up:
  const uLong bytes_read = uncomp_stream_.total_in - old_total_in;
  CHECK_LE(source + bytes_read, source + *sourceLen);
  *sourceLen = uncomp_stream_.avail_in;

  if ((err == Z_STREAM_END || err == Z_OK)  // everything went ok
             && uncomp_stream_.avail_in == 0) {    // and we read it all
    ;
  } else if (err == Z_STREAM_END && uncomp_stream_.avail_in > 0) {
    LOG(WARNING)
      << "UncompressChunkOrAll: Received some extra data, bytes total: "
      << uncomp_stream_.avail_in << " bytes: "
      << string(reinterpret_cast<const char *>(uncomp_stream_.next_in),
                min(int(uncomp_stream_.avail_in), 20));
    UncompressErrorInit();
    return Z_DATA_ERROR;       // what's the extra data for?
  } else if (err != Z_OK && err != Z_STREAM_END && err != Z_BUF_ERROR) {
    // an error happened
    LOG(WARNING) << "UncompressChunkOrAll: Error: " << err
                 << " avail_out: " << uncomp_stream_.avail_out;
    UncompressErrorInit();
    return err;
  } else if (uncomp_stream_.avail_out == 0) {
    err = Z_BUF_ERROR;
  }

  assert(err == Z_OK || err == Z_BUF_ERROR || err == Z_STREAM_END);
  if (err == Z_STREAM_END)
    err = Z_OK;

  *destLen = uncomp_stream_.total_out - old_total_out;  // size for this call

  return err;
}

int ZLib::UncompressChunkOrAll(Bytef *dest, uLongf *destLen,
                               const Bytef *source, uLong sourceLen,
                               int flush_mode) {  // Z_SYNC_FLUSH or Z_FINISH
  const int ret =
    UncompressAtMostOrAll(dest, destLen, source, &sourceLen, flush_mode);
  if (ret == Z_BUF_ERROR)
    UncompressErrorInit();
  return ret;
}

int ZLib::UncompressAtMost(Bytef *dest, uLongf *destLen,
                          const Bytef *source, uLong *sourceLen) {
  return UncompressAtMostOrAll(dest, destLen, source, sourceLen, Z_SYNC_FLUSH);
}

// We make sure we've uncompressed everything, that is, the current
// uncompress stream is at a compressed-buffer-EOF boundary.  In gzip
// mode, we also check the gzip footer to make sure we pass the gzip
// consistency checks.  We RETURN true iff both types of checks pass.
bool ZLib::UncompressChunkDone() {
  assert(!first_chunk_ && uncomp_init_);
  // Make sure we're at the end-of-compressed-data point.  This means
  // if we call inflate with Z_FINISH we won't consume any input or
  // write any output
  Bytef dummyin, dummyout;
  uLongf dummylen = 0;
  if ( UncompressChunkOrAll(&dummyout, &dummylen, &dummyin, 0, Z_FINISH)
       != Z_OK ) {
    return false;
  }

  // Make sure that when we exit, we can start a new round of chunks later
  Reset();

  return true;
}

// Uncompresses the source buffer into the destination buffer.
// The destination buffer must be long enough to hold the entire
// decompressed contents.
//
// We only initialize the uncomp_stream once.  Thereafter, we use
// inflateReset, which should be faster.
//
// Returns Z_OK on success, otherwise, it returns a zlib error code.
int ZLib::Uncompress(Bytef *dest, uLongf *destLen,
                     const Bytef *source, uLong sourceLen) {
  int err;
  if ( (err=UncompressChunkOrAll(dest, destLen, source, sourceLen,
                                 Z_FINISH)) != Z_OK ) {
    Reset();                           // let us try to compress again
    return err;
  }
  if ( !UncompressChunkDone() )        // calls Reset()
    return Z_DATA_ERROR;
  return Z_OK;  // stream_end is ok
}

#endif  // HAVE_LIBZ

}  // namespace snappy

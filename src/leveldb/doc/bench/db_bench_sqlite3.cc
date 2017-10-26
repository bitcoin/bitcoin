// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "util/histogram.h"
#include "util/random.h"
#include "util/testutil.h"

// Comma-separated list of operations to run in the specified order
//   Actual benchmarks:
//
//   fillseq       -- write N values in sequential key order in async mode
//   fillseqsync   -- write N/100 values in sequential key order in sync mode
//   fillseqbatch  -- batch write N values in sequential key order in async mode
//   fillrandom    -- write N values in random key order in async mode
//   fillrandsync  -- write N/100 values in random key order in sync mode
//   fillrandbatch -- batch write N values in sequential key order in async mode
//   overwrite     -- overwrite N values in random key order in async mode
//   fillrand100K  -- write N/1000 100K values in random order in async mode
//   fillseq100K   -- write N/1000 100K values in sequential order in async mode
//   readseq       -- read N times sequentially
//   readrandom    -- read N times in random order
//   readrand100K  -- read N/1000 100K values in sequential order in async mode
static const char* FLAGS_benchmarks =
    "fillseq,"
    "fillseqsync,"
    "fillseqbatch,"
    "fillrandom,"
    "fillrandsync,"
    "fillrandbatch,"
    "overwrite,"
    "overwritebatch,"
    "readrandom,"
    "readseq,"
    "fillrand100K,"
    "fillseq100K,"
    "readseq,"
    "readrand100K,"
    ;

// Number of key/values to place in database
static int FLAGS_num = 1000000;

// Number of read operations to do.  If negative, do FLAGS_num reads.
static int FLAGS_reads = -1;

// Size of each value
static int FLAGS_value_size = 100;

// Print histogram of operation timings
static bool FLAGS_histogram = false;

// Arrange to generate values that shrink to this fraction of
// their original size after compression
static double FLAGS_compression_ratio = 0.5;

// Page size. Default 1 KB.
static int FLAGS_page_size = 1024;

// Number of pages.
// Default cache size = FLAGS_page_size * FLAGS_num_pages = 4 MB.
static int FLAGS_num_pages = 4096;

// If true, do not destroy the existing database.  If you set this
// flag and also specify a benchmark that wants a fresh database, that
// benchmark will fail.
static bool FLAGS_use_existing_db = false;

// If true, we allow batch writes to occur
static bool FLAGS_transaction = true;

// If true, we enable Write-Ahead Logging
static bool FLAGS_WAL_enabled = true;

// Use the db with the following name.
static const char* FLAGS_db = NULL;

inline
static void ExecErrorCheck(int status, char *err_msg) {
  if (status != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", err_msg);
    sqlite3_free(err_msg);
    exit(1);
  }
}

inline
static void StepErrorCheck(int status) {
  if (status != SQLITE_DONE) {
    fprintf(stderr, "SQL step error: status = %d\n", status);
    exit(1);
  }
}

inline
static void ErrorCheck(int status) {
  if (status != SQLITE_OK) {
    fprintf(stderr, "sqlite3 error: status = %d\n", status);
    exit(1);
  }
}

inline
static void WalCheckpoint(sqlite3* db_) {
  // Flush all writes to disk
  if (FLAGS_WAL_enabled) {
    sqlite3_wal_checkpoint_v2(db_, NULL, SQLITE_CHECKPOINT_FULL, NULL, NULL);
  }
}

namespace leveldb {

// Helper for quickly generating random data.
namespace {
class RandomGenerator {
 private:
  std::string data_;
  int pos_;

 public:
  RandomGenerator() {
    // We use a limited amount of data over and over again and ensure
    // that it is larger than the compression window (32KB), and also
    // large enough to serve all typical value sizes we want to write.
    Random rnd(301);
    std::string piece;
    while (data_.size() < 1048576) {
      // Add a short fragment that is as compressible as specified
      // by FLAGS_compression_ratio.
      test::CompressibleString(&rnd, FLAGS_compression_ratio, 100, &piece);
      data_.append(piece);
    }
    pos_ = 0;
  }

  Slice Generate(int len) {
    if (pos_ + len > data_.size()) {
      pos_ = 0;
      assert(len < data_.size());
    }
    pos_ += len;
    return Slice(data_.data() + pos_ - len, len);
  }
};

static Slice TrimSpace(Slice s) {
  int start = 0;
  while (start < s.size() && isspace(s[start])) {
    start++;
  }
  int limit = s.size();
  while (limit > start && isspace(s[limit-1])) {
    limit--;
  }
  return Slice(s.data() + start, limit - start);
}

}  // namespace

class Benchmark {
 private:
  sqlite3* db_;
  int db_num_;
  int num_;
  int reads_;
  double start_;
  double last_op_finish_;
  int64_t bytes_;
  std::string message_;
  Histogram hist_;
  RandomGenerator gen_;
  Random rand_;

  // State kept for progress messages
  int done_;
  int next_report_;     // When to report next

  void PrintHeader() {
    const int kKeySize = 16;
    PrintEnvironment();
    fprintf(stdout, "Keys:       %d bytes each\n", kKeySize);
    fprintf(stdout, "Values:     %d bytes each\n", FLAGS_value_size);
    fprintf(stdout, "Entries:    %d\n", num_);
    fprintf(stdout, "RawSize:    %.1f MB (estimated)\n",
            ((static_cast<int64_t>(kKeySize + FLAGS_value_size) * num_)
             / 1048576.0));
    PrintWarnings();
    fprintf(stdout, "------------------------------------------------\n");
  }

  void PrintWarnings() {
#if defined(__GNUC__) && !defined(__OPTIMIZE__)
    fprintf(stdout,
            "WARNING: Optimization is disabled: benchmarks unnecessarily slow\n"
            );
#endif
#ifndef NDEBUG
    fprintf(stdout,
            "WARNING: Assertions are enabled; benchmarks unnecessarily slow\n");
#endif
  }

  void PrintEnvironment() {
    fprintf(stderr, "SQLite:     version %s\n", SQLITE_VERSION);

#if defined(__linux)
    time_t now = time(NULL);
    fprintf(stderr, "Date:       %s", ctime(&now));  // ctime() adds newline

    FILE* cpuinfo = fopen("/proc/cpuinfo", "r");
    if (cpuinfo != NULL) {
      char line[1000];
      int num_cpus = 0;
      std::string cpu_type;
      std::string cache_size;
      while (fgets(line, sizeof(line), cpuinfo) != NULL) {
        const char* sep = strchr(line, ':');
        if (sep == NULL) {
          continue;
        }
        Slice key = TrimSpace(Slice(line, sep - 1 - line));
        Slice val = TrimSpace(Slice(sep + 1));
        if (key == "model name") {
          ++num_cpus;
          cpu_type = val.ToString();
        } else if (key == "cache size") {
          cache_size = val.ToString();
        }
      }
      fclose(cpuinfo);
      fprintf(stderr, "CPU:        %d * %s\n", num_cpus, cpu_type.c_str());
      fprintf(stderr, "CPUCache:   %s\n", cache_size.c_str());
    }
#endif
  }

  void Start() {
    start_ = Env::Default()->NowMicros() * 1e-6;
    bytes_ = 0;
    message_.clear();
    last_op_finish_ = start_;
    hist_.Clear();
    done_ = 0;
    next_report_ = 100;
  }

  void FinishedSingleOp() {
    if (FLAGS_histogram) {
      double now = Env::Default()->NowMicros() * 1e-6;
      double micros = (now - last_op_finish_) * 1e6;
      hist_.Add(micros);
      if (micros > 20000) {
        fprintf(stderr, "long op: %.1f micros%30s\r", micros, "");
        fflush(stderr);
      }
      last_op_finish_ = now;
    }

    done_++;
    if (done_ >= next_report_) {
      if      (next_report_ < 1000)   next_report_ += 100;
      else if (next_report_ < 5000)   next_report_ += 500;
      else if (next_report_ < 10000)  next_report_ += 1000;
      else if (next_report_ < 50000)  next_report_ += 5000;
      else if (next_report_ < 100000) next_report_ += 10000;
      else if (next_report_ < 500000) next_report_ += 50000;
      else                            next_report_ += 100000;
      fprintf(stderr, "... finished %d ops%30s\r", done_, "");
      fflush(stderr);
    }
  }

  void Stop(const Slice& name) {
    double finish = Env::Default()->NowMicros() * 1e-6;

    // Pretend at least one op was done in case we are running a benchmark
    // that does not call FinishedSingleOp().
    if (done_ < 1) done_ = 1;

    if (bytes_ > 0) {
      char rate[100];
      snprintf(rate, sizeof(rate), "%6.1f MB/s",
               (bytes_ / 1048576.0) / (finish - start_));
      if (!message_.empty()) {
        message_  = std::string(rate) + " " + message_;
      } else {
        message_ = rate;
      }
    }

    fprintf(stdout, "%-12s : %11.3f micros/op;%s%s\n",
            name.ToString().c_str(),
            (finish - start_) * 1e6 / done_,
            (message_.empty() ? "" : " "),
            message_.c_str());
    if (FLAGS_histogram) {
      fprintf(stdout, "Microseconds per op:\n%s\n", hist_.ToString().c_str());
    }
    fflush(stdout);
  }

 public:
  enum Order {
    SEQUENTIAL,
    RANDOM
  };
  enum DBState {
    FRESH,
    EXISTING
  };

  Benchmark()
  : db_(NULL),
    db_num_(0),
    num_(FLAGS_num),
    reads_(FLAGS_reads < 0 ? FLAGS_num : FLAGS_reads),
    bytes_(0),
    rand_(301) {
    std::vector<std::string> files;
    std::string test_dir;
    Env::Default()->GetTestDirectory(&test_dir);
    Env::Default()->GetChildren(test_dir, &files);
    if (!FLAGS_use_existing_db) {
      for (int i = 0; i < files.size(); i++) {
        if (Slice(files[i]).starts_with("dbbench_sqlite3")) {
          std::string file_name(test_dir);
          file_name += "/";
          file_name += files[i];
          Env::Default()->DeleteFile(file_name.c_str());
        }
      }
    }
  }

  ~Benchmark() {
    int status = sqlite3_close(db_);
    ErrorCheck(status);
  }

  void Run() {
    PrintHeader();
    Open();

    const char* benchmarks = FLAGS_benchmarks;
    while (benchmarks != NULL) {
      const char* sep = strchr(benchmarks, ',');
      Slice name;
      if (sep == NULL) {
        name = benchmarks;
        benchmarks = NULL;
      } else {
        name = Slice(benchmarks, sep - benchmarks);
        benchmarks = sep + 1;
      }

      bytes_ = 0;
      Start();

      bool known = true;
      bool write_sync = false;
      if (name == Slice("fillseq")) {
        Write(write_sync, SEQUENTIAL, FRESH, num_, FLAGS_value_size, 1);
        WalCheckpoint(db_);
      } else if (name == Slice("fillseqbatch")) {
        Write(write_sync, SEQUENTIAL, FRESH, num_, FLAGS_value_size, 1000);
        WalCheckpoint(db_);
      } else if (name == Slice("fillrandom")) {
        Write(write_sync, RANDOM, FRESH, num_, FLAGS_value_size, 1);
        WalCheckpoint(db_);
      } else if (name == Slice("fillrandbatch")) {
        Write(write_sync, RANDOM, FRESH, num_, FLAGS_value_size, 1000);
        WalCheckpoint(db_);
      } else if (name == Slice("overwrite")) {
        Write(write_sync, RANDOM, EXISTING, num_, FLAGS_value_size, 1);
        WalCheckpoint(db_);
      } else if (name == Slice("overwritebatch")) {
        Write(write_sync, RANDOM, EXISTING, num_, FLAGS_value_size, 1000);
        WalCheckpoint(db_);
      } else if (name == Slice("fillrandsync")) {
        write_sync = true;
        Write(write_sync, RANDOM, FRESH, num_ / 100, FLAGS_value_size, 1);
        WalCheckpoint(db_);
      } else if (name == Slice("fillseqsync")) {
        write_sync = true;
        Write(write_sync, SEQUENTIAL, FRESH, num_ / 100, FLAGS_value_size, 1);
        WalCheckpoint(db_);
      } else if (name == Slice("fillrand100K")) {
        Write(write_sync, RANDOM, FRESH, num_ / 1000, 100 * 1000, 1);
        WalCheckpoint(db_);
      } else if (name == Slice("fillseq100K")) {
        Write(write_sync, SEQUENTIAL, FRESH, num_ / 1000, 100 * 1000, 1);
        WalCheckpoint(db_);
      } else if (name == Slice("readseq")) {
        ReadSequential();
      } else if (name == Slice("readrandom")) {
        Read(RANDOM, 1);
      } else if (name == Slice("readrand100K")) {
        int n = reads_;
        reads_ /= 1000;
        Read(RANDOM, 1);
        reads_ = n;
      } else {
        known = false;
        if (name != Slice()) {  // No error message for empty name
          fprintf(stderr, "unknown benchmark '%s'\n", name.ToString().c_str());
        }
      }
      if (known) {
        Stop(name);
      }
    }
  }

  void Open() {
    assert(db_ == NULL);

    int status;
    char file_name[100];
    char* err_msg = NULL;
    db_num_++;

    // Open database
    std::string tmp_dir;
    Env::Default()->GetTestDirectory(&tmp_dir);
    snprintf(file_name, sizeof(file_name),
             "%s/dbbench_sqlite3-%d.db",
             tmp_dir.c_str(),
             db_num_);
    status = sqlite3_open(file_name, &db_);
    if (status) {
      fprintf(stderr, "open error: %s\n", sqlite3_errmsg(db_));
      exit(1);
    }

    // Change SQLite cache size
    char cache_size[100];
    snprintf(cache_size, sizeof(cache_size), "PRAGMA cache_size = %d",
             FLAGS_num_pages);
    status = sqlite3_exec(db_, cache_size, NULL, NULL, &err_msg);
    ExecErrorCheck(status, err_msg);

    // FLAGS_page_size is defaulted to 1024
    if (FLAGS_page_size != 1024) {
      char page_size[100];
      snprintf(page_size, sizeof(page_size), "PRAGMA page_size = %d",
               FLAGS_page_size);
      status = sqlite3_exec(db_, page_size, NULL, NULL, &err_msg);
      ExecErrorCheck(status, err_msg);
    }

    // Change journal mode to WAL if WAL enabled flag is on
    if (FLAGS_WAL_enabled) {
      std::string WAL_stmt = "PRAGMA journal_mode = WAL";

      // LevelDB's default cache size is a combined 4 MB
      std::string WAL_checkpoint = "PRAGMA wal_autocheckpoint = 4096";
      status = sqlite3_exec(db_, WAL_stmt.c_str(), NULL, NULL, &err_msg);
      ExecErrorCheck(status, err_msg);
      status = sqlite3_exec(db_, WAL_checkpoint.c_str(), NULL, NULL, &err_msg);
      ExecErrorCheck(status, err_msg);
    }

    // Change locking mode to exclusive and create tables/index for database
    std::string locking_stmt = "PRAGMA locking_mode = EXCLUSIVE";
    std::string create_stmt =
          "CREATE TABLE test (key blob, value blob, PRIMARY KEY(key))";
    std::string stmt_array[] = { locking_stmt, create_stmt };
    int stmt_array_length = sizeof(stmt_array) / sizeof(std::string);
    for (int i = 0; i < stmt_array_length; i++) {
      status = sqlite3_exec(db_, stmt_array[i].c_str(), NULL, NULL, &err_msg);
      ExecErrorCheck(status, err_msg);
    }
  }

  void Write(bool write_sync, Order order, DBState state,
             int num_entries, int value_size, int entries_per_batch) {
    // Create new database if state == FRESH
    if (state == FRESH) {
      if (FLAGS_use_existing_db) {
        message_ = "skipping (--use_existing_db is true)";
        return;
      }
      sqlite3_close(db_);
      db_ = NULL;
      Open();
      Start();
    }

    if (num_entries != num_) {
      char msg[100];
      snprintf(msg, sizeof(msg), "(%d ops)", num_entries);
      message_ = msg;
    }

    char* err_msg = NULL;
    int status;

    sqlite3_stmt *replace_stmt, *begin_trans_stmt, *end_trans_stmt;
    std::string replace_str = "REPLACE INTO test (key, value) VALUES (?, ?)";
    std::string begin_trans_str = "BEGIN TRANSACTION;";
    std::string end_trans_str = "END TRANSACTION;";

    // Check for synchronous flag in options
    std::string sync_stmt = (write_sync) ? "PRAGMA synchronous = FULL" :
                                           "PRAGMA synchronous = OFF";
    status = sqlite3_exec(db_, sync_stmt.c_str(), NULL, NULL, &err_msg);
    ExecErrorCheck(status, err_msg);

    // Preparing sqlite3 statements
    status = sqlite3_prepare_v2(db_, replace_str.c_str(), -1,
                                &replace_stmt, NULL);
    ErrorCheck(status);
    status = sqlite3_prepare_v2(db_, begin_trans_str.c_str(), -1,
                                &begin_trans_stmt, NULL);
    ErrorCheck(status);
    status = sqlite3_prepare_v2(db_, end_trans_str.c_str(), -1,
                                &end_trans_stmt, NULL);
    ErrorCheck(status);

    bool transaction = (entries_per_batch > 1);
    for (int i = 0; i < num_entries; i += entries_per_batch) {
      // Begin write transaction
      if (FLAGS_transaction && transaction) {
        status = sqlite3_step(begin_trans_stmt);
        StepErrorCheck(status);
        status = sqlite3_reset(begin_trans_stmt);
        ErrorCheck(status);
      }

      // Create and execute SQL statements
      for (int j = 0; j < entries_per_batch; j++) {
        const char* value = gen_.Generate(value_size).data();

        // Create values for key-value pair
        const int k = (order == SEQUENTIAL) ? i + j :
                      (rand_.Next() % num_entries);
        char key[100];
        snprintf(key, sizeof(key), "%016d", k);

        // Bind KV values into replace_stmt
        status = sqlite3_bind_blob(replace_stmt, 1, key, 16, SQLITE_STATIC);
        ErrorCheck(status);
        status = sqlite3_bind_blob(replace_stmt, 2, value,
                                   value_size, SQLITE_STATIC);
        ErrorCheck(status);

        // Execute replace_stmt
        bytes_ += value_size + strlen(key);
        status = sqlite3_step(replace_stmt);
        StepErrorCheck(status);

        // Reset SQLite statement for another use
        status = sqlite3_clear_bindings(replace_stmt);
        ErrorCheck(status);
        status = sqlite3_reset(replace_stmt);
        ErrorCheck(status);

        FinishedSingleOp();
      }

      // End write transaction
      if (FLAGS_transaction && transaction) {
        status = sqlite3_step(end_trans_stmt);
        StepErrorCheck(status);
        status = sqlite3_reset(end_trans_stmt);
        ErrorCheck(status);
      }
    }

    status = sqlite3_finalize(replace_stmt);
    ErrorCheck(status);
    status = sqlite3_finalize(begin_trans_stmt);
    ErrorCheck(status);
    status = sqlite3_finalize(end_trans_stmt);
    ErrorCheck(status);
  }

  void Read(Order order, int entries_per_batch) {
    int status;
    sqlite3_stmt *read_stmt, *begin_trans_stmt, *end_trans_stmt;

    std::string read_str = "SELECT * FROM test WHERE key = ?";
    std::string begin_trans_str = "BEGIN TRANSACTION;";
    std::string end_trans_str = "END TRANSACTION;";

    // Preparing sqlite3 statements
    status = sqlite3_prepare_v2(db_, begin_trans_str.c_str(), -1,
                                &begin_trans_stmt, NULL);
    ErrorCheck(status);
    status = sqlite3_prepare_v2(db_, end_trans_str.c_str(), -1,
                                &end_trans_stmt, NULL);
    ErrorCheck(status);
    status = sqlite3_prepare_v2(db_, read_str.c_str(), -1, &read_stmt, NULL);
    ErrorCheck(status);

    bool transaction = (entries_per_batch > 1);
    for (int i = 0; i < reads_; i += entries_per_batch) {
      // Begin read transaction
      if (FLAGS_transaction && transaction) {
        status = sqlite3_step(begin_trans_stmt);
        StepErrorCheck(status);
        status = sqlite3_reset(begin_trans_stmt);
        ErrorCheck(status);
      }

      // Create and execute SQL statements
      for (int j = 0; j < entries_per_batch; j++) {
        // Create key value
        char key[100];
        int k = (order == SEQUENTIAL) ? i + j : (rand_.Next() % reads_);
        snprintf(key, sizeof(key), "%016d", k);

        // Bind key value into read_stmt
        status = sqlite3_bind_blob(read_stmt, 1, key, 16, SQLITE_STATIC);
        ErrorCheck(status);

        // Execute read statement
        while ((status = sqlite3_step(read_stmt)) == SQLITE_ROW) {}
        StepErrorCheck(status);

        // Reset SQLite statement for another use
        status = sqlite3_clear_bindings(read_stmt);
        ErrorCheck(status);
        status = sqlite3_reset(read_stmt);
        ErrorCheck(status);
        FinishedSingleOp();
      }

      // End read transaction
      if (FLAGS_transaction && transaction) {
        status = sqlite3_step(end_trans_stmt);
        StepErrorCheck(status);
        status = sqlite3_reset(end_trans_stmt);
        ErrorCheck(status);
      }
    }

    status = sqlite3_finalize(read_stmt);
    ErrorCheck(status);
    status = sqlite3_finalize(begin_trans_stmt);
    ErrorCheck(status);
    status = sqlite3_finalize(end_trans_stmt);
    ErrorCheck(status);
  }

  void ReadSequential() {
    int status;
    sqlite3_stmt *pStmt;
    std::string read_str = "SELECT * FROM test ORDER BY key";

    status = sqlite3_prepare_v2(db_, read_str.c_str(), -1, &pStmt, NULL);
    ErrorCheck(status);
    for (int i = 0; i < reads_ && SQLITE_ROW == sqlite3_step(pStmt); i++) {
      bytes_ += sqlite3_column_bytes(pStmt, 1) + sqlite3_column_bytes(pStmt, 2);
      FinishedSingleOp();
    }

    status = sqlite3_finalize(pStmt);
    ErrorCheck(status);
  }

};

}  // namespace leveldb

int main(int argc, char** argv) {
  std::string default_db_path;
  for (int i = 1; i < argc; i++) {
    double d;
    int n;
    char junk;
    if (leveldb::Slice(argv[i]).starts_with("--benchmarks=")) {
      FLAGS_benchmarks = argv[i] + strlen("--benchmarks=");
    } else if (sscanf(argv[i], "--histogram=%d%c", &n, &junk) == 1 &&
               (n == 0 || n == 1)) {
      FLAGS_histogram = n;
    } else if (sscanf(argv[i], "--compression_ratio=%lf%c", &d, &junk) == 1) {
      FLAGS_compression_ratio = d;
    } else if (sscanf(argv[i], "--use_existing_db=%d%c", &n, &junk) == 1 &&
               (n == 0 || n == 1)) {
      FLAGS_use_existing_db = n;
    } else if (sscanf(argv[i], "--num=%d%c", &n, &junk) == 1) {
      FLAGS_num = n;
    } else if (sscanf(argv[i], "--reads=%d%c", &n, &junk) == 1) {
      FLAGS_reads = n;
    } else if (sscanf(argv[i], "--value_size=%d%c", &n, &junk) == 1) {
      FLAGS_value_size = n;
    } else if (leveldb::Slice(argv[i]) == leveldb::Slice("--no_transaction")) {
      FLAGS_transaction = false;
    } else if (sscanf(argv[i], "--page_size=%d%c", &n, &junk) == 1) {
      FLAGS_page_size = n;
    } else if (sscanf(argv[i], "--num_pages=%d%c", &n, &junk) == 1) {
      FLAGS_num_pages = n;
    } else if (sscanf(argv[i], "--WAL_enabled=%d%c", &n, &junk) == 1 &&
               (n == 0 || n == 1)) {
      FLAGS_WAL_enabled = n;
    } else if (strncmp(argv[i], "--db=", 5) == 0) {
      FLAGS_db = argv[i] + 5;
    } else {
      fprintf(stderr, "Invalid flag '%s'\n", argv[i]);
      exit(1);
    }
  }

  // Choose a location for the test database if none given with --db=<path>
  if (FLAGS_db == NULL) {
      leveldb::Env::Default()->GetTestDirectory(&default_db_path);
      default_db_path += "/dbbench";
      FLAGS_db = default_db_path.c_str();
  }

  leveldb::Benchmark benchmark;
  benchmark.Run();
  return 0;
}

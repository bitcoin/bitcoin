**LevelDB is a fast key-value storage library written at Google that provides an ordered mapping from string keys to string values.**

[![Build Status](https://travis-ci.org/google/leveldb.svg?branch=master)](https://travis-ci.org/google/leveldb)

Authors: Sanjay Ghemawat (sanjay@google.com) and Jeff Dean (jeff@google.com)

# Features
  * Keys and values are arbitrary byte arrays.
  * Data is stored sorted by key.
  * Callers can provide a custom comparison function to override the sort order.
  * The basic operations are `Put(key,value)`, `Get(key)`, `Delete(key)`.
  * Multiple changes can be made in one atomic batch.
  * Users can create a transient snapshot to get a consistent view of data.
  * Forward and backward iteration is supported over the data.
  * Data is automatically compressed using the [Snappy compression library](http://google.github.io/snappy/).
  * External activity (file system operations etc.) is relayed through a virtual interface so users can customize the operating system interactions.

# Documentation
  [LevelDB library documentation](https://github.com/google/leveldb/blob/master/doc/index.md) is online and bundled with the source code.


# Limitations
  * This is not a SQL database.  It does not have a relational data model, it does not support SQL queries, and it has no support for indexes.
  * Only a single process (possibly multi-threaded) can access a particular database at a time.
  * There is no client-server support builtin to the library.  An application that needs such support will have to wrap their own server around the library.

# Contributing to the leveldb Project
The leveldb project welcomes contributions. leveldb's primary goal is to be
a reliable and fast key/value store. Changes that are in line with the
features/limitations outlined above, and meet the requirements below,
will be considered.

Contribution requirements:

1. **POSIX only**. We _generally_ will only accept changes that are both
   compiled, and tested on a POSIX platform - usually Linux. Very small
   changes will sometimes be accepted, but consider that more of an
   exception than the rule.

2. **Stable API**. We strive very hard to maintain a stable API. Changes that
   require changes for projects using leveldb _might_ be rejected without
   sufficient benefit to the project.

3. **Tests**: All changes must be accompanied by a new (or changed) test, or
   a sufficient explanation as to why a new (or changed) test is not required.

## Submitting a Pull Request
Before any pull request will be accepted the author must first sign a
Contributor License Agreement (CLA) at https://cla.developers.google.com/.

In order to keep the commit timeline linear
[squash](https://git-scm.com/book/en/v2/Git-Tools-Rewriting-History#Squashing-Commits)
your changes down to a single commit and [rebase](https://git-scm.com/docs/git-rebase)
on google/leveldb/master. This keeps the commit timeline linear and more easily sync'ed
with the internal repository at Google. More information at GitHub's
[About Git rebase](https://help.github.com/articles/about-git-rebase/) page.

# Performance

Here is a performance report (with explanations) from the run of the
included db_bench program.  The results are somewhat noisy, but should
be enough to get a ballpark performance estimate.

## Setup

We use a database with a million entries.  Each entry has a 16 byte
key, and a 100 byte value.  Values used by the benchmark compress to
about half their original size.

    LevelDB:    version 1.1
    Date:       Sun May  1 12:11:26 2011
    CPU:        4 x Intel(R) Core(TM)2 Quad CPU    Q6600  @ 2.40GHz
    CPUCache:   4096 KB
    Keys:       16 bytes each
    Values:     100 bytes each (50 bytes after compression)
    Entries:    1000000
    Raw Size:   110.6 MB (estimated)
    File Size:  62.9 MB (estimated)

## Write performance

The "fill" benchmarks create a brand new database, in either
sequential, or random order.  The "fillsync" benchmark flushes data
from the operating system to the disk after every operation; the other
write operations leave the data sitting in the operating system buffer
cache for a while.  The "overwrite" benchmark does random writes that
update existing keys in the database.

    fillseq      :       1.765 micros/op;   62.7 MB/s
    fillsync     :     268.409 micros/op;    0.4 MB/s (10000 ops)
    fillrandom   :       2.460 micros/op;   45.0 MB/s
    overwrite    :       2.380 micros/op;   46.5 MB/s

Each "op" above corresponds to a write of a single key/value pair.
I.e., a random write benchmark goes at approximately 400,000 writes per second.

Each "fillsync" operation costs much less (0.3 millisecond)
than a disk seek (typically 10 milliseconds).  We suspect that this is
because the hard disk itself is buffering the update in its memory and
responding before the data has been written to the platter.  This may
or may not be safe based on whether or not the hard disk has enough
power to save its memory in the event of a power failure.

## Read performance

We list the performance of reading sequentially in both the forward
and reverse direction, and also the performance of a random lookup.
Note that the database created by the benchmark is quite small.
Therefore the report characterizes the performance of leveldb when the
working set fits in memory.  The cost of reading a piece of data that
is not present in the operating system buffer cache will be dominated
by the one or two disk seeks needed to fetch the data from disk.
Write performance will be mostly unaffected by whether or not the
working set fits in memory.

    readrandom  : 16.677 micros/op;  (approximately 60,000 reads per second)
    readseq     :  0.476 micros/op;  232.3 MB/s
    readreverse :  0.724 micros/op;  152.9 MB/s

LevelDB compacts its underlying storage data in the background to
improve read performance.  The results listed above were done
immediately after a lot of random writes.  The results after
compactions (which are usually triggered automatically) are better.

    readrandom  : 11.602 micros/op;  (approximately 85,000 reads per second)
    readseq     :  0.423 micros/op;  261.8 MB/s
    readreverse :  0.663 micros/op;  166.9 MB/s

Some of the high cost of reads comes from repeated decompression of blocks
read from disk.  If we supply enough cache to the leveldb so it can hold the
uncompressed blocks in memory, the read performance improves again:

    readrandom  : 9.775 micros/op;  (approximately 100,000 reads per second before compaction)
    readrandom  : 5.215 micros/op;  (approximately 190,000 reads per second after compaction)

## Repository contents

See [doc/index.md](doc/index.md) for more explanation. See
[doc/impl.md](doc/impl.md) for a brief overview of the implementation.

The public interface is in include/*.h.  Callers should not include or
rely on the details of any other header files in this package.  Those
internal APIs may be changed without warning.

Guide to header files:

* **include/db.h**: Main interface to the DB: Start here

* **include/options.h**: Control over the behavior of an entire database,
and also control over the behavior of individual reads and writes.

* **include/comparator.h**: Abstraction for user-specified comparison function.
If you want just bytewise comparison of keys, you can use the default
comparator, but clients can write their own comparator implementations if they
want custom ordering (e.g. to handle different character encodings, etc.)

* **include/iterator.h**: Interface for iterating over data. You can get
an iterator from a DB object.

* **include/write_batch.h**: Interface for atomically applying multiple
updates to a database.

* **include/slice.h**: A simple module for maintaining a pointer and a
length into some other byte array.

* **include/status.h**: Status is returned from many of the public interfaces
and is used to report success and various kinds of errors.

* **include/env.h**:
Abstraction of the OS environment.  A posix implementation of this interface is
in util/env_posix.cc

* **include/table.h, include/table_builder.h**: Lower-level modules that most
clients probably won't use directly

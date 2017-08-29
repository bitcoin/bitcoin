:man_page: bson_threading

Threading
=========

Libbson's data structures are *NOT* thread-safe. You are responsible for accessing and mutating these structures from one thread at a time.

Libbson requires POSIX threads (pthreads) on all UNIX-like platforms. On Windows, the native threading interface is used. Libbson uses your system's threading library to safely generate unique :doc:`ObjectIds <bson_oid_t>`, and to provide a fallback implementation for atomic operations on platforms without built-in atomics.


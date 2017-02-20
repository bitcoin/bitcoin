/*
** 2001 September 15
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This header file defines the interface that the SQLite library
** presents to client programs.  If a C-function, structure, datatype,
** or constant definition does not appear in this file, then it is
** not a published API of SQLite, is subject to change without
** notice, and should not be referenced by programs that use SQLite.
**
** Some of the definitions that are in this file are marked as
** "experimental".  Experimental interfaces are normally new
** features recently added to SQLite.  We do not anticipate changes 
** to experimental interfaces but reserve to make minor changes if
** experience from use "in the wild" suggest such changes are prudent.
**
** The official C-language API documentation for SQLite is derived
** from comments in this file.  This file is the authoritative source
** on how SQLite interfaces are suppose to operate.
**
** The name of this file under configuration management is "sqlite.h.in".
** The makefile makes some minor changes to this file (such as inserting
** the version number) and changes its name to "sqlite3.h" as
** part of the build process.
**
** @(#) $Id$
*/
#ifndef _SQLITE3_H_
#define _SQLITE3_H_
#include <stdarg.h>     /* Needed for the definition of va_list */

/*
** Make sure we can call this stuff from C++.
*/
#ifdef __cplusplus
extern "C" {
#endif


/*
** Add the ability to override 'extern'
*/
#ifndef SQLITE_EXTERN
# define SQLITE_EXTERN extern
#endif

/*
** Make sure these symbols where not defined by some previous header
** file.
*/
#ifdef SQLITE_VERSION
# undef SQLITE_VERSION
#endif
#ifdef SQLITE_VERSION_NUMBER
# undef SQLITE_VERSION_NUMBER
#endif

/*
** CAPI3REF: Compile-Time Library Version Numbers {F10010}
**
** The SQLITE_VERSION and SQLITE_VERSION_NUMBER #defines in
** the sqlite3.h file specify the version of SQLite with which
** that header file is associated.
**
** The "version" of SQLite is a string of the form "X.Y.Z".
** The phrase "alpha" or "beta" might be appended after the Z.
** The X value is major version number always 3 in SQLite3.
** The X value only changes when  backwards compatibility is
** broken and we intend to never break
** backwards compatibility.  The Y value is the minor version
** number and only changes when
** there are major feature enhancements that are forwards compatible
** but not backwards compatible.  The Z value is release number
** and is incremented with
** each release but resets back to 0 when Y is incremented.
**
** See also: [sqlite3_libversion()] and [sqlite3_libversion_number()].
**
** INVARIANTS:
**
** {F10011} The SQLITE_VERSION #define in the sqlite3.h header file
**          evaluates to a string literal that is the SQLite version
**          with which the header file is associated.
**
** {F10014} The SQLITE_VERSION_NUMBER #define resolves to an integer
**          with the value  (X*1000000 + Y*1000 + Z) where X, Y, and
**          Z are the major version, minor version, and release number.
*/
#define SQLITE_VERSION         "3.5.9"
#define SQLITE_VERSION_NUMBER  3005009

/*
** CAPI3REF: Run-Time Library Version Numbers {F10020}
** KEYWORDS: sqlite3_version
**
** These features provide the same information as the [SQLITE_VERSION]
** and [SQLITE_VERSION_NUMBER] #defines in the header, but are associated
** with the library instead of the header file.  Cautious programmers might
** include a check in their application to verify that 
** sqlite3_libversion_number() always returns the value 
** [SQLITE_VERSION_NUMBER].
**
** The sqlite3_libversion() function returns the same information as is
** in the sqlite3_version[] string constant.  The function is provided
** for use in DLLs since DLL users usually do not have direct access to string
** constants within the DLL.
**
** INVARIANTS:
**
** {F10021} The [sqlite3_libversion_number()] interface returns an integer
**          equal to [SQLITE_VERSION_NUMBER]. 
**
** {F10022} The [sqlite3_version] string constant contains the text of the
**          [SQLITE_VERSION] string. 
**
** {F10023} The [sqlite3_libversion()] function returns
**          a pointer to the [sqlite3_version] string constant.
*/
SQLITE_EXTERN const char sqlite3_version[];
const char *sqlite3_libversion(void);
int sqlite3_libversion_number(void);

/*
** CAPI3REF: Test To See If The Library Is Threadsafe {F10100}
**
** SQLite can be compiled with or without mutexes.  When
** the SQLITE_THREADSAFE C preprocessor macro is true, mutexes
** are enabled and SQLite is threadsafe.  When that macro is false,
** the mutexes are omitted.  Without the mutexes, it is not safe
** to use SQLite from more than one thread.
**
** There is a measurable performance penalty for enabling mutexes.
** So if speed is of utmost importance, it makes sense to disable
** the mutexes.  But for maximum safety, mutexes should be enabled.
** The default behavior is for mutexes to be enabled.
**
** This interface can be used by a program to make sure that the
** version of SQLite that it is linking against was compiled with
** the desired setting of the SQLITE_THREADSAFE macro.
**
** INVARIANTS:
**
** {F10101} The [sqlite3_threadsafe()] function returns nonzero if
**          SQLite was compiled with its mutexes enabled or zero
**          if SQLite was compiled with mutexes disabled.
*/
int sqlite3_threadsafe(void);

/*
** CAPI3REF: Database Connection Handle {F12000}
** KEYWORDS: {database connection} {database connections}
**
** Each open SQLite database is represented by pointer to an instance of the
** opaque structure named "sqlite3".  It is useful to think of an sqlite3
** pointer as an object.  The [sqlite3_open()], [sqlite3_open16()], and
** [sqlite3_open_v2()] interfaces are its constructors
** and [sqlite3_close()] is its destructor.  There are many other interfaces
** (such as [sqlite3_prepare_v2()], [sqlite3_create_function()], and
** [sqlite3_busy_timeout()] to name but three) that are methods on this
** object.
*/
typedef struct sqlite3 sqlite3;


/*
** CAPI3REF: 64-Bit Integer Types {F10200}
** KEYWORDS: sqlite_int64 sqlite_uint64
**
** Because there is no cross-platform way to specify 64-bit integer types
** SQLite includes typedefs for 64-bit signed and unsigned integers.
**
** The sqlite3_int64 and sqlite3_uint64 are the preferred type
** definitions.  The sqlite_int64 and sqlite_uint64 types are
** supported for backwards compatibility only.
**
** INVARIANTS:
**
** {F10201} The [sqlite_int64] and [sqlite3_int64] types specify a
**          64-bit signed integer.
**
** {F10202} The [sqlite_uint64] and [sqlite3_uint64] types specify
**          a 64-bit unsigned integer.
*/
#ifdef SQLITE_INT64_TYPE
  typedef SQLITE_INT64_TYPE sqlite_int64;
  typedef unsigned SQLITE_INT64_TYPE sqlite_uint64;
#elif defined(_MSC_VER) || defined(__BORLANDC__)
  typedef __int64 sqlite_int64;
  typedef unsigned __int64 sqlite_uint64;
#else
  typedef long long int sqlite_int64;
  typedef unsigned long long int sqlite_uint64;
#endif
typedef sqlite_int64 sqlite3_int64;
typedef sqlite_uint64 sqlite3_uint64;

/*
** If compiling for a processor that lacks floating point support,
** substitute integer for floating-point
*/
#ifdef SQLITE_OMIT_FLOATING_POINT
# define double sqlite3_int64
#endif

/*
** CAPI3REF: Closing A Database Connection {F12010}
**
** This routine is the destructor for the [sqlite3] object.  
**
** Applications should [sqlite3_finalize | finalize] all
** [prepared statements] and
** [sqlite3_blob_close | close] all [sqlite3_blob | BLOBs] 
** associated with the [sqlite3] object prior
** to attempting to close the [sqlite3] object.
**
** <todo>What happens to pending transactions?  Are they
** rolled back, or abandoned?</todo>
**
** INVARIANTS:
**
** {F12011} The [sqlite3_close()] interface destroys an [sqlite3] object
**          allocated by a prior call to [sqlite3_open()],
**          [sqlite3_open16()], or [sqlite3_open_v2()].
**
** {F12012} The [sqlite3_close()] function releases all memory used by the
**          connection and closes all open files.
**
** {F12013} If the database connection contains
**          [prepared statements] that have not been
**          finalized by [sqlite3_finalize()], then [sqlite3_close()]
**          returns [SQLITE_BUSY] and leaves the connection open.
**
** {F12014} Giving sqlite3_close() a NULL pointer is a harmless no-op.
**
** LIMITATIONS:
**
** {U12015} The parameter to [sqlite3_close()] must be an [sqlite3] object
**          pointer previously obtained from [sqlite3_open()] or the 
**          equivalent, or NULL.
**
** {U12016} The parameter to [sqlite3_close()] must not have been previously
**          closed.
*/
int sqlite3_close(sqlite3 *);

/*
** The type for a callback function.
** This is legacy and deprecated.  It is included for historical
** compatibility and is not documented.
*/
typedef int (*sqlite3_callback)(void*,int,char**, char**);

/*
** CAPI3REF: One-Step Query Execution Interface {F12100}
**
** The sqlite3_exec() interface is a convenient way of running
** one or more SQL statements without a lot of C code.  The
** SQL statements are passed in as the second parameter to
** sqlite3_exec().  The statements are evaluated one by one
** until either an error or an interrupt is encountered or
** until they are all done.  The 3rd parameter is an optional
** callback that is invoked once for each row of any query results
** produced by the SQL statements.  The 5th parameter tells where
** to write any error messages.
**
** The sqlite3_exec() interface is implemented in terms of
** [sqlite3_prepare_v2()], [sqlite3_step()], and [sqlite3_finalize()].
** The sqlite3_exec() routine does nothing that cannot be done
** by [sqlite3_prepare_v2()], [sqlite3_step()], and [sqlite3_finalize()].
** The sqlite3_exec() is just a convenient wrapper.
**
** INVARIANTS:
** 
** {F12101} The [sqlite3_exec()] interface evaluates zero or more UTF-8
**          encoded, semicolon-separated, SQL statements in the
**          zero-terminated string of its 2nd parameter within the
**          context of the [sqlite3] object given in the 1st parameter.
**
** {F12104} The return value of [sqlite3_exec()] is SQLITE_OK if all
**          SQL statements run successfully.
**
** {F12105} The return value of [sqlite3_exec()] is an appropriate 
**          non-zero error code if any SQL statement fails.
**
** {F12107} If one or more of the SQL statements handed to [sqlite3_exec()]
**          return results and the 3rd parameter is not NULL, then
**          the callback function specified by the 3rd parameter is
**          invoked once for each row of result.
**
** {F12110} If the callback returns a non-zero value then [sqlite3_exec()]
**          will aborted the SQL statement it is currently evaluating,
**          skip all subsequent SQL statements, and return [SQLITE_ABORT].
**          <todo>What happens to *errmsg here?  Does the result code for
**          sqlite3_errcode() get set?</todo>
**
** {F12113} The [sqlite3_exec()] routine will pass its 4th parameter through
**          as the 1st parameter of the callback.
**
** {F12116} The [sqlite3_exec()] routine sets the 2nd parameter of its
**          callback to be the number of columns in the current row of
**          result.
**
** {F12119} The [sqlite3_exec()] routine sets the 3rd parameter of its 
**          callback to be an array of pointers to strings holding the
**          values for each column in the current result set row as
**          obtained from [sqlite3_column_text()].
**
** {F12122} The [sqlite3_exec()] routine sets the 4th parameter of its
**          callback to be an array of pointers to strings holding the
**          names of result columns as obtained from [sqlite3_column_name()].
**
** {F12125} If the 3rd parameter to [sqlite3_exec()] is NULL then
**          [sqlite3_exec()] never invokes a callback.  All query
**          results are silently discarded.
**
** {F12128} If an error occurs while parsing or evaluating any of the SQL
**          statements handed to [sqlite3_exec()] then [sqlite3_exec()] will
**          return an [error code] other than [SQLITE_OK].
**
** {F12131} If an error occurs while parsing or evaluating any of the SQL
**          handed to [sqlite3_exec()] and if the 5th parameter (errmsg)
**          to [sqlite3_exec()] is not NULL, then an error message is
**          allocated using the equivalent of [sqlite3_mprintf()] and
**          *errmsg is made to point to that message.
**
** {F12134} The [sqlite3_exec()] routine does not change the value of
**          *errmsg if errmsg is NULL or if there are no errors.
**
** {F12137} The [sqlite3_exec()] function sets the error code and message
**          accessible via [sqlite3_errcode()], [sqlite3_errmsg()], and
**          [sqlite3_errmsg16()].
**
** LIMITATIONS:
**
** {U12141} The first parameter to [sqlite3_exec()] must be an valid and open
**          [database connection].
**
** {U12142} The database connection must not be closed while
**          [sqlite3_exec()] is running.
** 
** {U12143} The calling function is should use [sqlite3_free()] to free
**          the memory that *errmsg is left pointing at once the error
**          message is no longer needed.
**
** {U12145} The SQL statement text in the 2nd parameter to [sqlite3_exec()]
**          must remain unchanged while [sqlite3_exec()] is running.
*/
int sqlite3_exec(
  sqlite3*,                                  /* An open database */
  const char *sql,                           /* SQL to be evaluted */
  int (*callback)(void*,int,char**,char**),  /* Callback function */
  void *,                                    /* 1st argument to callback */
  char **errmsg                              /* Error msg written here */
);

/*
** CAPI3REF: Result Codes {F10210}
** KEYWORDS: SQLITE_OK {error code} {error codes}
**
** Many SQLite functions return an integer result code from the set shown
** here in order to indicates success or failure.
**
** See also: [SQLITE_IOERR_READ | extended result codes]
*/
#define SQLITE_OK           0   /* Successful result */
/* beginning-of-error-codes */
#define SQLITE_ERROR        1   /* SQL error or missing database */
#define SQLITE_INTERNAL     2   /* Internal logic error in SQLite */
#define SQLITE_PERM         3   /* Access permission denied */
#define SQLITE_ABORT        4   /* Callback routine requested an abort */
#define SQLITE_BUSY         5   /* The database file is locked */
#define SQLITE_LOCKED       6   /* A table in the database is locked */
#define SQLITE_NOMEM        7   /* A malloc() failed */
#define SQLITE_READONLY     8   /* Attempt to write a readonly database */
#define SQLITE_INTERRUPT    9   /* Operation terminated by sqlite3_interrupt()*/
#define SQLITE_IOERR       10   /* Some kind of disk I/O error occurred */
#define SQLITE_CORRUPT     11   /* The database disk image is malformed */
#define SQLITE_NOTFOUND    12   /* NOT USED. Table or record not found */
#define SQLITE_FULL        13   /* Insertion failed because database is full */
#define SQLITE_CANTOPEN    14   /* Unable to open the database file */
#define SQLITE_PROTOCOL    15   /* NOT USED. Database lock protocol error */
#define SQLITE_EMPTY       16   /* Database is empty */
#define SQLITE_SCHEMA      17   /* The database schema changed */
#define SQLITE_TOOBIG      18   /* String or BLOB exceeds size limit */
#define SQLITE_CONSTRAINT  19   /* Abort due to constraint violation */
#define SQLITE_MISMATCH    20   /* Data type mismatch */
#define SQLITE_MISUSE      21   /* Library used incorrectly */
#define SQLITE_NOLFS       22   /* Uses OS features not supported on host */
#define SQLITE_AUTH        23   /* Authorization denied */
#define SQLITE_FORMAT      24   /* Auxiliary database format error */
#define SQLITE_RANGE       25   /* 2nd parameter to sqlite3_bind out of range */
#define SQLITE_NOTADB      26   /* File opened that is not a database file */
#define SQLITE_ROW         100  /* sqlite3_step() has another row ready */
#define SQLITE_DONE        101  /* sqlite3_step() has finished executing */
/* end-of-error-codes */

/*
** CAPI3REF: Extended Result Codes {F10220}
** KEYWORDS: {extended error code} {extended error codes}
** KEYWORDS: {extended result codes}
**
** In its default configuration, SQLite API routines return one of 26 integer
** [SQLITE_OK | result codes].  However, experience has shown that
** many of these result codes are too course-grained.  They do not provide as
** much information about problems as programmers might like.  In an effort to
** address this, newer versions of SQLite (version 3.3.8 and later) include
** support for additional result codes that provide more detailed information
** about errors. The extended result codes are enabled or disabled
** for each database connection using the [sqlite3_extended_result_codes()]
** API.
** 
** Some of the available extended result codes are listed here.
** One may expect the number of extended result codes will be expand
** over time.  Software that uses extended result codes should expect
** to see new result codes in future releases of SQLite.
**
** The SQLITE_OK result code will never be extended.  It will always
** be exactly zero.
** 
** INVARIANTS:
**
** {F10223} The symbolic name for an extended result code always contains
**          a related primary result code as a prefix.
**
** {F10224} Primary result code names contain a single "_" character.
**
** {F10225} Extended result code names contain two or more "_" characters.
**
** {F10226} The numeric value of an extended result code contains the
**          numeric value of its corresponding primary result code in
**          its least significant 8 bits.
*/
#define SQLITE_IOERR_READ          (SQLITE_IOERR | (1<<8))
#define SQLITE_IOERR_SHORT_READ    (SQLITE_IOERR | (2<<8))
#define SQLITE_IOERR_WRITE         (SQLITE_IOERR | (3<<8))
#define SQLITE_IOERR_FSYNC         (SQLITE_IOERR | (4<<8))
#define SQLITE_IOERR_DIR_FSYNC     (SQLITE_IOERR | (5<<8))
#define SQLITE_IOERR_TRUNCATE      (SQLITE_IOERR | (6<<8))
#define SQLITE_IOERR_FSTAT         (SQLITE_IOERR | (7<<8))
#define SQLITE_IOERR_UNLOCK        (SQLITE_IOERR | (8<<8))
#define SQLITE_IOERR_RDLOCK        (SQLITE_IOERR | (9<<8))
#define SQLITE_IOERR_DELETE        (SQLITE_IOERR | (10<<8))
#define SQLITE_IOERR_BLOCKED       (SQLITE_IOERR | (11<<8))
#define SQLITE_IOERR_NOMEM         (SQLITE_IOERR | (12<<8))

/*
** CAPI3REF: Flags For File Open Operations {F10230}
**
** These bit values are intended for use in the
** 3rd parameter to the [sqlite3_open_v2()] interface and
** in the 4th parameter to the xOpen method of the
** [sqlite3_vfs] object.
*/
#define SQLITE_OPEN_READONLY         0x00000001
#define SQLITE_OPEN_READWRITE        0x00000002
#define SQLITE_OPEN_CREATE           0x00000004
#define SQLITE_OPEN_DELETEONCLOSE    0x00000008
#define SQLITE_OPEN_EXCLUSIVE        0x00000010
#define SQLITE_OPEN_MAIN_DB          0x00000100
#define SQLITE_OPEN_TEMP_DB          0x00000200
#define SQLITE_OPEN_TRANSIENT_DB     0x00000400
#define SQLITE_OPEN_MAIN_JOURNAL     0x00000800
#define SQLITE_OPEN_TEMP_JOURNAL     0x00001000
#define SQLITE_OPEN_SUBJOURNAL       0x00002000
#define SQLITE_OPEN_MASTER_JOURNAL   0x00004000

/*
** CAPI3REF: Device Characteristics {F10240}
**
** The xDeviceCapabilities method of the [sqlite3_io_methods]
** object returns an integer which is a vector of the these
** bit values expressing I/O characteristics of the mass storage
** device that holds the file that the [sqlite3_io_methods]
** refers to.
**
** The SQLITE_IOCAP_ATOMIC property means that all writes of
** any size are atomic.  The SQLITE_IOCAP_ATOMICnnn values
** mean that writes of blocks that are nnn bytes in size and
** are aligned to an address which is an integer multiple of
** nnn are atomic.  The SQLITE_IOCAP_SAFE_APPEND value means
** that when data is appended to a file, the data is appended
** first then the size of the file is extended, never the other
** way around.  The SQLITE_IOCAP_SEQUENTIAL property means that
** information is written to disk in the same order as calls
** to xWrite().
*/
#define SQLITE_IOCAP_ATOMIC          0x00000001
#define SQLITE_IOCAP_ATOMIC512       0x00000002
#define SQLITE_IOCAP_ATOMIC1K        0x00000004
#define SQLITE_IOCAP_ATOMIC2K        0x00000008
#define SQLITE_IOCAP_ATOMIC4K        0x00000010
#define SQLITE_IOCAP_ATOMIC8K        0x00000020
#define SQLITE_IOCAP_ATOMIC16K       0x00000040
#define SQLITE_IOCAP_ATOMIC32K       0x00000080
#define SQLITE_IOCAP_ATOMIC64K       0x00000100
#define SQLITE_IOCAP_SAFE_APPEND     0x00000200
#define SQLITE_IOCAP_SEQUENTIAL      0x00000400

/*
** CAPI3REF: File Locking Levels {F10250}
**
** SQLite uses one of these integer values as the second
** argument to calls it makes to the xLock() and xUnlock() methods
** of an [sqlite3_io_methods] object.
*/
#define SQLITE_LOCK_NONE          0
#define SQLITE_LOCK_SHARED        1
#define SQLITE_LOCK_RESERVED      2
#define SQLITE_LOCK_PENDING       3
#define SQLITE_LOCK_EXCLUSIVE     4

/*
** CAPI3REF: Synchronization Type Flags {F10260}
**
** When SQLite invokes the xSync() method of an
** [sqlite3_io_methods] object it uses a combination of
** these integer values as the second argument.
**
** When the SQLITE_SYNC_DATAONLY flag is used, it means that the
** sync operation only needs to flush data to mass storage.  Inode
** information need not be flushed. The SQLITE_SYNC_NORMAL flag means 
** to use normal fsync() semantics. The SQLITE_SYNC_FULL flag means 
** to use Mac OS-X style fullsync instead of fsync().
*/
#define SQLITE_SYNC_NORMAL        0x00002
#define SQLITE_SYNC_FULL          0x00003
#define SQLITE_SYNC_DATAONLY      0x00010


/*
** CAPI3REF: OS Interface Open File Handle {F11110}
**
** An [sqlite3_file] object represents an open file in the OS
** interface layer.  Individual OS interface implementations will
** want to subclass this object by appending additional fields
** for their own use.  The pMethods entry is a pointer to an
** [sqlite3_io_methods] object that defines methods for performing
** I/O operations on the open file.
*/
typedef struct sqlite3_file sqlite3_file;
struct sqlite3_file {
  const struct sqlite3_io_methods *pMethods;  /* Methods for an open file */
};

/*
** CAPI3REF: OS Interface File Virtual Methods Object {F11120}
**
** Every file opened by the [sqlite3_vfs] xOpen method contains a pointer to
** an instance of this object.  This object defines the
** methods used to perform various operations against the open file.
**
** The flags argument to xSync may be one of [SQLITE_SYNC_NORMAL] or
** [SQLITE_SYNC_FULL].  The first choice is the normal fsync().
*  The second choice is an
** OS-X style fullsync.  The SQLITE_SYNC_DATA flag may be ORed in to
** indicate that only the data of the file and not its inode needs to be
** synced.
** 
** The integer values to xLock() and xUnlock() are one of
** <ul>
** <li> [SQLITE_LOCK_NONE],
** <li> [SQLITE_LOCK_SHARED],
** <li> [SQLITE_LOCK_RESERVED],
** <li> [SQLITE_LOCK_PENDING], or
** <li> [SQLITE_LOCK_EXCLUSIVE].
** </ul>
** xLock() increases the lock. xUnlock() decreases the lock.  
** The xCheckReservedLock() method looks
** to see if any database connection, either in this
** process or in some other process, is holding an RESERVED,
** PENDING, or EXCLUSIVE lock on the file.  It returns true
** if such a lock exists and false if not.
** 
** The xFileControl() method is a generic interface that allows custom
** VFS implementations to directly control an open file using the
** [sqlite3_file_control()] interface.  The second "op" argument
** is an integer opcode.   The third
** argument is a generic pointer which is intended to be a pointer
** to a structure that may contain arguments or space in which to
** write return values.  Potential uses for xFileControl() might be
** functions to enable blocking locks with timeouts, to change the
** locking strategy (for example to use dot-file locks), to inquire
** about the status of a lock, or to break stale locks.  The SQLite
** core reserves opcodes less than 100 for its own use. 
** A [SQLITE_FCNTL_LOCKSTATE | list of opcodes] less than 100 is available.
** Applications that define a custom xFileControl method should use opcodes 
** greater than 100 to avoid conflicts.
**
** The xSectorSize() method returns the sector size of the
** device that underlies the file.  The sector size is the
** minimum write that can be performed without disturbing
** other bytes in the file.  The xDeviceCharacteristics()
** method returns a bit vector describing behaviors of the
** underlying device:
**
** <ul>
** <li> [SQLITE_IOCAP_ATOMIC]
** <li> [SQLITE_IOCAP_ATOMIC512]
** <li> [SQLITE_IOCAP_ATOMIC1K]
** <li> [SQLITE_IOCAP_ATOMIC2K]
** <li> [SQLITE_IOCAP_ATOMIC4K]
** <li> [SQLITE_IOCAP_ATOMIC8K]
** <li> [SQLITE_IOCAP_ATOMIC16K]
** <li> [SQLITE_IOCAP_ATOMIC32K]
** <li> [SQLITE_IOCAP_ATOMIC64K]
** <li> [SQLITE_IOCAP_SAFE_APPEND]
** <li> [SQLITE_IOCAP_SEQUENTIAL]
** </ul>
**
** The SQLITE_IOCAP_ATOMIC property means that all writes of
** any size are atomic.  The SQLITE_IOCAP_ATOMICnnn values
** mean that writes of blocks that are nnn bytes in size and
** are aligned to an address which is an integer multiple of
** nnn are atomic.  The SQLITE_IOCAP_SAFE_APPEND value means
** that when data is appended to a file, the data is appended
** first then the size of the file is extended, never the other
** way around.  The SQLITE_IOCAP_SEQUENTIAL property means that
** information is written to disk in the same order as calls
** to xWrite().
*/
typedef struct sqlite3_io_methods sqlite3_io_methods;
struct sqlite3_io_methods {
  int iVersion;
  int (*xClose)(sqlite3_file*);
  int (*xRead)(sqlite3_file*, void*, int iAmt, sqlite3_int64 iOfst);
  int (*xWrite)(sqlite3_file*, const void*, int iAmt, sqlite3_int64 iOfst);
  int (*xTruncate)(sqlite3_file*, sqlite3_int64 size);
  int (*xSync)(sqlite3_file*, int flags);
  int (*xFileSize)(sqlite3_file*, sqlite3_int64 *pSize);
  int (*xLock)(sqlite3_file*, int);
  int (*xUnlock)(sqlite3_file*, int);
  int (*xCheckReservedLock)(sqlite3_file*);
  int (*xFileControl)(sqlite3_file*, int op, void *pArg);
  int (*xSectorSize)(sqlite3_file*);
  int (*xDeviceCharacteristics)(sqlite3_file*);
  /* Additional methods may be added in future releases */
};

/*
** CAPI3REF: Standard File Control Opcodes {F11310}
**
** These integer constants are opcodes for the xFileControl method
** of the [sqlite3_io_methods] object and to the [sqlite3_file_control()]
** interface.
**
** The [SQLITE_FCNTL_LOCKSTATE] opcode is used for debugging.  This
** opcode causes the xFileControl method to write the current state of
** the lock (one of [SQLITE_LOCK_NONE], [SQLITE_LOCK_SHARED],
** [SQLITE_LOCK_RESERVED], [SQLITE_LOCK_PENDING], or [SQLITE_LOCK_EXCLUSIVE])
** into an integer that the pArg argument points to. This capability
** is used during testing and only needs to be supported when SQLITE_TEST
** is defined.
*/
#define SQLITE_FCNTL_LOCKSTATE        1

/*
** CAPI3REF: Mutex Handle {F17110}
**
** The mutex module within SQLite defines [sqlite3_mutex] to be an
** abstract type for a mutex object.  The SQLite core never looks
** at the internal representation of an [sqlite3_mutex].  It only
** deals with pointers to the [sqlite3_mutex] object.
**
** Mutexes are created using [sqlite3_mutex_alloc()].
*/
typedef struct sqlite3_mutex sqlite3_mutex;

/*
** CAPI3REF: OS Interface Object {F11140}
**
** An instance of this object defines the interface between the
** SQLite core and the underlying operating system.  The "vfs"
** in the name of the object stands for "virtual file system".
**
** The iVersion field is initially 1 but may be larger for future
** versions of SQLite.  Additional fields may be appended to this
** object when the iVersion value is increased.
**
** The szOsFile field is the size of the subclassed [sqlite3_file]
** structure used by this VFS.  mxPathname is the maximum length of
** a pathname in this VFS.
**
** Registered sqlite3_vfs objects are kept on a linked list formed by
** the pNext pointer.  The [sqlite3_vfs_register()]
** and [sqlite3_vfs_unregister()] interfaces manage this list
** in a thread-safe way.  The [sqlite3_vfs_find()] interface
** searches the list.
**
** The pNext field is the only field in the sqlite3_vfs 
** structure that SQLite will ever modify.  SQLite will only access
** or modify this field while holding a particular static mutex.
** The application should never modify anything within the sqlite3_vfs
** object once the object has been registered.
**
** The zName field holds the name of the VFS module.  The name must
** be unique across all VFS modules.
**
** {F11141} SQLite will guarantee that the zFilename string passed to
** xOpen() is a full pathname as generated by xFullPathname() and
** that the string will be valid and unchanged until xClose() is
** called.  {END} So the [sqlite3_file] can store a pointer to the
** filename if it needs to remember the filename for some reason.
**
** {F11142} The flags argument to xOpen() includes all bits set in
** the flags argument to [sqlite3_open_v2()].  Or if [sqlite3_open()]
** or [sqlite3_open16()] is used, then flags includes at least
** [SQLITE_OPEN_READWRITE] | [SQLITE_OPEN_CREATE]. {END}
** If xOpen() opens a file read-only then it sets *pOutFlags to
** include [SQLITE_OPEN_READONLY].  Other bits in *pOutFlags may be
** set.
** 
** {F11143} SQLite will also add one of the following flags to the xOpen()
** call, depending on the object being opened:
** 
** <ul>
** <li>  [SQLITE_OPEN_MAIN_DB]
** <li>  [SQLITE_OPEN_MAIN_JOURNAL]
** <li>  [SQLITE_OPEN_TEMP_DB]
** <li>  [SQLITE_OPEN_TEMP_JOURNAL]
** <li>  [SQLITE_OPEN_TRANSIENT_DB]
** <li>  [SQLITE_OPEN_SUBJOURNAL]
** <li>  [SQLITE_OPEN_MASTER_JOURNAL]
** </ul> {END}
**
** The file I/O implementation can use the object type flags to
** changes the way it deals with files.  For example, an application
** that does not care about crash recovery or rollback might make
** the open of a journal file a no-op.  Writes to this journal would
** also be no-ops, and any attempt to read the journal would return 
** SQLITE_IOERR.  Or the implementation might recognize that a database 
** file will be doing page-aligned sector reads and writes in a random 
** order and set up its I/O subsystem accordingly.
** 
** SQLite might also add one of the following flags to the xOpen
** method:
** 
** <ul>
** <li> [SQLITE_OPEN_DELETEONCLOSE]
** <li> [SQLITE_OPEN_EXCLUSIVE]
** </ul>
** 
** {F11145} The [SQLITE_OPEN_DELETEONCLOSE] flag means the file should be
** deleted when it is closed.  {F11146} The [SQLITE_OPEN_DELETEONCLOSE]
** will be set for TEMP  databases, journals and for subjournals. 
** {F11147} The [SQLITE_OPEN_EXCLUSIVE] flag means the file should be opened
** for exclusive access.  This flag is set for all files except
** for the main database file. {END}
** 
** {F11148} At least szOsFile bytes of memory are allocated by SQLite 
** to hold the  [sqlite3_file] structure passed as the third 
** argument to xOpen.  {END}  The xOpen method does not have to
** allocate the structure; it should just fill it in.
** 
** {F11149} The flags argument to xAccess() may be [SQLITE_ACCESS_EXISTS] 
** to test for the existance of a file,
** or [SQLITE_ACCESS_READWRITE] to test to see
** if a file is readable and writable, or [SQLITE_ACCESS_READ]
** to test to see if a file is at least readable.  {END} The file can be a 
** directory.
** 
** {F11150} SQLite will always allocate at least mxPathname+1 bytes for
** the output buffers for xGetTempname and xFullPathname. {F11151} The exact
** size of the output buffer is also passed as a parameter to both 
** methods. {END} If the output buffer is not large enough, SQLITE_CANTOPEN
** should be returned. As this is handled as a fatal error by SQLite,
** vfs implementations should endeavor to prevent this by setting 
** mxPathname to a sufficiently large value.
** 
** The xRandomness(), xSleep(), and xCurrentTime() interfaces
** are not strictly a part of the filesystem, but they are
** included in the VFS structure for completeness.
** The xRandomness() function attempts to return nBytes bytes
** of good-quality randomness into zOut.  The return value is
** the actual number of bytes of randomness obtained.  The
** xSleep() method causes the calling thread to sleep for at
** least the number of microseconds given.  The xCurrentTime()
** method returns a Julian Day Number for the current date and
** time.
*/
typedef struct sqlite3_vfs sqlite3_vfs;
struct sqlite3_vfs {
  int iVersion;            /* Structure version number */
  int szOsFile;            /* Size of subclassed sqlite3_file */
  int mxPathname;          /* Maximum file pathname length */
  sqlite3_vfs *pNext;      /* Next registered VFS */
  const char *zName;       /* Name of this virtual file system */
  void *pAppData;          /* Pointer to application-specific data */
  int (*xOpen)(sqlite3_vfs*, const char *zName, sqlite3_file*,
               int flags, int *pOutFlags);
  int (*xDelete)(sqlite3_vfs*, const char *zName, int syncDir);
  int (*xAccess)(sqlite3_vfs*, const char *zName, int flags);
  int (*xGetTempname)(sqlite3_vfs*, int nOut, char *zOut);
  int (*xFullPathname)(sqlite3_vfs*, const char *zName, int nOut, char *zOut);
  void *(*xDlOpen)(sqlite3_vfs*, const char *zFilename);
  void (*xDlError)(sqlite3_vfs*, int nByte, char *zErrMsg);
  void *(*xDlSym)(sqlite3_vfs*,void*, const char *zSymbol);
  void (*xDlClose)(sqlite3_vfs*, void*);
  int (*xRandomness)(sqlite3_vfs*, int nByte, char *zOut);
  int (*xSleep)(sqlite3_vfs*, int microseconds);
  int (*xCurrentTime)(sqlite3_vfs*, double*);
  /* New fields may be appended in figure versions.  The iVersion
  ** value will increment whenever this happens. */
};

/*
** CAPI3REF: Flags for the xAccess VFS method {F11190}
**
** {F11191} These integer constants can be used as the third parameter to
** the xAccess method of an [sqlite3_vfs] object. {END}  They determine
** what kind of permissions the xAccess method is
** looking for.  {F11192} With SQLITE_ACCESS_EXISTS, the xAccess method
** simply checks to see if the file exists. {F11193} With
** SQLITE_ACCESS_READWRITE, the xAccess method checks to see
** if the file is both readable and writable.  {F11194} With
** SQLITE_ACCESS_READ the xAccess method
** checks to see if the file is readable.
*/
#define SQLITE_ACCESS_EXISTS    0
#define SQLITE_ACCESS_READWRITE 1
#define SQLITE_ACCESS_READ      2

/*
** CAPI3REF: Enable Or Disable Extended Result Codes {F12200}
**
** The sqlite3_extended_result_codes() routine enables or disables the
** [SQLITE_IOERR_READ | extended result codes] feature of SQLite.
** The extended result codes are disabled by default for historical
** compatibility.
**
** INVARIANTS:
**
** {F12201} Each new [database connection] has the 
**          [extended result codes] feature
**          disabled by default.
**
** {F12202} The [sqlite3_extended_result_codes(D,F)] interface will enable
**          [extended result codes] for the 
**          [database connection] D if the F parameter
**          is true, or disable them if F is false.
*/
int sqlite3_extended_result_codes(sqlite3*, int onoff);

/*
** CAPI3REF: Last Insert Rowid {F12220}
**
** Each entry in an SQLite table has a unique 64-bit signed
** integer key called the "rowid". The rowid is always available
** as an undeclared column named ROWID, OID, or _ROWID_ as long as those
** names are not also used by explicitly declared columns. If
** the table has a column of type INTEGER PRIMARY KEY then that column
** is another alias for the rowid.
**
** This routine returns the rowid of the most recent
** successful INSERT into the database from the database connection
** shown in the first argument.  If no successful inserts
** have ever occurred on this database connection, zero is returned.
**
** If an INSERT occurs within a trigger, then the rowid of the
** inserted row is returned by this routine as long as the trigger
** is running.  But once the trigger terminates, the value returned
** by this routine reverts to the last value inserted before the
** trigger fired.
**
** An INSERT that fails due to a constraint violation is not a
** successful insert and does not change the value returned by this
** routine.  Thus INSERT OR FAIL, INSERT OR IGNORE, INSERT OR ROLLBACK,
** and INSERT OR ABORT make no changes to the return value of this
** routine when their insertion fails.  When INSERT OR REPLACE 
** encounters a constraint violation, it does not fail.  The
** INSERT continues to completion after deleting rows that caused
** the constraint problem so INSERT OR REPLACE will always change
** the return value of this interface. 
**
** For the purposes of this routine, an insert is considered to
** be successful even if it is subsequently rolled back.
**
** INVARIANTS:
**
** {F12221} The [sqlite3_last_insert_rowid()] function returns the
**          rowid of the most recent successful insert done
**          on the same database connection and within the same
**          trigger context, or zero if there have
**          been no qualifying inserts on that connection.
**
** {F12223} The [sqlite3_last_insert_rowid()] function returns
**          same value when called from the same trigger context
**          immediately before and after a ROLLBACK.
**
** LIMITATIONS:
**
** {U12232} If a separate thread does a new insert on the same
**          database connection while the [sqlite3_last_insert_rowid()]
**          function is running and thus changes the last insert rowid,
**          then the value returned by [sqlite3_last_insert_rowid()] is
**          unpredictable and might not equal either the old or the new
**          last insert rowid.
*/
sqlite3_int64 sqlite3_last_insert_rowid(sqlite3*);

/*
** CAPI3REF: Count The Number Of Rows Modified {F12240}
**
** This function returns the number of database rows that were changed
** or inserted or deleted by the most recently completed SQL statement
** on the connection specified by the first parameter.  Only
** changes that are directly specified by the INSERT, UPDATE, or
** DELETE statement are counted.  Auxiliary changes caused by
** triggers are not counted. Use the [sqlite3_total_changes()] function
** to find the total number of changes including changes caused by triggers.
**
** A "row change" is a change to a single row of a single table
** caused by an INSERT, DELETE, or UPDATE statement.  Rows that
** are changed as side effects of REPLACE constraint resolution,
** rollback, ABORT processing, DROP TABLE, or by any other
** mechanisms do not count as direct row changes.
**
** A "trigger context" is a scope of execution that begins and
** ends with the script of a trigger.  Most SQL statements are
** evaluated outside of any trigger.  This is the "top level"
** trigger context.  If a trigger fires from the top level, a
** new trigger context is entered for the duration of that one
** trigger.  Subtriggers create subcontexts for their duration.
**
** Calling [sqlite3_exec()] or [sqlite3_step()] recursively does
** not create a new trigger context.
**
** This function returns the number of direct row changes in the
** most recent INSERT, UPDATE, or DELETE statement within the same
** trigger context.
**
** So when called from the top level, this function returns the
** number of changes in the most recent INSERT, UPDATE, or DELETE
** that also occurred at the top level.
** Within the body of a trigger, the sqlite3_changes() interface
** can be called to find the number of
** changes in the most recently completed INSERT, UPDATE, or DELETE
** statement within the body of the same trigger.
** However, the number returned does not include in changes
** caused by subtriggers since they have their own context.
**
** SQLite implements the command "DELETE FROM table" without
** a WHERE clause by dropping and recreating the table.  (This is much
** faster than going through and deleting individual elements from the
** table.)  Because of this optimization, the deletions in
** "DELETE FROM table" are not row changes and will not be counted
** by the sqlite3_changes() or [sqlite3_total_changes()] functions.
** To get an accurate count of the number of rows deleted, use
** "DELETE FROM table WHERE 1" instead.
**
** INVARIANTS:
**
** {F12241} The [sqlite3_changes()] function returns the number of
**          row changes caused by the most recent INSERT, UPDATE,
**          or DELETE statement on the same database connection and
**          within the same trigger context, or zero if there have
**          not been any qualifying row changes.
**
** LIMITATIONS:
**
** {U12252} If a separate thread makes changes on the same database connection
**          while [sqlite3_changes()] is running then the value returned
**          is unpredictable and unmeaningful.
*/
int sqlite3_changes(sqlite3*);

/*
** CAPI3REF: Total Number Of Rows Modified {F12260}
***
** This function returns the number of row changes caused
** by INSERT, UPDATE or DELETE statements since the database handle
** was opened.  The count includes all changes from all trigger
** contexts.  But the count does not include changes used to
** implement REPLACE constraints, do rollbacks or ABORT processing,
** or DROP table processing.
** The changes
** are counted as soon as the statement that makes them is completed 
** (when the statement handle is passed to [sqlite3_reset()] or 
** [sqlite3_finalize()]).
**
** SQLite implements the command "DELETE FROM table" without
** a WHERE clause by dropping and recreating the table.  (This is much
** faster than going
** through and deleting individual elements from the table.)  Because of
** this optimization, the change count for "DELETE FROM table" will be
** zero regardless of the number of elements that were originally in the
** table. To get an accurate count of the number of rows deleted, use
** "DELETE FROM table WHERE 1" instead.
**
** See also the [sqlite3_changes()] interface.
**
** INVARIANTS:
** 
** {F12261} The [sqlite3_total_changes()] returns the total number
**          of row changes caused by INSERT, UPDATE, and/or DELETE
**          statements on the same [database connection], in any
**          trigger context, since the database connection was
**          created.
**
** LIMITATIONS:
**
** {U12264} If a separate thread makes changes on the same database connection
**          while [sqlite3_total_changes()] is running then the value 
**          returned is unpredictable and unmeaningful.
*/
int sqlite3_total_changes(sqlite3*);

/*
** CAPI3REF: Interrupt A Long-Running Query {F12270}
**
** This function causes any pending database operation to abort and
** return at its earliest opportunity. This routine is typically
** called in response to a user action such as pressing "Cancel"
** or Ctrl-C where the user wants a long query operation to halt
** immediately.
**
** It is safe to call this routine from a thread different from the
** thread that is currently running the database operation.  But it
** is not safe to call this routine with a database connection that
** is closed or might close before sqlite3_interrupt() returns.
**
** If an SQL is very nearly finished at the time when sqlite3_interrupt()
** is called, then it might not have an opportunity to be interrupted.
** It might continue to completion.
** An SQL operation that is interrupted will return
** [SQLITE_INTERRUPT].  If the interrupted SQL operation is an
** INSERT, UPDATE, or DELETE that is inside an explicit transaction, 
** then the entire transaction will be rolled back automatically.
** A call to sqlite3_interrupt() has no effect on SQL statements
** that are started after sqlite3_interrupt() returns.
**
** INVARIANTS:
**
** {F12271} The [sqlite3_interrupt()] interface will force all running
**          SQL statements associated with the same database connection
**          to halt after processing at most one additional row of
**          data.
**
** {F12272} Any SQL statement that is interrupted by [sqlite3_interrupt()]
**          will return [SQLITE_INTERRUPT].
**
** LIMITATIONS:
**
** {U12279} If the database connection closes while [sqlite3_interrupt()]
**          is running then bad things will likely happen.
*/
void sqlite3_interrupt(sqlite3*);

/*
** CAPI3REF: Determine If An SQL Statement Is Complete {F10510}
**
** These routines are useful for command-line input to determine if the
** currently entered text seems to form complete a SQL statement or
** if additional input is needed before sending the text into
** SQLite for parsing.  These routines return true if the input string
** appears to be a complete SQL statement.  A statement is judged to be
** complete if it ends with a semicolon token and is not a fragment of a
** CREATE TRIGGER statement.  Semicolons that are embedded within
** string literals or quoted identifier names or comments are not
** independent tokens (they are part of the token in which they are
** embedded) and thus do not count as a statement terminator.
**
** These routines do not parse the SQL and
** so will not detect syntactically incorrect SQL.
**
** INVARIANTS:
**
** {F10511} The sqlite3_complete() and sqlite3_complete16() functions
**          return true (non-zero) if and only if the last
**          non-whitespace token in their input is a semicolon that
**          is not in between the BEGIN and END of a CREATE TRIGGER
**          statement.
**
** LIMITATIONS:
**
** {U10512} The input to sqlite3_complete() must be a zero-terminated
**          UTF-8 string.
**
** {U10513} The input to sqlite3_complete16() must be a zero-terminated
**          UTF-16 string in native byte order.
*/
int sqlite3_complete(const char *sql);
int sqlite3_complete16(const void *sql);

/*
** CAPI3REF: Register A Callback To Handle SQLITE_BUSY Errors {F12310}
**
** This routine identifies a callback function that might be
** invoked whenever an attempt is made to open a database table 
** that another thread or process has locked.
** If the busy callback is NULL, then [SQLITE_BUSY]
** or [SQLITE_IOERR_BLOCKED]
** is returned immediately upon encountering the lock.
** If the busy callback is not NULL, then the
** callback will be invoked with two arguments.  The
** first argument to the handler is a copy of the void* pointer which
** is the third argument to this routine.  The second argument to
** the handler is the number of times that the busy handler has
** been invoked for this locking event.   If the
** busy callback returns 0, then no additional attempts are made to
** access the database and [SQLITE_BUSY] or [SQLITE_IOERR_BLOCKED] is returned.
** If the callback returns non-zero, then another attempt
** is made to open the database for reading and the cycle repeats.
**
** The presence of a busy handler does not guarantee that
** it will be invoked when there is lock contention.
** If SQLite determines that invoking the busy handler could result in
** a deadlock, it will go ahead and return [SQLITE_BUSY] or
** [SQLITE_IOERR_BLOCKED] instead of invoking the
** busy handler.
** Consider a scenario where one process is holding a read lock that
** it is trying to promote to a reserved lock and
** a second process is holding a reserved lock that it is trying
** to promote to an exclusive lock.  The first process cannot proceed
** because it is blocked by the second and the second process cannot
** proceed because it is blocked by the first.  If both processes
** invoke the busy handlers, neither will make any progress.  Therefore,
** SQLite returns [SQLITE_BUSY] for the first process, hoping that this
** will induce the first process to release its read lock and allow
** the second process to proceed.
**
** The default busy callback is NULL.
**
** The [SQLITE_BUSY] error is converted to [SQLITE_IOERR_BLOCKED]
** when SQLite is in the middle of a large transaction where all the
** changes will not fit into the in-memory cache.  SQLite will
** already hold a RESERVED lock on the database file, but it needs
** to promote this lock to EXCLUSIVE so that it can spill cache
** pages into the database file without harm to concurrent
** readers.  If it is unable to promote the lock, then the in-memory
** cache will be left in an inconsistent state and so the error
** code is promoted from the relatively benign [SQLITE_BUSY] to
** the more severe [SQLITE_IOERR_BLOCKED].  This error code promotion
** forces an automatic rollback of the changes.  See the
** <a href="http://www.sqlite.org/cvstrac/wiki?p=CorruptionFollowingBusyError">
** CorruptionFollowingBusyError</a> wiki page for a discussion of why
** this is important.
**  
** There can only be a single busy handler defined for each database
** connection.  Setting a new busy handler clears any previous one. 
** Note that calling [sqlite3_busy_timeout()] will also set or clear
** the busy handler.
**
** INVARIANTS:
**
** {F12311} The [sqlite3_busy_handler()] function replaces the busy handler
**          callback in the database connection identified by the 1st
**          parameter with a new busy handler identified by the 2nd and 3rd
**          parameters.
**
** {F12312} The default busy handler for new database connections is NULL.
**
** {F12314} When two or more database connection share a common cache,
**          the busy handler for the database connection currently using
**          the cache is invoked when the cache encounters a lock.
**
** {F12316} If a busy handler callback returns zero, then the SQLite
**          interface that provoked the locking event will return
**          [SQLITE_BUSY].
**
** {F12318} SQLite will invokes the busy handler with two argument which
**          are a copy of the pointer supplied by the 3rd parameter to
**          [sqlite3_busy_handler()] and a count of the number of prior
**          invocations of the busy handler for the same locking event.
**
** LIMITATIONS:
**
** {U12319} A busy handler should not call close the database connection
**          or prepared statement that invoked the busy handler.
*/
int sqlite3_busy_handler(sqlite3*, int(*)(void*,int), void*);

/*
** CAPI3REF: Set A Busy Timeout {F12340}
**
** This routine sets a [sqlite3_busy_handler | busy handler]
** that sleeps for a while when a
** table is locked.  The handler will sleep multiple times until 
** at least "ms" milliseconds of sleeping have been done. {F12343} After
** "ms" milliseconds of sleeping, the handler returns 0 which
** causes [sqlite3_step()] to return [SQLITE_BUSY] or [SQLITE_IOERR_BLOCKED].
**
** Calling this routine with an argument less than or equal to zero
** turns off all busy handlers.
**
** There can only be a single busy handler for a particular database
** connection.  If another busy handler was defined  
** (using [sqlite3_busy_handler()]) prior to calling
** this routine, that other busy handler is cleared.
**
** INVARIANTS:
**
** {F12341} The [sqlite3_busy_timeout()] function overrides any prior
**          [sqlite3_busy_timeout()] or [sqlite3_busy_handler()] setting
**          on the same database connection.
**
** {F12343} If the 2nd parameter to [sqlite3_busy_timeout()] is less than
**          or equal to zero, then the busy handler is cleared so that
**          all subsequent locking events immediately return [SQLITE_BUSY].
**
** {F12344} If the 2nd parameter to [sqlite3_busy_timeout()] is a positive
**          number N, then a busy handler is set that repeatedly calls
**          the xSleep() method in the VFS interface until either the
**          lock clears or until the cumulative sleep time reported back
**          by xSleep() exceeds N milliseconds.
*/
int sqlite3_busy_timeout(sqlite3*, int ms);

/*
** CAPI3REF: Convenience Routines For Running Queries {F12370}
**
** Definition: A <b>result table</b> is memory data structure created by the
** [sqlite3_get_table()] interface.  A result table records the
** complete query results from one or more queries.
**
** The table conceptually has a number of rows and columns.  But
** these numbers are not part of the result table itself.  These
** numbers are obtained separately.  Let N be the number of rows
** and M be the number of columns.
**
** A result table is an array of pointers to zero-terminated
** UTF-8 strings.  There are (N+1)*M elements in the array.  
** The first M pointers point to zero-terminated strings that 
** contain the names of the columns.
** The remaining entries all point to query results.  NULL
** values are give a NULL pointer.  All other values are in
** their UTF-8 zero-terminated string representation as returned by
** [sqlite3_column_text()].
**
** A result table might consists of one or more memory allocations.
** It is not safe to pass a result table directly to [sqlite3_free()].
** A result table should be deallocated using [sqlite3_free_table()].
**
** As an example of the result table format, suppose a query result
** is as follows:
**
** <blockquote><pre>
**        Name        | Age
**        -----------------------
**        Alice       | 43
**        Bob         | 28
**        Cindy       | 21
** </pre></blockquote>
**
** There are two column (M==2) and three rows (N==3).  Thus the
** result table has 8 entries.  Suppose the result table is stored
** in an array names azResult.  Then azResult holds this content:
**
** <blockquote><pre>
**        azResult&#91;0] = "Name";
**        azResult&#91;1] = "Age";
**        azResult&#91;2] = "Alice";
**        azResult&#91;3] = "43";
**        azResult&#91;4] = "Bob";
**        azResult&#91;5] = "28";
**        azResult&#91;6] = "Cindy";
**        azResult&#91;7] = "21";
** </pre></blockquote>
**
** The sqlite3_get_table() function evaluates one or more
** semicolon-separated SQL statements in the zero-terminated UTF-8
** string of its 2nd parameter.  It returns a result table to the
** pointer given in its 3rd parameter.
**
** After the calling function has finished using the result, it should 
** pass the pointer to the result table to sqlite3_free_table() in order to 
** release the memory that was malloc-ed.  Because of the way the 
** [sqlite3_malloc()] happens within sqlite3_get_table(), the calling
** function must not try to call [sqlite3_free()] directly.  Only 
** [sqlite3_free_table()] is able to release the memory properly and safely.
**
** The sqlite3_get_table() interface is implemented as a wrapper around
** [sqlite3_exec()].  The sqlite3_get_table() routine does not have access
** to any internal data structures of SQLite.  It uses only the public
** interface defined here.  As a consequence, errors that occur in the
** wrapper layer outside of the internal [sqlite3_exec()] call are not
** reflected in subsequent calls to [sqlite3_errcode()] or
** [sqlite3_errmsg()].
**
** INVARIANTS:
**
** {F12371} If a [sqlite3_get_table()] fails a memory allocation, then
**          it frees the result table under construction, aborts the
**          query in process, skips any subsequent queries, sets the
**          *resultp output pointer to NULL and returns [SQLITE_NOMEM].
**
** {F12373} If the ncolumn parameter to [sqlite3_get_table()] is not NULL
**          then [sqlite3_get_table()] write the number of columns in the
**          result set of the query into *ncolumn if the query is
**          successful (if the function returns SQLITE_OK).
**
** {F12374} If the nrow parameter to [sqlite3_get_table()] is not NULL
**          then [sqlite3_get_table()] write the number of rows in the
**          result set of the query into *nrow if the query is
**          successful (if the function returns SQLITE_OK).
**
** {F12376} The [sqlite3_get_table()] function sets its *ncolumn value
**          to the number of columns in the result set of the query in the
**          sql parameter, or to zero if the query in sql has an empty
**          result set.
*/
int sqlite3_get_table(
  sqlite3*,             /* An open database */
  const char *sql,      /* SQL to be evaluated */
  char ***pResult,      /* Results of the query */
  int *nrow,            /* Number of result rows written here */
  int *ncolumn,         /* Number of result columns written here */
  char **errmsg         /* Error msg written here */
);
void sqlite3_free_table(char **result);

/*
** CAPI3REF: Formatted String Printing Functions {F17400}
**
** These routines are workalikes of the "printf()" family of functions
** from the standard C library.
**
** The sqlite3_mprintf() and sqlite3_vmprintf() routines write their
** results into memory obtained from [sqlite3_malloc()].
** The strings returned by these two routines should be
** released by [sqlite3_free()].   Both routines return a
** NULL pointer if [sqlite3_malloc()] is unable to allocate enough
** memory to hold the resulting string.
**
** In sqlite3_snprintf() routine is similar to "snprintf()" from
** the standard C library.  The result is written into the
** buffer supplied as the second parameter whose size is given by
** the first parameter. Note that the order of the
** first two parameters is reversed from snprintf().  This is an
** historical accident that cannot be fixed without breaking
** backwards compatibility.  Note also that sqlite3_snprintf()
** returns a pointer to its buffer instead of the number of
** characters actually written into the buffer.  We admit that
** the number of characters written would be a more useful return
** value but we cannot change the implementation of sqlite3_snprintf()
** now without breaking compatibility.
**
** As long as the buffer size is greater than zero, sqlite3_snprintf()
** guarantees that the buffer is always zero-terminated.  The first
** parameter "n" is the total size of the buffer, including space for
** the zero terminator.  So the longest string that can be completely
** written will be n-1 characters.
**
** These routines all implement some additional formatting
** options that are useful for constructing SQL statements.
** All of the usual printf formatting options apply.  In addition, there
** is are "%q", "%Q", and "%z" options.
**
** The %q option works like %s in that it substitutes a null-terminated
** string from the argument list.  But %q also doubles every '\'' character.
** %q is designed for use inside a string literal.  By doubling each '\''
** character it escapes that character and allows it to be inserted into
** the string.
**
** For example, so some string variable contains text as follows:
**
** <blockquote><pre>
**  char *zText = "It's a happy day!";
** </pre></blockquote>
**
** One can use this text in an SQL statement as follows:
**
** <blockquote><pre>
**  char *zSQL = sqlite3_mprintf("INSERT INTO table VALUES('%q')", zText);
**  sqlite3_exec(db, zSQL, 0, 0, 0);
**  sqlite3_free(zSQL);
** </pre></blockquote>
**
** Because the %q format string is used, the '\'' character in zText
** is escaped and the SQL generated is as follows:
**
** <blockquote><pre>
**  INSERT INTO table1 VALUES('It''s a happy day!')
** </pre></blockquote>
**
** This is correct.  Had we used %s instead of %q, the generated SQL
** would have looked like this:
**
** <blockquote><pre>
**  INSERT INTO table1 VALUES('It's a happy day!');
** </pre></blockquote>
**
** This second example is an SQL syntax error.  As a general rule you
** should always use %q instead of %s when inserting text into a string 
** literal.
**
** The %Q option works like %q except it also adds single quotes around
** the outside of the total string.  Or if the parameter in the argument
** list is a NULL pointer, %Q substitutes the text "NULL" (without single
** quotes) in place of the %Q option. {END}  So, for example, one could say:
**
** <blockquote><pre>
**  char *zSQL = sqlite3_mprintf("INSERT INTO table VALUES(%Q)", zText);
**  sqlite3_exec(db, zSQL, 0, 0, 0);
**  sqlite3_free(zSQL);
** </pre></blockquote>
**
** The code above will render a correct SQL statement in the zSQL
** variable even if the zText variable is a NULL pointer.
**
** The "%z" formatting option works exactly like "%s" with the
** addition that after the string has been read and copied into
** the result, [sqlite3_free()] is called on the input string. {END}
**
** INVARIANTS:
**
** {F17403}  The [sqlite3_mprintf()] and [sqlite3_vmprintf()] interfaces
**           return either pointers to zero-terminated UTF-8 strings held in
**           memory obtained from [sqlite3_malloc()] or NULL pointers if
**           a call to [sqlite3_malloc()] fails.
**
** {F17406}  The [sqlite3_snprintf()] interface writes a zero-terminated
**           UTF-8 string into the buffer pointed to by the second parameter
**           provided that the first parameter is greater than zero.
**
** {F17407}  The [sqlite3_snprintf()] interface does not writes slots of
**           its output buffer (the second parameter) outside the range
**           of 0 through N-1 (where N is the first parameter)
**           regardless of the length of the string
**           requested by the format specification.
**   
*/
char *sqlite3_mprintf(const char*,...);
char *sqlite3_vmprintf(const char*, va_list);
char *sqlite3_snprintf(int,char*,const char*, ...);

/*
** CAPI3REF: Memory Allocation Subsystem {F17300}
**
** The SQLite core  uses these three routines for all of its own
** internal memory allocation needs. "Core" in the previous sentence
** does not include operating-system specific VFS implementation.  The
** windows VFS uses native malloc and free for some operations.
**
** The sqlite3_malloc() routine returns a pointer to a block
** of memory at least N bytes in length, where N is the parameter.
** If sqlite3_malloc() is unable to obtain sufficient free
** memory, it returns a NULL pointer.  If the parameter N to
** sqlite3_malloc() is zero or negative then sqlite3_malloc() returns
** a NULL pointer.
**
** Calling sqlite3_free() with a pointer previously returned
** by sqlite3_malloc() or sqlite3_realloc() releases that memory so
** that it might be reused.  The sqlite3_free() routine is
** a no-op if is called with a NULL pointer.  Passing a NULL pointer
** to sqlite3_free() is harmless.  After being freed, memory
** should neither be read nor written.  Even reading previously freed
** memory might result in a segmentation fault or other severe error.
** Memory corruption, a segmentation fault, or other severe error
** might result if sqlite3_free() is called with a non-NULL pointer that
** was not obtained from sqlite3_malloc() or sqlite3_free().
**
** The sqlite3_realloc() interface attempts to resize a
** prior memory allocation to be at least N bytes, where N is the
** second parameter.  The memory allocation to be resized is the first
** parameter.  If the first parameter to sqlite3_realloc()
** is a NULL pointer then its behavior is identical to calling
** sqlite3_malloc(N) where N is the second parameter to sqlite3_realloc().
** If the second parameter to sqlite3_realloc() is zero or
** negative then the behavior is exactly the same as calling
** sqlite3_free(P) where P is the first parameter to sqlite3_realloc().
** Sqlite3_realloc() returns a pointer to a memory allocation
** of at least N bytes in size or NULL if sufficient memory is unavailable.
** If M is the size of the prior allocation, then min(N,M) bytes
** of the prior allocation are copied into the beginning of buffer returned
** by sqlite3_realloc() and the prior allocation is freed.
** If sqlite3_realloc() returns NULL, then the prior allocation
** is not freed.
**
** The memory returned by sqlite3_malloc() and sqlite3_realloc()
** is always aligned to at least an 8 byte boundary. {END}
**
** The default implementation
** of the memory allocation subsystem uses the malloc(), realloc()
** and free() provided by the standard C library. {F17382} However, if 
** SQLite is compiled with the following C preprocessor macro
**
** <blockquote> SQLITE_MEMORY_SIZE=<i>NNN</i> </blockquote>
**
** where <i>NNN</i> is an integer, then SQLite create a static
** array of at least <i>NNN</i> bytes in size and use that array
** for all of its dynamic memory allocation needs. {END}  Additional
** memory allocator options may be added in future releases.
**
** In SQLite version 3.5.0 and 3.5.1, it was possible to define
** the SQLITE_OMIT_MEMORY_ALLOCATION which would cause the built-in
** implementation of these routines to be omitted.  That capability
** is no longer provided.  Only built-in memory allocators can be
** used.
**
** The windows OS interface layer calls
** the system malloc() and free() directly when converting
** filenames between the UTF-8 encoding used by SQLite
** and whatever filename encoding is used by the particular windows
** installation.  Memory allocation errors are detected, but
** they are reported back as [SQLITE_CANTOPEN] or
** [SQLITE_IOERR] rather than [SQLITE_NOMEM].
**
** INVARIANTS:
**
** {F17303}  The [sqlite3_malloc(N)] interface returns either a pointer to 
**           newly checked-out block of at least N bytes of memory
**           that is 8-byte aligned, 
**           or it returns NULL if it is unable to fulfill the request.
**
** {F17304}  The [sqlite3_malloc(N)] interface returns a NULL pointer if
**           N is less than or equal to zero.
**
** {F17305}  The [sqlite3_free(P)] interface releases memory previously
**           returned from [sqlite3_malloc()] or [sqlite3_realloc()],
**           making it available for reuse.
**
** {F17306}  A call to [sqlite3_free(NULL)] is a harmless no-op.
**
** {F17310}  A call to [sqlite3_realloc(0,N)] is equivalent to a call
**           to [sqlite3_malloc(N)].
**
** {F17312}  A call to [sqlite3_realloc(P,0)] is equivalent to a call
**           to [sqlite3_free(P)].
**
** {F17315}  The SQLite core uses [sqlite3_malloc()], [sqlite3_realloc()],
**           and [sqlite3_free()] for all of its memory allocation and
**           deallocation needs.
**
** {F17318}  The [sqlite3_realloc(P,N)] interface returns either a pointer
**           to a block of checked-out memory of at least N bytes in size
**           that is 8-byte aligned, or a NULL pointer.
**
** {F17321}  When [sqlite3_realloc(P,N)] returns a non-NULL pointer, it first
**           copies the first K bytes of content from P into the newly allocated
**           where K is the lessor of N and the size of the buffer P.
**
** {F17322}  When [sqlite3_realloc(P,N)] returns a non-NULL pointer, it first
**           releases the buffer P.
**
** {F17323}  When [sqlite3_realloc(P,N)] returns NULL, the buffer P is
**           not modified or released.
**
** LIMITATIONS:
**
** {U17350}  The pointer arguments to [sqlite3_free()] and [sqlite3_realloc()]
**           must be either NULL or else a pointer obtained from a prior
**           invocation of [sqlite3_malloc()] or [sqlite3_realloc()] that has
**           not been released.
**
** {U17351}  The application must not read or write any part of 
**           a block of memory after it has been released using
**           [sqlite3_free()] or [sqlite3_realloc()].
**
*/
void *sqlite3_malloc(int);
void *sqlite3_realloc(void*, int);
void sqlite3_free(void*);

/*
** CAPI3REF: Memory Allocator Statistics {F17370}
**
** SQLite provides these two interfaces for reporting on the status
** of the [sqlite3_malloc()], [sqlite3_free()], and [sqlite3_realloc()]
** the memory allocation subsystem included within the SQLite.
**
** INVARIANTS:
**
** {F17371} The [sqlite3_memory_used()] routine returns the
**          number of bytes of memory currently outstanding 
**          (malloced but not freed).
**
** {F17373} The [sqlite3_memory_highwater()] routine returns the maximum
**          value of [sqlite3_memory_used()] 
**          since the highwater mark was last reset.
**
** {F17374} The values returned by [sqlite3_memory_used()] and
**          [sqlite3_memory_highwater()] include any overhead
**          added by SQLite in its implementation of [sqlite3_malloc()],
**          but not overhead added by the any underlying system library
**          routines that [sqlite3_malloc()] may call.
** 
** {F17375} The memory highwater mark is reset to the current value of
**          [sqlite3_memory_used()] if and only if the parameter to
**          [sqlite3_memory_highwater()] is true.  The value returned
**          by [sqlite3_memory_highwater(1)] is the highwater mark
**          prior to the reset.
*/
sqlite3_int64 sqlite3_memory_used(void);
sqlite3_int64 sqlite3_memory_highwater(int resetFlag);

/*
** CAPI3REF: Pseudo-Random Number Generator {F17390}
**
** SQLite contains a high-quality pseudo-random number generator (PRNG) used to
** select random ROWIDs when inserting new records into a table that
** already uses the largest possible ROWID.  The PRNG is also used for
** the build-in random() and randomblob() SQL functions.  This interface allows
** appliations to access the same PRNG for other purposes.
**
** A call to this routine stores N bytes of randomness into buffer P.
**
** The first time this routine is invoked (either internally or by
** the application) the PRNG is seeded using randomness obtained
** from the xRandomness method of the default [sqlite3_vfs] object.
** On all subsequent invocations, the pseudo-randomness is generated
** internally and without recourse to the [sqlite3_vfs] xRandomness
** method.
**
** INVARIANTS:
**
** {F17392} The [sqlite3_randomness(N,P)] interface writes N bytes of
**          high-quality pseudo-randomness into buffer P.
*/
void sqlite3_randomness(int N, void *P);

/*
** CAPI3REF: Compile-Time Authorization Callbacks {F12500}
**
** This routine registers a authorizer callback with a particular
** [database connection], supplied in the first argument.
** The authorizer callback is invoked as SQL statements are being compiled
** by [sqlite3_prepare()] or its variants [sqlite3_prepare_v2()],
** [sqlite3_prepare16()] and [sqlite3_prepare16_v2()].  At various
** points during the compilation process, as logic is being created
** to perform various actions, the authorizer callback is invoked to
** see if those actions are allowed.  The authorizer callback should
** return [SQLITE_OK] to allow the action, [SQLITE_IGNORE] to disallow the
** specific action but allow the SQL statement to continue to be
** compiled, or [SQLITE_DENY] to cause the entire SQL statement to be
** rejected with an error.   If the authorizer callback returns
** any value other than [SQLITE_IGNORE], [SQLITE_OK], or [SQLITE_DENY]
** then [sqlite3_prepare_v2()] or equivalent call that triggered
** the authorizer will fail with an error message.
**
** When the callback returns [SQLITE_OK], that means the operation
** requested is ok.  When the callback returns [SQLITE_DENY], the
** [sqlite3_prepare_v2()] or equivalent call that triggered the
** authorizer will fail with an error message explaining that
** access is denied.  If the authorizer code is [SQLITE_READ]
** and the callback returns [SQLITE_IGNORE] then the
** [prepared statement] statement is constructed to substitute
** a NULL value in place of the table column that would have
** been read if [SQLITE_OK] had been returned.  The [SQLITE_IGNORE]
** return can be used to deny an untrusted user access to individual
** columns of a table.
**
** The first parameter to the authorizer callback is a copy of
** the third parameter to the sqlite3_set_authorizer() interface.
** The second parameter to the callback is an integer 
** [SQLITE_COPY | action code] that specifies the particular action
** to be authorized. The third through sixth
** parameters to the callback are zero-terminated strings that contain 
** additional details about the action to be authorized.
**
** An authorizer is used when [sqlite3_prepare | preparing]
** SQL statements from an untrusted
** source, to ensure that the SQL statements do not try to access data
** that they are not allowed to see, or that they do not try to
** execute malicious statements that damage the database.  For
** example, an application may allow a user to enter arbitrary
** SQL queries for evaluation by a database.  But the application does
** not want the user to be able to make arbitrary changes to the
** database.  An authorizer could then be put in place while the
** user-entered SQL is being [sqlite3_prepare | prepared] that
** disallows everything except [SELECT] statements.
**
** Applications that need to process SQL from untrusted sources
** might also consider lowering resource limits using [sqlite3_limit()]
** and limiting database size using the [max_page_count] [PRAGMA]
** in addition to using an authorizer.
**
** Only a single authorizer can be in place on a database connection
** at a time.  Each call to sqlite3_set_authorizer overrides the
** previous call.  Disable the authorizer by installing a NULL callback.
** The authorizer is disabled by default.
**
** Note that the authorizer callback is invoked only during 
** [sqlite3_prepare()] or its variants.  Authorization is not
** performed during statement evaluation in [sqlite3_step()].
**
** INVARIANTS:
**
** {F12501} The [sqlite3_set_authorizer(D,...)] interface registers a
**          authorizer callback with database connection D.
**
** {F12502} The authorizer callback is invoked as SQL statements are
**          being compiled
**
** {F12503} If the authorizer callback returns any value other than
**          [SQLITE_IGNORE], [SQLITE_OK], or [SQLITE_DENY] then
**          the [sqlite3_prepare_v2()] or equivalent call that caused
**          the authorizer callback to run shall fail with an
**          [SQLITE_ERROR] error code and an appropriate error message.
**
** {F12504} When the authorizer callback returns [SQLITE_OK], the operation
**          described is coded normally.
**
** {F12505} When the authorizer callback returns [SQLITE_DENY], the
**          [sqlite3_prepare_v2()] or equivalent call that caused the
**          authorizer callback to run shall fail
**          with an [SQLITE_ERROR] error code and an error message
**          explaining that access is denied.
**
** {F12506} If the authorizer code (the 2nd parameter to the authorizer
**          callback) is [SQLITE_READ] and the authorizer callback returns
**          [SQLITE_IGNORE] then the prepared statement is constructed to
**          insert a NULL value in place of the table column that would have
**          been read if [SQLITE_OK] had been returned.
**
** {F12507} If the authorizer code (the 2nd parameter to the authorizer
**          callback) is anything other than [SQLITE_READ], then
**          a return of [SQLITE_IGNORE] has the same effect as [SQLITE_DENY]. 
**
** {F12510} The first parameter to the authorizer callback is a copy of
**          the third parameter to the [sqlite3_set_authorizer()] interface.
**
** {F12511} The second parameter to the callback is an integer 
**          [SQLITE_COPY | action code] that specifies the particular action
**          to be authorized.
**
** {F12512} The third through sixth parameters to the callback are
**          zero-terminated strings that contain 
**          additional details about the action to be authorized.
**
** {F12520} Each call to [sqlite3_set_authorizer()] overrides the
**          any previously installed authorizer.
**
** {F12521} A NULL authorizer means that no authorization
**          callback is invoked.
**
** {F12522} The default authorizer is NULL.
*/
int sqlite3_set_authorizer(
  sqlite3*,
  int (*xAuth)(void*,int,const char*,const char*,const char*,const char*),
  void *pUserData
);

/*
** CAPI3REF: Authorizer Return Codes {F12590}
**
** The [sqlite3_set_authorizer | authorizer callback function] must
** return either [SQLITE_OK] or one of these two constants in order
** to signal SQLite whether or not the action is permitted.  See the
** [sqlite3_set_authorizer | authorizer documentation] for additional
** information.
*/
#define SQLITE_DENY   1   /* Abort the SQL statement with an error */
#define SQLITE_IGNORE 2   /* Don't allow access, but don't generate an error */

/*
** CAPI3REF: Authorizer Action Codes {F12550}
**
** The [sqlite3_set_authorizer()] interface registers a callback function
** that is invoked to authorizer certain SQL statement actions.  The
** second parameter to the callback is an integer code that specifies
** what action is being authorized.  These are the integer action codes that
** the authorizer callback may be passed.
**
** These action code values signify what kind of operation is to be 
** authorized.  The 3rd and 4th parameters to the authorization
** callback function will be parameters or NULL depending on which of these
** codes is used as the second parameter.  The 5th parameter to the
** authorizer callback is the name of the database ("main", "temp", 
** etc.) if applicable.  The 6th parameter to the authorizer callback
** is the name of the inner-most trigger or view that is responsible for
** the access attempt or NULL if this access attempt is directly from 
** top-level SQL code.
**
** INVARIANTS:
**
** {F12551} The second parameter to an 
**          [sqlite3_set_authorizer | authorizer callback is always an integer
**          [SQLITE_COPY | authorizer code] that specifies what action
**          is being authorized.
**
** {F12552} The 3rd and 4th parameters to the 
**          [sqlite3_set_authorizer | authorization callback function]
**          will be parameters or NULL depending on which 
**          [SQLITE_COPY | authorizer code] is used as the second parameter.
**
** {F12553} The 5th parameter to the
**          [sqlite3_set_authorizer | authorizer callback] is the name
**          of the database (example: "main", "temp", etc.) if applicable.
**
** {F12554} The 6th parameter to the
**          [sqlite3_set_authorizer | authorizer callback] is the name
**          of the inner-most trigger or view that is responsible for
**          the access attempt or NULL if this access attempt is directly from 
**          top-level SQL code.
*/
/******************************************* 3rd ************ 4th ***********/
#define SQLITE_CREATE_INDEX          1   /* Index Name      Table Name      */
#define SQLITE_CREATE_TABLE          2   /* Table Name      NULL            */
#define SQLITE_CREATE_TEMP_INDEX     3   /* Index Name      Table Name      */
#define SQLITE_CREATE_TEMP_TABLE     4   /* Table Name      NULL            */
#define SQLITE_CREATE_TEMP_TRIGGER   5   /* Trigger Name    Table Name      */
#define SQLITE_CREATE_TEMP_VIEW      6   /* View Name       NULL            */
#define SQLITE_CREATE_TRIGGER        7   /* Trigger Name    Table Name      */
#define SQLITE_CREATE_VIEW           8   /* View Name       NULL            */
#define SQLITE_DELETE                9   /* Table Name      NULL            */
#define SQLITE_DROP_INDEX           10   /* Index Name      Table Name      */
#define SQLITE_DROP_TABLE           11   /* Table Name      NULL            */
#define SQLITE_DROP_TEMP_INDEX      12   /* Index Name      Table Name      */
#define SQLITE_DROP_TEMP_TABLE      13   /* Table Name      NULL            */
#define SQLITE_DROP_TEMP_TRIGGER    14   /* Trigger Name    Table Name      */
#define SQLITE_DROP_TEMP_VIEW       15   /* View Name       NULL            */
#define SQLITE_DROP_TRIGGER         16   /* Trigger Name    Table Name      */
#define SQLITE_DROP_VIEW            17   /* View Name       NULL            */
#define SQLITE_INSERT               18   /* Table Name      NULL            */
#define SQLITE_PRAGMA               19   /* Pragma Name     1st arg or NULL */
#define SQLITE_READ                 20   /* Table Name      Column Name     */
#define SQLITE_SELECT               21   /* NULL            NULL            */
#define SQLITE_TRANSACTION          22   /* NULL            NULL            */
#define SQLITE_UPDATE               23   /* Table Name      Column Name     */
#define SQLITE_ATTACH               24   /* Filename        NULL            */
#define SQLITE_DETACH               25   /* Database Name   NULL            */
#define SQLITE_ALTER_TABLE          26   /* Database Name   Table Name      */
#define SQLITE_REINDEX              27   /* Index Name      NULL            */
#define SQLITE_ANALYZE              28   /* Table Name      NULL            */
#define SQLITE_CREATE_VTABLE        29   /* Table Name      Module Name     */
#define SQLITE_DROP_VTABLE          30   /* Table Name      Module Name     */
#define SQLITE_FUNCTION             31   /* Function Name   NULL            */
#define SQLITE_COPY                  0   /* No longer used */

/*
** CAPI3REF: Tracing And Profiling Functions {F12280}
**
** These routines register callback functions that can be used for
** tracing and profiling the execution of SQL statements.
**
** The callback function registered by sqlite3_trace() is invoked at
** various times when an SQL statement is being run by [sqlite3_step()].
** The callback returns a UTF-8 rendering of the SQL statement text
** as the statement first begins executing.  Additional callbacks occur
** as each triggersubprogram is entered.  The callbacks for triggers
** contain a UTF-8 SQL comment that identifies the trigger.
** 
** The callback function registered by sqlite3_profile() is invoked
** as each SQL statement finishes.  The profile callback contains
** the original statement text and an estimate of wall-clock time
** of how long that statement took to run.
**
** The sqlite3_profile() API is currently considered experimental and
** is subject to change or removal in a future release.
**
** The trigger reporting feature of the trace callback is considered
** experimental and is subject to change or removal in future releases.
** Future versions of SQLite might also add new trace callback 
** invocations.
**
** INVARIANTS:
**
** {F12281} The callback function registered by [sqlite3_trace()] is
**          whenever an SQL statement first begins to execute and
**          whenever a trigger subprogram first begins to run.
**
** {F12282} Each call to [sqlite3_trace()] overrides the previously
**          registered trace callback.
**
** {F12283} A NULL trace callback disables tracing.
**
** {F12284} The first argument to the trace callback is a copy of
**          the pointer which was the 3rd argument to [sqlite3_trace()].
**
** {F12285} The second argument to the trace callback is a
**          zero-terminated UTF8 string containing the original text
**          of the SQL statement as it was passed into [sqlite3_prepare_v2()]
**          or the equivalent, or an SQL comment indicating the beginning
**          of a trigger subprogram.
**
** {F12287} The callback function registered by [sqlite3_profile()] is invoked
**          as each SQL statement finishes.
**
** {F12288} The first parameter to the profile callback is a copy of
**          the 3rd parameter to [sqlite3_profile()].
**
** {F12289} The second parameter to the profile callback is a
**          zero-terminated UTF-8 string that contains the complete text of
**          the SQL statement as it was processed by [sqlite3_prepare_v2()]
**          or the equivalent.
**
** {F12290} The third parameter to the profile  callback is an estimate
**          of the number of nanoseconds of wall-clock time required to
**          run the SQL statement from start to finish.
*/
void *sqlite3_trace(sqlite3*, void(*xTrace)(void*,const char*), void*);
void *sqlite3_profile(sqlite3*,
   void(*xProfile)(void*,const char*,sqlite3_uint64), void*);

/*
** CAPI3REF: Query Progress Callbacks {F12910}
**
** This routine configures a callback function - the
** progress callback - that is invoked periodically during long
** running calls to [sqlite3_exec()], [sqlite3_step()] and
** [sqlite3_get_table()].   An example use for this 
** interface is to keep a GUI updated during a large query.
**
** If the progress callback returns non-zero, the opertion is
** interrupted.  This feature can be used to implement a
** "Cancel" button on a GUI dialog box.
**
** INVARIANTS:
**
** {F12911} The callback function registered by [sqlite3_progress_handler()]
**          is invoked periodically during long running calls to
**          [sqlite3_step()].
**
** {F12912} The progress callback is invoked once for every N virtual
**          machine opcodes, where N is the second argument to 
**          the [sqlite3_progress_handler()] call that registered
**          the callback.  <todo>What if N is less than 1?</todo>
**
** {F12913} The progress callback itself is identified by the third
**          argument to [sqlite3_progress_handler()].
**
** {F12914} The fourth argument [sqlite3_progress_handler()] is a
***         void pointer passed to the progress callback
**          function each time it is invoked.
**
** {F12915} If a call to [sqlite3_step()] results in fewer than
**          N opcodes being executed,
**          then the progress callback is never invoked. {END}
** 
** {F12916} Every call to [sqlite3_progress_handler()]
**          overwrites any previously registere progress handler.
**
** {F12917} If the progress handler callback is NULL then no progress
**          handler is invoked.
**
** {F12918} If the progress callback returns a result other than 0, then
**          the behavior is a if [sqlite3_interrupt()] had been called.
*/
void sqlite3_progress_handler(sqlite3*, int, int(*)(void*), void*);

/*
** CAPI3REF: Opening A New Database Connection {F12700}
**
** These routines open an SQLite database file whose name
** is given by the filename argument.
** The filename argument is interpreted as UTF-8
** for [sqlite3_open()] and [sqlite3_open_v2()] and as UTF-16
** in the native byte order for [sqlite3_open16()].
** An [sqlite3*] handle is usually returned in *ppDb, even
** if an error occurs.  The only exception is if SQLite is unable
** to allocate memory to hold the [sqlite3] object, a NULL will
** be written into *ppDb instead of a pointer to the [sqlite3] object.
** If the database is opened (and/or created)
** successfully, then [SQLITE_OK] is returned.  Otherwise an
** error code is returned.  The
** [sqlite3_errmsg()] or [sqlite3_errmsg16()]  routines can be used to obtain
** an English language description of the error.
**
** The default encoding for the database will be UTF-8 if
** [sqlite3_open()] or [sqlite3_open_v2()] is called and
** UTF-16 in the native byte order if [sqlite3_open16()] is used.
**
** Whether or not an error occurs when it is opened, resources
** associated with the [sqlite3*] handle should be released by passing it
** to [sqlite3_close()] when it is no longer required.
**
** The [sqlite3_open_v2()] interface works like [sqlite3_open()] 
** except that it acccepts two additional parameters for additional control
** over the new database connection.  The flags parameter can be
** one of:
**
** <ol>
** <li>  [SQLITE_OPEN_READONLY]
** <li>  [SQLITE_OPEN_READWRITE]
** <li>  [SQLITE_OPEN_READWRITE] | [SQLITE_OPEN_CREATE]
** </ol>
**
** The first value opens the database read-only. 
** If the database does not previously exist, an error is returned.
** The second option opens
** the database for reading and writing if possible, or reading only if
** if the file is write protected.  In either case the database
** must already exist or an error is returned.  The third option
** opens the database for reading and writing and creates it if it does
** not already exist.
** The third options is behavior that is always used for [sqlite3_open()]
** and [sqlite3_open16()].
**
** If the 3rd parameter to [sqlite3_open_v2()] is not one of the
** combinations shown above then the behavior is undefined.
**
** If the filename is ":memory:", then an private
** in-memory database is created for the connection.  This in-memory
** database will vanish when the database connection is closed.  Future
** version of SQLite might make use of additional special filenames
** that begin with the ":" character.  It is recommended that 
** when a database filename really does begin with
** ":" that you prefix the filename with a pathname like "./" to
** avoid ambiguity.
**
** If the filename is an empty string, then a private temporary
** on-disk database will be created.  This private database will be
** automatically deleted as soon as the database connection is closed.
**
** The fourth parameter to sqlite3_open_v2() is the name of the
** [sqlite3_vfs] object that defines the operating system 
** interface that the new database connection should use.  If the
** fourth parameter is a NULL pointer then the default [sqlite3_vfs]
** object is used.
**
** <b>Note to windows users:</b>  The encoding used for the filename argument
** of [sqlite3_open()] and [sqlite3_open_v2()] must be UTF-8, not whatever
** codepage is currently defined.  Filenames containing international
** characters must be converted to UTF-8 prior to passing them into
** [sqlite3_open()] or [sqlite3_open_v2()].
**
** INVARIANTS:
**
** {F12701} The [sqlite3_open()], [sqlite3_open16()], and
**          [sqlite3_open_v2()] interfaces create a new
**          [database connection] associated with
**          the database file given in their first parameter.
**
** {F12702} The filename argument is interpreted as UTF-8
**          for [sqlite3_open()] and [sqlite3_open_v2()] and as UTF-16
**          in the native byte order for [sqlite3_open16()].
**
** {F12703} A successful invocation of [sqlite3_open()], [sqlite3_open16()], 
**          or [sqlite3_open_v2()] writes a pointer to a new
**          [database connection] into *ppDb.
**
** {F12704} The [sqlite3_open()], [sqlite3_open16()], and
**          [sqlite3_open_v2()] interfaces return [SQLITE_OK] upon success,
**          or an appropriate [error code] on failure.
**
** {F12706} The default text encoding for a new database created using
**          [sqlite3_open()] or [sqlite3_open_v2()] will be UTF-8.
**
** {F12707} The default text encoding for a new database created using
**          [sqlite3_open16()] will be UTF-16.
**
** {F12709} The [sqlite3_open(F,D)] interface is equivalent to
**          [sqlite3_open_v2(F,D,G,0)] where the G parameter is
**          [SQLITE_OPEN_READWRITE]|[SQLITE_OPEN_CREATE].
**
** {F12711} If the G parameter to [sqlite3_open_v2(F,D,G,V)] contains the
**          bit value [SQLITE_OPEN_READONLY] then the database is opened
**          for reading only.
**
** {F12712} If the G parameter to [sqlite3_open_v2(F,D,G,V)] contains the
**          bit value [SQLITE_OPEN_READWRITE] then the database is opened
**          reading and writing if possible, or for reading only if the
**          file is write protected by the operating system.
**
** {F12713} If the G parameter to [sqlite3_open(v2(F,D,G,V)] omits the
**          bit value [SQLITE_OPEN_CREATE] and the database does not
**          previously exist, an error is returned.
**
** {F12714} If the G parameter to [sqlite3_open(v2(F,D,G,V)] contains the
**          bit value [SQLITE_OPEN_CREATE] and the database does not
**          previously exist, then an attempt is made to create and
**          initialize the database.
**
** {F12717} If the filename argument to [sqlite3_open()], [sqlite3_open16()],
**          or [sqlite3_open_v2()] is ":memory:", then an private,
**          ephemeral, in-memory database is created for the connection.
**          <todo>Is SQLITE_OPEN_CREATE|SQLITE_OPEN_READWRITE required
**          in sqlite3_open_v2()?</todo>
**
** {F12719} If the filename is NULL or an empty string, then a private,
**          ephermeral on-disk database will be created.
**          <todo>Is SQLITE_OPEN_CREATE|SQLITE_OPEN_READWRITE required
**          in sqlite3_open_v2()?</todo>
**
** {F12721} The [database connection] created by 
**          [sqlite3_open_v2(F,D,G,V)] will use the
**          [sqlite3_vfs] object identified by the V parameter, or
**          the default [sqlite3_vfs] object is V is a NULL pointer.
*/
int sqlite3_open(
  const char *filename,   /* Database filename (UTF-8) */
  sqlite3 **ppDb          /* OUT: SQLite db handle */
);
int sqlite3_open16(
  const void *filename,   /* Database filename (UTF-16) */
  sqlite3 **ppDb          /* OUT: SQLite db handle */
);
int sqlite3_open_v2(
  const char *filename,   /* Database filename (UTF-8) */
  sqlite3 **ppDb,         /* OUT: SQLite db handle */
  int flags,              /* Flags */
  const char *zVfs        /* Name of VFS module to use */
);

/*
** CAPI3REF: Error Codes And Messages {F12800}
**
** The sqlite3_errcode() interface returns the numeric
** [SQLITE_OK | result code] or [SQLITE_IOERR_READ | extended result code]
** for the most recent failed sqlite3_* API call associated
** with [sqlite3] handle 'db'. If a prior API call failed but the
** most recent API call succeeded, the return value from sqlite3_errcode()
** is undefined.
**
** The sqlite3_errmsg() and sqlite3_errmsg16() return English-language
** text that describes the error, as either UTF8 or UTF16 respectively.
** Memory to hold the error message string is managed internally.
** The application does not need to worry with freeing the result.
** However, the error string might be overwritten or deallocated by
** subsequent calls to other SQLite interface functions.
**
** INVARIANTS:
**
** {F12801} The [sqlite3_errcode(D)] interface returns the numeric
**          [SQLITE_OK | result code] or
**          [SQLITE_IOERR_READ | extended result code]
**          for the most recently failed interface call associated
**          with [database connection] D.
**
** {F12803} The [sqlite3_errmsg(D)] and [sqlite3_errmsg16(D)]
**          interfaces return English-language text that describes
**          the error in the mostly recently failed interface call,
**          encoded as either UTF8 or UTF16 respectively.
**
** {F12807} The strings returned by [sqlite3_errmsg()] and [sqlite3_errmsg16()]
**          are valid until the next SQLite interface call.
**
** {F12808} Calls to API routines that do not return an error code
**          (example: [sqlite3_data_count()]) do not
**          change the error code or message returned by
**          [sqlite3_errcode()], [sqlite3_errmsg()], or [sqlite3_errmsg16()].
**
** {F12809} Interfaces that are not associated with a specific
**          [database connection] (examples:
**          [sqlite3_mprintf()] or [sqlite3_enable_shared_cache()]
**          do not change the values returned by
**          [sqlite3_errcode()], [sqlite3_errmsg()], or [sqlite3_errmsg16()].
*/
int sqlite3_errcode(sqlite3 *db);
const char *sqlite3_errmsg(sqlite3*);
const void *sqlite3_errmsg16(sqlite3*);

/*
** CAPI3REF: SQL Statement Object {F13000}
** KEYWORDS: {prepared statement} {prepared statements}
**
** An instance of this object represent single SQL statements.  This
** object is variously known as a "prepared statement" or a 
** "compiled SQL statement" or simply as a "statement".
** 
** The life of a statement object goes something like this:
**
** <ol>
** <li> Create the object using [sqlite3_prepare_v2()] or a related
**      function.
** <li> Bind values to host parameters using
**      [sqlite3_bind_blob | sqlite3_bind_* interfaces].
** <li> Run the SQL by calling [sqlite3_step()] one or more times.
** <li> Reset the statement using [sqlite3_reset()] then go back
**      to step 2.  Do this zero or more times.
** <li> Destroy the object using [sqlite3_finalize()].
** </ol>
**
** Refer to documentation on individual methods above for additional
** information.
*/
typedef struct sqlite3_stmt sqlite3_stmt;

/*
** CAPI3REF: Run-time Limits {F12760}
**
** This interface allows the size of various constructs to be limited
** on a connection by connection basis.  The first parameter is the
** [database connection] whose limit is to be set or queried.  The
** second parameter is one of the [limit categories] that define a
** class of constructs to be size limited.  The third parameter is the
** new limit for that construct.  The function returns the old limit.
**
** If the new limit is a negative number, the limit is unchanged.
** For the limit category of SQLITE_LIMIT_XYZ there is a hard upper
** bound set by a compile-time C-preprocess macro named SQLITE_MAX_XYZ.
** (The "_LIMIT_" in the name is changed to "_MAX_".)
** Attempts to increase a limit above its hard upper bound are
** silently truncated to the hard upper limit.
**
** Run time limits are intended for use in applications that manage
** both their own internal database and also databases that are controlled
** by untrusted external sources.  An example application might be a
** webbrowser that has its own databases for storing history and
** separate databases controlled by javascript applications downloaded
** off the internet.  The internal databases can be given the
** large, default limits.  Databases managed by external sources can
** be given much smaller limits designed to prevent a denial of service
** attach.  Developers might also want to use the [sqlite3_set_authorizer()]
** interface to further control untrusted SQL.  The size of the database
** created by an untrusted script can be contained using the
** [max_page_count] [PRAGMA].
**
** This interface is currently considered experimental and is subject
** to change or removal without prior notice.
**
** INVARIANTS:
**
** {F12762} A successful call to [sqlite3_limit(D,C,V)] where V is
**          positive changes the
**          limit on the size of construct C in [database connection] D
**          to the lessor of V and the hard upper bound on the size
**          of C that is set at compile-time.
**
** {F12766} A successful call to [sqlite3_limit(D,C,V)] where V is negative
**          leaves the state of [database connection] D unchanged.
**
** {F12769} A successful call to [sqlite3_limit(D,C,V)] returns the
**          value of the limit on the size of construct C in
**          in [database connection] D as it was prior to the call.
*/
int sqlite3_limit(sqlite3*, int id, int newVal);

/*
** CAPI3REF: Run-Time Limit Categories {F12790}
** KEYWORDS: {limit category} {limit categories}
** 
** These constants define various aspects of a [database connection]
** that can be limited in size by calls to [sqlite3_limit()].
** The meanings of the various limits are as follows:
**
** <dl>
** <dt>SQLITE_LIMIT_LENGTH</dt>
** <dd>The maximum size of any
** string or blob or table row.<dd>
**
** <dt>SQLITE_LIMIT_SQL_LENGTH</dt>
** <dd>The maximum length of an SQL statement.</dd>
**
** <dt>SQLITE_LIMIT_COLUMN</dt>
** <dd>The maximum number of columns in a table definition or in the
** result set of a SELECT or the maximum number of columns in an index
** or in an ORDER BY or GROUP BY clause.</dd>
**
** <dt>SQLITE_LIMIT_EXPR_DEPTH</dt>
** <dd>The maximum depth of the parse tree on any expression.</dd>
**
** <dt>SQLITE_LIMIT_COMPOUND_SELECT</dt>
** <dd>The maximum number of terms in a compound SELECT statement.</dd>
**
** <dt>SQLITE_LIMIT_VDBE_OP</dt>
** <dd>The maximum number of instructions in a virtual machine program
** used to implement an SQL statement.</dd>
**
** <dt>SQLITE_LIMIT_FUNCTION_ARG</dt>
** <dd>The maximum number of arguments on a function.</dd>
**
** <dt>SQLITE_LIMIT_ATTACHED</dt>
** <dd>The maximum number of attached databases.</dd>
**
** <dt>SQLITE_LIMIT_LIKE_PATTERN_LENGTH</dt>
** <dd>The maximum length of the pattern argument to the LIKE or
** GLOB operators.</dd>
**
** <dt>SQLITE_LIMIT_VARIABLE_NUMBER</dt>
** <dd>The maximum number of variables in an SQL statement that can
** be bound.</dd>
** </dl>
*/
#define SQLITE_LIMIT_LENGTH                    0
#define SQLITE_LIMIT_SQL_LENGTH                1
#define SQLITE_LIMIT_COLUMN                    2
#define SQLITE_LIMIT_EXPR_DEPTH                3
#define SQLITE_LIMIT_COMPOUND_SELECT           4
#define SQLITE_LIMIT_VDBE_OP                   5
#define SQLITE_LIMIT_FUNCTION_ARG              6
#define SQLITE_LIMIT_ATTACHED                  7
#define SQLITE_LIMIT_LIKE_PATTERN_LENGTH       8
#define SQLITE_LIMIT_VARIABLE_NUMBER           9

/*
** CAPI3REF: Compiling An SQL Statement {F13010}
**
** To execute an SQL query, it must first be compiled into a byte-code
** program using one of these routines. 
**
** The first argument "db" is an [database connection] 
** obtained from a prior call to [sqlite3_open()], [sqlite3_open_v2()]
** or [sqlite3_open16()]. 
** The second argument "zSql" is the statement to be compiled, encoded
** as either UTF-8 or UTF-16.  The sqlite3_prepare() and sqlite3_prepare_v2()
** interfaces uses UTF-8 and sqlite3_prepare16() and sqlite3_prepare16_v2()
** use UTF-16. {END}
**
** If the nByte argument is less
** than zero, then zSql is read up to the first zero terminator.
** If nByte is non-negative, then it is the maximum number of 
** bytes read from zSql.  When nByte is non-negative, the
** zSql string ends at either the first '\000' or '\u0000' character or 
** the nByte-th byte, whichever comes first. If the caller knows
** that the supplied string is nul-terminated, then there is a small
** performance advantage to be had by passing an nByte parameter that 
** is equal to the number of bytes in the input string <i>including</i> 
** the nul-terminator bytes.{END}
**
** *pzTail is made to point to the first byte past the end of the
** first SQL statement in zSql.  These routines only compiles the first
** statement in zSql, so *pzTail is left pointing to what remains
** uncompiled.
**
** *ppStmt is left pointing to a compiled [prepared statement] that can be
** executed using [sqlite3_step()].  Or if there is an error, *ppStmt is
** set to NULL.  If the input text contains no SQL (if the input
** is and empty string or a comment) then *ppStmt is set to NULL.
** {U13018} The calling procedure is responsible for deleting the
** compiled SQL statement
** using [sqlite3_finalize()] after it has finished with it.
**
** On success, [SQLITE_OK] is returned.  Otherwise an 
** [error code] is returned.
**
** The sqlite3_prepare_v2() and sqlite3_prepare16_v2() interfaces are
** recommended for all new programs. The two older interfaces are retained
** for backwards compatibility, but their use is discouraged.
** In the "v2" interfaces, the prepared statement
** that is returned (the [sqlite3_stmt] object) contains a copy of the 
** original SQL text. {END} This causes the [sqlite3_step()] interface to
** behave a differently in two ways:
**
** <ol>
** <li>
** If the database schema changes, instead of returning [SQLITE_SCHEMA] as it
** always used to do, [sqlite3_step()] will automatically recompile the SQL
** statement and try to run it again.  If the schema has changed in
** a way that makes the statement no longer valid, [sqlite3_step()] will still
** return [SQLITE_SCHEMA].  But unlike the legacy behavior, 
** [SQLITE_SCHEMA] is now a fatal error.  Calling
** [sqlite3_prepare_v2()] again will not make the
** error go away.  Note: use [sqlite3_errmsg()] to find the text
** of the parsing error that results in an [SQLITE_SCHEMA] return. {END}
** </li>
**
** <li>
** When an error occurs, 
** [sqlite3_step()] will return one of the detailed 
** [error codes] or [extended error codes]. 
** The legacy behavior was that [sqlite3_step()] would only return a generic
** [SQLITE_ERROR] result code and you would have to make a second call to
** [sqlite3_reset()] in order to find the underlying cause of the problem.
** With the "v2" prepare interfaces, the underlying reason for the error is
** returned immediately.
** </li>
** </ol>
**
** INVARIANTS:
**
** {F13011} The [sqlite3_prepare(db,zSql,...)] and
**          [sqlite3_prepare_v2(db,zSql,...)] interfaces interpret the
**          text in their zSql parameter as UTF-8.
**
** {F13012} The [sqlite3_prepare16(db,zSql,...)] and
**          [sqlite3_prepare16_v2(db,zSql,...)] interfaces interpret the
**          text in their zSql parameter as UTF-16 in the native byte order.
**
** {F13013} If the nByte argument to [sqlite3_prepare_v2(db,zSql,nByte,...)]
**          and its variants is less than zero, then SQL text is
**          read from zSql is read up to the first zero terminator.
**
** {F13014} If the nByte argument to [sqlite3_prepare_v2(db,zSql,nByte,...)]
**          and its variants is non-negative, then at most nBytes bytes
**          SQL text is read from zSql.
**
** {F13015} In [sqlite3_prepare_v2(db,zSql,N,P,pzTail)] and its variants
**          if the zSql input text contains more than one SQL statement
**          and pzTail is not NULL, then *pzTail is made to point to the
**          first byte past the end of the first SQL statement in zSql.
**          <todo>What does *pzTail point to if there is one statement?</todo>
**
** {F13016} A successful call to [sqlite3_prepare_v2(db,zSql,N,ppStmt,...)]
**          or one of its variants writes into *ppStmt a pointer to a new
**          [prepared statement] or a pointer to NULL
**          if zSql contains nothing other than whitespace or comments. 
**
** {F13019} The [sqlite3_prepare_v2()] interface and its variants return
**          [SQLITE_OK] or an appropriate [error code] upon failure.
**
** {F13021} Before [sqlite3_prepare(db,zSql,nByte,ppStmt,pzTail)] or its
**          variants returns an error (any value other than [SQLITE_OK])
**          it first sets *ppStmt to NULL.
*/
int sqlite3_prepare(
  sqlite3 *db,            /* Database handle */
  const char *zSql,       /* SQL statement, UTF-8 encoded */
  int nByte,              /* Maximum length of zSql in bytes. */
  sqlite3_stmt **ppStmt,  /* OUT: Statement handle */
  const char **pzTail     /* OUT: Pointer to unused portion of zSql */
);
int sqlite3_prepare_v2(
  sqlite3 *db,            /* Database handle */
  const char *zSql,       /* SQL statement, UTF-8 encoded */
  int nByte,              /* Maximum length of zSql in bytes. */
  sqlite3_stmt **ppStmt,  /* OUT: Statement handle */
  const char **pzTail     /* OUT: Pointer to unused portion of zSql */
);
int sqlite3_prepare16(
  sqlite3 *db,            /* Database handle */
  const void *zSql,       /* SQL statement, UTF-16 encoded */
  int nByte,              /* Maximum length of zSql in bytes. */
  sqlite3_stmt **ppStmt,  /* OUT: Statement handle */
  const void **pzTail     /* OUT: Pointer to unused portion of zSql */
);
int sqlite3_prepare16_v2(
  sqlite3 *db,            /* Database handle */
  const void *zSql,       /* SQL statement, UTF-16 encoded */
  int nByte,              /* Maximum length of zSql in bytes. */
  sqlite3_stmt **ppStmt,  /* OUT: Statement handle */
  const void **pzTail     /* OUT: Pointer to unused portion of zSql */
);

/*
** CAPIREF: Retrieving Statement SQL {F13100}
**
** This intereface can be used to retrieve a saved copy of the original
** SQL text used to create a [prepared statement].
**
** INVARIANTS:
**
** {F13101} If the [prepared statement] passed as 
**          the an argument to [sqlite3_sql()] was compiled
**          compiled using either [sqlite3_prepare_v2()] or
**          [sqlite3_prepare16_v2()],
**          then [sqlite3_sql()] function returns a pointer to a
**          zero-terminated string containing a UTF-8 rendering
**          of the original SQL statement.
**
** {F13102} If the [prepared statement] passed as 
**          the an argument to [sqlite3_sql()] was compiled
**          compiled using either [sqlite3_prepare()] or
**          [sqlite3_prepare16()],
**          then [sqlite3_sql()] function returns a NULL pointer.
**
** {F13103} The string returned by [sqlite3_sql(S)] is valid until the
**          [prepared statement] S is deleted using [sqlite3_finalize(S)].
*/
const char *sqlite3_sql(sqlite3_stmt *pStmt);

/*
** CAPI3REF:  Dynamically Typed Value Object  {F15000}
** KEYWORDS: {protected sqlite3_value} {unprotected sqlite3_value}
**
** SQLite uses the sqlite3_value object to represent all values
** that can be stored in a database table.
** SQLite uses dynamic typing for the values it stores.  
** Values stored in sqlite3_value objects can be
** be integers, floating point values, strings, BLOBs, or NULL.
**
** An sqlite3_value object may be either "protected" or "unprotected".
** Some interfaces require a protected sqlite3_value.  Other interfaces
** will accept either a protected or an unprotected sqlite3_value.
** Every interface that accepts sqlite3_value arguments specifies 
** whether or not it requires a protected sqlite3_value.
**
** The terms "protected" and "unprotected" refer to whether or not
** a mutex is held.  A internal mutex is held for a protected
** sqlite3_value object but no mutex is held for an unprotected
** sqlite3_value object.  If SQLite is compiled to be single-threaded
** (with SQLITE_THREADSAFE=0 and with [sqlite3_threadsafe()] returning 0)
** then there is no distinction between
** protected and unprotected sqlite3_value objects and they can be
** used interchangable.  However, for maximum code portability it
** is recommended that applications make the distinction between
** between protected and unprotected sqlite3_value objects even if
** they are single threaded.
**
** The sqlite3_value objects that are passed as parameters into the
** implementation of application-defined SQL functions are protected.
** The sqlite3_value object returned by
** [sqlite3_column_value()] is unprotected.
** Unprotected sqlite3_value objects may only be used with
** [sqlite3_result_value()] and [sqlite3_bind_value()].  All other
** interfaces that use sqlite3_value require protected sqlite3_value objects.
*/
typedef struct Mem sqlite3_value;

/*
** CAPI3REF:  SQL Function Context Object {F16001}
**
** The context in which an SQL function executes is stored in an
** sqlite3_context object.  A pointer to an sqlite3_context
** object is always first parameter to application-defined SQL functions.
*/
typedef struct sqlite3_context sqlite3_context;

/*
** CAPI3REF:  Binding Values To Prepared Statements {F13500}
**
** In the SQL strings input to [sqlite3_prepare_v2()] and its
** variants, literals may be replace by a parameter in one
** of these forms:
**
** <ul>
** <li>  ?
** <li>  ?NNN
** <li>  :VVV
** <li>  @VVV
** <li>  $VVV
** </ul>
**
** In the parameter forms shown above NNN is an integer literal,
** VVV alpha-numeric parameter name.
** The values of these parameters (also called "host parameter names"
** or "SQL parameters")
** can be set using the sqlite3_bind_*() routines defined here.
**
** The first argument to the sqlite3_bind_*() routines always
** is a pointer to the [sqlite3_stmt] object returned from
** [sqlite3_prepare_v2()] or its variants. The second
** argument is the index of the parameter to be set. The
** first parameter has an index of 1.  When the same named
** parameter is used more than once, second and subsequent
** occurrences have the same index as the first occurrence. 
** The index for named parameters can be looked up using the
** [sqlite3_bind_parameter_name()] API if desired.  The index
** for "?NNN" parameters is the value of NNN.
** The NNN value must be between 1 and the compile-time
** parameter SQLITE_MAX_VARIABLE_NUMBER (default value: 999).
**
** The third argument is the value to bind to the parameter.
**
** In those
** routines that have a fourth argument, its value is the number of bytes
** in the parameter.  To be clear: the value is the number of <u>bytes</u>
** in the value, not the number of characters. 
** If the fourth parameter is negative, the length of the string is
** number of bytes up to the first zero terminator.
**
** The fifth argument to sqlite3_bind_blob(), sqlite3_bind_text(), and
** sqlite3_bind_text16() is a destructor used to dispose of the BLOB or
** string after SQLite has finished with it. If the fifth argument is
** the special value [SQLITE_STATIC], then SQLite assumes that the
** information is in static, unmanaged space and does not need to be freed.
** If the fifth argument has the value [SQLITE_TRANSIENT], then
** SQLite makes its own private copy of the data immediately, before
** the sqlite3_bind_*() routine returns.
**
** The sqlite3_bind_zeroblob() routine binds a BLOB of length N that
** is filled with zeros.  A zeroblob uses a fixed amount of memory
** (just an integer to hold it size) while it is being processed.
** Zeroblobs are intended to serve as place-holders for BLOBs whose
** content is later written using 
** [sqlite3_blob_open | increment BLOB I/O] routines. A negative
** value for the zeroblob results in a zero-length BLOB.
**
** The sqlite3_bind_*() routines must be called after
** [sqlite3_prepare_v2()] (and its variants) or [sqlite3_reset()] and
** before [sqlite3_step()].
** Bindings are not cleared by the [sqlite3_reset()] routine.
** Unbound parameters are interpreted as NULL.
**
** These routines return [SQLITE_OK] on success or an error code if
** anything goes wrong.  [SQLITE_RANGE] is returned if the parameter
** index is out of range.  [SQLITE_NOMEM] is returned if malloc fails.
** [SQLITE_MISUSE] might be returned if these routines are called on a
** virtual machine that is the wrong state or which has already been finalized.
** Detection of misuse is unreliable.  Applications should not depend
** on SQLITE_MISUSE returns.  SQLITE_MISUSE is intended to indicate a
** a logic error in the application.  Future versions of SQLite might
** panic rather than return SQLITE_MISUSE.
**
** See also: [sqlite3_bind_parameter_count()],
** [sqlite3_bind_parameter_name()], and
** [sqlite3_bind_parameter_index()].
**
** INVARIANTS:
**
** {F13506} The [sqlite3_prepare | SQL statement compiler] recognizes
**          tokens of the forms "?", "?NNN", "$VVV", ":VVV", and "@VVV"
**          as SQL parameters, where NNN is any sequence of one or more
**          digits and where VVV is any sequence of one or more 
**          alphanumeric characters or "::" optionally followed by
**          a string containing no spaces and contained within parentheses.
**
** {F13509} The initial value of an SQL parameter is NULL.
**
** {F13512} The index of an "?" SQL parameter is one larger than the
**          largest index of SQL parameter to the left, or 1 if
**          the "?" is the leftmost SQL parameter.
**
** {F13515} The index of an "?NNN" SQL parameter is the integer NNN.
**
** {F13518} The index of an ":VVV", "$VVV", or "@VVV" SQL parameter is
**          the same as the index of leftmost occurances of the same
**          parameter, or one more than the largest index over all
**          parameters to the left if this is the first occurrance
**          of this parameter, or 1 if this is the leftmost parameter.
**
** {F13521} The [sqlite3_prepare | SQL statement compiler] fail with
**          an [SQLITE_RANGE] error if the index of an SQL parameter
**          is less than 1 or greater than SQLITE_MAX_VARIABLE_NUMBER.
**
** {F13524} Calls to [sqlite3_bind_text | sqlite3_bind(S,N,V,...)]
**          associate the value V with all SQL parameters having an
**          index of N in the [prepared statement] S.
**
** {F13527} Calls to [sqlite3_bind_text | sqlite3_bind(S,N,...)]
**          override prior calls with the same values of S and N.
**
** {F13530} Bindings established by [sqlite3_bind_text | sqlite3_bind(S,...)]
**          persist across calls to [sqlite3_reset(S)].
**
** {F13533} In calls to [sqlite3_bind_blob(S,N,V,L,D)],
**          [sqlite3_bind_text(S,N,V,L,D)], or
**          [sqlite3_bind_text16(S,N,V,L,D)] SQLite binds the first L
**          bytes of the blob or string pointed to by V, when L
**          is non-negative.
**
** {F13536} In calls to [sqlite3_bind_text(S,N,V,L,D)] or
**          [sqlite3_bind_text16(S,N,V,L,D)] SQLite binds characters
**          from V through the first zero character when L is negative.
**
** {F13539} In calls to [sqlite3_bind_blob(S,N,V,L,D)],
**          [sqlite3_bind_text(S,N,V,L,D)], or
**          [sqlite3_bind_text16(S,N,V,L,D)] when D is the special
**          constant [SQLITE_STATIC], SQLite assumes that the value V
**          is held in static unmanaged space that will not change
**          during the lifetime of the binding.
**
** {F13542} In calls to [sqlite3_bind_blob(S,N,V,L,D)],
**          [sqlite3_bind_text(S,N,V,L,D)], or
**          [sqlite3_bind_text16(S,N,V,L,D)] when D is the special
**          constant [SQLITE_TRANSIENT], the routine makes a 
**          private copy of V value before it returns.
**
** {F13545} In calls to [sqlite3_bind_blob(S,N,V,L,D)],
**          [sqlite3_bind_text(S,N,V,L,D)], or
**          [sqlite3_bind_text16(S,N,V,L,D)] when D is a pointer to
**          a function, SQLite invokes that function to destroy the
**          V value after it has finished using the V value.
**
** {F13548} In calls to [sqlite3_bind_zeroblob(S,N,V,L)] the value bound
**          is a blob of L bytes, or a zero-length blob if L is negative.
**
** {F13551} In calls to [sqlite3_bind_value(S,N,V)] the V argument may
**          be either a [protected sqlite3_value] object or an
**          [unprotected sqlite3_value] object.
*/
int sqlite3_bind_blob(sqlite3_stmt*, int, const void*, int n, void(*)(void*));
int sqlite3_bind_double(sqlite3_stmt*, int, double);
int sqlite3_bind_int(sqlite3_stmt*, int, int);
int sqlite3_bind_int64(sqlite3_stmt*, int, sqlite3_int64);
int sqlite3_bind_null(sqlite3_stmt*, int);
int sqlite3_bind_text(sqlite3_stmt*, int, const char*, int n, void(*)(void*));
int sqlite3_bind_text16(sqlite3_stmt*, int, const void*, int, void(*)(void*));
int sqlite3_bind_value(sqlite3_stmt*, int, const sqlite3_value*);
int sqlite3_bind_zeroblob(sqlite3_stmt*, int, int n);

/*
** CAPI3REF: Number Of SQL Parameters {F13600}
**
** This routine can be used to find the number of SQL parameters
** in a prepared statement.  SQL parameters are tokens of the
** form "?", "?NNN", ":AAA", "$AAA", or "@AAA" that serve as
** place-holders for values that are [sqlite3_bind_blob | bound]
** to the parameters at a later time.
**
** This routine actually returns the index of the largest parameter.
** For all forms except ?NNN, this will correspond to the number of
** unique parameters.  If parameters of the ?NNN are used, there may
** be gaps in the list.
**
** See also: [sqlite3_bind_blob|sqlite3_bind()],
** [sqlite3_bind_parameter_name()], and
** [sqlite3_bind_parameter_index()].
**
** INVARIANTS:
**
** {F13601} The [sqlite3_bind_parameter_count(S)] interface returns
**          the largest index of all SQL parameters in the
**          [prepared statement] S, or 0 if S
**          contains no SQL parameters.
*/
int sqlite3_bind_parameter_count(sqlite3_stmt*);

/*
** CAPI3REF: Name Of A Host Parameter {F13620}
**
** This routine returns a pointer to the name of the n-th
** SQL parameter in a [prepared statement].
** SQL parameters of the form "?NNN" or ":AAA" or "@AAA" or "$AAA"
** have a name which is the string "?NNN" or ":AAA" or "@AAA" or "$AAA"
** respectively.
** In other words, the initial ":" or "$" or "@" or "?"
** is included as part of the name.
** Parameters of the form "?" without a following integer have no name.
**
** The first host parameter has an index of 1, not 0.
**
** If the value n is out of range or if the n-th parameter is
** nameless, then NULL is returned.  The returned string is
** always in the UTF-8 encoding even if the named parameter was
** originally specified as UTF-16 in [sqlite3_prepare16()] or
** [sqlite3_prepare16_v2()].
**
** See also: [sqlite3_bind_blob|sqlite3_bind()],
** [sqlite3_bind_parameter_count()], and
** [sqlite3_bind_parameter_index()].
**
** INVARIANTS:
**
** {F13621} The [sqlite3_bind_parameter_name(S,N)] interface returns
**          a UTF-8 rendering of the name of the SQL parameter in
**          [prepared statement] S having index N, or
**          NULL if there is no SQL parameter with index N or if the
**          parameter with index N is an anonymous parameter "?".
*/
const char *sqlite3_bind_parameter_name(sqlite3_stmt*, int);

/*
** CAPI3REF: Index Of A Parameter With A Given Name {F13640}
**
** Return the index of an SQL parameter given its name.  The
** index value returned is suitable for use as the second
** parameter to [sqlite3_bind_blob|sqlite3_bind()].  A zero
** is returned if no matching parameter is found.  The parameter
** name must be given in UTF-8 even if the original statement
** was prepared from UTF-16 text using [sqlite3_prepare16_v2()].
**
** See also: [sqlite3_bind_blob|sqlite3_bind()],
** [sqlite3_bind_parameter_count()], and
** [sqlite3_bind_parameter_index()].
**
** INVARIANTS:
**
** {F13641} The [sqlite3_bind_parameter_index(S,N)] interface returns
**          the index of SQL parameter in [prepared statement]
**          S whose name matches the UTF-8 string N, or 0 if there is
**          no match.
*/
int sqlite3_bind_parameter_index(sqlite3_stmt*, const char *zName);

/*
** CAPI3REF: Reset All Bindings On A Prepared Statement {F13660}
**
** Contrary to the intuition of many, [sqlite3_reset()] does not
** reset the [sqlite3_bind_blob | bindings] on a 
** [prepared statement].  Use this routine to
** reset all host parameters to NULL.
**
** INVARIANTS:
**
** {F13661} The [sqlite3_clear_bindings(S)] interface resets all
**          SQL parameter bindings in [prepared statement] S
**          back to NULL.
*/
int sqlite3_clear_bindings(sqlite3_stmt*);

/*
** CAPI3REF: Number Of Columns In A Result Set {F13710}
**
** Return the number of columns in the result set returned by the 
** [prepared statement]. This routine returns 0
** if pStmt is an SQL statement that does not return data (for 
** example an UPDATE).
**
** INVARIANTS:
**
** {F13711} The [sqlite3_column_count(S)] interface returns the number of
**          columns in the result set generated by the
**          [prepared statement] S, or 0 if S does not generate
**          a result set.
*/
int sqlite3_column_count(sqlite3_stmt *pStmt);

/*
** CAPI3REF: Column Names In A Result Set {F13720}
**
** These routines return the name assigned to a particular column
** in the result set of a SELECT statement.  The sqlite3_column_name()
** interface returns a pointer to a zero-terminated UTF8 string
** and sqlite3_column_name16() returns a pointer to a zero-terminated
** UTF16 string.  The first parameter is the
** [prepared statement] that implements the SELECT statement.
** The second parameter is the column number.  The left-most column is
** number 0.
**
** The returned string pointer is valid until either the 
** [prepared statement] is destroyed by [sqlite3_finalize()]
** or until the next call sqlite3_column_name() or sqlite3_column_name16()
** on the same column.
**
** If sqlite3_malloc() fails during the processing of either routine
** (for example during a conversion from UTF-8 to UTF-16) then a
** NULL pointer is returned.
**
** The name of a result column is the value of the "AS" clause for
** that column, if there is an AS clause.  If there is no AS clause
** then the name of the column is unspecified and may change from
** one release of SQLite to the next.
**
** INVARIANTS:
**
** {F13721} A successful invocation of the [sqlite3_column_name(S,N)]
**          interface returns the name
**          of the Nth column (where 0 is the left-most column) for the
**          result set of [prepared statement] S as a
**          zero-terminated UTF-8 string.
**
** {F13723} A successful invocation of the [sqlite3_column_name16(S,N)]
**          interface returns the name
**          of the Nth column (where 0 is the left-most column) for the
**          result set of [prepared statement] S as a
**          zero-terminated UTF-16 string in the native byte order.
**
** {F13724} The [sqlite3_column_name()] and [sqlite3_column_name16()]
**          interfaces return a NULL pointer if they are unable to
**          allocate memory memory to hold there normal return strings.
**
** {F13725} If the N parameter to [sqlite3_column_name(S,N)] or
**          [sqlite3_column_name16(S,N)] is out of range, then the
**          interfaces returns a NULL pointer.
** 
** {F13726} The strings returned by [sqlite3_column_name(S,N)] and
**          [sqlite3_column_name16(S,N)] are valid until the next
**          call to either routine with the same S and N parameters
**          or until [sqlite3_finalize(S)] is called.
**
** {F13727} When a result column of a [SELECT] statement contains
**          an AS clause, the name of that column is the indentifier
**          to the right of the AS keyword.
*/
const char *sqlite3_column_name(sqlite3_stmt*, int N);
const void *sqlite3_column_name16(sqlite3_stmt*, int N);

/*
** CAPI3REF: Source Of Data In A Query Result {F13740}
**
** These routines provide a means to determine what column of what
** table in which database a result of a SELECT statement comes from.
** The name of the database or table or column can be returned as
** either a UTF8 or UTF16 string.  The _database_ routines return
** the database name, the _table_ routines return the table name, and
** the origin_ routines return the column name.
** The returned string is valid until
** the [prepared statement] is destroyed using
** [sqlite3_finalize()] or until the same information is requested
** again in a different encoding.
**
** The names returned are the original un-aliased names of the
** database, table, and column.
**
** The first argument to the following calls is a [prepared statement].
** These functions return information about the Nth column returned by 
** the statement, where N is the second function argument.
**
** If the Nth column returned by the statement is an expression
** or subquery and is not a column value, then all of these functions
** return NULL.  These routine might also return NULL if a memory
** allocation error occurs.  Otherwise, they return the 
** name of the attached database, table and column that query result
** column was extracted from.
**
** As with all other SQLite APIs, those postfixed with "16" return
** UTF-16 encoded strings, the other functions return UTF-8. {END}
**
** These APIs are only available if the library was compiled with the 
** SQLITE_ENABLE_COLUMN_METADATA preprocessor symbol defined.
**
** {U13751}
** If two or more threads call one or more of these routines against the same
** prepared statement and column at the same time then the results are
** undefined.
**
** INVARIANTS:
**
** {F13741} The [sqlite3_column_database_name(S,N)] interface returns either
**          the UTF-8 zero-terminated name of the database from which the 
**          Nth result column of [prepared statement] S 
**          is extracted, or NULL if the the Nth column of S is a
**          general expression or if unable to allocate memory
**          to store the name.
**          
** {F13742} The [sqlite3_column_database_name16(S,N)] interface returns either
**          the UTF-16 native byte order
**          zero-terminated name of the database from which the 
**          Nth result column of [prepared statement] S 
**          is extracted, or NULL if the the Nth column of S is a
**          general expression or if unable to allocate memory
**          to store the name.
**          
** {F13743} The [sqlite3_column_table_name(S,N)] interface returns either
**          the UTF-8 zero-terminated name of the table from which the 
**          Nth result column of [prepared statement] S 
**          is extracted, or NULL if the the Nth column of S is a
**          general expression or if unable to allocate memory
**          to store the name.
**          
** {F13744} The [sqlite3_column_table_name16(S,N)] interface returns either
**          the UTF-16 native byte order
**          zero-terminated name of the table from which the 
**          Nth result column of [prepared statement] S 
**          is extracted, or NULL if the the Nth column of S is a
**          general expression or if unable to allocate memory
**          to store the name.
**          
** {F13745} The [sqlite3_column_origin_name(S,N)] interface returns either
**          the UTF-8 zero-terminated name of the table column from which the 
**          Nth result column of [prepared statement] S 
**          is extracted, or NULL if the the Nth column of S is a
**          general expression or if unable to allocate memory
**          to store the name.
**          
** {F13746} The [sqlite3_column_origin_name16(S,N)] interface returns either
**          the UTF-16 native byte order
**          zero-terminated name of the table column from which the 
**          Nth result column of [prepared statement] S 
**          is extracted, or NULL if the the Nth column of S is a
**          general expression or if unable to allocate memory
**          to store the name.
**          
** {F13748} The return values from
**          [sqlite3_column_database_name|column metadata interfaces]
**          are valid
**          for the lifetime of the [prepared statement]
**          or until the encoding is changed by another metadata
**          interface call for the same prepared statement and column.
**
** LIMITATIONS:
**
** {U13751} If two or more threads call one or more
**          [sqlite3_column_database_name|column metadata interfaces]
**          the same [prepared statement] and result column
**          at the same time then the results are undefined.
*/
const char *sqlite3_column_database_name(sqlite3_stmt*,int);
const void *sqlite3_column_database_name16(sqlite3_stmt*,int);
const char *sqlite3_column_table_name(sqlite3_stmt*,int);
const void *sqlite3_column_table_name16(sqlite3_stmt*,int);
const char *sqlite3_column_origin_name(sqlite3_stmt*,int);
const void *sqlite3_column_origin_name16(sqlite3_stmt*,int);

/*
** CAPI3REF: Declared Datatype Of A Query Result {F13760}
**
** The first parameter is a [prepared statement]. 
** If this statement is a SELECT statement and the Nth column of the 
** returned result set of that SELECT is a table column (not an
** expression or subquery) then the declared type of the table
** column is returned.  If the Nth column of the result set is an
** expression or subquery, then a NULL pointer is returned.
** The returned string is always UTF-8 encoded.  {END} 
** For example, in the database schema:
**
** CREATE TABLE t1(c1 VARIANT);
**
** And the following statement compiled:
**
** SELECT c1 + 1, c1 FROM t1;
**
** Then this routine would return the string "VARIANT" for the second
** result column (i==1), and a NULL pointer for the first result column
** (i==0).
**
** SQLite uses dynamic run-time typing.  So just because a column
** is declared to contain a particular type does not mean that the
** data stored in that column is of the declared type.  SQLite is
** strongly typed, but the typing is dynamic not static.  Type
** is associated with individual values, not with the containers
** used to hold those values.
**
** INVARIANTS:
**
** {F13761}  A successful call to [sqlite3_column_decltype(S,N)]
**           returns a zero-terminated UTF-8 string containing the
**           the declared datatype of the table column that appears
**           as the Nth column (numbered from 0) of the result set to the
**           [prepared statement] S.
**
** {F13762}  A successful call to [sqlite3_column_decltype16(S,N)]
**           returns a zero-terminated UTF-16 native byte order string
**           containing the declared datatype of the table column that appears
**           as the Nth column (numbered from 0) of the result set to the
**           [prepared statement] S.
**
** {F13763}  If N is less than 0 or N is greater than or equal to
**           the number of columns in [prepared statement] S
**           or if the Nth column of S is an expression or subquery rather
**           than a table column or if a memory allocation failure
**           occurs during encoding conversions, then
**           calls to [sqlite3_column_decltype(S,N)] or
**           [sqlite3_column_decltype16(S,N)] return NULL.
*/
const char *sqlite3_column_decltype(sqlite3_stmt*,int);
const void *sqlite3_column_decltype16(sqlite3_stmt*,int);

/* 
** CAPI3REF:  Evaluate An SQL Statement {F13200}
**
** After an [prepared statement] has been prepared with a call
** to either [sqlite3_prepare_v2()] or [sqlite3_prepare16_v2()] or to one of
** the legacy interfaces [sqlite3_prepare()] or [sqlite3_prepare16()],
** then this function must be called one or more times to evaluate the 
** statement.
**
** The details of the behavior of this sqlite3_step() interface depend
** on whether the statement was prepared using the newer "v2" interface
** [sqlite3_prepare_v2()] and [sqlite3_prepare16_v2()] or the older legacy
** interface [sqlite3_prepare()] and [sqlite3_prepare16()].  The use of the
** new "v2" interface is recommended for new applications but the legacy
** interface will continue to be supported.
**
** In the legacy interface, the return value will be either [SQLITE_BUSY], 
** [SQLITE_DONE], [SQLITE_ROW], [SQLITE_ERROR], or [SQLITE_MISUSE].
** With the "v2" interface, any of the other [SQLITE_OK | result code]
** or [SQLITE_IOERR_READ | extended result code] might be returned as
** well.
**
** [SQLITE_BUSY] means that the database engine was unable to acquire the
** database locks it needs to do its job.  If the statement is a COMMIT
** or occurs outside of an explicit transaction, then you can retry the
** statement.  If the statement is not a COMMIT and occurs within a
** explicit transaction then you should rollback the transaction before
** continuing.
**
** [SQLITE_DONE] means that the statement has finished executing
** successfully.  sqlite3_step() should not be called again on this virtual
** machine without first calling [sqlite3_reset()] to reset the virtual
** machine back to its initial state.
**
** If the SQL statement being executed returns any data, then 
** [SQLITE_ROW] is returned each time a new row of data is ready
** for processing by the caller. The values may be accessed using
** the [sqlite3_column_int | column access functions].
** sqlite3_step() is called again to retrieve the next row of data.
** 
** [SQLITE_ERROR] means that a run-time error (such as a constraint
** violation) has occurred.  sqlite3_step() should not be called again on
** the VM. More information may be found by calling [sqlite3_errmsg()].
** With the legacy interface, a more specific error code (example:
** [SQLITE_INTERRUPT], [SQLITE_SCHEMA], [SQLITE_CORRUPT], and so forth)
** can be obtained by calling [sqlite3_reset()] on the
** [prepared statement].  In the "v2" interface,
** the more specific error code is returned directly by sqlite3_step().
**
** [SQLITE_MISUSE] means that the this routine was called inappropriately.
** Perhaps it was called on a [prepared statement] that has
** already been [sqlite3_finalize | finalized] or on one that had 
** previously returned [SQLITE_ERROR] or [SQLITE_DONE].  Or it could
** be the case that the same database connection is being used by two or
** more threads at the same moment in time.
**
** <b>Goofy Interface Alert:</b>
** In the legacy interface, 
** the sqlite3_step() API always returns a generic error code,
** [SQLITE_ERROR], following any error other than [SQLITE_BUSY]
** and [SQLITE_MISUSE].  You must call [sqlite3_reset()] or
** [sqlite3_finalize()] in order to find one of the specific
** [error codes] that better describes the error.
** We admit that this is a goofy design.  The problem has been fixed
** with the "v2" interface.  If you prepare all of your SQL statements
** using either [sqlite3_prepare_v2()] or [sqlite3_prepare16_v2()] instead
** of the legacy [sqlite3_prepare()] and [sqlite3_prepare16()], then the 
** more specific [error codes] are returned directly
** by sqlite3_step().  The use of the "v2" interface is recommended.
**
** INVARIANTS:
**
** {F13202}  If [prepared statement] S is ready to be
**           run, then [sqlite3_step(S)] advances that prepared statement
**           until to completion or until it is ready to return another
**           row of the result set or an interrupt or run-time error occurs.
**
** {F15304}  When a call to [sqlite3_step(S)] causes the 
**           [prepared statement] S to run to completion,
**           the function returns [SQLITE_DONE].
**
** {F15306}  When a call to [sqlite3_step(S)] stops because it is ready
**           to return another row of the result set, it returns
**           [SQLITE_ROW].
**
** {F15308}  If a call to [sqlite3_step(S)] encounters an
**           [sqlite3_interrupt|interrupt] or a run-time error,
**           it returns an appropraite error code that is not one of
**           [SQLITE_OK], [SQLITE_ROW], or [SQLITE_DONE].
**
** {F15310}  If an [sqlite3_interrupt|interrupt] or run-time error
**           occurs during a call to [sqlite3_step(S)]
**           for a [prepared statement] S created using
**           legacy interfaces [sqlite3_prepare()] or
**           [sqlite3_prepare16()] then the function returns either
**           [SQLITE_ERROR], [SQLITE_BUSY], or [SQLITE_MISUSE].
*/
int sqlite3_step(sqlite3_stmt*);

/*
** CAPI3REF: Number of columns in a result set {F13770}
**
** Return the number of values in the current row of the result set.
**
** INVARIANTS:
**
** {F13771}  After a call to [sqlite3_step(S)] that returns
**           [SQLITE_ROW], the [sqlite3_data_count(S)] routine
**           will return the same value as the
**           [sqlite3_column_count(S)] function.
**
** {F13772}  After [sqlite3_step(S)] has returned any value other than
**           [SQLITE_ROW] or before [sqlite3_step(S)] has been 
**           called on the [prepared statement] for
**           the first time since it was [sqlite3_prepare|prepared]
**           or [sqlite3_reset|reset], the [sqlite3_data_count(S)]
**           routine returns zero.
*/
int sqlite3_data_count(sqlite3_stmt *pStmt);

/*
** CAPI3REF: Fundamental Datatypes {F10265}
** KEYWORDS: SQLITE_TEXT
**
** {F10266}Every value in SQLite has one of five fundamental datatypes:
**
** <ul>
** <li> 64-bit signed integer
** <li> 64-bit IEEE floating point number
** <li> string
** <li> BLOB
** <li> NULL
** </ul> {END}
**
** These constants are codes for each of those types.
**
** Note that the SQLITE_TEXT constant was also used in SQLite version 2
** for a completely different meaning.  Software that links against both
** SQLite version 2 and SQLite version 3 should use SQLITE3_TEXT not
** SQLITE_TEXT.
*/
#define SQLITE_INTEGER  1
#define SQLITE_FLOAT    2
#define SQLITE_BLOB     4
#define SQLITE_NULL     5
#ifdef SQLITE_TEXT
# undef SQLITE_TEXT
#else
# define SQLITE_TEXT     3
#endif
#define SQLITE3_TEXT     3

/*
** CAPI3REF: Results Values From A Query {F13800}
**
** These routines form the "result set query" interface.
**
** These routines return information about
** a single column of the current result row of a query.  In every
** case the first argument is a pointer to the 
** [prepared statement] that is being
** evaluated (the [sqlite3_stmt*] that was returned from 
** [sqlite3_prepare_v2()] or one of its variants) and
** the second argument is the index of the column for which information 
** should be returned.  The left-most column of the result set
** has an index of 0.
**
** If the SQL statement is not currently point to a valid row, or if the
** the column index is out of range, the result is undefined. 
** These routines may only be called when the most recent call to
** [sqlite3_step()] has returned [SQLITE_ROW] and neither
** [sqlite3_reset()] nor [sqlite3_finalize()] has been call subsequently.
** If any of these routines are called after [sqlite3_reset()] or
** [sqlite3_finalize()] or after [sqlite3_step()] has returned
** something other than [SQLITE_ROW], the results are undefined.
** If [sqlite3_step()] or [sqlite3_reset()] or [sqlite3_finalize()]
** are called from a different thread while any of these routines
** are pending, then the results are undefined.  
**
** The sqlite3_column_type() routine returns 
** [SQLITE_INTEGER | datatype code] for the initial data type
** of the result column.  The returned value is one of [SQLITE_INTEGER],
** [SQLITE_FLOAT], [SQLITE_TEXT], [SQLITE_BLOB], or [SQLITE_NULL].  The value
** returned by sqlite3_column_type() is only meaningful if no type
** conversions have occurred as described below.  After a type conversion,
** the value returned by sqlite3_column_type() is undefined.  Future
** versions of SQLite may change the behavior of sqlite3_column_type()
** following a type conversion.
**
** If the result is a BLOB or UTF-8 string then the sqlite3_column_bytes() 
** routine returns the number of bytes in that BLOB or string.
** If the result is a UTF-16 string, then sqlite3_column_bytes() converts
** the string to UTF-8 and then returns the number of bytes.
** If the result is a numeric value then sqlite3_column_bytes() uses
** [sqlite3_snprintf()] to convert that value to a UTF-8 string and returns
** the number of bytes in that string.
** The value returned does not include the zero terminator at the end
** of the string.  For clarity: the value returned is the number of
** bytes in the string, not the number of characters.
**
** Strings returned by sqlite3_column_text() and sqlite3_column_text16(),
** even empty strings, are always zero terminated.  The return
** value from sqlite3_column_blob() for a zero-length blob is an arbitrary
** pointer, possibly even a NULL pointer.
**
** The sqlite3_column_bytes16() routine is similar to sqlite3_column_bytes()
** but leaves the result in UTF-16 in native byte order instead of UTF-8.  
** The zero terminator is not included in this count.
**
** The object returned by [sqlite3_column_value()] is an
** [unprotected sqlite3_value] object.  An unprotected sqlite3_value object
** may only be used with [sqlite3_bind_value()] and [sqlite3_result_value()].
** If the [unprotected sqlite3_value] object returned by
** [sqlite3_column_value()] is used in any other way, including calls
** to routines like 
** [sqlite3_value_int()], [sqlite3_value_text()], or [sqlite3_value_bytes()],
** then the behavior is undefined.
**
** These routines attempt to convert the value where appropriate.  For
** example, if the internal representation is FLOAT and a text result
** is requested, [sqlite3_snprintf()] is used internally to do the conversion
** automatically.  The following table details the conversions that
** are applied:
**
** <blockquote>
** <table border="1">
** <tr><th> Internal<br>Type <th> Requested<br>Type <th>  Conversion
**
** <tr><td>  NULL    <td> INTEGER   <td> Result is 0
** <tr><td>  NULL    <td>  FLOAT    <td> Result is 0.0
** <tr><td>  NULL    <td>   TEXT    <td> Result is NULL pointer
** <tr><td>  NULL    <td>   BLOB    <td> Result is NULL pointer
** <tr><td> INTEGER  <td>  FLOAT    <td> Convert from integer to float
** <tr><td> INTEGER  <td>   TEXT    <td> ASCII rendering of the integer
** <tr><td> INTEGER  <td>   BLOB    <td> Same as for INTEGER->TEXT
** <tr><td>  FLOAT   <td> INTEGER   <td> Convert from float to integer
** <tr><td>  FLOAT   <td>   TEXT    <td> ASCII rendering of the float
** <tr><td>  FLOAT   <td>   BLOB    <td> Same as FLOAT->TEXT
** <tr><td>  TEXT    <td> INTEGER   <td> Use atoi()
** <tr><td>  TEXT    <td>  FLOAT    <td> Use atof()
** <tr><td>  TEXT    <td>   BLOB    <td> No change
** <tr><td>  BLOB    <td> INTEGER   <td> Convert to TEXT then use atoi()
** <tr><td>  BLOB    <td>  FLOAT    <td> Convert to TEXT then use atof()
** <tr><td>  BLOB    <td>   TEXT    <td> Add a zero terminator if needed
** </table>
** </blockquote>
**
** The table above makes reference to standard C library functions atoi()
** and atof().  SQLite does not really use these functions.  It has its
** on equavalent internal routines.  The atoi() and atof() names are
** used in the table for brevity and because they are familiar to most
** C programmers.
**
** Note that when type conversions occur, pointers returned by prior
** calls to sqlite3_column_blob(), sqlite3_column_text(), and/or
** sqlite3_column_text16() may be invalidated. 
** Type conversions and pointer invalidations might occur
** in the following cases:
**
** <ul>
** <li><p>  The initial content is a BLOB and sqlite3_column_text() 
**          or sqlite3_column_text16() is called.  A zero-terminator might
**          need to be added to the string.</p></li>
**
** <li><p>  The initial content is UTF-8 text and sqlite3_column_bytes16() or
**          sqlite3_column_text16() is called.  The content must be converted
**          to UTF-16.</p></li>
**
** <li><p>  The initial content is UTF-16 text and sqlite3_column_bytes() or
**          sqlite3_column_text() is called.  The content must be converted
**          to UTF-8.</p></li>
** </ul>
**
** Conversions between UTF-16be and UTF-16le are always done in place and do
** not invalidate a prior pointer, though of course the content of the buffer
** that the prior pointer points to will have been modified.  Other kinds
** of conversion are done in place when it is possible, but sometime it is
** not possible and in those cases prior pointers are invalidated.  
**
** The safest and easiest to remember policy is to invoke these routines
** in one of the following ways:
**
**  <ul>
**  <li>sqlite3_column_text() followed by sqlite3_column_bytes()</li>
**  <li>sqlite3_column_blob() followed by sqlite3_column_bytes()</li>
**  <li>sqlite3_column_text16() followed by sqlite3_column_bytes16()</li>
**  </ul>
**
** In other words, you should call sqlite3_column_text(), sqlite3_column_blob(),
** or sqlite3_column_text16() first to force the result into the desired
** format, then invoke sqlite3_column_bytes() or sqlite3_column_bytes16() to
** find the size of the result.  Do not mix call to sqlite3_column_text() or
** sqlite3_column_blob() with calls to sqlite3_column_bytes16().  And do not
** mix calls to sqlite3_column_text16() with calls to sqlite3_column_bytes().
**
** The pointers returned are valid until a type conversion occurs as
** described above, or until [sqlite3_step()] or [sqlite3_reset()] or
** [sqlite3_finalize()] is called.  The memory space used to hold strings
** and blobs is freed automatically.  Do <b>not</b> pass the pointers returned
** [sqlite3_column_blob()], [sqlite3_column_text()], etc. into 
** [sqlite3_free()].
**
** If a memory allocation error occurs during the evaluation of any
** of these routines, a default value is returned.  The default value
** is either the integer 0, the floating point number 0.0, or a NULL
** pointer.  Subsequent calls to [sqlite3_errcode()] will return
** [SQLITE_NOMEM].
**
** INVARIANTS:
**
** {F13803} The [sqlite3_column_blob(S,N)] interface converts the
**          Nth column in the current row of the result set for
**          [prepared statement] S into a blob and then returns a
**          pointer to the converted value.
**
** {F13806} The [sqlite3_column_bytes(S,N)] interface returns the
**          number of bytes in the blob or string (exclusive of the
**          zero terminator on the string) that was returned by the
**          most recent call to [sqlite3_column_blob(S,N)] or
**          [sqlite3_column_text(S,N)].
**
** {F13809} The [sqlite3_column_bytes16(S,N)] interface returns the
**          number of bytes in the string (exclusive of the
**          zero terminator on the string) that was returned by the
**          most recent call to [sqlite3_column_text16(S,N)].
**
** {F13812} The [sqlite3_column_double(S,N)] interface converts the
**          Nth column in the current row of the result set for
**          [prepared statement] S into a floating point value and
**          returns a copy of that value.
**
** {F13815} The [sqlite3_column_int(S,N)] interface converts the
**          Nth column in the current row of the result set for
**          [prepared statement] S into a 64-bit signed integer and
**          returns the lower 32 bits of that integer.
**
** {F13818} The [sqlite3_column_int64(S,N)] interface converts the
**          Nth column in the current row of the result set for
**          [prepared statement] S into a 64-bit signed integer and
**          returns a copy of that integer.
**
** {F13821} The [sqlite3_column_text(S,N)] interface converts the
**          Nth column in the current row of the result set for
**          [prepared statement] S into a zero-terminated UTF-8 
**          string and returns a pointer to that string.
**
** {F13824} The [sqlite3_column_text16(S,N)] interface converts the
**          Nth column in the current row of the result set for
**          [prepared statement] S into a zero-terminated 2-byte
**          aligned UTF-16 native byte order
**          string and returns a pointer to that string.
**
** {F13827} The [sqlite3_column_type(S,N)] interface returns
**          one of [SQLITE_NULL], [SQLITE_INTEGER], [SQLITE_FLOAT],
**          [SQLITE_TEXT], or [SQLITE_BLOB] as appropriate for
**          the Nth column in the current row of the result set for
**          [prepared statement] S.
**
** {F13830} The [sqlite3_column_value(S,N)] interface returns a
**          pointer to an [unprotected sqlite3_value] object for the
**          Nth column in the current row of the result set for
**          [prepared statement] S.
*/
const void *sqlite3_column_blob(sqlite3_stmt*, int iCol);
int sqlite3_column_bytes(sqlite3_stmt*, int iCol);
int sqlite3_column_bytes16(sqlite3_stmt*, int iCol);
double sqlite3_column_double(sqlite3_stmt*, int iCol);
int sqlite3_column_int(sqlite3_stmt*, int iCol);
sqlite3_int64 sqlite3_column_int64(sqlite3_stmt*, int iCol);
const unsigned char *sqlite3_column_text(sqlite3_stmt*, int iCol);
const void *sqlite3_column_text16(sqlite3_stmt*, int iCol);
int sqlite3_column_type(sqlite3_stmt*, int iCol);
sqlite3_value *sqlite3_column_value(sqlite3_stmt*, int iCol);

/*
** CAPI3REF: Destroy A Prepared Statement Object {F13300}
**
** The sqlite3_finalize() function is called to delete a 
** [prepared statement]. If the statement was
** executed successfully, or not executed at all, then SQLITE_OK is returned.
** If execution of the statement failed then an 
** [error code] or [extended error code]
** is returned. 
**
** This routine can be called at any point during the execution of the
** [prepared statement].  If the virtual machine has not 
** completed execution when this routine is called, that is like
** encountering an error or an interrupt.  (See [sqlite3_interrupt()].) 
** Incomplete updates may be rolled back and transactions cancelled,  
** depending on the circumstances, and the 
** [error code] returned will be [SQLITE_ABORT].
**
** INVARIANTS:
**
** {F11302} The [sqlite3_finalize(S)] interface destroys the
**          [prepared statement] S and releases all
**          memory and file resources held by that object.
**
** {F11304} If the most recent call to [sqlite3_step(S)] for the
**          [prepared statement] S returned an error,
**          then [sqlite3_finalize(S)] returns that same error.
*/
int sqlite3_finalize(sqlite3_stmt *pStmt);

/*
** CAPI3REF: Reset A Prepared Statement Object {F13330}
**
** The sqlite3_reset() function is called to reset a 
** [prepared statement] object.
** back to its initial state, ready to be re-executed.
** Any SQL statement variables that had values bound to them using
** the [sqlite3_bind_blob | sqlite3_bind_*() API] retain their values.
** Use [sqlite3_clear_bindings()] to reset the bindings.
**
** {F11332} The [sqlite3_reset(S)] interface resets the [prepared statement] S
**          back to the beginning of its program.
**
** {F11334} If the most recent call to [sqlite3_step(S)] for 
**          [prepared statement] S returned [SQLITE_ROW] or [SQLITE_DONE],
**          or if [sqlite3_step(S)] has never before been called on S,
**          then [sqlite3_reset(S)] returns [SQLITE_OK].
**
** {F11336} If the most recent call to [sqlite3_step(S)] for
**          [prepared statement] S indicated an error, then
**          [sqlite3_reset(S)] returns an appropriate [error code].
**
** {F11338} The [sqlite3_reset(S)] interface does not change the values
**          of any [sqlite3_bind_blob|bindings] on [prepared statement] S.
*/
int sqlite3_reset(sqlite3_stmt *pStmt);

/*
** CAPI3REF: Create Or Redefine SQL Functions {F16100}
** KEYWORDS: {function creation routines} 
**
** These two functions (collectively known as
** "function creation routines") are used to add SQL functions or aggregates
** or to redefine the behavior of existing SQL functions or aggregates.  The
** difference only between the two is that the second parameter, the
** name of the (scalar) function or aggregate, is encoded in UTF-8 for
** sqlite3_create_function() and UTF-16 for sqlite3_create_function16().
**
** The first parameter is the [database connection] to which the SQL
** function is to be added.  If a single
** program uses more than one [database connection] internally, then SQL
** functions must be added individually to each [database connection].
**
** The second parameter is the name of the SQL function to be created
** or redefined.
** The length of the name is limited to 255 bytes, exclusive of the 
** zero-terminator.  Note that the name length limit is in bytes, not
** characters.  Any attempt to create a function with a longer name
** will result in an SQLITE_ERROR error.
**
** The third parameter is the number of arguments that the SQL function or
** aggregate takes. If this parameter is negative, then the SQL function or
** aggregate may take any number of arguments.
**
** The fourth parameter, eTextRep, specifies what 
** [SQLITE_UTF8 | text encoding] this SQL function prefers for
** its parameters.  Any SQL function implementation should be able to work
** work with UTF-8, UTF-16le, or UTF-16be.  But some implementations may be
** more efficient with one encoding than another.  It is allowed to
** invoke sqlite3_create_function() or sqlite3_create_function16() multiple
** times with the same function but with different values of eTextRep.
** When multiple implementations of the same function are available, SQLite
** will pick the one that involves the least amount of data conversion.
** If there is only a single implementation which does not care what
** text encoding is used, then the fourth argument should be
** [SQLITE_ANY].
**
** The fifth parameter is an arbitrary pointer.  The implementation
** of the function can gain access to this pointer using
** [sqlite3_user_data()].
**
** The seventh, eighth and ninth parameters, xFunc, xStep and xFinal, are
** pointers to C-language functions that implement the SQL
** function or aggregate. A scalar SQL function requires an implementation of
** the xFunc callback only, NULL pointers should be passed as the xStep
** and xFinal parameters. An aggregate SQL function requires an implementation
** of xStep and xFinal and NULL should be passed for xFunc. To delete an
** existing SQL function or aggregate, pass NULL for all three function
** callback.
**
** It is permitted to register multiple implementations of the same
** functions with the same name but with either differing numbers of
** arguments or differing perferred text encodings.  SQLite will use
** the implementation most closely matches the way in which the
** SQL function is used.
**
** INVARIANTS:
**
** {F16103} The [sqlite3_create_function16()] interface behaves exactly
**          like [sqlite3_create_function()] in every way except that it
**          interprets the zFunctionName argument as
**          zero-terminated UTF-16 native byte order instead of as a
**          zero-terminated UTF-8.
**
** {F16106} A successful invocation of
**          the [sqlite3_create_function(D,X,N,E,...)] interface registers
**          or replaces callback functions in [database connection] D
**          used to implement the SQL function named X with N parameters
**          and having a perferred text encoding of E.
**
** {F16109} A successful call to [sqlite3_create_function(D,X,N,E,P,F,S,L)]
**          replaces the P, F, S, and L values from any prior calls with
**          the same D, X, N, and E values.
**
** {F16112} The [sqlite3_create_function(D,X,...)] interface fails with
**          a return code of [SQLITE_ERROR] if the SQL function name X is
**          longer than 255 bytes exclusive of the zero terminator.
**
** {F16118} Either F must be NULL and S and L are non-NULL or else F
**          is non-NULL and S and L are NULL, otherwise
**          [sqlite3_create_function(D,X,N,E,P,F,S,L)] returns [SQLITE_ERROR].
**
** {F16121} The [sqlite3_create_function(D,...)] interface fails with an
**          error code of [SQLITE_BUSY] if there exist [prepared statements]
**          associated with the [database connection] D.
**
** {F16124} The [sqlite3_create_function(D,X,N,...)] interface fails with an
**          error code of [SQLITE_ERROR] if parameter N (specifying the number
**          of arguments to the SQL function being registered) is less
**          than -1 or greater than 127.
**
** {F16127} When N is non-negative, the [sqlite3_create_function(D,X,N,...)]
**          interface causes callbacks to be invoked for the SQL function
**          named X when the number of arguments to the SQL function is
**          exactly N.
**
** {F16130} When N is -1, the [sqlite3_create_function(D,X,N,...)]
**          interface causes callbacks to be invoked for the SQL function
**          named X with any number of arguments.
**
** {F16133} When calls to [sqlite3_create_function(D,X,N,...)]
**          specify multiple implementations of the same function X
**          and when one implementation has N>=0 and the other has N=(-1)
**          the implementation with a non-zero N is preferred.
**
** {F16136} When calls to [sqlite3_create_function(D,X,N,E,...)]
**          specify multiple implementations of the same function X with
**          the same number of arguments N but with different
**          encodings E, then the implementation where E matches the
**          database encoding is preferred.
**
** {F16139} For an aggregate SQL function created using
**          [sqlite3_create_function(D,X,N,E,P,0,S,L)] the finializer
**          function L will always be invoked exactly once if the
**          step function S is called one or more times.
**
** {F16142} When SQLite invokes either the xFunc or xStep function of
**          an application-defined SQL function or aggregate created
**          by [sqlite3_create_function()] or [sqlite3_create_function16()],
**          then the array of [sqlite3_value] objects passed as the
**          third parameter are always [protected sqlite3_value] objects.
*/
int sqlite3_create_function(
  sqlite3 *db,
  const char *zFunctionName,
  int nArg,
  int eTextRep,
  void *pApp,
  void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
  void (*xStep)(sqlite3_context*,int,sqlite3_value**),
  void (*xFinal)(sqlite3_context*)
);
int sqlite3_create_function16(
  sqlite3 *db,
  const void *zFunctionName,
  int nArg,
  int eTextRep,
  void *pApp,
  void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
  void (*xStep)(sqlite3_context*,int,sqlite3_value**),
  void (*xFinal)(sqlite3_context*)
);

/*
** CAPI3REF: Text Encodings {F10267}
**
** These constant define integer codes that represent the various
** text encodings supported by SQLite.
*/
#define SQLITE_UTF8           1
#define SQLITE_UTF16LE        2
#define SQLITE_UTF16BE        3
#define SQLITE_UTF16          4    /* Use native byte order */
#define SQLITE_ANY            5    /* sqlite3_create_function only */
#define SQLITE_UTF16_ALIGNED  8    /* sqlite3_create_collation only */

/*
** CAPI3REF: Obsolete Functions
**
** These functions are all now obsolete.  In order to maintain
** backwards compatibility with older code, we continue to support
** these functions.  However, new development projects should avoid
** the use of these functions.  To help encourage people to avoid
** using these functions, we are not going to tell you want they do.
*/
int sqlite3_aggregate_count(sqlite3_context*);
int sqlite3_expired(sqlite3_stmt*);
int sqlite3_transfer_bindings(sqlite3_stmt*, sqlite3_stmt*);
int sqlite3_global_recover(void);
void sqlite3_thread_cleanup(void);
int sqlite3_memory_alarm(void(*)(void*,sqlite3_int64,int),void*,sqlite3_int64);

/*
** CAPI3REF: Obtaining SQL Function Parameter Values {F15100}
**
** The C-language implementation of SQL functions and aggregates uses
** this set of interface routines to access the parameter values on
** the function or aggregate.
**
** The xFunc (for scalar functions) or xStep (for aggregates) parameters
** to [sqlite3_create_function()] and [sqlite3_create_function16()]
** define callbacks that implement the SQL functions and aggregates.
** The 4th parameter to these callbacks is an array of pointers to
** [protected sqlite3_value] objects.  There is one [sqlite3_value] object for
** each parameter to the SQL function.  These routines are used to
** extract values from the [sqlite3_value] objects.
**
** These routines work only with [protected sqlite3_value] objects.
** Any attempt to use these routines on an [unprotected sqlite3_value]
** object results in undefined behavior.
**
** These routines work just like the corresponding 
** [sqlite3_column_blob | sqlite3_column_* routines] except that 
** these routines take a single [protected sqlite3_value] object pointer
** instead of an [sqlite3_stmt*] pointer and an integer column number.
**
** The sqlite3_value_text16() interface extracts a UTF16 string
** in the native byte-order of the host machine.  The
** sqlite3_value_text16be() and sqlite3_value_text16le() interfaces
** extract UTF16 strings as big-endian and little-endian respectively.
**
** The sqlite3_value_numeric_type() interface attempts to apply
** numeric affinity to the value.  This means that an attempt is
** made to convert the value to an integer or floating point.  If
** such a conversion is possible without loss of information (in other
** words if the value is a string that looks like a number)
** then the conversion is done.  Otherwise no conversion occurs.  The 
** [SQLITE_INTEGER | datatype] after conversion is returned.
**
** Please pay particular attention to the fact that the pointer that
** is returned from [sqlite3_value_blob()], [sqlite3_value_text()], or
** [sqlite3_value_text16()] can be invalidated by a subsequent call to
** [sqlite3_value_bytes()], [sqlite3_value_bytes16()], [sqlite3_value_text()],
** or [sqlite3_value_text16()].  
**
** These routines must be called from the same thread as
** the SQL function that supplied the [sqlite3_value*] parameters.
**
**
** INVARIANTS:
**
** {F15103} The [sqlite3_value_blob(V)] interface converts the
**          [protected sqlite3_value] object V into a blob and then returns a
**          pointer to the converted value.
**
** {F15106} The [sqlite3_value_bytes(V)] interface returns the
**          number of bytes in the blob or string (exclusive of the
**          zero terminator on the string) that was returned by the
**          most recent call to [sqlite3_value_blob(V)] or
**          [sqlite3_value_text(V)].
**
** {F15109} The [sqlite3_value_bytes16(V)] interface returns the
**          number of bytes in the string (exclusive of the
**          zero terminator on the string) that was returned by the
**          most recent call to [sqlite3_value_text16(V)],
**          [sqlite3_value_text16be(V)], or [sqlite3_value_text16le(V)].
**
** {F15112} The [sqlite3_value_double(V)] interface converts the
**          [protected sqlite3_value] object V into a floating point value and
**          returns a copy of that value.
**
** {F15115} The [sqlite3_value_int(V)] interface converts the
**          [protected sqlite3_value] object V into a 64-bit signed integer and
**          returns the lower 32 bits of that integer.
**
** {F15118} The [sqlite3_value_int64(V)] interface converts the
**          [protected sqlite3_value] object V into a 64-bit signed integer and
**          returns a copy of that integer.
**
** {F15121} The [sqlite3_value_text(V)] interface converts the
**          [protected sqlite3_value] object V into a zero-terminated UTF-8 
**          string and returns a pointer to that string.
**
** {F15124} The [sqlite3_value_text16(V)] interface converts the
**          [protected sqlite3_value] object V into a zero-terminated 2-byte
**          aligned UTF-16 native byte order
**          string and returns a pointer to that string.
**
** {F15127} The [sqlite3_value_text16be(V)] interface converts the
**          [protected sqlite3_value] object V into a zero-terminated 2-byte
**          aligned UTF-16 big-endian
**          string and returns a pointer to that string.
**
** {F15130} The [sqlite3_value_text16le(V)] interface converts the
**          [protected sqlite3_value] object V into a zero-terminated 2-byte
**          aligned UTF-16 little-endian
**          string and returns a pointer to that string.
**
** {F15133} The [sqlite3_value_type(V)] interface returns
**          one of [SQLITE_NULL], [SQLITE_INTEGER], [SQLITE_FLOAT],
**          [SQLITE_TEXT], or [SQLITE_BLOB] as appropriate for
**          the [sqlite3_value] object V.
**
** {F15136} The [sqlite3_value_numeric_type(V)] interface converts
**          the [protected sqlite3_value] object V into either an integer or
**          a floating point value if it can do so without loss of
**          information, and returns one of [SQLITE_NULL],
**          [SQLITE_INTEGER], [SQLITE_FLOAT], [SQLITE_TEXT], or
**          [SQLITE_BLOB] as appropriate for
**          the [protected sqlite3_value] object V after the conversion attempt.
*/
const void *sqlite3_value_blob(sqlite3_value*);
int sqlite3_value_bytes(sqlite3_value*);
int sqlite3_value_bytes16(sqlite3_value*);
double sqlite3_value_double(sqlite3_value*);
int sqlite3_value_int(sqlite3_value*);
sqlite3_int64 sqlite3_value_int64(sqlite3_value*);
const unsigned char *sqlite3_value_text(sqlite3_value*);
const void *sqlite3_value_text16(sqlite3_value*);
const void *sqlite3_value_text16le(sqlite3_value*);
const void *sqlite3_value_text16be(sqlite3_value*);
int sqlite3_value_type(sqlite3_value*);
int sqlite3_value_numeric_type(sqlite3_value*);

/*
** CAPI3REF: Obtain Aggregate Function Context {F16210}
**
** The implementation of aggregate SQL functions use this routine to allocate
** a structure for storing their state.  
** The first time the sqlite3_aggregate_context() routine is
** is called for a particular aggregate, SQLite allocates nBytes of memory
** zeros that memory, and returns a pointer to it.
** On second and subsequent calls to sqlite3_aggregate_context()
** for the same aggregate function index, the same buffer is returned.
** The implementation
** of the aggregate can use the returned buffer to accumulate data.
**
** SQLite automatically frees the allocated buffer when the aggregate
** query concludes.
**
** The first parameter should be a copy of the 
** [sqlite3_context | SQL function context] that is the first
** parameter to the callback routine that implements the aggregate
** function.
**
** This routine must be called from the same thread in which
** the aggregate SQL function is running.
**
** INVARIANTS:
**
** {F16211} The first invocation of [sqlite3_aggregate_context(C,N)] for
**          a particular instance of an aggregate function (for a particular
**          context C) causes SQLite to allocation N bytes of memory,
**          zero that memory, and return a pointer to the allocationed
**          memory.
**
** {F16213} If a memory allocation error occurs during
**          [sqlite3_aggregate_context(C,N)] then the function returns 0.
**
** {F16215} Second and subsequent invocations of
**          [sqlite3_aggregate_context(C,N)] for the same context pointer C
**          ignore the N parameter and return a pointer to the same
**          block of memory returned by the first invocation.
**
** {F16217} The memory allocated by [sqlite3_aggregate_context(C,N)] is
**          automatically freed on the next call to [sqlite3_reset()]
**          or [sqlite3_finalize()] for the [prepared statement] containing
**          the aggregate function associated with context C.
*/
void *sqlite3_aggregate_context(sqlite3_context*, int nBytes);

/*
** CAPI3REF: User Data For Functions {F16240}
**
** The sqlite3_user_data() interface returns a copy of
** the pointer that was the pUserData parameter (the 5th parameter)
** of the the [sqlite3_create_function()]
** and [sqlite3_create_function16()] routines that originally
** registered the application defined function. {END}
**
** This routine must be called from the same thread in which
** the application-defined function is running.
**
** INVARIANTS:
**
** {F16243} The [sqlite3_user_data(C)] interface returns a copy of the
**          P pointer from the [sqlite3_create_function(D,X,N,E,P,F,S,L)]
**          or [sqlite3_create_function16(D,X,N,E,P,F,S,L)] call that
**          registered the SQL function associated with 
**          [sqlite3_context] C.
*/
void *sqlite3_user_data(sqlite3_context*);

/*
** CAPI3REF: Database Connection For Functions {F16250}
**
** The sqlite3_context_db_handle() interface returns a copy of
** the pointer to the [database connection] (the 1st parameter)
** of the the [sqlite3_create_function()]
** and [sqlite3_create_function16()] routines that originally
** registered the application defined function.
**
** INVARIANTS:
**
** {F16253} The [sqlite3_context_db_handle(C)] interface returns a copy of the
**          D pointer from the [sqlite3_create_function(D,X,N,E,P,F,S,L)]
**          or [sqlite3_create_function16(D,X,N,E,P,F,S,L)] call that
**          registered the SQL function associated with 
**          [sqlite3_context] C.
*/
sqlite3 *sqlite3_context_db_handle(sqlite3_context*);

/*
** CAPI3REF: Function Auxiliary Data {F16270}
**
** The following two functions may be used by scalar SQL functions to
** associate meta-data with argument values. If the same value is passed to
** multiple invocations of the same SQL function during query execution, under
** some circumstances the associated meta-data may be preserved. This may
** be used, for example, to add a regular-expression matching scalar
** function. The compiled version of the regular expression is stored as
** meta-data associated with the SQL value passed as the regular expression
** pattern.  The compiled regular expression can be reused on multiple
** invocations of the same function so that the original pattern string
** does not need to be recompiled on each invocation.
**
** The sqlite3_get_auxdata() interface returns a pointer to the meta-data
** associated by the sqlite3_set_auxdata() function with the Nth argument
** value to the application-defined function.
** If no meta-data has been ever been set for the Nth
** argument of the function, or if the cooresponding function parameter
** has changed since the meta-data was set, then sqlite3_get_auxdata()
** returns a NULL pointer.
**
** The sqlite3_set_auxdata() interface saves the meta-data
** pointed to by its 3rd parameter as the meta-data for the N-th
** argument of the application-defined function.  Subsequent
** calls to sqlite3_get_auxdata() might return this data, if it has
** not been destroyed. 
** If it is not NULL, SQLite will invoke the destructor 
** function given by the 4th parameter to sqlite3_set_auxdata() on
** the meta-data when the corresponding function parameter changes
** or when the SQL statement completes, whichever comes first.
**
** SQLite is free to call the destructor and drop meta-data on
** any parameter of any function at any time.  The only guarantee
** is that the destructor will be called before the metadata is
** dropped.
**
** In practice, meta-data is preserved between function calls for
** expressions that are constant at compile time. This includes literal
** values and SQL variables.
**
** These routines must be called from the same thread in which
** the SQL function is running.
**
** INVARIANTS:
**
** {F16272} The [sqlite3_get_auxdata(C,N)] interface returns a pointer
**          to metadata associated with the Nth parameter of the SQL function
**          whose context is C, or NULL if there is no metadata associated
**          with that parameter.
**
** {F16274} The [sqlite3_set_auxdata(C,N,P,D)] interface assigns a metadata
**          pointer P to the Nth parameter of the SQL function with context
**          C.
**
** {F16276} SQLite will invoke the destructor D with a single argument
**          which is the metadata pointer P following a call to
**          [sqlite3_set_auxdata(C,N,P,D)] when SQLite ceases to hold
**          the metadata.
**
** {F16277} SQLite ceases to hold metadata for an SQL function parameter
**          when the value of that parameter changes.
**
** {F16278} When [sqlite3_set_auxdata(C,N,P,D)] is invoked, the destructor
**          is called for any prior metadata associated with the same function
**          context C and parameter N.
**
** {F16279} SQLite will call destructors for any metadata it is holding
**          in a particular [prepared statement] S when either
**          [sqlite3_reset(S)] or [sqlite3_finalize(S)] is called.
*/
void *sqlite3_get_auxdata(sqlite3_context*, int N);
void sqlite3_set_auxdata(sqlite3_context*, int N, void*, void (*)(void*));


/*
** CAPI3REF: Constants Defining Special Destructor Behavior {F10280}
**
** These are special value for the destructor that is passed in as the
** final argument to routines like [sqlite3_result_blob()].  If the destructor
** argument is SQLITE_STATIC, it means that the content pointer is constant
** and will never change.  It does not need to be destroyed.  The 
** SQLITE_TRANSIENT value means that the content will likely change in
** the near future and that SQLite should make its own private copy of
** the content before returning.
**
** The typedef is necessary to work around problems in certain
** C++ compilers.  See ticket #2191.
*/
typedef void (*sqlite3_destructor_type)(void*);
#define SQLITE_STATIC      ((sqlite3_destructor_type)0)
#define SQLITE_TRANSIENT   ((sqlite3_destructor_type)-1)

/*
** CAPI3REF: Setting The Result Of An SQL Function {F16400}
**
** These routines are used by the xFunc or xFinal callbacks that
** implement SQL functions and aggregates.  See
** [sqlite3_create_function()] and [sqlite3_create_function16()]
** for additional information.
**
** These functions work very much like the 
** [sqlite3_bind_blob | sqlite3_bind_*] family of functions used
** to bind values to host parameters in prepared statements.
** Refer to the
** [sqlite3_bind_blob | sqlite3_bind_* documentation] for
** additional information.
**
** The sqlite3_result_blob() interface sets the result from
** an application defined function to be the BLOB whose content is pointed
** to by the second parameter and which is N bytes long where N is the
** third parameter. 
** The sqlite3_result_zeroblob() inerfaces set the result of
** the application defined function to be a BLOB containing all zero
** bytes and N bytes in size, where N is the value of the 2nd parameter.
**
** The sqlite3_result_double() interface sets the result from
** an application defined function to be a floating point value specified
** by its 2nd argument.
**
** The sqlite3_result_error() and sqlite3_result_error16() functions
** cause the implemented SQL function to throw an exception.
** SQLite uses the string pointed to by the
** 2nd parameter of sqlite3_result_error() or sqlite3_result_error16()
** as the text of an error message.  SQLite interprets the error
** message string from sqlite3_result_error() as UTF8. SQLite
** interprets the string from sqlite3_result_error16() as UTF16 in native
** byte order.  If the third parameter to sqlite3_result_error()
** or sqlite3_result_error16() is negative then SQLite takes as the error
** message all text up through the first zero character.
** If the third parameter to sqlite3_result_error() or
** sqlite3_result_error16() is non-negative then SQLite takes that many
** bytes (not characters) from the 2nd parameter as the error message.
** The sqlite3_result_error() and sqlite3_result_error16()
** routines make a copy private copy of the error message text before
** they return.  Hence, the calling function can deallocate or
** modify the text after they return without harm.
** The sqlite3_result_error_code() function changes the error code
** returned by SQLite as a result of an error in a function.  By default,
** the error code is SQLITE_ERROR.  A subsequent call to sqlite3_result_error()
** or sqlite3_result_error16() resets the error code to SQLITE_ERROR.
**
** The sqlite3_result_toobig() interface causes SQLite
** to throw an error indicating that a string or BLOB is to long
** to represent.  The sqlite3_result_nomem() interface
** causes SQLite to throw an exception indicating that the a
** memory allocation failed.
**
** The sqlite3_result_int() interface sets the return value
** of the application-defined function to be the 32-bit signed integer
** value given in the 2nd argument.
** The sqlite3_result_int64() interface sets the return value
** of the application-defined function to be the 64-bit signed integer
** value given in the 2nd argument.
**
** The sqlite3_result_null() interface sets the return value
** of the application-defined function to be NULL.
**
** The sqlite3_result_text(), sqlite3_result_text16(), 
** sqlite3_result_text16le(), and sqlite3_result_text16be() interfaces
** set the return value of the application-defined function to be
** a text string which is represented as UTF-8, UTF-16 native byte order,
** UTF-16 little endian, or UTF-16 big endian, respectively.
** SQLite takes the text result from the application from
** the 2nd parameter of the sqlite3_result_text* interfaces.
** If the 3rd parameter to the sqlite3_result_text* interfaces
** is negative, then SQLite takes result text from the 2nd parameter 
** through the first zero character.
** If the 3rd parameter to the sqlite3_result_text* interfaces
** is non-negative, then as many bytes (not characters) of the text
** pointed to by the 2nd parameter are taken as the application-defined
** function result.
** If the 4th parameter to the sqlite3_result_text* interfaces
** or sqlite3_result_blob is a non-NULL pointer, then SQLite calls that
** function as the destructor on the text or blob result when it has
** finished using that result.
** If the 4th parameter to the sqlite3_result_text* interfaces
** or sqlite3_result_blob is the special constant SQLITE_STATIC, then
** SQLite assumes that the text or blob result is constant space and
** does not copy the space or call a destructor when it has
** finished using that result.
** If the 4th parameter to the sqlite3_result_text* interfaces
** or sqlite3_result_blob is the special constant SQLITE_TRANSIENT
** then SQLite makes a copy of the result into space obtained from
** from [sqlite3_malloc()] before it returns.
**
** The sqlite3_result_value() interface sets the result of
** the application-defined function to be a copy the
** [unprotected sqlite3_value] object specified by the 2nd parameter.  The
** sqlite3_result_value() interface makes a copy of the [sqlite3_value]
** so that [sqlite3_value] specified in the parameter may change or
** be deallocated after sqlite3_result_value() returns without harm.
** A [protected sqlite3_value] object may always be used where an
** [unprotected sqlite3_value] object is required, so either
** kind of [sqlite3_value] object can be used with this interface.
**
** If these routines are called from within the different thread 
** than the one containing the application-defined function that recieved
** the [sqlite3_context] pointer, the results are undefined.
**
** INVARIANTS:
**
** {F16403} The default return value from any SQL function is NULL.
**
** {F16406} The [sqlite3_result_blob(C,V,N,D)] interface changes the
**          return value of function C to be a blob that is N bytes
**          in length and with content pointed to by V.
**
** {F16409} The [sqlite3_result_double(C,V)] interface changes the
**          return value of function C to be the floating point value V.
**
** {F16412} The [sqlite3_result_error(C,V,N)] interface changes the return
**          value of function C to be an exception with error code
**          [SQLITE_ERROR] and a UTF8 error message copied from V up to the
**          first zero byte or until N bytes are read if N is positive.
**
** {F16415} The [sqlite3_result_error16(C,V,N)] interface changes the return
**          value of function C to be an exception with error code
**          [SQLITE_ERROR] and a UTF16 native byte order error message
**          copied from V up to the first zero terminator or until N bytes
**          are read if N is positive.
**
** {F16418} The [sqlite3_result_error_toobig(C)] interface changes the return
**          value of the function C to be an exception with error code
**          [SQLITE_TOOBIG] and an appropriate error message.
**
** {F16421} The [sqlite3_result_error_nomem(C)] interface changes the return
**          value of the function C to be an exception with error code
**          [SQLITE_NOMEM] and an appropriate error message.
**
** {F16424} The [sqlite3_result_error_code(C,E)] interface changes the return
**          value of the function C to be an exception with error code E.
**          The error message text is unchanged.
**
** {F16427} The [sqlite3_result_int(C,V)] interface changes the
**          return value of function C to be the 32-bit integer value V.
**
** {F16430} The [sqlite3_result_int64(C,V)] interface changes the
**          return value of function C to be the 64-bit integer value V.
**
** {F16433} The [sqlite3_result_null(C)] interface changes the
**          return value of function C to be NULL.
**
** {F16436} The [sqlite3_result_text(C,V,N,D)] interface changes the
**          return value of function C to be the UTF8 string
**          V up to the first zero if N is negative
**          or the first N bytes of V if N is non-negative.
**
** {F16439} The [sqlite3_result_text16(C,V,N,D)] interface changes the
**          return value of function C to be the UTF16 native byte order
**          string V up to the first zero if N is
**          negative or the first N bytes of V if N is non-negative.
**
** {F16442} The [sqlite3_result_text16be(C,V,N,D)] interface changes the
**          return value of function C to be the UTF16 big-endian
**          string V up to the first zero if N is
**          is negative or the first N bytes or V if N is non-negative.
**
** {F16445} The [sqlite3_result_text16le(C,V,N,D)] interface changes the
**          return value of function C to be the UTF16 little-endian
**          string V up to the first zero if N is
**          negative or the first N bytes of V if N is non-negative.
**
** {F16448} The [sqlite3_result_value(C,V)] interface changes the
**          return value of function C to be [unprotected sqlite3_value]
**          object V.
**
** {F16451} The [sqlite3_result_zeroblob(C,N)] interface changes the
**          return value of function C to be an N-byte blob of all zeros.
**
** {F16454} The [sqlite3_result_error()] and [sqlite3_result_error16()]
**          interfaces make a copy of their error message strings before
**          returning.
**
** {F16457} If the D destructor parameter to [sqlite3_result_blob(C,V,N,D)],
**          [sqlite3_result_text(C,V,N,D)], [sqlite3_result_text16(C,V,N,D)],
**          [sqlite3_result_text16be(C,V,N,D)], or
**          [sqlite3_result_text16le(C,V,N,D)] is the constant [SQLITE_STATIC]
**          then no destructor is ever called on the pointer V and SQLite
**          assumes that V is immutable.
**
** {F16460} If the D destructor parameter to [sqlite3_result_blob(C,V,N,D)],
**          [sqlite3_result_text(C,V,N,D)], [sqlite3_result_text16(C,V,N,D)],
**          [sqlite3_result_text16be(C,V,N,D)], or
**          [sqlite3_result_text16le(C,V,N,D)] is the constant
**          [SQLITE_TRANSIENT] then the interfaces makes a copy of the
**          content of V and retains the copy.
**
** {F16463} If the D destructor parameter to [sqlite3_result_blob(C,V,N,D)],
**          [sqlite3_result_text(C,V,N,D)], [sqlite3_result_text16(C,V,N,D)],
**          [sqlite3_result_text16be(C,V,N,D)], or
**          [sqlite3_result_text16le(C,V,N,D)] is some value other than
**          the constants [SQLITE_STATIC] and [SQLITE_TRANSIENT] then 
**          SQLite will invoke the destructor D with V as its only argument
**          when it has finished with the V value.
*/
void sqlite3_result_blob(sqlite3_context*, const void*, int, void(*)(void*));
void sqlite3_result_double(sqlite3_context*, double);
void sqlite3_result_error(sqlite3_context*, const char*, int);
void sqlite3_result_error16(sqlite3_context*, const void*, int);
void sqlite3_result_error_toobig(sqlite3_context*);
void sqlite3_result_error_nomem(sqlite3_context*);
void sqlite3_result_error_code(sqlite3_context*, int);
void sqlite3_result_int(sqlite3_context*, int);
void sqlite3_result_int64(sqlite3_context*, sqlite3_int64);
void sqlite3_result_null(sqlite3_context*);
void sqlite3_result_text(sqlite3_context*, const char*, int, void(*)(void*));
void sqlite3_result_text16(sqlite3_context*, const void*, int, void(*)(void*));
void sqlite3_result_text16le(sqlite3_context*, const void*, int,void(*)(void*));
void sqlite3_result_text16be(sqlite3_context*, const void*, int,void(*)(void*));
void sqlite3_result_value(sqlite3_context*, sqlite3_value*);
void sqlite3_result_zeroblob(sqlite3_context*, int n);

/*
** CAPI3REF: Define New Collating Sequences {F16600}
**
** These functions are used to add new collation sequences to the
** [sqlite3*] handle specified as the first argument. 
**
** The name of the new collation sequence is specified as a UTF-8 string
** for sqlite3_create_collation() and sqlite3_create_collation_v2()
** and a UTF-16 string for sqlite3_create_collation16(). In all cases
** the name is passed as the second function argument.
**
** The third argument may be one of the constants [SQLITE_UTF8],
** [SQLITE_UTF16LE] or [SQLITE_UTF16BE], indicating that the user-supplied
** routine expects to be passed pointers to strings encoded using UTF-8,
** UTF-16 little-endian or UTF-16 big-endian respectively. The
** third argument might also be [SQLITE_UTF16_ALIGNED] to indicate that
** the routine expects pointers to 16-bit word aligned strings
** of UTF16 in the native byte order of the host computer.
**
** A pointer to the user supplied routine must be passed as the fifth
** argument.  If it is NULL, this is the same as deleting the collation
** sequence (so that SQLite cannot call it anymore).
** Each time the application
** supplied function is invoked, it is passed a copy of the void* passed as
** the fourth argument to sqlite3_create_collation() or
** sqlite3_create_collation16() as its first parameter.
**
** The remaining arguments to the application-supplied routine are two strings,
** each represented by a (length, data) pair and encoded in the encoding
** that was passed as the third argument when the collation sequence was
** registered. {END} The application defined collation routine should
** return negative, zero or positive if
** the first string is less than, equal to, or greater than the second
** string. i.e. (STRING1 - STRING2).
**
** The sqlite3_create_collation_v2() works like sqlite3_create_collation()
** excapt that it takes an extra argument which is a destructor for
** the collation.  The destructor is called when the collation is
** destroyed and is passed a copy of the fourth parameter void* pointer
** of the sqlite3_create_collation_v2().
** Collations are destroyed when
** they are overridden by later calls to the collation creation functions
** or when the [sqlite3*] database handle is closed using [sqlite3_close()].
**
** INVARIANTS:
**
** {F16603} A successful call to the
**          [sqlite3_create_collation_v2(B,X,E,P,F,D)] interface
**          registers function F as the comparison function used to
**          implement collation X on [database connection] B for
**          databases having encoding E.
**
** {F16604} SQLite understands the X parameter to
**          [sqlite3_create_collation_v2(B,X,E,P,F,D)] as a zero-terminated
**          UTF-8 string in which case is ignored for ASCII characters and
**          is significant for non-ASCII characters.
**
** {F16606} Successive calls to [sqlite3_create_collation_v2(B,X,E,P,F,D)]
**          with the same values for B, X, and E, override prior values
**          of P, F, and D.
**
** {F16609} The destructor D in [sqlite3_create_collation_v2(B,X,E,P,F,D)]
**          is not NULL then it is called with argument P when the
**          collating function is dropped by SQLite.
**
** {F16612} A collating function is dropped when it is overloaded.
**
** {F16615} A collating function is dropped when the database connection
**          is closed using [sqlite3_close()].
**
** {F16618} The pointer P in [sqlite3_create_collation_v2(B,X,E,P,F,D)]
**          is passed through as the first parameter to the comparison
**          function F for all subsequent invocations of F.
**
** {F16621} A call to [sqlite3_create_collation(B,X,E,P,F)] is exactly
**          the same as a call to [sqlite3_create_collation_v2()] with
**          the same parameters and a NULL destructor.
**
** {F16624} Following a [sqlite3_create_collation_v2(B,X,E,P,F,D)],
**          SQLite uses the comparison function F for all text comparison
**          operations on [database connection] B on text values that
**          use the collating sequence name X.
**
** {F16627} The [sqlite3_create_collation16(B,X,E,P,F)] works the same
**          as [sqlite3_create_collation(B,X,E,P,F)] except that the
**          collation name X is understood as UTF-16 in native byte order
**          instead of UTF-8.
**
** {F16630} When multiple comparison functions are available for the same
**          collating sequence, SQLite chooses the one whose text encoding
**          requires the least amount of conversion from the default
**          text encoding of the database.
*/
int sqlite3_create_collation(
  sqlite3*, 
  const char *zName, 
  int eTextRep, 
  void*,
  int(*xCompare)(void*,int,const void*,int,const void*)
);
int sqlite3_create_collation_v2(
  sqlite3*, 
  const char *zName, 
  int eTextRep, 
  void*,
  int(*xCompare)(void*,int,const void*,int,const void*),
  void(*xDestroy)(void*)
);
int sqlite3_create_collation16(
  sqlite3*, 
  const char *zName, 
  int eTextRep, 
  void*,
  int(*xCompare)(void*,int,const void*,int,const void*)
);

/*
** CAPI3REF: Collation Needed Callbacks {F16700}
**
** To avoid having to register all collation sequences before a database
** can be used, a single callback function may be registered with the
** database handle to be called whenever an undefined collation sequence is
** required.
**
** If the function is registered using the sqlite3_collation_needed() API,
** then it is passed the names of undefined collation sequences as strings
** encoded in UTF-8. {F16703} If sqlite3_collation_needed16() is used, the names
** are passed as UTF-16 in machine native byte order. A call to either
** function replaces any existing callback.
**
** When the callback is invoked, the first argument passed is a copy
** of the second argument to sqlite3_collation_needed() or
** sqlite3_collation_needed16().  The second argument is the database
** handle.  The third argument is one of [SQLITE_UTF8],
** [SQLITE_UTF16BE], or [SQLITE_UTF16LE], indicating the most
** desirable form of the collation sequence function required.
** The fourth parameter is the name of the
** required collation sequence.
**
** The callback function should register the desired collation using
** [sqlite3_create_collation()], [sqlite3_create_collation16()], or
** [sqlite3_create_collation_v2()].
**
** INVARIANTS:
**
** {F16702} A successful call to [sqlite3_collation_needed(D,P,F)]
**          or [sqlite3_collation_needed16(D,P,F)] causes
**          the [database connection] D to invoke callback F with first
**          parameter P whenever it needs a comparison function for a
**          collating sequence that it does not know about.
**
** {F16704} Each successful call to [sqlite3_collation_needed()] or
**          [sqlite3_collation_needed16()] overrides the callback registered
**          on the same [database connection] by prior calls to either
**          interface.
**
** {F16706} The name of the requested collating function passed in the
**          4th parameter to the callback is in UTF-8 if the callback
**          was registered using [sqlite3_collation_needed()] and
**          is in UTF-16 native byte order if the callback was
**          registered using [sqlite3_collation_needed16()].
**
** 
*/
int sqlite3_collation_needed(
  sqlite3*, 
  void*, 
  void(*)(void*,sqlite3*,int eTextRep,const char*)
);
int sqlite3_collation_needed16(
  sqlite3*, 
  void*,
  void(*)(void*,sqlite3*,int eTextRep,const void*)
);

/*
** Specify the key for an encrypted database.  This routine should be
** called right after sqlite3_open().
**
** The code to implement this API is not available in the public release
** of SQLite.
*/
int sqlite3_key(
  sqlite3 *db,                   /* Database to be rekeyed */
  const void *pKey, int nKey     /* The key */
);

/*
** Change the key on an open database.  If the current database is not
** encrypted, this routine will encrypt it.  If pNew==0 or nNew==0, the
** database is decrypted.
**
** The code to implement this API is not available in the public release
** of SQLite.
*/
int sqlite3_rekey(
  sqlite3 *db,                   /* Database to be rekeyed */
  const void *pKey, int nKey     /* The new key */
);

/*
** CAPI3REF:  Suspend Execution For A Short Time {F10530}
**
** The sqlite3_sleep() function
** causes the current thread to suspend execution
** for at least a number of milliseconds specified in its parameter.
**
** If the operating system does not support sleep requests with 
** millisecond time resolution, then the time will be rounded up to 
** the nearest second. The number of milliseconds of sleep actually 
** requested from the operating system is returned.
**
** SQLite implements this interface by calling the xSleep()
** method of the default [sqlite3_vfs] object.
**
** INVARIANTS:
**
** {F10533} The [sqlite3_sleep(M)] interface invokes the xSleep
**          method of the default [sqlite3_vfs|VFS] in order to
**          suspend execution of the current thread for at least
**          M milliseconds.
**
** {F10536} The [sqlite3_sleep(M)] interface returns the number of
**          milliseconds of sleep actually requested of the operating
**          system, which might be larger than the parameter M.
*/
int sqlite3_sleep(int);

/*
** CAPI3REF:  Name Of The Folder Holding Temporary Files {F10310}
**
** If this global variable is made to point to a string which is
** the name of a folder (a.ka. directory), then all temporary files
** created by SQLite will be placed in that directory.  If this variable
** is NULL pointer, then SQLite does a search for an appropriate temporary
** file directory.
**
** It is not safe to modify this variable once a database connection
** has been opened.  It is intended that this variable be set once
** as part of process initialization and before any SQLite interface
** routines have been call and remain unchanged thereafter.
*/
SQLITE_EXTERN char *sqlite3_temp_directory;

/*
** CAPI3REF:  Test To See If The Database Is In Auto-Commit Mode {F12930}
**
** The sqlite3_get_autocommit() interfaces returns non-zero or
** zero if the given database connection is or is not in autocommit mode,
** respectively.   Autocommit mode is on
** by default.  Autocommit mode is disabled by a [BEGIN] statement.
** Autocommit mode is reenabled by a [COMMIT] or [ROLLBACK].
**
** If certain kinds of errors occur on a statement within a multi-statement
** transactions (errors including [SQLITE_FULL], [SQLITE_IOERR], 
** [SQLITE_NOMEM], [SQLITE_BUSY], and [SQLITE_INTERRUPT]) then the
** transaction might be rolled back automatically.  The only way to
** find out if SQLite automatically rolled back the transaction after
** an error is to use this function.
**
** INVARIANTS:
**
** {F12931} The [sqlite3_get_autocommit(D)] interface returns non-zero or
**          zero if the [database connection] D is or is not in autocommit
**          mode, respectively.
**
** {F12932} Autocommit mode is on by default.
**
** {F12933} Autocommit mode is disabled by a successful [BEGIN] statement.
**
** {F12934} Autocommit mode is enabled by a successful [COMMIT] or [ROLLBACK]
**          statement.
** 
**
** LIMITATIONS:
***
** {U12936} If another thread changes the autocommit status of the database
**          connection while this routine is running, then the return value
**          is undefined.
*/
int sqlite3_get_autocommit(sqlite3*);

/*
** CAPI3REF:  Find The Database Handle Of A Prepared Statement {F13120}
**
** The sqlite3_db_handle interface
** returns the [sqlite3*] database handle to which a
** [prepared statement] belongs.
** The database handle returned by sqlite3_db_handle
** is the same database handle that was
** the first argument to the [sqlite3_prepare_v2()] or its variants
** that was used to create the statement in the first place.
**
** INVARIANTS:
**
** {F13123} The [sqlite3_db_handle(S)] interface returns a pointer
**          to the [database connection] associated with
**          [prepared statement] S.
*/
sqlite3 *sqlite3_db_handle(sqlite3_stmt*);


/*
** CAPI3REF: Commit And Rollback Notification Callbacks {F12950}
**
** The sqlite3_commit_hook() interface registers a callback
** function to be invoked whenever a transaction is committed.
** Any callback set by a previous call to sqlite3_commit_hook()
** for the same database connection is overridden.
** The sqlite3_rollback_hook() interface registers a callback
** function to be invoked whenever a transaction is committed.
** Any callback set by a previous call to sqlite3_commit_hook()
** for the same database connection is overridden.
** The pArg argument is passed through
** to the callback.  If the callback on a commit hook function 
** returns non-zero, then the commit is converted into a rollback.
**
** If another function was previously registered, its
** pArg value is returned.  Otherwise NULL is returned.
**
** Registering a NULL function disables the callback.
**
** For the purposes of this API, a transaction is said to have been 
** rolled back if an explicit "ROLLBACK" statement is executed, or
** an error or constraint causes an implicit rollback to occur.
** The rollback callback is not invoked if a transaction is
** automatically rolled back because the database connection is closed.
** The rollback callback is not invoked if a transaction is
** rolled back because a commit callback returned non-zero.
** <todo> Check on this </todo>
**
** These are experimental interfaces and are subject to change.
**
** INVARIANTS:
**
** {F12951} The [sqlite3_commit_hook(D,F,P)] interface registers the
**          callback function F to be invoked with argument P whenever
**          a transaction commits on [database connection] D.
**
** {F12952} The [sqlite3_commit_hook(D,F,P)] interface returns the P
**          argument from the previous call with the same 
**          [database connection ] D , or NULL on the first call
**          for a particular [database connection] D.
**
** {F12953} Each call to [sqlite3_commit_hook()] overwrites the callback
**          registered by prior calls.
**
** {F12954} If the F argument to [sqlite3_commit_hook(D,F,P)] is NULL
**          then the commit hook callback is cancelled and no callback
**          is invoked when a transaction commits.
**
** {F12955} If the commit callback returns non-zero then the commit is
**          converted into a rollback.
**
** {F12961} The [sqlite3_rollback_hook(D,F,P)] interface registers the
**          callback function F to be invoked with argument P whenever
**          a transaction rolls back on [database connection] D.
**
** {F12962} The [sqlite3_rollback_hook(D,F,P)] interface returns the P
**          argument from the previous call with the same 
**          [database connection ] D , or NULL on the first call
**          for a particular [database connection] D.
**
** {F12963} Each call to [sqlite3_rollback_hook()] overwrites the callback
**          registered by prior calls.
**
** {F12964} If the F argument to [sqlite3_rollback_hook(D,F,P)] is NULL
**          then the rollback hook callback is cancelled and no callback
**          is invoked when a transaction rolls back.
*/
void *sqlite3_commit_hook(sqlite3*, int(*)(void*), void*);
void *sqlite3_rollback_hook(sqlite3*, void(*)(void *), void*);

/*
** CAPI3REF: Data Change Notification Callbacks {F12970}
**
** The sqlite3_update_hook() interface
** registers a callback function with the database connection identified by the 
** first argument to be invoked whenever a row is updated, inserted or deleted.
** Any callback set by a previous call to this function for the same 
** database connection is overridden.
**
** The second argument is a pointer to the function to invoke when a 
** row is updated, inserted or deleted. 
** The first argument to the callback is
** a copy of the third argument to sqlite3_update_hook().
** The second callback 
** argument is one of [SQLITE_INSERT], [SQLITE_DELETE] or [SQLITE_UPDATE],
** depending on the operation that caused the callback to be invoked.
** The third and 
** fourth arguments to the callback contain pointers to the database and 
** table name containing the affected row.
** The final callback parameter is 
** the rowid of the row.
** In the case of an update, this is the rowid after 
** the update takes place.
**
** The update hook is not invoked when internal system tables are
** modified (i.e. sqlite_master and sqlite_sequence).
**
** If another function was previously registered, its pArg value
** is returned.  Otherwise NULL is returned.
**
** INVARIANTS:
**
** {F12971} The [sqlite3_update_hook(D,F,P)] interface causes callback
**          function F to be invoked with first parameter P whenever
**          a table row is modified, inserted, or deleted on
**          [database connection] D.
**
** {F12973} The [sqlite3_update_hook(D,F,P)] interface returns the value
**          of P for the previous call on the same [database connection] D,
**          or NULL for the first call.
**
** {F12975} If the update hook callback F in [sqlite3_update_hook(D,F,P)]
**          is NULL then the no update callbacks are made.
**
** {F12977} Each call to [sqlite3_update_hook(D,F,P)] overrides prior calls
**          to the same interface on the same [database connection] D.
**
** {F12979} The update hook callback is not invoked when internal system
**          tables such as sqlite_master and sqlite_sequence are modified.
**
** {F12981} The second parameter to the update callback 
**          is one of [SQLITE_INSERT], [SQLITE_DELETE] or [SQLITE_UPDATE],
**          depending on the operation that caused the callback to be invoked.
**
** {F12983} The third and fourth arguments to the callback contain pointers
**          to zero-terminated UTF-8 strings which are the names of the
**          database and table that is being updated.

** {F12985} The final callback parameter is the rowid of the row after
**          the change occurs.
*/
void *sqlite3_update_hook(
  sqlite3*, 
  void(*)(void *,int ,char const *,char const *,sqlite3_int64),
  void*
);

/*
** CAPI3REF:  Enable Or Disable Shared Pager Cache {F10330}
**
** This routine enables or disables the sharing of the database cache
** and schema data structures between connections to the same database.
** Sharing is enabled if the argument is true and disabled if the argument
** is false.
**
** Cache sharing is enabled and disabled
** for an entire process. {END} This is a change as of SQLite version 3.5.0.
** In prior versions of SQLite, sharing was
** enabled or disabled for each thread separately.
**
** The cache sharing mode set by this interface effects all subsequent
** calls to [sqlite3_open()], [sqlite3_open_v2()], and [sqlite3_open16()].
** Existing database connections continue use the sharing mode
** that was in effect at the time they were opened.
**
** Virtual tables cannot be used with a shared cache.   When shared
** cache is enabled, the [sqlite3_create_module()] API used to register
** virtual tables will always return an error.
**
** This routine returns [SQLITE_OK] if shared cache was
** enabled or disabled successfully.  An [error code]
** is returned otherwise.
**
** Shared cache is disabled by default. But this might change in
** future releases of SQLite.  Applications that care about shared
** cache setting should set it explicitly.
**
** INVARIANTS:
** 
** {F10331} A successful invocation of [sqlite3_enable_shared_cache(B)]
**          will enable or disable shared cache mode for any subsequently
**          created [database connection] in the same process.
**
** {F10336} When shared cache is enabled, the [sqlite3_create_module()]
**          interface will always return an error.
**
** {F10337} The [sqlite3_enable_shared_cache(B)] interface returns
**          [SQLITE_OK] if shared cache was enabled or disabled successfully.
**
** {F10339} Shared cache is disabled by default.
*/
int sqlite3_enable_shared_cache(int);

/*
** CAPI3REF:  Attempt To Free Heap Memory {F17340}
**
** The sqlite3_release_memory() interface attempts to
** free N bytes of heap memory by deallocating non-essential memory
** allocations held by the database labrary. {END}  Memory used
** to cache database pages to improve performance is an example of
** non-essential memory.  Sqlite3_release_memory() returns
** the number of bytes actually freed, which might be more or less
** than the amount requested.
**
** INVARIANTS:
**
** {F17341} The [sqlite3_release_memory(N)] interface attempts to
**          free N bytes of heap memory by deallocating non-essential
**          memory allocations held by the database labrary.
**
** {F16342} The [sqlite3_release_memory(N)] returns the number
**          of bytes actually freed, which might be more or less
**          than the amount requested.
*/
int sqlite3_release_memory(int);

/*
** CAPI3REF:  Impose A Limit On Heap Size {F17350}
**
** The sqlite3_soft_heap_limit() interface
** places a "soft" limit on the amount of heap memory that may be allocated
** by SQLite. If an internal allocation is requested 
** that would exceed the soft heap limit, [sqlite3_release_memory()] is
** invoked one or more times to free up some space before the allocation
** is made.
**
** The limit is called "soft", because if
** [sqlite3_release_memory()] cannot
** free sufficient memory to prevent the limit from being exceeded,
** the memory is allocated anyway and the current operation proceeds.
**
** A negative or zero value for N means that there is no soft heap limit and
** [sqlite3_release_memory()] will only be called when memory is exhausted.
** The default value for the soft heap limit is zero.
**
** SQLite makes a best effort to honor the soft heap limit.  
** But if the soft heap limit cannot honored, execution will
** continue without error or notification.  This is why the limit is 
** called a "soft" limit.  It is advisory only.
**
** Prior to SQLite version 3.5.0, this routine only constrained the memory
** allocated by a single thread - the same thread in which this routine
** runs.  Beginning with SQLite version 3.5.0, the soft heap limit is
** applied to all threads. The value specified for the soft heap limit
** is an upper bound on the total memory allocation for all threads. In
** version 3.5.0 there is no mechanism for limiting the heap usage for
** individual threads.
**
** INVARIANTS:
**
** {F16351} The [sqlite3_soft_heap_limit(N)] interface places a soft limit
**          of N bytes on the amount of heap memory that may be allocated
**          using [sqlite3_malloc()] or [sqlite3_realloc()] at any point
**          in time.
**
** {F16352} If a call to [sqlite3_malloc()] or [sqlite3_realloc()] would
**          cause the total amount of allocated memory to exceed the
**          soft heap limit, then [sqlite3_release_memory()] is invoked
**          in an attempt to reduce the memory usage prior to proceeding
**          with the memory allocation attempt.
**
** {F16353} Calls to [sqlite3_malloc()] or [sqlite3_realloc()] that trigger
**          attempts to reduce memory usage through the soft heap limit
**          mechanism continue even if the attempt to reduce memory
**          usage is unsuccessful.
**
** {F16354} A negative or zero value for N in a call to
**          [sqlite3_soft_heap_limit(N)] means that there is no soft
**          heap limit and [sqlite3_release_memory()] will only be
**          called when memory is completely exhausted.
**
** {F16355} The default value for the soft heap limit is zero.
**
** {F16358} Each call to [sqlite3_soft_heap_limit(N)] overrides the
**          values set by all prior calls.
*/
void sqlite3_soft_heap_limit(int);

/*
** CAPI3REF:  Extract Metadata About A Column Of A Table {F12850}
**
** This routine
** returns meta-data about a specific column of a specific database
** table accessible using the connection handle passed as the first function 
** argument.
**
** The column is identified by the second, third and fourth parameters to 
** this function. The second parameter is either the name of the database
** (i.e. "main", "temp" or an attached database) containing the specified
** table or NULL. If it is NULL, then all attached databases are searched
** for the table using the same algorithm as the database engine uses to 
** resolve unqualified table references.
**
** The third and fourth parameters to this function are the table and column 
** name of the desired column, respectively. Neither of these parameters 
** may be NULL.
**
** Meta information is returned by writing to the memory locations passed as
** the 5th and subsequent parameters to this function. Any of these 
** arguments may be NULL, in which case the corresponding element of meta 
** information is ommitted.
**
** <pre>
** Parameter     Output Type      Description
** -----------------------------------
**
**   5th         const char*      Data type
**   6th         const char*      Name of the default collation sequence 
**   7th         int              True if the column has a NOT NULL constraint
**   8th         int              True if the column is part of the PRIMARY KEY
**   9th         int              True if the column is AUTOINCREMENT
** </pre>
**
**
** The memory pointed to by the character pointers returned for the 
** declaration type and collation sequence is valid only until the next 
** call to any sqlite API function.
**
** If the specified table is actually a view, then an error is returned.
**
** If the specified column is "rowid", "oid" or "_rowid_" and an 
** INTEGER PRIMARY KEY column has been explicitly declared, then the output 
** parameters are set for the explicitly declared column. If there is no
** explicitly declared IPK column, then the output parameters are set as 
** follows:
**
** <pre>
**     data type: "INTEGER"
**     collation sequence: "BINARY"
**     not null: 0
**     primary key: 1
**     auto increment: 0
** </pre>
**
** This function may load one or more schemas from database files. If an
** error occurs during this process, or if the requested table or column
** cannot be found, an SQLITE error code is returned and an error message
** left in the database handle (to be retrieved using sqlite3_errmsg()).
**
** This API is only available if the library was compiled with the
** SQLITE_ENABLE_COLUMN_METADATA preprocessor symbol defined.
*/
int sqlite3_table_column_metadata(
  sqlite3 *db,                /* Connection handle */
  const char *zDbName,        /* Database name or NULL */
  const char *zTableName,     /* Table name */
  const char *zColumnName,    /* Column name */
  char const **pzDataType,    /* OUTPUT: Declared data type */
  char const **pzCollSeq,     /* OUTPUT: Collation sequence name */
  int *pNotNull,              /* OUTPUT: True if NOT NULL constraint exists */
  int *pPrimaryKey,           /* OUTPUT: True if column part of PK */
  int *pAutoinc               /* OUTPUT: True if column is auto-increment */
);

/*
** CAPI3REF: Load An Extension {F12600}
**
** {F12601} The sqlite3_load_extension() interface
** attempts to load an SQLite extension library contained in the file
** zFile. {F12602} The entry point is zProc. {F12603} zProc may be 0
** in which case the name of the entry point defaults
** to "sqlite3_extension_init".
**
** {F12604} The sqlite3_load_extension() interface shall
** return [SQLITE_OK] on success and [SQLITE_ERROR] if something goes wrong.
**
** {F12605}
** If an error occurs and pzErrMsg is not 0, then the
** sqlite3_load_extension() interface shall attempt to fill *pzErrMsg with 
** error message text stored in memory obtained from [sqlite3_malloc()].
** {END}  The calling function should free this memory
** by calling [sqlite3_free()].
**
** {F12606}
** Extension loading must be enabled using [sqlite3_enable_load_extension()]
** prior to calling this API or an error will be returned.
*/
int sqlite3_load_extension(
  sqlite3 *db,          /* Load the extension into this database connection */
  const char *zFile,    /* Name of the shared library containing extension */
  const char *zProc,    /* Entry point.  Derived from zFile if 0 */
  char **pzErrMsg       /* Put error message here if not 0 */
);

/*
** CAPI3REF:  Enable Or Disable Extension Loading {F12620}
**
** So as not to open security holes in older applications that are
** unprepared to deal with extension loading, and as a means of disabling
** extension loading while evaluating user-entered SQL, the following
** API is provided to turn the [sqlite3_load_extension()] mechanism on and
** off.  {F12622} It is off by default. {END} See ticket #1863.
**
** {F12621} Call the sqlite3_enable_load_extension() routine
** with onoff==1 to turn extension loading on
** and call it with onoff==0 to turn it back off again. {END}
*/
int sqlite3_enable_load_extension(sqlite3 *db, int onoff);

/*
** CAPI3REF: Make Arrangements To Automatically Load An Extension {F12640}
**
** {F12641} This function
** registers an extension entry point that is automatically invoked
** whenever a new database connection is opened using
** [sqlite3_open()], [sqlite3_open16()], or [sqlite3_open_v2()]. {END}
**
** This API can be invoked at program startup in order to register
** one or more statically linked extensions that will be available
** to all new database connections.
**
** {F12642} Duplicate extensions are detected so calling this routine multiple
** times with the same extension is harmless.
**
** {F12643} This routine stores a pointer to the extension in an array
** that is obtained from sqlite_malloc(). {END} If you run a memory leak
** checker on your program and it reports a leak because of this
** array, then invoke [sqlite3_reset_auto_extension()] prior
** to shutdown to free the memory.
**
** {F12644} Automatic extensions apply across all threads. {END}
**
** This interface is experimental and is subject to change or
** removal in future releases of SQLite.
*/
int sqlite3_auto_extension(void *xEntryPoint);


/*
** CAPI3REF: Reset Automatic Extension Loading {F12660}
**
** {F12661} This function disables all previously registered
** automatic extensions. {END}  This
** routine undoes the effect of all prior [sqlite3_auto_extension()]
** calls.
**
** {F12662} This call disabled automatic extensions in all threads. {END}
**
** This interface is experimental and is subject to change or
** removal in future releases of SQLite.
*/
void sqlite3_reset_auto_extension(void);


/*
****** EXPERIMENTAL - subject to change without notice **************
**
** The interface to the virtual-table mechanism is currently considered
** to be experimental.  The interface might change in incompatible ways.
** If this is a problem for you, do not use the interface at this time.
**
** When the virtual-table mechanism stablizes, we will declare the
** interface fixed, support it indefinitely, and remove this comment.
*/

/*
** Structures used by the virtual table interface
*/
typedef struct sqlite3_vtab sqlite3_vtab;
typedef struct sqlite3_index_info sqlite3_index_info;
typedef struct sqlite3_vtab_cursor sqlite3_vtab_cursor;
typedef struct sqlite3_module sqlite3_module;

/*
** CAPI3REF: Virtual Table Object {F18000}
** KEYWORDS: sqlite3_module
**
** A module is a class of virtual tables.  Each module is defined
** by an instance of the following structure.  This structure consists
** mostly of methods for the module.
*/
struct sqlite3_module {
  int iVersion;
  int (*xCreate)(sqlite3*, void *pAux,
               int argc, const char *const*argv,
               sqlite3_vtab **ppVTab, char**);
  int (*xConnect)(sqlite3*, void *pAux,
               int argc, const char *const*argv,
               sqlite3_vtab **ppVTab, char**);
  int (*xBestIndex)(sqlite3_vtab *pVTab, sqlite3_index_info*);
  int (*xDisconnect)(sqlite3_vtab *pVTab);
  int (*xDestroy)(sqlite3_vtab *pVTab);
  int (*xOpen)(sqlite3_vtab *pVTab, sqlite3_vtab_cursor **ppCursor);
  int (*xClose)(sqlite3_vtab_cursor*);
  int (*xFilter)(sqlite3_vtab_cursor*, int idxNum, const char *idxStr,
                int argc, sqlite3_value **argv);
  int (*xNext)(sqlite3_vtab_cursor*);
  int (*xEof)(sqlite3_vtab_cursor*);
  int (*xColumn)(sqlite3_vtab_cursor*, sqlite3_context*, int);
  int (*xRowid)(sqlite3_vtab_cursor*, sqlite3_int64 *pRowid);
  int (*xUpdate)(sqlite3_vtab *, int, sqlite3_value **, sqlite3_int64 *);
  int (*xBegin)(sqlite3_vtab *pVTab);
  int (*xSync)(sqlite3_vtab *pVTab);
  int (*xCommit)(sqlite3_vtab *pVTab);
  int (*xRollback)(sqlite3_vtab *pVTab);
  int (*xFindFunction)(sqlite3_vtab *pVtab, int nArg, const char *zName,
                       void (**pxFunc)(sqlite3_context*,int,sqlite3_value**),
                       void **ppArg);

  int (*xRename)(sqlite3_vtab *pVtab, const char *zNew);
};

/*
** CAPI3REF: Virtual Table Indexing Information {F18100}
** KEYWORDS: sqlite3_index_info
**
** The sqlite3_index_info structure and its substructures is used to
** pass information into and receive the reply from the xBestIndex
** method of an sqlite3_module.  The fields under **Inputs** are the
** inputs to xBestIndex and are read-only.  xBestIndex inserts its
** results into the **Outputs** fields.
**
** The aConstraint[] array records WHERE clause constraints of the
** form:
**
**         column OP expr
**
** Where OP is =, &lt;, &lt;=, &gt;, or &gt;=.  
** The particular operator is stored
** in aConstraint[].op.  The index of the column is stored in 
** aConstraint[].iColumn.  aConstraint[].usable is TRUE if the
** expr on the right-hand side can be evaluated (and thus the constraint
** is usable) and false if it cannot.
**
** The optimizer automatically inverts terms of the form "expr OP column"
** and makes other simplifications to the WHERE clause in an attempt to
** get as many WHERE clause terms into the form shown above as possible.
** The aConstraint[] array only reports WHERE clause terms in the correct
** form that refer to the particular virtual table being queried.
**
** Information about the ORDER BY clause is stored in aOrderBy[].
** Each term of aOrderBy records a column of the ORDER BY clause.
**
** The xBestIndex method must fill aConstraintUsage[] with information
** about what parameters to pass to xFilter.  If argvIndex>0 then
** the right-hand side of the corresponding aConstraint[] is evaluated
** and becomes the argvIndex-th entry in argv.  If aConstraintUsage[].omit
** is true, then the constraint is assumed to be fully handled by the
** virtual table and is not checked again by SQLite.
**
** The idxNum and idxPtr values are recorded and passed into xFilter.
** sqlite3_free() is used to free idxPtr if needToFreeIdxPtr is true.
**
** The orderByConsumed means that output from xFilter will occur in
** the correct order to satisfy the ORDER BY clause so that no separate
** sorting step is required.
**
** The estimatedCost value is an estimate of the cost of doing the
** particular lookup.  A full scan of a table with N entries should have
** a cost of N.  A binary search of a table of N entries should have a
** cost of approximately log(N).
*/
struct sqlite3_index_info {
  /* Inputs */
  int nConstraint;           /* Number of entries in aConstraint */
  struct sqlite3_index_constraint {
     int iColumn;              /* Column on left-hand side of constraint */
     unsigned char op;         /* Constraint operator */
     unsigned char usable;     /* True if this constraint is usable */
     int iTermOffset;          /* Used internally - xBestIndex should ignore */
  } *aConstraint;            /* Table of WHERE clause constraints */
  int nOrderBy;              /* Number of terms in the ORDER BY clause */
  struct sqlite3_index_orderby {
     int iColumn;              /* Column number */
     unsigned char desc;       /* True for DESC.  False for ASC. */
  } *aOrderBy;               /* The ORDER BY clause */

  /* Outputs */
  struct sqlite3_index_constraint_usage {
    int argvIndex;           /* if >0, constraint is part of argv to xFilter */
    unsigned char omit;      /* Do not code a test for this constraint */
  } *aConstraintUsage;
  int idxNum;                /* Number used to identify the index */
  char *idxStr;              /* String, possibly obtained from sqlite3_malloc */
  int needToFreeIdxStr;      /* Free idxStr using sqlite3_free() if true */
  int orderByConsumed;       /* True if output is already ordered */
  double estimatedCost;      /* Estimated cost of using this index */
};
#define SQLITE_INDEX_CONSTRAINT_EQ    2
#define SQLITE_INDEX_CONSTRAINT_GT    4
#define SQLITE_INDEX_CONSTRAINT_LE    8
#define SQLITE_INDEX_CONSTRAINT_LT    16
#define SQLITE_INDEX_CONSTRAINT_GE    32
#define SQLITE_INDEX_CONSTRAINT_MATCH 64

/*
** CAPI3REF: Register A Virtual Table Implementation {F18200}
**
** This routine is used to register a new module name with an SQLite
** connection.  Module names must be registered before creating new
** virtual tables on the module, or before using preexisting virtual
** tables of the module.
*/
int sqlite3_create_module(
  sqlite3 *db,               /* SQLite connection to register module with */
  const char *zName,         /* Name of the module */
  const sqlite3_module *,    /* Methods for the module */
  void *                     /* Client data for xCreate/xConnect */
);

/*
** CAPI3REF: Register A Virtual Table Implementation {F18210}
**
** This routine is identical to the sqlite3_create_module() method above,
** except that it allows a destructor function to be specified. It is
** even more experimental than the rest of the virtual tables API.
*/
int sqlite3_create_module_v2(
  sqlite3 *db,               /* SQLite connection to register module with */
  const char *zName,         /* Name of the module */
  const sqlite3_module *,    /* Methods for the module */
  void *,                    /* Client data for xCreate/xConnect */
  void(*xDestroy)(void*)     /* Module destructor function */
);

/*
** CAPI3REF: Virtual Table Instance Object {F18010}
** KEYWORDS: sqlite3_vtab
**
** Every module implementation uses a subclass of the following structure
** to describe a particular instance of the module.  Each subclass will
** be tailored to the specific needs of the module implementation.   The
** purpose of this superclass is to define certain fields that are common
** to all module implementations.
**
** Virtual tables methods can set an error message by assigning a
** string obtained from sqlite3_mprintf() to zErrMsg.  The method should
** take care that any prior string is freed by a call to sqlite3_free()
** prior to assigning a new string to zErrMsg.  After the error message
** is delivered up to the client application, the string will be automatically
** freed by sqlite3_free() and the zErrMsg field will be zeroed.  Note
** that sqlite3_mprintf() and sqlite3_free() are used on the zErrMsg field
** since virtual tables are commonly implemented in loadable extensions which
** do not have access to sqlite3MPrintf() or sqlite3Free().
*/
struct sqlite3_vtab {
  const sqlite3_module *pModule;  /* The module for this virtual table */
  int nRef;                       /* Used internally */
  char *zErrMsg;                  /* Error message from sqlite3_mprintf() */
  /* Virtual table implementations will typically add additional fields */
};

/*
** CAPI3REF: Virtual Table Cursor Object  {F18020}
** KEYWORDS: sqlite3_vtab_cursor
**
** Every module implementation uses a subclass of the following structure
** to describe cursors that point into the virtual table and are used
** to loop through the virtual table.  Cursors are created using the
** xOpen method of the module.  Each module implementation will define
** the content of a cursor structure to suit its own needs.
**
** This superclass exists in order to define fields of the cursor that
** are common to all implementations.
*/
struct sqlite3_vtab_cursor {
  sqlite3_vtab *pVtab;      /* Virtual table of this cursor */
  /* Virtual table implementations will typically add additional fields */
};

/*
** CAPI3REF: Declare The Schema Of A Virtual Table {F18280}
**
** The xCreate and xConnect methods of a module use the following API
** to declare the format (the names and datatypes of the columns) of
** the virtual tables they implement.
*/
int sqlite3_declare_vtab(sqlite3*, const char *zCreateTable);

/*
** CAPI3REF: Overload A Function For A Virtual Table {F18300}
**
** Virtual tables can provide alternative implementations of functions
** using the xFindFunction method.  But global versions of those functions
** must exist in order to be overloaded.
**
** This API makes sure a global version of a function with a particular
** name and number of parameters exists.  If no such function exists
** before this API is called, a new function is created.  The implementation
** of the new function always causes an exception to be thrown.  So
** the new function is not good for anything by itself.  Its only
** purpose is to be a place-holder function that can be overloaded
** by virtual tables.
**
** This API should be considered part of the virtual table interface,
** which is experimental and subject to change.
*/
int sqlite3_overload_function(sqlite3*, const char *zFuncName, int nArg);

/*
** The interface to the virtual-table mechanism defined above (back up
** to a comment remarkably similar to this one) is currently considered
** to be experimental.  The interface might change in incompatible ways.
** If this is a problem for you, do not use the interface at this time.
**
** When the virtual-table mechanism stabilizes, we will declare the
** interface fixed, support it indefinitely, and remove this comment.
**
****** EXPERIMENTAL - subject to change without notice **************
*/

/*
** CAPI3REF: A Handle To An Open BLOB {F17800}
**
** An instance of this object represents an open BLOB on which
** incremental I/O can be preformed.
** Objects of this type are created by
** [sqlite3_blob_open()] and destroyed by [sqlite3_blob_close()].
** The [sqlite3_blob_read()] and [sqlite3_blob_write()] interfaces
** can be used to read or write small subsections of the blob.
** The [sqlite3_blob_bytes()] interface returns the size of the
** blob in bytes.
*/
typedef struct sqlite3_blob sqlite3_blob;

/*
** CAPI3REF: Open A BLOB For Incremental I/O {F17810}
**
** This interfaces opens a handle to the blob located
** in row iRow, column zColumn, table zTable in database zDb;
** in other words,  the same blob that would be selected by:
**
** <pre>
**     SELECT zColumn FROM zDb.zTable WHERE rowid = iRow;
** </pre> {END}
**
** If the flags parameter is non-zero, the blob is opened for 
** read and write access. If it is zero, the blob is opened for read 
** access.
**
** Note that the database name is not the filename that contains
** the database but rather the symbolic name of the database that
** is assigned when the database is connected using [ATTACH].
** For the main database file, the database name is "main".  For
** TEMP tables, the database name is "temp".
**
** On success, [SQLITE_OK] is returned and the new 
** [sqlite3_blob | blob handle] is written to *ppBlob. 
** Otherwise an error code is returned and 
** any value written to *ppBlob should not be used by the caller.
** This function sets the database-handle error code and message
** accessible via [sqlite3_errcode()] and [sqlite3_errmsg()].
** 
** INVARIANTS:
**
** {F17813} A successful invocation of the [sqlite3_blob_open(D,B,T,C,R,F,P)]
**          interface opens an [sqlite3_blob] object P on the blob
**          in column C of table T in database B on [database connection] D.
**
** {F17814} A successful invocation of [sqlite3_blob_open(D,...)] starts
**          a new transaction on [database connection] D if that connection
**          is not already in a transaction.
**
** {F17816} The [sqlite3_blob_open(D,B,T,C,R,F,P)] interface opens the blob
**          for read and write access if and only if the F parameter
**          is non-zero.
**
** {F17819} The [sqlite3_blob_open()] interface returns [SQLITE_OK] on 
**          success and an appropriate [error code] on failure.
**
** {F17821} If an error occurs during evaluation of [sqlite3_blob_open(D,...)]
**          then subsequent calls to [sqlite3_errcode(D)],
**          [sqlite3_errmsg(D)], and [sqlite3_errmsg16(D)] will return
**          information approprate for that error.
*/
int sqlite3_blob_open(
  sqlite3*,
  const char *zDb,
  const char *zTable,
  const char *zColumn,
  sqlite3_int64 iRow,
  int flags,
  sqlite3_blob **ppBlob
);

/*
** CAPI3REF:  Close A BLOB Handle {F17830}
**
** Close an open [sqlite3_blob | blob handle].
**
** Closing a BLOB shall cause the current transaction to commit
** if there are no other BLOBs, no pending prepared statements, and the
** database connection is in autocommit mode.
** If any writes were made to the BLOB, they might be held in cache
** until the close operation if they will fit. {END}
** Closing the BLOB often forces the changes
** out to disk and so if any I/O errors occur, they will likely occur
** at the time when the BLOB is closed.  {F17833} Any errors that occur during
** closing are reported as a non-zero return value.
**
** The BLOB is closed unconditionally.  Even if this routine returns
** an error code, the BLOB is still closed.
**
** INVARIANTS:
**
** {F17833} The [sqlite3_blob_close(P)] interface closes an
**          [sqlite3_blob] object P previously opened using
**          [sqlite3_blob_open()].
**
** {F17836} Closing an [sqlite3_blob] object using
**          [sqlite3_blob_close()] shall cause the current transaction to
**          commit if there are no other open [sqlite3_blob] objects
**          or [prepared statements] on the same [database connection] and
**          the [database connection] is in
**          [sqlite3_get_autocommit | autocommit mode].
**
** {F17839} The [sqlite3_blob_close(P)] interfaces closes the 
**          [sqlite3_blob] object P unconditionally, even if
**          [sqlite3_blob_close(P)] returns something other than [SQLITE_OK].
**          
*/
int sqlite3_blob_close(sqlite3_blob *);

/*
** CAPI3REF:  Return The Size Of An Open BLOB {F17840}
**
** Return the size in bytes of the blob accessible via the open 
** [sqlite3_blob] object in its only argument.
**
** INVARIANTS:
**
** {F17843} The [sqlite3_blob_bytes(P)] interface returns the size
**          in bytes of the BLOB that the [sqlite3_blob] object P
**          refers to.
*/
int sqlite3_blob_bytes(sqlite3_blob *);

/*
** CAPI3REF:  Read Data From A BLOB Incrementally {F17850}
**
** This function is used to read data from an open 
** [sqlite3_blob | blob-handle] into a caller supplied buffer.
** N bytes of data are copied into buffer
** Z from the open blob, starting at offset iOffset.
**
** If offset iOffset is less than N bytes from the end of the blob, 
** [SQLITE_ERROR] is returned and no data is read.  If N or iOffset is
** less than zero [SQLITE_ERROR] is returned and no data is read.
**
** On success, SQLITE_OK is returned. Otherwise, an 
** [error code] or an [extended error code] is returned.
**
** INVARIANTS:
**
** {F17853} The [sqlite3_blob_read(P,Z,N,X)] interface reads N bytes
**          beginning at offset X from
**          the blob that [sqlite3_blob] object P refers to
**          and writes those N bytes into buffer Z.
**
** {F17856} In [sqlite3_blob_read(P,Z,N,X)] if the size of the blob
**          is less than N+X bytes, then the function returns [SQLITE_ERROR]
**          and nothing is read from the blob.
**
** {F17859} In [sqlite3_blob_read(P,Z,N,X)] if X or N is less than zero
**          then the function returns [SQLITE_ERROR]
**          and nothing is read from the blob.
**
** {F17862} The [sqlite3_blob_read(P,Z,N,X)] interface returns [SQLITE_OK]
**          if N bytes where successfully read into buffer Z.
**
** {F17865} If the requested read could not be completed,
**          the [sqlite3_blob_read(P,Z,N,X)] interface returns an
**          appropriate [error code] or [extended error code].
**
** {F17868} If an error occurs during evaluation of [sqlite3_blob_read(P,...)]
**          then subsequent calls to [sqlite3_errcode(D)],
**          [sqlite3_errmsg(D)], and [sqlite3_errmsg16(D)] will return
**          information approprate for that error, where D is the
**          database handle that was used to open blob handle P.
*/
int sqlite3_blob_read(sqlite3_blob *, void *Z, int N, int iOffset);

/*
** CAPI3REF:  Write Data Into A BLOB Incrementally {F17870}
**
** This function is used to write data into an open 
** [sqlite3_blob | blob-handle] from a user supplied buffer.
** n bytes of data are copied from the buffer
** pointed to by z into the open blob, starting at offset iOffset.
**
** If the [sqlite3_blob | blob-handle] passed as the first argument
** was not opened for writing (the flags parameter to [sqlite3_blob_open()]
*** was zero), this function returns [SQLITE_READONLY].
**
** This function may only modify the contents of the blob; it is
** not possible to increase the size of a blob using this API.
** If offset iOffset is less than n bytes from the end of the blob, 
** [SQLITE_ERROR] is returned and no data is written.  If n is
** less than zero [SQLITE_ERROR] is returned and no data is written.
**
** On success, SQLITE_OK is returned. Otherwise, an 
** [error code] or an [extended error code] is returned.
**
** INVARIANTS:
**
** {F17873} The [sqlite3_blob_write(P,Z,N,X)] interface writes N bytes
**          from buffer Z into
**          the blob that [sqlite3_blob] object P refers to
**          beginning at an offset of X into the blob.
**
** {F17875} The [sqlite3_blob_write(P,Z,N,X)] interface returns
**          [SQLITE_READONLY] if the [sqlite3_blob] object P was
**          [sqlite3_blob_open | opened] for reading only.
**
** {F17876} In [sqlite3_blob_write(P,Z,N,X)] if the size of the blob
**          is less than N+X bytes, then the function returns [SQLITE_ERROR]
**          and nothing is written into the blob.
**
** {F17879} In [sqlite3_blob_write(P,Z,N,X)] if X or N is less than zero
**          then the function returns [SQLITE_ERROR]
**          and nothing is written into the blob.
**
** {F17882} The [sqlite3_blob_write(P,Z,N,X)] interface returns [SQLITE_OK]
**          if N bytes where successfully written into blob.
**
** {F17885} If the requested write could not be completed,
**          the [sqlite3_blob_write(P,Z,N,X)] interface returns an
**          appropriate [error code] or [extended error code].
**
** {F17888} If an error occurs during evaluation of [sqlite3_blob_write(D,...)]
**          then subsequent calls to [sqlite3_errcode(D)],
**          [sqlite3_errmsg(D)], and [sqlite3_errmsg16(D)] will return
**          information approprate for that error.
*/
int sqlite3_blob_write(sqlite3_blob *, const void *z, int n, int iOffset);

/*
** CAPI3REF:  Virtual File System Objects {F11200}
**
** A virtual filesystem (VFS) is an [sqlite3_vfs] object
** that SQLite uses to interact
** with the underlying operating system.  Most SQLite builds come with a
** single default VFS that is appropriate for the host computer.
** New VFSes can be registered and existing VFSes can be unregistered.
** The following interfaces are provided.
**
** The sqlite3_vfs_find() interface returns a pointer to 
** a VFS given its name.  Names are case sensitive.
** Names are zero-terminated UTF-8 strings.
** If there is no match, a NULL
** pointer is returned.  If zVfsName is NULL then the default 
** VFS is returned. 
**
** New VFSes are registered with sqlite3_vfs_register().
** Each new VFS becomes the default VFS if the makeDflt flag is set.
** The same VFS can be registered multiple times without injury.
** To make an existing VFS into the default VFS, register it again
** with the makeDflt flag set.  If two different VFSes with the
** same name are registered, the behavior is undefined.  If a
** VFS is registered with a name that is NULL or an empty string,
** then the behavior is undefined.
** 
** Unregister a VFS with the sqlite3_vfs_unregister() interface.
** If the default VFS is unregistered, another VFS is chosen as
** the default.  The choice for the new VFS is arbitrary.
**
** INVARIANTS:
**
** {F11203} The [sqlite3_vfs_find(N)] interface returns a pointer to the
**          registered [sqlite3_vfs] object whose name exactly matches
**          the zero-terminated UTF-8 string N, or it returns NULL if
**          there is no match.
**
** {F11206} If the N parameter to [sqlite3_vfs_find(N)] is NULL then
**          the function returns a pointer to the default [sqlite3_vfs]
**          object if there is one, or NULL if there is no default 
**          [sqlite3_vfs] object.
**
** {F11209} The [sqlite3_vfs_register(P,F)] interface registers the
**          well-formed [sqlite3_vfs] object P using the name given
**          by the zName field of the object.
**
** {F11212} Using the [sqlite3_vfs_register(P,F)] interface to register
**          the same [sqlite3_vfs] object multiple times is a harmless no-op.
**
** {F11215} The [sqlite3_vfs_register(P,F)] interface makes the
**          the [sqlite3_vfs] object P the default [sqlite3_vfs] object
**          if F is non-zero.
**
** {F11218} The [sqlite3_vfs_unregister(P)] interface unregisters the
**          [sqlite3_vfs] object P so that it is no longer returned by
**          subsequent calls to [sqlite3_vfs_find()].
*/
sqlite3_vfs *sqlite3_vfs_find(const char *zVfsName);
int sqlite3_vfs_register(sqlite3_vfs*, int makeDflt);
int sqlite3_vfs_unregister(sqlite3_vfs*);

/*
** CAPI3REF: Mutexes {F17000}
**
** The SQLite core uses these routines for thread
** synchronization.  Though they are intended for internal
** use by SQLite, code that links against SQLite is
** permitted to use any of these routines.
**
** The SQLite source code contains multiple implementations 
** of these mutex routines.  An appropriate implementation
** is selected automatically at compile-time.  The following
** implementations are available in the SQLite core:
**
** <ul>
** <li>   SQLITE_MUTEX_OS2
** <li>   SQLITE_MUTEX_PTHREAD
** <li>   SQLITE_MUTEX_W32
** <li>   SQLITE_MUTEX_NOOP
** </ul>
**
** The SQLITE_MUTEX_NOOP implementation is a set of routines 
** that does no real locking and is appropriate for use in 
** a single-threaded application.  The SQLITE_MUTEX_OS2,
** SQLITE_MUTEX_PTHREAD, and SQLITE_MUTEX_W32 implementations
** are appropriate for use on os/2, unix, and windows.
** 
** If SQLite is compiled with the SQLITE_MUTEX_APPDEF preprocessor
** macro defined (with "-DSQLITE_MUTEX_APPDEF=1"), then no mutex
** implementation is included with the library.  The
** mutex interface routines defined here become external
** references in the SQLite library for which implementations
** must be provided by the application.  This facility allows an
** application that links against SQLite to provide its own mutex
** implementation without having to modify the SQLite core.
**
** {F17011} The sqlite3_mutex_alloc() routine allocates a new
** mutex and returns a pointer to it. {F17012} If it returns NULL
** that means that a mutex could not be allocated. {F17013} SQLite
** will unwind its stack and return an error. {F17014} The argument
** to sqlite3_mutex_alloc() is one of these integer constants:
**
** <ul>
** <li>  SQLITE_MUTEX_FAST
** <li>  SQLITE_MUTEX_RECURSIVE
** <li>  SQLITE_MUTEX_STATIC_MASTER
** <li>  SQLITE_MUTEX_STATIC_MEM
** <li>  SQLITE_MUTEX_STATIC_MEM2
** <li>  SQLITE_MUTEX_STATIC_PRNG
** <li>  SQLITE_MUTEX_STATIC_LRU
** <li>  SQLITE_MUTEX_STATIC_LRU2
** </ul> {END}
**
** {F17015} The first two constants cause sqlite3_mutex_alloc() to create
** a new mutex.  The new mutex is recursive when SQLITE_MUTEX_RECURSIVE
** is used but not necessarily so when SQLITE_MUTEX_FAST is used. {END}
** The mutex implementation does not need to make a distinction
** between SQLITE_MUTEX_RECURSIVE and SQLITE_MUTEX_FAST if it does
** not want to.  {F17016} But SQLite will only request a recursive mutex in
** cases where it really needs one.  {END} If a faster non-recursive mutex
** implementation is available on the host platform, the mutex subsystem
** might return such a mutex in response to SQLITE_MUTEX_FAST.
**
** {F17017} The other allowed parameters to sqlite3_mutex_alloc() each return
** a pointer to a static preexisting mutex. {END}  Four static mutexes are
** used by the current version of SQLite.  Future versions of SQLite
** may add additional static mutexes.  Static mutexes are for internal
** use by SQLite only.  Applications that use SQLite mutexes should
** use only the dynamic mutexes returned by SQLITE_MUTEX_FAST or
** SQLITE_MUTEX_RECURSIVE.
**
** {F17018} Note that if one of the dynamic mutex parameters (SQLITE_MUTEX_FAST
** or SQLITE_MUTEX_RECURSIVE) is used then sqlite3_mutex_alloc()
** returns a different mutex on every call.  {F17034} But for the static 
** mutex types, the same mutex is returned on every call that has
** the same type number. {END}
**
** {F17019} The sqlite3_mutex_free() routine deallocates a previously
** allocated dynamic mutex. {F17020} SQLite is careful to deallocate every
** dynamic mutex that it allocates. {U17021} The dynamic mutexes must not be in 
** use when they are deallocated. {U17022} Attempting to deallocate a static
** mutex results in undefined behavior. {F17023} SQLite never deallocates
** a static mutex. {END}
**
** The sqlite3_mutex_enter() and sqlite3_mutex_try() routines attempt
** to enter a mutex. {F17024} If another thread is already within the mutex,
** sqlite3_mutex_enter() will block and sqlite3_mutex_try() will return
** SQLITE_BUSY. {F17025}  The sqlite3_mutex_try() interface returns SQLITE_OK
** upon successful entry.  {F17026} Mutexes created using
** SQLITE_MUTEX_RECURSIVE can be entered multiple times by the same thread.
** {F17027} In such cases the,
** mutex must be exited an equal number of times before another thread
** can enter.  {U17028} If the same thread tries to enter any other
** kind of mutex more than once, the behavior is undefined.
** {F17029} SQLite will never exhibit
** such behavior in its own use of mutexes. {END}
**
** Some systems (ex: windows95) do not the operation implemented by
** sqlite3_mutex_try().  On those systems, sqlite3_mutex_try() will
** always return SQLITE_BUSY.  {F17030} The SQLite core only ever uses
** sqlite3_mutex_try() as an optimization so this is acceptable behavior. {END}
**
** {F17031} The sqlite3_mutex_leave() routine exits a mutex that was
** previously entered by the same thread.  {U17032} The behavior
** is undefined if the mutex is not currently entered by the
** calling thread or is not currently allocated.  {F17033} SQLite will
** never do either. {END}
**
** See also: [sqlite3_mutex_held()] and [sqlite3_mutex_notheld()].
*/
sqlite3_mutex *sqlite3_mutex_alloc(int);
void sqlite3_mutex_free(sqlite3_mutex*);
void sqlite3_mutex_enter(sqlite3_mutex*);
int sqlite3_mutex_try(sqlite3_mutex*);
void sqlite3_mutex_leave(sqlite3_mutex*);

/*
** CAPI3REF: Mutex Verifcation Routines {F17080}
**
** The sqlite3_mutex_held() and sqlite3_mutex_notheld() routines
** are intended for use inside assert() statements. {F17081} The SQLite core
** never uses these routines except inside an assert() and applications
** are advised to follow the lead of the core.  {F17082} The core only
** provides implementations for these routines when it is compiled
** with the SQLITE_DEBUG flag.  {U17087} External mutex implementations
** are only required to provide these routines if SQLITE_DEBUG is
** defined and if NDEBUG is not defined.
**
** {F17083} These routines should return true if the mutex in their argument
** is held or not held, respectively, by the calling thread. {END}
**
** {X17084} The implementation is not required to provided versions of these
** routines that actually work.
** If the implementation does not provide working
** versions of these routines, it should at least provide stubs
** that always return true so that one does not get spurious
** assertion failures. {END}
**
** {F17085} If the argument to sqlite3_mutex_held() is a NULL pointer then
** the routine should return 1.  {END} This seems counter-intuitive since
** clearly the mutex cannot be held if it does not exist.  But the
** the reason the mutex does not exist is because the build is not
** using mutexes.  And we do not want the assert() containing the
** call to sqlite3_mutex_held() to fail, so a non-zero return is
** the appropriate thing to do.  {F17086} The sqlite3_mutex_notheld() 
** interface should also return 1 when given a NULL pointer.
*/
int sqlite3_mutex_held(sqlite3_mutex*);
int sqlite3_mutex_notheld(sqlite3_mutex*);

/*
** CAPI3REF: Mutex Types {F17001}
**
** {F17002} The [sqlite3_mutex_alloc()] interface takes a single argument
** which is one of these integer constants. {END}
*/
#define SQLITE_MUTEX_FAST             0
#define SQLITE_MUTEX_RECURSIVE        1
#define SQLITE_MUTEX_STATIC_MASTER    2
#define SQLITE_MUTEX_STATIC_MEM       3  /* sqlite3_malloc() */
#define SQLITE_MUTEX_STATIC_MEM2      4  /* sqlite3_release_memory() */
#define SQLITE_MUTEX_STATIC_PRNG      5  /* sqlite3_random() */
#define SQLITE_MUTEX_STATIC_LRU       6  /* lru page list */
#define SQLITE_MUTEX_STATIC_LRU2      7  /* lru page list */

/*
** CAPI3REF: Low-Level Control Of Database Files {F11300}
**
** {F11301} The [sqlite3_file_control()] interface makes a direct call to the
** xFileControl method for the [sqlite3_io_methods] object associated
** with a particular database identified by the second argument. {F11302} The
** name of the database is the name assigned to the database by the
** <a href="lang_attach.html">ATTACH</a> SQL command that opened the
** database. {F11303} To control the main database file, use the name "main"
** or a NULL pointer. {F11304} The third and fourth parameters to this routine
** are passed directly through to the second and third parameters of
** the xFileControl method.  {F11305} The return value of the xFileControl
** method becomes the return value of this routine.
**
** {F11306} If the second parameter (zDbName) does not match the name of any
** open database file, then SQLITE_ERROR is returned. {F11307} This error
** code is not remembered and will not be recalled by [sqlite3_errcode()]
** or [sqlite3_errmsg()]. {U11308} The underlying xFileControl method might
** also return SQLITE_ERROR.  {U11309} There is no way to distinguish between
** an incorrect zDbName and an SQLITE_ERROR return from the underlying
** xFileControl method. {END}
**
** See also: [SQLITE_FCNTL_LOCKSTATE]
*/
int sqlite3_file_control(sqlite3*, const char *zDbName, int op, void*);

/*
** CAPI3REF: Testing Interface {F11400}
**
** The sqlite3_test_control() interface is used to read out internal
** state of SQLite and to inject faults into SQLite for testing
** purposes.  The first parameter a operation code that determines
** the number, meaning, and operation of all subsequent parameters.
**
** This interface is not for use by applications.  It exists solely
** for verifying the correct operation of the SQLite library.  Depending
** on how the SQLite library is compiled, this interface might not exist.
**
** The details of the operation codes, their meanings, the parameters
** they take, and what they do are all subject to change without notice.
** Unlike most of the SQLite API, this function is not guaranteed to
** operate consistently from one release to the next.
*/
int sqlite3_test_control(int op, ...);

/*
** CAPI3REF: Testing Interface Operation Codes {F11410}
**
** These constants are the valid operation code parameters used
** as the first argument to [sqlite3_test_control()].
**
** These parameters and their meansing are subject to change
** without notice.  These values are for testing purposes only.
** Applications should not use any of these parameters or the
** [sqlite3_test_control()] interface.
*/
#define SQLITE_TESTCTRL_FAULT_CONFIG             1
#define SQLITE_TESTCTRL_FAULT_FAILURES           2
#define SQLITE_TESTCTRL_FAULT_BENIGN_FAILURES    3
#define SQLITE_TESTCTRL_FAULT_PENDING            4
#define SQLITE_TESTCTRL_PRNG_SAVE                5
#define SQLITE_TESTCTRL_PRNG_RESTORE             6
#define SQLITE_TESTCTRL_PRNG_RESET               7
#define SQLITE_TESTCTRL_BITVEC_TEST              8


/*
** Undo the hack that converts floating point types to integer for
** builds on processors without floating point support.
*/
#ifdef SQLITE_OMIT_FLOATING_POINT
# undef double
#endif

#ifdef __cplusplus
}  /* End of the 'extern "C"' block */
#endif
#endif

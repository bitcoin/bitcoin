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
** Internal interface definitions for SQLite.
**
** @(#) $Id$
*/
#ifndef _SQLITEINT_H_
#define _SQLITEINT_H_
#define DB_SQL
/*
** Include the configuration header output by 'configure' if we're using the
** autoconf-based build
*/
#ifdef _HAVE_SQLITE_CONFIG_H
#include "config.h"
#endif

#include "sqliteLimit.h"

/* Disable nuisance warnings on Borland compilers */
#if defined(__BORLANDC__)
#pragma warn -rch /* unreachable code */
#pragma warn -ccc /* Condition is always true or false */
#pragma warn -aus /* Assigned value is never used */
#pragma warn -csu /* Comparing signed and unsigned */
#pragma warn -spa /* Suspicous pointer arithmetic */
#endif

/* Needed for various definitions... */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/*
** Include standard header files as necessary
*/
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

/*
** A macro used to aid in coverage testing.  When doing coverage
** testing, the condition inside the argument must be evaluated 
** both true and false in order to get full branch coverage.
** This macro can be inserted to ensure adequate test coverage
** in places where simple condition/decision coverage is inadequate.
*/
#ifdef SQLITE_COVERAGE_TEST
  void sqlite3Coverage(int);
# define testcase(X)  if( X ){ sqlite3Coverage(__LINE__); }
#else
# define testcase(X)
#endif


/*
** The macro unlikely() is a hint that surrounds a boolean
** expression that is usually false.  Macro likely() surrounds
** a boolean expression that is usually true.  GCC is able to
** use these hints to generate better code, sometimes.
*/
#if defined(__GNUC__) && 0
# define likely(X)    __builtin_expect((X),1)
# define unlikely(X)  __builtin_expect((X),0)
#else
# define likely(X)    !!(X)
# define unlikely(X)  !!(X)
#endif


/*
** These #defines should enable >2GB file support on Posix if the
** underlying operating system supports it.  If the OS lacks
** large file support, or if the OS is windows, these should be no-ops.
**
** Ticket #2739:  The _LARGEFILE_SOURCE macro must appear before any
** system #includes.  Hence, this block of code must be the very first
** code in all source files.
**
** Large file support can be disabled using the -DSQLITE_DISABLE_LFS switch
** on the compiler command line.  This is necessary if you are compiling
** on a recent machine (ex: RedHat 7.2) but you want your code to work
** on an older machine (ex: RedHat 6.0).  If you compile on RedHat 7.2
** without this option, LFS is enable.  But LFS does not exist in the kernel
** in RedHat 6.0, so the code won't work.  Hence, for maximum binary
** portability you should omit LFS.
**
** Similar is true for MacOS.  LFS is only supported on MacOS 9 and later.
*/
#ifndef SQLITE_DISABLE_LFS
# define _LARGE_FILE       1
# ifndef _FILE_OFFSET_BITS
#   define _FILE_OFFSET_BITS 64
# endif
# define _LARGEFILE_SOURCE 1
#endif


/*
** The SQLITE_THREADSAFE macro must be defined as either 0 or 1.
** Older versions of SQLite used an optional THREADSAFE macro.
** We support that for legacy
*/
#if !defined(SQLITE_THREADSAFE)
#if defined(THREADSAFE)
# define SQLITE_THREADSAFE THREADSAFE
#else
# define SQLITE_THREADSAFE 1
#endif
#endif

/*
** Exactly one of the following macros must be defined in order to
** specify which memory allocation subsystem to use.
**
**     SQLITE_SYSTEM_MALLOC          // Use normal system malloc()
**     SQLITE_MEMDEBUG               // Debugging version of system malloc()
**     SQLITE_MEMORY_SIZE            // internal allocator #1
**     SQLITE_MMAP_HEAP_SIZE         // internal mmap() allocator
**     SQLITE_POW2_MEMORY_SIZE       // internal power-of-two allocator
**
** If none of the above are defined, then set SQLITE_SYSTEM_MALLOC as
** the default.
*/
#if defined(SQLITE_SYSTEM_MALLOC)+defined(SQLITE_MEMDEBUG)+\
    defined(SQLITE_MEMORY_SIZE)+defined(SQLITE_MMAP_HEAP_SIZE)+\
    defined(SQLITE_POW2_MEMORY_SIZE)>1
# error "At most one of the following compile-time configuration options\
 is allows: SQLITE_SYSTEM_MALLOC, SQLITE_MEMDEBUG, SQLITE_MEMORY_SIZE,\
 SQLITE_MMAP_HEAP_SIZE, SQLITE_POW2_MEMORY_SIZE"
#endif
#if defined(SQLITE_SYSTEM_MALLOC)+defined(SQLITE_MEMDEBUG)+\
    defined(SQLITE_MEMORY_SIZE)+defined(SQLITE_MMAP_HEAP_SIZE)+\
    defined(SQLITE_POW2_MEMORY_SIZE)==0
# define SQLITE_SYSTEM_MALLOC 1
#endif

/*
** If SQLITE_MALLOC_SOFT_LIMIT is defined, then try to keep the
** sizes of memory allocations below this value where possible.
*/
#if defined(SQLITE_POW2_MEMORY_SIZE) && !defined(SQLITE_MALLOC_SOFT_LIMIT)
# define SQLITE_MALLOC_SOFT_LIMIT 1024
#endif

/*
** We need to define _XOPEN_SOURCE as follows in order to enable
** recursive mutexes on most unix systems.  But Mac OS X is different.
** The _XOPEN_SOURCE define causes problems for Mac OS X we are told,
** so it is omitted there.  See ticket #2673.
**
** Later we learn that _XOPEN_SOURCE is poorly or incorrectly
** implemented on some systems.  So we avoid defining it at all
** if it is already defined or if it is unneeded because we are
** not doing a threadsafe build.  Ticket #2681.
**
** See also ticket #2741.
*/
#if !defined(_XOPEN_SOURCE) && !defined(__DARWIN__) && !defined(__APPLE__) && SQLITE_THREADSAFE
#  define _XOPEN_SOURCE 500  /* Needed to enable pthread recursive mutexes */
#endif

#if defined(SQLITE_TCL) || defined(TCLSH)
# include <tcl.h>
#endif

/*
** Many people are failing to set -DNDEBUG=1 when compiling SQLite.
** Setting NDEBUG makes the code smaller and run faster.  So the following
** lines are added to automatically set NDEBUG unless the -DSQLITE_DEBUG=1
** option is set.  Thus NDEBUG becomes an opt-in rather than an opt-out
** feature.
*/
#if !defined(NDEBUG) && !defined(SQLITE_DEBUG) 
# define NDEBUG 1
#endif

#include "sqlite3.h"

#ifndef DB_SQL
#include "hash.h"
#endif

#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stddef.h>

/*
** If compiling for a processor that lacks floating point support,
** substitute integer for floating-point
*/
#ifdef SQLITE_OMIT_FLOATING_POINT
# define double sqlite_int64
# define LONGDOUBLE_TYPE sqlite_int64
# ifndef SQLITE_BIG_DBL
#   define SQLITE_BIG_DBL (0x7fffffffffffffff)
# endif
# define SQLITE_OMIT_DATETIME_FUNCS 1
# define SQLITE_OMIT_TRACE 1
# undef SQLITE_MIXED_ENDIAN_64BIT_FLOAT
#endif
#ifndef SQLITE_BIG_DBL
# define SQLITE_BIG_DBL (1e99)
#endif

/*
** OMIT_TEMPDB is set to 1 if SQLITE_OMIT_TEMPDB is defined, or 0
** afterward. Having this macro allows us to cause the C compiler 
** to omit code used by TEMP tables without messy #ifndef statements.
*/
#ifdef SQLITE_OMIT_TEMPDB
#define OMIT_TEMPDB 1
#else
#define OMIT_TEMPDB 0
#endif

/*
** If the following macro is set to 1, then NULL values are considered
** distinct when determining whether or not two entries are the same
** in a UNIQUE index.  This is the way PostgreSQL, Oracle, DB2, MySQL,
** OCELOT, and Firebird all work.  The SQL92 spec explicitly says this
** is the way things are suppose to work.
**
** If the following macro is set to 0, the NULLs are indistinct for
** a UNIQUE index.  In this mode, you can only have a single NULL entry
** for a column declared UNIQUE.  This is the way Informix and SQL Server
** work.
*/
#define NULL_DISTINCT_FOR_UNIQUE 1

/*
** The "file format" number is an integer that is incremented whenever
** the VDBE-level file format changes.  The following macros define the
** the default file format for new databases and the maximum file format
** that the library can read.
*/
#define SQLITE_MAX_FILE_FORMAT 4
#ifndef SQLITE_DEFAULT_FILE_FORMAT
# define SQLITE_DEFAULT_FILE_FORMAT 1
#endif

/*
** Provide a default value for TEMP_STORE in case it is not specified
** on the command-line
*/
#ifndef TEMP_STORE
# define TEMP_STORE 1
#endif

/*
** GCC does not define the offsetof() macro so we'll have to do it
** ourselves.
*/
#ifndef offsetof
#define offsetof(STRUCTURE,FIELD) ((int)((char*)&((STRUCTURE*)0)->FIELD))
#endif

/*
** Check to see if this machine uses EBCDIC.  (Yes, believe it or
** not, there are still machines out there that use EBCDIC.)
*/
#if 'A' == '\301'
# define SQLITE_EBCDIC 1
#else
# define SQLITE_ASCII 1
#endif

/*
** Integers of known sizes.  These typedefs might change for architectures
** where the sizes very.  Preprocessor macros are available so that the
** types can be conveniently redefined at compile-type.  Like this:
**
**         cc '-DUINTPTR_TYPE=long long int' ...
*/
#ifndef UINT32_TYPE
# ifdef HAVE_UINT32_T
#  define UINT32_TYPE uint32_t
# else
#  define UINT32_TYPE unsigned int
# endif
#endif
#ifndef UINT16_TYPE
# ifdef HAVE_UINT16_T
#  define UINT16_TYPE uint16_t
# else
#  define UINT16_TYPE unsigned short int
# endif
#endif
#ifndef INT16_TYPE
# ifdef HAVE_INT16_T
#  define INT16_TYPE int16_t
# else
#  define INT16_TYPE short int
# endif
#endif
#ifndef UINT8_TYPE
# ifdef HAVE_UINT8_T
#  define UINT8_TYPE uint8_t
# else
#  define UINT8_TYPE unsigned char
# endif
#endif
#ifndef INT8_TYPE
# ifdef HAVE_INT8_T
#  define INT8_TYPE int8_t
# else
#  define INT8_TYPE signed char
# endif
#endif
#ifndef LONGDOUBLE_TYPE
# define LONGDOUBLE_TYPE long double
#endif
typedef sqlite_int64 i64;          /* 8-byte signed integer */
typedef sqlite_uint64 u64;         /* 8-byte unsigned integer */
typedef UINT32_TYPE u32;           /* 4-byte unsigned integer */
typedef UINT16_TYPE u16;           /* 2-byte unsigned integer */
typedef INT16_TYPE i16;            /* 2-byte signed integer */
typedef UINT8_TYPE u8;             /* 1-byte unsigned integer */
typedef UINT8_TYPE i8;             /* 1-byte signed integer */

/*
** Macros to determine whether the machine is big or little endian,
** evaluated at runtime.
*/
#ifdef SQLITE_AMALGAMATION
const int sqlite3one;
#else
extern const int sqlite3one;
#endif
#if defined(i386) || defined(__i386__) || defined(_M_IX86)
# define SQLITE_BIGENDIAN    0
# define SQLITE_LITTLEENDIAN 1
# define SQLITE_UTF16NATIVE  SQLITE_UTF16LE
#else
# define SQLITE_BIGENDIAN    (*(char *)(&sqlite3one)==0)
# define SQLITE_LITTLEENDIAN (*(char *)(&sqlite3one)==1)
# define SQLITE_UTF16NATIVE (SQLITE_BIGENDIAN?SQLITE_UTF16BE:SQLITE_UTF16LE)
#endif

/*
** Constants for the largest and smallest possible 64-bit signed integers.
** These macros are designed to work correctly on both 32-bit and 64-bit
** compilers.
*/
#define LARGEST_INT64  (0xffffffff|(((i64)0x7fffffff)<<32))
#define SMALLEST_INT64 (((i64)-1) - LARGEST_INT64)

/*
** An instance of the following structure is used to store the busy-handler
** callback for a given sqlite handle. 
**
** The sqlite.busyHandler member of the sqlite struct contains the busy
** callback for the database handle. Each pager opened via the sqlite
** handle is passed a pointer to sqlite.busyHandler. The busy-handler
** callback is currently invoked only from within pager.c.
*/
typedef struct BusyHandler BusyHandler;
struct BusyHandler {
  int (*xFunc)(void *,int);  /* The busy callback */
  void *pArg;                /* First arg to busy callback */
  int nBusy;                 /* Incremented with each busy call */
};

/*
** Name of the master database table.  The master database table
** is a special table that holds the names and attributes of all
** user tables and indices.
*/
#define MASTER_NAME       "sqlite_master"
#define TEMP_MASTER_NAME  "sqlite_temp_master"

/*
** The root-page of the master database table.
*/
#define MASTER_ROOT       1

/*
** The name of the schema table.
*/
#define SCHEMA_TABLE(x)  ((!OMIT_TEMPDB)&&(x==1)?TEMP_MASTER_NAME:MASTER_NAME)

/*
** A convenience macro that returns the number of elements in
** an array.
*/
#define ArraySize(X)    (sizeof(X)/sizeof(X[0]))

/*
** Forward references to structures
*/
typedef struct AggInfo AggInfo;
typedef struct AuthContext AuthContext;
typedef struct Bitvec Bitvec;
typedef struct CollSeq CollSeq;
typedef struct Column Column;
typedef struct Db Db;
typedef struct Schema Schema;
typedef struct Expr Expr;
typedef struct ExprList ExprList;
typedef struct FKey FKey;
typedef struct FuncDef FuncDef;
typedef struct IdList IdList;
typedef struct Index Index;
typedef struct KeyClass KeyClass;
typedef struct KeyInfo KeyInfo;
typedef struct Module Module;
typedef struct NameContext NameContext;
typedef struct Parse Parse;
typedef struct Select Select;
typedef struct SrcList SrcList;
typedef struct StrAccum StrAccum;
typedef struct Table Table;
typedef struct TableLock TableLock;
typedef struct Token Token;
typedef struct TriggerStack TriggerStack;
typedef struct TriggerStep TriggerStep;
typedef struct Trigger Trigger;
typedef struct WhereInfo WhereInfo;
typedef struct WhereLevel WhereLevel;

/*
** Defer sourcing vdbe.h and btree.h until after the "u8" and 
** "BusyHandler" typedefs. vdbe.h also requires a few of the opaque
** pointer types (i.e. FuncDef) defined above.
*/
#ifndef DB_SQL
#include "btree.h"
#include "vdbe.h"
#include "pager.h"

#include "os.h"
#include "mutex.h"
#endif

/*
** Each database file to be accessed by the system is an instance
** of the following structure.  There are normally two of these structures
** in the sqlite.aDb[] array.  aDb[0] is the main database file and
** aDb[1] is the database file used to hold temporary tables.  Additional
** databases may be attached.
*/
struct Db {
  char *zName;         /* Name of this database */
#ifndef DB_SQL
  Btree *pBt;          /* The B*Tree structure for this database file */
#endif
  u8 inTrans;          /* 0: not writable.  1: Transaction.  2: Checkpoint */
  u8 safety_level;     /* How aggressive at synching data to disk */
  void *pAux;               /* Auxiliary data.  Usually NULL */
  void (*xFreeAux)(void*);  /* Routine to free pAux */
  Schema *pSchema;     /* Pointer to database schema (possibly shared) */
};

/*
** An instance of the following structure stores a database schema.
**
** If there are no virtual tables configured in this schema, the
** Schema.db variable is set to NULL. After the first virtual table
** has been added, it is set to point to the database connection 
** used to create the connection. Once a virtual table has been
** added to the Schema structure and the Schema.db variable populated, 
** only that database connection may use the Schema to prepare 
** statements.
*/
struct Schema {
  int schema_cookie;   /* Database schema version number for this file */
#ifndef DB_SQL
  Hash tblHash;        /* All tables indexed by name */
  Hash idxHash;        /* All (named) indices indexed by name */
  Hash trigHash;       /* All triggers indexed by name */
  Hash aFKey;          /* Foreign keys indexed by to-table */
#endif
  Table *pSeqTab;      /* The sqlite_sequence table used by AUTOINCREMENT */
  u8 file_format;      /* Schema format version for this file */
  u8 enc;              /* Text encoding used by this database */
  u16 flags;           /* Flags associated with this schema */
  int cache_size;      /* Number of pages to use in the cache */
#ifndef SQLITE_OMIT_VIRTUALTABLE
  sqlite3 *db;         /* "Owner" connection. See comment above */
#endif
};

/*
** These macros can be used to test, set, or clear bits in the 
** Db.flags field.
*/
#define DbHasProperty(D,I,P)     (((D)->aDb[I].pSchema->flags&(P))==(P))
#define DbHasAnyProperty(D,I,P)  (((D)->aDb[I].pSchema->flags&(P))!=0)
#define DbSetProperty(D,I,P)     (D)->aDb[I].pSchema->flags|=(P)
#define DbClearProperty(D,I,P)   (D)->aDb[I].pSchema->flags&=~(P)

/*
** Allowed values for the DB.flags field.
**
** The DB_SchemaLoaded flag is set after the database schema has been
** read into internal hash tables.
**
** DB_UnresetViews means that one or more views have column names that
** have been filled out.  If the schema changes, these column names might
** changes and so the view will need to be reset.
*/
#define DB_SchemaLoaded    0x0001  /* The schema has been loaded */
#define DB_UnresetViews    0x0002  /* Some views have defined column names */
#define DB_Empty           0x0004  /* The file is empty (length 0 bytes) */

/*
** The number of different kinds of things that can be limited
** using the sqlite3_limit() interface.
*/
#define SQLITE_N_LIMIT (SQLITE_LIMIT_VARIABLE_NUMBER+1)

/*
** Each database is an instance of the following structure.
**
** The sqlite.lastRowid records the last insert rowid generated by an
** insert statement.  Inserts on views do not affect its value.  Each
** trigger has its own context, so that lastRowid can be updated inside
** triggers as usual.  The previous value will be restored once the trigger
** exits.  Upon entering a before or instead of trigger, lastRowid is no
** longer (since after version 2.8.12) reset to -1.
**
** The sqlite.nChange does not count changes within triggers and keeps no
** context.  It is reset at start of sqlite3_exec.
** The sqlite.lsChange represents the number of changes made by the last
** insert, update, or delete statement.  It remains constant throughout the
** length of a statement and is then updated by OP_SetCounts.  It keeps a
** context stack just like lastRowid so that the count of changes
** within a trigger is not seen outside the trigger.  Changes to views do not
** affect the value of lsChange.
** The sqlite.csChange keeps track of the number of current changes (since
** the last statement) and is used to update sqlite_lsChange.
**
** The member variables sqlite.errCode, sqlite.zErrMsg and sqlite.zErrMsg16
** store the most recent error code and, if applicable, string. The
** internal function sqlite3Error() is used to set these variables
** consistently.
*/
struct sqlite3 {
  sqlite3_vfs *pVfs;            /* OS Interface */
  int nDb;                      /* Number of backends currently in use */
  Db *aDb;                      /* All backends */
  int flags;                    /* Miscellanous flags. See below */
  int openFlags;                /* Flags passed to sqlite3_vfs.xOpen() */
  int errCode;                  /* Most recent error code (SQLITE_*) */
  int errMask;                  /* & result codes with this before returning */
  u8 autoCommit;                /* The auto-commit flag. */
  u8 temp_store;                /* 1: file 2: memory 0: default */
  u8 mallocFailed;              /* True if we have seen a malloc failure */
  u8 dfltLockMode;              /* Default locking-mode for attached dbs */
  u8 dfltJournalMode;           /* Default journal mode for attached dbs */
  signed char nextAutovac;      /* Autovac setting after VACUUM if >=0 */
  int nextPagesize;             /* Pagesize after VACUUM if >0 */
  int nTable;                   /* Number of tables in the database */
  CollSeq *pDfltColl;           /* The default collating sequence (BINARY) */
  i64 lastRowid;                /* ROWID of most recent insert (see above) */
  i64 priorNewRowid;            /* Last randomly generated ROWID */
  int magic;                    /* Magic number for detect library misuse */
  int nChange;                  /* Value returned by sqlite3_changes() */
  int nTotalChange;             /* Value returned by sqlite3_total_changes() */
  sqlite3_mutex *mutex;         /* Connection mutex */
  int aLimit[SQLITE_N_LIMIT];   /* Limits */
  struct sqlite3InitInfo {      /* Information used during initialization */
    int iDb;                    /* When back is being initialized */
    int newTnum;                /* Rootpage of table being initialized */
    u8 busy;                    /* TRUE if currently initializing */
  } init;
  int nExtension;               /* Number of loaded extensions */
  void **aExtension;            /* Array of shared libraray handles */
  struct Vdbe *pVdbe;           /* List of active virtual machines */
  int activeVdbeCnt;            /* Number of vdbes currently executing */
  void (*xTrace)(void*,const char*);        /* Trace function */
  void *pTraceArg;                          /* Argument to the trace function */
  void (*xProfile)(void*,const char*,u64);  /* Profiling function */
  void *pProfileArg;                        /* Argument to profile function */
  void *pCommitArg;                 /* Argument to xCommitCallback() */   
  int (*xCommitCallback)(void*);    /* Invoked at every commit. */
  void *pRollbackArg;               /* Argument to xRollbackCallback() */   
  void (*xRollbackCallback)(void*); /* Invoked at every commit. */
  void *pUpdateArg;
  void (*xUpdateCallback)(void*,int, const char*,const char*,sqlite_int64);
  void(*xCollNeeded)(void*,sqlite3*,int eTextRep,const char*);
  void(*xCollNeeded16)(void*,sqlite3*,int eTextRep,const void*);
  void *pCollNeededArg;
  sqlite3_value *pErr;          /* Most recent error message */
  char *zErrMsg;                /* Most recent error message (UTF-8 encoded) */
  char *zErrMsg16;              /* Most recent error message (UTF-16 encoded) */
  union {
    int isInterrupted;          /* True if sqlite3_interrupt has been called */
    double notUsed1;            /* Spacer */
  } u1;
#ifndef SQLITE_OMIT_AUTHORIZATION
  int (*xAuth)(void*,int,const char*,const char*,const char*,const char*);
                                /* Access authorization function */
  void *pAuthArg;               /* 1st argument to the access auth function */
#endif
#ifndef SQLITE_OMIT_PROGRESS_CALLBACK
  int (*xProgress)(void *);     /* The progress callback */
  void *pProgressArg;           /* Argument to the progress callback */
  int nProgressOps;             /* Number of opcodes for progress callback */
#endif
#ifndef SQLITE_OMIT_VIRTUALTABLE
#ifndef DB_SQL
  Hash aModule;                 /* populated by sqlite3_create_module() */
#endif
  Table *pVTab;                 /* vtab with active Connect/Create method */
  sqlite3_vtab **aVTrans;       /* Virtual tables with open transactions */
  int nVTrans;                  /* Allocated size of aVTrans */
#endif
#ifndef DB_SQL
  Hash aFunc;                   /* All functions that can be in SQL exprs */
  Hash aCollSeq;                /* All collating sequences */
#endif
  BusyHandler busyHandler;      /* Busy callback */
  int busyTimeout;              /* Busy handler timeout, in msec */
  Db aDbStatic[2];              /* Static space for the 2 default backends */
#ifdef SQLITE_SSE
  sqlite3_stmt *pFetch;         /* Used by SSE to fetch stored statements */
#endif
};

/*
** A macro to discover the encoding of a database.
*/
#define ENC(db) ((db)->aDb[0].pSchema->enc)

/*
** Possible values for the sqlite.flags and or Db.flags fields.
**
** On sqlite.flags, the SQLITE_InTrans value means that we have
** executed a BEGIN.  On Db.flags, SQLITE_InTrans means a statement
** transaction is active on that particular database file.
*/
#define SQLITE_VdbeTrace      0x00000001  /* True to trace VDBE execution */
#define SQLITE_InTrans        0x00000008  /* True if in a transaction */
#define SQLITE_InternChanges  0x00000010  /* Uncommitted Hash table changes */
#define SQLITE_FullColNames   0x00000020  /* Show full column names on SELECT */
#define SQLITE_ShortColNames  0x00000040  /* Show short columns names */
#define SQLITE_CountRows      0x00000080  /* Count rows changed by INSERT, */
                                          /*   DELETE, or UPDATE and return */
                                          /*   the count using a callback. */
#define SQLITE_NullCallback   0x00000100  /* Invoke the callback once if the */
                                          /*   result set is empty */
#define SQLITE_SqlTrace       0x00000200  /* Debug print SQL as it executes */
#define SQLITE_VdbeListing    0x00000400  /* Debug listings of VDBE programs */
#define SQLITE_WriteSchema    0x00000800  /* OK to update SQLITE_MASTER */
#define SQLITE_NoReadlock     0x00001000  /* Readlocks are omitted when 
                                          ** accessing read-only databases */
#define SQLITE_IgnoreChecks   0x00002000  /* Do not enforce check constraints */
#define SQLITE_ReadUncommitted 0x00004000 /* For shared-cache mode */
#define SQLITE_LegacyFileFmt  0x00008000  /* Create new databases in format 1 */
#define SQLITE_FullFSync      0x00010000  /* Use full fsync on the backend */
#define SQLITE_LoadExtension  0x00020000  /* Enable load_extension */

#define SQLITE_RecoveryMode   0x00040000  /* Ignore schema errors */
#define SQLITE_SharedCache    0x00080000  /* Cache sharing is enabled */
#define SQLITE_Vtab           0x00100000  /* There exists a virtual table */

/*
** Possible values for the sqlite.magic field.
** The numbers are obtained at random and have no special meaning, other
** than being distinct from one another.
*/
#define SQLITE_MAGIC_OPEN     0xa029a697  /* Database is open */
#define SQLITE_MAGIC_CLOSED   0x9f3c2d33  /* Database is closed */
#define SQLITE_MAGIC_SICK     0x4b771290  /* Error and awaiting close */
#define SQLITE_MAGIC_BUSY     0xf03b7906  /* Database currently in use */
#define SQLITE_MAGIC_ERROR    0xb5357930  /* An SQLITE_MISUSE error occurred */

/*
** Each SQL function is defined by an instance of the following
** structure.  A pointer to this structure is stored in the sqlite.aFunc
** hash table.  When multiple functions have the same name, the hash table
** points to a linked list of these structures.
*/
struct FuncDef {
  i16 nArg;            /* Number of arguments.  -1 means unlimited */
  u8 iPrefEnc;         /* Preferred text encoding (SQLITE_UTF8, 16LE, 16BE) */
  u8 needCollSeq;      /* True if sqlite3GetFuncCollSeq() might be called */
  u8 flags;            /* Some combination of SQLITE_FUNC_* */
  void *pUserData;     /* User data parameter */
  FuncDef *pNext;      /* Next function with same name */
  void (*xFunc)(sqlite3_context*,int,sqlite3_value**); /* Regular function */
  void (*xStep)(sqlite3_context*,int,sqlite3_value**); /* Aggregate step */
  void (*xFinalize)(sqlite3_context*);                /* Aggregate finializer */
  char zName[1];       /* SQL name of the function.  MUST BE LAST */
};

/*
** Each SQLite module (virtual table definition) is defined by an
** instance of the following structure, stored in the sqlite3.aModule
** hash table.
*/
struct Module {
  const sqlite3_module *pModule;       /* Callback pointers */
  const char *zName;                   /* Name passed to create_module() */
  void *pAux;                          /* pAux passed to create_module() */
  void (*xDestroy)(void *);            /* Module destructor function */
};

/*
** Possible values for FuncDef.flags
*/
#define SQLITE_FUNC_LIKE   0x01  /* Candidate for the LIKE optimization */
#define SQLITE_FUNC_CASE   0x02  /* Case-sensitive LIKE-type function */
#define SQLITE_FUNC_EPHEM  0x04  /* Ephermeral.  Delete with VDBE */

/*
** information about each column of an SQL table is held in an instance
** of this structure.
*/
struct Column {
  char *zName;     /* Name of this column */
  Expr *pDflt;     /* Default value of this column */
  char *zType;     /* Data type for this column */
  char *zColl;     /* Collating sequence.  If NULL, use the default */
  u8 notNull;      /* True if there is a NOT NULL constraint */
  u8 isPrimKey;    /* True if this column is part of the PRIMARY KEY */
  char affinity;   /* One of the SQLITE_AFF_... values */
#ifndef SQLITE_OMIT_VIRTUALTABLE
  u8 isHidden;     /* True if this column is 'hidden' */
#endif
};

/*
** A "Collating Sequence" is defined by an instance of the following
** structure. Conceptually, a collating sequence consists of a name and
** a comparison routine that defines the order of that sequence.
**
** There may two seperate implementations of the collation function, one
** that processes text in UTF-8 encoding (CollSeq.xCmp) and another that
** processes text encoded in UTF-16 (CollSeq.xCmp16), using the machine
** native byte order. When a collation sequence is invoked, SQLite selects
** the version that will require the least expensive encoding
** translations, if any.
**
** The CollSeq.pUser member variable is an extra parameter that passed in
** as the first argument to the UTF-8 comparison function, xCmp.
** CollSeq.pUser16 is the equivalent for the UTF-16 comparison function,
** xCmp16.
**
** If both CollSeq.xCmp and CollSeq.xCmp16 are NULL, it means that the
** collating sequence is undefined.  Indices built on an undefined
** collating sequence may not be read or written.
*/
struct CollSeq {
  char *zName;          /* Name of the collating sequence, UTF-8 encoded */
  u8 enc;               /* Text encoding handled by xCmp() */
  u8 type;              /* One of the SQLITE_COLL_... values below */
  void *pUser;          /* First argument to xCmp() */
  int (*xCmp)(void*,int, const void*, int, const void*);
  void (*xDel)(void*);  /* Destructor for pUser */
};

/*
** Allowed values of CollSeq flags:
*/
#define SQLITE_COLL_BINARY  1  /* The default memcmp() collating sequence */
#define SQLITE_COLL_NOCASE  2  /* The built-in NOCASE collating sequence */
#define SQLITE_COLL_REVERSE 3  /* The built-in REVERSE collating sequence */
#define SQLITE_COLL_USER    0  /* Any other user-defined collating sequence */

/*
** A sort order can be either ASC or DESC.
*/
#define SQLITE_SO_ASC       0  /* Sort in ascending order */
#define SQLITE_SO_DESC      1  /* Sort in ascending order */

/*
** Column affinity types.
**
** These used to have mnemonic name like 'i' for SQLITE_AFF_INTEGER and
** 't' for SQLITE_AFF_TEXT.  But we can save a little space and improve
** the speed a little by number the values consecutively.  
**
** But rather than start with 0 or 1, we begin with 'a'.  That way,
** when multiple affinity types are concatenated into a string and
** used as the P4 operand, they will be more readable.
**
** Note also that the numeric types are grouped together so that testing
** for a numeric type is a single comparison.
*/
#define SQLITE_AFF_TEXT     'a'
#define SQLITE_AFF_NONE     'b'
#define SQLITE_AFF_NUMERIC  'c'
#define SQLITE_AFF_INTEGER  'd'
#define SQLITE_AFF_REAL     'e'

#define sqlite3IsNumericAffinity(X)  ((X)>=SQLITE_AFF_NUMERIC)

/*
** The SQLITE_AFF_MASK values masks off the significant bits of an
** affinity value. 
*/
#define SQLITE_AFF_MASK     0x67

/*
** Additional bit values that can be ORed with an affinity without
** changing the affinity.
*/
#define SQLITE_JUMPIFNULL   0x08  /* jumps if either operand is NULL */
#define SQLITE_NULLEQUAL    0x10  /* compare NULLs equal */
#define SQLITE_STOREP2      0x80  /* Store result in reg[P2] rather than jump */

/*
** Each SQL table is represented in memory by an instance of the
** following structure.
**
** Table.zName is the name of the table.  The case of the original
** CREATE TABLE statement is stored, but case is not significant for
** comparisons.
**
** Table.nCol is the number of columns in this table.  Table.aCol is a
** pointer to an array of Column structures, one for each column.
**
** If the table has an INTEGER PRIMARY KEY, then Table.iPKey is the index of
** the column that is that key.   Otherwise Table.iPKey is negative.  Note
** that the datatype of the PRIMARY KEY must be INTEGER for this field to
** be set.  An INTEGER PRIMARY KEY is used as the rowid for each row of
** the table.  If a table has no INTEGER PRIMARY KEY, then a random rowid
** is generated for each row of the table.  Table.hasPrimKey is true if
** the table has any PRIMARY KEY, INTEGER or otherwise.
**
** Table.tnum is the page number for the root BTree page of the table in the
** database file.  If Table.iDb is the index of the database table backend
** in sqlite.aDb[].  0 is for the main database and 1 is for the file that
** holds temporary tables and indices.  If Table.isEphem
** is true, then the table is stored in a file that is automatically deleted
** when the VDBE cursor to the table is closed.  In this case Table.tnum 
** refers VDBE cursor number that holds the table open, not to the root
** page number.  Transient tables are used to hold the results of a
** sub-query that appears instead of a real table name in the FROM clause 
** of a SELECT statement.
*/
struct Table {
  char *zName;     /* Name of the table */
  int nCol;        /* Number of columns in this table */
  Column *aCol;    /* Information about each column */
  int iPKey;       /* If not less then 0, use aCol[iPKey] as the primary key */
  Index *pIndex;   /* List of SQL indexes on this table. */
  int tnum;        /* Root BTree node for this table (see note above) */
  Select *pSelect; /* NULL for tables.  Points to definition if a view. */
  int nRef;          /* Number of pointers to this Table */
  Trigger *pTrigger; /* List of SQL triggers on this table */
  FKey *pFKey;       /* Linked list of all foreign keys in this table */
  char *zColAff;     /* String defining the affinity of each column */
#ifndef SQLITE_OMIT_CHECK
  Expr *pCheck;      /* The AND of all CHECK constraints */
#endif
#ifndef SQLITE_OMIT_ALTERTABLE
  int addColOffset;  /* Offset in CREATE TABLE statement to add a new column */
#endif
  u8 readOnly;     /* True if this table should not be written by the user */
  u8 isEphem;      /* True if created using OP_OpenEphermeral */
  u8 hasPrimKey;   /* True if there exists a primary key */
  u8 keyConf;      /* What to do in case of uniqueness conflict on iPKey */
  u8 autoInc;      /* True if the integer primary key is autoincrement */
#ifndef SQLITE_OMIT_VIRTUALTABLE
  u8 isVirtual;             /* True if this is a virtual table */
  u8 isCommit;              /* True once the CREATE TABLE has been committed */
  Module *pMod;             /* Pointer to the implementation of the module */
  sqlite3_vtab *pVtab;      /* Pointer to the module instance */
  int nModuleArg;           /* Number of arguments to the module */
  char **azModuleArg;       /* Text of all module args. [0] is module name */
#endif
  Schema *pSchema;          /* Schema that contains this table */
};

/*
** Test to see whether or not a table is a virtual table.  This is
** done as a macro so that it will be optimized out when virtual
** table support is omitted from the build.
*/
#ifndef SQLITE_OMIT_VIRTUALTABLE
#  define IsVirtual(X)      ((X)->isVirtual)
#  define IsHiddenColumn(X) ((X)->isHidden)
#else
#  define IsVirtual(X)      0
#  define IsHiddenColumn(X) 0
#endif

/*
** Each foreign key constraint is an instance of the following structure.
**
** A foreign key is associated with two tables.  The "from" table is
** the table that contains the REFERENCES clause that creates the foreign
** key.  The "to" table is the table that is named in the REFERENCES clause.
** Consider this example:
**
**     CREATE TABLE ex1(
**       a INTEGER PRIMARY KEY,
**       b INTEGER CONSTRAINT fk1 REFERENCES ex2(x)
**     );
**
** For foreign key "fk1", the from-table is "ex1" and the to-table is "ex2".
**
** Each REFERENCES clause generates an instance of the following structure
** which is attached to the from-table.  The to-table need not exist when
** the from-table is created.  The existance of the to-table is not checked
** until an attempt is made to insert data into the from-table.
**
** The sqlite.aFKey hash table stores pointers to this structure
** given the name of a to-table.  For each to-table, all foreign keys
** associated with that table are on a linked list using the FKey.pNextTo
** field.
*/
struct FKey {
  Table *pFrom;     /* The table that constains the REFERENCES clause */
  FKey *pNextFrom;  /* Next foreign key in pFrom */
  char *zTo;        /* Name of table that the key points to */
  FKey *pNextTo;    /* Next foreign key that points to zTo */
  int nCol;         /* Number of columns in this key */
  struct sColMap {  /* Mapping of columns in pFrom to columns in zTo */
    int iFrom;         /* Index of column in pFrom */
    char *zCol;        /* Name of column in zTo.  If 0 use PRIMARY KEY */
  } *aCol;          /* One entry for each of nCol column s */
  u8 isDeferred;    /* True if constraint checking is deferred till COMMIT */
  u8 updateConf;    /* How to resolve conflicts that occur on UPDATE */
  u8 deleteConf;    /* How to resolve conflicts that occur on DELETE */
  u8 insertConf;    /* How to resolve conflicts that occur on INSERT */
};

/*
** SQLite supports many different ways to resolve a constraint
** error.  ROLLBACK processing means that a constraint violation
** causes the operation in process to fail and for the current transaction
** to be rolled back.  ABORT processing means the operation in process
** fails and any prior changes from that one operation are backed out,
** but the transaction is not rolled back.  FAIL processing means that
** the operation in progress stops and returns an error code.  But prior
** changes due to the same operation are not backed out and no rollback
** occurs.  IGNORE means that the particular row that caused the constraint
** error is not inserted or updated.  Processing continues and no error
** is returned.  REPLACE means that preexisting database rows that caused
** a UNIQUE constraint violation are removed so that the new insert or
** update can proceed.  Processing continues and no error is reported.
**
** RESTRICT, SETNULL, and CASCADE actions apply only to foreign keys.
** RESTRICT is the same as ABORT for IMMEDIATE foreign keys and the
** same as ROLLBACK for DEFERRED keys.  SETNULL means that the foreign
** key is set to NULL.  CASCADE means that a DELETE or UPDATE of the
** referenced table row is propagated into the row that holds the
** foreign key.
** 
** The following symbolic values are used to record which type
** of action to take.
*/
#define OE_None     0   /* There is no constraint to check */
#define OE_Rollback 1   /* Fail the operation and rollback the transaction */
#define OE_Abort    2   /* Back out changes but do no rollback transaction */
#define OE_Fail     3   /* Stop the operation but leave all prior changes */
#define OE_Ignore   4   /* Ignore the error. Do not do the INSERT or UPDATE */
#define OE_Replace  5   /* Delete existing record, then do INSERT or UPDATE */

#define OE_Restrict 6   /* OE_Abort for IMMEDIATE, OE_Rollback for DEFERRED */
#define OE_SetNull  7   /* Set the foreign key value to NULL */
#define OE_SetDflt  8   /* Set the foreign key value to its default */
#define OE_Cascade  9   /* Cascade the changes */

#define OE_Default  99  /* Do whatever the default action is */


/*
** An instance of the following structure is passed as the first
** argument to sqlite3VdbeKeyCompare and is used to control the 
** comparison of the two index keys.
**
** If the KeyInfo.incrKey value is true and the comparison would
** otherwise be equal, then return a result as if the second key
** were larger.
*/
struct KeyInfo {
  sqlite3 *db;        /* The database connection */
  u8 enc;             /* Text encoding - one of the TEXT_Utf* values */
  u8 incrKey;         /* Increase 2nd key by epsilon before comparison */
  u8 prefixIsEqual;   /* Treat a prefix as equal */
  int nField;         /* Number of entries in aColl[] */
  u8 *aSortOrder;     /* If defined an aSortOrder[i] is true, sort DESC */
  CollSeq *aColl[1];  /* Collating sequence for each term of the key */
};

/*
** Each SQL index is represented in memory by an
** instance of the following structure.
**
** The columns of the table that are to be indexed are described
** by the aiColumn[] field of this structure.  For example, suppose
** we have the following table and index:
**
**     CREATE TABLE Ex1(c1 int, c2 int, c3 text);
**     CREATE INDEX Ex2 ON Ex1(c3,c1);
**
** In the Table structure describing Ex1, nCol==3 because there are
** three columns in the table.  In the Index structure describing
** Ex2, nColumn==2 since 2 of the 3 columns of Ex1 are indexed.
** The value of aiColumn is {2, 0}.  aiColumn[0]==2 because the 
** first column to be indexed (c3) has an index of 2 in Ex1.aCol[].
** The second column to be indexed (c1) has an index of 0 in
** Ex1.aCol[], hence Ex2.aiColumn[1]==0.
**
** The Index.onError field determines whether or not the indexed columns
** must be unique and what to do if they are not.  When Index.onError=OE_None,
** it means this is not a unique index.  Otherwise it is a unique index
** and the value of Index.onError indicate the which conflict resolution 
** algorithm to employ whenever an attempt is made to insert a non-unique
** element.
*/
struct Index {
  char *zName;     /* Name of this index */
  int nColumn;     /* Number of columns in the table used by this index */
  int *aiColumn;   /* Which columns are used by this index.  1st is 0 */
  unsigned *aiRowEst; /* Result of ANALYZE: Est. rows selected by each column */
  Table *pTable;   /* The SQL table being indexed */
  int tnum;        /* Page containing root of this index in database file */
  u8 onError;      /* OE_Abort, OE_Ignore, OE_Replace, or OE_None */
  u8 autoIndex;    /* True if is automatically created (ex: by UNIQUE) */
  char *zColAff;   /* String defining the affinity of each column */
  Index *pNext;    /* The next index associated with the same table */
  Schema *pSchema; /* Schema containing this index */
  u8 *aSortOrder;  /* Array of size Index.nColumn. True==DESC, False==ASC */
  char **azColl;   /* Array of collation sequence names for index */
};

/*
** Each token coming out of the lexer is an instance of
** this structure.  Tokens are also used as part of an expression.
**
** Note if Token.z==0 then Token.dyn and Token.n are undefined and
** may contain random values.  Do not make any assuptions about Token.dyn
** and Token.n when Token.z==0.
*/
struct Token {
  const unsigned char *z; /* Text of the token.  Not NULL-terminated! */
  unsigned dyn  : 1;      /* True for malloced memory, false for static */
  unsigned n    : 31;     /* Number of characters in this token */
};

/*
** An instance of this structure contains information needed to generate
** code for a SELECT that contains aggregate functions.
**
** If Expr.op==TK_AGG_COLUMN or TK_AGG_FUNCTION then Expr.pAggInfo is a
** pointer to this structure.  The Expr.iColumn field is the index in
** AggInfo.aCol[] or AggInfo.aFunc[] of information needed to generate
** code for that node.
**
** AggInfo.pGroupBy and AggInfo.aFunc.pExpr point to fields within the
** original Select structure that describes the SELECT statement.  These
** fields do not need to be freed when deallocating the AggInfo structure.
*/
struct AggInfo {
  u8 directMode;          /* Direct rendering mode means take data directly
                          ** from source tables rather than from accumulators */
  u8 useSortingIdx;       /* In direct mode, reference the sorting index rather
                          ** than the source table */
  int sortingIdx;         /* Cursor number of the sorting index */
  ExprList *pGroupBy;     /* The group by clause */
  int nSortingColumn;     /* Number of columns in the sorting index */
  struct AggInfo_col {    /* For each column used in source tables */
    Table *pTab;             /* Source table */
    int iTable;              /* Cursor number of the source table */
    int iColumn;             /* Column number within the source table */
    int iSorterColumn;       /* Column number in the sorting index */
    int iMem;                /* Memory location that acts as accumulator */
    Expr *pExpr;             /* The original expression */
  } *aCol;
  int nColumn;            /* Number of used entries in aCol[] */
  int nColumnAlloc;       /* Number of slots allocated for aCol[] */
  int nAccumulator;       /* Number of columns that show through to the output.
                          ** Additional columns are used only as parameters to
                          ** aggregate functions */
  struct AggInfo_func {   /* For each aggregate function */
    Expr *pExpr;             /* Expression encoding the function */
    FuncDef *pFunc;          /* The aggregate function implementation */
    int iMem;                /* Memory location that acts as accumulator */
    int iDistinct;           /* Ephermeral table used to enforce DISTINCT */
  } *aFunc;
  int nFunc;              /* Number of entries in aFunc[] */
  int nFuncAlloc;         /* Number of slots allocated for aFunc[] */
};

/*
** Each node of an expression in the parse tree is an instance
** of this structure.
**
** Expr.op is the opcode.  The integer parser token codes are reused
** as opcodes here.  For example, the parser defines TK_GE to be an integer
** code representing the ">=" operator.  This same integer code is reused
** to represent the greater-than-or-equal-to operator in the expression
** tree.
**
** Expr.pRight and Expr.pLeft are subexpressions.  Expr.pList is a list
** of argument if the expression is a function.
**
** Expr.token is the operator token for this node.  For some expressions
** that have subexpressions, Expr.token can be the complete text that gave
** rise to the Expr.  In the latter case, the token is marked as being
** a compound token.
**
** An expression of the form ID or ID.ID refers to a column in a table.
** For such expressions, Expr.op is set to TK_COLUMN and Expr.iTable is
** the integer cursor number of a VDBE cursor pointing to that table and
** Expr.iColumn is the column number for the specific column.  If the
** expression is used as a result in an aggregate SELECT, then the
** value is also stored in the Expr.iAgg column in the aggregate so that
** it can be accessed after all aggregates are computed.
**
** If the expression is a function, the Expr.iTable is an integer code
** representing which function.  If the expression is an unbound variable
** marker (a question mark character '?' in the original SQL) then the
** Expr.iTable holds the index number for that variable.
**
** If the expression is a subquery then Expr.iColumn holds an integer
** register number containing the result of the subquery.  If the
** subquery gives a constant result, then iTable is -1.  If the subquery
** gives a different answer at different times during statement processing
** then iTable is the address of a subroutine that computes the subquery.
**
** The Expr.pSelect field points to a SELECT statement.  The SELECT might
** be the right operand of an IN operator.  Or, if a scalar SELECT appears
** in an expression the opcode is TK_SELECT and Expr.pSelect is the only
** operand.
**
** If the Expr is of type OP_Column, and the table it is selecting from
** is a disk table or the "old.*" pseudo-table, then pTab points to the
** corresponding table definition.
*/
struct Expr {
  u8 op;                 /* Operation performed by this node */
  char affinity;         /* The affinity of the column or 0 if not a column */
  u16 flags;             /* Various flags.  See below */
  CollSeq *pColl;        /* The collation type of the column or 0 */
  Expr *pLeft, *pRight;  /* Left and right subnodes */
  ExprList *pList;       /* A list of expressions used as function arguments
                         ** or in "<expr> IN (<expr-list)" */
  Token token;           /* An operand token */
  Token span;            /* Complete text of the expression */
  int iTable, iColumn;   /* When op==TK_COLUMN, then this expr node means the
                         ** iColumn-th field of the iTable-th table. */
  AggInfo *pAggInfo;     /* Used by TK_AGG_COLUMN and TK_AGG_FUNCTION */
  int iAgg;              /* Which entry in pAggInfo->aCol[] or ->aFunc[] */
  int iRightJoinTable;   /* If EP_FromJoin, the right table of the join */
  Select *pSelect;       /* When the expression is a sub-select.  Also the
                         ** right side of "<expr> IN (<select>)" */
  Table *pTab;           /* Table for OP_Column expressions. */
/*  Schema *pSchema; */
#if defined(SQLITE_TEST) || SQLITE_MAX_EXPR_DEPTH>0
  int nHeight;           /* Height of the tree headed by this node */
#endif
};

/*
** The following are the meanings of bits in the Expr.flags field.
*/
#define EP_FromJoin   0x0001  /* Originated in ON or USING clause of a join */
#define EP_Agg        0x0002  /* Contains one or more aggregate functions */
#define EP_Resolved   0x0004  /* IDs have been resolved to COLUMNs */
#define EP_Error      0x0008  /* Expression contains one or more errors */
#define EP_Distinct   0x0010  /* Aggregate function with DISTINCT keyword */
#define EP_VarSelect  0x0020  /* pSelect is correlated, not constant */
#define EP_Dequoted   0x0040  /* True if the string has been dequoted */
#define EP_InfixFunc  0x0080  /* True for an infix function: LIKE, GLOB, etc */
#define EP_ExpCollate 0x0100  /* Collating sequence specified explicitly */
#define EP_AnyAff     0x0200  /* Can take a cached column of any affinity */
#define EP_FixedDest  0x0400  /* Result needed in a specific register */

/*
** These macros can be used to test, set, or clear bits in the 
** Expr.flags field.
*/
#define ExprHasProperty(E,P)     (((E)->flags&(P))==(P))
#define ExprHasAnyProperty(E,P)  (((E)->flags&(P))!=0)
#define ExprSetProperty(E,P)     (E)->flags|=(P)
#define ExprClearProperty(E,P)   (E)->flags&=~(P)

/*
** A list of expressions.  Each expression may optionally have a
** name.  An expr/name combination can be used in several ways, such
** as the list of "expr AS ID" fields following a "SELECT" or in the
** list of "ID = expr" items in an UPDATE.  A list of expressions can
** also be used as the argument to a function, in which case the a.zName
** field is not used.
*/
struct ExprList {
  int nExpr;             /* Number of expressions on the list */
  int nAlloc;            /* Number of entries allocated below */
  int iECursor;          /* VDBE Cursor associated with this ExprList */
  struct ExprList_item {
    Expr *pExpr;           /* The list of expressions */
    char *zName;           /* Token associated with this expression */
    u8 sortOrder;          /* 1 for DESC or 0 for ASC */
    u8 isAgg;              /* True if this is an aggregate like count(*) */
    u8 done;               /* A flag to indicate when processing is finished */
  } *a;                  /* One entry for each expression */
};

/*
** An instance of this structure can hold a simple list of identifiers,
** such as the list "a,b,c" in the following statements:
**
**      INSERT INTO t(a,b,c) VALUES ...;
**      CREATE INDEX idx ON t(a,b,c);
**      CREATE TRIGGER trig BEFORE UPDATE ON t(a,b,c) ...;
**
** The IdList.a.idx field is used when the IdList represents the list of
** column names after a table name in an INSERT statement.  In the statement
**
**     INSERT INTO t(a,b,c) ...
**
** If "a" is the k-th column of table "t", then IdList.a[0].idx==k.
*/
struct IdList {
  struct IdList_item {
    char *zName;      /* Name of the identifier */
    int idx;          /* Index in some Table.aCol[] of a column named zName */
  } *a;
  int nId;         /* Number of identifiers on the list */
  int nAlloc;      /* Number of entries allocated for a[] below */
};

/*
** The bitmask datatype defined below is used for various optimizations.
**
** Changing this from a 64-bit to a 32-bit type limits the number of
** tables in a join to 32 instead of 64.  But it also reduces the size
** of the library by 738 bytes on ix86.
*/
typedef u64 Bitmask;

/*
** The following structure describes the FROM clause of a SELECT statement.
** Each table or subquery in the FROM clause is a separate element of
** the SrcList.a[] array.
**
** With the addition of multiple database support, the following structure
** can also be used to describe a particular table such as the table that
** is modified by an INSERT, DELETE, or UPDATE statement.  In standard SQL,
** such a table must be a simple name: ID.  But in SQLite, the table can
** now be identified by a database name, a dot, then the table name: ID.ID.
**
** The jointype starts out showing the join type between the current table
** and the next table on the list.  The parser builds the list this way.
** But sqlite3SrcListShiftJoinType() later shifts the jointypes so that each
** jointype expresses the join between the table and the previous table.
*/
struct SrcList {
  i16 nSrc;        /* Number of tables or subqueries in the FROM clause */
  i16 nAlloc;      /* Number of entries allocated in a[] below */
  struct SrcList_item {
    char *zDatabase;  /* Name of database holding this table */
    char *zName;      /* Name of the table */
    char *zAlias;     /* The "B" part of a "A AS B" phrase.  zName is the "A" */
    Table *pTab;      /* An SQL table corresponding to zName */
    Select *pSelect;  /* A SELECT statement used in place of a table name */
    u8 isPopulated;   /* Temporary table associated with SELECT is populated */
    u8 jointype;      /* Type of join between this able and the previous */
    int iCursor;      /* The VDBE cursor number used to access this table */
    Expr *pOn;        /* The ON clause of a join */
    IdList *pUsing;   /* The USING clause of a join */
    Bitmask colUsed;  /* Bit N (1<<N) set if column N or pTab is used */
  } a[1];             /* One entry for each identifier on the list */
};

/*
** Permitted values of the SrcList.a.jointype field
*/
#define JT_INNER     0x0001    /* Any kind of inner or cross join */
#define JT_CROSS     0x0002    /* Explicit use of the CROSS keyword */
#define JT_NATURAL   0x0004    /* True for a "natural" join */
#define JT_LEFT      0x0008    /* Left outer join */
#define JT_RIGHT     0x0010    /* Right outer join */
#define JT_OUTER     0x0020    /* The "OUTER" keyword is present */
#define JT_ERROR     0x0040    /* unknown or unsupported join type */

/*
** For each nested loop in a WHERE clause implementation, the WhereInfo
** structure contains a single instance of this structure.  This structure
** is intended to be private the the where.c module and should not be
** access or modified by other modules.
**
** The pIdxInfo and pBestIdx fields are used to help pick the best
** index on a virtual table.  The pIdxInfo pointer contains indexing
** information for the i-th table in the FROM clause before reordering.
** All the pIdxInfo pointers are freed by whereInfoFree() in where.c.
** The pBestIdx pointer is a copy of pIdxInfo for the i-th table after
** FROM clause ordering.  This is a little confusing so I will repeat
** it in different words.  WhereInfo.a[i].pIdxInfo is index information 
** for WhereInfo.pTabList.a[i].  WhereInfo.a[i].pBestInfo is the
** index information for the i-th loop of the join.  pBestInfo is always
** either NULL or a copy of some pIdxInfo.  So for cleanup it is 
** sufficient to free all of the pIdxInfo pointers.
** 
*/
struct WhereLevel {
  int iFrom;            /* Which entry in the FROM clause */
  int flags;            /* Flags associated with this level */
  int iMem;             /* First memory cell used by this level */
  int iLeftJoin;        /* Memory cell used to implement LEFT OUTER JOIN */
  Index *pIdx;          /* Index used.  NULL if no index */
  int iTabCur;          /* The VDBE cursor used to access the table */
  int iIdxCur;          /* The VDBE cursor used to acesss pIdx */
  int brk;              /* Jump here to break out of the loop */
  int nxt;              /* Jump here to start the next IN combination */
  int cont;             /* Jump here to continue with the next loop cycle */
  int top;              /* First instruction of interior of the loop */
  int op, p1, p2;       /* Opcode used to terminate the loop */
  int nEq;              /* Number of == or IN constraints on this loop */
  int nIn;              /* Number of IN operators constraining this loop */
  struct InLoop {
    int iCur;              /* The VDBE cursor used by this IN operator */
    int topAddr;           /* Top of the IN loop */
  } *aInLoop;           /* Information about each nested IN operator */
  sqlite3_index_info *pBestIdx;  /* Index information for this level */

  /* The following field is really not part of the current level.  But
  ** we need a place to cache index information for each table in the
  ** FROM clause and the WhereLevel structure is a convenient place.
  */
  sqlite3_index_info *pIdxInfo;  /* Index info for n-th source table */
};

/*
** Flags appropriate for the wflags parameter of sqlite3WhereBegin().
*/
#define WHERE_ORDERBY_NORMAL     0   /* No-op */
#define WHERE_ORDERBY_MIN        1   /* ORDER BY processing for min() func */
#define WHERE_ORDERBY_MAX        2   /* ORDER BY processing for max() func */
#define WHERE_ONEPASS_DESIRED    4   /* Want to do one-pass UPDATE/DELETE */

/*
** The WHERE clause processing routine has two halves.  The
** first part does the start of the WHERE loop and the second
** half does the tail of the WHERE loop.  An instance of
** this structure is returned by the first half and passed
** into the second half to give some continuity.
*/
struct WhereInfo {
  Parse *pParse;       /* Parsing and code generating context */
  u8 okOnePass;        /* Ok to use one-pass algorithm for UPDATE or DELETE */
  SrcList *pTabList;   /* List of tables in the join */
  int iTop;            /* The very beginning of the WHERE loop */
  int iContinue;       /* Jump here to continue with next record */
  int iBreak;          /* Jump here to break out of the loop */
  int nLevel;          /* Number of nested loop */
  sqlite3_index_info **apInfo;  /* Array of pointers to index info structures */
  WhereLevel a[1];     /* Information about each nest loop in the WHERE */
};

/*
** A NameContext defines a context in which to resolve table and column
** names.  The context consists of a list of tables (the pSrcList) field and
** a list of named expression (pEList).  The named expression list may
** be NULL.  The pSrc corresponds to the FROM clause of a SELECT or
** to the table being operated on by INSERT, UPDATE, or DELETE.  The
** pEList corresponds to the result set of a SELECT and is NULL for
** other statements.
**
** NameContexts can be nested.  When resolving names, the inner-most 
** context is searched first.  If no match is found, the next outer
** context is checked.  If there is still no match, the next context
** is checked.  This process continues until either a match is found
** or all contexts are check.  When a match is found, the nRef member of
** the context containing the match is incremented. 
**
** Each subquery gets a new NameContext.  The pNext field points to the
** NameContext in the parent query.  Thus the process of scanning the
** NameContext list corresponds to searching through successively outer
** subqueries looking for a match.
*/
struct NameContext {
  Parse *pParse;       /* The parser */
  SrcList *pSrcList;   /* One or more tables used to resolve names */
  ExprList *pEList;    /* Optional list of named expressions */
  int nRef;            /* Number of names resolved by this context */
  int nErr;            /* Number of errors encountered while resolving names */
  u8 allowAgg;         /* Aggregate functions allowed here */
  u8 hasAgg;           /* True if aggregates are seen */
  u8 isCheck;          /* True if resolving names in a CHECK constraint */
  int nDepth;          /* Depth of subquery recursion. 1 for no recursion */
  AggInfo *pAggInfo;   /* Information about aggregates at this level */
  NameContext *pNext;  /* Next outer name context.  NULL for outermost */
};

/*
** An instance of the following structure contains all information
** needed to generate code for a single SELECT statement.
**
** nLimit is set to -1 if there is no LIMIT clause.  nOffset is set to 0.
** If there is a LIMIT clause, the parser sets nLimit to the value of the
** limit and nOffset to the value of the offset (or 0 if there is not
** offset).  But later on, nLimit and nOffset become the memory locations
** in the VDBE that record the limit and offset counters.
**
** addrOpenEphm[] entries contain the address of OP_OpenEphemeral opcodes.
** These addresses must be stored so that we can go back and fill in
** the P4_KEYINFO and P2 parameters later.  Neither the KeyInfo nor
** the number of columns in P2 can be computed at the same time
** as the OP_OpenEphm instruction is coded because not
** enough information about the compound query is known at that point.
** The KeyInfo for addrOpenTran[0] and [1] contains collating sequences
** for the result set.  The KeyInfo for addrOpenTran[2] contains collating
** sequences for the ORDER BY clause.
*/
struct Select {
  ExprList *pEList;      /* The fields of the result */
  u8 op;                 /* One of: TK_UNION TK_ALL TK_INTERSECT TK_EXCEPT */
  u8 isDistinct;         /* True if the DISTINCT keyword is present */
  u8 isResolved;         /* True once sqlite3SelectResolve() has run. */
  u8 isAgg;              /* True if this is an aggregate query */
  u8 usesEphm;           /* True if uses an OpenEphemeral opcode */
  u8 disallowOrderBy;    /* Do not allow an ORDER BY to be attached if TRUE */
  char affinity;         /* MakeRecord with this affinity for SRT_Set */
  SrcList *pSrc;         /* The FROM clause */
  Expr *pWhere;          /* The WHERE clause */
  ExprList *pGroupBy;    /* The GROUP BY clause */
  Expr *pHaving;         /* The HAVING clause */
  ExprList *pOrderBy;    /* The ORDER BY clause */
  Select *pPrior;        /* Prior select in a compound select statement */
  Select *pNext;         /* Next select to the left in a compound */
  Select *pRightmost;    /* Right-most select in a compound select statement */
  Expr *pLimit;          /* LIMIT expression. NULL means not used. */
  Expr *pOffset;         /* OFFSET expression. NULL means not used. */
  int iLimit, iOffset;   /* Memory registers holding LIMIT & OFFSET counters */
  int addrOpenEphm[3];   /* OP_OpenEphem opcodes related to this select */
};

/*
** The results of a select can be distributed in several ways.
*/
#define SRT_Union        1  /* Store result as keys in an index */
#define SRT_Except       2  /* Remove result from a UNION index */
#define SRT_Exists       3  /* Store 1 if the result is not empty */
#define SRT_Discard      4  /* Do not save the results anywhere */

/* The ORDER BY clause is ignored for all of the above */
#define IgnorableOrderby(X) ((X->eDest)<=SRT_Discard)

#define SRT_Callback     5  /* Invoke a callback with each row of result */
#define SRT_Mem          6  /* Store result in a memory cell */
#define SRT_Set          7  /* Store non-null results as keys in an index */
#define SRT_Table        8  /* Store result as data with an automatic rowid */
#define SRT_EphemTab     9  /* Create transient tab and store like SRT_Table */
#define SRT_Subroutine  10  /* Call a subroutine to handle results */

/*
** A structure used to customize the behaviour of sqlite3Select(). See
** comments above sqlite3Select() for details.
*/
typedef struct SelectDest SelectDest;
struct SelectDest {
  u8 eDest;         /* How to dispose of the results */
  u8 affinity;      /* Affinity used when eDest==SRT_Set */
  int iParm;        /* A parameter used by the eDest disposal method */
  int iMem;         /* Base register where results are written */
  int nMem;         /* Number of registers allocated */
};

/*
** An SQL parser context.  A copy of this structure is passed through
** the parser and down into all the parser action routine in order to
** carry around information that is global to the entire parse.
**
** The structure is divided into two parts.  When the parser and code
** generate call themselves recursively, the first part of the structure
** is constant but the second part is reset at the beginning and end of
** each recursion.
**
** The nTableLock and aTableLock variables are only used if the shared-cache 
** feature is enabled (if sqlite3Tsd()->useSharedData is true). They are
** used to store the set of table-locks required by the statement being
** compiled. Function sqlite3TableLock() is used to add entries to the
** list.
*/
struct Parse {
  sqlite3 *db;         /* The main database structure */
  int rc;              /* Return code from execution */
  char *zErrMsg;       /* An error message */
#ifndef DB_SQL
  Vdbe *pVdbe;         /* An engine for executing database bytecode */
#endif
  u8 colNamesSet;      /* TRUE after OP_ColumnName has been issued to pVdbe */
  u8 nameClash;        /* A permanent table name clashes with temp table name */
  u8 checkSchema;      /* Causes schema cookie check after an error */
  u8 nested;           /* Number of nested calls to the parser/code generator */
  u8 parseError;       /* True after a parsing error.  Ticket #1794 */
  u8 nTempReg;         /* Number of temporary registers in aTempReg[] */
  u8 nTempInUse;       /* Number of aTempReg[] currently checked out */
  int aTempReg[8];     /* Holding area for temporary registers */
  int nRangeReg;       /* Size of the temporary register block */
  int iRangeReg;       /* First register in temporary register block */
  int nErr;            /* Number of errors seen */
  int nTab;            /* Number of previously allocated VDBE cursors */
  int nMem;            /* Number of memory cells used so far */
  int nSet;            /* Number of sets used so far */
  int ckBase;          /* Base register of data during check constraints */
  int disableColCache; /* True to disable adding to column cache */
  int nColCache;       /* Number of entries in the column cache */
  int iColCache;       /* Next entry of the cache to replace */
  struct yColCache {
    int iTable;           /* Table cursor number */
    int iColumn;          /* Table column number */
    char affChange;       /* True if this register has had an affinity change */
    int iReg;             /* Register holding value of this column */
  } aColCache[10];     /* One for each valid column cache entry */
  u32 writeMask;       /* Start a write transaction on these databases */
  u32 cookieMask;      /* Bitmask of schema verified databases */
  int cookieGoto;      /* Address of OP_Goto to cookie verifier subroutine */
  int cookieValue[SQLITE_MAX_ATTACHED+2];  /* Values of cookies to verify */
#ifndef SQLITE_OMIT_SHARED_CACHE
  int nTableLock;        /* Number of locks in aTableLock */
  TableLock *aTableLock; /* Required table locks for shared-cache mode */
#endif
  int regRowid;        /* Register holding rowid of CREATE TABLE entry */
  int regRoot;         /* Register holding root page number for new objects */

  /* Above is constant between recursions.  Below is reset before and after
  ** each recursion */

  int nVar;            /* Number of '?' variables seen in the SQL so far */
  int nVarExpr;        /* Number of used slots in apVarExpr[] */
  int nVarExprAlloc;   /* Number of allocated slots in apVarExpr[] */
  Expr **apVarExpr;    /* Pointers to :aaa and $aaaa wildcard expressions */
  u8 explain;          /* True if the EXPLAIN flag is found on the query */
  Token sErrToken;     /* The token at which the error occurred */
  Token sNameToken;    /* Token with unqualified schema object name */
  Token sLastToken;    /* The last token parsed */
  const char *zSql;    /* All SQL text */
  const char *zTail;   /* All SQL text past the last semicolon parsed */
  Table *pNewTable;    /* A table being constructed by CREATE TABLE */
  Trigger *pNewTrigger;     /* Trigger under construct by a CREATE TRIGGER */
  TriggerStack *trigStack;  /* Trigger actions being coded */
  const char *zAuthContext; /* The 6th parameter to db->xAuth callbacks */
#ifndef SQLITE_OMIT_VIRTUALTABLE
  Token sArg;                /* Complete text of a module argument */
  u8 declareVtab;            /* True if inside sqlite3_declare_vtab() */
  int nVtabLock;             /* Number of virtual tables to lock */
  Table **apVtabLock;        /* Pointer to virtual tables needing locking */
#endif
#if defined(SQLITE_TEST) || SQLITE_MAX_EXPR_DEPTH>0
  int nHeight;            /* Expression tree height of current sub-select */
#endif
};

#ifdef SQLITE_OMIT_VIRTUALTABLE
  #define IN_DECLARE_VTAB 0
#else
  #define IN_DECLARE_VTAB (pParse->declareVtab)
#endif

/*
** An instance of the following structure can be declared on a stack and used
** to save the Parse.zAuthContext value so that it can be restored later.
*/
struct AuthContext {
  const char *zAuthContext;   /* Put saved Parse.zAuthContext here */
  Parse *pParse;              /* The Parse structure */
};

/*
** Bitfield flags for P2 value in OP_Insert and OP_Delete
*/
#define OPFLAG_NCHANGE   1    /* Set to update db->nChange */
#define OPFLAG_LASTROWID 2    /* Set to update db->lastRowid */
#define OPFLAG_ISUPDATE  4    /* This OP_Insert is an sql UPDATE */
#define OPFLAG_APPEND    8    /* This is likely to be an append */

/*
 * Each trigger present in the database schema is stored as an instance of
 * struct Trigger. 
 *
 * Pointers to instances of struct Trigger are stored in two ways.
 * 1. In the "trigHash" hash table (part of the sqlite3* that represents the 
 *    database). This allows Trigger structures to be retrieved by name.
 * 2. All triggers associated with a single table form a linked list, using the
 *    pNext member of struct Trigger. A pointer to the first element of the
 *    linked list is stored as the "pTrigger" member of the associated
 *    struct Table.
 *
 * The "step_list" member points to the first element of a linked list
 * containing the SQL statements specified as the trigger program.
 */
struct Trigger {
  char *name;             /* The name of the trigger                        */
  char *table;            /* The table or view to which the trigger applies */
  u8 op;                  /* One of TK_DELETE, TK_UPDATE, TK_INSERT         */
  u8 tr_tm;               /* One of TRIGGER_BEFORE, TRIGGER_AFTER */
  Expr *pWhen;            /* The WHEN clause of the expresion (may be NULL) */
  IdList *pColumns;       /* If this is an UPDATE OF <column-list> trigger,
                             the <column-list> is stored here */
  Token nameToken;        /* Token containing zName. Use during parsing only */
  Schema *pSchema;        /* Schema containing the trigger */
  Schema *pTabSchema;     /* Schema containing the table */
  TriggerStep *step_list; /* Link list of trigger program steps             */
  Trigger *pNext;         /* Next trigger associated with the table */
};

/*
** A trigger is either a BEFORE or an AFTER trigger.  The following constants
** determine which. 
**
** If there are multiple triggers, you might of some BEFORE and some AFTER.
** In that cases, the constants below can be ORed together.
*/
#define TRIGGER_BEFORE  1
#define TRIGGER_AFTER   2

/*
 * An instance of struct TriggerStep is used to store a single SQL statement
 * that is a part of a trigger-program. 
 *
 * Instances of struct TriggerStep are stored in a singly linked list (linked
 * using the "pNext" member) referenced by the "step_list" member of the 
 * associated struct Trigger instance. The first element of the linked list is
 * the first step of the trigger-program.
 * 
 * The "op" member indicates whether this is a "DELETE", "INSERT", "UPDATE" or
 * "SELECT" statement. The meanings of the other members is determined by the 
 * value of "op" as follows:
 *
 * (op == TK_INSERT)
 * orconf    -> stores the ON CONFLICT algorithm
 * pSelect   -> If this is an INSERT INTO ... SELECT ... statement, then
 *              this stores a pointer to the SELECT statement. Otherwise NULL.
 * target    -> A token holding the name of the table to insert into.
 * pExprList -> If this is an INSERT INTO ... VALUES ... statement, then
 *              this stores values to be inserted. Otherwise NULL.
 * pIdList   -> If this is an INSERT INTO ... (<column-names>) VALUES ... 
 *              statement, then this stores the column-names to be
 *              inserted into.
 *
 * (op == TK_DELETE)
 * target    -> A token holding the name of the table to delete from.
 * pWhere    -> The WHERE clause of the DELETE statement if one is specified.
 *              Otherwise NULL.
 * 
 * (op == TK_UPDATE)
 * target    -> A token holding the name of the table to update rows of.
 * pWhere    -> The WHERE clause of the UPDATE statement if one is specified.
 *              Otherwise NULL.
 * pExprList -> A list of the columns to update and the expressions to update
 *              them to. See sqlite3Update() documentation of "pChanges"
 *              argument.
 * 
 */
struct TriggerStep {
  int op;              /* One of TK_DELETE, TK_UPDATE, TK_INSERT, TK_SELECT */
  int orconf;          /* OE_Rollback etc. */
  Trigger *pTrig;      /* The trigger that this step is a part of */

  Select *pSelect;     /* Valid for SELECT and sometimes 
                          INSERT steps (when pExprList == 0) */
  Token target;        /* Valid for DELETE, UPDATE, INSERT steps */
  Expr *pWhere;        /* Valid for DELETE, UPDATE steps */
  ExprList *pExprList; /* Valid for UPDATE statements and sometimes 
                           INSERT steps (when pSelect == 0)         */
  IdList *pIdList;     /* Valid for INSERT statements only */
  TriggerStep *pNext;  /* Next in the link-list */
  TriggerStep *pLast;  /* Last element in link-list. Valid for 1st elem only */
};

/*
 * An instance of struct TriggerStack stores information required during code
 * generation of a single trigger program. While the trigger program is being
 * coded, its associated TriggerStack instance is pointed to by the
 * "pTriggerStack" member of the Parse structure.
 *
 * The pTab member points to the table that triggers are being coded on. The 
 * newIdx member contains the index of the vdbe cursor that points at the temp
 * table that stores the new.* references. If new.* references are not valid
 * for the trigger being coded (for example an ON DELETE trigger), then newIdx
 * is set to -1. The oldIdx member is analogous to newIdx, for old.* references.
 *
 * The ON CONFLICT policy to be used for the trigger program steps is stored 
 * as the orconf member. If this is OE_Default, then the ON CONFLICT clause 
 * specified for individual triggers steps is used.
 *
 * struct TriggerStack has a "pNext" member, to allow linked lists to be
 * constructed. When coding nested triggers (triggers fired by other triggers)
 * each nested trigger stores its parent trigger's TriggerStack as the "pNext" 
 * pointer. Once the nested trigger has been coded, the pNext value is restored
 * to the pTriggerStack member of the Parse stucture and coding of the parent
 * trigger continues.
 *
 * Before a nested trigger is coded, the linked list pointed to by the 
 * pTriggerStack is scanned to ensure that the trigger is not about to be coded
 * recursively. If this condition is detected, the nested trigger is not coded.
 */
struct TriggerStack {
  Table *pTab;         /* Table that triggers are currently being coded on */
  int newIdx;          /* Index of vdbe cursor to "new" temp table */
  int oldIdx;          /* Index of vdbe cursor to "old" temp table */
  u32 newColMask;
  u32 oldColMask;
  int orconf;          /* Current orconf policy */
  int ignoreJump;      /* where to jump to for a RAISE(IGNORE) */
  Trigger *pTrigger;   /* The trigger currently being coded */
  TriggerStack *pNext; /* Next trigger down on the trigger stack */
};

/*
** The following structure contains information used by the sqliteFix...
** routines as they walk the parse tree to make database references
** explicit.  
*/
typedef struct DbFixer DbFixer;
struct DbFixer {
  Parse *pParse;      /* The parsing context.  Error messages written here */
  const char *zDb;    /* Make sure all objects are contained in this database */
  const char *zType;  /* Type of the container - used for error messages */
  const Token *pName; /* Name of the container - used for error messages */
};

/*
** An objected used to accumulate the text of a string where we
** do not necessarily know how big the string will be in the end.
*/
struct StrAccum {
  char *zBase;     /* A base allocation.  Not from malloc. */
  char *zText;     /* The string collected so far */
  int  nChar;      /* Length of the string so far */
  int  nAlloc;     /* Amount of space allocated in zText */
  int  mxAlloc;        /* Maximum allowed string length */
  u8   mallocFailed;   /* Becomes true if any memory allocation fails */
  u8   useMalloc;      /* True if zText is enlargable using realloc */
  u8   tooBig;         /* Becomes true if string size exceeds limits */
};

/*
** A pointer to this structure is used to communicate information
** from sqlite3Init and OP_ParseSchema into the sqlite3InitCallback.
*/
typedef struct {
  sqlite3 *db;        /* The database being initialized */
  int iDb;            /* 0 for main database.  1 for TEMP, 2.. for ATTACHed */
  char **pzErrMsg;    /* Error message stored here */
  int rc;             /* Result code stored here */
} InitData;

/*
** Assuming zIn points to the first byte of a UTF-8 character,
** advance zIn to point to the first byte of the next UTF-8 character.
*/
#define SQLITE_SKIP_UTF8(zIn) {                        \
  if( (*(zIn++))>=0xc0 ){                              \
    while( (*zIn & 0xc0)==0x80 ){ zIn++; }             \
  }                                                    \
}

/*
** The SQLITE_CORRUPT_BKPT macro can be either a constant (for production
** builds) or a function call (for debugging).  If it is a function call,
** it allows the operator to set a breakpoint at the spot where database
** corruption is first detected.
*/
#ifdef SQLITE_DEBUG
  int sqlite3Corrupt(void);
# define SQLITE_CORRUPT_BKPT sqlite3Corrupt()
# define DEBUGONLY(X)        X
#else
# define SQLITE_CORRUPT_BKPT SQLITE_CORRUPT
# define DEBUGONLY(X)
#endif

/*
** Internal function prototypes
*/
int sqlite3StrICmp(const char *, const char *);
int sqlite3StrNICmp(const char *, const char *, int);
int sqlite3IsNumber(const char*, int*, u8);

void *sqlite3MallocZero(unsigned);
void *sqlite3DbMallocZero(sqlite3*, unsigned);
void *sqlite3DbMallocRaw(sqlite3*, unsigned);
char *sqlite3StrDup(const char*);
char *sqlite3StrNDup(const char*, int);
char *sqlite3DbStrDup(sqlite3*,const char*);
char *sqlite3DbStrNDup(sqlite3*,const char*, int);
void *sqlite3DbReallocOrFree(sqlite3 *, void *, int);
void *sqlite3DbRealloc(sqlite3 *, void *, int);
int sqlite3MallocSize(void *);

int sqlite3IsNaN(double);

char *sqlite3MPrintf(sqlite3*,const char*, ...);
char *sqlite3VMPrintf(sqlite3*,const char*, va_list);
#if defined(SQLITE_TEST) || defined(SQLITE_DEBUG)
  void sqlite3DebugPrintf(const char*, ...);
#endif
#if defined(SQLITE_TEST)
  void *sqlite3TextToPtr(const char*);
#endif
void sqlite3SetString(char **, ...);
void sqlite3ErrorMsg(Parse*, const char*, ...);
void sqlite3ErrorClear(Parse*);
void sqlite3Dequote(char*);
void sqlite3DequoteExpr(sqlite3*, Expr*);
int sqlite3KeywordCode(const unsigned char*, int);
int sqlite3RunParser(Parse*, const char*, char **);
void sqlite3FinishCoding(Parse*);
int sqlite3GetTempReg(Parse*);
void sqlite3ReleaseTempReg(Parse*,int);
int sqlite3GetTempRange(Parse*,int);
void sqlite3ReleaseTempRange(Parse*,int,int);
Expr *sqlite3Expr(sqlite3*, int, Expr*, Expr*, const Token*);
Expr *sqlite3PExpr(Parse*, int, Expr*, Expr*, const Token*);
Expr *sqlite3RegisterExpr(Parse*,Token*);
Expr *sqlite3ExprAnd(sqlite3*,Expr*, Expr*);
void sqlite3ExprSpan(Expr*,Token*,Token*);
Expr *sqlite3ExprFunction(Parse*,ExprList*, Token*);
void sqlite3ExprAssignVarNumber(Parse*, Expr*);
void sqlite3ExprDelete(Expr*);
ExprList *sqlite3ExprListAppend(Parse*,ExprList*,Expr*,Token*);
void sqlite3ExprListDelete(ExprList*);
int sqlite3Init(sqlite3*, char**);
int sqlite3InitCallback(void*, int, char**, char**);
void sqlite3Pragma(Parse*,Token*,Token*,Token*,int);
void sqlite3ResetInternalSchema(sqlite3*, int);
void sqlite3BeginParse(Parse*,int);
void sqlite3CommitInternalChanges(sqlite3*);
Table *sqlite3ResultSetOfSelect(Parse*,char*,Select*);
void sqlite3OpenMasterTable(Parse *, int);
void sqlite3StartTable(Parse*,Token*,Token*,int,int,int,int);
void sqlite3AddColumn(Parse*,Token*);
void sqlite3AddNotNull(Parse*, int);
void sqlite3AddPrimaryKey(Parse*, ExprList*, int, int, int);
void sqlite3AddCheckConstraint(Parse*, Expr*);
void sqlite3AddColumnType(Parse*,Token*);
void sqlite3AddDefaultValue(Parse*,Expr*);
void sqlite3AddCollateType(Parse*, Token*);
void sqlite3EndTable(Parse*,Token*,Token*,Select*);

Bitvec *sqlite3BitvecCreate(u32);
int sqlite3BitvecTest(Bitvec*, u32);
int sqlite3BitvecSet(Bitvec*, u32);
void sqlite3BitvecClear(Bitvec*, u32);
void sqlite3BitvecDestroy(Bitvec*);
int sqlite3BitvecBuiltinTest(int,int*);

void sqlite3CreateView(Parse*,Token*,Token*,Token*,Select*,int,int);

#if !defined(SQLITE_OMIT_VIEW) || !defined(SQLITE_OMIT_VIRTUALTABLE)
  int sqlite3ViewGetColumnNames(Parse*,Table*);
#else
# define sqlite3ViewGetColumnNames(A,B) 0
#endif

void sqlite3DropTable(Parse*, SrcList*, int, int);
void sqlite3DeleteTable(Table*);
void sqlite3Insert(Parse*, SrcList*, ExprList*, Select*, IdList*, int);
void *sqlite3ArrayAllocate(sqlite3*,void*,int,int,int*,int*,int*);
IdList *sqlite3IdListAppend(sqlite3*, IdList*, Token*);
int sqlite3IdListIndex(IdList*,const char*);
SrcList *sqlite3SrcListAppend(sqlite3*, SrcList*, Token*, Token*);
SrcList *sqlite3SrcListAppendFromTerm(Parse*, SrcList*, Token*, Token*, Token*,
                                      Select*, Expr*, IdList*);
void sqlite3SrcListShiftJoinType(SrcList*);
void sqlite3SrcListAssignCursors(Parse*, SrcList*);
void sqlite3IdListDelete(IdList*);
void sqlite3SrcListDelete(SrcList*);
void sqlite3CreateIndex(Parse*,Token*,Token*,SrcList*,ExprList*,int,Token*,
                        Token*, int, int);
void sqlite3DropIndex(Parse*, SrcList*, int);
int sqlite3Select(Parse*, Select*, SelectDest*, Select*, int, int*, char *aff);
Select *sqlite3SelectNew(Parse*,ExprList*,SrcList*,Expr*,ExprList*,
                         Expr*,ExprList*,int,Expr*,Expr*);
void sqlite3SelectDelete(Select*);
Table *sqlite3SrcListLookup(Parse*, SrcList*);
int sqlite3IsReadOnly(Parse*, Table*, int);
void sqlite3OpenTable(Parse*, int iCur, int iDb, Table*, int);
void sqlite3DeleteFrom(Parse*, SrcList*, Expr*);
void sqlite3Update(Parse*, SrcList*, ExprList*, Expr*, int);
WhereInfo *sqlite3WhereBegin(Parse*, SrcList*, Expr*, ExprList**, u8);
void sqlite3WhereEnd(WhereInfo*);
int sqlite3ExprCodeGetColumn(Parse*, Table*, int, int, int, int);
void sqlite3ExprCodeMove(Parse*, int, int);
void sqlite3ExprClearColumnCache(Parse*, int);
void sqlite3ExprCacheAffinityChange(Parse*, int, int);
int sqlite3ExprWritableRegister(Parse*,int,int);
void sqlite3ExprHardCopy(Parse*,int,int);
int sqlite3ExprCode(Parse*, Expr*, int);
int sqlite3ExprCodeTemp(Parse*, Expr*, int*);
int sqlite3ExprCodeTarget(Parse*, Expr*, int);
int sqlite3ExprCodeAndCache(Parse*, Expr*, int);
void sqlite3ExprCodeConstants(Parse*, Expr*);
int sqlite3ExprCodeExprList(Parse*, ExprList*, int, int);
void sqlite3ExprIfTrue(Parse*, Expr*, int, int);
void sqlite3ExprIfFalse(Parse*, Expr*, int, int);
Table *sqlite3FindTable(sqlite3*,const char*, const char*);
Table *sqlite3LocateTable(Parse*,int isView,const char*, const char*);
Index *sqlite3FindIndex(sqlite3*,const char*, const char*);
void sqlite3UnlinkAndDeleteTable(sqlite3*,int,const char*);
void sqlite3UnlinkAndDeleteIndex(sqlite3*,int,const char*);
void sqlite3Vacuum(Parse*);
int sqlite3RunVacuum(char**, sqlite3*);
char *sqlite3NameFromToken(sqlite3*, Token*);
int sqlite3ExprCompare(Expr*, Expr*);
int sqlite3ExprResolveNames(NameContext *, Expr *);
void sqlite3ExprAnalyzeAggregates(NameContext*, Expr*);
void sqlite3ExprAnalyzeAggList(NameContext*,ExprList*);
#ifndef DB_SQL
Vdbe *sqlite3GetVdbe(Parse*);
#endif
Expr *sqlite3CreateIdExpr(Parse *, const char*);
void sqlite3PrngSaveState(void);
void sqlite3PrngRestoreState(void);
void sqlite3PrngResetState(void);
void sqlite3RollbackAll(sqlite3*);
void sqlite3CodeVerifySchema(Parse*, int);
void sqlite3BeginTransaction(Parse*, int);
void sqlite3CommitTransaction(Parse*);
void sqlite3RollbackTransaction(Parse*);
int sqlite3ExprIsConstant(Expr*);
int sqlite3ExprIsConstantNotJoin(Expr*);
int sqlite3ExprIsConstantOrFunction(Expr*);
int sqlite3ExprIsInteger(Expr*, int*);
int sqlite3IsRowid(const char*);
void sqlite3GenerateRowDelete(Parse*, Table*, int, int, int);
void sqlite3GenerateRowIndexDelete(Parse*, Table*, int, int*);
int sqlite3GenerateIndexKey(Parse*, Index*, int, int, int);
void sqlite3GenerateConstraintChecks(Parse*,Table*,int,int,
                                     int*,int,int,int,int);
void sqlite3CompleteInsertion(Parse*, Table*, int, int, int*,int,int,int,int);
int sqlite3OpenTableAndIndices(Parse*, Table*, int, int);
void sqlite3BeginWriteOperation(Parse*, int, int);
Expr *sqlite3ExprDup(sqlite3*,Expr*);
void sqlite3TokenCopy(sqlite3*,Token*, Token*);
ExprList *sqlite3ExprListDup(sqlite3*,ExprList*);
SrcList *sqlite3SrcListDup(sqlite3*,SrcList*);
IdList *sqlite3IdListDup(sqlite3*,IdList*);
Select *sqlite3SelectDup(sqlite3*,Select*);
FuncDef *sqlite3FindFunction(sqlite3*,const char*,int,int,u8,int);
void sqlite3RegisterBuiltinFunctions(sqlite3*);
void sqlite3RegisterDateTimeFunctions(sqlite3*);
#ifdef SQLITE_DEBUG
  int sqlite3SafetyOn(sqlite3*);
  int sqlite3SafetyOff(sqlite3*);
#else
# define sqlite3SafetyOn(A) 0
# define sqlite3SafetyOff(A) 0
#endif
int sqlite3SafetyCheckOk(sqlite3*);
int sqlite3SafetyCheckSickOrOk(sqlite3*);
void sqlite3ChangeCookie(Parse*, int);
void sqlite3MaterializeView(Parse*, Select*, Expr*, int);

#ifndef SQLITE_OMIT_TRIGGER
  void sqlite3BeginTrigger(Parse*, Token*,Token*,int,int,IdList*,SrcList*,
                           Expr*,int, int);
  void sqlite3FinishTrigger(Parse*, TriggerStep*, Token*);
  void sqlite3DropTrigger(Parse*, SrcList*, int);
  void sqlite3DropTriggerPtr(Parse*, Trigger*);
  int sqlite3TriggersExist(Parse*, Table*, int, ExprList*);
  int sqlite3CodeRowTrigger(Parse*, int, ExprList*, int, Table *, int, int, 
                           int, int, u32*, u32*);
  void sqliteViewTriggers(Parse*, Table*, Expr*, int, ExprList*);
  void sqlite3DeleteTriggerStep(TriggerStep*);
  TriggerStep *sqlite3TriggerSelectStep(sqlite3*,Select*);
  TriggerStep *sqlite3TriggerInsertStep(sqlite3*,Token*, IdList*,
                                        ExprList*,Select*,int);
  TriggerStep *sqlite3TriggerUpdateStep(sqlite3*,Token*,ExprList*, Expr*, int);
  TriggerStep *sqlite3TriggerDeleteStep(sqlite3*,Token*, Expr*);
  void sqlite3DeleteTrigger(Trigger*);
  void sqlite3UnlinkAndDeleteTrigger(sqlite3*,int,const char*);
#else
# define sqlite3TriggersExist(A,B,C,D,E,F) 0
# define sqlite3DeleteTrigger(A)
# define sqlite3DropTriggerPtr(A,B)
# define sqlite3UnlinkAndDeleteTrigger(A,B,C)
# define sqlite3CodeRowTrigger(A,B,C,D,E,F,G,H,I,J,K) 0
#endif

int sqlite3JoinType(Parse*, Token*, Token*, Token*);
void sqlite3CreateForeignKey(Parse*, ExprList*, Token*, ExprList*, int);
void sqlite3DeferForeignKey(Parse*, int);
#ifndef SQLITE_OMIT_AUTHORIZATION
  void sqlite3AuthRead(Parse*,Expr*,Schema*,SrcList*);
  int sqlite3AuthCheck(Parse*,int, const char*, const char*, const char*);
  void sqlite3AuthContextPush(Parse*, AuthContext*, const char*);
  void sqlite3AuthContextPop(AuthContext*);
#else
# define sqlite3AuthRead(a,b,c,d)
# define sqlite3AuthCheck(a,b,c,d,e)    SQLITE_OK
# define sqlite3AuthContextPush(a,b,c)
# define sqlite3AuthContextPop(a)  ((void)(a))
#endif
void sqlite3Attach(Parse*, Expr*, Expr*, Expr*);
void sqlite3Detach(Parse*, Expr*);
#ifndef DB_SQL
int sqlite3BtreeFactory(const sqlite3 *db, const char *zFilename,
                       int omitJournal, int nCache, int flags, Btree **ppBtree);
#endif
int sqlite3FixInit(DbFixer*, Parse*, int, const char*, const Token*);
int sqlite3FixSrcList(DbFixer*, SrcList*);
int sqlite3FixSelect(DbFixer*, Select*);
int sqlite3FixExpr(DbFixer*, Expr*);
int sqlite3FixExprList(DbFixer*, ExprList*);
int sqlite3FixTriggerStep(DbFixer*, TriggerStep*);
int sqlite3AtoF(const char *z, double*);
char *sqlite3_snprintf(int,char*,const char*,...);
int sqlite3GetInt32(const char *, int*);
int sqlite3FitsIn64Bits(const char *, int);
int sqlite3Utf16ByteLen(const void *pData, int nChar);
int sqlite3Utf8CharLen(const char *pData, int nByte);
int sqlite3Utf8Read(const u8*, const u8*, const u8**);

/*
** Routines to read and write variable-length integers.  These used to
** be defined locally, but now we use the varint routines in the util.c
** file.  Code should use the MACRO forms below, as the Varint32 versions
** are coded to assume the single byte case is already handled (which 
** the MACRO form does).
*/
int sqlite3PutVarint(unsigned char*, u64);
int sqlite3PutVarint32(unsigned char*, u32);
int sqlite3GetVarint(const unsigned char *, u64 *);
int sqlite3GetVarint32(const unsigned char *, u32 *);
int sqlite3VarintLen(u64 v);

/*
** The header of a record consists of a sequence variable-length integers.
** These integers are almost always small and are encoded as a single byte.
** The following macros take advantage this fact to provide a fast encode
** and decode of the integers in a record header.  It is faster for the common
** case where the integer is a single byte.  It is a little slower when the
** integer is two or more bytes.  But overall it is faster.
**
** The following expressions are equivalent:
**
**     x = sqlite3GetVarint32( A, &B );
**     x = sqlite3PutVarint32( A, B );
**
**     x = getVarint32( A, B );
**     x = putVarint32( A, B );
**
*/
#define getVarint32(A,B)  ((*(A)<(unsigned char)0x80) ? ((B) = (u32)*(A)),1 : sqlite3GetVarint32((A), &(B)))
#define putVarint32(A,B)  (((B)<(u32)0x80) ? (*(A) = (unsigned char)(B)),1 : sqlite3PutVarint32((A), (B)))
#define getVarint    sqlite3GetVarint
#define putVarint    sqlite3PutVarint


#ifndef DB_SQL
void sqlite3IndexAffinityStr(Vdbe *, Index *);
void sqlite3TableAffinityStr(Vdbe *, Table *);
#endif
char sqlite3CompareAffinity(Expr *pExpr, char aff2);
int sqlite3IndexAffinityOk(Expr *pExpr, char idx_affinity);
char sqlite3ExprAffinity(Expr *pExpr);
int sqlite3Atoi64(const char*, i64*);
void sqlite3Error(sqlite3*, int, const char*,...);
void *sqlite3HexToBlob(sqlite3*, const char *z, int n);
int sqlite3TwoPartName(Parse *, Token *, Token *, Token **);
const char *sqlite3ErrStr(int);
int sqlite3ReadSchema(Parse *pParse);
CollSeq *sqlite3FindCollSeq(sqlite3*,u8 enc, const char *,int,int);
CollSeq *sqlite3LocateCollSeq(Parse *pParse, const char *zName, int nName);
CollSeq *sqlite3ExprCollSeq(Parse *pParse, Expr *pExpr);
Expr *sqlite3ExprSetColl(Parse *pParse, Expr *, Token *);
int sqlite3CheckCollSeq(Parse *, CollSeq *);
int sqlite3CheckObjectName(Parse *, const char *);
void sqlite3VdbeSetChanges(sqlite3 *, int);

const void *sqlite3ValueText(sqlite3_value*, u8);
int sqlite3ValueBytes(sqlite3_value*, u8);
void sqlite3ValueSetStr(sqlite3_value*, int, const void *,u8, 
                        void(*)(void*));
void sqlite3ValueFree(sqlite3_value*);
sqlite3_value *sqlite3ValueNew(sqlite3 *);
char *sqlite3Utf16to8(sqlite3 *, const void*, int);
int sqlite3ValueFromExpr(sqlite3 *, Expr *, u8, u8, sqlite3_value **);
void sqlite3ValueApplyAffinity(sqlite3_value *, u8, u8);
#ifndef SQLITE_AMALGAMATION
extern const unsigned char sqlite3UpperToLower[];
#endif
void sqlite3RootPageMoved(Db*, int, int);
void sqlite3Reindex(Parse*, Token*, Token*);
void sqlite3AlterFunctions(sqlite3*);
void sqlite3AlterRenameTable(Parse*, SrcList*, Token*);
int sqlite3GetToken(const unsigned char *, int *);
void sqlite3NestedParse(Parse*, const char*, ...);
void sqlite3ExpirePreparedStatements(sqlite3*);
void sqlite3CodeSubselect(Parse *, Expr *);
int sqlite3SelectResolve(Parse *, Select *, NameContext *);
#ifndef DB_SQL
void sqlite3ColumnDefault(Vdbe *, Table *, int);
#endif
void sqlite3AlterFinishAddColumn(Parse *, Token *);
void sqlite3AlterBeginAddColumn(Parse *, SrcList *);
CollSeq *sqlite3GetCollSeq(sqlite3*, CollSeq *, const char *, int);
char sqlite3AffinityType(const Token*);
void sqlite3Analyze(Parse*, Token*, Token*);
int sqlite3InvokeBusyHandler(BusyHandler*);
int sqlite3FindDb(sqlite3*, Token*);
int sqlite3AnalysisLoad(sqlite3*,int iDB);
void sqlite3DefaultRowEst(Index*);
void sqlite3RegisterLikeFunctions(sqlite3*, int);
int sqlite3IsLikeFunction(sqlite3*,Expr*,int*,char*);
void sqlite3AttachFunctions(sqlite3 *);
void sqlite3MinimumFileFormat(Parse*, int, int);
void sqlite3SchemaFree(void *);
#ifndef DB_SQL
Schema *sqlite3SchemaGet(sqlite3 *, Btree *);
#endif
int sqlite3SchemaToIndex(sqlite3 *db, Schema *);
KeyInfo *sqlite3IndexKeyinfo(Parse *, Index *);
int sqlite3CreateFunc(sqlite3 *, const char *, int, int, void *, 
  void (*)(sqlite3_context*,int,sqlite3_value **),
  void (*)(sqlite3_context*,int,sqlite3_value **), void (*)(sqlite3_context*));
int sqlite3ApiExit(sqlite3 *db, int);
int sqlite3OpenTempDatabase(Parse *);

void sqlite3StrAccumAppend(StrAccum*,const char*,int);
char *sqlite3StrAccumFinish(StrAccum*);
void sqlite3StrAccumReset(StrAccum*);
void sqlite3SelectDestInit(SelectDest*,int,int);

/*
** The interface to the LEMON-generated parser
*/
void *sqlite3ParserAlloc(void*(*)(size_t));
void sqlite3ParserFree(void*, void(*)(void*));
void sqlite3Parser(void*, int, Token, Parse*);

int sqlite3AutoLoadExtensions(sqlite3*);
#ifndef SQLITE_OMIT_LOAD_EXTENSION
  void sqlite3CloseExtensions(sqlite3*);
#else
# define sqlite3CloseExtensions(X)
#endif

#ifndef SQLITE_OMIT_SHARED_CACHE
  void sqlite3TableLock(Parse *, int, int, u8, const char *);
#else
  #define sqlite3TableLock(v,w,x,y,z)
#endif

#ifdef SQLITE_TEST
  int sqlite3Utf8To8(unsigned char*);
#endif

#ifdef SQLITE_OMIT_VIRTUALTABLE
#  define sqlite3VtabClear(X)
#  define sqlite3VtabSync(X,Y) (Y)
#  define sqlite3VtabRollback(X)
#  define sqlite3VtabCommit(X)
#else
   void sqlite3VtabClear(Table*);
   int sqlite3VtabSync(sqlite3 *db, int rc);
   int sqlite3VtabRollback(sqlite3 *db);
   int sqlite3VtabCommit(sqlite3 *db);
#endif
void sqlite3VtabMakeWritable(Parse*,Table*);
void sqlite3VtabLock(sqlite3_vtab*);
void sqlite3VtabUnlock(sqlite3*, sqlite3_vtab*);
void sqlite3VtabBeginParse(Parse*, Token*, Token*, Token*);
void sqlite3VtabFinishParse(Parse*, Token*);
void sqlite3VtabArgInit(Parse*);
void sqlite3VtabArgExtend(Parse*, Token*);
int sqlite3VtabCallCreate(sqlite3*, int, const char *, char **);
int sqlite3VtabCallConnect(Parse*, Table*);
int sqlite3VtabCallDestroy(sqlite3*, int, const char *);
int sqlite3VtabBegin(sqlite3 *, sqlite3_vtab *);
FuncDef *sqlite3VtabOverloadFunction(sqlite3 *,FuncDef*, int nArg, Expr*);
void sqlite3InvalidFunction(sqlite3_context*,int,sqlite3_value**);
#ifndef DB_SQL
int sqlite3Reprepare(Vdbe*);
#endif
void sqlite3ExprListCheckLength(Parse*, ExprList*, const char*);
CollSeq *sqlite3BinaryCompareCollSeq(Parse *, Expr *, Expr *);


/*
** Available fault injectors.  Should be numbered beginning with 0.
*/
#define SQLITE_FAULTINJECTOR_MALLOC     0
#define SQLITE_FAULTINJECTOR_COUNT      1

/*
** The interface to the fault injector subsystem.  If the fault injector
** mechanism is disabled at compile-time then set up macros so that no
** unnecessary code is generated.
*/
#ifndef SQLITE_OMIT_BUILTIN_TEST
  void sqlite3FaultConfig(int,int,int);
  int sqlite3FaultFailures(int);
  int sqlite3FaultBenignFailures(int);
  int sqlite3FaultPending(int);
  void sqlite3FaultBeginBenign(int);
  void sqlite3FaultEndBenign(int);
  int sqlite3FaultStep(int);
#else
# define sqlite3FaultConfig(A,B,C)
# define sqlite3FaultFailures(A)         0
# define sqlite3FaultBenignFailures(A)   0
# define sqlite3FaultPending(A)          (-1)
# define sqlite3FaultBeginBenign(A)
# define sqlite3FaultEndBenign(A)
# define sqlite3FaultStep(A)             0
#endif
  
  

#define IN_INDEX_ROWID           1
#define IN_INDEX_EPH             2
#define IN_INDEX_INDEX           3
int sqlite3FindInIndex(Parse *, Expr *, int);

#ifdef SQLITE_ENABLE_ATOMIC_WRITE
  int sqlite3JournalOpen(sqlite3_vfs *, const char *, sqlite3_file *, int, int);
  int sqlite3JournalSize(sqlite3_vfs *);
  int sqlite3JournalCreate(sqlite3_file *);
#else
  #define sqlite3JournalSize(pVfs) ((pVfs)->szOsFile)
#endif

#if defined(SQLITE_TEST) || SQLITE_MAX_EXPR_DEPTH>0
  void sqlite3ExprSetHeight(Expr *);
  int sqlite3SelectExprHeight(Select *);
#else
  #define sqlite3ExprSetHeight(x)
#endif

u32 sqlite3Get4byte(const u8*);
void sqlite3Put4byte(u8*, u32);

#ifdef SQLITE_SSE
#include "sseInt.h"
#endif

#ifdef SQLITE_DEBUG
  void sqlite3ParserTrace(FILE*, char *);
#endif

/*
** If the SQLITE_ENABLE IOTRACE exists then the global variable
** sqlite3IoTrace is a pointer to a printf-like routine used to
** print I/O tracing messages. 
*/
#ifdef SQLITE_ENABLE_IOTRACE
# define IOTRACE(A)  if( sqlite3IoTrace ){ sqlite3IoTrace A; }
  void sqlite3VdbeIOTraceSql(Vdbe*);
SQLITE_EXTERN void (*sqlite3IoTrace)(const char*,...);
#else
# define IOTRACE(A)
# define sqlite3VdbeIOTraceSql(X)
#endif

#endif

#ifdef DB_SQL
#define sqlite3_malloc malloc
#define sqlite3_free free
#endif

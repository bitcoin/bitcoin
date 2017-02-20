/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

typedef void *AEEAppInfo;
typedef void *AEECLSID;
typedef void *IFileMgr;
typedef void *IShell;

typedef int FileSeekType;
typedef int OpenFileMode;
typedef int boolean;

typedef struct {
    IShell *m_pIShell;
} AEEApplet;

typedef struct {
    unsigned int     attrib;
    unsigned int     dwSize;
    char        *szName;
} FileInfo;

typedef struct {
    int wSecond;
    int wMinute;
    int wHour;
    int wDay;
    int wWeekDay;
    int wMonth;
    int wYear;
} JulianType;

#define AECHAR                  char
#define AEECLSID_FILEMGR            (1)
#define BREW_EPOCH_OFFSET           (1)
#define DBGPRINTF               printf
#define FILE_MANAGER_CREATE(a, b, c)        (b = (IFileMgr *)a, c = 0)
#define FILE_MANAGER_ERR(a, b, c, d, e)     (b = (IFileMgr *)a, e = 0)
#define FLOAT_TO_WSTR(a, b, c)          (a = c)
#define GETAPPINSTANCE()            (NULL)
#define GETJULIANDATE(a, b)
#define GETTIMESECONDS()            (0)
#define GETUTCSECONDS()             (0)
#define IFILEMGR_EnumInit(a, b, c)      (b = b, 1)
#define IFILEMGR_EnumNext(a, b)         (1)
#define IFILEMGR_GetInfo(a, b, c)       (1)
#define IFILEMGR_GetLastError(a)        (1)
#define IFILEMGR_MkDir(a, b)            (b = b, 1)
#define IFILEMGR_OpenFile(a, b, c)      (NULL)
#define IFILEMGR_Release(a)         (a = a)
#define IFILEMGR_Remove(a, b)           (b = b, 1)
#define IFILEMGR_Rename(a, b, c)        (a = a, b = b, c = c, 0)
#define IFILEMGR_ResolvePath(a, b, c, d)    (1)
#define IFILEMGR_Test(a, b)         (b = b, 1)
#define IFILE_GetInfo(a, b)         (a = a, 1)
#define IFILE_Read(a, b, c)         (a = a, 1)
#define IFILE_Release(a)            (a = a)
#define IFILE_Seek(a, b, c)         (1)
#define IFILE_Truncate(a, b)            (a = a, 1)
#define IFILE_Write(a, b, c)            (a = a, 1)
#define ISHELL_ActiveApplet(a)          (NULL)
#define ISHELL_CloseApplet(a, b)
#define ISHELL_CreateInstance(a, b, c)      (1)
#define ISHELL_QueryClass(a, b, c)      (*c = NULL, 1)
#define JULIANTOSECONDS(a)          (1)
#define LOCALTIMEOFFSET(a)          (1)
#define MEMCPY(a, b, c)             (NULL)
#define MSLEEP(a)
#define WSTR_TO_STR(a, b, c)            strncpy(b, a, c)
#define __os_fsync(a, b)            (0)

#define SUCCESS                 (0)
#define EFAILED                 (1)

#define _FA_DIR                 (1)
#define _OFM_APPEND             (1)
#define _OFM_CREATE             (1)
#define _OFM_READ               (1)
#define _OFM_READWRITE              (1)
#define _SEEK_CURRENT               (1)
#define _SEEK_END               (1)
#define _SEEK_START             (1)

#define EBADFILENAME                (1)
#define EBADSEEKPOS             (2)
#define EDIRNOEXISTS                (3)
#define EDIRNOTEMPTY                (4)
#define EFILEEOF                (5)
#define EFILEEXISTS             (6)
#define EFILENOEXISTS               (7)
#define EFILEOPEN               (8)
#define EFSFULL                 (9)
#define EINVALIDOPERATION           (10)
#define ENOMEDIA                (11)
#define ENOMEMORY               (12)
#define EOUTOFNODES             (13)

#define static

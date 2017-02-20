/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$ 
 */

#include "test.h"

char rand_str_dbt::buf[rand_str_dbt::BUFLEN];
bool rand_str_dbt::init = false;
//boost::mutex g_mtx_co;// global mutex to sync create/open
DbEnv *g_env = NULL;
int g_test_start_txn = 0;
int g_StopInsert = 0;

bool strlt0(const char *s1, const char *s2)
{
    return strcmp(s1, s2) < 0;
}

bool streq0(const char *s1, const char *s2)// for list::unique
{
    return strcmp(s1, s2) == 0;
}

bool strlt(const string&s1, const string&s2)// for list::merge and list::sort
{
    return strcmp(s1.c_str(), s2.c_str()) < 0;
}

bool streq(const string&s1, const string&s2)// for list::unique
{
    return strcmp(s1.c_str(), s2.c_str()) == 0;
}

bool operator==(ElementHolder<const char *>s1, const char *s2)
{
    return strcmp(s1, s2) == 0;
}

bool operator==(ElementHolder<const wchar_t *>s1, const wchar_t *s2)
{
    return wcscmp(s1, s2) == 0;
}

bool operator!=(ElementHolder<const char *>s1, const char *s2)
{
    return !operator==(s1, s2);
}

bool operator!=(ElementHolder<const wchar_t *>s1, const wchar_t *s2)
{
    return !operator==(s1, s2);
}

size_t g_sum(size_t s, size_t e) 
{
    size_t sum = 0;

    for (size_t i = s; i <= e; i++)
        sum += g_count[i];
    return sum;
}

bool is_odd(ptint s)
{
    return ( s % 2) != 0;
}

ptint addup(const ptint&i, const ptint&j)
{
    return i + j;
}

ptint randint()
{
    return -999;
}

int randpos(int p)
{
    return abs(rand()) % p;
}

bool is2digits(ptint  i)
{
       return (i >  (9)) && (i <  (100));
}

int get_dest_secdb_callback(Db * /* secondary */, const Dbt * /* key */, 
    const Dbt *data, Dbt *result)
{
    SMSMsg *p = (SMSMsg*)data->get_data();

    result->set_data(&(p->to));
    result->set_size(sizeof(p->to));
    return 0;
}

void usage()
{
    cout<<
        "\nusage: test_dbstl \n\
[-M         run multi-thread test only]\n\
[-T number      total number of insert in multithread test]\n\
[-c number      cache size]\n\
[-h         print this help message then exit]\n\
[-k number      shortest string inserted]\n\
[-l number      longest string inserted]\n\
[-m ds/cds/tds      use ds/cds/tds product]\n\
[-r number      number of reader threads in multithread test]\n\
[-s b/h         use btree/hash type of DB for assocative \
containers] \n\
[-t a/e         for tds, use autocommit/explicit transaction in \
the test] \n\
[-v         verbose mode, output more info in multithread \
test]\n\
[-w number      number of writer threads in multithread test]\n\
";
}

void using_charstr(TCHAR*str)
{
    cout<<_T("using str read only with non-const parameter type:")<<str;
    str[0] = _T('U');
    str[1] = _T('R');
}

void copy_array(TCHAR**arr, TCHAR***dest)
{
    int i;

    *dest = new TCHAR*[32];
    for (i = 0; arr[i] != NULL; i++) {
        (*dest)[i] = new TCHAR[_tcslen(arr[i])];
        _tcscpy((*dest)[i], arr[i]);
    }
    
}

void SMSMsgRestore(SMSMsg2& dest, const void *srcdata)
{
    char *p = dest.msg;

    memcpy(&dest, srcdata, sizeof(dest));

    dest.msg = (char *)DbstlReAlloc(p, dest.szmsg);
    strcpy(dest.msg, (char*)srcdata + sizeof(dest));
}

u_int32_t SMSMsgSize(const SMSMsg2& elem)
{
    return (u_int32_t)(sizeof(elem) + strlen(elem.msg) + 1);
}

void SMSMsgCopy(void *dest, const SMSMsg2&elem)
{
    memcpy(dest, &elem, sizeof(elem));
    strcpy((char*)dest + sizeof(elem), elem.msg);
}

bool ptintless(const ptint& a, const ptint& b)
{
    return a < b;
}

u_int32_t rgblen(const RGBB *seq)
{
    size_t s = 0;
    
    const RGBB *p = seq, rgb0;
    for (s = 0, p = seq; memcmp(p, &rgb0, sizeof(rgb0)) != 0; p++, s++);
    // this size includes the all-0 last element used like '\0' 
    // for char* strings 
    return (u_int32_t)(s + 1);
        
}

// the seqs sequence of RGBB objects may not reside in a consecutive chunk of mem
// but the seqd points to a consecutive chunk of mem large enough to hold all objects 
// from seqs
void rgbcpy(RGBB *seqd, const RGBB *seqs, size_t num)
{
    const RGBB *p = seqs;
    RGBB rgb0;
    RGBB *q = seqd;

    memset((void *)&rgb0, 0, sizeof(rgb0));
    for (p = seqs, q = seqd; memcmp(p, &rgb0, sizeof(rgb0)) != 0 && 
        num > 0; num--, p++, q++)
        memcpy((void *)q, p, sizeof(RGBB));
    memcpy((void *)q, p, sizeof(RGBB));// append trailing end token.

    return;
}

// Create test directory.
inline int dir_setup(const char *testdir)
{
    int ret;

#if DB_VERSION_MAJOR > 4 || DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR > 3
    if ((ret = __os_mkdir(NULL, testdir, 0755)) != 0) {
#else
    if ((ret = mkdir(testdir, 0755)) != 0) {
#endif
        fprintf(stderr,
            "dir_setup: Creating directory %s: %s\n", 
            testdir, db_strerror(ret));
        return (1);
    }
    return (0);
}

inline int os_unlink(const char *path)
{
#if DB_VERSION_MAJOR < 4 || DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR < 7
    return (__os_unlink(NULL, path));
#else
    return (__os_unlink(NULL, path, 0));
#endif
}

#if DB_VERSION_MAJOR > 4 || DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR > 4
#define OS_EXISTS(a, b, c)  __os_exists(a, b, c)
#else
#define OS_EXISTS(a, b, c)  __os_exists(b, c)
#endif

// Remove contents in specified directory recursively(in DB versions earlier 
// than 4.6, don't recursively), and optionally remove the directory itself.
int rmdir_rcsv(const char *dir, bool keep_this_dir)
{
    int cnt, i, isdir, ret;
    char buf[1024], **names;

    ret = 0;

    /* If the directory doesn't exist, we're done. */
    if (OS_EXISTS(NULL, dir, &isdir) != 0)
        return (0);

    /* Get a list of the directory contents. */
#if DB_VERSION_MAJOR > 4 || DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR > 6
    if ((ret = __os_dirlist(NULL, dir, 1, &names, &cnt)) != 0)
        return (ret);
#else
    if ((ret = __os_dirlist(NULL, dir, &names, &cnt)) != 0)
        return (ret);
#endif
    /* Go through the file name list, remove each file in the list */
    for (i = 0; i < cnt; ++i) {
        (void)snprintf(buf, sizeof(buf),
            "%s%c%s", dir, PATH_SEPARATOR[0], names[i]);
        if ((ret = OS_EXISTS(NULL, buf, &isdir)) != 0)
            goto file_err;
        if (!isdir && (ret = os_unlink(buf)) != 0) {
file_err:       fprintf(stderr, 
                "os_unlink: Error unlinking file: %s: %s\n",
                buf, db_strerror(ret));
            break;
        }
        if (isdir && rmdir_rcsv(buf, false) != 0)
            goto file_err;
    }

    __os_dirfree(NULL, names, cnt);

    /*
     * If we removed the contents of the directory and we don't want to
     * keep this directory, remove the directory itself.
     */
    if (i == cnt && !keep_this_dir && (ret = rmdir(dir)) != 0)
        fprintf(stderr,
            "rmdir_rcsv(%s): %s\n", dir, db_strerror(errno));
    return (ret);
} // rmdir_rcsv


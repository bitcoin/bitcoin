/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2007-2009 Oracle.  All rights reserved.
 *
 * $Id$ 
 */

#ifndef _TPCBEXAMPLE_H_INCLUDE__
#define _TPCBEXAMPLE_H_INCLUDE__

#include <windows.h>
#include "db.h"

#define ACCOUNTS        1000
#define BRANCHES          10
#define TELLERS          100
#define HISTORY        10000
#define TRANSACTIONS    1000
#define TESTDIR     "TESTDIR"

typedef enum { ACCOUNT, BRANCH, TELLER } FTYPE;

extern "C" {
void tpcb_errcallback(const DB_ENV *, const char *, const char *);
}

class TpcbExample
{
public:
    int createEnv(int);
    void closeEnv();
    int populate();
    int run(int);
    int txn(DB *, DB *, DB *, DB *, int, int, int);
    int populateHistory(DB *, int, u_int32_t, u_int32_t, u_int32_t);
    int populateTable(DB *, u_int32_t, u_int32_t, int, const char *);

    TpcbExample();

    char *getHomeDir(char *, int);
    wchar_t *getHomeDirW(wchar_t *, int);
    void setHomeDir(char *);
    void setHomeDirW(wchar_t *);

#define ERR_STRING_MAX 1024
    char msgString[ERR_STRING_MAX];
    int accounts;
    int branches;
    int history;
    int tellers;

    // options configured through the advanced dialog.
    int fast_mode;
    int verbose;
    int cachesize;
    int rand_seed;
private:
    DB_ENV *dbenv;
    char homeDirName[MAX_PATH];
    wchar_t wHomeDirName[MAX_PATH];

    u_int32_t randomId(FTYPE, u_int32_t, u_int32_t, u_int32_t);
    u_int32_t randomInt(u_int32_t, u_int32_t);
    // no need for copy and assignment
    TpcbExample(const TpcbExample &);
    void operator = (const TpcbExample &);
};

#endif

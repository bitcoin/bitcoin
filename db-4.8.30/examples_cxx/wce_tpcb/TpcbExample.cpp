/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2007-2009 Oracle.  All rights reserved.
 *
 * $Id$ 
 */

#include "TpcbExample.h"

#define HISTORY_LEN     100
#define RECLEN          100
#define BEGID       1000000

struct Defrec {
    u_int32_t   id;
    u_int32_t   balance;
    u_int8_t    pad[RECLEN - sizeof(u_int32_t) - sizeof(u_int32_t)];
};

struct Histrec {
    u_int32_t   aid;
    u_int32_t   bid;
    u_int32_t   tid;
    u_int32_t   amount;
    u_int8_t    pad[RECLEN - 4 * sizeof(u_int32_t)];
};

const char *progname = "wce_tpcb";

TpcbExample::TpcbExample()
:   dbenv(0), accounts(ACCOUNTS), branches(BRANCHES),
    tellers(TELLERS), history(HISTORY), fast_mode(1), verbose(0),
    cachesize(0)
{
    rand_seed = GetTickCount();
    setHomeDir(TESTDIR);
}

int TpcbExample::createEnv(int flags)
{
    int ret;
    u_int32_t local_flags;

    // If the env object already exists, close and re-open
    // don't just return immediately, since advanced options
    // may have been altered (cachesize, NOSYNC..)
    if (dbenv != NULL)
        closeEnv();

    srand(rand_seed);

    if ((ret = db_env_create(&dbenv, 0)) != 0) {
        _snprintf(msgString, ERR_STRING_MAX,
            "%s: db_env_create: %s\n", progname, db_strerror(ret));
        return (1);
    }
    dbenv->set_errcall(dbenv, &tpcb_errcallback);
    dbenv->set_errpfx(dbenv, "TpcbExample");
    dbenv->set_cachesize(dbenv, 0, cachesize == 0 ?
                1 * 1024 * 1024 : (u_int32_t)cachesize, 0);

    if (fast_mode)
        dbenv->set_flags(dbenv, DB_TXN_NOSYNC, 1);

    local_flags = DB_CREATE | DB_INIT_LOCK | DB_INIT_LOG |
        DB_INIT_MPOOL | DB_INIT_TXN;
    dbenv->open(dbenv, homeDirName, local_flags, 0);
    return (0);
}

void TpcbExample::closeEnv()
{
    if (dbenv != NULL) {
        dbenv->close(dbenv, 0);
        dbenv = 0;
    }
}

//
// Initialize the database to the specified number of accounts, branches,
// history records, and tellers.
//
int
TpcbExample::populate()
{
    DB *dbp;

    int err;
    u_int32_t balance, idnum;
    u_int32_t end_anum, end_bnum, end_tnum;
    u_int32_t start_anum, start_bnum, start_tnum;

    idnum = BEGID;
    balance = 500000;

    if ((err = db_create(&dbp, dbenv, 0)) != 0) {
        _snprintf(msgString, ERR_STRING_MAX,
            "db_create of accounts db failed.");
        return (1);
    }
    dbp->set_h_nelem(dbp, (unsigned int)accounts);

    if ((err = dbp->open(dbp, NULL, "account", NULL, DB_HASH,
                 DB_CREATE, 0644)) != 0) {
        _snprintf(msgString, ERR_STRING_MAX,
            "Account file create failed. error: %s.", db_strerror(err));
        return (1);
    }

    start_anum = idnum;
    if ((err =
        populateTable(dbp, idnum, balance, accounts, "account")) != 0)
        return (1);
    idnum += accounts;
    end_anum = idnum - 1;
    if ((err = dbp->close(dbp, 0)) != 0) {
        _snprintf(msgString, ERR_STRING_MAX,
            "Account file close failed. error: %s.", db_strerror(err));
        return (1);
    }

    if ((err = db_create(&dbp, dbenv, 0)) != 0) {
        _snprintf(msgString, ERR_STRING_MAX,
            "db_create of branches db failed.");
        return (1);
    }
    //
    // Since the number of branches is very small, we want to use very
    // small pages and only 1 key per page.  This is the poor-man's way
    // of getting key locking instead of page locking.
    //
    dbp->set_h_ffactor(dbp, 1);
    dbp->set_h_nelem(dbp, (unsigned int)branches);
    dbp->set_pagesize(dbp, 512);

    if ((err = dbp->open(dbp, NULL, "branch", NULL, DB_HASH,
                 DB_CREATE, 0644)) != 0) {
        _snprintf(msgString, ERR_STRING_MAX,
            "Branch file create failed. error: %s.", db_strerror(err));
        return (1);
    }
    start_bnum = idnum;
    if ((err = populateTable(dbp, idnum, balance, branches, "branch")) != 0)
        return (1);
    idnum += branches;
    end_bnum = idnum - 1;
    if ((err = dbp->close(dbp, 0)) != 0) {
        _snprintf(msgString, ERR_STRING_MAX,
            "Close of branch file failed. error: %s.",
            db_strerror(err));
        return (1);
    }

    if ((err = db_create(&dbp, dbenv, 0)) != 0) {
        _snprintf(msgString, ERR_STRING_MAX,
            "db_create of teller db failed.");
        return (1);
    }
    //
    // In the case of tellers, we also want small pages, but we'll let
    // the fill factor dynamically adjust itself.
    //
    dbp->set_h_ffactor(dbp, 0);
    dbp->set_h_nelem(dbp, (unsigned int)tellers);
    dbp->set_pagesize(dbp, 512);

    if ((err = dbp->open(dbp, NULL, "teller", NULL, DB_HASH,
                 DB_CREATE, 0644)) != 0) {
        _snprintf(msgString, ERR_STRING_MAX,
            "Teller file create failed. error: %s.", db_strerror(err));
        return (1);
    }

    start_tnum = idnum;
    if ((err = populateTable(dbp, idnum, balance, tellers, "teller")) != 0)
        return (1);
    idnum += tellers;
    end_tnum = idnum - 1;
    if ((err = dbp->close(dbp, 0)) != 0) {
        _snprintf(msgString, ERR_STRING_MAX,
            "Close of teller file failed. error: %s.",
            db_strerror(err));
        return (1);
    }

    if ((err = db_create(&dbp, dbenv, 0)) != 0) {
        _snprintf(msgString, ERR_STRING_MAX,
            "db_create of history db failed.");
        return (1);
    }
    dbp->set_re_len(dbp, HISTORY_LEN);
    if ((err = dbp->open(dbp, NULL, "history", NULL, DB_RECNO,
                 DB_CREATE, 0644)) != 0) {
        _snprintf(msgString, ERR_STRING_MAX,
            "Create of history file failed. error: %s.",
            db_strerror(err));
        return (1);
    }

    populateHistory(dbp, history, accounts, branches, tellers);
    if ((err = dbp->close(dbp, 0)) != 0) {
        _snprintf(msgString, ERR_STRING_MAX,
            "Close of history file failed. error: %s.",
            db_strerror(err));
        return (1);
    }

    _snprintf(msgString, ERR_STRING_MAX, "Populated OK.");
    return (0);
}

int
TpcbExample::populateTable(DB *dbp,
               u_int32_t start_id, u_int32_t balance,
               int nrecs, const char *msg)
{
    DBT kdbt, ddbt;
    Defrec drec;
    memset(&drec.pad[0], 1, sizeof(drec.pad));

    memset(&kdbt, 0, sizeof(kdbt));
    memset(&ddbt, 0, sizeof(ddbt));
    kdbt.data = &drec.id;
    kdbt.size = sizeof(u_int32_t);
    ddbt.data = &drec;
    ddbt.size = sizeof(drec);

    for (int i = 0; i < nrecs; i++) {
        drec.id = start_id + (u_int32_t)i;
        drec.balance = balance;
        int err;
        if ((err =
            dbp->put(dbp, NULL, &kdbt, &ddbt, DB_NOOVERWRITE)) != 0) {
            _snprintf(msgString, ERR_STRING_MAX,
        "Failure initializing %s file: %s. Likely re-initializing.",
                msg, db_strerror(err));
            return (1);
        }
    }
    return (0);
}

int
TpcbExample::populateHistory(DB *dbp, int nrecs, u_int32_t accounts,
                 u_int32_t branches, u_int32_t tellers)
{
    DBT kdbt, ddbt;
    Histrec hrec;
    memset(&hrec.pad[0], 1, sizeof(hrec.pad));
    hrec.amount = 10;
    db_recno_t key;

    memset(&kdbt, 0, sizeof(kdbt));
    memset(&ddbt, 0, sizeof(ddbt));
    kdbt.data = &key;
    kdbt.size = sizeof(u_int32_t);
    ddbt.data = &hrec;
    ddbt.size = sizeof(hrec);

    for (int i = 1; i <= nrecs; i++) {
        hrec.aid = randomId(ACCOUNT, accounts, branches, tellers);
        hrec.bid = randomId(BRANCH, accounts, branches, tellers);
        hrec.tid = randomId(TELLER, accounts, branches, tellers);

        int err;
        key = (db_recno_t)i;
        if ((err = dbp->put(dbp, NULL, &kdbt, &ddbt, DB_APPEND)) != 0) {
            _snprintf(msgString, ERR_STRING_MAX,
                "Failure initializing history file: %s.",
                db_strerror(err));
            return (1);
        }
    }
    return (0);
}

int
TpcbExample::run(int n)
{
    DB *adb, *bdb, *hdb, *tdb;
    int failed, ret, txns;
    DWORD start_time, end_time;
    double elapsed_secs;

    //
    // Open the database files.
    //

    int err;
    if ((err = db_create(&adb, dbenv, 0)) != 0) {
        _snprintf(msgString, ERR_STRING_MAX,
            "db_create of account db failed. Error: %s",
            db_strerror(err));
        return (1);
    }
    if ((err = adb->open(adb, NULL, "account", NULL, DB_UNKNOWN,
                 DB_AUTO_COMMIT, 0)) != 0) {
        _snprintf(msgString, ERR_STRING_MAX,
            "Open of account file failed. Error: %s", db_strerror(err));
        return (1);
    }

    if ((err = db_create(&bdb, dbenv, 0)) != 0) {
        _snprintf(msgString, ERR_STRING_MAX,
            "db_create of branch db failed. Error: %s",
            db_strerror(err));
        return (1);
    }
    if ((err = bdb->open(bdb, NULL, "branch", NULL, DB_UNKNOWN,
                 DB_AUTO_COMMIT, 0)) != 0) {
        _snprintf(msgString, ERR_STRING_MAX,
            "Open of branch file failed. Error: %s", db_strerror(err));
        return (1);
    }

    if ((err = db_create(&tdb, dbenv, 0)) != 0) {
        _snprintf(msgString, ERR_STRING_MAX,
            "db_create of teller db failed. Error: %s",
            db_strerror(err));
        return (1);
    }
    if ((err = tdb->open(tdb, NULL, "teller", NULL, DB_UNKNOWN,
                 DB_AUTO_COMMIT, 0)) != 0) {
        _snprintf(msgString, ERR_STRING_MAX,
            "Open of teller file failed. Error: %s", db_strerror(err));
        return (1);
    }

    if ((err = db_create(&hdb, dbenv, 0)) != 0) {
        _snprintf(msgString, ERR_STRING_MAX,
            "db_create of teller db failed. Error: %s",
            db_strerror(err));
        return (1);
    }
    if ((err = hdb->open(hdb, NULL, "history", NULL, DB_UNKNOWN,
                 DB_AUTO_COMMIT, 0)) != 0) {
        _snprintf(msgString, ERR_STRING_MAX,
            "Open of history file failed. Error: %s", db_strerror(err));
        return (1);
    }

    start_time = GetTickCount();
    for (txns = n, failed = 0; n-- > 0;)
        if ((ret = txn(adb, bdb, tdb, hdb,
            accounts, branches, tellers)) != 0)
            ++failed;
    end_time = GetTickCount();
    if (end_time == start_time)
        ++end_time;
#define MILLISECS_PER_SEC 1000
    elapsed_secs = (double)((end_time - start_time))/MILLISECS_PER_SEC;
    _snprintf(msgString, ERR_STRING_MAX,
        "%s: %d txns: %d failed, %.2f TPS\n", progname, txns, failed,
        (txns - failed) / elapsed_secs);

    (void)adb->close(adb, 0);
    (void)bdb->close(bdb, 0);
    (void)tdb->close(tdb, 0);
    (void)hdb->close(hdb, 0);

    return (0);
}

//
// XXX Figure out the appropriate way to pick out IDs.
//
int
TpcbExample::txn(DB *adb, DB *bdb, DB *tdb, DB *hdb,
         int accounts, int branches, int tellers)
{
    DBC *acurs, *bcurs, *tcurs;
    DB_TXN *t;
    DBT d_dbt, d_histdbt, k_dbt, k_histdbt;

    db_recno_t key;
    Defrec rec;
    Histrec hrec;
    int account, branch, teller, ret;

    memset(&d_dbt, 0, sizeof(d_dbt));
    memset(&d_histdbt, 0, sizeof(d_histdbt));
    memset(&k_dbt, 0, sizeof(k_dbt));
    memset(&k_histdbt, 0, sizeof(k_histdbt));
    k_histdbt.data = &key;
    k_histdbt.size = sizeof(key);

    // !!!
    // This is sample code -- we could move a lot of this into the driver
    // to make it faster.
    //
    account = randomId(ACCOUNT, accounts, branches, tellers);
    branch = randomId(BRANCH, accounts, branches, tellers);
    teller = randomId(TELLER, accounts, branches, tellers);

    k_dbt.size = sizeof(int);

    d_dbt.flags |= DB_DBT_USERMEM;
    d_dbt.data = &rec;
    d_dbt.ulen = sizeof(rec);

    hrec.aid = account;
    hrec.bid = branch;
    hrec.tid = teller;
    hrec.amount = 10;
    // Request 0 bytes since we're just positioning.
    d_histdbt.flags |= DB_DBT_PARTIAL;

    // START PER-TRANSACTION TIMING.
    //
    // Technically, TPCB requires a limit on response time, you only get
    // to count transactions that complete within 2 seconds.  That's not
    // an issue for this sample application -- regardless, here's where
    // the transaction begins.
    if (dbenv->txn_begin(dbenv, NULL, &t, 0) != 0)
        goto err;

    if (adb->cursor(adb, t, &acurs, 0) != 0 ||
        bdb->cursor(bdb, t, &bcurs, 0) != 0 ||
        tdb->cursor(tdb, t, &tcurs, 0) != 0)
        goto err;

    // Account record
    k_dbt.data = &account;
    if (acurs->get(acurs, &k_dbt, &d_dbt, DB_SET) != 0)
        goto err;
    rec.balance += 10;
    if (acurs->put(acurs, &k_dbt, &d_dbt, DB_CURRENT) != 0)
        goto err;

    // Branch record
    k_dbt.data = &branch;
    if (bcurs->get(bcurs, &k_dbt, &d_dbt, DB_SET) != 0)
        goto err;
    rec.balance += 10;
    if (bcurs->put(bcurs, &k_dbt, &d_dbt, DB_CURRENT) != 0)
        goto err;

    // Teller record
    k_dbt.data = &teller;
    if (tcurs->get(tcurs, &k_dbt, &d_dbt, DB_SET) != 0)
        goto err;
    rec.balance += 10;
    if (tcurs->put(tcurs, &k_dbt, &d_dbt, DB_CURRENT) != 0)
        goto err;

    // History record
    d_histdbt.flags = 0;
    d_histdbt.data = &hrec;
    d_histdbt.ulen = sizeof(hrec);
    if (hdb->put(hdb, t, &k_histdbt, &d_histdbt, DB_APPEND) != 0)
        goto err;

    if (acurs->close(acurs) != 0 || bcurs->close(bcurs) != 0
        || tcurs->close(tcurs) != 0)
        goto err;

    ret = t->commit(t, 0);
    t = NULL;
    if (ret != 0)
        goto err;

    // END PER-TRANSACTION TIMING.
    return (0);

err:
    if (acurs != NULL)
        (void)acurs->close(acurs);
    if (bcurs != NULL)
        (void)bcurs->close(bcurs);
    if (tcurs != NULL)
        (void)tcurs->close(tcurs);
    if (t != NULL)
        (void)t->abort(t);

    return (-1);
}

// Utility functions

u_int32_t
TpcbExample::randomInt(u_int32_t lo, u_int32_t hi)
{
    u_int32_t ret;
    int t;

    t = rand();
    ret = (u_int32_t)(((double)t / ((double)(RAND_MAX) + 1)) *
              (hi - lo + 1));
    ret += lo;
    return (ret);
}

u_int32_t
TpcbExample::randomId(FTYPE type, u_int32_t accounts,
              u_int32_t branches, u_int32_t tellers)
{
    u_int32_t min, max, num;

    max = min = BEGID;
    num = accounts;
    switch (type) {
    case TELLER:
        min += branches;
        num = tellers;
        // Fallthrough
    case BRANCH:
        if (type == BRANCH)
            num = branches;
        min += accounts;
        // Fallthrough
    case ACCOUNT:
        max = min + num - 1;
    }
    return (randomInt(min, max));
}

char *
TpcbExample::getHomeDir(char *path, int max)
{
    memcpy(path, homeDirName, min(max, MAX_PATH));
    return path;
}

wchar_t *
TpcbExample::getHomeDirW(wchar_t *path, int max)
{
    memcpy(path, wHomeDirName, min(max, MAX_PATH)*sizeof(wchar_t));
    return path;
}

void
TpcbExample::setHomeDir(char *path)
{
    int path_len;

    path_len = strlen(path);
    strncpy(homeDirName, path, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, path, path_len, wHomeDirName, MAX_PATH);
    wHomeDirName[path_len] = L'\0';
}

void
TpcbExample::setHomeDirW(wchar_t *path)
{
    int path_len;

    path_len = wcslen(path);
    wcsncpy(wHomeDirName, path, MAX_PATH);
    WideCharToMultiByte(CP_ACP, 0, path, path_len, homeDirName,
        MAX_PATH, 0, 0);
    homeDirName[path_len] = '\0';
}

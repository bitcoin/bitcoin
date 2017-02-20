#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <db.h>

#define ENV_DIRECTORY   "TXNAPP"

void  add_cat(DB_ENV *, DB *, char *, ...);
void  add_color(DB_ENV *, DB *, char *, int);
void  add_fruit(DB_ENV *, DB *, char *, char *);
void *checkpoint_thread(void *);
void  log_archlist(DB_ENV *);
void *logfile_thread(void *);
void  db_open(DB_ENV *, DB **, char *, int);
void  env_dir_create(void);
void  env_open(DB_ENV **);
void  usage(void);

int
main(int argc, char *argv[])
{
    extern int optind;
    DB *db_cats, *db_color, *db_fruit;
    DB_ENV *dbenv;
    pthread_t ptid;
    int ch, ret;

    while ((ch = getopt(argc, argv, "")) != EOF)
        switch (ch) {
        case '?':
        default:
            usage();
        }
    argc -= optind;
    argv += optind;

    env_dir_create();
    env_open(&dbenv);

    /* Start a checkpoint thread. */
    if ((ret = pthread_create(
        &ptid, NULL, checkpoint_thread, (void *)dbenv)) != 0) {
        fprintf(stderr,
            "txnapp: failed spawning checkpoint thread: %s\n",
            strerror(ret));
        exit (1);
    }

    /* Start a logfile removal thread. */
    if ((ret = pthread_create(
        &ptid, NULL, logfile_thread, (void *)dbenv)) != 0) {
        fprintf(stderr,
            "txnapp: failed spawning log file removal thread: %s\n",
            strerror(ret));
        exit (1);
    }

    /* Open database: Key is fruit class; Data is specific type. */
    db_open(dbenv, &db_fruit, "fruit", 0);

    /* Open database: Key is a color; Data is an integer. */
    db_open(dbenv, &db_color, "color", 0);

    /*
     * Open database:
     *  Key is a name; Data is: company name, address, cat breeds.
     */
    db_open(dbenv, &db_cats, "cats", 1);

    add_fruit(dbenv, db_fruit, "apple", "yellow delicious");

    add_color(dbenv, db_color, "blue", 0);
    add_color(dbenv, db_color, "blue", 3);

    add_cat(dbenv, db_cats,
        "Amy Adams",
        "Oracle",
        "394 E. Riding Dr., Carlisle, MA 01741, USA",
        "abyssinian",
        "bengal",
        "chartreaux",
        NULL);

    return (0);
}

void
env_dir_create()
{
    struct stat sb;

    /*
     * If the directory exists, we're done.  We do not further check
     * the type of the file, DB will fail appropriately if it's the
     * wrong type.
     */
    if (stat(ENV_DIRECTORY, &sb) == 0)
        return;

    /* Create the directory, read/write/access owner only. */
    if (mkdir(ENV_DIRECTORY, S_IRWXU) != 0) {
        fprintf(stderr,
            "txnapp: mkdir: %s: %s\n", ENV_DIRECTORY, strerror(errno));
        exit (1);
    }
}

void
env_open(DB_ENV **dbenvp)
{
    DB_ENV *dbenv;
    int ret;

    /* Create the environment handle. */
    if ((ret = db_env_create(&dbenv, 0)) != 0) {
        fprintf(stderr,
            "txnapp: db_env_create: %s\n", db_strerror(ret));
        exit (1);
    }

    /* Set up error handling. */
    dbenv->set_errpfx(dbenv, "txnapp");
    dbenv->set_errfile(dbenv, stderr);

    /* Do deadlock detection internally. */
    if ((ret = dbenv->set_lk_detect(dbenv, DB_LOCK_DEFAULT)) != 0) {
        dbenv->err(dbenv, ret, "set_lk_detect: DB_LOCK_DEFAULT");
        exit (1);
    }

    /*
     * Open a transactional environment:
     *  create if it doesn't exist
     *  free-threaded handle
     *  run recovery
     *  read/write owner only
     */
    if ((ret = dbenv->open(dbenv, ENV_DIRECTORY,
        DB_CREATE | DB_INIT_LOCK | DB_INIT_LOG |
        DB_INIT_MPOOL | DB_INIT_TXN | DB_RECOVER | DB_THREAD,
        S_IRUSR | S_IWUSR)) != 0) {
        dbenv->err(dbenv, ret, "dbenv->open: %s", ENV_DIRECTORY);
        exit (1);
    }

    *dbenvp = dbenv;
}

void *
checkpoint_thread(void *arg)
{
    DB_ENV *dbenv;
    int ret;

    dbenv = arg;
    dbenv->errx(dbenv, "Checkpoint thread: %lu", (u_long)pthread_self());

    /* Checkpoint once a minute. */
    for (;; sleep(60))
        if ((ret = dbenv->txn_checkpoint(dbenv, 0, 0, 0)) != 0) {
            dbenv->err(dbenv, ret, "checkpoint thread");
            exit (1);
        }

    /* NOTREACHED */
}

void *
logfile_thread(void *arg)
{
    DB_ENV *dbenv;
    int ret;
    char **begin, **list;

    dbenv = arg;
    dbenv->errx(dbenv,
        "Log file removal thread: %lu", (u_long)pthread_self());

    /* Check once every 5 minutes. */
    for (;; sleep(300)) {
        /* Get the list of log files. */
        if ((ret =
            dbenv->log_archive(dbenv, &list, DB_ARCH_ABS)) != 0) {
            dbenv->err(dbenv, ret, "DB_ENV->log_archive");
            exit (1);
        }

        /* Remove the log files. */
        if (list != NULL) {
            for (begin = list; *list != NULL; ++list)
                if ((ret = remove(*list)) != 0) {
                    dbenv->err(dbenv,
                        ret, "remove %s", *list);
                    exit (1);
                }
            free (begin);
        }
    }
    /* NOTREACHED */
}

void
log_archlist(DB_ENV *dbenv)
{
    int ret;
    char **begin, **list;

    /* Get the list of database files. */
    if ((ret = dbenv->log_archive(dbenv,
        &list, DB_ARCH_ABS | DB_ARCH_DATA)) != 0) {
        dbenv->err(dbenv, ret, "DB_ENV->log_archive: DB_ARCH_DATA");
        exit (1);
    }
    if (list != NULL) {
        for (begin = list; *list != NULL; ++list)
            printf("database file: %s\n", *list);
        free (begin);
    }

    /* Get the list of log files. */
    if ((ret = dbenv->log_archive(dbenv,
        &list, DB_ARCH_ABS | DB_ARCH_LOG)) != 0) {
        dbenv->err(dbenv, ret, "DB_ENV->log_archive: DB_ARCH_LOG");
        exit (1);
    }
    if (list != NULL) {
        for (begin = list; *list != NULL; ++list)
            printf("log file: %s\n", *list);
        free (begin);
    }
}

void
db_open(DB_ENV *dbenv, DB **dbp, char *name, int dups)
{
    DB *db;
    int ret;

    /* Create the database handle. */
    if ((ret = db_create(&db, dbenv, 0)) != 0) {
        dbenv->err(dbenv, ret, "db_create");
        exit (1);
    }

    /* Optionally, turn on duplicate data items. */
    if (dups && (ret = db->set_flags(db, DB_DUP)) != 0) {
        dbenv->err(dbenv, ret, "db->set_flags: DB_DUP");
        exit (1);
    }

    /*
     * Open a database in the environment:
     *  create if it doesn't exist
     *  free-threaded handle
     *  read/write owner only
     */
    if ((ret = db->open(db, NULL, name, NULL, DB_BTREE,
        DB_AUTO_COMMIT | DB_CREATE | DB_THREAD, S_IRUSR | S_IWUSR)) != 0) {
        (void)db->close(db, 0);
        dbenv->err(dbenv, ret, "db->open: %s", name);
        exit (1);
    }

    *dbp = db;
}

void
add_fruit(DB_ENV *dbenv, DB *db, char *fruit, char *name)
{
    DBT key, data;
    DB_TXN *tid;
    int ret;

    /* Initialization. */
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    key.data = fruit;
    key.size = strlen(fruit);
    data.data = name;
    data.size = strlen(name);

    for (;;) {
        /* Begin the transaction. */
        if ((ret = dbenv->txn_begin(dbenv, NULL, &tid, 0)) != 0) {
            dbenv->err(dbenv, ret, "DB_ENV->txn_begin");
            exit (1);
        }

        /* Store the value. */
        switch (ret = db->put(db, tid, &key, &data, 0)) {
        case 0:
            /* Success: commit the change. */
            if ((ret = tid->commit(tid, 0)) != 0) {
                dbenv->err(dbenv, ret, "DB_TXN->commit");
                exit (1);
            }
            return;
        case DB_LOCK_DEADLOCK:
            /* Deadlock: retry the operation. */
            if ((ret = tid->abort(tid)) != 0) {
                dbenv->err(dbenv, ret, "DB_TXN->abort");
                exit (1);
            }
            break;
        default:
            /* Error: run recovery. */
            dbenv->err(dbenv, ret, "dbc->put: %s/%s", fruit, name);
            exit (1);
        }
    }
}

void
add_color(DB_ENV *dbenv, DB *dbp, char *color, int increment)
{
    DBT key, data;
    DB_TXN *tid;
    int original, ret;
    char buf[64];

    /* Initialization. */
    memset(&key, 0, sizeof(key));
    key.data = color;
    key.size = strlen(color);
    memset(&data, 0, sizeof(data));
    data.flags = DB_DBT_MALLOC;

    for (;;) {
        /* Begin the transaction. */
        if ((ret = dbenv->txn_begin(dbenv, NULL, &tid, 0)) != 0) {
            dbenv->err(dbenv, ret, "DB_ENV->txn_begin");
            exit (1);
        }

        /*
         * Get the key.  If it exists, we increment the value.  If it
         * doesn't exist, we create it.
         */
        switch (ret = dbp->get(dbp, tid, &key, &data, 0)) {
        case 0:
            original = atoi(data.data);
            break;
        case DB_LOCK_DEADLOCK:
            /* Deadlock: retry the operation. */
            if ((ret = tid->abort(tid)) != 0) {
                dbenv->err(dbenv, ret, "DB_TXN->abort");
                exit (1);
            }
            continue;
        case DB_NOTFOUND:
            original = 0;
            break;
        default:
            /* Error: run recovery. */
            dbenv->err(
                dbenv, ret, "dbc->get: %s/%d", color, increment);
            exit (1);
        }
        if (data.data != NULL)
            free(data.data);

        /* Create the new data item. */
        (void)snprintf(buf, sizeof(buf), "%d", original + increment);
        data.data = buf;
        data.size = strlen(buf) + 1;

        /* Store the new value. */
        switch (ret = dbp->put(dbp, tid, &key, &data, 0)) {
        case 0:
            /* Success: commit the change. */
            if ((ret = tid->commit(tid, 0)) != 0) {
                dbenv->err(dbenv, ret, "DB_TXN->commit");
                exit (1);
            }
            return;
        case DB_LOCK_DEADLOCK:
            /* Deadlock: retry the operation. */
            if ((ret = tid->abort(tid)) != 0) {
                dbenv->err(dbenv, ret, "DB_TXN->abort");
                exit (1);
            }
            break;
        default:
            /* Error: run recovery. */
            dbenv->err(
                dbenv, ret, "dbc->put: %s/%d", color, increment);
            exit (1);
        }
    }
}

void
add_cat(DB_ENV *dbenv, DB *db, char *name, ...)
{
    va_list ap;
    DBC *dbc;
    DBT key, data;
    DB_TXN *tid;
    int ret;
    char *s;

    /* Initialization. */
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    key.data = name;
    key.size = strlen(name);

retry:  /* Begin the transaction. */
    if ((ret = dbenv->txn_begin(dbenv, NULL, &tid, 0)) != 0) {
        dbenv->err(dbenv, ret, "DB_ENV->txn_begin");
        exit (1);
    }

    /* Delete any previously existing item. */
    switch (ret = db->del(db, tid, &key, 0)) {
    case 0:
    case DB_NOTFOUND:
        break;
    case DB_LOCK_DEADLOCK:
        /* Deadlock: retry the operation. */
        if ((ret = tid->abort(tid)) != 0) {
            dbenv->err(dbenv, ret, "DB_TXN->abort");
            exit (1);
        }
        goto retry;
    default:
        dbenv->err(dbenv, ret, "db->del: %s", name);
        exit (1);
    }

    /* Create a cursor. */
    if ((ret = db->cursor(db, tid, &dbc, 0)) != 0) {
        dbenv->err(dbenv, ret, "db->cursor");
        exit (1);
    }

    /* Append the items, in order. */
    va_start(ap, name);
    while ((s = va_arg(ap, char *)) != NULL) {
        data.data = s;
        data.size = strlen(s);
        switch (ret = dbc->c_put(dbc, &key, &data, DB_KEYLAST)) {
        case 0:
            break;
        case DB_LOCK_DEADLOCK:
            va_end(ap);

            /* Deadlock: retry the operation. */
            if ((ret = dbc->c_close(dbc)) != 0) {
                dbenv->err(
                    dbenv, ret, "dbc->c_close");
                exit (1);
            }
            if ((ret = tid->abort(tid)) != 0) {
                dbenv->err(dbenv, ret, "DB_TXN->abort");
                exit (1);
            }
            goto retry;
        default:
            /* Error: run recovery. */
            dbenv->err(dbenv, ret, "dbc->put: %s/%s", name, s);
            exit (1);
        }
    }
    va_end(ap);

    /* Success: commit the change. */
    if ((ret = dbc->c_close(dbc)) != 0) {
        dbenv->err(dbenv, ret, "dbc->c_close");
        exit (1);
    }
    if ((ret = tid->commit(tid, 0)) != 0) {
        dbenv->err(dbenv, ret, "DB_TXN->commit");
        exit (1);
    }
}

void
usage()
{
    (void)fprintf(stderr, "usage: txnapp\n");
    exit(1);
}

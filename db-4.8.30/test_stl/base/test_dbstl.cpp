/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$ 
 */

// test_dbstl.cpp : A test program which tests all container's all methods, 
// by verify the result is identical to expected value, or by doing paralell 
// operations with corresponding container of STL and verify the result of 
// both containers are identical after the same operations. Also, it tests 
// multithreaded test
//
#include "db_config.h"
#include "db_int.h"

#include "test.h"
#include "test_util.h"
#include "test_vector.h"
#include "test_assoc.h"
#include "test_mt.h"

int main(int argc, char *argv[])
{
    int c, i, j, ret;
    int nreader = 5, nwriter = 3;
    char *mode = NULL;
    int flags = DB_THREAD, setflags = 0, EXPLICIT_TXN = 0;
    int TEST_AUTOCOMMIT = 1, totalinsert = 100, verbose = 0;
    DBTYPE dbtype = DB_BTREE;
    int gi, shortest = 50, longest = 200, loop_count = 1;
    bool multionly = false;
    u_int32_t sflags = 0, oflags = 0;
    u_int32_t cachesize = 8 * 1024 * 1024;
    db_threadid_t tid;
    string contname;
    DbEnv *penv = NULL;
    Db *dbstrv = NULL;
    __os_id(0, 0, &tid);

    TestParam *ptp = new TestParam;
    ContainerType contypes[] = {
        ct_vector, 
        ct_map, 
        ct_multimap, 
        ct_set, 
        ct_multiset, 
        ct_none
    };

    while ((c = getopt(argc, argv, "MT:c:hk:l:m:n:r:s:t:vw:")) != EOF) {
        switch (c) {
            case 'M':
                multionly = true;
                break;
            case 'T':
                totalinsert = atoi(optarg);
                break;
            case 'c':
                cachesize = atoi(optarg);
                break;
            case 'h':
                usage();
                return 0;
                break;
            case 'k':
                shortest = atoi(optarg);
                break;
            case 'l':
                longest = atoi(optarg);
                break;
            case 'm':
                mode = optarg;
                break;
            case 'n':
                loop_count = atoi(optarg);
                break;
            case 'r':
                nreader = atoi(optarg);
                break;
            case 's': // db type for associative containers
                if (*optarg == 'h') // hash db type
                    dbtype = DB_HASH;
                else if (*optarg == 'b')
                    dbtype = DB_BTREE;
                else
                    usage();
                break;
            case 't':
                EXPLICIT_TXN = 1;
                if (*optarg == 'a') 
                    setflags = DB_AUTO_COMMIT;
                else if (*optarg == 'e') // explicit txn
                    TEST_AUTOCOMMIT = 0;
                else 
                    usage();
                break;
            case 'v':
                verbose = 1;
                break;
            case 'w':
                nwriter = atoi(optarg);
                break;
            default:
                usage();
                break;
        }
    }

    if (nwriter < 3)
        nwriter = 3;

    if (mode == NULL) 
        flags |= 0;
    else if (*mode == 'c') //cds
        flags |= DB_INIT_CDB;
    else if (*mode == 't')
        flags |= DB_INIT_TXN | DB_RECOVER 
            | DB_INIT_LOG | DB_INIT_LOCK;
    else
        flags |= 0;
    g_test_start_txn = TEST_AUTOCOMMIT * EXPLICIT_TXN;
    ptp->EXPLICIT_TXN = EXPLICIT_TXN;
    ptp->flags = flags;
    ptp->dbtype = dbtype;
    ptp->setflags = setflags;
    ptp->TEST_AUTOCOMMIT = TEST_AUTOCOMMIT;
    ptp->dboflags = DB_THREAD;

    // This may fail because there is already dbenv.
    __os_mkdir(NULL, "dbenv", 0777);
    if (!multionly){
        if (loop_count <= 0)
            loop_count = 1;
        dbstl_startup();
        for (; loop_count > 0; loop_count--){
            rmdir_rcsv("dbenv", true);
            BDBOP(__os_mkdir(NULL, "dbenv/data", 0777), ret);// This should not fail.
            penv = new DbEnv(DB_CXX_NO_EXCEPTIONS);
            BDBOP(penv->set_flags(setflags, 1), ret);
            BDBOP(penv->set_cachesize(0, cachesize, 1), ret);
            BDBOP(penv->set_data_dir("data"), ret);

            // Methods of containers returning a reference costs 
            // locker/object/lock slots.
            penv->set_lk_max_lockers(10000); 
            penv->set_lk_max_objects(10000);
            penv->set_lk_max_locks(10000);

            penv->set_flags(DB_TXN_NOSYNC, 1);
            penv->log_set_config(DB_LOG_IN_MEMORY, 1);
            penv->set_lg_bsize(128 * 1024 * 1024);

            BDBOP(penv->open("dbenv", flags | DB_CREATE | DB_INIT_MPOOL | DB_PRIVATE, 0777), ret);
            register_db_env(penv);
            // CDS dose not need deadlock detection
            if (penv->get_open_flags(&oflags) == 0 && ((oflags & DB_INIT_TXN) != 0)) {
                penv->set_lk_detect(DB_LOCK_RANDOM);
                penv->set_timeout(1000000, DB_SET_LOCK_TIMEOUT);

            }

            ptp->dbenv = penv;
            g_env = penv;
            TestVector testvector(ptp);
            testvector.start_test();
            TestAssoc testassoc(ptp);
            testassoc.start_test();
            
            close_all_dbs();
            //close_db_env(penv);
            close_all_db_envs();
            cerr<<"\n\nOne round of dbstl test is run to completion successfully!\n\n";
        }
    }
    else {
        dbstl_startup();
        rmdir_rcsv("dbenv", true);
        BDBOP(__os_mkdir(NULL, "dbenv/data", 0777), ret);// This should not fail.
        penv = new DbEnv(DB_CXX_NO_EXCEPTIONS);
        BDBOP(penv->set_flags(setflags, 1), ret);
        BDBOP(penv->set_cachesize(0, cachesize, 1), ret);
        BDBOP(penv->set_data_dir("data"), ret);

        // Multi-CPU machines requires larger lockers/locks/objects because of 
        // the higher concurrency, more iterators can be opened at the same time. 
        // This setting was required by the stahp05 8-cpu machine, not others.
                BDBOP(penv->set_lk_max_lockers(100000), ret);
                BDBOP(penv->set_lk_max_locks(100000), ret);
                BDBOP(penv->set_lk_max_objects(100000), ret);

        penv->set_flags(DB_TXN_NOSYNC, 1);
        penv->log_set_config(DB_LOG_IN_MEMORY, 1);
        penv->set_lg_bsize(128 * 1024 * 1024);

        BDBOP(penv->open("dbenv", flags | DB_CREATE | DB_INIT_MPOOL | DB_PRIVATE, 0777), ret);
        register_db_env(penv);
        // CDS dose not need deadlock detection
        if (penv->get_open_flags(&oflags) == 0 && ((oflags & DB_INIT_TXN) != 0)) {
            penv->set_lk_detect(DB_LOCK_RANDOM);
            penv->set_timeout(1000000, DB_SET_LOCK_TIMEOUT);
        }

        ptp->dbenv = penv;
        g_env = penv;
        
        ThreadMgr thrds, insthrds;
        vector<wt_job> jobs;
        jobs.push_back(wt_insert);
        jobs.push_back(wt_update);
        jobs.push_back(wt_delete);
        srand ((unsigned int)tid);

        for (i = 3; i < nwriter; i++) {
            j = rand() % 3;
            switch (j) {
                case 0: jobs.push_back(wt_insert); break;
                case 1: jobs.push_back(wt_update); break;
                case 2: jobs.push_back(wt_delete); break;
            }
        }

        vector<WorkerThread *> vctthrds;
        WorkerThread *pwthrd;
        for (gi = 0; contypes[gi] != ct_none; gi++) {

            switch (contypes[gi]) {
            case ct_vector: contname = "db_vector"; break;
            case ct_map: contname = "db_map"; break;
            case ct_multimap: contname = "db_multimap"; break;
            case ct_set: contname = "db_set"; break;
            case ct_multiset: contname = "db_multiset"; break;
            default: throw "unexpected container type!"; break;
            }
            cout<<endl<<contname<<" multi-threaded tests:\n";

            g_StopInsert = 0;
            WorkerThread par;
            memset(&par, 0, sizeof(par));
            par.total_insert = totalinsert;
            par.verbose = verbose != 0;
            par.strlenmax = longest;
            par.strlenmin = shortest;
            par.cntnr_type = contypes[gi];
            par.penv = penv;
            if (par.cntnr_type != ct_vector) {
                if (par.cntnr_type == ct_map || 
                    par.cntnr_type == ct_set)
                    sflags = 0;
                else    if (par.cntnr_type == ct_multimap || 
                    par.cntnr_type == ct_multiset)
                    sflags = DB_DUP;
                par.pdb = dbstl::
                    open_db(penv, "db_mt_map.db", 
                    ptp->dbtype, DB_CREATE | ptp->dboflags, sflags);
            } else if (par.cntnr_type == ct_vector) {
                if (NULL == dbstrv)
                    dbstrv = dbstl::open_db(penv, "dbstr.db", 
                    DB_RECNO, DB_CREATE | ptp->dboflags, DB_RENUMBER);
                par.pdb = dbstrv;
            }

            par.txnal = ptp->EXPLICIT_TXN != 0;
            for (j = 0; j < nwriter; j++) {
                par.job = jobs[j];
                pwthrd = new WorkerThread(par);
                if (par.job == wt_insert) {
                    insthrds.create_thread(pwthrd);
                } else {
                    thrds.create_thread(pwthrd); 
                }
                
                vctthrds.push_back(pwthrd);
                if (j == 0)
                    __os_yield(NULL, 3, 0);// sleep 3 secs
            }
        
            par.job = wt_read;
            for (i = 0; i < nreader; i++) {
                pwthrd = new WorkerThread(par);
                thrds.create_thread(pwthrd);
                vctthrds.push_back(pwthrd);
            }
            
            thrds.join_all();

            g_StopInsert = 1;
            insthrds.join_all();
            for (vector<WorkerThread *>::size_type ui = 0; ui < vctthrds.size(); ui++)
                delete vctthrds[ui];
            vctthrds.clear();
            if (ptp->EXPLICIT_TXN && !TEST_AUTOCOMMIT)
                dbstl::begin_txn(0, penv);
            dbstl::close_db(par.pdb);
            penv->dbremove(dbstl::current_txn(penv), "db_mt_map.db", NULL, 0);
            if (ptp->EXPLICIT_TXN && !TEST_AUTOCOMMIT)
                dbstl::commit_txn(penv);
            thrds.clear();
            insthrds.clear();
            cout.flush();
            
        } // for (int gi
        
        
    } // else

    delete ptp;
    dbstl_exit();

    if (penv) 
        delete penv;
}

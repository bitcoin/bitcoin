/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$ 
 */

#include "test.h"

// tests containers in multiple threads, this thread class 
// runs the thread function in specified manner---manipulating
// a specified type of container, with some thread inserting,
// updating, and deleting, some others reading specified amount
// of key/data pairs.
//
class WorkerThread 
{
public:
    bool txnal/* if true, will create txn */, verbose;
    wt_job job;
    ContainerType cntnr_type;
    int total_insert;
    int strlenmin, strlenmax;
    rand_str_dbt rand_str_maker;
    Db *pdb;
    DbEnv *penv;

    WorkerThread()
    {
        pdb = NULL;
        penv = NULL;

    }

    WorkerThread(const WorkerThread& wt)
    {
        memcpy(this, &wt, sizeof(wt));
    }

    void run()
    {
        WorkerThread *p = this;
        db_threadid_t tid;
        int cnt = 0, i, total = total_insert, ndx;
        DbstlDbt dbt, dbt2;
        string str, str2;
        bool iscds = false;
        u_int32_t oflags;
        size_t dlcnt = 0;
        

        dbt.set_flags(DB_DBT_USERMEM);
        dbt.set_data(DbstlMalloc(strlenmax + 10));
        dbt.set_ulen(strlenmax + 10);
        dbt2.set_flags(DB_DBT_USERMEM);
        dbt2.set_data(DbstlMalloc(strlenmax + 10));
        dbt2.set_ulen(strlenmax + 10);

        penv->get_open_flags(&oflags);
        if (oflags & DB_INIT_CDB)
            iscds = true;
        __os_id(NULL, 0, &tid);
        dbstl::register_db(p->pdb);
        dbstl::register_db_env(penv);
        if (p->cntnr_type == ct_vector) {
            typedef db_vector<DbstlDbt> container_t;
            container_t vi(p->pdb, penv);
            const container_t& vi0 = vi;
            srand((unsigned int)tid);
            switch (p->job) {
                case wt_read: {
                
                    while (cnt < total - 1) {
                        cnt = 0;
                        
                        try {
                            if (p->txnal)
                                dbstl::begin_txn(0, penv);
                            for (container_t::const_iterator itr = vi0.begin(); itr != vi0.end() && cnt < total - 1; 
                                itr++, cnt++) {
                                   str = (char*)((*itr).get_data());
                                   if (verbose)
                                       cout<<"\n [ "<<cnt<<" ] tid: "<<tid<<"\t [ "<< itr.get_current_index()<<" ] = "<<str.c_str();
                            }
                            if (p->txnal)
                                dbstl::commit_txn(penv);
                        } catch (DbException e) {
                            dlcnt++;
                            dbstl::abort_txn(penv);
                            if (e.get_errno() != DB_LOCK_DEADLOCK)
                                throw e;
                            else 
                                continue;
                        }
                        
                        __os_yield(NULL, 1, 0); // sleep 1 sec
                    }
                    if (iscds) // The update and delete thread will block until insert thread exits in cds mode
                        g_StopInsert = 1;
                    cout<<"\nthread "<<tid<<" got enough key/data pairs now, exiting, dead lock count = "<<dlcnt;
                    break;
                } // case wt_job:

                case wt_insert: {// writer

                    for (i = 0; g_StopInsert == 0; i++) {
                        rand_str_maker(dbt, str, strlenmin, strlenmax); 
                        
                        try {
                            if (p->txnal)
                                dbstl::begin_txn(0, penv);
                            vi.push_back(dbt);
                            if (p->txnal)
                                dbstl::commit_txn(penv);
                            if (verbose)
                                cout<<endl<<"[ "<<i<<" ] thread "<<tid<<" put a string "<<str.c_str();
                        } catch (DbException e) {
                            i--;
                            dlcnt++;
                            dbstl::abort_txn(penv);
                            if (e.get_errno() != DB_LOCK_DEADLOCK)
                                throw e;
                            else 
                                continue;
                        }
                        
                    }
                    cout<<"\n[ "<<tid<<" ] writer OK, exiting... dead lock count = "<<dlcnt;

                    break;
                
                } // case wt_insert

                case wt_delete: {
                    i = 0; // ADDITIVE, do total * 0.3 times of successful deletes, not necessarily delete so many recs because some ops may be aborted
                    while ( i < total * 0.1) {
                        
                        try {
                            if (p->txnal)
                                dbstl::begin_txn(0, penv);
                            while (i < total * 0.1) {
                                container_t::iterator itr = vi.begin();
                                itr += (ndx = rand() % total);
                                if (itr != vi.end()) {
                                    
                                    vi.erase(itr);
                                    i++;
                                    if (verbose)
                                        cout<<endl<<"vector element deleted: "<<itr.get_current_index()<<" : "<<(char*)(itr->get_data());
                                } else {
                                    if (vi.size() == 0)
                                        return;
                                    __os_yield(NULL, 0, 500000);
                                }
                            }
                            if (p->txnal)
                                dbstl::commit_txn(penv);
                        } catch (DbException e) {
                            dlcnt++;    
                            dbstl::abort_txn(penv);
                            if (e.get_errno() != DB_LOCK_DEADLOCK)
                                throw e;
                            else 
                                continue;
                        }
                        
                    }
                    cout<<"\n[ "<<tid<<" ] deleter OK, exiting... dead lock count = "<<dlcnt;
                    break;
                    
                }// case wt_delete

                case wt_update: {
                    rand_str_maker(dbt, str, strlenmin, strlenmax);
                    i = 0;// ADDITIVE
                    
                    while ( i < total * 0.5) {
                        
                        try {
                            if (p->txnal)
                                dbstl::begin_txn(0, penv);
                            while (i < total * 0.5) {
                                container_t::iterator itr = vi.begin();
                                itr += (ndx = rand() % total);
                                if (itr != vi.end()) {
                                    if (verbose)
                                        cout<<endl<<"vector element updated: "<<itr.get_current_index()<<" updated: "<<(char*)(itr->get_data())<<" ===> ";
                                    *itr = dbt;
                                    i++;
                                    if (verbose)
                                        cout<<(char*)dbt.get_data();
                                } else {
                                    if (vi.size() == 0)
                                        return;
                                    __os_yield(NULL, 0, 500000);
                                }
                            }
                            if (p->txnal)
                                dbstl::commit_txn(penv);
                        } catch (DbException e) {
                            dlcnt++;    
                            dbstl::abort_txn(penv);
                            if (e.get_errno() != DB_LOCK_DEADLOCK)
                                throw e;
                            else 
                                continue;
                        }
                        
                    }
                    cout<<"\n[ "<<tid<<" ] updater OK, exiting... dead lock count = "<<dlcnt;       
                    break;
                }; // case wt_update

            } // switch (p->job)
        } // fi
        if (p->cntnr_type == ct_map) {
            typedef db_map<ptint , DbstlDbt> container_t;
            container_t vi(p->pdb, penv);
            const container_t& vi0 = vi;
            container_t::iterator itr; 
            pair<container_t::iterator, bool> insret;
            switch (p->job) {
                case wt_read: 
                {
                    while (cnt < total - 1) {
                        cnt = 0;
                        
                        try {
                            if (p->txnal)
                                dbstl::begin_txn(0, penv);
                            for (container_t::const_iterator itrc = vi0.begin(); itrc != vi0.end(); 
                                itrc++, cnt++) {
                                   //str = (char*)((*itrc).first.get_data());
                                   str2 = (char*)((*itrc).second.get_data());
                                   if (verbose)
                                    cout<<"\n [ "<<cnt<<" ] tid: "<<tid<<'\t'<<itrc->first<<"--> "<<str2.c_str();
                            }
                            if (p->txnal)
                                dbstl::commit_txn(penv);
                        } catch (DbException e) {
                            dlcnt++;
                            dbstl::abort_txn(penv);
                            if (e.get_errno() != DB_LOCK_DEADLOCK)
                                throw e;
                            else 
                                continue;
                        }
                        
                        __os_yield(NULL, 1, 0); // sleep 1 sec
                    }
                    if (iscds) // The update and delete thread will block until insert thread exits in cds mode
                        g_StopInsert = 1;
                    cout<<"\nthread "<<tid<<" got enough key/data pairs now, exiting... dead lock count = "<<dlcnt;
                    break;
                }
                case wt_insert: 
                {// writer
                    
tag6:               for (i = 0; g_StopInsert == 0; i++) {
                        //rand_str_maker(dbt, str); 
                        rand_str_maker(dbt2, str2, strlenmin, strlenmax);
                        if (i % 100 == 0)
                            __os_yield(NULL, 0, 0); // yield cpu
                        try {
                            if (p->txnal)
                                dbstl::begin_txn(0, penv);
                            insret = vi.insert(make_pair(i, dbt2));
                            //if (insret.second == false) // fail to insert
                            //  i--;
                            if (p->txnal)
                                dbstl::commit_txn(penv);
                            if (verbose)
                                cout<<endl<<"[ "<<i<<" ] thread "<<tid<<" put a string "<<str.c_str()<<" --> "<<str2.c_str();
                        } catch (DbException e) {
                            i--;
                            dlcnt++;
                            dbstl::abort_txn(penv);
                            if (e.get_errno() != DB_LOCK_DEADLOCK)
                                throw e;
                            else 
                                goto tag6; // continue;
                        }
                        
                    }
                    cout<<"\n [ "<<tid<<" ] writer thread finishes writing, exiting... dead lock count = "<<dlcnt;
                    break;
                } //case wt_insert: 

                case wt_delete: 
                {
                    i = 0;// ADDITIVE
                    while (i < total * 0.1) {
                        
                        try {
                            if (p->txnal)
                                dbstl::begin_txn(0, penv);
                            for (; i < total * 0.1;) {
                                itr = vi.begin();
                                itr.move_to((ndx = rand() % total));
                                if (itr != vi.end()) {
                                    vi.erase(itr);
                                    i++;
                                    if (verbose)
                                        cout<<endl<<"map k/d deleted: "<<itr->first<<" : "<<(char*)(itr->second.get_data());
                                } else {
                                    if (vi.size() == 0)
                                        return;
                                    __os_yield(NULL, 0, 500000);
                                }
                            }
                            if (p->txnal)
                                dbstl::commit_txn(penv);
                        } catch (DbException e) {
                            dlcnt++;
                            dbstl::abort_txn(penv);
                            if (e.get_errno() != DB_LOCK_DEADLOCK)
                                throw e;
                            else 
                                continue;
                        }
                        
                        __os_yield(NULL, 0, 500000);
                    }
                    cout<<"\n[ "<<tid<<" ] deleter OK, exiting... dead lock count = "<<dlcnt;   
                    break;
                    
                }// case wt_delete

                case wt_update: 
                {
                    rand_str_maker(dbt, str, strlenmin, strlenmax);
                    i = 0;
                    while ( i < total * 0.5) {
                        
                        try {
                            if (p->txnal)
                                dbstl::begin_txn(0, penv);
                            for (i = 0; i < total * 0.5;) {// REAL DELETE: really update  total * 0.5 number of recs from db
                                itr = vi.begin();
                                itr.move_to(ndx =  rand() % total);
                                if (itr != vi.end()) {  
                                    if (verbose)
                                        cout<<endl<<"map key = "<<itr->first<<" updated: "<<(char*)(itr->second.get_data())<<" ===> ";
                                    itr->second = dbt;
                                    i++;
                                    if (verbose)
                                        cout<<(char*)dbt.get_data();
                                } else {
                                    if (vi.size() == 0)
                                        return;
                                    __os_yield(NULL, 0, 500000);
                                }
                            }
                            if (p->txnal)
                                dbstl::commit_txn(penv);
                        } catch (DbException e) {
                            dlcnt++;    
                            dbstl::abort_txn(penv);
                            if (e.get_errno() != DB_LOCK_DEADLOCK)
                                throw e;
                            else 
                                continue;
                        }
                        
                        __os_yield(NULL, 0, 500000);
                    }
                    cout<<"\n[ "<<tid<<" ] updater OK, exiting... dead lock count = "<<dlcnt;   
                    break;
                }; // case wt_update
            } //switch (p->job) 

        }

        if (p->cntnr_type == ct_multimap) {
            typedef db_multimap<ptint , DbstlDbt> container_t;
            container_t vi(p->pdb, penv);
            const container_t& vi0 = vi;
            
            switch (p->job) {
                case wt_read:
                {
                
                    while (cnt < total - 1) {
                        cnt = 0;
                        
                        try {
                            if (p->txnal)
                                dbstl::begin_txn(0, penv);
                            for (container_t::const_iterator itrc = vi0.begin(); itrc != vi0.end(); 
                                itrc++, cnt++) {
                                   //str = (char*)((*itrc).first.get_data());
                                   str2 = (char*)((*itrc).second.get_data());
                                   if (verbose)
                                    cout<<"\n [ "<<cnt<<" ] tid: "<<tid<<'\t'<<itrc->first<<"--> "<<str2.c_str();
                            }
                            if (p->txnal)
                                dbstl::commit_txn(penv);
                        } catch (DbException e) {
                            dlcnt++;
                            dbstl::abort_txn(penv);
                            if (e.get_errno() != DB_LOCK_DEADLOCK)
                                throw e;
                            else 
                                continue;
                        }
                        
                        __os_yield(NULL, 1, 0); // sleep 1 sec
                    }
                    if (iscds) // The update and delete thread will block until insert thread exits in cds mode
                        g_StopInsert = 1;
                    cout<<"\nthread "<<tid<<" got enough key/data pairs now, exiting... dead lock count = "<<dlcnt;
                    break;
                } //case wt_read:

                case wt_insert:
                {// writer
                    srand ((unsigned int)tid);
                    int j, dupcnt;
                    for (i = 0; g_StopInsert == 0; i += dupcnt) {
                        //rand_str_maker(dbt, str);
                        for (j = 0, dupcnt = (rand() % 5); j < dupcnt; j++) {
                                
                            rand_str_maker(dbt2, str2, strlenmin, strlenmax);
                        
                            try {
                                if (p->txnal)
                                    dbstl::begin_txn(0, penv);
                                vi.insert(make_pair(i, dbt2));
                                if (p->txnal)
                                    dbstl::commit_txn(penv);
                                if (verbose)
                                    cout<<endl<<"[ "<<i<<" ] thread "<<tid<<" put a string "<<str.c_str()<<" --> "<<str2.c_str();
                            } catch (DbException e) {
                                i--;
                                dlcnt++;
                                dbstl::abort_txn(penv);
                                if (e.get_errno() != DB_LOCK_DEADLOCK)
                                    throw e;
                                else 
                                    continue;
                            }
                            
                        }
                    } // there may be more than total number of recs inserted because of the dupcnt
                    cout<<"\n[ "<<tid<<" ] writer thread finishes writing, exiting... dead lock count = "<<dlcnt;
                    break;
                }   //case wt_insert:

                case wt_delete: 
                {
                    i = 0;// ADDITIVE
                    while ( i < total * 0.1) {
                        
                        try {
                            if (p->txnal)
                                dbstl::begin_txn(0, penv);
                            for (container_t::iterator itr = vi.begin(); i < total * 0.1;) {
                                
                                itr.move_to(ndx = rand() % total);
                                if (itr != vi.end()) {
                                
                                    vi.erase(itr);
                                    i++;
                                    if (verbose)
                                        cout<<endl<<"multimap k/d deleted: "<<itr->first<<" : "<<(char*)(itr->second.get_data());
                                    
                                } else {
                                    if (vi.size() == 0)
                                        return;
                                    __os_yield(NULL, 0, 500000);
                                }
                            }
                            if (p->txnal)
                                dbstl::commit_txn(penv);
                        } catch (DbException e) {
                            dlcnt++;    
                            dbstl::abort_txn(penv);
                            if (e.get_errno() != DB_LOCK_DEADLOCK)
                                throw e;
                            else 
                                continue;
                        }
                        
                        __os_yield(NULL, 0, 500000);
                    }
                    cout<<"\n[ "<<tid<<" ] deleter OK, exiting... dead lock count = "<<dlcnt;   
                    break;
                    
                }// case wt_delete

                case wt_update: 
                {
                    rand_str_maker(dbt, str, strlenmin, strlenmax);
                    i = 0;// ADDITIVE
                    while (i < total * 0.5) {
                        
                        try {
                            if (p->txnal)
                                dbstl::begin_txn(0, penv);
                            for (container_t::iterator itr = vi.begin(); i < total * 0.5;) {
                                 
                                itr.move_to(ndx = rand() % total);
                                if (itr != vi.end()) {
                                    if (verbose)
                                        cout<<endl<<"multimap key = "<<itr->first<<" updated: "<<(char*)(itr->second.get_data())<<" ===> ";
                                    itr->second = dbt;
                                    i++;
                                    if (verbose)
                                        cout<<(char*)dbt.get_data();
                                    
                                } else {
                                    if (vi.size() == 0)
                                        return;
                                    __os_yield(NULL, 0, 500000);
                                }
                            }
                            if (p->txnal)
                                dbstl::commit_txn(penv);
                        } catch (DbException e) {
                            dlcnt++;        
                            dbstl::abort_txn(penv);
                            if (e.get_errno() != DB_LOCK_DEADLOCK)
                                throw e;
                            else 
                                continue;
                        }
                        
                        __os_yield(NULL, 0, 500000);
                        
                    }
                    cout<<"\n[ "<<tid<<" ] updater OK, exiting... dead lock count = "<<dlcnt;   
                    break;

                } // case wt_update
            } // switch (p->job) 
        }

        if (p->cntnr_type == ct_set) {
            typedef db_set<DbstlDbt> container_t;
            container_t vi(p->pdb, penv);
            const container_t &vi0 = vi;
            container_t::iterator itr;
            int rowcnt;
            int j;
            switch (p->job) {
                case wt_read:
                {
                
                    while (cnt < total - 1) {
                        cnt = 0;
                        
                        try {
                            if (p->txnal)
                                dbstl::begin_txn(0, penv);
                            for (container_t::const_iterator itrc = vi0.begin(); itrc != vi0.end(); 
                                itrc++, cnt++) {
                                   str = (char*)((*itrc).get_data());
                                if (verbose)
                                     cout<<"\n [ "<<cnt<<" ] tid: "<<tid<<'\t'<<str.c_str();
                            }
                            if (p->txnal)
                                dbstl::commit_txn(penv);
                        } catch (DbException e) {
                            dlcnt++;
                            dbstl::abort_txn(penv);
                            if (e.get_errno() != DB_LOCK_DEADLOCK)
                                throw e;
                            else 
                                continue;
                        }
                        
                        __os_yield(NULL, 1, 0); // sleep 1 sec
                    }
                    if (iscds) // The update and delete thread will block until insert thread exits in cds mode
                        g_StopInsert = 1;
                    cout<<"\nthread "<<tid<<" got enough key/data pairs now, exiting... dead lock count = "<<dlcnt;
                    break;
                } 
                case wt_insert: 
                {// writer
                    srand ((unsigned int)tid);
            
                    pair<container_t::iterator, bool> insret;
                    for (i = 0; g_StopInsert == 0; i += 1) {
                    
                        rand_str_maker(dbt, str, strlenmin, strlenmax); 
                        
                        try {
                            if (p->txnal)
                                dbstl::begin_txn(0, penv);
                            insret = vi.insert(dbt);
                            if (p->txnal)
                                dbstl::commit_txn(penv);
                            if (insret.second == false) // fail to insert
                                i--;
                            if (verbose)
                                cout<<endl<<"[ "<<i<<" ] thread "<<tid<<" put a string "<<str.c_str();
                        } catch (DbException e) {
                            i--;
                            dlcnt++;
                            dbstl::abort_txn(penv);
                            if (e.get_errno() != DB_LOCK_DEADLOCK)
                                throw e;
                            else 
                                continue;
                        }
                        
                        
                    } // there may be more than total number of recs inserted because of the dupcnt
                    cout<<"\n[ "<<tid<<" ] writer thread finishes writing, exiting... dead lock count = "<<dlcnt;
                    break;
                } // case wt_insert: 

                case wt_delete: 
                {
                    i = 0;
                    j = 0;
                    while (i < total * 0.1) {
                        
                        try {
                            if (p->txnal)
                                dbstl::begin_txn(0, penv);
                            for ( ; i < total * 0.1;) {// ADDITIVE
                                rowcnt = rand() % total;
                                for (itr = vi.begin(), j = 0; j < rowcnt; j++, itr++)
                                    ; // move to a random pos
                                if (itr != vi.end()) {
                                    
                                    vi.erase(itr);
                                    i++;
                                    if (verbose)
                                        cout<<endl<<"set key deleted: "<<(char*)(itr->get_data());
                            
                                } else {
                                    if (vi.size() == 0)
                                        return;
                                    __os_yield(NULL, 0, 500000);
                                }
                            }
                            if (p->txnal)
                                dbstl::commit_txn(penv);
                        } catch (DbException e) {
                            dlcnt++;
                            dbstl::abort_txn(penv);
                            if (e.get_errno() != DB_LOCK_DEADLOCK)
                                throw e;
                            else 
                                continue;
                        }
                        
                        __os_yield(NULL, 0, 500000);
                    }   
                    cout<<"\n[ "<<tid<<" ] deleter OK, exiting... dead lock count = "<<dlcnt;   
                    break;
                    
                }// case wt_delete

                case wt_update: 
                {
                    rand_str_maker(dbt, str, strlenmin, strlenmax);
                    i = 0;
                    while ( i < total * 0.5) {
                        
                        try {
                            if (p->txnal)
                                dbstl::begin_txn(0, penv);
                            for (i = 0; i < total * 0.5;) { // REAL UPDATE
                                rowcnt = rand() % total;
                                
                                for (itr = vi.begin(), j = 0; j < rowcnt; j++, itr++)
                                    ; // move to a random pos
                                if (itr != vi.end()) {
                                    if (verbose)
                                        cout<<endl<<"set key updated: "<<(char*)(itr->get_data())<<" ===> ";
                                    *itr = dbt;
                                    i++;
                                    if (verbose)
                                        cout<<(char*)dbt.get_data();
                                
                                } else {
                                    if (vi.size() == 0)
                                        return;
                                    __os_yield(NULL, 0, 500000);
                                }
                            }
                            if (p->txnal)
                                dbstl::commit_txn(penv);
                        } catch (DbException e) {
                            dlcnt++;
                            dbstl::abort_txn(penv);
                            if (e.get_errno() != DB_LOCK_DEADLOCK)
                                throw e;
                            else 
                                continue;
                        }
                        
                        __os_yield(NULL, 0, 500000);
                    }
                    cout<<"\n[ "<<tid<<" ] updater OK, exiting... dead lock count = "<<dlcnt;   
                    break;
                }// case wt_update
            } // switch (p->job) 
        } // fi

        if (p->cntnr_type == ct_multiset) {
            typedef db_multiset<DbstlDbt> container_t;
            container_t::iterator itr;
            int rowcnt;
            int j, dupcnt;
            container_t vi(p->pdb, penv);
            const container_t &vi0 = vi;
            switch (p->job) {
                case wt_read:
                {
                
                    while (cnt < total - 1) {
                        cnt = 0;
                        
                        try {
                            if (p->txnal)
                                dbstl::begin_txn(0, penv);
                            for (container_t::const_iterator itrc = vi0.begin(); itrc != vi0.end(); 
                                itrc++, cnt++) {
                                   str = (char*)((*itrc).get_data());
                                   if (verbose)
                                    cout<<"\n [ "<<cnt<<" ] tid: "<<tid<<'\t'<<str.c_str();
                            }
                            if (p->txnal)
                                dbstl::commit_txn(penv);
                        } catch (DbException e) {
                            dlcnt++;
                            dbstl::abort_txn(penv);
                            if (e.get_errno() != DB_LOCK_DEADLOCK)
                                throw e;
                            else 
                                continue;
                        }
                        
                        __os_yield(NULL, 1, 0); // sleep 1 sec
                    }
                    if (iscds) // The update and delete thread will block until insert thread exits in cds mode
                        g_StopInsert = 1;
                    cout<<"\nthread "<<tid<<" got enough key/data pairs now, exiting... dead lock count = "<<dlcnt;
                    break;
                } //  case wt_read:
                case wt_insert:
                {// writer
                    srand ((unsigned int)tid);
                    
                    for (i = 0; g_StopInsert == 0; i += dupcnt) {
                        
                        for (j = 0, dupcnt = (rand() % 5); j < dupcnt; j++) {
                            rand_str_maker(dbt, str, strlenmin, strlenmax); 
                            
                            try {
                                if (p->txnal)
                                    dbstl::begin_txn(0, penv);
                                vi.insert(dbt);
                                if (p->txnal)
                                    dbstl::commit_txn(penv);
                                if (verbose)
                                    cout<<endl<<"[ "<<i<<" ] thread "<<tid<<" put a string "<<str.c_str();
                            } catch (DbException e) {
                                i--;
                                dlcnt++;
                                dbstl::abort_txn(penv);
                                if (e.get_errno() != DB_LOCK_DEADLOCK)
                                    throw e;
                                else 
                                    continue;
                            }
                            
                        }
                    } // there may be more than total number of recs inserted because of the dupcnt
                    cout<<"\n[ "<<tid<<" ] writer thread finishes writing, exiting... dead lock count = "<<dlcnt;
                    break;
                } // case wt_insert:
                case wt_delete: 
                {
                    i = 0;
                    j = 0;
                    while (i < total * 0.1) {
                        
                        try {
                            if (p->txnal)
                                dbstl::begin_txn(0, penv);
                            for (; i < total * 0.1;) {// ADDITIVE
                                rowcnt = rand() % total;
                                for (itr = vi.begin(), j = 0; j < rowcnt; j++, itr++)
                                    ; // move to a random pos
                                if (itr != vi.end()) {
                                    
                                    vi.erase(itr);
                                    i++;
                                    if (verbose)
                                        cout<<endl<<"multiset key deleted: "<<(char*)(itr->get_data());
                                
                                } else {
                                    if (vi.size() == 0)
                                        return;
                                    __os_yield(NULL, 0, 500000);
                                }
                            }
                            if (p->txnal)
                                dbstl::commit_txn(penv);
                        } catch (DbException e) {
                            dlcnt++;    
                            dbstl::abort_txn(penv);
                            if (e.get_errno() != DB_LOCK_DEADLOCK)
                                throw e;
                            else 
                                continue;
                        }
                        
                        __os_yield(NULL, 0, 500000);
                    }
                    cout<<"\n[ "<<tid<<" ] deleter OK, exiting... dead lock count = "<<dlcnt;   
                    break;
                    
                }// case wt_delete

                case wt_update: 
                {
                    rand_str_maker(dbt, str, strlenmin, strlenmax);
                    i = 0;
                    while ( i < total * 0.5) {
                        
                        try {
                            if (p->txnal)
                                dbstl::begin_txn(0, penv);
                            for (i = 0; i < total * 0.5;) {// REAL UPDATE
                                rowcnt = rand() % total;
                                for (itr = vi.begin(), j = 0; j < rowcnt; j++, itr++)
                                    ; // move to a random pos
                                if (itr != vi.end()) {
                                    if (verbose)
                                        cout<<endl<<"multiset key updated: "<<(char*)(itr->get_data())<<" ===> ";
                                    *itr = dbt;
                                    i++;
                                    if (verbose)
                                        cout<<(char*)dbt.get_data();
                                
                                } else {
                                    if (vi.size() == 0)
                                        return;
                                    __os_yield(NULL, 0, 500000);
                                }
                            }
                            if (p->txnal)
                                dbstl::commit_txn(penv);
                        } catch (DbException e) {
                            dlcnt++;        
                            dbstl::abort_txn(penv);
                            if (e.get_errno() != DB_LOCK_DEADLOCK)
                                throw e;
                            else 
                                continue;
                        }
                        
                        __os_yield(NULL, 0, 500000);
                    }
                    cout<<"\n[ "<<tid<<" ] updater OK, exiting... dead lock count = "<<dlcnt;   
                    break;
                } // case wt_update
            } //switch (p->job)
        }// fi

    }// void operator()()
}; // WorkerThread

// Used in the multi-threaded test to manage test threads,
// works like boost::thread_group
class ThreadMgr
{
public:
    os_thread_t create_thread(WorkerThread *thrd);
    void clear() { threads.clear();}
    void join_all();
    ThreadMgr() : threads(0){}
private:
    vector<os_thread_t> threads;
}; // ThreadMgr

#ifdef WIN32
#define ThrdFuncRet DWORD
#define FunctRet return (0)
#else
#define ThrdFuncRet void*
#define FunctRet return (NULL)
#endif

static ThrdFuncRet thread_func(void *arg)
{
    (*((WorkerThread *)arg)).run();
    FunctRet;
}

os_thread_t ThreadMgr::create_thread(WorkerThread *thrd)
{
    os_thread_t thrdhandle;

    os_thread_create(&thrdhandle, NULL, thread_func, thrd);
    threads.push_back(thrdhandle);
    return thrdhandle;
}

void ThreadMgr::join_all()
{
    ThrdFuncRet ret;

    for (size_t i = 0; i < threads.size(); i++)
        os_thread_join(threads[i], &ret);
}

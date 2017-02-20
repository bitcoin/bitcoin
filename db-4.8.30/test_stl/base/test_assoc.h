/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$ 
 */

#include "test.h"
// This function tests all member functions in the four assocative 
// containers and their iterators. The containers are db_map,
// db_multimap, db_set, db_multiset . They are equivalent to std::map, 
// hash_map, std::multimap, hash_multimap, std::set, hash_set, 
// std::multiset, hash_multiset respectively---passing btree db handles,
// they are equivalent to std assocative container; passing hash db handles
// they are hash associative containers
//
// This function tests the classes also in pair with std/hash counterparts,
// but the result of some operations are checked not by comparing contents
// after operation, but by self-check, often use another member function of
// the same class to verify. Also, only the four container types in std is
// used, the hash containers are not standarized, so not used, although 
// hash db type can be passed in, so the db_containers can be of hash type
//
// Most of the algorithms in std are targeting at a sequential range, while
// ranges in associative containers are not, so only some read only 
// algorithms are applied to the db_map container.
//
// This function also tests some advanced features, like using containers
// with secondary db, bulk retrieval, varying length records storage/load,
// storing native char*/wchar_t* strings, etc
// 

static int hcmp_def(Db *pdb, const Dbt *dbt1, const Dbt *dbt2)
{
    u_int32_t sz1 = dbt1->get_size(), sz2 = dbt2->get_size();

    int ret = memcmp(dbt1->get_data(), dbt2->get_data(), sz1 < sz2 ? sz1 : sz2);
    ret = (ret == 0) ? (sz1 - sz2) : ret;

    return ret;
}

class TestAssoc
{
public:
    typedef map<int, int> m_int_t;
    typedef ptint tpint;
    ~TestAssoc();
    TestAssoc(void *param1);
    void start_test()
    {
        tb.begin("db_map");
        test_map_member_functions();
        test_map_std_functions();
        test_hot_dbclose();
        test_arbitrary_object_storage();
        test_storing_std_strings();
        test_secondary_containers();
        tb.end();

        tb.begin("db_multimap");
        test_mmap_member_functions();
        tb.end();

        tb.begin("db_set");
        test_set_member_functions();
        tb.end();

        tb.begin("db_multiset");
        test_mset_member_functions();
        tb.end();

        tb.begin("Special functionalities of all dbstl containers.");
        test_char_star_string_storage();
        test_fixed_len_obj_storage();
        test_arbitray_sequence_storage();
        test_bulk_retrieval_read();
        test_nested_txns();
        test_etc();
        tb.end();

    
    }
private:
    // Test all member functions of db_map<>
    void test_map_member_functions();

    // Use std functions to manipulate db_map containers.
    void test_map_std_functions();

    // Close a live container's db handle then open it again and 
    // reassign to the same container, verify the container works.
    void test_hot_dbclose();

    // Use two ways to store an object of arbitrary length. The object
    // contains some varying length members, char* string for example.
    void test_arbitrary_object_storage();

    void test_storing_std_strings();

    // Open a secondary db H and associate it with an exisitng db handle
    // which is being used by a container C1, then use H to create another
    // container C2, verify we can get C1's data via C2.
    // This technique works for all types of db and containers.
    void test_secondary_containers();

    // Test all member functions of db_multimap<>.
    void test_mmap_member_functions();

    // Test all member functions of db_set<>.
    void test_set_member_functions();

    // Test all member functions of db_multiset<>.
    void test_mset_member_functions();

    // Test direct storage of char* strings.
    void test_char_star_string_storage();

    // Testing storage of fixed length objects.
    void test_fixed_len_obj_storage();

    // Testing storage of arbitrary element type of sequence.
    void test_arbitray_sequence_storage();

    // Testing reading with bulk retrieval flag.
    void test_bulk_retrieval_read();

    // Testing nested transaction implementation.
    void test_nested_txns();

    // Testing miscellaneous functions.
    void test_etc();

    int flags, setflags, EXPLICIT_TXN, TEST_AUTOCOMMIT, n;
    DBTYPE dbtype;
    dm_int_t::difference_type oddcnt;
    Db *dmdb1, *dmdb2, *dmmdb1, *dmmdb2, *dmsdb1, 
        *dmsdb2, *dmmsdb1, *dmmsdb2, *dbstrv;
    Db *dbp3;
    Db *dmdb6;
    Db *dbp3sec;
    Db *dmmdb4, *dbstrmap;
    Db *dmstringdb;
    DbEnv *penv;
    u_int32_t dboflags;

    test_block tb;
    map<int, int> m1;
    multimap<int, int> mm1;
};

TestAssoc::~TestAssoc()
{
    dbstl::close_db_cursors(this->dmsdb2);
    dbstl::close_db_cursors(this->dmdb1);
    dbstl::close_db_cursors(this->dmdb2);
    dbstl::close_db_cursors(this->dmdb6);
    dbstl::close_db_cursors(this->dmmdb1);
}

TestAssoc::TestAssoc(void *param1)
{
    check_expr(param1 != NULL);
    TestParam *param = (TestParam*)param1;
    TestParam *ptp = param;

    flags = 0, setflags = 0, EXPLICIT_TXN = 1, TEST_AUTOCOMMIT = 0;
    dbtype = DB_BTREE;
    penv = param->dbenv;
    dmdb1 = dmdb2 = dmmdb1 = dmmdb2 = dmsdb1 = dmsdb2 = dmmsdb1 = 
        dmmsdb2 = dbstrv = NULL;
    flags = param->flags;
    dbtype = param->dbtype;
    setflags = param->setflags;
    TEST_AUTOCOMMIT = param->TEST_AUTOCOMMIT;
    EXPLICIT_TXN = param->EXPLICIT_TXN;
    dboflags = ptp->dboflags;
    n = 10;

    dmdb1 = dbstl::open_db(penv, "db_map.db", 
        dbtype, DB_CREATE | ptp->dboflags, 0);
    dmdb2 = dbstl::open_db(penv, "db_map2.db", 
        dbtype, DB_CREATE | ptp->dboflags, 0);
    dmdb6 = dbstl::open_db(penv, "db_map6.db", 
        dbtype, DB_CREATE | ptp->dboflags, 0);

    dmmdb1 = dbstl::open_db(penv, 
        "db_multimap.db", dbtype, DB_CREATE | ptp->dboflags, DB_DUP);
    dmmdb2 = dbstl::open_db(penv, 
        "db_multimap2.db", dbtype, DB_CREATE | ptp->dboflags, DB_DUP);
    
    dmsdb1 = dbstl::open_db(penv, "db_set.db", 
        dbtype, DB_CREATE | ptp->dboflags, 0);
    dmsdb2 = dbstl::open_db(penv, "db_set2.db", 
        dbtype, DB_CREATE | ptp->dboflags, 0);
    
    dmmsdb1 = dbstl::open_db(penv, 
        "db_multiset.db", dbtype, DB_CREATE | ptp->dboflags, DB_DUP);
    dmmsdb2 = dbstl::open_db(penv, 
        "db_multiset2.db", dbtype, DB_CREATE | ptp->dboflags, DB_DUP);

    dbstrv = dbstl::open_db(penv, "dbstr.db", 
        DB_RECNO, DB_CREATE | ptp->dboflags, DB_RENUMBER);
    dbp3sec = dbstl::open_db(penv, "db_map_sec.db", 
        dbtype, DB_CREATE | ptp->dboflags, DB_DUP);

    dmmdb4 = dbstl::open_db(penv, 
        "db_multimap4.db", dbtype, DB_CREATE | dboflags, DB_DUPSORT);
    dbstrmap = dbstl::open_db(penv, "dbstrmap.db", 
        DB_BTREE, DB_CREATE, 0);

    dmstringdb = dbstl::open_db(penv, "db_map_stringdb.db", 
        dbtype, DB_CREATE | dboflags, 0);
    dbp3 = NULL;
    
}

void TestAssoc::test_map_member_functions()
{
    int i;

    if (EXPLICIT_TXN)
        dbstl::begin_txn(0, penv);
    
    dm_int_t dm1(dmdb1, penv);
    const dm_int_t& cnstdm1 = dm1;
    dm_int_t simple_map;
    map<ptint, ptint> ssimple_map;
    map<ptint, ptint>::iterator sitr, sitr1;

    for (i = 0; i < n; i++) {
        dm1[i] = ptint(i);
        ssimple_map[i] = ptint(i);
    }

    for (i = 0; i < n; i++) {
        dm_int_t::const_iterator citr, citr1;
        dm_int_t::iterator itr;

        citr = dm1.find(i);
        check_expr(citr->second == dm1.find(i)->second);
        itr = citr;
        check_expr(*citr == *itr);
        itr->second = i * 2 + 1;
        citr1 = itr;
        check_expr(*citr == *itr);
        check_expr(*citr == *citr1);
        check_expr(citr->second == dm1.find(i)->second);
    }

    for (i = 0; i < n; i++) {
        dm_int_t::const_iterator citr = dm1.find(i);
        check_expr(citr->second == dm1.find(i)->second);
        dm_int_t::iterator itr = citr;
        check_expr(*citr == *itr);
        itr->second = i * 2 + 1;
        dm_int_t::const_iterator citr1 = itr;
        check_expr(*citr == *itr);
        check_expr(*citr == *citr1);
        check_expr(citr->second == dm1.find(i)->second);
    }

    for (i = 0; i < n; i++) {
        dm_int_t::iterator ncitr, ncitr1;
        dm_int_t::const_iterator citr, citr1;

        ncitr = cnstdm1.find(i);
        check_expr(ncitr->second == cnstdm1.find(i)->second);
        citr = ncitr;
        check_expr(*citr == *ncitr);
        //*ncitr = i * 2 + 1;
        citr1 = ncitr;
        ncitr1 = citr1;
        check_expr(*citr == *ncitr);
        check_expr(*citr == *ncitr1);
        check_expr(*citr == *citr1);
        check_expr(citr->second == cnstdm1.find(i)->second);
    }

    for (i = 0; i < n; i++) {

        dm_int_t::iterator ncitr = cnstdm1.find(i);
        check_expr(ncitr->second == cnstdm1.find(i)->second);
        dm_int_t::const_iterator citr = ncitr;
        check_expr(*citr == *ncitr);
        //*itr = i * 2 + 1;
        dm_int_t::const_iterator citr1 = ncitr;
        dm_int_t::iterator ncitr1 = citr1;

        check_expr(*citr == *ncitr);
        check_expr(*citr == *ncitr1);
        check_expr(*citr == *citr1);
        check_expr(citr->second == cnstdm1.find(i)->second);
    }

    if (dm1.is_hash() == false)
    {
        dm1.clear();
        ssimple_map.clear();
        for (i = 0; i < n; i++) {
            dm1[i] = ptint(i);
            ssimple_map[i] = ptint(i);
        }
        dm_int_t::iterator ncitr = dm1.end();
        dm_int_t::iterator ncrend = --dm1.begin();
        map<ptint, ptint>::iterator sitr = ssimple_map.end();
        for (sitr--, --ncitr; ncitr != ncrend; ncitr--, sitr--) {
            ncitr.refresh(true); // not needed because by default we are using direct db get, only for test purpose.
            check_expr(*sitr == *ncitr);
        }
        ncitr.close_cursor();

        const dm_int_t&cnstdm1 = dm1;
        dm_int_t::const_iterator citr = cnstdm1.end();
        dm_int_t::const_iterator crend = --cnstdm1.begin();
        map<ptint, ptint>::iterator sitr2;
        map<ptint, ptint>::reverse_iterator rsitr;
        
        sitr = ssimple_map.end();
        for (sitr--, --citr; citr!= crend; sitr--, citr--) {
            citr.refresh(true); // not needed because by default we are using direct db get, only for test purpose.
            check_expr(*sitr == *citr);
        }

        for (sitr2 = --ssimple_map.begin(), citr = crend; citr != cnstdm1.end();) {
            citr++;
            sitr2++;
            check_expr(*sitr2 == *citr);
        }

        simple_map.insert(simple_map.begin(), *ssimple_map.begin());
        check_expr(*simple_map.begin() == *ssimple_map.begin());
        crend = --cnstdm1.begin();
        --(--crend);
        ++crend;
        check_expr(*crend == *cnstdm1.begin());
        citr.close_cursor();

        simple_map.clear();
        simple_map.insert(cnstdm1.begin(), cnstdm1.end());
        rsitr = ssimple_map.rbegin();
        for (dm_int_t::reverse_iterator itrr1 = simple_map.rbegin(); itrr1 != simple_map.rend(); ++itrr1, ++rsitr)
            check_expr(*itrr1 == *rsitr);

        const dm_int_t &csimple_map = simple_map;
        rsitr = ssimple_map.rbegin();
        for (dm_int_t::const_reverse_iterator citrr1 = csimple_map.rbegin(); citrr1 != csimple_map.rend(); ++citrr1, ++rsitr)
            check_expr((citrr1->first == rsitr->first) && (citrr1->second == rsitr->second));

        for (sitr = ssimple_map.begin(); sitr != ssimple_map.end(); ++sitr)
            check_expr(csimple_map[sitr->first] == sitr->second);

        simple_map.erase(simple_map.end());
        check_expr(simple_map.size() == ssimple_map.size());
        check_expr(csimple_map.find(123) == csimple_map.end());

        check_expr(*csimple_map.lower_bound(1) == *ssimple_map.lower_bound(1));
        check_expr(*csimple_map.upper_bound(5) == *ssimple_map.upper_bound(5));
        pair<dm_int_t::const_iterator, dm_int_t::const_iterator> cnsteqrg = csimple_map.equal_range(3);
        check_expr(cnsteqrg.first->first == 3 && cnsteqrg.second->first == ssimple_map.equal_range(3).second->first);

    }
    dm1.clear();
    ssimple_map.clear();

    if (!dm1.is_hash()) {
        for (i = 0; i < n; i++) {
            dm1[i] = ptint(i);
            ssimple_map[i] = ptint(i);
        }
        dm_int_t::iterator itr, itr1;

        itr = dm1.find(3);
        itr1 = dm1.find(8);
        sitr = ssimple_map.find(3);
        sitr1 = ssimple_map.find(8);

        dm1.erase(itr, itr1);
        ssimple_map.erase(sitr, sitr1);
        pprint(dm1, "dm1 after erasing range: ");
        check_expr(is_equal(dm1, ssimple_map));
        dm1.clear();
        ssimple_map.clear();
    }
    for (i = 0; i < 10; i++) {
        simple_map.insert(make_pair(ptint(i), ptint(i)));
        ssimple_map.insert(make_pair(ptint(i), ptint(i)));
    }
    for (i = 0; i < 10; i++)
        check_expr(simple_map[i] == ssimple_map[i]);

    dm1.clear();
    // db_map<>::empty
    check_expr(dm1.empty());
    fill(dm1, m1, i = 3, n = 5);
    check_expr(!dm1.empty());
    
    dm_int_t::iterator dmi, dmi2;
    m_int_t::iterator mi;
    dm_int_t::reverse_iterator dmri;
    m_int_t::reverse_iterator mri;
    ptint ptmp;
    int itmp;


    // db_map<>::find, count
    for (dmi = dm1.begin(), mi = m1.begin(), i = 3; dmi != dm1.end() && 
        mi !=m1.end(); dmi++, mi++, i++) {
        // check_expr both contain
        check_expr((dm1.find(i) != dm1.end()) && 
            (m1.find(i) != m1.end()));
        
        pair<dm_int_t::iterator, dm_int_t::iterator> erp = 
            dm1.equal_range(i);
        int jc = 0;
        dm_int_t::iterator jj;
        for (jj = erp.first, jc = 0; 
            jj != erp.second; jj++, jc++)
            check_expr((*jj).second == ptint(i));
        check_expr(jc == 1);
        if (i < 7 && !dm1.is_hash()) {// 7 is last element
            check_expr((*(dm1.upper_bound(i))).second == i + 1);
            check_expr((*(dm1.lower_bound(i))).second == i);
        }
        else if (i == 7 && !dm1.is_hash()) {
            check_expr(dm1.upper_bound(i) == dm1.end());
            check_expr((*(dm1.lower_bound(i))).second == i);
        } else if (!dm1.is_hash())
            check_expr(false);

        check_expr(dm1.count(i) == 1);
        check_expr(dm1.count(-i) == 0);
        check_expr((ptmp = dm1[i]) == (itmp = m1[i]));

        // order of elements in hash can not be expected
        if (!dm1.is_hash()) 
            check_expr((*dmi).second == (*mi).second);
        if (i == 3 + n - 1) {// last element
            
            for (; dmi != dm1.end(); mi--, i--) {
                // check_expr both contains
                check_expr((*(dm1.find(i))).second == i);
                check_expr((ptmp = dm1[i]) == (itmp = m1[i]));
                // order of elements in hash can not be 
                // expected
                if (!dm1.is_hash()) 
                    check_expr((*dmi).second == (*mi).second);
                if (i % 2)
                    dmi--;
                else 
                    --dmi;
            }
            break;
        }
        
    }
    for (dmri = dm1.rbegin(), mri = m1.rbegin(), i = 3; 
        dmri != dm1.rend() && mri != m1.rend(); dmri++, mri++, i++) {
        check_expr((dm1.find(i) != dm1.end()) && 
            (m1.find(i) != m1.end()));// check_expr both contain

        check_expr(dm1[i] == m1[i]);    
        if (!dm1.is_hash())
            check_expr((*dmri).second == (*mri).second);
        if (i == 3 + n - 1) {// last element
            
            for (; dmri != dm1.rend(); mri--, i--) {
                // check_expr both contain
                check_expr((*(dm1.find(i))).second == i);
                // order of elements in hash can not be expected
                if (!dm1.is_hash()) 
                    check_expr((*dmri).second == (*mri).second);
                check_expr((ptmp = dm1[i]) == (itmp = m1[i]));
                if (i % 2)
                    dmri--;
                else
                    --dmri;
                if (i == 3)
                    break;
            }
            break;
        }
        
    }
    for (dmi = dm1.begin(), mi = m1.begin(), i = 3; dmi != dm1.end() && 
        mi !=m1.end(); ++dmi, mi++)
        i++;
    check_expr(i == 3 + n);// i must have been incremented to 8
    for (dmri = dm1.rbegin(), mri = m1.rbegin(), i = 3; 
        dmri != dm1.rend() && mri !=m1.rend(); ++dmri, mri++)
        i++;
    check_expr(i == 3 + n);// i must have been incremented to 8
        
    
    if (EXPLICIT_TXN)
        commit_txn(penv);
    if (!TEST_AUTOCOMMIT)
        begin_txn(0, penv);
    dm_int_t dm2(dmdb2, penv);
    dm2.clear();
    if (!TEST_AUTOCOMMIT)
        commit_txn(penv);
    if (EXPLICIT_TXN)
        begin_txn(0, penv);
    dm2.insert(dm1.begin(), dm1.end());
    if (EXPLICIT_TXN)
        commit_txn(penv);
    if (!TEST_AUTOCOMMIT)
        begin_txn(0, penv);
    dm_int_t dm3 = dm2;
    if (!TEST_AUTOCOMMIT)
        commit_txn(penv);
    if (EXPLICIT_TXN)
        begin_txn(0, penv);
    check_expr(dm3 == dm2);
    if (EXPLICIT_TXN)
        commit_txn(penv);
    if (!TEST_AUTOCOMMIT)
        begin_txn(0, penv);
    dm3 = dm1;
    if (!TEST_AUTOCOMMIT)
        commit_txn(penv);
    if (EXPLICIT_TXN)
        begin_txn(0, penv);
    check_expr(dm3 == dm1);
    // this test case should be fine for hash because hash config is 
    // identical in dm1 and dm2
    for (dmi = dm1.begin(), dmi2 = dm2.begin(); dmi != dm1.end() && 
        dmi2 != dm2.end(); ++dmi, dmi2++)
        check_expr(*dmi == *dmi2);
    int arr1[] = {33, 44, 55, 66, 77};
    
    for (dmi = dm1.begin(), i = 0; dmi != dm1.end(); dmi++, i++)
        (*dmi).second = tpint(arr1[i]);

    for (dmi = dm1.begin(), dmi2 = dm2.begin(), i = 0; 
        dmi != dm1.end() && dmi2 != dm2.end(); dmi++, i++, dmi2++) {
        check_expr((*dmi).second == tpint(arr1[i]));
        dmi2->second = dmi->second;
        check_expr(*dmi == *dmi2);
    }
    // db_map<>::insert(const value_type&). the range insert is already 
    // tested in fill
    //
    pair<dm_int_t::iterator, bool> res = 
        dm1.insert(make_pair(3, tpint(33)));
    check_expr((*(res.first)).first == 3 && res.second == false);
    // we don't know which value is assigned to key 3 on hash
    if (!dm1.is_hash()) 
        check_expr((*(res.first)).second == 33);

    // db_map<>::count, insert, erase, find
    check_expr(dm1.count(3) == 1);
    check_expr(dm1.size() == (size_t)n);// n is 5
    check_expr(dm1.count(9) == 0);
    res = dm1.insert(make_pair(9, tpint(99)));
    check_expr((*(res.first)).second == 99 && (*(res.first)).first == 9 && 
        res.second == true);
    check_expr(dm1.count(9) == 1);
    check_expr(dm1.size() == (size_t)n + 1);

    if (EXPLICIT_TXN)
        dbstl::commit_txn(penv);
    
    if (!TEST_AUTOCOMMIT)
        dbstl::begin_txn(0, penv);
    dm1.erase(9);
    if (!TEST_AUTOCOMMIT)
        commit_txn(penv);
    if (EXPLICIT_TXN)
        begin_txn(0, penv);

    check_expr(dm1.size() == (size_t)n);
    check_expr(dm1.count(9) == 0);
    dm1.erase(dm1.find(3));
    check_expr(dm1.size() == (size_t)n - 1);
    check_expr(dm1.count(3) == 0);
    dm2.erase(dm2.begin(), dm2.end());
    check_expr(dm2.size() == 0);
    check_expr(dm2.empty());

    dmi = dm1.begin();
    dmi++;
    dmi2 = dmi;
    dmi2++;
    dmi2++;
    
    if (dm1.is_hash()) {
        check_expr(dm1.key_eq()(3, 4) == false);
        check_expr(dm1.key_eq()(3, 3) == true);
        check_expr(dm1.bucket_count() != 0);
    } else
        check_expr(dm1.key_comp()(3, 4));

    if (dm1.is_hash()) {
        check_expr(dm1.key_eq()(3, 4) == false);
        check_expr(dm1.key_eq()(3, 3) == true);
    } else {
        check_expr(dm1.key_comp()(3, 4));
    }

    check_expr(dm1.value_comp()(*dmi, *dmi2));
    if (dm1.is_hash())
        cout<<"hash value for key = 3 is: "
            <<dm1.hash_funct()(3);

    dm2.insert(dmi, dmi2);// 2 recs inserted, [dmi, dmi2)
    check_expr(dm2.size() == 2);
    for (dmi = dm1.begin(); dmi != dm1.end(); dmi++)
        cout<<'\t'<<dmi->first<<'\t'<<dmi->second<<endl;
    if (EXPLICIT_TXN)
        commit_txn(penv);

    if (!TEST_AUTOCOMMIT) {
        begin_txn(0, penv);
    }
    dm1.swap(dm2);
    if (!TEST_AUTOCOMMIT) {
        commit_txn(penv);
    }
    if (EXPLICIT_TXN)
        begin_txn(0, penv);
    size_t dm1sz, dm2sz;
    check_expr((dm1sz = dm1.size()) == 2 && 
        (dm2sz = dm2.size()) == (size_t)n - 1);
    if (EXPLICIT_TXN)
        dbstl::commit_txn(penv);

    if (!TEST_AUTOCOMMIT) {
        begin_txn(0, penv);
    }
    dm1.clear();
    dm2.clear();
    dm3.clear();
    if (!TEST_AUTOCOMMIT) {
        commit_txn(penv);
    }
    if (EXPLICIT_TXN)
        begin_txn(0, penv);
    fill(dm1, m1, i = 3, n = 5);
    dm1sz = dm1.size();
    for (i = (int)dm1sz -1; i >0; i--) {
        dm1[i - 1] = dm1[i];
        m1[i - 1] = m1[i];
    }
    if (!dm1.is_hash())
        check_expr (is_equal(dm1, m1));
    dm1[421] = 421;
    check_expr(dm1.count(421) == 1);
    m1[421] = 421;
    int j;
    for (i = 0; i < 100; i++) {
        j = rand() ;
        dm1[i] = i * j;
#ifdef TEST_PRIMITIVE
        m1[i] = ((ptint)dm1[i]);
#else 
        m1[i] = ((ptint)dm1[i]).v;
#endif
    }
    check_expr(dm1.size() == m1.size());
    if (!dm1.is_hash())
        check_expr (is_equal(dm1, m1));

    for (i = 0; i < 99; i++) {
        dm1[i] = dm1[i + 1];
        m1[i] = m1[i + 1];
    }
    if (!dm1.is_hash())
        check_expr (is_equal(dm1, m1));
    dm1.clear();
    m1.clear();
    if (EXPLICIT_TXN)
        commit_txn(penv);
} // test_map_member_functions

void TestAssoc::test_map_std_functions()
{
    int i;

    if (EXPLICIT_TXN)
        begin_txn(0, penv);
    dm_int_t dm1(dmdb1, penv);
    // the following test assumes dm1 and m1 contains
    // (6,6), (7,7),...(14,14)
    for(i = 6; i < 15; i++) {
        dm1[i] = i;
        m1[i] = i;
    }
    dm_int_t::iterator mpos, mpos2;
    dm_int_t::reverse_iterator mrpos, mrpos2;
// for_each
    cout<<"\n_testing std algorithms applied to db_map...\n"; 
    cout<<"\nfor_each begin to end\n";
    for_each(dm1.begin(), dm1.end(), square_pair<dm_int_t::value_type_wrap>);
    cout<<endl;
    for_each(m1.begin(), m1.end(), square_pair2<map<int, int>::value_type>);
    cout<<"\nfor_each begin +1 to end\n";
    for_each(dm1.begin() ++, (dm1.begin()--)--, square_pair<dm_int_t::value_type_wrap>);
    cout<<endl;
    for_each(m1.begin() ++, (m1.begin() --)--, square_pair2<map<int, int>::value_type>);

    //find
    ptint tgt(12);
    mpos = find(dm1.begin(), dm1.end(), make_pair(tgt, tgt));
    check_expr(mpos != dm1.end() && ((mpos->second) == tgt));
    mpos = find(++(dm1.begin() ++), --(dm1.end() --), make_pair(tgt, tgt));
    check_expr(mpos != dm1.end() && ((mpos->second) == tgt));
    tgt = -123;
    mpos = find(dm1.begin(), dm1.end(), make_pair(tgt, tgt));
    check_expr(mpos == dm1.end());
    pair<int, int>* subseq3 = new pair<int, int>[3];
    subseq3[0] = make_pair(8, 8);
    subseq3[1] = make_pair(9, 9);
    subseq3[2] = make_pair(10, 10);
    pair<int, int>* subseq4 = new pair<int, int>[4];
    subseq4[0] = make_pair(9, 9);
    subseq4[1] = make_pair(8, 8);
    subseq4[2] = make_pair(10, 10);
    subseq4[3] = make_pair(11, 11);
    // find_end
    if (!dm1.is_hash()) {
#ifdef WIN32
        mpos = find_end(dm1.begin(), dm1.end(), dm1.begin(), ++(dm1.begin()) );
        check_expr(mpos == dm1.begin());
        mpos = find_end(dm1.begin(), dm1.end(), mpos2 = ++(++(dm1.begin()++)), 
            --(--(dm1.end()--)));
        check_expr(mpos == mpos2);

        mpos = find_end(++dm1.begin(), dm1.end(), subseq3, subseq3 + 3);
        check_expr(mpos == ++(++(dm1.begin())));
#endif
        // find_first_of
        mpos = find_first_of(dm1.begin(), dm1.end(), dm1.begin(), 
            ++dm1.begin());
        check_expr(mpos == dm1.begin());

        mpos = find_first_of(++dm1.begin(), --dm1.end(),
            subseq4, subseq4 + 4);
        check_expr(mpos == ++(++dm1.begin()));
        mpos = find_first_of(--(--dm1.end()), dm1.end(), 
            subseq4, subseq4 + 4);
        check_expr(mpos == dm1.end());
        
        // mismatch
        
        pair<dm_int_t::iterator , pair<int, int>*> resmm2 = mismatch(dm1.begin(),
            ++(++(++dm1.begin())), subseq3);
        check_expr(resmm2.first == dm1.begin() && resmm2.second == subseq3);
    
        //search
        mpos = search(dm1.begin(), dm1.end(), dm1.begin(), ++dm1.begin());
        check_expr(mpos == dm1.begin());
        mpos = search(++dm1.begin(), dm1.end(), subseq3, subseq3 + 3);
        check_expr(mpos == ++(++dm1.begin()));
        //equal
        mpos2 = dm1.begin();
        mpos2.move_to(8);
        check_expr (equal(dm1.begin(), mpos2, subseq3) == false);
        mpos = mpos2; mpos.move_to(11);
        check_expr(equal(mpos2, mpos, subseq3) == true);
        mpos.move_to(10);
        check_expr(equal(mpos2, mpos, subseq3) == true);
    }
    delete [] subseq4;
    delete [] subseq3;
    //find_if
    mpos = find_if(dm1.begin(), dm1.end(), is2digits_pair);
    check_expr(ptint(mpos->second) == 10);

    // count_if
    oddcnt = count_if(dm1.begin(), dm1.end(), is_odd_pair);
    check_expr(oddcnt == 4);
    oddcnt = count_if(dm1.begin(), dm1.begin(), is_odd_pair);
    check_expr(oddcnt == 0);
    

    if ( EXPLICIT_TXN) 
        dbstl::commit_txn(penv);
        

} // test_map_std_functions

void TestAssoc::test_hot_dbclose()
{
    int i;
    map<ptint, ptint> m2;
    dm_int_t dm2(dmdb2, penv);

    if (!TEST_AUTOCOMMIT)
        dbstl::begin_txn(0, penv);
    dm2.clear();// can be auto commit
    if (!TEST_AUTOCOMMIT)
        dbstl::commit_txn(penv);

    if (EXPLICIT_TXN)
        begin_txn(0, penv);
    for (i = 0; i < 5; i++) {
        dm2[i] = i;
        m2[i] = i;
    }

    if (EXPLICIT_TXN)
        dbstl::commit_txn(penv);
    dm_int_t dm1(dmdb1, penv);
    dbstl::close_db(dm1.get_db_handle());
    if (EXPLICIT_TXN)
        dbstl::begin_txn(0, penv);
    
    dm1.set_db_handle(dbp3 = dbstl::open_db(penv, "db_map.db", 
        dbtype, DB_CREATE | dboflags, 0));
    if (!dm1.is_hash())
        check_expr (is_equal(dm1, m1));
    dm1.clear();
    if (EXPLICIT_TXN)
        commit_txn(penv);
} // test_hot_dbclose

void TestAssoc::test_arbitrary_object_storage()
{
    int i;

    if (EXPLICIT_TXN)
        begin_txn(0, penv);
    // varying length objects test
    cout<<"\n_testing arbitary object storage using Dbt..\n";
    
    rand_str_dbt smsdbt;
    DbstlDbt dbt, dbtmsg;
    string msgstr;
    SMSMsg *smsmsgs[10];

    dbtmsg.set_flags(DB_DBT_USERMEM);
    dbt.set_data(DbstlMalloc(256));
    dbt.set_flags(DB_DBT_USERMEM);
    dbt.set_ulen(256);
    db_map<int, DbstlDbt> msgmap(dbp3, penv);
    for (i = 0; i < 10; i++) {
        smsdbt(dbt, msgstr, 10, 200);
        SMSMsg *pmsg = SMSMsg::make_sms_msg(time(NULL), 
            (char *)dbt.get_data(), i);
        smsmsgs[i] = SMSMsg::make_sms_msg(time(NULL), 
            (char *)dbt.get_data(), i);
        dbtmsg.set_data(pmsg);
        dbtmsg.set_ulen((u_int32_t)(pmsg->mysize));
        dbtmsg.set_size((u_int32_t)(pmsg->mysize));
        dbtmsg.set_flags(DB_DBT_USERMEM);
        msgmap.insert(make_pair(i, dbtmsg));
        free(pmsg);
        memset(&dbtmsg, 0, sizeof(dbtmsg));
    }
    dbtmsg.set_data(NULL);
    // check that retrieved data is identical to stored data
    SMSMsg *psmsmsg;
    for (i = 0; i < 10; i++) {
        db_map<int, DbstlDbt>::data_type_wrap msgref = msgmap[i];
        psmsmsg = (SMSMsg *)msgref.get_data();
        
        check_expr(memcmp(smsmsgs[i], psmsmsg, 
            smsmsgs[i]->mysize) == 0);
    }

    i = 0;
    for (db_map<int, DbstlDbt>::iterator msgitr = msgmap.begin(ReadModifyWriteOption::
        read_modify_write()); msgitr != msgmap.end(); ++msgitr, i++) {
        db_map<int, DbstlDbt>::reference smsmsg = *msgitr;
        (((SMSMsg*)(smsmsg.second.get_data())))->when = time(NULL);
        smsmsg.second._DB_STL_StoreElement();
        
    }

    for (i = 0; i < 10; i++) 
        free(smsmsgs[i]);
    
    msgmap.clear();


    cout<<"\n_testing arbitary object(sparse, varying length) storage support using registered callbacks...\n";
    db_map<int, SMSMsg2> msgmap2(dbp3, penv);
    SMSMsg2 smsmsgs2[10];
    DbstlElemTraits<SMSMsg2>::instance()->set_copy_function(SMSMsgCopy);
    DbstlElemTraits<SMSMsg2>::instance()->set_size_function(SMSMsgSize);
    DbstlElemTraits<SMSMsg2>::instance()->set_restore_function(SMSMsgRestore);
    // use new technique to store varying length and inconsecutive objs
    for (i = 0; i < 10; i++) {
        smsdbt(dbt, msgstr, 10, 200);
        SMSMsg2 msg2(time(NULL), msgstr.c_str(), i);
        smsmsgs2[i] = msg2;
        
        msgmap2.insert(make_pair(i, msg2));
     
    }
    
    // check that retrieved data is identical to stored data
    SMSMsg2 tmpmsg2;
    for (i = 0; i < 10; i++) {
        tmpmsg2 = msgmap2[i];
        check_expr(smsmsgs2[i] == tmpmsg2);
    }
    for (db_map<int, SMSMsg2>::iterator msgitr = msgmap2.begin(ReadModifyWriteOption::
        read_modify_write()); msgitr != msgmap2.end(); msgitr++) {
            db_map<int, SMSMsg2>::reference smsmsg = *msgitr;
        smsmsg.second.when = time(NULL);
        smsmsg.second._DB_STL_StoreElement();
        
    }
    msgmap2.clear();
    if (EXPLICIT_TXN)
        commit_txn(penv);
} // test_arbitrary_object_storage

// std::string persistent test.
void TestAssoc::test_storing_std_strings()
{   
    string kstring = "hello world", *sstring = new string("hi there");
    if (EXPLICIT_TXN)
        begin_txn(0, penv);

    db_map<string, string> pmap(dmstringdb, NULL);

    pmap[kstring] = *sstring + "!";
    *sstring = pmap[kstring];
    map<string, string> spmap;
    spmap.insert(make_pair(kstring, *sstring));
    cout<<"sstring append ! is : "<<pmap[kstring]<<" ; *sstring is : "<<*sstring;
    delete sstring;
    for (db_map<string, string>::iterator ii = pmap.begin();
        ii != pmap.end();
        ++ii) {
        cout << (*ii).first << ": " << (*ii).second << endl;
    } 
    close_db(dmstringdb);
    
    dmstringdb = dbstl::open_db(penv, "db_map_stringdb.db", 
        dbtype, DB_CREATE | dboflags, 0);
    db_map<string, string> pmap2(dmstringdb, NULL);
    for (db_map<string, string>::iterator ii = pmap2.begin();
        ii != pmap2.end();
        ++ii) {
        cout << (*ii).first << ": " << (*ii).second << endl;
        // assert key/data pair set equal
        check_expr((spmap.count(ii->first) == 1) && (spmap[ii->first] == ii->second));
    } 
    if (EXPLICIT_TXN)
        commit_txn(penv);

    db_vector<string> strvctor(10);
    vector<string> sstrvctor(10);
    for (int i = 0; i < 10; i++) {
        strvctor[i] = "abc";
        sstrvctor[i] = strvctor[i];
    }
    check_expr(is_equal(strvctor, sstrvctor));
}

void TestAssoc::test_secondary_containers()
{
    int i;

    if (EXPLICIT_TXN)
        begin_txn(0, penv);
    // test secondary db
    cout<<"\n_testing db container backed by secondary database...";
    
    dbp3->associate(dbstl::current_txn(penv), dbp3sec, 
        get_dest_secdb_callback, DB_CREATE);
    typedef db_multimap<int, BaseMsg>  sec_mmap_t;
    sec_mmap_t secmmap(dbp3sec, penv);// index "to" field
    db_map<int, BaseMsg> basemsgs(dbp3, penv);
    basemsgs.clear();
    BaseMsg tmpmsg;
    multiset<BaseMsg> bsmsgs, bsmsgs2;
    multiset<BaseMsg>::iterator bsitr1, bsitr2;
    // populate primary and sec db
    for (i = 0; i < 10; i++) {
        tmpmsg.when = time(NULL);
        tmpmsg.to = 100 - i % 3;// sec index multiple
        tmpmsg.from = i + 20;
        bsmsgs.insert( tmpmsg);
        basemsgs.insert(make_pair(i, tmpmsg));

    }
    check_expr(basemsgs.size() == 10);
    // check retrieved data is identical to those fed in
    sec_mmap_t::iterator itrsec;
    for (itrsec = secmmap.begin(ReadModifyWriteOption::no_read_modify_write(), true);
        itrsec != secmmap.end(); itrsec++) {
        bsmsgs2.insert(itrsec->second);
    }
    for (bsitr1 = bsmsgs.begin(), bsitr2 = bsmsgs2.begin(); 
        bsitr1 != bsmsgs.end() && bsitr2 != bsmsgs2.end(); bsitr1++, bsitr2++) {
        check_expr(*bsitr1 == *bsitr2);
    }
    check_expr(bsitr1 == bsmsgs.end() && bsitr2 == bsmsgs2.end());

    // search using sec index, check the retrieved data is expected
    // and exists in bsmsgs
    check_expr(secmmap.size() == 10);
    pair<sec_mmap_t::iterator, sec_mmap_t::iterator> secrg = 
        secmmap.equal_range(98);

    for (itrsec = secrg.first; itrsec != secrg.second; itrsec++) {
        check_expr(itrsec->second.to == 98 && 
            bsmsgs.count(itrsec->second) > 0);
    }
    // delete via sec db
    size_t nersd = secmmap.erase(98);
    check_expr(10 - nersd == basemsgs.size());
    secrg = secmmap.equal_range(98);
    check_expr(secrg.first == secrg.second);

    if (EXPLICIT_TXN)
        dbstl::commit_txn(penv);
     
} // test_secondary_containers

void TestAssoc::test_mmap_member_functions()
{
    // db_multimap<>
    int i;

    if (EXPLICIT_TXN)
        dbstl::begin_txn(0, penv);  
    dmm_int_t dmm1(dmmdb1, penv);
    
    dmm1.clear();
    // db_multimap<>::empty
    check_expr(dmm1.empty());
    {
    size_t sz0, szeq1, szeq2;

    fill(dmm1, mm1, i = 3, n = 5, 4);
    const dmm_int_t &cdmm1 = dmm1;
    dmm_int_t dmmn(NULL, penv);
    dmmn.insert(cdmm1.begin(), cdmm1.end());
    check_expr(!(dmmn != cdmm1));

    pair<dmm_int_t::const_iterator, dmm_int_t::const_iterator> cnstrg, ceqrgend;
    pair<dmm_int_t::iterator, dmm_int_t::iterator> nceqrg, eqrgend;

    for (i = 3; i < 8; i++) {
        cnstrg = cdmm1.equal_range(i);
        nceqrg = dmm1.equal_range(i);
        dmm_int_t::const_iterator cmmitr;
        dmm_int_t::iterator mmitr;
        for (sz0 = 0, cmmitr = cnstrg.first, mmitr = nceqrg.first; mmitr != nceqrg.second; mmitr++, cmmitr++, sz0++)
            check_expr(*cmmitr == *mmitr);
        check_expr(cmmitr == cnstrg.second);
        if (i < 7)
            check_expr(*(cdmm1.upper_bound(i)) == *(dmm1.upper_bound(i)));
        cnstrg = cdmm1.equal_range_N(i, szeq1);
        nceqrg = dmm1.equal_range_N(i, szeq2);
        check_expr(szeq1 == szeq2 && szeq2 == sz0);
    }
    eqrgend = dmm1.equal_range(65535);
    check_expr(eqrgend.first == dmm1.end() && eqrgend.second == dmm1.end());
    eqrgend = dmm1.equal_range_N(65535, szeq1);
    check_expr(eqrgend.first == dmm1.end() && eqrgend.second == dmm1.end() && szeq1 == 0);
    ceqrgend = cdmm1.equal_range(65535);
    check_expr(ceqrgend.first == cdmm1.end() && ceqrgend.second == cdmm1.end());
    ceqrgend = cdmm1.equal_range_N(65535, szeq1);
    check_expr(ceqrgend.first == cdmm1.end() && ceqrgend.second == cdmm1.end() && szeq1 == 0);
    if (!dmm1.is_hash()) {
        eqrgend = dmm1.equal_range(2);
        check_expr((((eqrgend.first))->first == 3) && ((eqrgend.second))->first == 3);
        ceqrgend = cdmm1.equal_range(2);
        check_expr(((ceqrgend.first))->first == 3 && ((ceqrgend.second))->first == 3);
        eqrgend = dmm1.equal_range_N(2, szeq1);
        check_expr((eqrgend.first)->first == 3 && ((eqrgend.second))->first == 3 && szeq1 == 0);
        ceqrgend = cdmm1.equal_range_N(2, szeq1);
        check_expr((ceqrgend.first)->first == 3 && ((ceqrgend.second))->first == 3 && szeq1 == 0);

        check_expr(((dmm1.upper_bound(2)))->first == 3);
        check_expr(((cdmm1.upper_bound(2)))->first == 3);
        check_expr(dmm1.upper_bound(65535) == dmm1.end());
        check_expr(cdmm1.upper_bound(65535) == cdmm1.end());
        check_expr(dmm1.lower_bound(65535) == dmm1.end());
        check_expr(cdmm1.lower_bound(65535) == cdmm1.end());
        check_expr(dmm1.count(65535) == 0);
        check_expr(dmm1.find(65535) == dmm1.end());
        check_expr(cdmm1.find(65535) == cdmm1.end());

        dmm_int_t tmpdmm0(dmm1);
        dmm1.erase(dmm1.end());
        check_expr(dmm1 == tmpdmm0);
        nceqrg = tmpdmm0.equal_range(5);
    
        for (dmm_int_t::iterator itr0 = nceqrg.first; itr0 != nceqrg.second; ++itr0)
            itr0->second *= 2;
        check_expr(tmpdmm0 != dmm1);
        tmpdmm0.insert(dmm1.begin(), ++(++dmm1.begin()));
        check_expr(tmpdmm0 != dmm1);
    }
    dmm1.clear();
    mm1.clear();
    }
    fill(dmm1, mm1, i = 3, n = 5, 4);
    check_expr(!dmm1.empty());

    typedef multimap<int, int> mm_int_t;
    dmm_int_t::iterator dmmi, dmmi2;
    mm_int_t::iterator mmi;
    dmm_int_t::reverse_iterator dmmri;
    mm_int_t::reverse_iterator mmri;

    print_mm(dmm1, dmmi);
    // db_multimap<>::find, count
    for (dmmi = dmm1.begin(), mmi = mm1.begin(), i = 3; 
        dmmi != dmm1.end() && 
        mmi !=mm1.end(); dmmi++, mmi++, i++) {
        // check_expr both contain
        check_expr((*(dmm1.find(i))).second == i);
        
        pair<dmm_int_t::iterator, dmm_int_t::iterator> erp1 = 
            dmm1.equal_range(i);
        int jc = 0;
        dmm_int_t::iterator jj;
        for (jj = erp1.first, jc = 0; 
            jj != erp1.second; jj++, jc++)
            check_expr((*jj).second == ptint(i));
        
        check_expr((size_t)jc == g_count[i]); // g_count[i] duplicates
        if (i < 7 && !dmm1.is_hash()) {// 7 is last element
            check_expr((*(dmm1.upper_bound(i))).second == i + 1);
            check_expr((*(dmm1.lower_bound(i))).second == i);
        }
        else if (i == 7 && !dmm1.is_hash()) {
            check_expr(dmm1.upper_bound(i) == dmm1.end());
            check_expr((*(dmm1.lower_bound(i))).second == i);
        } else if (!dmm1.is_hash())
            check_expr(false);

        check_expr(dmm1.count(i) == g_count[i]);
        check_expr(dmm1.count(-i) == 0);
        if (!dmm1.is_hash())
            check_expr((*dmmi).second == (*mmi).second);
        if (i == 3 + n - 1) {// last element
            
            for (; dmmi != dmm1.end(); mmi--, i--) {
                // check_expr both contains
                check_expr((*(dmm1.find(i))).second == i);
                if (!dmm1.is_hash())
                    check_expr((*dmmi).second == (*mmi).second);
                if (i % 2)
                    dmmi--;
                else 
                    --dmmi;
            }
            break;
        }
        
    }
    for (dmmri = dmm1.rbegin(), mmri = mm1.rbegin(), i = 3; 
        dmmri != dmm1.rend() && mmri !=mm1.rend(); dmmri++, mmri++, i++) {
        check_expr((dmm1.find(i) != dmm1.end()) && 
            (mm1.find(i) != mm1.end()));// check_expr both contain
        if (dmm1.is_hash() == false)
            check_expr((*dmmri).second == (*mmri).second);
        if (i == 3 + n - 1) {// last element
            
            for (; dmmri != dmm1.rend() && mmri != mm1.rend(); i--) {
                // check_expr both contain
                check_expr((*(dmm1.find(i))).second == i);
                if (!dmm1.is_hash())
                    check_expr((*dmmri).second == (*mmri).second);
                if (i % 2)
                    dmmri--;
                else
                    --dmmri;
                if (i > 3) // MS STL bug: when msri points to last element, it can not go back
                    mmri--;
                
            }
            break;
        }
        
    }
    for (dmmi = dmm1.begin(), mmi = mm1.begin(), i = 3; 
        dmmi != dmm1.end() && 
        mmi !=mm1.end(); ++dmmi, mmi++)
        i++;
    check_expr((size_t)i == 3 + g_sum(3, 8));
    for (dmmri = dmm1.rbegin(), mmri = mm1.rbegin(), i = 3; 
        dmmri != dmm1.rend() && mmri !=mm1.rend(); ++dmmri, mmri++)
        i++;
    check_expr((size_t)i == 3 + g_sum(3, 8));
    
    dmm_int_t dmm2(dmmdb2, penv);
    dmm2.clear();
    dmmi = dmm1.begin();
    dmmi2 = dmm1.end();
    dmm2.insert(dmmi, dmmi2);
    if (EXPLICIT_TXN)
        dbstl::commit_txn(penv);

    if (!TEST_AUTOCOMMIT)
        dbstl::begin_txn(0, penv);
    dmm_int_t dmm3 = dmm2;
    if (!TEST_AUTOCOMMIT)
        dbstl::commit_txn(penv);

    if (EXPLICIT_TXN)
        dbstl::begin_txn(0, penv);
    check_expr(dmm3 == dmm2);
    if (EXPLICIT_TXN)
        dbstl::commit_txn(penv);

    if (!TEST_AUTOCOMMIT)
        dbstl::begin_txn(0, penv);
    dmm3 = dmm1;
    if (!TEST_AUTOCOMMIT)
        dbstl::commit_txn(penv);

    if (EXPLICIT_TXN)
        dbstl::begin_txn(0, penv);
    check_expr(dmm3 == dmm1);

    for (dmmi = dmm1.begin(), dmmi2 = dmm2.begin(); dmmi != dmm1.end() && 
        dmmi2 != dmm2.end(); ++dmmi, dmmi2++)
        check_expr(*dmmi == *dmmi2);

    // content of dmm1 and dmm2 are changed since now
    int arr1[] = {33, 44, 55, 66, 77};
    int arr11[] = {33, 44, 55, 66, 77};
    for (dmmi = dmm1.begin(), i = 0; dmmi != dmm1.end(); dmmi++, i++)
        (*dmmi).second = tpint(arr11[i % 5]);

    for (dmmi = dmm1.begin(), dmmi2 = dmm2.begin(), i = 0; 
        dmmi != dmm1.end() && dmmi2 != dmm2.end(); 
        dmmi++, i++, dmmi2++) {
        check_expr((*dmmi).second == tpint(arr1[i % 5]));
        dmmi2->second = dmmi->second;
        check_expr(*dmmi == *dmmi2);
    }

    // db_multimap<>::insert(const value_type&). the range insert is already 
    // tested in fill
    //
    dmm_int_t::iterator iitr = dmm1.insert(make_pair(3, tpint(33)));
    check_expr(iitr->first == 3);
    // the returned iitr points to any rec with key==3, not necessarily 
    // the newly inserted rec, so not sure of the value on hash
    if (dmm1.is_hash() == false)
        check_expr(iitr->second == 33);

    dmm1.clear();
    mm1.clear();
    fill(dmm1, mm1, i = 3, n = 5, 4);
    // db_multimap<>::count, insert, erase, find
    size_t sizet1;
    check_expr(dmm1.count(3) == g_count[3]);
    check_expr(dmm1.size() == (sizet1 = g_sum(3, 8)));// sum of recs from 3 to 8
    check_expr(dmm1.count(9) == 0);
    dmm_int_t::iterator iitr2 = dmm1.insert(make_pair(9, tpint(99)));
    check_expr(iitr2->first == 9 && iitr2->second == 99);
    check_expr(dmm1.count(9) == 1);
    check_expr(dmm1.size() == sizet1 + 1);
    dmm1.erase(9);
    check_expr(dmm1.size() == (sizet1 = g_sum(3, 8)));
    check_expr(dmm1.count(9) == 0);
    dmm1.erase(dmm1.find(3));
    check_expr(dmm1.size() == g_sum(3, 8) - 1);
    check_expr(dmm1.count(3) == (g_count[3] - 1 > 0 ? g_count[3] - 1 : 0));
    dmm2.erase(dmm2.begin(), dmm2.end());
    check_expr(dmm2.size() == 0);
    check_expr(dmm2.empty());

    dmmi = dmm1.begin();
    dmmi++;
    dmmi2 = dmmi;
    dmmi2++;
    dmmi2++;
    size_t sizet2;
    dmm2.insert(dmmi, dmmi2);// 2 recs inserted, [dmi, dmi2)
    check_expr((sizet2 = dmm2.size()) == 2);
    if (EXPLICIT_TXN)
        dbstl::commit_txn(penv);

    if (!TEST_AUTOCOMMIT)
        dbstl::begin_txn(0, penv);

    dmm1.swap(dmm2);
    if (!TEST_AUTOCOMMIT)
        dbstl::commit_txn(penv);

    if (EXPLICIT_TXN)
        dbstl::begin_txn(0, penv);

    size_t dmm1sz, dmm2sz;
    check_expr((dmm1sz = dmm1.size()) == 2 && (dmm2sz = 
        dmm2.size()) == sizet1 - 1);
    if (EXPLICIT_TXN)
        dbstl::commit_txn(penv);

    if (!TEST_AUTOCOMMIT)
        dbstl::begin_txn(0, penv);
    dmm1.clear();
    dmm2.clear();
    dmm3.clear();
    if (!TEST_AUTOCOMMIT)
        dbstl::commit_txn(penv);

    if (EXPLICIT_TXN)
        dbstl::begin_txn(0, penv);
    
    dmm_int_t dmm4(dmmdb4, penv);
    dmm4.clear();
    multimap<ptint, ptint> mm4;
    int jj;
    for (i = 0; i < 10; i++) {
        sizet2 = abs(rand() % 20) + 1;
        for (jj = 0; jj < (int)sizet2; jj++) {
            dmm4.insert(make_pair(i, jj));
            mm4.insert(make_pair(i, jj));
        }
    }
    pair<dmm_int_t::iterator, dmm_int_t::iterator> eqrgdmm;
    pair<multimap<ptint, ptint>::iterator, multimap<ptint, ptint>::iterator> 
        eqrgmm;
    dmm_int_t::iterator dmmitr;
    multimap<ptint, ptint>::iterator mmitr;
    for (i = 0; i < 10; i++) {
        eqrgdmm = dmm4.equal_range(i);
        eqrgmm = mm4.equal_range(i);
        for (dmmitr = eqrgdmm.first, mmitr = eqrgmm.first; dmmitr != 
            eqrgdmm.second && mmitr != eqrgmm.second; dmmitr++, mmitr++)
            check_expr(*dmmitr == *mmitr);
        check_expr((dmmitr == eqrgdmm.second) && (mmitr == eqrgmm.second));
    }
    if (EXPLICIT_TXN)
        dbstl::commit_txn(penv);
    
} // test_mmap_member_functions

void TestAssoc::test_set_member_functions()
{
// db_set<>
    int i;
    int arr1[] = {33, 44, 55, 66, 77};

    if (EXPLICIT_TXN)
        dbstl::begin_txn(0, penv);
    dms_int_t dms1(dmsdb1, penv);
    const dms_int_t& cnstdms1 = dms1;
    set<ptint> ms1;

    n = 10;
    for (i = 0; i < n; i++)
        dms1.insert(i);

    for (i = 0; i < n; i++) {
        dms_int_t::const_iterator citr, citr1;
        citr = dms1.find(i);
        check_expr(*citr == *dms1.find(i));
        dms_int_t::iterator itr = citr;
        check_expr(*citr == *itr);
        *itr = i;
        itr = dms1.find(i);
        citr = itr;
        citr1 = itr;
        check_expr(*citr == *itr);
        check_expr(*citr == *citr1);
        check_expr(*citr == *dms1.find(i));
    }

    for (i = 0; i < n; i++) {
        dms_int_t::const_iterator citr = dms1.find(i);
        check_expr(*citr == *dms1.find(i));
        dms_int_t::iterator itr = citr;
        check_expr(*citr == *itr);
        *itr = i;
        itr = dms1.find(i);
        citr = itr;
        dms_int_t::const_iterator citr1 = itr;
        check_expr(*citr == *itr);
        check_expr(*citr == *citr1);
        check_expr(*citr == *dms1.find(i));
    }

    for (i = 0; i < n; i++) {
        dms_int_t::iterator ncitr, ncitr1;
        dms_int_t::const_iterator citr, citr1;

        ncitr = cnstdms1.find(i);
        check_expr(*ncitr == *cnstdms1.find(i));
        citr = ncitr;
        check_expr(*citr == *ncitr);
        //*ncitr = i * 2 + 1;
        citr1 = ncitr;
        ncitr1 = citr1;
        check_expr(*citr == *ncitr);
        check_expr(*citr == *ncitr1);
        check_expr(*citr == *citr1);
        check_expr(*citr == *cnstdms1.find(i));
    }

    for (i = 0; i < n; i++) {

        dms_int_t::iterator ncitr = cnstdms1.find(i);
        check_expr(*ncitr == *cnstdms1.find(i));
        dms_int_t::const_iterator citr = ncitr;
        check_expr(*citr == *ncitr);
        //*itr = i * 2 + 1;
        dms_int_t::const_iterator citr1 = ncitr;
        dms_int_t::iterator ncitr1 = citr1;

        check_expr(*citr == *ncitr);
        check_expr(*citr == *ncitr1);
        check_expr(*citr == *citr1);
        check_expr(*citr == *cnstdms1.find(i));
    }

    if (dms1.is_hash() == false)
    {
        ms1.clear();
        dms1.clear();
        for (i = 0; i < n; i++) {
            dms1.insert(ptint(i));
            ms1.insert(ptint(i));
        }

        const dms_int_t &cnstdms1 = dms1;
        dms_int_t tmpdms0;

        tmpdms0.insert(cnstdms1.begin(), cnstdms1.end());
        dms_int_t::const_iterator citr = dms1.end(), citr2 = --dms1.begin(), itr2;
        set<ptint>::const_iterator scitr = ms1.end(), scitr2 = --ms1.begin();       
        for (citr--, scitr--; citr != citr2; --citr, --scitr) {
            check_expr(*citr == *scitr);
        }
        check_expr(scitr == scitr2);

        db_set<ptype<int> > dmspt;
        dmspt.insert(cnstdms1.begin(), cnstdms1.end());
        
        db_set<ptype<int> >::const_iterator itrpt;
        db_set<ptype<int> >::iterator itrpt2;

        for (itrpt = dmspt.begin(), citr = dms1.begin(); itrpt != dmspt.end(); ++itrpt, ++citr) {
            itrpt.refresh(true);
            check_expr(itrpt->v == *citr);
        }

        for (itrpt2 = dmspt.begin(), itr2 = dms1.begin(); itrpt2 != dmspt.end(); ++itrpt2, ++itr2) {
            itrpt2.refresh(true);
            check_expr(itrpt2->v == *itr2);
        }

        dms_int_t dmstmp(dms1);
        check_expr(dms1 == dmstmp);
        dms1.insert(dms1.begin(), 101);
        check_expr(dms1 != dmstmp);

        ms1.clear();
        dms1.clear();
    }
    dms1.clear();
    // db_map<>::empty
    check_expr(dms1.empty());
    fill(dms1, ms1, i = 3, n = 5);
    check_expr(!dms1.empty());

    typedef set<int> ms_int_t;
    dms_int_t::iterator dmsi, dmsi2;
    ms_int_t::iterator msi;
    dms_int_t::reverse_iterator dmsri;
    ms_int_t::reverse_iterator msri;

    // db_set<>::find, count
    for (dmsi = dms1.begin(), msi = ms1.begin(), i = 3; dmsi != dms1.end() && 
        msi !=ms1.end(); dmsi++, msi++, i++) {
        // check_expr both contain
        check_expr(*(dms1.find(i)) == i);
        
        pair<dms_int_t::iterator, dms_int_t::iterator> erp = 
            dms1.equal_range(i);
        int jc = 0;
        dms_int_t::iterator jj;
        for (jj = erp.first, jc = 0; 
            jj != erp.second; jj++, jc++)
            check_expr((*jj) == ptint(i));
        check_expr(jc == 1);
        if (i < 7 && !dms1.is_hash()) {// 7 is last element
            check_expr((*(dms1.upper_bound(i))) == i + 1);
            check_expr((*(dms1.lower_bound(i))) == i);
        }
        else if (i == 7 && !dms1.is_hash()) {
            check_expr(dms1.upper_bound(i) == dms1.end());
            check_expr((*(dms1.lower_bound(i))) == i);
        } else if (!dms1.is_hash())
            check_expr(false);

        check_expr(dms1.count(i) == 1);
        check_expr(dms1.count(-i) == 0);
        if (!dms1.is_hash()) {
            check_expr((*dmsi) == (*msi));
            check_expr((*dmsi) == (*msi));
        }
        if (i == 3 + n - 1) {// last element
            if (!dms1.is_hash()) 
                check_expr((*dmsi) == (*msi));
            for (; dmsi != dms1.end() && (*dmsi) == (*msi); i--) {
                // check_expr both contains
                if (!dms1.is_hash()) 
                    check_expr((*dmsi) == (*msi));
                check_expr((*(dms1.find(i)) == i));
                
                if (i % 2)
                    dmsi--;
                else 
                    --dmsi;
                msi--;
            }
            break;
        }
        
    }
    for (dmsri = dms1.rbegin(), msri = ms1.rbegin(), i = 3; 
        dmsri != dms1.rend() && msri != ms1.rend(); dmsri++, msri++, i++) {
        check_expr((*(dms1.find(i)) == i));
        if (!dms1.is_hash()) 
            check_expr((*dmsri) == (*msri));
        if (i == 3 + n - 1) {// last element
            
            for (; dmsri != dms1.rend() && msri != ms1.rend(); i--) {
                // check_expr both contain
                check_expr((dms1.find(i) != dms1.end()) && 
                    (ms1.find(i) != ms1.end()));
                if (!dms1.is_hash()) 
                    check_expr((*dmsri) == (*msri));

                if (i % 2)
                    dmsri--;
                else
                    --dmsri;
                if (i > 3) // MS STL bug: when msri points to last element, it can not go back
                    msri--;
                
            }
            break;
        }
        
    }
    for (dmsi = dms1.begin(), msi = ms1.begin(), i = 3; dmsi != dms1.end() && 
        msi !=ms1.end(); ++dmsi, msi++)
        i++;
    check_expr(i == 3 + n);

    for (dmsri = dms1.rbegin(), msri = ms1.rbegin(), i = 3; 
        dmsri != dms1.rend() && msri !=ms1.rend(); ++dmsri, msri++)
        i++;
    check_expr(i == 3 + n);
    
    dms_int_t dms2(dmsdb2, penv);
    dms2.clear();
    dms2.insert(dms1.begin(), dms1.end());
    if (EXPLICIT_TXN)
        dbstl::commit_txn(penv);
    
    if (!TEST_AUTOCOMMIT)
        dbstl::begin_txn(0, penv);
    dms_int_t dms3 = dms2;
    if (!TEST_AUTOCOMMIT)
        dbstl::commit_txn(penv);

    if (EXPLICIT_TXN)
        dbstl::begin_txn(0, penv);

    check_expr(dms3 == dms2);
    dms3 = dms1;
    if (!TEST_AUTOCOMMIT)
        dbstl::commit_txn(penv);

    if (EXPLICIT_TXN)
        dbstl::begin_txn(0, penv);

    check_expr(dms3 == dms1);

    for (dmsi = dms1.begin(), dmsi2 = dms2.begin(); dmsi != dms1.end() && 
        dmsi2 != dms2.end(); ++dmsi, dmsi2++)
        check_expr(*dmsi == *dmsi2);
    
    /* !!!XXX 
    set keys are not supposed to be mutated, so this is an extra functionality, just like 
    the set iterator assignment in ms's stl library. Here we must use i < dms1.size() to
    limit the number of loops because after each assignment, dmsi will be positioned to 
    the element which is immediately next to the old key it sits on before the assignment,
    thus it may take more loops to go to the end() position.
    */
    pprint(dms1, "dms1 before iterator assignment : \n");
    dms_int_t::size_type ui;
    for (dmsi = dms1.begin(), ui = 0; dmsi != dms1.end() && ui < dms1.size(); dmsi++, ui++) {
        (*dmsi) = tpint(arr1[ui]);
        pprint(dms1, "\ndms1 after one element assignment : ");
    }
    if (dms1.is_hash() == false) {
        pprint(dms1, "dms1 after iterator assignment : \n");
        for (dmsi = dms1.begin(), dmsi2 = dms2.begin(), i = 0; 
            dmsi != dms1.end() && dmsi2 != dms2.end(); dmsi++, i++, dmsi2++) {
            check_expr((*dmsi) == tpint(arr1[i]));
            *dmsi2 = *dmsi;
            // check_expr(*dmsi == *dmsi2); dmsi2 is invalidated by the assignment, so can't compare here.
        }
        // Compare here.
        for (dmsi = dms1.begin(), dmsi2 = dms2.begin(), i = 0; 
            dmsi != dms1.end() && dmsi2 != dms2.end(); dmsi++, i++, dmsi2++)
            check_expr(*dmsi2 == *dmsi);
    }

    dms1.clear();
    //dms2.clear();
    fill(dms1, ms1, i = 3, n = 5);
    // db_set<>::insert(const value_type&). the range insert is already 
    // tested in fill
    //
    pair<dms_int_t::iterator, bool> ress = 
        dms1.insert(tpint(3));
    check_expr((*(ress.first)) == 3 && ress.second == false);

    // db_set<>::count, insert, erase, find
    check_expr(dms1.count(3) == 1);
    check_expr(dms1.size() == (size_t)n);// n is 5
    check_expr(dms1.count(9) == 0);
    ress = dms1.insert(tpint(9));
    check_expr((*(ress.first)) == 9 && ress.second == true);
    check_expr(dms1.count(9) == 1);
    check_expr(dms1.size() == (size_t)n + 1);
    dms1.erase(9);
    check_expr(dms1.size() == (size_t)n);
    check_expr(dms1.count(9) == 0);
    dms1.erase(dms1.find(3));
    check_expr(dms1.size() == (size_t)n - 1);
    check_expr(dms1.count(3) == 0);
    dms2.erase(dms2.begin(), dms2.end());
    check_expr(dms2.size() == 0);
    check_expr(dms2.empty());

    dmsi = dms1.begin();
    dmsi++;
    dmsi2 = dmsi;
    dmsi2++;
    dmsi2++;
    dms2.insert(dmsi, dmsi2);// 2 recs inserted, [dmsi, dmsi2)
    check_expr(dms2.size() == 2);
    if (EXPLICIT_TXN)
        dbstl::commit_txn(penv);
    
    if (!TEST_AUTOCOMMIT)
        dbstl::begin_txn(0, penv);
    dms1.swap(dms2);
    if (!TEST_AUTOCOMMIT)
        dbstl::commit_txn(penv);

    if (EXPLICIT_TXN)
        dbstl::begin_txn(0, penv);
    size_t dms1sz, dms2sz;
    check_expr((dms1sz = dms1.size()) == 2 && (dms2sz = dms2.size()) == (size_t)n - 1);
    dms1.clear();
    dms2.clear();
    dms3.clear();

    if (EXPLICIT_TXN)
        dbstl::commit_txn(penv);

} // test_set_member_functions

void TestAssoc::test_mset_member_functions()
{
// db_multiset<>
    int i;
    size_t sizet1;

    if (EXPLICIT_TXN)
        dbstl::begin_txn(0, penv);
    
    dmms_int_t dmms1(dmmsdb1, penv);
    multiset<int> mms1;

    dmms1.clear();
    // db_multiset<>::empty
    check_expr(dmms1.empty());
    fill(dmms1, mms1, i = 3, n = 5, 4);
    check_expr(!dmms1.empty());
    dmms_int_t tmpmms0;
    const dmms_int_t &cnstmms = dmms1;
    dmms_int_t tmpmms1;
    tmpmms1.insert(dmms1.begin(), dmms1.end());
    tmpmms0.insert(cnstmms.begin(), cnstmms.end());
    tmpmms0.insert(tmpmms0.begin(), *(--tmpmms0.end()));
    check_expr(tmpmms0 != tmpmms1);

    typedef multiset<int> mms_int_t;
    dmms_int_t::iterator dmmsi, dmmsi2;
    mms_int_t::iterator mmsi;
    dmms_int_t::reverse_iterator dmmsri;
    mms_int_t::reverse_iterator mmsri;

    // db_multiset<>::find, count
    for (dmmsi = dmms1.begin(), mmsi = mms1.begin(), i = 3; 
        dmmsi != dmms1.end() && 
        mmsi !=mms1.end(); dmmsi++, mmsi++, i++) {
        // check_expr both contain
        check_expr((*(dmms1.find(i))) == i);
        
        pair<dmms_int_t::iterator, dmms_int_t::iterator> erp1 = 
            dmms1.equal_range(i);
        int jc = 0;
        dmms_int_t::iterator jj;
        for (jj = erp1.first, jc = 0; 
            jj != erp1.second; jj++, jc++) {
            // there is bug so this line can be reached
            if ((size_t)jc == g_count[i]) {
                jj == erp1.second;// cmp again to debug it
                dmms1.get_db_handle()->stat_print(DB_STAT_ALL);
            }
            check_expr((*jj) == ptint(i));
        }
        check_expr((size_t)jc == g_count[i]); // g_count[i] duplicates
        if (i < 7 && !dmms1.is_hash()) {// 7 is last element
            check_expr((*(dmms1.upper_bound(i))) == i + 1);
            check_expr((*(dmms1.lower_bound(i))) == i);
        }
        else if (i == 7 && !dmms1.is_hash()) {
            check_expr(dmms1.upper_bound(i) == dmms1.end());
            check_expr((*(dmms1.lower_bound(i))) == i);
        } else if (!dmms1.is_hash())
            check_expr(false);

        check_expr(dmms1.count(i) == g_count[i]);
        check_expr(dmms1.count(-i) == 0);
        if (!dmms1.is_hash())
            check_expr((*dmmsi) == (*mmsi));
        if (i == 3 + n - 1) {// last element
            
            for (; dmmsi != dmms1.end(); mmsi--, i--) {
                // check_expr both contains
                check_expr((*(dmms1.find(i))) == i);
                if (!dmms1.is_hash())
                    check_expr((*dmmsi) == (*mmsi));
                if (i % 2)
                    dmmsi--;
                else 
                    --dmmsi;
            }
            break;
        }
        
    }
    for (dmmsri = dmms1.rbegin(), mmsri = mms1.rbegin(), i = 3; 
        dmmsri != dmms1.rend() && mmsri !=mms1.rend(); dmmsri++, mmsri++, i++) {
        check_expr((dmms1.find(i) != dmms1.end()) && 
            (mms1.find(i) != mms1.end()));// check_expr both contain
        if (dmms1.is_hash() == false)
            check_expr((*dmmsri) == (*mmsri));
        if (i == 3 + n - 1) {// last element
            
            for (; dmmsri != dmms1.rend() && mmsri != mms1.rend(); i--) {
                // check_expr both contain
                check_expr((*(dmms1.find(i))) == i);
                if (!dmms1.is_hash())
                    check_expr((*dmmsri) == (*mmsri));
                if (i % 2)
                    dmmsri--;
                else
                    --dmmsri;
                if (i > 3) // MS STL bug: when msri points to last element, it can not go back
                    mmsri--;
            }
            break;
        }
        
    }
    for (dmmsi = dmms1.begin(), mmsi = mms1.begin(), i = 3; 
        dmmsi != dmms1.end() && 
        mmsi !=mms1.end(); ++dmmsi, mmsi++)
        i++;
    check_expr((size_t)i == 3 + g_sum(3, 8));
    for (dmmsri = dmms1.rbegin(), mmsri = mms1.rbegin(), i = 3; 
        dmmsri != dmms1.rend() && mmsri !=mms1.rend(); ++dmmsri, mmsri++)
        i++;
    check_expr((size_t)i == 3 + g_sum(3, 8));
    
    
        
    
    dmms_int_t dmms2(dmmsdb2, penv);
    dmms2.clear();
    dmms2.insert(dmms1.begin(), dmms1.end());
    if (EXPLICIT_TXN)
        dbstl::commit_txn(penv);
    
    if (!TEST_AUTOCOMMIT)
        dbstl::begin_txn(0, penv);
    dmms_int_t dmms3 = dmms2;
    if (!TEST_AUTOCOMMIT)
        dbstl::commit_txn(penv);

    if (EXPLICIT_TXN)
        dbstl::begin_txn(0, penv);
    cout<<"\ndmms2: \n";
    for (dmms_int_t::iterator itr = dmms2.begin(); itr != dmms2.end(); ++itr)
        cout<<"\t"<<*itr;
    cout<<"\ndmms3: \n";
    for (dmms_int_t::iterator itr = dmms3.begin(); itr != dmms3.end(); ++itr)
        cout<<"\t"<<*itr;

    check_expr(dmms3 == dmms2);
    if (EXPLICIT_TXN)
        dbstl::commit_txn(penv);
    
    if (!TEST_AUTOCOMMIT)
        dbstl::begin_txn(0, penv);
    dmms3 = dmms1;
    if (!TEST_AUTOCOMMIT)
        dbstl::commit_txn(penv);

    if (EXPLICIT_TXN)
        dbstl::begin_txn(0, penv);
    check_expr(dmms3 == dmms1);

    for (dmmsi = dmms1.begin(), dmmsi2 = dmms2.begin(); dmmsi != dmms1.end() && 
        dmmsi2 != dmms2.end(); ++dmmsi, dmmsi2++)
        check_expr(*dmmsi == *dmmsi2);

    // content of dmms1 and dmms2 are changed since now
    // int arr11[] = {33, 44, 55, 66, 77};
    
    // db_multiset<>::insert(const value_type&). the range insert is already 
    // tested in fill
    //
    dmms_int_t::iterator iitrms = dmms1.insert(ptint (3));
    check_expr(*iitrms == 3);
     
    dmms1.clear();
    mms1.clear();
    fill(dmms1, mms1, i = 3, n = 5, 4);
    // db_multiset<>::count, insert, erase, find
    check_expr(dmms1.count(3) == g_count[3]);
    check_expr(dmms1.size() == (sizet1 = g_sum(3, 8)));// sum of recs from 3 to 8
    check_expr(dmms1.count(9) == 0);
    dmms_int_t::iterator iitrms2 = dmms1.insert(ptint (9));
    check_expr(*iitrms2 == 9);
    check_expr(dmms1.count(9) == 1);
    check_expr(dmms1.size() == sizet1 + 1);
    dmms1.erase(9);
    check_expr(dmms1.size() == g_sum(3, 8));
    check_expr(dmms1.count(9) == 0);
    dmms1.erase(dmms1.find(3));
    check_expr(dmms1.size() == g_sum(3, 8) - 1);
    check_expr(dmms1.count(3) == (g_count[3] - 1 > 0 ? g_count[3] - 1 : 0));
    dmms2.erase(dmms2.begin(), dmms2.end());
    check_expr(dmms2.size() == 0);
    check_expr(dmms2.empty());

    dmmsi = dmms1.begin();
    dmmsi++;
    dmmsi2 = dmmsi;
    dmmsi2++;
    dmmsi2++;
    dmms2.insert(dmmsi, dmmsi2);// 2 recs inserted, [dmi, dmi2)
    size_t sssz;
    check_expr((sssz = dmms2.size()) == 2);
    if (EXPLICIT_TXN)
        dbstl::commit_txn(penv);
    
    if (!TEST_AUTOCOMMIT)
        dbstl::begin_txn(0, penv);
    dmms1.swap(dmms2);
    if (!TEST_AUTOCOMMIT)
        dbstl::commit_txn(penv);

    if (EXPLICIT_TXN)
        dbstl::begin_txn(0, penv);
    size_t dmms1sz, dmms2sz;
    check_expr((dmms1sz = dmms1.size()) == 2 && (dmms2sz = dmms2.size()) == sizet1 - 1);

    dmms1.clear();
    dmms2.clear();
    dmms3.clear();
    if (EXPLICIT_TXN)
        dbstl::commit_txn(penv);
    
} // test_mset_member_functions

void TestAssoc::test_char_star_string_storage()
{
    int i;
    // testing varying length data element storage/retrieval
    cout<<"\n_testing char*/wchar_t* string storage support...\n";
    
    if (EXPLICIT_TXN)
        dbstl::begin_txn(0, penv);
    // Use Dbt to wrap any object and store them. This is rarely needed, 
    // so this piece of code is only for test purpose.
    db_vector<DbstlDbt> strv(dbstrv, penv);
    vector<string> strsv;
    vector<DbstlDbt> strvdbts;
    strv.clear();
    
    int strlenmax = 256, strlenmin = 64;
    string str;
    DbstlDbt dbt;
    rand_str_dbt rand_str_maker;
    dbt.set_flags(DB_DBT_USERMEM);
    dbt.set_data(DbstlMalloc(strlenmax + 10));
    dbt.set_ulen(strlenmax + 10);

    for (int jj = 0; jj < 10; jj++) {
        rand_str_maker(dbt, str, strlenmin, strlenmax);
        strsv.push_back(str);
        strv.push_back(dbt);
    }
    
    cout<<"\nstrings:\n";
    for (i = 0; i < 10; i++) {
        
        db_vector<DbstlDbt>::value_type_wrap elemref = strv[i];
        strvdbts.push_back(elemref);
        printf("\n%s\n%s",  (char*)(strvdbts[i].get_data()), strsv[i].c_str());
        check_expr(strcmp((char*)(elemref.get_data()), strsv[i].c_str()) == 0);
        check_expr(strcmp((char*)(strvdbts[i].get_data()), strsv[i].c_str()) == 0);
    }
    strv.clear();
    
    if (EXPLICIT_TXN) {
        dbstl::commit_txn(penv);
        dbstl::begin_txn(0, penv);
    }

    // Use ordinary way to store strings.
    TCHAR cstr1[32], cstr2[32], cstr3[32];
    strcpy(cstr1, "abc");
    strcpy(cstr2, "defcd");
    strcpy(cstr3, "edggsefcd");
// = _T("abc"), *cstr2 = _T("defcd"), *cstr3 = _T("edggsefcd");
    typedef db_map<int, TCHAR*, ElementHolder<TCHAR*> > strmap_t;
    strmap_t strmap(dmdb6, penv);
    strmap.clear();
    strmap.insert(make_pair(1, cstr1));
    strmap.insert(make_pair(2, cstr2));
    strmap.insert(make_pair(3, cstr3));
    cout<<"\n strings in strmap:\n";
    for (strmap_t::const_iterator citr = strmap.begin(); citr != strmap.end(); citr++)
        cout<<(*citr).second<<'\t';
    cout<<strmap[1]<<strmap[2]<<strmap[3];
    TCHAR cstr4[32], cstr5[32], cstr6[32];
    _tcscpy(cstr4, strmap[1]);
    _tcscpy(cstr5, strmap[2]);
    _tcscpy(cstr6, strmap[3]);
    _tprintf(_T("%s, %s, %s"), cstr4, cstr5, cstr6);
    strmap_t::value_type_wrap::second_type vts = strmap[1];
    using_charstr(vts);
    vts._DB_STL_StoreElement();
    _tcscpy(cstr4, _T("hello world"));
    vts = cstr4;
    vts._DB_STL_StoreElement();
    cout<<"\n\nstrmap[1]: "<<strmap[1];
    check_expr(_tcscmp(strmap[1], cstr4) == 0);
    vts[0] = _T('H');// itis wrong to do it this way
    vts._DB_STL_StoreElement();
    check_expr(_tcscmp(strmap[1], _T("Hello world")) == 0);
    TCHAR *strbase = vts._DB_STL_value();
    strbase[6] = _T('W');
    vts._DB_STL_StoreElement();
    check_expr(_tcscmp(strmap[1], _T("Hello World")) == 0);
    strmap.clear();

    typedef db_map<const char *, const char *, ElementHolder<const char *> > cstrpairs_t;
    cstrpairs_t strpairs(dmdb6, penv);
    strpairs["abc"] = "def";
    strpairs["ghi"] = "jkl";
    strpairs["mno"] = "pqrs";
    strpairs["tuv"] = "wxyz";
    cstrpairs_t::const_iterator ciitr;
    cstrpairs_t::iterator iitr;
    for (ciitr = strpairs.begin(), iitr = strpairs.begin(); iitr != strpairs.end(); ++iitr, ++ciitr) {
        cout<<"\n"<<iitr->first<<"\t"<<iitr->second;
        cout<<"\n"<<ciitr->first<<"\t"<<ciitr->second;
        check_expr(strcmp(ciitr->first, iitr->first) == 0 && strcmp(ciitr->second, iitr->second) == 0);
    }

    typedef db_map<char *, char *, ElementHolder<char *> > strpairs_t;
    typedef std::map<string, string> sstrpairs_t;
    sstrpairs_t sstrpairs2;
    strpairs_t strpairs2;
    rand_str_dbt randstr;
    
    for (i = 0; i < 100; i++) {
        string rdstr, rdstr2;
        randstr(dbt, rdstr);
        randstr(dbt, rdstr2);
        strpairs2[(char *)rdstr.c_str()] = (char *)rdstr2.c_str();
        sstrpairs2[rdstr] = rdstr2;
    }
    strpairs_t::iterator itr;
    strpairs_t::const_iterator citr;
    
    for (itr = strpairs2.begin(); itr != strpairs2.end(); ++itr) {
        check_expr(strcmp(strpairs2[itr->first], itr->second) == 0);
        check_expr(string(itr->second) == sstrpairs2[string(itr->first)]);
        strpairs_t::value_type_wrap::second_type&secref = itr->second;
        std::reverse((char *)secref, (char *)secref + strlen(secref));
        secref._DB_STL_StoreElement();
        std::reverse(sstrpairs2[itr->first].begin(), sstrpairs2[itr->first].end());
    }

    check_expr(strpairs2.size() == sstrpairs2.size());
    for (citr = strpairs2.begin(ReadModifyWriteOption::no_read_modify_write(), 
        true, BulkRetrievalOption::bulk_retrieval()); citr != strpairs2.end(); ++citr) {
        check_expr(strcmp(strpairs2[citr->first], citr->second) == 0);
        check_expr(string(citr->second) == sstrpairs2[string(citr->first)]);
    }

    
    if (EXPLICIT_TXN) 
        dbstl::commit_txn(penv);

    db_vector<const char *, ElementHolder<const char *> > csvct(10);
    vector<const char *> scsvct(10);
    const char *pconststr = "abc";
    for (i = 0; i < 10; i++) {
        scsvct[i] = pconststr;
        csvct[i] = pconststr;
        csvct[i] = scsvct[i];
        // scsvct[i] = csvct[i]; assignment won't work because scsvct 
        // only stores pointer but do not copy the sequence, thus it 
        // will refer to an invalid pointer when i changes.
    }
    for (i = 0; i < 10; i++) {
        check_expr(strcmp(csvct[i], scsvct[i]) == 0);
        cout<<endl<<(const char *)(csvct[i]);
    }

    db_vector<const wchar_t *, ElementHolder<const wchar_t *> > wcsvct(10);
    vector<const wchar_t *> wscsvct(10);
    const wchar_t *pconstwstr = L"abc";
    for (i = 0; i < 10; i++) {
        wscsvct[i] = pconstwstr;
        wcsvct[i] = pconstwstr;
        wcsvct[i] = wscsvct[i];
        // scsvct[i] = csvct[i]; assignment won't work because scsvct 
        // only stores pointer but do not copy the sequence, thus it 
        // will refer to an invalid pointer when i changes.
    }
    for (i = 0; i < 10; i++) {
        check_expr(wcscmp(wcsvct[i], wscsvct[i]) == 0);

    }
    
} // test_char_star_string_storage

void TestAssoc::test_fixed_len_obj_storage()
{
    int i;
    map<int, sms_t> ssmsmap;

    if (EXPLICIT_TXN) 
        dbstl::begin_txn(0, penv);
    
    typedef db_map<int, sms_t> smsmap_t;
    smsmap_t smsmap(dmdb6, penv);
    
    sms_t smsmsg;
    time_t now;
    for (i = 0; i < 2008; i++) {
        smsmsg.from = 1000 + i;
        smsmsg.to = 10000 - i;
        smsmsg.sz = sizeof(smsmsg);
        time(&now);
        smsmsg.when = now;
        ssmsmap.insert(make_pair(i, smsmsg));
        smsmap.insert(make_pair(i, smsmsg));
    }
    smsmap.clear();
    if (EXPLICIT_TXN) 
        dbstl::commit_txn(penv);
} //test_var_len_obj_storage

void TestAssoc::test_arbitray_sequence_storage()
{
    int i, j;

    if (EXPLICIT_TXN) 
        dbstl::begin_txn(0, penv);
    // storing arbitary sequence test .  
    cout<<endl<<"Testing arbitary type of sequence storage support...\n";
    RGBB *rgbs[10], *prgb1, *prgb2;
    typedef db_map<int, RGBB *, ElementHolder<RGBB *> > rgbmap_t;
    rgbmap_t rgbsmap(dmdb6, penv);
    
    map<int, RGBB *> srgbsmap;

    DbstlElemTraits<RGBB>::instance()->set_sequence_len_function(rgblen);
    DbstlElemTraits<RGBB>::instance()->set_sequence_copy_function(rgbcpy);
    // populate srgbsmap and rgbsmap
    for (i = 0; i < 10; i++) {
        n = abs(rand()) % 10 + 2;
        rgbs[i] = new RGBB[n];
        memset(&rgbs[i][n - 1], 0, sizeof(RGBB));//make last element 0
        for (j = 0; j < n - 1; j++) {
            rgbs[i][j].r_ = i + 128;
            rgbs[i][j].g_ = 256 - i;
            rgbs[i][j].b_ = 128 - i;
            rgbs[i][j].bright_ = 256 / (i + 1);
            
        }
        rgbsmap.insert(make_pair(i, rgbs[i]));
        srgbsmap.insert(make_pair(i, rgbs[i]));
    }

    // retrieve and assert equal, then modify and store
    for (i = 0; i < 10; i++) {
        rgbmap_t::value_type_wrap::second_type rgbelem = rgbsmap[i];
        prgb1 = rgbelem;
        check_expr(memcmp(prgb1, prgb2 = srgbsmap[i], 
            (n = (int)rgblen(srgbsmap[i])) * sizeof(RGBB)) == 0);
        for (j = 0; j < n - 1; j++) {
            prgb1[j].r_ = 256 - prgb1[j].r_;
            prgb1[j].g_ = 256 - prgb1[j].g_;
            prgb1[j].b_ = 256 - prgb1[j].b_;
            prgb2[j].r_ = 256 - prgb2[j].r_;
            prgb2[j].g_ = 256 - prgb2[j].g_;
            prgb2[j].b_ = 256 - prgb2[j].b_;
        }
        rgbelem._DB_STL_StoreElement();
    }

    // retrieve again and assert equal
    for (i = 0; i < 10; i++) {
        rgbmap_t::value_type_wrap::second_type rgbelem = rgbsmap[i];
        prgb1 = rgbelem;// Can't use rgbsmap[i] here because container::operator[] is an temporary value.;
        check_expr(memcmp(prgb1, prgb2 = srgbsmap[i], 
            sizeof(RGBB) * rgblen(srgbsmap[i])) == 0);
    }
    
    rgbmap_t::iterator rmitr;
    map<int, RGBB *>::iterator srmitr;

    for (rmitr = rgbsmap.begin();
        rmitr != rgbsmap.end(); ++rmitr) {
        rgbmap_t::value_type_wrap::second_type rgbelem2 = (*rmitr).second;
        prgb1 = (*rmitr).second;
        srmitr = srgbsmap.find(rmitr->first);
        check_expr(memcmp(prgb1, prgb2 = srmitr->second, 
            sizeof(RGBB) * rgblen(srmitr->second)) == 0);
        rmitr.refresh();
    }

    for (i = 0; i < 10; i++)
        delete []rgbs[i];
    if (EXPLICIT_TXN)
        dbstl::commit_txn(penv);
} // test_arbitray_sequence_storage

void TestAssoc::test_bulk_retrieval_read()
{

    int i;
    
    typedef db_map<int, sms_t> smsmap_t;
    smsmap_t smsmap(dmdb6, penv);
    map<int, sms_t> ssmsmap;
    if (EXPLICIT_TXN) 
        dbstl::begin_txn(0, penv);
        
    cout<<"\n_testing bulk retrieval support:\n";
    sms_t smsmsg;
    time_t now;
    smsmap.clear();
    for (i = 0; i < 2008; i++) {
        smsmsg.from = 1000 + i;
        smsmsg.to = 10000 - i;
        smsmsg.sz = sizeof(smsmsg);
        time(&now);
        smsmsg.when = now;
        ssmsmap.insert(make_pair(i, smsmsg));
        smsmap.insert(make_pair(i, smsmsg));
    }

    // bulk retrieval test. 
    map<int, sms_t>::iterator ssmsitr = ssmsmap.begin();
    i = 0;
    const smsmap_t &rosmsmap = smsmap;
    smsmap_t::const_iterator smsitr;
    for (smsitr = rosmsmap.begin(
        BulkRetrievalOption::bulk_retrieval());
        smsitr != smsmap.end(); i++) {
        // The order may be different, so if the two key set are 
        // identical, it is right.
        check_expr((ssmsmap.count(smsitr->first) == 1)); 
        check_expr((smsitr->second == ssmsmap[smsitr->first]));
        if (i % 2)
            smsitr++;
        else 
            ++smsitr; // Exercise both pre/post increment.
        if (i % 100 == 0)
            smsitr.set_bulk_buffer((u_int32_t)(smsitr.get_bulk_bufsize() * 1.1));
    }

    smsmap.clear();
    ssmsmap.clear();

    // Using db_vector. when moving its iterator sequentially to end(),
    // bulk retrieval works, if moving randomly, it dose not function
    // for db_vector iterators. Also, note that we can create a read only
    // iterator when using db_vector<>::iterator rather than 
    // db_vector<>::const_iterator.
    db_vector<sms_t> vctsms;
    db_vector<sms_t>::iterator itrv;
    vector<sms_t>::iterator sitrv;
    vector<sms_t> svctsms;
    for (i = 0; i < 2008; i++) {
        smsmsg.from = 1000 + i;
        smsmsg.to = 10000 - i;
        smsmsg.sz = sizeof(smsmsg);
        time(&now);
        smsmsg.when = now;
        vctsms.push_back(smsmsg);
        svctsms.push_back(smsmsg);
    }

    for (itrv = vctsms.begin(ReadModifyWriteOption::no_read_modify_write(), 
        true, BulkRetrievalOption::bulk_retrieval(64 * 1024)), 
        sitrv = svctsms.begin(), i = 0; itrv != vctsms.end();
        ++itrv, ++sitrv, ++i) {
        check_expr(*itrv == *sitrv);
        if (i % 100 == 0)
            itrv.set_bulk_buffer((u_int32_t)(itrv.get_bulk_bufsize() * 1.1));
    }

    if (EXPLICIT_TXN)
        dbstl::commit_txn(penv);

    
} //test_bulk_retrieval_read

void TestAssoc::test_nested_txns()
{
    int i;
    size_t sizet1;

    if (!EXPLICIT_TXN)
        return;// nested txn tests can't be run if container not txnal

    // nested transaction test
    // 1. many external txns nested, no internal txn. 
    
    typedef db_map<char*, char*, ElementHolder<char*> > strstrmap_t;
    DbTxn *txn1 = NULL, *txn2, *txn3;
    char kstr[10], dstr[10];
    std::map<int, string> kstrs;
    int commit_or_abort = 0;
    set<int> idxset;
    char *keystr;
 
    strstrmap_t smap(dbstrmap, penv);   
commit_abort:

#define _DB_STL_TEST_SET_TXN_NAME(txn)  \
    txn->set_name(commit_or_abort == 0 ? #txn"_commit" : #txn"_abort")

    txn1 = begin_txn(0, penv);
    smap.clear();
    //txn1->set_name(commit_or_abort == 0 ? "txn1_commit" : "txn1_abort");
    _DB_STL_TEST_SET_TXN_NAME(txn1);
    memset(kstr, 0, sizeof(kstr));
    memset(dstr, 0, sizeof(dstr));
    for (char cc = 'a'; cc <= 'z'; cc++) {
        _snprintf(kstr, 10, "%c%c%c", cc, cc, cc);
        for (int kk = 0; kk < 9; kk++)
            dstr[kk] = cc;
        
        i = cc - 'a';
        kstrs.insert(make_pair(i, string(kstr)));
        switch (i) {
        case 3:
            txn2 = begin_txn(0, penv);// nest txn2 into txn1
            _DB_STL_TEST_SET_TXN_NAME(txn2);
            break;
        case 6:// 
            abort_txn(penv); // abort txn2
            
            for (int kk = 3; kk < 6; kk++) {
                keystr = const_cast<char*>(kstrs[kk].c_str());
                check_expr(smap.count(keystr) == 0);
            }

            txn2 = begin_txn(0, penv);// nest txn2 into txn1
            _DB_STL_TEST_SET_TXN_NAME(txn2);
            break;
        case 9:// 6--11: txn3 abort and txn2 commit
            txn3 = begin_txn(0, penv);// nest txn3 into txn2, txn2 is in txn1
            _DB_STL_TEST_SET_TXN_NAME(txn3);
            break;
        case 12:
            abort_txn(penv);// abort txn3
            commit_txn(penv);// commit txn2, its txn3 part is not applied
            for (int kk = 6; kk < 12; kk++) {
                keystr = const_cast<char*>(kstrs[kk].c_str());
                if (kk < 9)
                    check_expr((sizet1 = smap.count(keystr)) > 0);
                if (kk >= 9)
                    check_expr(smap.count(keystr) == 0);
            }

            break;
        case 15:// 15--18: txn3 commit and txn2 abort
            txn2 = begin_txn(0, penv);
            _DB_STL_TEST_SET_TXN_NAME(txn2);
            break;
        case 17:
            txn3 = begin_txn(0, penv);
            _DB_STL_TEST_SET_TXN_NAME(txn3);
            break;
        case 19:
            commit_txn(penv);// commit txn3
            abort_txn(penv);// abort txn2;
            for (int kk = 15; kk < 19; kk++) {
                keystr = const_cast<char*>(kstrs[kk].c_str());
                check_expr(smap.count(keystr) == 0);
            }
            txn2 = begin_txn(0, penv);
            _DB_STL_TEST_SET_TXN_NAME(txn2);
            break;
        case 20:
            txn3 = begin_txn(0, penv);
            _DB_STL_TEST_SET_TXN_NAME(txn3);
            break;
        case 22:
    // txn3 is unresolved, after commit txn2, txn3 should have been commited
            commit_txn(penv, txn2);
            for (int kk = 19; kk < 22; kk++) {
                keystr = const_cast<char*>(kstrs[kk].c_str());
                check_expr(smap.count(keystr) > 0);
            }
            txn2 = begin_txn(0, penv);
            _DB_STL_TEST_SET_TXN_NAME(txn2);
            break;
        case 23:
            txn3 = begin_txn(0, penv);
            _DB_STL_TEST_SET_TXN_NAME(txn3);
            break;
        case 25:
    // txn3 is unresolved, after abort txn2, txn3 should have been aborted
            abort_txn(penv, txn2);
            for (int kk = 22; kk < 24; kk++) {
                keystr = const_cast<char*>(kstrs[kk].c_str());
                check_expr(smap.count(keystr) == 0);
            }
            
            break;

        default:
            break;

        }//switch
        
        smap.insert(make_pair(kstr, dstr));
    }// for

    if (commit_or_abort == 0) {
        
        // if txn1 commits,  0 1 2 6 7 8 12 13 14 19 20 21 25, 26 
        idxset.insert(0);
        idxset.insert(1);
        idxset.insert(2);
        idxset.insert(6);
        idxset.insert(7);
        idxset.insert(8);
        idxset.insert(12);
        idxset.insert(13);
        idxset.insert(14);
        idxset.insert(19);
        idxset.insert(20);
        idxset.insert(21);
        idxset.insert(25);
        
        
        
        for (int kk = 0; kk < 26; kk++) {
            keystr = const_cast<char*>(kstrs[kk].c_str());
            sizet1 = smap.count(keystr);
            if (idxset.count(kk) > 0) {
                check_expr(sizet1 > 0);
            } else {
                check_expr(sizet1 == 0);
            }
        }

        commit_or_abort = 1;
        smap.clear();
        commit_txn(penv, txn1);
        goto commit_abort;
    } 

    if (commit_or_abort != 0){
        abort_txn(penv, txn1);// non could have been inserted
        begin_txn(0, penv);// put this call in a txnal context
        check_expr((sizet1 = smap.size()) == 0);
        commit_txn(penv);
    }
     
    if (TEST_AUTOCOMMIT)
        dbstl::commit_txn(penv);
    

     return;
} // test_nested_txns


// Testing miscellaneous functions.
void TestAssoc::test_etc()
{
    set_global_dbfile_suffix_number(65536);
    DbEnv *penvtmp = dbstl::open_env(".", 0, DB_CREATE | DB_PRIVATE | DB_INIT_MPOOL);
    dbstl::close_db_env(penvtmp);
    db_mutex_t mtxhdl = dbstl::alloc_mutex();
    lock_mutex(mtxhdl);
    unlock_mutex(mtxhdl);
    free_mutex(mtxhdl);

    try {
    throw_bdb_exception("test_assoc::test_etc()", DB_LOCK_DEADLOCK);
    } catch (DbDeadlockException e1)
    {}

    try {
    throw_bdb_exception("test_assoc::test_etc()", DB_LOCK_NOTGRANTED);
    } catch (DbLockNotGrantedException e2)
    {}

    try {
    throw_bdb_exception("test_assoc::test_etc()", DB_REP_HANDLE_DEAD);
    } catch (DbRepHandleDeadException e13)
    {}

    try {
    throw_bdb_exception("test_assoc::test_etc()", DB_RUNRECOVERY);
    } catch (DbRunRecoveryException e4)
    {}

    try {
    throw_bdb_exception("test_assoc::test_etc()", 12345);
    } catch (DbException e1)
    {}

    try {
    throw_bdb_exception("test_assoc::test_etc()", 54321);
    } catch (DbException e1)
    {}

    ptint_vector vctr;
    vctr.set_all_flags(1, 1, 1);
    check_expr(vctr.get_commit_flags() == 1);
    check_expr(vctr.get_cursor_open_flags() == 1);
    check_expr(vctr.get_db_open_flags() & DB_CREATE);
    check_expr(vctr.get_txn_begin_flags() == 1);
    vctr.get_db_set_flags();
    vctr.set_commit_flags(2);
    check_expr(vctr.get_commit_flags() == 2);
    vctr.set_cursor_open_flags(2);
    check_expr(vctr.get_cursor_open_flags() == 2);
    vctr.set_txn_begin_flags(2);
    check_expr(vctr.get_txn_begin_flags() == 2);


}



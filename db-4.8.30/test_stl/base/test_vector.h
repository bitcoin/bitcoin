/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$ 
 */

#include "test.h"

#include <list>

using std::list;
using std::priority_queue;

// This class tests all methods of db_vector and db_vector_iterator
// Most tests are done by doing identical operations to db_vector
// and std::vector and check the result data set in the two vectors
// are identical. Also, almost all algorithms in std are applied to
// the different ranges created from db_vector and std::vector, and
// also compare result of the in the same way.
//
class TestVector
{
public:
    ~TestVector();
    TestVector(void *param1);
    void start_test()
    {

        tb.begin("db_vector");
        // Do not change the order of these functions.

        // Test each member function of db_vector and 
        // db_vector_iterator that are shared by all three 
        // std containers std::vector, std::deque and std::list.
        test_member_functions();

        // Test std::list specific methods.
        test_list_specific_member_functions();

        // Use std functions to manipulate db_vector containers.
        test_std_functions();

        // Test storing primitive data.
        test_primitive();

        // Test whether db_vector works with std container adapters 
        // std::stack, std::queue and std::priority_queue
        test_queue_stack();
        tb.end();
    }

    void test_member_functions();
    void test_list_specific_member_functions();
    void test_std_functions();
    void test_primitive();
    void test_queue_stack();
private:
    
    int n;
    int flags, setflags, EXPLICIT_TXN, TEST_AUTOCOMMIT;
    DBTYPE dbtype;
    DbEnv *penv;
    Db *db, *db2, *db3;
    Db *quedb, *pquedb;
    test_block tb;

    vector<int> svi, svi2, svi3;
    
};

TestVector::~TestVector()
{

}

TestVector::TestVector(void *param1)
{
    check_expr(param1 != NULL);
    TestParam *param = (TestParam*)param1;
    TestParam *ptp = param;
    penv = param->dbenv;
    db = db2 = db3 = NULL; 
    flags = 0, setflags = 0, EXPLICIT_TXN = 1, TEST_AUTOCOMMIT = 0;
    flags = param->flags;
    dbtype = param->dbtype;
    setflags = param->setflags;
    TEST_AUTOCOMMIT = param->TEST_AUTOCOMMIT;
    EXPLICIT_TXN = param->EXPLICIT_TXN;
    n = 5;
    dbtype = DB_BTREE;
    db = dbstl::open_db(penv, "db1.db", 
        DB_RECNO, DB_CREATE | ptp->dboflags | DB_THREAD, DB_RENUMBER);
    db2 = dbstl::open_db(penv, "db2.db", 
        DB_RECNO, DB_CREATE | ptp->dboflags, DB_RENUMBER);
    db3 = dbstl::open_db(penv, "db3.db", 
        DB_RECNO, DB_CREATE | ptp->dboflags, DB_RENUMBER);
    // NO DB_RENUMBER needed 
    quedb = dbstl::open_db(penv, "dbquedb.db", 
        DB_RECNO, DB_CREATE | ptp->dboflags | DB_THREAD, 0);
    pquedb = dbstl::open_db(penv, "dbpquedb.db", 
        DB_RECNO, DB_CREATE | ptp->dboflags | DB_THREAD, DB_RENUMBER);
}

void TestVector::test_member_functions()
{
    int i;
    ptint_vector::iterator itr1, itr;
    ptint_vector::reverse_iterator ritr1, ritr;
    vector<int>::iterator sitr, sitr1;
    vector<int>::reverse_iterator rsitr, rsitr1;
    
        
    ptint_vector vi(db, penv);
    const ptint_vector& cnstvi = vi;
    vi.clear();
    if ( EXPLICIT_TXN)
        dbstl::begin_txn(0, penv);

    fill(vi, svi, 0, n);
    
    for (i = 0; i < n; i++) {
        ptint_vector::const_iterator citr, citr1;
        citr = vi.begin() + i;
        check_expr(*citr == vi[i]);
        itr = citr;
        check_expr(*citr == *itr);
        *itr = i * 2 + 1;
        citr1 = itr;
        check_expr(*citr == *itr);
        check_expr(*citr == *citr1);
        check_expr(*citr == vi[i]);
    }

    for (i = 0; i < n; i++) {
        ptint_vector::const_iterator citr = vi.begin() + i;
        check_expr(*citr == vi[i]);
        itr = citr;
        check_expr(*citr == *itr);
        *itr = i * 2 + 1;
        ptint_vector::const_iterator citr1 = itr;
        check_expr(*citr == *itr);
        check_expr(*citr == *citr1);
        check_expr(*citr == vi[i]);
    }

    for (i = 0; i < n; i++) {
        ptint_vector::iterator ncitr, ncitr1;
        ptint_vector::const_iterator citr, citr1;

        ncitr = cnstvi.begin() + i;
        check_expr(*ncitr == cnstvi[i]);
        citr = ncitr;
        check_expr(*citr == *ncitr);
        //*ncitr = i * 2 + 1;
        citr1 = ncitr;
        ncitr1 = citr1;
        check_expr(*citr == *ncitr);
        check_expr(*citr == *ncitr1);
        check_expr(*citr == *citr1);
        check_expr(*citr == cnstvi[i]);
    }

    for (i = 0; i < n; i++) {

        ptint_vector::iterator ncitr = cnstvi.begin() + i;
        check_expr(*ncitr == cnstvi[i]);
        ptint_vector::const_iterator citr = ncitr;
        check_expr(*citr == *ncitr);
        //*itr = i * 2 + 1;
        ptint_vector::const_iterator citr1 = ncitr;
        ptint_vector::iterator ncitr1 = citr1;

        check_expr(*citr == *ncitr);
        check_expr(*citr == *ncitr1);
        check_expr(*citr == *citr1);
        check_expr(*citr == cnstvi[i]);
    }

    {
        vi.clear();
        svi.clear();
        vi.push_back(123);
        svi.push_back(123);
        ptint_vector::iterator ncitr = vi.begin();
        vector<int>::iterator ncsitr = svi.begin();
        *ncitr += 5;
        *ncitr -= 3;
        *ncitr *= 4;
        *ncitr /= 2;
        *ncitr %= 97;
        (*ncitr)++;
        (*ncitr)--;
        ++(*ncitr);
        --(*ncitr);
        *ncsitr += 5;
        *ncsitr -= 3;
        *ncsitr *= 4;
        *ncsitr /= 2;
        *ncsitr %= 97;
        (*ncsitr)++;
        (*ncsitr)--;
        ++(*ncsitr);
        --(*ncsitr);
        check_expr(*ncsitr == *ncitr);

        *ncitr &= 0x9874abcd;
        *ncitr |= 0x12345678;
        *ncitr ^= 0x11223344;
        *ncsitr &= 0x9874abcd;
        *ncsitr |= 0x12345678;
        *ncsitr ^= 0x11223344;
        ncitr.refresh(true);
        check_expr(*ncsitr == *ncitr);

        vi.clear();
        svi.clear();
        db_vector<ptype<int> > tmpvi;
        const db_vector<ptype<int> > & ctmpvi = tmpvi;
        fill(tmpvi, svi, 0, n);
        db_vector<ptype<int> >::const_reverse_iterator critr;
        vector<int>::const_reverse_iterator crsitr;
        db_vector<ptype<int> >::const_iterator citr2;
        vector<int>::const_iterator csitr2;

        check_expr(ctmpvi.back() == svi.back());
        check_expr(ctmpvi.at(0) == svi.at(0));
        critr = tmpvi.rbegin();//XXXXXXXXX modify db_reverse_iterator, use twin type
        for (i = n - 1, crsitr = svi.rbegin(); critr != tmpvi.rend(); ++critr, ++crsitr, i--) {
            check_expr(crsitr < crsitr + 1);
            check_expr(critr < critr + 1);
            check_expr(critr <= critr + 1);
            
                
            check_expr(critr > critr - 1);
            check_expr(critr >= critr - 1);
            if (i > 0) {
                ptype<int> tmpv1 = critr[1];
                int tmpv2 = crsitr[1];
                check_expr(tmpv1 == tmpv2);
            }
        
            check_expr(critr <= 2 + critr);
            //if (i < n - 2)
            check_expr(critr >= -2 + critr);
            check_expr(*critr == *crsitr);  
        }

        for (critr = ctmpvi.rbegin(), crsitr = svi.rbegin(), i = 0; critr != ctmpvi.rend(); critr += 3, crsitr += 3, i++) {
            check_expr(*critr == *crsitr);
            if (i > 0) {
                critr -= 2;
                crsitr -= 2;
            }
            check_expr(*critr == *crsitr);
        }

        for (citr2 = tmpvi.begin(), csitr2 = svi.begin(), i = 0; citr2 != tmpvi.rend(); ++citr2, ++csitr2, i++) {
            check_expr(citr2 < citr2 + 1);
            check_expr(citr2 <= citr2 + 1); 
            check_expr(citr2 > citr2 - 1);
            check_expr(citr2 >= citr2 - 1);
            check_expr(citr2 <= 2 + citr2);

            if (i > 1)
                check_expr(citr2 >= -2 + citr2);
            check_expr(*citr2 == *csitr2);
            if (i < n - 1)
                check_expr(citr2[1] == csitr2[1]);
        }

        for (citr2 = ctmpvi.begin(), csitr2 = svi.begin(), i = 0; citr2 != ctmpvi.rend(); citr2 += 3, csitr2 += 3, i++) {
            citr2.refresh(true);
            check_expr(*citr2 == *csitr2);
            if (i > 0) {
                citr2 -= 2;
                csitr2 -= 2;
            }
            check_expr(*citr2 == *csitr2);
            check_expr(citr2->v == *csitr2);
        }
        citr2 = ctmpvi.begin() + 2;
        csitr2 = svi.begin() + 2;
        check_expr(*citr2 == *csitr2);
        citr2--;
        csitr2--;
        check_expr(*citr2 == *csitr2);
        --citr2;
        --csitr2;
        check_expr(*citr2 == *csitr2);

        db_vector<ptype<int> > tmpvi2;
        vector<int> tmpsvi2;
        tmpvi2.insert(tmpvi2.begin(), tmpvi.begin() + 1, tmpvi.end() - 1);
        //tmpsvi2.insert(svi.begin(), svi.begin() + 1, svi.end() - 1); c++ stl does  not support this
        for (i = 1; i < (int)tmpvi.size() - 1; i++)
            tmpsvi2.push_back(tmpvi[i]);
        check_expr(is_equal(tmpvi2, tmpsvi2));

        tmpvi2.clear(false);
        tmpsvi2.clear();

        tmpvi2.insert(tmpvi2.end(), tmpvi.begin(), tmpvi.end());
        tmpsvi2.insert(tmpsvi2.end(), svi.begin(), svi.end());
        check_expr(is_equal(tmpvi2, tmpsvi2));

        tmpvi2.insert(tmpvi2.end() - 3, tmpvi.begin() + 2, tmpvi.end() - 1);
        tmpsvi2.insert(tmpsvi2.end() - 3, svi.begin() + 2, svi.end() - 1);
        check_expr(is_equal(tmpvi2, tmpsvi2));

        db_vector<char, ElementHolder<char> > charvec(n);
        for (i = 0; i < n; i++)
            charvec[i] = (char)i;
        db_vector<char, ElementHolder<char> > charvec3(charvec);
        const db_vector<char, ElementHolder<char> > &ccharvec3 = charvec3;
        check_expr(ctmpvi == charvec);
        charvec[n / 2] *= 2;
        check_expr(ctmpvi != charvec);
        charvec.push_back(127);
        check_expr(ctmpvi != charvec);
        charvec.resize(charvec.size());
        charvec.assign(ccharvec3.begin(), ccharvec3.end());
        check_expr(charvec == ccharvec3);
        charvec.clear(false);
        charvec.insert(charvec.begin(), ccharvec3.begin(), ccharvec3.end());
        charvec.insert(charvec.begin(), 113);
        db_vector<char, ElementHolder<char> > charvec2(5);
        charvec2.assign(charvec.begin(), charvec.end(), false);
        check_expr((charvec2 != charvec) == false);

        db_vector<string> tmpvi3, tmpvi4;
        db_vector<char *, ElementHolder<char *> > tmpvi5, tmpvi6;

        std::list<string> tmpsvi3, tmpsvi4;
        char tmpbuf[4];
        tmpbuf[3] = '\0';
        int exec_cnt = 0;
doagain:
        for (i = 0; i < 64; i++) {
            tmpbuf[0] = 'a' + rand() % 3;
            tmpbuf[1] = 'a' + rand() % 3;
            tmpbuf[2] = 'a' + rand() % 3;
            tmpvi3.push_back(string(tmpbuf));
            tmpsvi3.push_back(string(tmpbuf));
            tmpvi5.push_back(tmpbuf);
        }

        // sort
        if (exec_cnt == 0) {
            tmpvi5.sort(strlt0);
            tmpvi3.sort(strlt);
            tmpsvi3.sort(strlt);
        } else {
            tmpvi5.sort();
            tmpvi3.sort();
            tmpsvi3.sort();
        }
        check_expr(is_equal(tmpvi3, tmpsvi3));
        check_expr(is_equal(tmpvi5, tmpsvi3));

        // unique
        if (exec_cnt == 0) {
            tmpvi5.unique(streq0);
            tmpvi3.unique(streq);
            tmpsvi3.unique(streq);
        } else {
            tmpvi5.unique();
            tmpvi3.unique();
            tmpsvi3.unique();
        }
        check_expr(is_equal(tmpvi3,tmpsvi3));
        check_expr(is_equal(tmpvi5, tmpsvi3));

        // merge
        for (i = 0; i < 64; i++) {
            tmpbuf[0] = 'b' + rand() % 3;
            tmpbuf[1] = 'b' + rand() % 3;
            tmpbuf[2] = 'b' + rand() % 3;
            tmpvi4.push_back(string(tmpbuf));
            tmpsvi4.push_back(string(tmpbuf));
            tmpvi6.push_back(tmpbuf);
        }

        if (exec_cnt == 0) {
            tmpvi6.sort(strlt0);
            tmpvi4.sort(strlt);
            tmpsvi4.sort(strlt);
            tmpvi5.merge(tmpvi6, strlt0);
            tmpvi3.merge(tmpvi4, strlt);
            tmpsvi3.merge(tmpsvi4, strlt);
        } else {
            tmpvi6.sort();
            tmpvi4.sort();
            tmpsvi4.sort();
            tmpvi5.merge(tmpvi6);
            tmpvi3.merge(tmpvi4);
            tmpsvi3.merge(tmpsvi4);
        }
        check_expr(is_equal(tmpvi3, tmpsvi3));
        check_expr(is_equal(tmpvi5, tmpsvi3));

        exec_cnt++;
        if (exec_cnt == 1) {
            tmpvi3.clear();
            tmpvi4.clear();
            tmpvi5.clear();
            tmpvi6.clear();
            tmpsvi3.clear();
            tmpsvi4.clear();
            goto doagain;
        }
        vi.clear();
        svi.clear();
    }

    vi.clear();
    // tested: push_back, operator!= , db_vector::size, 
    fill(vi, svi, 0, n);
    pprint(vi);
    check_expr(is_equal(vi, svi) == true);
    ptint tmp;

    // tested functions: 
    // db_vector<>::begin, end; iterator::operator ++/*/=, 
    // iterator copy constructor, RandDbc<>::operator=, 
    // RandDbc<>(const RandDbc&), 
    // T(const RandDbc<T>&), 
    // RandDbc<T>::operator=(const RandDbc<T>&), 
    // and also the reverse iterator versions of these functions/classes
    //
    for(itr1 = vi.begin(), itr = itr1, sitr1 = svi.begin(), 
        sitr = sitr1, i = 0; 
        itr1 != vi.end() && sitr1 != svi.end(); itr1++, sitr1++, i++) {
        
        *itr = (tmp = ptint(*itr1) * 2);
        cout<<"\n*itr = "<<(ptint(*itr))<<"\ttmp = "<<tmp;
        itr = itr1;
        *sitr = (*sitr1) * 2;
        sitr = sitr1;
    }
    pprint(vi);
    check_expr(is_equal(vi, svi) == true);
    size_t ii;
    for(itr1 = vi.begin(), sitr1 = svi.begin(), ii = 0; 
        ii < svi.size(); ii++) {
        itr1[ii] = 2 * itr1[ii];
        sitr1[ii] = 2 * sitr1[ii];
    }
    pprint(vi);
    check_expr(is_equal(vi, svi) == true);
    //itr.close_cursor();
    if ( EXPLICIT_TXN) {
        
        dbstl::commit_txn(penv);
        dbstl::begin_txn(0, penv);
    }
    for(ritr1 = vi.rbegin(), ritr = ritr1, rsitr1 = svi.rbegin(), 
        rsitr = rsitr1, i = 0; 
        ritr1 != vi.rend() && rsitr1 != svi.rend(); 
        ritr1++, rsitr1++, i++) {
        
        *ritr = (tmp = (*ritr1 * 2));
        cout<<"\n*itr = "<<(ptint(*ritr))<<"\ttmp = "<<tmp;
        ritr = ritr1;
        *rsitr = *rsitr1 * 2;
        rsitr = rsitr1;
    }

    if (EXPLICIT_TXN) {
        
        dbstl::commit_txn(penv);
        dbstl::begin_txn(0, penv);
    }
    pprint(vi);
    check_expr(is_equal(vi, svi) == true);

    for(ritr1 = vi.rbegin(), ritr = ritr1, rsitr1 = svi.rbegin(), 
        rsitr = rsitr1, i = 0; 
        ritr1 != vi.rend() && rsitr1 != svi.rend(); 
        ritr1++, rsitr1++, i++) {
        
        *ritr = (tmp = ptint(*ritr1) * 2);
        cout<<"\n*itr = "<<(ptint(*ritr))<<"\ttmp = "<<tmp;
        ritr = ritr1;
        *rsitr = (*rsitr1) * 2;
        rsitr = rsitr1;
    }

    pprint(vi);
    check_expr(is_equal(vi, svi) == true);
    if (EXPLICIT_TXN){
        dbstl::commit_txn(penv);
        dbstl::begin_txn(0, penv);
    }
    for(itr1 = vi.begin(), itr = itr1, sitr1 = svi.begin(), 
        sitr = sitr1, i = 0; 
        itr1 != vi.end() && sitr1 != svi.end(); 
        itr1++, sitr1++, i++) {
        
        *itr = (tmp = ptint(*itr1) * 2);
        cout<<"\n*itr = "<<(ptint(*itr))<<"\ttmp = "<<tmp;
        itr = itr1;
        *sitr = (*sitr1) * 2;
        sitr = sitr1;
    }

    itr.close_cursor();
    pprint(vi);
    check_expr(is_equal(vi, svi) == true);
    if (EXPLICIT_TXN) {
        
        dbstl::commit_txn(penv);
        dbstl::begin_txn(0, penv);
    }

    for (i = 0; i< n - 1; i++) {
        vi[i] = ptint(vi[i]) * 2;
        vi[i] = TOINT( ptint(vi[i]) -  (ptint(vi[i + 1]) * 2));

        svi[i] = svi[i] * 2;
        svi[i] = TOINT(svi[i] - svi[i + 1] * 2);

    }
    pprint(vi);
    check_expr(is_equal(vi, svi) == true);
    if (EXPLICIT_TXN)
        dbstl::commit_txn(penv);
    if (!TEST_AUTOCOMMIT)
        dbstl::begin_txn(0, penv);
    
    // testing pop_back and back()
    ptint t;
    for (i = n - 1; i >= 0; i--) {
        check_expr(ptint(vi.back()) == svi[i]);
        vi.pop_back();
    }

    list<int> lvi;
    list<int>::iterator lvit;
    for (i = 0; i < n; i++) {
        lvi.push_front(i);
        vi.push_front(i);
    }
    pprint(vi, "inserted by push_front");
    for (i = 0; i < n; i++) {
        pprint(vi, "pop front :");
        check_expr(ptint(vi.front()) == lvi.front());
        vi.pop_front();
        lvi.pop_front();
    }
    fill(vi, svi, 0, n);
    if ( !TEST_AUTOCOMMIT) 
        dbstl::commit_txn(penv);
    if (EXPLICIT_TXN)
        dbstl::begin_txn(0, penv);
    
    //testing front(), erase()  
    cout<<"front test\n";
    for (i = 0, itr = vi.begin(); i < n; i++) {
        pprint(vi);
        check_expr(ptint(vi.front()) == svi[i]); 
        if (i % 2) // try both
            itr = vi.erase(itr);
        else {
            vi.pop_front(); // here pop_front is also OK
            itr = vi.begin();
        }
    }
    itr.close_cursor();
    fill(vi, svi, 0, n);

    
    /*
    * Currently we can only use a ref to ref the *itr and use 
    * the ref UNTIL we store changes, rahter than
    * use *itr multiple times, because iterator::operator* will erase 
    * changed data members.  this limitation won't influence std 
    * algorithms because they all manipulate the elements in their 
    * whole, never access their members.
    */
    for (itr1 = vi.begin(), itr = itr1, 
        sitr1 = svi.begin(), sitr = sitr1, i = 0; 
        itr1 != vi.end() && sitr1 != svi.end();i++, itr1++, sitr1++) {

        if (i % 2 == 0) {
            ptint_vector::iterator::reference itrref = (*itr);
            t = itrref;
            check_expr( t == *sitr);
            itrref = (t) * 3;
            (*sitr) = 3 * (*sitr);
            itrref._DB_STL_StoreElement();
            itr = itr1;
            sitr = sitr1;
            
        } else {
            ptint_vector::reference itrref = vi[i];
            t = itrref;
            check_expr(t == svi[i]);
            itrref = t * 3;
            svi[i] = 3 * svi[i];
            itrref._DB_STL_StoreElement();
        }

    }
    pprint(vi);
    check_expr(is_equal(vi, svi) == true);
    
    ptint_vector vi2(db2, penv);
    vi2.clear();
    
    vi2.insert(vi2.end(), 1024);
    svi2.insert(svi2.end(), 1024);
    check_expr(is_equal(vi2, svi2) == true);
    if ( EXPLICIT_TXN) 
        dbstl::commit_txn(penv);
    if (!TEST_AUTOCOMMIT)
        dbstl::begin_txn(0, penv);
    pprint(vi, "vi before swapping: ");
    fill(vi2, svi2, 6, 9);
    check_expr(vi != vi2);
    pprint(vi, "vi before swapping2:");
    pprint(vi2, "vi2 before swapping: ");
    vi.swap(vi2);
    check_expr(vi != vi2);
    svi.swap(svi2);
    pprint(vi, "vi:");
    pprint(vi2, "vi2:");
    check_expr(is_equal(vi, svi) == true);
    check_expr(is_equal(vi2, svi2) == true);
    if (!TEST_AUTOCOMMIT)
        dbstl::commit_txn(penv);
    if (EXPLICIT_TXN)
        dbstl::begin_txn(0, penv);
    cout<<"\ntesting db container's insert method...";
    vi.insert(vi.end(), 1024);
    svi.insert(svi.end(), 1024);
    vi.insert(vi.begin(), vi2.begin(), vi2.end());
    svi.insert(svi.begin(), svi2.begin(), svi2.end());
    vi.insert(vi.begin() + 3, 10101);
    svi.insert(svi.begin() + 3, 10101);
    vi.insert(vi.end(), (size_t)10, ptint (987));
    svi.insert(svi.end(), 10, 987);
    check_expr(is_equal(vi, svi) == true);
    vi.insert(vi.end(), vi2.begin() + 3, vi2.end());
    svi.insert(svi.end(), svi2.begin() + 3, svi2.end());
    pprint(vi, "\nvi after inserting a range and 10 identical numbers");
    check_expr(is_equal(vi, svi) == true);

    vi.assign(vi2.begin(), vi2.end());
    svi.assign(svi2.begin(), svi2.end());
    check_expr(is_equal(vi, svi) == true);
    
    vi.assign(vi2.begin(), vi2.end());
    svi.assign(svi2.begin(), svi2.end());
    check_expr(is_equal(vi, svi) == true);
    pprint(vi, "vi before assigning");

    vi.assign(vi2.begin() + 1, vi2.begin() + 3);
    svi.assign(svi2.begin() + 1, svi2.begin() + 3);
    check_expr(is_equal(vi, svi) == true);
    vi.assign((size_t)10, ptint (141));
    svi.assign(10, 141);
    check_expr(is_equal(vi, svi) == true);

    vi.insert(vi.begin(), vi2.begin(), vi2.end());
    svi.insert(svi.begin(), svi2.begin(), svi2.end());

    vi.resize(vi.size() / 2);
    svi.resize(svi.size() / 2);
    check_expr(is_equal(vi, svi) == true);
    vi.resize(vi.size() * 4);
    svi.resize(svi.size() * 4);
    check_expr(is_equal(vi, svi) == true);

    for (int ui = 0; ui < (int)svi.size(); ui++) {
        svi[ui] = ui;
        vi[ui] = ui;
    }

    itr = vi.begin() + 3;
    itr1 = vi.begin() + vi.size() - 8;
    sitr = svi.begin() + 3;
    sitr1 = svi.begin() + svi.size() - 8;
    pprint(vi, "vi before erasing range: ");
    vi.erase(itr, itr1);
    svi.erase(sitr, sitr1);
    check_expr(is_equal(vi, svi) == true);

    vi.clear();
    svi.clear();
    if ( EXPLICIT_TXN) 
        dbstl::commit_txn(penv);
} // test_member_functions

// Testing list specific methods.   
void TestVector::test_list_specific_member_functions()
{
    int i;
    std::list<ptint> slvi;
    std::list<ptint>::iterator slitr;
    ptint_vector vi(db, penv);

    if (EXPLICIT_TXN)
        dbstl::begin_txn(0, penv);
    fill(vi, svi, 1,10);
    for (i = 1; i < 11; i++)
        slvi.push_back(i);
    pprint(vi, "vi after fill: ");
    vi.push_back(8);
    vi.push_back(1);
    vi.push_front(2);
    vi.push_front(100);
    vi.insert(vi.begin() + 4, 7);
    slvi.push_back(8);
    slvi.push_back(1);
    slvi.push_front(2);
    slvi.push_front(100);
    slitr = slvi.begin();
    std::advance(slitr, 4);
    slvi.insert(slitr, 7);
    pprint(vi, "vi after push_back: ");

    vi.remove(8);
    slvi.remove(8);
    pprint(vi, "vi after remove(8): ");
    check_expr(is_equal(vi, slvi));
    vi.remove_if(is7);
    slvi.remove_if(is7);
    check_expr(is_equal(vi, slvi));

    vi.reverse();
    slvi.reverse();
    check_expr(is_equal(vi, slvi));

    vi.sort();
    slvi.sort();
    check_expr(is_equal(vi, slvi));

    vi.push_back(100);
    slvi.push_back(100);
    vi.push_front(1);
    slvi.push_front(1);
    pprint(vi, "vi before vi.unique():");
    vi.unique();
    slvi.unique();
    pprint(vi, "vi after vi.unique():");
    check_expr(is_equal(vi, slvi));
    
    ptint_vector vi3(db3, penv);
    list<ptint> slvi3;

    fill(vi3, svi, 4, 15);
    for (i = 4; i < 19; i++)
        slvi3.push_back(i);

    vi.merge(vi3);
    slvi.merge(slvi3);
    check_expr(is_equal(vi, slvi));
    pprint(vi3, "vi3 after merge:");
    check_expr(is_equal(vi3, slvi3));

    vi3.clear();
    slvi3.clear();
    svi.clear();
    fill(vi3, svi, 4, 15);
    for (i = 4; i < 19; i++)
        slvi3.push_back(i);

    list<ptint>::iterator slitr2;
    slitr = slvi3.begin();
    std::advance(slitr, 4);
    slitr2 = slvi.begin();
    std::advance(slitr2, 6);

    pprint(vi, "vi before splice :");
    vi.splice(vi.begin() + 6, vi3, vi3.begin() + 4);
    slvi.splice(slitr2, slvi3, slitr);
    pprint(vi, "vi after splice :");
    check_expr(is_equal(vi, slvi));
    pprint(vi3, "vi3 after splice :");
    check_expr(is_equal(vi3, slvi3));

    slitr = slvi3.begin();
    std::advance(slitr, 5);
    vi.splice(vi.begin(), vi3, vi3.begin() + 5, vi3.end());
    slvi.splice(slvi.begin(), slvi3, slitr, slvi3.end());
    pprint(vi, "vi after splice :");
    pprint(vi3, "vi3 after splice :");
    check_expr(is_equal(vi, slvi));
    check_expr(is_equal(vi3, slvi3));

    vi.splice(vi.end(), vi3);
    slvi.splice(slvi.end(), slvi3);
    pprint(vi, "vi after splice :");
    pprint(vi3, "vi3 after splice :");
    check_expr(is_equal(vi, slvi));
    check_expr(is_equal(vi3, slvi3));
    
    vi.clear();
    vi3.clear();

    check_expr(!(vi < vi3));
    fill(vi, svi, 1, 10);
    
    fill(vi3, svi3, 1, 10);
    check_expr(!(vi < vi3));
    vi.push_front(0);
    check_expr((vi < vi3));
    vi3.push_front(0);
    vi.push_back(10);
    check_expr((vi3 < vi));
    vi.erase(vi.begin());
    check_expr((vi3 < vi));

    if ( EXPLICIT_TXN) 
        dbstl::commit_txn(penv);
    
}

// Use std algorithms to manipulate dbstl containers.
void TestVector::test_std_functions()
{
    ptint_vector::iterator itr1, itr;
    ptint_vector::reverse_iterator ritr1, ritr;
    vector<int>::iterator sitr, sitr1;
    vector<int>::reverse_iterator rsitr, rsitr1;
    ptint_vector vi(db, penv);
    ptint_vector vi2(db2, penv);
    ptint_vector vi3(db3, penv);
    // test db_vector by applying it to each and every 
    // function in std algorithm
    //
    // for_each
    // here after vi is supposed to be 
    // {6, 7, 8, 9, 10, 11, 12, 13, 14}
    //
    if (!TEST_AUTOCOMMIT)
        dbstl::begin_txn(0, penv);  
    vi.clear();
    svi.clear();
    fill(vi, svi, 6, 9);
    if (!TEST_AUTOCOMMIT)
        dbstl::commit_txn(penv);

    if (EXPLICIT_TXN)
        dbstl::begin_txn(0, penv);
    square squareobj;
    cout<<"\nfor_each begin to end\n";
    for_each(vi.begin(), vi.end(), squareobj);
    cout<<endl;
    for_each(svi.begin(), svi.end(), squareobj);
    cout<<"\nfor_each begin +2 to begin +6\n";
    for_each(vi.begin() + 2, vi.begin() + 6, squareobj);
    cout<<endl;
    for_each(svi.begin() + 2, svi.begin() + 6, squareobj);

    vector<int>::difference_type ssi = svi.rend() - svi.rbegin();
    ptint_vector::difference_type si;
    si = vi.rend() - vi.rbegin();
    check_expr(ssi == si);
    vector<int>::reverse_iterator sj = svi.rbegin() + 3;
    cout<<"si, sj ="<<si<<'\t'<<*sj;

    //find
    ptint tgt(12);
    ptint_vector::iterator vpos = 
        find(vi.begin(), vi.end(), tgt);
    check_expr(vpos != vi.end() && (ptint(*vpos) == tgt));
    vpos = find(vi.begin() + 3, vi.begin() + 7, tgt);
    check_expr(vpos != vi.end() && (ptint(*vpos) == tgt));
    tgt = -123;
    vpos = find(vi.begin(), vi.end(), tgt);
    check_expr(vpos == vi.end());
    
    // find_end
    int subseq[] = {8, 9, 10};
#ifdef WIN32
    // C++ STL specification requires this equivalence relation hold for
    // any iterators:
    // "itr1==itr2" <===> "&*itr1 == &*itr2"
    // this means that each iterator don't hold value for themselvs, 
    // but refer to a shared piece of memory held by its container object,
    // which is not true for dbstl. The impl of find_end in gcc's stl lib
    // relies on this requirement, but not for MS's stl lib. So this test
    // only runs on WIN32.
    pprint(vi, "vi before find_end(): ");
    vpos = find_end(vi.begin(), vi.end(), vi.begin(), vi.begin() + 1);
    check_expr(vpos == vi.begin());
    vpos = find_end(vi.begin(), vi.end(), vi.begin() + 3, 
        vi.begin() + 5);
    check_expr(vpos == vi.begin() + 3);
    vpos = find_end(vi.begin() + 1, vi.end(), subseq, subseq + 3);
    check_expr(vpos == vi.begin() + 2);
#endif
    //search
    vpos = search(vi.begin(), vi.end(), vi.begin(), vi.begin() + 1);
    check_expr(vpos == vi.begin());
    vpos = search(vi.begin(), vi.end(), vi.begin() + 3, vi.begin() + 5);
    check_expr(vpos == vi.begin() + 3);
    vpos = search(vi.begin() + 1, vi.end(), subseq, subseq + 3);
    check_expr(vpos == vi.begin() + 2);

    // find_first_of
    vpos = find_first_of(vi.begin(), vi.end(), vi.begin(), 
        vi.begin() + 1);
    check_expr(vpos == vi.begin());
    vpos = find_first_of(vi.begin() + 2, vi.begin() + 6, 
        vi.begin() + 4, vi.begin() + 5);
    check_expr(vpos == vi.begin() + 4);
    int subseq2[] = {9, 8, 10, 11};
    vpos = find_first_of(vi.begin() + 1, vi.begin() + 7, 
        subseq2, subseq2 + 4);
    check_expr(vpos == vi.begin() + 2);
    vpos = find_first_of(vi.begin() + 6, vi.end(), 
        subseq2, subseq2 + 4);
    check_expr(vpos == vi.end());

    //find_if
    vpos = find_if(vi.begin(), vi.end(), is2digits);
    check_expr(ptint(*vpos) == 10);
    vpos = find_if(vi.begin() + 5, vi.begin() + 8, is2digits);
    check_expr(ptint(*vpos) == 11);
    vpos = find_if(vi.begin() + 1, vi.begin() + 3, is2digits);
    check_expr(vpos == vi.begin() + 3);

    // count_if
    ptint_vector::difference_type oddcnt = 
        count_if(vi.begin(), vi.end(), is_odd);
    check_expr(oddcnt == 4);
    oddcnt = count_if(vi.begin(), vi.begin() + 5, is_odd);
    check_expr(oddcnt == 2);
    oddcnt = count_if(vi.begin(), vi.begin(), is_odd);
    check_expr(oddcnt == 0);
    
    // mismatch
    pair<intvec_t::iterator , int*> resmm = mismatch(vi.begin(),
        vi.begin() + 3, subseq);
    check_expr(resmm.first == vi.begin() && resmm.second == subseq);
    resmm = mismatch(vi.begin() + 2, vi.begin() + 5, subseq);
    check_expr(resmm.first == vi.begin() + 5 && resmm.second == subseq + 3);
    resmm = mismatch(vi.begin() + 2, vi.begin() + 4, subseq);
    check_expr(resmm.first == vi.begin() + 4 && resmm.second == subseq + 2);

    //equal
    check_expr (equal(vi.begin(), vi.begin() + 3, subseq) == false);
    check_expr(equal(vi.begin() + 2, vi.begin() + 5, subseq) == true);
    check_expr(equal(vi.begin() + 2, vi.begin() + 4, subseq) == true);
    if ( EXPLICIT_TXN) {
        dbstl::commit_txn(penv);
        dbstl::begin_txn(0, penv);
    }
    //copy
    pprint(vi2, "\nvi2 before copying");
    pprint(vi, "\nvi1 before copying");
    copy(vi.begin(), vi.begin() + 3, vi2.begin() + 1);
    copy(svi.begin(), svi.begin() + 3, svi2.begin() + 1);
    // vi2 should be 0, 6, 7, 8,4
    int vi22[] = { 0, 6, 7, 8,4};
    pprint(vi2, "\nvi2 after copying");
    check_expr(equal(vi2.begin(), vi2.end(), vi22) == true);
    check_expr(is_equal(vi, svi));

    // copy_backward
    copy_backward(vi.begin(), vi.begin() + 3, vi.begin() + 8);
    copy_backward(svi.begin(), svi.begin() + 3, svi.begin() + 8);
    pprint(vi);// should be 6,7,8,9,10,6,7,8,14
    check_expr(is_equal(vi, svi));
    
    copy_backward(vi.begin(), vi.begin() + 4, vi.begin() + 6);
    copy_backward(svi.begin(), svi.begin() + 4, svi.begin() + 6);
    pprint(vi);// should be 6,7, 6,7, 8, 9, 7, 8, 14
    check_expr(is_equal(vi, svi));
    copy_backward(vi.begin() + 7, vi.end(), vi.begin() + 5);
    copy_backward(svi.begin() + 7, svi.end(), svi.begin() + 5);
    pprint(vi);//should be 6,7,6,8,14,8,14,8,14
    check_expr(is_equal(vi, svi)); //here the check_expr fails but I think 
    //the values in vi are right, those in svi are wrong
    

    // swap_ranges
    swap_ranges(vi.begin() + 3, vi.begin() + 7, vi.begin() + 4);
    swap_ranges(svi.begin() + 3, svi.begin() + 7, svi.begin() + 4);
    check_expr(is_equal(vi, svi));

    /* 
     * std::swap can only swap data of same type, so following tests 
     * do not compile, we can provide our own swap(T, ElementRef<T>)
     * func to address this
     */
    vector<ptint> spvi, spvi2;
    spvi.insert(spvi.begin(), svi.begin(), svi.end());
    spvi2.insert(spvi2.begin(), svi2.begin(), svi2.end());
    swap_ranges(vi.begin() + 3, vi.begin() + 5, spvi.begin() + 6);
    swap_ranges(spvi.begin() + 3, spvi.begin() + 5, vi.begin() + 6);
    check_expr(is_equal(vi, spvi));

    swap_ranges(spvi.begin() + 3, spvi.begin() + 7, vi.begin() + 5);
    //swap back
    swap_ranges(spvi.begin() + 3, spvi.begin() + 7, vi.begin() + 5);
    check_expr(is_equal(vi, spvi));

    transform(vi2.begin(), vi2.end(), vi.begin() + 2,
        vi.begin() + 4, addup);
    transform(spvi2.begin(), spvi2.end(), spvi.begin() + 2, 
        spvi.begin() + 4, addup);
    check_expr(is_equal(vi, spvi));

    replace(vi.begin(), vi.end(), 8, 88);
    replace(spvi.begin(), spvi.end(), 8, 88);
    check_expr(is_equal(vi, spvi));

    generate(vi.begin(), vi.begin() + 2, randint);
    generate(spvi.begin(), spvi.begin() + 2, randint);
    pprint(vi);
    check_expr(is_equal(vi, spvi));

    remove(vi.begin(), vi.end(), -999);
    remove(spvi.begin(), spvi.end(), -999);
    pprint(vi, "\nafter remove");
    check_expr(is_equal(vi, spvi));

    reverse(vi.begin(), vi.end());
    reverse(spvi.begin(), spvi.end());
    pprint(vi, "\nafter reverse");
    check_expr(is_equal(vi, spvi));

    rotate(vi.begin() + 1, vi.begin() + 5, vi.end());
    rotate(spvi.begin() + 1, spvi.begin() + 5, spvi.end());
    pprint(vi, "\n after rotate");
    check_expr(is_equal(vi, spvi));

    separator<ptint> part;
    part.mid = 11;
    partition(vi.begin(), vi.end(), part);
    partition(spvi.begin(), spvi.end(), part);
    pprint(vi, "\nafter partition");
    check_expr(is_equal(vi, spvi));

    sort(vi.begin(), vi.end());
    sort(spvi.begin(), spvi.end());
    check_expr(is_equal(vi, spvi));

    random_shuffle(vi.begin(), vi.end());
    random_shuffle(spvi.begin(), spvi.end());   
    pprint(vi, "\nvi after random_shuffle");
    sort(vi.begin(), vi.end());
    sort(spvi.begin(), spvi.end());
    pprint(vi, "\nvi after sort");
    check_expr(is_equal(vi, spvi));

    random_shuffle(vi2.begin(), vi2.end());
    random_shuffle(spvi2.begin(), spvi2.end());
#ifdef WIN32
    // The implementation of  __get_temporary_buffer in g++'s stl 
    // lib makes it impossible to correctly use ElementHolder or 
    // ElementRef<>, so this function dose not work with dbstl on gcc.
    stable_sort(vi2.begin(), vi2.end());
    stable_sort(spvi2.begin(), spvi2.end());
#else
    sort(vi2.begin(), vi2.end());
    sort(spvi2.begin(), spvi2.end());
#endif
    check_expr(is_equal(vi2, spvi2)); 

    random_shuffle(vi2.begin(), vi2.end());
    random_shuffle(spvi2.begin(), spvi2.end());
    pprint(vi2);
    sort(vi2.begin(), vi2.end());
    sort(spvi2.begin(), spvi2.end());
    pprint(vi2);
    check_expr(is_equal(vi2, spvi2)); 

    vi3.clear();
    vi3.insert(vi3.begin(), (size_t)100, ptint(0));
    vector<ptint> spvi3(100);
    merge(vi.begin(), vi.end(), vi2.begin(), vi2.end(), vi3.begin());
    merge(spvi.begin(), spvi.end(), spvi2.begin(), spvi2.end(), 
        spvi3.begin());
    pprint(vi3, "\n vi3 after merge vi1 with vi2:");
    check_expr(is_equal(vi3, spvi3));

    spvi3.clear();
    vi3.clear();
    sort(vi2.begin(), vi2.end());
    sort(spvi2.begin(), spvi2.end());
    pprint(vi2, "\nvi2 after sort:");
    vi3.insert(vi3.begin(), vi2.begin(), vi2.end());
    pprint(vi3, "\nvi3 after insert vi2 at beginning");
    spvi3.insert(spvi3.begin(), spvi2.begin(), spvi2.end());
    vi3.insert(vi3.begin(), vi.begin(), vi.end());
    spvi3.insert(spvi3.begin(), spvi.begin(), spvi.end());
    pprint(vi3, "\nvi3: v1 v2 combined in this order");
#ifdef WIN32 
    // The implementation of  __get_temporary_buffer in g++'s stl 
    // lib makes it impossible to correctly use ElementHolder or 
    // ElementRef<>, so this function dose not work with dbstl on gcc.
    inplace_merge(vi3.begin(), vi3.begin() + (vi.end() - vi.begin()), 
        vi3.end());
    inplace_merge(spvi3.begin(), spvi3.begin() + (spvi.end() - 
        spvi.begin()), spvi3.end());
#endif
    check_expr(is_equal(vi3, spvi3));
    make_heap(vi3.begin(), vi3.end());
    make_heap(spvi3.begin(), spvi3.end());
    check_expr(is_equal(vi3, spvi3));
    cout<<endl<<"pop_heap test"<<endl;
    
    int r ;
    int rdcnt = 0;
    while (vi3.size() > 0) {
        rdcnt++;

        pop_heap(vi3.begin(), vi3.end());
        vi3.pop_back();
        pop_heap(spvi3.begin(), spvi3.end());
        spvi3.pop_back();
        pprint(vi3);

    }
    cout<<endl<<"push_heap test"<<endl;
    rdcnt = 0;
    while (vi3.size() < 10) {
        rdcnt++;
        r = rand();
        vi3.push_back(ptint(r));
        push_heap(vi3.begin(), vi3.end());
        spvi3.push_back(ptint(r));
        push_heap(spvi3.begin(), spvi3.end());
        check_expr(is_equal(vi3, spvi3));
    }
    
    vi.clear();
    vi2.clear();
    vi3.clear(); 
    if ( EXPLICIT_TXN) 
        dbstl::commit_txn(penv);
    
} // void test_std_functions()

void TestVector::test_primitive()
{

    int i;

    if ( EXPLICIT_TXN) 
        dbstl::begin_txn(0, penv);
    

    db_vector<int, ElementHolder<int> > ivi(db, penv);
    vector<int> spvi4;
    fill(ivi, spvi4);
    check_expr(is_equal(ivi, spvi4));
    ivi.clear(false);

    db_vector<int, ElementHolder<int> > ivi2(db, penv);
    vector<int> spvi5;
    fill(ivi2, spvi5);
    check_expr(is_equal(ivi2, spvi5));
    size_t vsz = ivi2.size();
    for (i = 0; i < (int)vsz - 1; i++) {
        ivi2[i] += 3;
        ivi2[i]--;
        ivi2[i] <<= 2;
        ivi2[i] = (~ivi2[i] | ivi2[i] & ivi2[i + 1] ^ (2 * (-ivi2[i + 1]) + ivi2[i]) * 3) / (ivi2[i] * ivi2[i + 1] + 1);

        spvi5[i] += 3;
        spvi5[i]--;
        spvi5[i] <<= 2;
        spvi5[i] = (~spvi5[i] | spvi5[i] & spvi5[i + 1] ^ (2 * (-spvi5[i + 1]) + spvi5[i]) * 3) / (spvi5[i] * spvi5[i + 1] + 1);
    }
    check_expr(is_equal(ivi2, spvi5));
    ivi2.clear(false);

    db_vector<ptype<int> > ivi3(db, penv);
    vector<int> spvi6;
    fill(ivi3, spvi6);
    check_expr(is_equal(ivi3, spvi6));
    ivi3.clear(false);
    
    typedef db_vector<double, ElementHolder<double> > dbl_vct_t;
    dbl_vct_t dvi(db, penv);
    vector<double> dsvi;
    for (i = 0; i < 10; i++) {
        dvi.push_back(i * 3.14);
        dsvi.push_back(i * 3.14);
    }
    check_expr(is_equal(dvi, dsvi));

    dbl_vct_t::iterator ditr;
    vector<double>::iterator sditr;
    for (ditr = dvi.begin(), sditr = dsvi.begin(); 
        ditr != dvi.end(); ++ditr, ++sditr){
        *ditr *= 2;
        *sditr *= 2;
    }

    check_expr(is_equal(dvi, dsvi));

    for (i = 0; i < 9; i++) {
        dvi[i] /= (-dvi[i] / 3 + 2 * dvi[i + 1]) / (1 + dvi[i]) + 1;
        dsvi[i] /= (-dsvi[i] / 3 + 2 * dsvi[i + 1]) / (1 + dsvi[i]) + 1;
    }
    cout<<"\ndvi after math operations: \n";
    pprint(dvi);
    cout<<"\ndsvi after math operations: \n";
    for (i = 0; i <= 9; i++)
        cout<<dsvi[i]<<"  ";
    for (i = 0; i < 10; i++) {
        cout<<i<<"\t";
        check_expr((int)(dvi[i] * 100000) == (int)(dsvi[i] * 100000));
    }

    if ( EXPLICIT_TXN) 
        dbstl::commit_txn(penv);
} // test_primitive

void TestVector::test_queue_stack()
{
    int i;

    if ( EXPLICIT_TXN) 
        dbstl::begin_txn(0, penv);
    ptint_vector vi3(db3, penv);
    // test whether db_vector works with std::queue, 
    // std::priority_queue and std::stack
    cout<<"\n_testing db_vector working with std::queue\n";
    vi3.clear();
    
    // std::queue test
    intvec_t quev(quedb, penv);
    quev.clear();
    std::queue<ptint, intvec_t> intq(quev);
    std::queue<ptint> sintq;
    check_expr(intq.empty());
    check_expr(intq.size() == 0);
    for (i = 0; i < 100; i++) {
        intq.push(ptint(i));
        sintq.push(i);
        check_expr(intq.front() == 0);
        check_expr(intq.back() == i);
    }
    check_expr(intq.size() == 100);
    for (i = 0; i < 100; i++) {
        check_expr(intq.front() == i);
        check_expr(intq.back() == 99);
        check_expr(intq.front() == sintq.front());
        check_expr(sintq.back() == intq.back());
        sintq.pop();
        intq.pop(); 
    }
    check_expr(intq.size() == 0);
    check_expr(intq.empty());
    quev.clear();

    // std::priority_queue test
    cout<<"\n_testing db_vector working with std::priority_queue\n";

    std::vector<ptint> squev;
    intvec_t pquev(pquedb, penv);
    pquev.clear();
    std::priority_queue<ptint, intvec_t, ptintless_ft> intpq(ptintless, pquev);
    std::priority_queue<ptint, vector<ptint>, ptintless_ft> 
        sintpq(ptintless, squev);

    check_expr(intpq.empty());
    check_expr(intpq.size() == 0);
    ptint tmppq, tmppq1;
    set<ptint> ptintset;
    for (i = 0; i < 100; i++) {
        for (;;) {// avoid duplicate
            tmppq = rand();
            if (ptintset.count(tmppq) == 0) {
                intpq.push(tmppq);      
                sintpq.push(tmppq);
                ptintset.insert(tmppq);
                break;
            } 
        }

    }
    check_expr(intpq.empty() == false);
    check_expr(intpq.size() == 100);
    for (i = 0; i < 100; i++) {
        tmppq = intpq.top();
        tmppq1 = sintpq.top();
        if (i == 98 && tmppq != tmppq1) {
            tmppq = intpq.top();
        }
        if (i < 98)
            check_expr(tmppq == tmppq1);
        if (i == 97)
            intpq.pop();
        else
            intpq.pop();
        sintpq.pop();
    }
    check_expr(intpq.empty());
    check_expr(intpq.size() == 0);


    // std::stack test
    cout<<"\n_testing db_vector working with std::stack\n";
    std::stack<ptint, intvec_t> intstk(quev);
    std::stack<ptint> sintstk;
    check_expr(intstk.size() == 0);
    check_expr(intstk.empty());
    for (i = 0; i < 100; i++) {
        intstk.push(ptint(i));
        sintstk.push(ptint(i));
        check_expr(intstk.top() == i);
        check_expr(intstk.size() == (size_t)i + 1);
    }
    
    for (i = 99; i >= 0; i--) {
        check_expr(intstk.top() == ptint(i));
        check_expr(intstk.top() == sintstk.top());
        sintstk.pop();
        intstk.pop();
        check_expr(intstk.size() == (size_t)i);
    }
    check_expr(intstk.size() == 0);
    check_expr(intstk.empty());
    
    // Vector with no handles specified. 
    ptint_vector simple_vct(10);
    vector<ptint> ssvct(10);
    for (i = 0; i < 10; i++) {
        simple_vct[i] = ptint(i);
        ssvct[i] = ptint(i);
    }
    check_expr(is_equal(simple_vct, ssvct));

    if ( EXPLICIT_TXN)
        dbstl::commit_txn(penv);
    
    return;
} // test_queue_stack

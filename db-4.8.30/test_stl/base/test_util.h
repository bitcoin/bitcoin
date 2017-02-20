/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "test.h"
#include "ptype.h"

/* 
 * this function should not be included into dbstl because swapping
 * different type of vars is possible to lose precision, this is why std::swap 
 * requires same type. here I know I won't lose precision so I do it at my 
 * cost
 */  
namespace std {
template <Typename T1, Typename T2>
void swap(T1&t1, T2&t2) 
{
    T2 t;
    t = t2;
    t2 = t1;
    t1 = t;

}

bool operator==(const pair<ptint, ElementHolder<ptint> >&v1, const pair<ptint, ElementHolder<ptint> >&v2)
{
    return v1.first == v2.first && v1.second._DB_STL_value() == v2.second._DB_STL_value();
}

template <typename T1, typename T2, typename T3, typename T4>
bool operator==(const pair<T1, T2>&v1, const pair<T3, T4> &v2)
{
    return v1.first == v2.first && v1.second == v2.second;
}
} // namespace std

template <Typename T1, Typename T2, Typename T3, Typename T4> 
bool is_equal(db_map<T1, T2, ElementHolder<T2> >& dv, map<T3, T4>&v)
{
    size_t s1, s2;
    bool ret;
    typename db_map<T1, T2, ElementHolder<T2> >::iterator itr1;
    typename map<T3, T4>::iterator itr2;

    if (g_test_start_txn)
        begin_txn(0, g_env);
    if ((s1 = dv.size()) != (s2 = v.size())){
        ret = false;
        goto done;
    }
         
    for (itr1 = dv.begin(), itr2 = v.begin(); itr1 != dv.end(); ++itr1, ++itr2) {
        if (itr1->first != itr2->first || itr1->second != itr2->second){
            ret = false;
            goto done;
        }
             
    }
    
    ret = true;
done:
    if (g_test_start_txn)
        commit_txn(g_env);
    return ret;

}

template <Typename T1, Typename T2, Typename T3, Typename T4> 
bool is_equal(db_map<T1, T2, ElementRef<T2> >& dv, map<T3, T4>&v)
{
    size_t s1, s2;
    bool ret;
    typename db_map<T1, T2, ElementRef<T2> >::iterator itr1;
    typename map<T3, T4>::iterator itr2;

    if (g_test_start_txn)
        begin_txn(0, g_env);
    if ((s1 = dv.size()) != (s2 = v.size())){
         
        ret = false;
        goto done;
    }


    for (itr1 = dv.begin(), itr2 = v.begin(); itr1 != dv.end(); ++itr1, ++itr2) {
        if (itr1->first != itr2->first || itr1->second != itr2->second){
            ret = false;
            goto done;
        }
    }
    
    ret = true;
done:
    if (g_test_start_txn)
        commit_txn(g_env);
    return ret;

}


template<Typename T1, Typename T2>
bool is_equal(const db_set<T1, ElementHolder<T1> >&s1, const set<T2>&s2)
{
    bool ret;
    typename db_set<T1, ElementHolder<T1> >::iterator itr;

    if (g_test_start_txn)
        begin_txn(0, g_env);
    if (s1.size() != s2.size()){
        ret = false;
        goto done;
    }

    for (itr = s1.begin(); itr != s1.end(); itr++) {
        if (s2.count(*itr) == 0) {
            ret = false;
            goto done;
        }
    }
    ret = true;
done:
    if (g_test_start_txn)
        commit_txn(g_env);
    return ret;

}


template <Typename T1, Typename T2>
bool is_equal(const db_vector<T1>& dv, const vector<T2>&v)
{
    size_t s1, s2;
    bool ret;
    T1 t1;
    size_t i, sz = v.size();

    if (g_test_start_txn)
        begin_txn(0, g_env);
    if ((s1 = dv.size()) != (s2 = v.size())) {
        ret = false;
        goto done;
    }

    
    for (i = 0; i < sz; i++) {
        t1 = T1(dv[(index_type)i] );
        if (t1 != v[i]){
            ret = false;
            goto done;
        }
    }
    ret = true;
done:

    if (g_test_start_txn)
        commit_txn(g_env);
    return ret;
}

// The following four functions are designed to work with is_equal to compare
// char*/wchar_t* strings properly, unforturnately they can not override
// the default pointer value comparison behavior.
bool operator==(ElementHolder<const char *>s1, const char *s2);
bool operator==(ElementHolder<const wchar_t *>s1, const wchar_t *s2);
bool operator!=(ElementHolder<const char *>s1, const char *s2);
bool operator!=(ElementHolder<const wchar_t *>s1, const wchar_t *s2);

template <Typename T1>
bool is_equal(const db_vector<T1, ElementHolder<T1> >& dv, const vector<T1>&v)
{
    size_t s1, s2;
    bool ret;
    size_t i, sz = v.size();

    if (g_test_start_txn)
        begin_txn(0, g_env);
    if ((s1 = dv.size()) != (s2 = v.size())) {
        ret = false;
        goto done;
    }
    
    for (i = 0; i < sz; i++) {
        if (dv[(index_type)i] != v[i]) {
            ret = false;
            goto done;
        }
    }
    ret = true;
done:

    if (g_test_start_txn)
        commit_txn(g_env);
    return ret;
}


template <Typename T1, Typename T2>
bool is_equal(db_vector<T1>& dv, list<T2>&v)
{
    size_t s1, s2;
    bool ret;
    typename db_vector<T1>::iterator itr;
    typename list<T2>::iterator itr2;

    if (g_test_start_txn)
        begin_txn(0, g_env);
    if ((s1 = dv.size()) != (s2 = v.size())) {
        ret = false;
        goto done;
    }

    
    for (itr = dv.begin(), itr2 = v.begin(); 
        itr2 != v.end(); ++itr, ++itr2) {
        if (*itr != *itr2) {
            ret = false;
            goto done;
        }
    }
    ret = true;
done:

    if (g_test_start_txn)
        commit_txn(g_env);
    return ret;
}

bool is_equal(db_vector<char *, ElementHolder<char *> >&v1, std::list<string> &v2)
{
    db_vector<char *, ElementHolder<char *> >::iterator itr;
    std::list<string>::iterator itr2;

    if (v1.size() != v2.size())
        return false;

    for (itr = v1.begin(), itr2 = v2.begin(); itr2 != v2.end(); ++itr, ++itr2)
        if (strcmp(*itr, (*itr2).c_str()) != 0)
            return false;

    return true;
}

template <Typename T1>
class atom_equal {
public:
    bool operator()(T1 a, T1 b)
    {
        return a == b;
    }
};
template<>
class atom_equal<const char *> {
public:
    bool operator()(const char *s1, const char *s2)
    {
        return strcmp(s1, s2) == 0;
    }
};

template <Typename T1>
bool is_equal(const db_vector<T1, ElementHolder<T1> >& dv, const list<T1>&v)
{
    size_t s1, s2;
    bool ret;
    typename db_vector<T1, ElementHolder<T1> >::const_iterator itr;
    typename list<T1>::const_iterator itr2;
    atom_equal<T1> eqcmp;
    if (g_test_start_txn)
        begin_txn(0, g_env);
    if ((s1 = dv.size()) != (s2 = v.size())) {
        ret = false;
        goto done;
    }

    
    for (itr = dv.begin(), itr2 = v.begin(); 
        itr2 != v.end(); ++itr, ++itr2) {
        if (!eqcmp(*itr, *itr2)) {
            ret = false;
            goto done;
        }
    }
    ret = true;
done:

    if (g_test_start_txn)
        commit_txn(g_env);
    return ret;
}

// fill the two vectors with identical n integers,starting from start 
template<Typename T>
void fill(db_vector<ptype<T> >&v, vector<T>&sv, T start = 0, int n = 5)
{
    T i;
    
    v.clear();
    sv.clear();
    for (i = start; i < n + start; i++) {
        v.push_back(i);
        sv.push_back(i);
    }
}


template<Typename T>
void fill(db_vector<T, ElementHolder<T> >&v, vector<T>&sv, T start = 0, int n = 5 )
{
    int i;
    
    v.clear();
    sv.clear();
    for (i = 0; i < n; i++) {
        v.push_back(i + start);
        sv.push_back(i + start);
    }
}

template <Typename T>
void fill(db_map<ptype<T>, ptype<T> >&m, map<T, T>&sm, 
    T start = 0, int n = 5) 
{
    int i;
    ptype<T> pi;

    m.clear();
    sm.clear();
    for (i = 0; i < n; i++) {
        pi = i + start;
        m.insert(make_pair(pi, pi));
        sm.insert(make_pair(pi.v, pi.v));
    }
    
}


template <Typename T>
void fill(db_map<T, T, ElementHolder<T> >&m, map<T, T>&sm, 
    T start = 0, int n = 5) 
{
    int i;
    T pi;

    m.clear();
    sm.clear();
    for (i = 0; i < n; i++) {
        pi = i + start;
        m.insert(make_pair(pi, pi));
        sm.insert(make_pair(pi, pi));
    }
    
}


template <Typename T>
void fill(db_set<ptype<T> >&m, set<T>&sm, T start = 0, int n = 5) 
{
    int i;
    ptype<T> pi;

    m.clear();
    sm.clear();

    for (i = 0; i < n; i++) {

        pi = i + start;
        m.insert(pi);
        sm.insert(pi.v);
    }
    
}


template <Typename T>
void fill(db_set<T, ElementHolder<T> >&m, set<T>&sm, 
    T start = 0, int n = 5) 
{
    int i;
    T pi;

    m.clear();
    sm.clear();
    for (i = 0; i < n; i++) {
        pi = i + start;
        m.insert(pi);
        sm.insert(pi);
    }
    
}


size_t g_count[256];
template <Typename T>
void fill(db_multimap<ptype<T>, ptype<T> >&m, multimap<T, T>&sm, 
    T start = 0, int n = 5, size_t randn = 5) 
{

    int i;
    size_t j, cnt;

    if (randn == 0)
        randn = 1;

    m.clear();
    sm.clear();
    for (i = 0; i < n; i++) {
        if (randn > 1) 
            cnt = abs(rand()) % randn;
        if (cnt == 0)
            cnt = randn;
        i += start;
        g_count[i] = cnt;
        for (j = 0; j < cnt; j++) {// insert duplicates
            m.insert(make_pair(i, ptype<T>(i)));
            sm.insert(make_pair(i, i));
        }
        i -= start;
    }
}


template <Typename T>
void fill(db_multimap<T, T, ElementHolder<T> >&m, multimap<T, T>&sm, 
    T start = 0, int n = 5, size_t randn = 5) 
{
    int i;
    size_t j, cnt = 0;

    if (randn < 5)
        randn = 5;

    m.clear();
    sm.clear();
    for (i = 0; i < n; i++) {
        cnt = abs(rand()) % randn;
        if (cnt == 0)
            cnt = randn;
        i += start;
        g_count[i] = cnt;
        for (j = 0; j < cnt; j++) {// insert duplicates
            m.insert(make_pair(i, i));
            sm.insert(make_pair(i, i));
        }
        i -= start;
    }
    
}


template <Typename T>
void fill(db_multiset<ptype<T> >&m, multiset<T>&sm, T start, 
    int n, size_t randn) 
{

    int i;
    size_t j, cnt;

    if (randn == 0)
        randn = 1;
    
    m.clear();
    sm.clear();
    for (i = 0; i < n; i++) {
        if (randn > 1) 
            cnt = abs(rand()) % randn;
        if (cnt == 0)
            cnt = randn;
        i += start;
        g_count[i] = cnt;
        for (j = 0; j < cnt; j++) {// insert duplicates
            m.insert(ptype<T>(i));
            sm.insert(i);
        }
        i -= start;
    }
}


template <Typename T>
void fill(db_multiset<T, ElementHolder<T> >&m, multiset<T>&sm, 
    T start = 0, int n = 5 , size_t randn = 5) 
{
    int i;
    size_t j, cnt;

    if (randn < 5)
        randn = 5;
    
    m.clear();
    sm.clear();
    for (i = 0; i < n; i++) {
        cnt = abs(rand()) % randn;
        if (cnt == 0)
            cnt = randn;
        i += start;
        g_count[i] = cnt;
        for (j = 0; j < cnt; j++) {// insert duplicates
            m.insert(i);
            sm.insert(i);
        }
        i -= start;
    }
}


#ifdef TEST_PRIMITIVE
bool is_odd_pair(pair<ptint, ElementHolder<ptint> >& s)
{
    return ( s.second._DB_STL_value() % 2) != 0;
}
#else
bool is_odd_pair(pair<ptint, ElementRef<ptint> >& s)
{
    return ( s.second._DB_STL_value() % 2) != 0;
}
#endif


template<Typename pair_type> 
void square_pair(pair_type&s)
{ 
    cout<<s.second*s.second<<'\t';
}

template<Typename pair_type> 
void square_pair2(pair_type&s)
{ 
    cout<<s.second*s.second<<'\t';
}
 



#ifndef TEST_PRIMITIVE
bool is2digits_pair(pair<ptint, ElementRef<ptint> >&  i)
{
    return (i.second._DB_STL_value() >  (9)) && (i.second._DB_STL_value() <  (100));
}

#else
bool is2digits_pair(pair<ptint, ElementHolder<ptint> >&  i)
{
    return (i.second._DB_STL_value() >  (9)) && (i.second._DB_STL_value() <  (100));
}
#endif
bool is7(const ptint& value) { return (value == (ptint)7); }


template <Typename T, Typename value_type_sub>
void pprint(db_vector<T, value_type_sub>&v, const char *prefix = NULL)
{
return;
    size_t i;
    size_t sz = v.size();

    cout<<"\n";
    if (prefix)
        cout<<prefix;
    if (g_test_start_txn)
        begin_txn(0, g_env);
    for (i = 0; i < sz; i++) {
        cout<<"\t"<<(v[(index_type)i]);
    }
    if (g_test_start_txn)
        commit_txn(g_env);
}


template <Typename T, Typename ddt, Typename value_type_sub>
void pprint(db_map<T, ddt, value_type_sub>&v, const char *prefix = NULL)
{
return;
    if (g_test_start_txn)
        begin_txn(0, g_env);
    cout<<"\n";
    if (prefix)
        cout<<prefix;
    typename db_map<T, ddt, value_type_sub>::iterator itr;
    for (itr = v.begin(); itr != v.end(); itr++)
        cout<<'\t'<<itr->first<<'\t'<<itr->second;
    if (g_test_start_txn)
        commit_txn(g_env);
}


template <Typename T, Typename value_type_sub>
void pprint(db_set<T, value_type_sub>&v, const char *prefix = NULL)
{
return;
    if (g_test_start_txn)
        begin_txn(0, g_env);
    cout<<"\n";
    if (prefix)
        cout<<prefix;
    typename db_set<T, value_type_sub>::iterator itr;
    for (itr = v.begin(); itr != v.end(); itr++)
        cout<<'\t'<<*itr;
    if (g_test_start_txn)
        commit_txn(g_env);
}


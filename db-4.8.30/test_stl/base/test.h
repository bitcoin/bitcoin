/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$ 
 */

#ifndef _DB_STL_TEST_H__
#define _DB_STL_TEST_H__

#include <time.h>

#include <utility>
#include <functional>
#include <algorithm>
#include <iostream>
#include <vector>
#include <list>
#include <queue>
#include <stack>
#include <fstream>
#include <map>
#include <set>
#include <string>

#include "db_config.h"
#include "db_int.h"

#define DB_STL_HAVE_DB_TIMESPEC 1
#include "dbstl_map.h"
#include "dbstl_set.h"
#include "dbstl_vector.h"

#include "ptype.h"

using namespace std;
using namespace dbstl;

/////////////////////////////////////////////////////////////////////////
///////////////////////// Macro and typedef definitions ///////////

#define check_expr(expression) do {         \
    if (!(expression)) {                \
        FailedAssertionException ex(__FILE__, __LINE__, #expression);\
        throw ex; } } while (0)

#define TEST_PRIMITIVE
#ifndef TEST_PRIMITIVE
typedef ptype<int>  ptint;
typedef db_vector<ptint> intvec_t;
typedef db_vector<ptint> ptint_vector;
typedef db_map<ptint, ptint> dm_int_t;
typedef db_multimap<ptint, ptint> dmm_int_t;
typedef db_set<ptint> dms_int_t;
typedef db_multiset<ptint> dmms_int_t;
#define TOINT  
#else
#define ptint int
#define TOINT (int)
typedef db_vector<ptint, ElementHolder<ptint> > intvec_t;
typedef db_vector<ptint, ElementHolder<ptint> > ptint_vector;
typedef db_map<ptint, ptint, ElementHolder<ptint> > dm_int_t;
typedef db_multimap<ptint, ptint, ElementHolder<ptint> > dmm_int_t;
typedef db_set<ptint, ElementHolder<ptint> > dms_int_t;
typedef db_multiset<ptint, ElementHolder<ptint> > dmms_int_t;
#endif

#define DELDB(pdb) if (pdb){delete pdb; pdb = NULL;}
#define print_mm(dmm1, dmmi)    \
for (dmmi = dmm1.begin(); dmmi != dmm1.end(); dmmi++)   \
        cout<<dmmi->first<<'\t'<<dmmi->second;

// Portable thread functions, supporting Win32 and pthread libraries 
// These functions are used by the multi-threaded test.
#if defined(DB_WIN32) || defined(WIN32)

extern "C" {
extern int getopt(int, char * const *, const char *);
extern char *optarg;
extern int optind;
}

typedef HANDLE os_pid_t;
typedef HANDLE os_thread_t;

#define os_thread_create(thrp, attr, func, arg)             \
    (((*(thrp) = CreateThread(NULL, 0,                  \
    (LPTHREAD_START_ROUTINE)(func), (arg), 0, NULL)) == NULL) ? -1 : 0)
#define os_thread_join(thr, statusp)                    \
    ((WaitForSingleObject((thr), INFINITE) == WAIT_OBJECT_0) &&     \
    GetExitCodeThread((thr), (LPDWORD)(statusp)) ? 0 : -1)
#define os_thread_self() GetCurrentThreadId()
#else /* !DB_WIN32 */

#include <sys/wait.h>
#include <pthread.h>

typedef pid_t os_pid_t;
typedef pthread_t os_thread_t;

#define os_thread_create(thrp, attr, func, arg)             \
    pthread_create((thrp), (attr), (func), (arg))
#define os_thread_join(thr, statusp) pthread_join((thr), (statusp))
#define os_thread_self() pthread_self()
#endif /* !DB_WIN32 */

////////////////////////////////////////////////////////////////////////////
///////////////////// Global variable declarations ////////////////////////
/*extern DbEnv *penv;
extern Db *db, *db2, *db3, *dmdb1, *dmdb2, *dmmdb1, *dmmdb2, *dmsdb1, 
    *dmsdb2, *dmmsdb1, *dmmsdb2, *dbstrv;
*/
extern int g_StopInsert;
extern ofstream fout;
extern size_t g_count[256];
extern char *optarg;
extern int g_test_start_txn;
extern DbEnv *g_env;
extern int optind;
///////////////////////////////////////////////////////////////////////
//////////////////////// Function Declarations ////////////////////
// XXX!!! Function templates can't be declared here otherwise the declarations
// here will be deemed as the definition, so at link time these symbols are 
// not resolved. So like class templates, function templates can't be separated
// as declarations and definitions, only definitions and only be built into one
// object file otherwise there will be "multiple symbol definitions". OTOH, compilers
// can avoid the multiple definitions if we are building a class template instantiation
// in multiple object files, so class tempaltes are recommended to use rather than
// function templates. Only use function templates if it is a simple one and used
// only in one code file.
//
size_t g_sum(size_t s, size_t e); 
bool is_odd(ptint s);
ptint addup(const ptint&i, const ptint&j);
ptint randint();
int randpos(int p);
bool is2digits(ptint  i);
int rmdir_rcsv(const char *dir, bool keep_this_dir);
bool strlt(const string&, const string&);
bool strlt0(const char *s1, const char *s2);
bool streq0(const char *s1, const char *s2);
bool streq(const string&, const string&);

int get_dest_secdb_callback(Db *secondary, const Dbt *key, 
    const Dbt *data, Dbt *result);
void copy_array(TCHAR**arr, TCHAR***dest);
void using_charstr(TCHAR*str);
void usage();
int test_assoc(void* param1);
int test_vector(void* param1);

class RGBB;
class SMSMsg2;
void SMSMsgRestore(SMSMsg2& dest, const void *srcdata);
u_int32_t SMSMsgSize(const SMSMsg2& elem);
void SMSMsgCopy(void *dest, const SMSMsg2&elem);
typedef bool (*ptintless_ft)(const ptint& a, const ptint& b);
bool ptintless(const ptint& a, const ptint& b);
u_int32_t rgblen(const RGBB * seq);
void rgbcpy(RGBB *seq, const RGBB *, size_t);


/////////////////////////////////////////////////////////////////////////////////
////////////////////////// Utility class definitions  ///////////////////////////

class BaseMsg
{
public:
    time_t when;
    int to;
    int from;

    BaseMsg()
    {
        to = from = 0;
        when = 0;
    }

    BaseMsg(const BaseMsg& msg)
    {
        to = msg.to;
        from = msg.from;
        when = msg.when;
    }   

    bool operator==(const BaseMsg& msg2) const 
    {
        return when == msg2.when && to == msg2.to && from == msg2.from;
    }

    bool operator<(const BaseMsg& msg2) const
    {
        return to < msg2.to;
    }
};

// used to test arbitary obj storage(not in one chunk)
class SMSMsg2 : public BaseMsg
{
public:
    typedef SMSMsg2 self;
    SMSMsg2(time_t tm, const char *msgp, int t)
    {
        memset(this, 0, sizeof(*this));
        when = tm;
        szmsg = strlen(msgp) + 1;
        msg = (char *)DbstlMalloc(szmsg);
        strncpy(msg, msgp, szmsg);
        to = t;
        
        mysize = sizeof(*this); //+ szmsg;
    }

    SMSMsg2()
    {
        memset(this, 0, sizeof(SMSMsg2));
    }

    SMSMsg2(const self& obj) : BaseMsg(obj)
    {
        mysize = obj.mysize;
        szmsg = obj.szmsg;
        if (szmsg > 0 && obj.msg != NULL) {
            msg = (char *)DbstlMalloc(szmsg);
            strncpy(msg, obj.msg, szmsg);
        } else 
            msg = NULL;
    }

    ~SMSMsg2()
    {
        if (msg)
            free(msg);
    }

    const self& operator = (const self &obj)
    {

        this->from = obj.from;
        to = obj.to;
        when = obj.when;
        mysize = obj.mysize;
        szmsg = obj.szmsg;
        if (szmsg > 0 && obj.msg != NULL) {
            msg = (char *)DbstlReAlloc(msg, szmsg);
            strncpy(msg, obj.msg, szmsg);
        }
        return obj;
    }

    bool operator == (const self&obj) const
    {
        return BaseMsg::operator==(obj) && strcmp(obj.msg, msg) == 0;
    }

    const static size_t BUFLEN = 256;
    size_t mysize;
    size_t szmsg;
    char *msg;
    
};//SMSMsg2

// SMS message class
class SMSMsg : public BaseMsg
{

    
public:

    size_t mysize;
    size_t szmsg;
    char msg[1];
    static SMSMsg* make_sms_msg(time_t t, const char*msg, int dest)
    {
        size_t mlen = 0, totalsz = 0;

        SMSMsg *p = (SMSMsg *)DbstlMalloc(totalsz = (sizeof(SMSMsg) + (mlen = strlen(msg) + 4)));
        memset(p, 0, totalsz);
        // adding sizeof(p->to) to avoid memory alignment issues
        p->mysize = sizeof(SMSMsg) + mlen;
        p->when = t;
        p->szmsg = mlen - 3;
        p->to = dest;
        p->from = 0;
        strcpy(&(p->msg[0]), msg);

        return p;
    }

    SMSMsg()
    {

    }
protected:
    SMSMsg(time_t t, const char*msg1, int dest)
    {
        size_t mlen = 0;

        when = t;
        szmsg = strlen(msg1) + 1;
        mlen = strlen(msg1);
        strncpy((char*)&(this->msg[0]), msg1, mlen);
        *(int*)(((char*)&(this->msg[0])) + mlen + 1) = dest;
    }

};// SMSMsg

class RGBB 
{
public:
    typedef unsigned char color_t;

    color_t r_, g_, b_, bright_;

    RGBB()
    {
        memset(this, 0, sizeof(RGBB));// complete 0 means invalid
    }

    RGBB(color_t r, color_t g, color_t b, color_t brightness)
    {
        r_ = r;
        g_ = g;
        b_ = b;
        bright_ = brightness;
    }

};// RGBB

template <Typename T>
class separator
{
public:
    T mid;
    bool operator()(const T& ele)
    {
           return ele < mid;
    }
};


class square 
{
public:
    void operator()(ptint s)
    {
        cout<<s*s<<'\t';
    }
};


class rand_str_dbt
{
public:
    static const size_t BUFLEN = 2048;
    static bool init;
    static char buf[BUFLEN];

    rand_str_dbt() 
    {
        int len = BUFLEN, i;
        
        if (!init) {
            init = true;
            
            for (i = 0; i < len - 1; i++) {
                buf[i] = 'a' + rand() % 26;
            }
            buf[i] = '\0';
        }
    }
    // dbt is of DB_DBT_USERMEM, mem allocated by DbstlMalloc
    void operator()(Dbt&dbt, string&str, size_t shortest = 30, size_t longest = 150) 
    {
        int rd = rand();

        if (rd < 0)
            rd = -rd;
        str.clear();

        check_expr(shortest > 0 && longest < BUFLEN);
        check_expr(dbt.get_flags() & DB_DBT_USERMEM);// USER PROVIDE MEM
        size_t len = (u_int32_t)(rd % longest);
        if (len < shortest)
            len = shortest;
        else if (len >= BUFLEN)
            len = BUFLEN - 1;
        // start must be less than BUFLEN - len, otherwise we have no 
        // len bytes to offer
        size_t start = rand() % (BUFLEN - len);

        char c = buf[start + len];

        buf[start + len] = '\0';
        str = buf + start;
        if (dbt.get_ulen() < (len + 1)) {
            free(dbt.get_data());
            dbt.set_data(DbstlMalloc(len + 1));
            check_expr(dbt.get_data() != NULL);
        }
        memcpy(dbt.get_data(), (void*)(buf + start), len + 1);
        dbt.set_size(u_int32_t(len + 1));// store the '\0' at the end
        buf[start + len] = c; 
    }
}; // rand_str_dbt

class rand_str_dbt;
struct TestParam{
    int flags, setflags, TEST_AUTOCOMMIT, dboflags, EXPLICIT_TXN;
    DBTYPE dbtype;
    DbEnv *dbenv;
};

class test_block
{
private:
    db_timespec tp;
public:
    string blkname;

    void begin(const char* name)
    {

        blkname = name;
        cout<<endl<<"============= start of test "<<
            name<<" ==============";
        __os_gettime(NULL, &tp, 1);
    }
    void end()
    {
        db_timespec tp2;
        __os_gettime(NULL, &tp2, 1);
        
        cout<<"\n============ end of test "<<blkname<<" , time spent: "
            <<tp2.tv_sec - tp.tv_sec + (tp2.tv_nsec - 
            tp.tv_nsec) / 1000000000.0<<" secs =============";
    }
};

// a mobile phone SMS structure for test. will add more members in future
class sms_t
{
public:
    size_t sz;
    time_t when;
    int from;
    int to;
    char msg[512];
    
    const sms_t& operator=(const sms_t&me)
    {
        memcpy(this, &me, sizeof(*this));
        return me;
    }

    bool operator==(const sms_t& me) const
    {
        return memcmp(this, &me, sizeof(me)) == 0;
    }
    bool operator!=(const sms_t& me)
    {
        return memcmp(this, &me, sizeof(me)) != 0;
    }
};

enum wt_job {wt_read, wt_insert, wt_update, wt_delete};
enum ContainerType {ct_vector, ct_map, ct_multimap, ct_set, ct_multiset, ct_none};

#endif // ! _DB_STL_TEST_H__

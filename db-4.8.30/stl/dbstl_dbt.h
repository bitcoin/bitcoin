/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_STL_DBT_H
#define _DB_STL_DBT_H

#include <assert.h>
#include <string>

#include "dbstl_common.h"
#include "dbstl_exception.h"
#include "dbstl_utility.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//
// DataItem class template definition
//
// 1. DataItem is a Dbt wrapper, it provides both typed data to/from memory
// chunk mapping as well as iostream support. Note that iostream functionality
// is not yet implemented.
// 2. DataItem is used inside dbstl to provide consistent Dbt object memory
// management.
// 3. DataItem is not only capable of mapping fixed size objects, but also
// varying length objects and objects not located in a consecutive chunk of
// memory, with the condition that user configures the required methods in
// DbstlElemTraits.
// 4. DataItem can not be a class template because inside it, the "member
// function template override" support is needed.
//
START_NS(dbstl)

using std::string;
#ifdef HAVE_WSTRING
using std::wstring;
#endif

class DataItem
{
private:
    typedef DataItem self;

    ////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////
    //
    // DataItem memory management
    //
    // The dbt_ member is the current dbt, data is stored in the dbt's 
    // referenced memory, it may
    // deep copy from constructor and from other Dbt, depending on
    // the constructors "onstack" parameter --- if true, this object
    // is only used as a stack object inside a function,
    // so do shallow copy; otherwise do deep copy.
    // There is always a DB_DBT_USERMEM flag set to the dbt,
    // its ulen data member stores the length of referenced memory,
    // its size data member stores the actual size of data;
    // If onstack is true, its dlen is INVALID_DLEN, and freemem()
    // will not free such memory because this object only reference
    // other object's memory, its the referenced object's responsibility
    // to free their memory.
    //
    // A DataItem object is not used everywhere, so it is impossible for
    // such an object to have two kinds of usages as above at the same
    // time, so we are safe doing so.
    Dbt dbt_;

    // Free dbt_'s referenced memory if that memory is allocated in heap
    // and owned by dbt_.
    inline void freemem()
    {
        void *buf = dbt_.get_data();

        if (buf != NULL && (dbt_.get_flags() & DB_DBT_USERMEM) != 0
            && dbt_.get_dlen() != INVALID_DLEN)
            free(buf);
        memset(&dbt_, 0, sizeof(dbt_));
    }

public:

    // Deep copy, because dbt2.data pointed memory may be short lived.
    inline void set_dbt(const DbstlDbt&dbt2, bool onstack)
    {
        void *buf;
        u_int32_t s1, s2;
        DBT *pdbt2, *pdbt;
           
        pdbt2 = (DBT *)&dbt2;
        pdbt = (DBT *)&dbt_;

        if (!onstack) {
            buf = pdbt->data;
            s1 = pdbt->ulen;
            s2 = pdbt2->size;
            if(s2 > s1) {
                buf = DbstlReAlloc(buf, s2);
                pdbt->size = s2;
                pdbt->data = buf;
                pdbt->ulen = s2;
                pdbt->flags |= DB_DBT_USERMEM;
            } else
                pdbt->size = s2;
            memcpy(buf, pdbt2->data, s2);
        } else {
            freemem();
            dbt_ = (const Dbt)dbt2;
            pdbt->dlen = (INVALID_DLEN);
        }
    }

    // Deep copy, because dbt2.data pointed memory may be short lived.
    inline void set_dbt(const Dbt&dbt2, bool onstack)
    {
        void *buf;
        u_int32_t s1, s2;
        DBT *pdbt2, *pdbt;
           
        pdbt2 = (DBT *)&dbt2;
        pdbt = (DBT *)&dbt_;

        if (!onstack) {
            buf = pdbt->data;
            s1 = pdbt->ulen;
            s2 = pdbt2->size;
            if(s2 > s1) {
                buf = DbstlReAlloc(buf, s2);
                pdbt->size = s2;
                pdbt->data = buf;
                pdbt->ulen = s2;
                pdbt->flags |= DB_DBT_USERMEM;
            } else
                pdbt->size = s2;
            memcpy(buf, pdbt2->data, s2);
        } else {
            freemem();
            dbt_ = dbt2;
            pdbt->dlen = (INVALID_DLEN);
        }
    }

    inline void set_dbt(const DBT&dbt2, bool onstack)
    {
        void *buf;
        u_int32_t s1, s2;
        DBT *pdbt = (DBT *)&dbt_;

        if (!onstack) {
            buf = pdbt->data;
            s1 = pdbt->ulen;
            s2 = dbt2.size;
            if(s2 > s1) {
                buf = DbstlReAlloc(buf, s2);
                pdbt->size = s2;
                pdbt->data = buf;
                pdbt->ulen = s2;
                pdbt->flags |= DB_DBT_USERMEM;
            } else
                pdbt->size = s2;
            memcpy(buf, dbt2.data, s2);
        } else {
            freemem();
            // The following is right because Dbt derives from
            // DBT with no extra members or any virtual functions.
            memcpy(&dbt_, &dbt2, sizeof(dbt2));
            pdbt->dlen = INVALID_DLEN;
        }
    }

    // Return to the initial state.
    inline void reset()
    {
        void *buf = dbt_.get_data();
        if (buf) {
            memset(buf, 0, dbt_.get_ulen());
            dbt_.set_size(0);
        }
    }

    inline Dbt& get_dbt()
    {
        return dbt_;
    }

    // Return data of this object. If no data return -1, if it has data
    // return 0.
    //
    // !!!XXX Note that the type parameter T can only be in this function
    // because "template type parameter overload" applies only to a
    // functions template argument list, rather than that of classes.
    // If you put the "template<Typename T>" to this class's declaration,
    // making it a class template, then when T is any of Dbt, DBT, or
    // DataItem<T>, there will be two copies of this function. One will be
    // this function's instantiated version, the other one is one of the
    // three functions defined below.
    //
    template <Typename T>
    inline int get_data(T& data) const
    {
        int ret;
        typedef DbstlElemTraits<T> EM;
        typename EM::ElemRstoreFunct restore;
        void *pdata = NULL;

        if ((pdata = dbt_.get_data()) != NULL) {
            if ((restore = EM::instance()->
                get_restore_function()) != NULL)
                restore(data, pdata);
            else
                data = *((T*)pdata);
            ret = 0;
        } else
            ret = -1;
        return ret;
    }

    ////////////////////////////////////////////////////////////////////
    //
    // Begin functions supporting direct naked string storage.
    //
    // Always store the data, rather than the container object.
    //
    // The returned string lives no longer than the next iterator
    // movement call.
    //
    inline int get_data(char*& data) const
    {
        data = (char*)dbt_.get_data();
        return 0;
    }

    inline int get_data(string &data) const
    {
        data = (string::pointer) dbt_.get_data();
        return 0;
    }

    inline int get_data(wchar_t*& data) const
    {
        data = (wchar_t*)dbt_.get_data();
        return 0;
    }

#ifdef HAVE_WSTRING
    inline int get_data(wstring &data) const
    {
        data = (wstring::pointer) dbt_.get_data();
        return 0;
    }
#endif

    ////////////////////////////////////////////////////////////////////

    // Supporting storing arbitrary type of sequence.
    template <Typename T>
    inline int get_data(T*& data) const
    {
        data = (T*)dbt_.get_data();
        return 0;
    }

    inline int get_data(DataItem& data) const
    {
        int ret;

        if (dbt_.get_data()) {
            data.set_dbt(dbt_, false);
            ret = 0;
        } else
            ret = -1;
        return ret;
    }

    ////////////////////////////////////////////////////////////////////
    //
    // Begin functions supporting Dbt storage.
    //
    // This member function allows storing a Dbt type, so that user can
    // store the varying length data into Dbt.
    //
    // This method is required to copy a data element's bytes to another 
    // Dbt object, used inside by dbstl.
    // If there is no data return -1, if it has data return 0.
    //
    inline int get_data(Dbt& data) const
    {
        int ret;
        void *addr;
        u_int32_t sz;
        DBT *pdbt = (DBT *)&dbt_, *pdata = (DBT *)&data;

        if (pdbt->data) {
            addr = pdata->data;
            sz = pdbt->size;
            if (pdata->ulen < sz) {
                pdata->data = DbstlReAlloc(addr, sz);
                pdata->size = sz;
                pdata->ulen = sz;
                pdata->flags |= DB_DBT_USERMEM;
            } else
                pdata->size = sz;
            memcpy(pdata->data, pdbt->data, sz);
            ret = 0;
        } else
            ret = -1;
        return ret;
    }

    inline int get_data(DBT& data) const
    {
        int ret;
        void*addr;
        u_int32_t sz;

        if (dbt_.get_data()) {
            addr = data.data;
            if (data.ulen < (sz = dbt_.get_size())) {
                data.data = DbstlReAlloc(addr, sz);
                // User need to free this memory
                data.flags = data.flags | DB_DBT_USERMEM;
                data.size = sz;
                data.ulen = sz;
            } else
                data.size = sz;
            memcpy(data.data, dbt_.get_data(), sz);
            ret = 0;
        } else
            ret = -1;
        return ret;
    }

    inline int get_data(DbstlDbt& data) const
    {
        int ret;
        void *addr;
        u_int32_t sz;
        DBT *pdbt = (DBT *)&dbt_, *pdata = (DBT *)&data;

        if (pdbt->data) {
            addr = pdata->data;
            sz = pdbt->size;
            if (pdata->ulen < sz) {
                pdata->data = DbstlReAlloc(addr, sz);
                pdata->size = sz;
                pdata->ulen = sz;
                pdata->flags |= DB_DBT_USERMEM;
            } else
                pdata->size = sz;
            memcpy(pdata->data, pdbt->data, sz);
            ret = 0;
        } else
            ret = -1;
        return ret;
    }

    ////////////////////////////////////////////////////////////////////

    // Deep copy in assignment and copy constructor.
    inline const DbstlDbt& operator=(const DbstlDbt& t2)
    {
        set_dbt(t2, false);
        return t2;
    }

    // Deep copy in assignment and copy constructor.
    inline const Dbt& operator=(const Dbt& t2)
    {
        set_dbt(t2, false);
        return t2;
    }

    // Deep copy in assignment and copy constructor.
    inline const DBT& operator=(const DBT& t2)
    {
        set_dbt(t2, false);
        return t2;
    }

    // Deep copy in assignment and copy constructor.
    template <Typename T>
    inline const T& operator = (const T&dt)
    {

        make_dbt(dt, false);
        return dt;
    }

    // Generic way of storing an object or variable. Note that DataItem
    // is not a class template but a class with function templates.
    // Variable t locates on a consecutive chunk of memory, and objects 
    // of T have the same size. 
    //
    template <Typename T>
    void make_dbt(const T& dt, bool onstack)
    {
        typedef DbstlElemTraits<T> EM;
        u_int32_t sz;
        typename EM::ElemSizeFunct sizef;
        typename EM::ElemCopyFunct copyf;
        DBT *pdbt = (DBT *)&dbt_;

        if ((sizef = EM::instance()->get_size_function()) != NULL)
            sz = sizef(dt);
        else
            sz = sizeof(dt);

        if (onstack) {
            freemem();
            pdbt->data = ((void*)&dt);
            // We have to set DB_DBT_USERMEM for DB_THREAD to work.
            pdbt->flags = (DB_DBT_USERMEM);
            pdbt->size = (sz);
            pdbt->ulen = (sz);
            // This is a flag that this memory can't be freed
            // because it is on stack.
            pdbt->dlen = (INVALID_DLEN);
            return;
        }

        // Not on stack, allocate enough space and "copy" the object
        // using shall copy or customized copy.
        if (pdbt->ulen < sz) {
            pdbt->data = (DbstlReAlloc(pdbt->data, sz));
            assert(pdbt->data != NULL);
            pdbt->size = (sz);
            pdbt->ulen = (sz);
            pdbt->flags = (DB_DBT_USERMEM);
        } else
            pdbt->size = (sz);

        if ((copyf = EM::instance()->get_copy_function()) != NULL)
            copyf(pdbt->data, dt);
        else
            memcpy(pdbt->data, &dt, sz);
    }

    inline const char*&operator = (const char*&dt)
    {
        make_dbt(dt, false);
        return dt;
    }

    inline const wchar_t*&operator = (const wchar_t*&dt)
    {
        make_dbt(dt, false);
        return dt;
    }

    inline const string &operator=(const string &dt)
    {
        make_dbt(dt, false);
        return dt;
    }

#ifdef HAVE_WSTRING
    inline const wstring &operator=(const wstring &dt)
    {
        make_dbt(dt, false);
        return dt;
    }
#endif

    template <Typename T>
    inline const T*&operator = (const T*&dt)
    {
        make_dbt(dt, false);
        return dt;
    }

    inline const self& operator=(const self&dbt1)
    {
        ASSIGNMENT_PREDCOND(dbt1)
        this->set_dbt(dbt1.dbt_, false);
        return dbt1;
    }

    // Deep copy.
    inline DataItem(const self&dbt1)
    {
        set_dbt(dbt1.dbt_, false);
    }


    inline DataItem(u_int32_t sz)
    {
        void *buf;
        DBT *pdbt = (DBT *)&dbt_;

        buf = NULL;
        buf = DbstlMalloc(sz);
        memset(buf, 0, sz);
        pdbt->size = sz;
        pdbt->ulen = sz;
        pdbt->data = buf;
        pdbt->flags = DB_DBT_USERMEM;
    }

    // Deep copy. The onstack parameter means whether the object referenced
    // by this DataItem is on used with a function call where this DataItem
    // object is used. If so, we don't deep copy the object, simply refer
    // to its memory location. The meaining is the same for this parameter
    // in constructors that follow.
    inline DataItem(const Dbt&dbt2, bool onstack)
    {
        set_dbt(dbt2, onstack);
    }
    
    inline DataItem(const DbstlDbt&dbt2, bool onstack)
    {
        set_dbt(dbt2, onstack);
    }
    
    inline DataItem(const DBT&dbt2, bool onstack)
    {
        set_dbt(dbt2, onstack);
    }

    // Deep copy. There is a partial specialization for char*/wchar_t*/
    // string/wstring.
    template<Typename T>
    inline DataItem(const T& dt, bool onstack)
    {
        make_dbt(dt, onstack);
    }

    inline ~DataItem(void)
    {
        freemem();
    }

protected:

    // Store a char*/wchar_t* string. Need four versions for char*
    // and wchar_t* respectively to catch all
    // possibilities otherwise the most generic one will be called.
    // Note that the two const decorator matters when doing type
    // matching.
    inline void make_dbt_chars(const char *t, bool onstack)
    {
        DBT *d = (DBT *)&dbt_;
        u_int32_t sz;
        sz = ((t == NULL) ? 
            sizeof(char) : 
            (u_int32_t)((strlen(t) + 1) * sizeof(char)));
        if (!onstack) {
            if (d->ulen < sz) {
                d->flags |= DB_DBT_USERMEM;
                d->data = DbstlReAlloc(d->data, sz);
                d->ulen = sz;
            }
            d->size = sz;
            if (t != NULL)
                strcpy((char*)d->data, t);
            else
                memset(d->data, '\0', sizeof(char));
        } else {
            freemem();
            d->data = ((t == NULL) ? (void *)"" : (void *)t);
            d->size = sz;
            d->ulen = sz;
            d->flags = (DB_DBT_USERMEM);
            d->dlen = (INVALID_DLEN);
        }
    }

    inline void make_dbt_wchars(const wchar_t *t, bool onstack)
    {
        DBT *d = (DBT *)&dbt_;
        u_int32_t sz;
        sz = ((t == NULL) ? 
            sizeof(wchar_t) : 
            (u_int32_t)((wcslen(t) + 1) * sizeof(wchar_t)));
        if (!onstack) {                 
            if (d->ulen < sz) {
                d->flags |= DB_DBT_USERMEM;
                d->data = DbstlReAlloc(d->data, sz);
                d->ulen = sz;
            }
            d->size = sz;
            if (t != NULL)
                wcscpy((wchar_t*)d->data, t);
            else
                memset(d->data, L'\0', sizeof(wchar_t));
        } else {
            freemem();
            d->data = ((t == NULL) ? (void *)L"" : (void *)t);
            d->size = sz;
            d->ulen = sz;
            d->flags = (DB_DBT_USERMEM);
            d->dlen = (INVALID_DLEN);
        }
    }

    inline void make_dbt(const char*& t, bool onstack)
    {
        make_dbt_chars(t, onstack);
    }

    inline void make_dbt(const char* const& t, bool onstack)
    {
        make_dbt_chars(t, onstack);
    }

    inline void make_dbt(char*& t, bool onstack)
    {
        make_dbt_chars(t, onstack);
    }

    inline void make_dbt(char* const& t, bool onstack)
    {
        make_dbt_chars(t, onstack);
    }

    inline void make_dbt(const string& t, bool onstack)
    {
        make_dbt_chars(t.c_str(), onstack);
    }

    inline void make_dbt(const wchar_t*& t, bool onstack)
    {
        make_dbt_wchars(t, onstack);
    }

    inline void make_dbt(const wchar_t* const& t, bool onstack)
    {
        make_dbt_wchars(t, onstack);
    }

    inline void make_dbt(wchar_t*& t, bool onstack)
    {
        make_dbt_wchars(t, onstack);
    }

    inline void make_dbt(wchar_t* const& t, bool onstack)
    {
        make_dbt_wchars(t, onstack);
    }

#ifdef HAVE_WSTRING
    inline void make_dbt(const wstring& t, bool onstack)
    {
        make_dbt_wchars(t.c_str(), onstack);
    }
#endif

    template <Typename T>
    void make_dbt_internal(const T*t, bool onstack)
    {
        u_int32_t i, sz, totalsz, sql;
        DBT *pdbt = (DBT *)&dbt_;
        typename DbstlElemTraits<T>::ElemSizeFunct szf = NULL;
        typename DbstlElemTraits<T>::SequenceLenFunct
            seqlenf = NULL;

        szf = DbstlElemTraits<T>::instance()->
            get_size_function();
        seqlenf = DbstlElemTraits<T>::instance()->
            get_sequence_len_function();

        assert(seqlenf != NULL);
        sql = sz = (u_int32_t)seqlenf(t);
        if (szf)
            for (i = 0, totalsz = 0; i < sz; i++)
                totalsz += szf(t[i]);
        else
            totalsz = sz * sizeof(T);

        sz = totalsz;

        if (onstack) {
            freemem();
            pdbt->data = (void *)t;
            pdbt->size = sz;
            pdbt->ulen = sz;
            pdbt->flags = DB_DBT_USERMEM;
            pdbt->dlen = INVALID_DLEN; // onstack flag;
        } else {
            // ulen stores the real length of the pointed memory.
            if (pdbt->ulen < sz) {
                pdbt->data = DbstlReAlloc(pdbt->data, sz);
                pdbt->ulen = sz;
                pdbt->flags |= DB_DBT_USERMEM;
            }
            pdbt->size = sz;

            DbstlElemTraits<T>::instance()->
                get_sequence_copy_function()
                    ((T *)pdbt->data, t, sql);
        }
    }

    // Store a sequence of base type T. Need four versions to catch all
    // possibilities otherwise the most generic one will be called.
    template <Typename T>
    inline void make_dbt(const T*const&tt, bool onstack)
    {
        make_dbt_internal((const T*)tt, onstack);
    }
    template <Typename T>
    inline void make_dbt(T*const&tt, bool onstack)
    {
        make_dbt_internal((const T*)tt, onstack);
    }
    template <Typename T>
    inline void make_dbt(T*&tt, bool onstack)
    {
        make_dbt_internal((const T*)tt, onstack);
    }
    template <Typename T>
    inline void make_dbt(const T*&tt, bool onstack)
    {
        make_dbt_internal((const T*)tt, onstack);
    }


public:
    inline DataItem(const char*& t, bool onstack)
    {
        make_dbt_chars(t, onstack);
    }

    inline DataItem(const char* const& t, bool onstack)
    {
        make_dbt_chars(t, onstack);
    }

    inline DataItem(char*& t, bool onstack)
    {
        make_dbt_chars(t, onstack);
    }

    inline DataItem(char* const& t, bool onstack)
    {
        make_dbt_chars(t, onstack);
    }

    inline DataItem(const string& t, bool onstack)
    {
        make_dbt_chars(t.c_str(), onstack);
    }

    inline DataItem(const wchar_t*& t, bool onstack)
    {
        make_dbt_wchars(t, onstack);
    }

    inline DataItem(const wchar_t* const& t, bool onstack)
    {
        make_dbt_wchars(t, onstack);
    }

    inline DataItem(wchar_t*& t, bool onstack)
    {
        make_dbt_wchars(t, onstack);
    }

    inline DataItem(wchar_t* const& t, bool onstack)
    {
        make_dbt_wchars(t, onstack);
    }

#ifdef HAVE_WSTRING
    inline DataItem(const wstring& t, bool onstack)
    {
        make_dbt_wchars(t.c_str(), onstack);
    }
#endif
    template<Typename T>
    inline DataItem(T*&tt, bool onstack)
    {
        make_dbt_internal((const T*)tt, onstack);
    }

    template<Typename T>
    inline DataItem(const T*&tt, bool onstack)
    {
        make_dbt_internal((const T*)tt, onstack);
    }

    template<Typename T>
    inline DataItem(T*const&tt, bool onstack)
    {
        make_dbt_internal((const T*)tt, onstack);
    }

    template<Typename T>
    inline DataItem(const T*const&tt, bool onstack)
    {
        make_dbt_internal((const T*)tt, onstack);
    }


}; // DataItem<>

bool operator==(const Dbt&d1, const Dbt&d2);
bool operator==(const DBT&d1, const DBT&d2);
END_NS

#endif // !_DB_STL_DBT_H

/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_STL_GLOBAL_INNER_OBJECT_
#define _DB_STL_GLOBAL_INNER_OBJECT_

#include "dbstl_common.h"

START_NS(dbstl)
/* 
 * This is the interface for all classes that has some global/singleton 
 * instances that will survive during the entire process lifetime and 
 * need to be deleted before process exit. Not deleting them won't make
 * a difference because they have to be alive when the process is alive,
 * they are not memory leaks. However, we will still delete them before
 * process exit, to make no memory leak reports by memory leak checkers.
 */
class _exported DbstlGlobalInnerObject
{
public:
    DbstlGlobalInnerObject(){}
    virtual ~DbstlGlobalInnerObject(){}

}; // DbstlGlobalInnerObject

void _exported register_global_object(DbstlGlobalInnerObject *gio);

// This class stores the pointer of an object allocated on heap, and when
// an instance of this class is destructed, it deletes that object.
// Any instance of this class can only be created on the heap so that we
// can control when to destruct its instances. It derives from 
// DbstlGlobalInnerObject so that we can register pointers to instances of
// this class into ResourceManager, just like other objects which implements
// the DbstlGlobalInnerObject interface. So the ultimate purpose for this
// template is to manage objects which can't implement DbstlGlobalInnerObject
// interface, like objects of Db, DbEnv, etc.
//
template<typename T>
class _exported DbstlHeapObject : public DbstlGlobalInnerObject
{
private:
    typedef DbstlHeapObject<T> self;
    T *obj;

    // Only allow creating to heap.
    DbstlHeapObject(T *obj1) { obj = obj1; }
public:
    static self *instance(T *obj1) { return new self(obj1); }
    virtual ~DbstlHeapObject() { delete obj; }
}; // DbstlHeapObject

END_NS
#endif // !_DB_STL_GLOBAL_INNER_OBJECT_


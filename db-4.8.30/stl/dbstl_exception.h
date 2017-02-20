/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_STL_EXCEPTION_H
#define _DB_STL_EXCEPTION_H

#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <iostream>
#include <exception>

#include "dbstl_common.h"

START_NS(dbstl)

using std::cerr;

// Internally used only.
void _exported throw_bdb_exception(const char *caller, int err_ret);
#define COPY_CONSTRUCTOR(type) type(const type& t) : DbstlException(t){}

/** \defgroup Exception_classes_group dbstl exception classes
dbstl throws several types of exceptions on several kinds of errors, the
exception classes form a class hiarachy. First, there is the DbstlException,
which is the base class for all types of dbstl specific concrete exception 
classes.
DbstlException inherits from the class DbException of Berkeley DB C++ API. Since
DbException class inherits from C++ STL exception base class std::exception, 
you can make use of all Berkeley DB C++ and dbstl API exceptions in the same
way you use the C++ std::exception class. 

Besides exceptions of DbstlException and its subclasses, dbstl may also
throw exceptions of DbException and its subclasses, which happens when a 
Berkeley DB call failed. So you should use the same way you catch Berkeley DB
C++ API exceptions when you want to catch exceptions throw by Berkeley DB 
operations.

When an exception occurs, dbstl initialize an local exception object on the 
stack and throws the exception object, so you should catch an exception like 
this:

try {
    // dbstl operations
}
catch(DbstlException ex){
    // Exception handling
    throw ex; // Optionally throw ex again
}

@{
*/

/// Base class of all dbstl exception classes. It is derived from Berkeley 
/// DB C++ API DbException class to maintain consistency with all 
/// Berkeley DB exceptions.
///
class _exported DbstlException : public DbException
{
public:
    explicit DbstlException(const char *msg) : DbException(msg) {}
    DbstlException(const char *msg, int err) : DbException(msg, err) {}
    DbstlException(const DbstlException&ex) : DbException(ex) {}
    explicit DbstlException(int err) : DbException(err) {}
    DbstlException(const char *prefix, const char *msg, int err) : 
        DbException(prefix, msg, err) {}

    const DbstlException& operator=(const DbstlException&exobj)
    {
        ASSIGNMENT_PREDCOND(exobj)
        DbException::operator =
            (dynamic_cast<const DbException&>(exobj));
        return exobj;
    }

    virtual ~DbstlException() throw(){}
};

/// Failed to allocate memory because memory is not enough.
class _exported NotEnoughMemoryException : public DbstlException
{
    size_t failed_size; // The size of the failed allocation.
public:
    NotEnoughMemoryException(const char *msg, size_t sz) 
        : DbstlException(msg)
    {
        failed_size = sz;
    }

     
    NotEnoughMemoryException(const NotEnoughMemoryException &ex)
        : DbstlException(ex)
    {
        this->failed_size = ex.failed_size;
    }
};

/// The iterator has inconsistent status, it is unable to be used any more.
class _exported InvalidIteratorException : public DbstlException
{
public:
    InvalidIteratorException() : DbstlException("Invalid Iterator")
    {
    }

    explicit InvalidIteratorException(int error_code) : 
        DbstlException("Invalid Iterator", error_code)
    {
    }
    COPY_CONSTRUCTOR(InvalidIteratorException)
};

/// The cursor has inconsistent status, it is unable to be used any more.
class _exported InvalidCursorException : public DbstlException
{
public:
    InvalidCursorException() : DbstlException("Invalid cursor")
    {
    }

    explicit InvalidCursorException(int error_code) : 
        DbstlException("Invalid cursor", error_code)
    {
    }
    COPY_CONSTRUCTOR(InvalidCursorException)
};

/// The Dbt object has inconsistent status or has no valid data, it is unable
/// to be used any more.
class _exported InvalidDbtException : public DbstlException
{
public:
    InvalidDbtException() : DbstlException("Invalid Dbt object")
    {
    }

    explicit InvalidDbtException(int error_code) : 
        DbstlException("Invalid Dbt object", error_code)
    {
    }
    COPY_CONSTRUCTOR(InvalidDbtException)
};

/// The assertions inside dbstl failed. The code file name and line number
/// will be passed to the exception object of this class.
class _exported FailedAssertionException : public DbstlException
{
private:
    char *err_msg_;
public:
    virtual const char *what() const throw()
    {
        return err_msg_;
    }

    FailedAssertionException(const char *fname, size_t lineno, 
        const char *msg) : DbstlException(0)
    {
        u_int32_t sz;
        char *str;
        
        str = (char *)DbstlMalloc(sz = (u_int32_t)(strlen(msg) + 
            strlen(fname) + 128));
        _snprintf(str, sz, 
            "In file %s at line %u, %s expression failed", 
            fname, (unsigned int)lineno, msg);
        err_msg_ = str;
#ifdef DEBUG
        fprintf(stderr, "%s", str);
#endif
    }

    FailedAssertionException(const FailedAssertionException&ex) : 
        DbstlException(ex)
    {
        err_msg_ = (char *)DbstlMalloc((u_int32_t)
            strlen(ex.err_msg_) + 1);
        strcpy(err_msg_, ex.err_msg_);
    }
    virtual ~FailedAssertionException() throw()
    {
        free(err_msg_);
    }
};

/// There is no such key in the database. The key can't not be passed into 
/// the exception instance because this class has to be a class template for
/// that to work.
class _exported NoSuchKeyException : public DbstlException
{
public:
    NoSuchKeyException() 
        : DbstlException("\nNo such key in the container.")
    {
    }

    COPY_CONSTRUCTOR(NoSuchKeyException)
};

/// Some argument of a function is invalid.
class _exported InvalidArgumentException : public DbstlException 
{
public:
    explicit InvalidArgumentException(const char *errmsg) : 
        DbstlException(errmsg)
    {
#ifdef DEBUG
        cerr<<errmsg;
#endif
    }

    InvalidArgumentException(const char *argtype, const char *arg) : 
        DbstlException(argtype, arg, 0)
    {
#ifdef DEBUG
        cerr<<"\nInvalid argument exception: "<<argtype<<"\t"<<arg;
#endif
    }

    COPY_CONSTRUCTOR(InvalidArgumentException)
};

/// The function called is not supported in this class.
class _exported NotSupportedException : public DbstlException 
{
public:
    explicit NotSupportedException(const char *str) : DbstlException(str)
    {
    }

    COPY_CONSTRUCTOR(NotSupportedException)
};

/// The function can not be called in this context or in current configurations.
class _exported InvalidFunctionCall : public DbstlException 
{
public:
    explicit InvalidFunctionCall(const char *str) : DbstlException(str)
    {
#ifdef DEBUG
        cerr<<"\nInvalid function call: "<<str;
#endif
    }

    COPY_CONSTRUCTOR(InvalidFunctionCall)
};
/** @}*/
#undef COPY_CONSTRUCTOR
END_NS

#endif //_DB_STL_EXCEPTION_H

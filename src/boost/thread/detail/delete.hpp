// Copyright (C) 2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THREAD_DETAIL_DELETE_HPP
#define BOOST_THREAD_DETAIL_DELETE_HPP

#include <boost/config.hpp>

/**
 * BOOST_THREAD_DELETE_COPY_CTOR deletes the copy constructor when the compiler supports it or
 * makes it private.
 *
 * BOOST_THREAD_DELETE_COPY_ASSIGN deletes the copy assignment when the compiler supports it or
 * makes it private.
 */

#if ! defined BOOST_NO_CXX11_DELETED_FUNCTIONS && ! defined BOOST_NO_CXX11_RVALUE_REFERENCES
#define BOOST_THREAD_DELETE_COPY_CTOR(CLASS) \
      CLASS(CLASS const&) = delete; \

#define BOOST_THREAD_DELETE_COPY_ASSIGN(CLASS) \
      CLASS& operator=(CLASS const&) = delete;

#else // BOOST_NO_CXX11_DELETED_FUNCTIONS
#if defined(BOOST_MSVC) && _MSC_VER >= 1600
#define BOOST_THREAD_DELETE_COPY_CTOR(CLASS) \
    private: \
      CLASS(CLASS const&); \
    public:

#define BOOST_THREAD_DELETE_COPY_ASSIGN(CLASS) \
    private: \
      CLASS& operator=(CLASS const&); \
    public:
#else
#define BOOST_THREAD_DELETE_COPY_CTOR(CLASS) \
    private: \
      CLASS(CLASS&); \
    public:

#define BOOST_THREAD_DELETE_COPY_ASSIGN(CLASS) \
    private: \
      CLASS& operator=(CLASS&); \
    public:
#endif
#endif // BOOST_NO_CXX11_DELETED_FUNCTIONS

/**
 * BOOST_THREAD_NO_COPYABLE deletes the copy constructor and assignment when the compiler supports it or
 * makes them private.
 */
#define BOOST_THREAD_NO_COPYABLE(CLASS) \
    BOOST_THREAD_DELETE_COPY_CTOR(CLASS) \
    BOOST_THREAD_DELETE_COPY_ASSIGN(CLASS)

#endif // BOOST_THREAD_DETAIL_DELETE_HPP

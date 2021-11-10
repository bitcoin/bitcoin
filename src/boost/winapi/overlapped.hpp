/*
 * Copyright 2016 Klemens D. Morgenstern
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef BOOST_WINAPI_OVERLAPPED_HPP_INCLUDED_
#define BOOST_WINAPI_OVERLAPPED_HPP_INCLUDED_

#include <boost/winapi/basic_types.hpp>
#include <boost/winapi/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if !defined( BOOST_USE_WINDOWS_H )
extern "C" {
struct _OVERLAPPED;
}
#endif

namespace boost {
namespace winapi {

typedef struct BOOST_MAY_ALIAS _OVERLAPPED {
    ULONG_PTR_ Internal;
    ULONG_PTR_ InternalHigh;
    union {
        BOOST_WINAPI_DETAIL_EXTENSION struct {
            DWORD_ Offset;
            DWORD_ OffsetHigh;
        };
        PVOID_  Pointer;
    };
    HANDLE_    hEvent;
} OVERLAPPED_, *LPOVERLAPPED_;

} // namespace winapi
} // namespace boost

#include <boost/winapi/detail/footer.hpp>

#endif // BOOST_WINAPI_OVERLAPPED_HPP_INCLUDED_

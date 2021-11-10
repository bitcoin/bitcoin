/*
 * Copyright 2020 Andrey Semashev
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef BOOST_WINAPI_WAIT_ON_ADDRESS_HPP_INCLUDED_
#define BOOST_WINAPI_WAIT_ON_ADDRESS_HPP_INCLUDED_

#include <boost/winapi/config.hpp>

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN8 && (BOOST_WINAPI_PARTITION_APP || BOOST_WINAPI_PARTITION_SYSTEM)

#include <boost/winapi/basic_types.hpp>
#include <boost/winapi/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if !defined(BOOST_USE_WINDOWS_H)
extern "C" {

// Note: These functions are not dllimport
boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
WaitOnAddress(
    volatile boost::winapi::VOID_* addr,
    boost::winapi::PVOID_ compare_addr,
    boost::winapi::SIZE_T_ size,
    boost::winapi::DWORD_ timeout_ms);

boost::winapi::VOID_ BOOST_WINAPI_WINAPI_CC
WakeByAddressSingle(boost::winapi::PVOID_ addr);

boost::winapi::VOID_ BOOST_WINAPI_WINAPI_CC
WakeByAddressAll(boost::winapi::PVOID_ addr);

} // extern "C"
#endif // !defined(BOOST_USE_WINDOWS_H)

namespace boost {
namespace winapi {

using ::WaitOnAddress;
using ::WakeByAddressSingle;
using ::WakeByAddressAll;

}
}

#include <boost/winapi/detail/footer.hpp>

#endif // BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN8 && (BOOST_WINAPI_PARTITION_APP || BOOST_WINAPI_PARTITION_SYSTEM)

#endif // BOOST_WINAPI_WAIT_ON_ADDRESS_HPP_INCLUDED_

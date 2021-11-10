/*
 * Copyright 2015 Andrey Semashev
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef BOOST_WINAPI_DETAIL_CAST_PTR_HPP_INCLUDED_
#define BOOST_WINAPI_DETAIL_CAST_PTR_HPP_INCLUDED_

#include <boost/winapi/config.hpp>
#include <boost/winapi/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace winapi {
namespace detail {

//! This class is used to automatically cast pointers to the type used in the current Windows SDK function declarations
class cast_ptr
{
private:
    const void* m_p;

public:
    explicit BOOST_FORCEINLINE cast_ptr(const void* p) BOOST_NOEXCEPT : m_p(p) {}
    template< typename T >
    BOOST_FORCEINLINE operator T* () const BOOST_NOEXCEPT { return (T*)m_p; }
};

}
}
}

#include <boost/winapi/detail/footer.hpp>

#endif // BOOST_WINAPI_DETAIL_CAST_PTR_HPP_INCLUDED_

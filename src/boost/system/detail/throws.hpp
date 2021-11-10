#ifndef BOOST_SYSTEM_DETAIL_THROWS_HPP_INCLUDED
#define BOOST_SYSTEM_DETAIL_THROWS_HPP_INCLUDED

//  Copyright Beman Dawes 2006, 2007
//  Copyright Christoper Kohlhoff 2007
//  Copyright Peter Dimov 2017, 2018
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  See library home page at http://www.boost.org/libs/system

namespace boost
{

namespace system
{

class error_code;

} // namespace system

// boost::throws()

namespace detail
{

//  Misuse of the error_code object is turned into a noisy failure by
//  poisoning the reference. This particular implementation doesn't
//  produce warnings or errors from popular compilers, is very efficient
//  (as determined by inspecting generated code), and does not suffer
//  from order of initialization problems. In practice, it also seems
//  cause user function error handling implementation errors to be detected
//  very early in the development cycle.

inline system::error_code* throws()
{
    // See github.com/boostorg/system/pull/12 by visigoth for why the return
    // is poisoned with nonzero rather than (0). A test, test_throws_usage(),
    // has been added to error_code_test.cpp, and as visigoth mentioned it
    // fails on clang for release builds with a return of 0 but works fine
    // with (1).
    // Since the undefined behavior sanitizer (-fsanitize=undefined) does not
    // allow a reference to be formed to the unaligned address of (1), we use
    // (8) instead.

    return reinterpret_cast<system::error_code*>(8);
}

} // namespace detail

inline system::error_code& throws()
{
    return *detail::throws();
}

} // namespace boost

#endif // #ifndef BOOST_SYSTEM_DETAIL_THROWS_HPP_INCLUDED

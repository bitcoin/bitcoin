
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_EXCEPTIONS_H
#define BOOST_COROUTINES_EXCEPTIONS_H

#include <stdexcept>
#include <string>

#include <boost/config.hpp>
#include <boost/core/scoped_enum.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/type_traits/integral_constant.hpp>

#include <boost/coroutine/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines {
namespace detail {

struct forced_unwind {};

}

BOOST_SCOPED_ENUM_DECLARE_BEGIN(coroutine_errc)
{
  no_data = 1
}
BOOST_SCOPED_ENUM_DECLARE_END(coroutine_errc)

BOOST_COROUTINES_DECL
system::error_category const& coroutine_category() BOOST_NOEXCEPT;

}

namespace system {

template<>
struct is_error_code_enum< coroutines::coroutine_errc > : public true_type
{};

#ifdef BOOST_NO_CXX11_SCOPED_ENUMS
template<>
struct is_error_code_enum< coroutines::coroutine_errc::enum_type > : public true_type
{};
#endif

inline
error_code make_error_code( coroutines::coroutine_errc e) //BOOST_NOEXCEPT
{
    return error_code( underlying_cast< int >( e), coroutines::coroutine_category() );
}

inline
error_condition make_error_condition( coroutines::coroutine_errc e) //BOOST_NOEXCEPT
{
    return error_condition( underlying_cast< int >( e), coroutines::coroutine_category() );
}

}

namespace coroutines {

class coroutine_error : public std::logic_error
{
private:
    system::error_code  ec_;

public:
    coroutine_error( system::error_code ec) :
        logic_error( ec.message() ),
        ec_( ec)
    {}

    system::error_code const& code() const BOOST_NOEXCEPT
    { return ec_; }
};

class invalid_result : public coroutine_error
{
public:
    invalid_result() :
        coroutine_error(
            system::make_error_code(
                coroutine_errc::no_data) )
    {}
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES_EXCEPTIONS_H

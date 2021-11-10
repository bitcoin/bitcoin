//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_STRING_PARAM_HPP
#define BOOST_BEAST_STRING_PARAM_HPP

#if defined(BOOST_BEAST_ALLOW_DEPRECATED) && !defined(BOOST_BEAST_DOXYGEN)


#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/string.hpp>
#include <boost/beast/core/static_string.hpp>
#include <boost/beast/core/detail/static_ostream.hpp>
#include <boost/optional.hpp>

namespace boost {
namespace beast {

/** A function parameter which efficiently converts to string.

    This is used as a function parameter type to allow callers
    notational convenience: objects other than strings may be
    passed in contexts where a string is expected. The conversion
    to string is made using `operator<<` to a non-dynamically
    allocated static buffer if possible, else to a `std::string`
    on overflow.

    To use it, modify your function signature to accept
    `string_param` and then extract the string inside the
    function:
    @code
    void print(string_param s)
    {
        std::cout << s.str();
    }
    @endcode
*/
class string_param
{
    string_view sv_;
    char buf_[128];
    boost::optional<detail::static_ostream> os_;

    template<class T>
    typename std::enable_if<
        std::is_integral<T>::value>::type
    print(T const&);

    template<class T>
    typename std::enable_if<
        ! std::is_integral<T>::value &&
        ! std::is_convertible<T, string_view>::value
    >::type
    print(T const&);

    void
    print(string_view);

    template<class T>
    typename std::enable_if<
        std::is_integral<T>::value>::type
    print_1(T const&);

    template<class T>
    typename std::enable_if<
        ! std::is_integral<T>::value>::type
    print_1(T const&);

    void
    print_n()
    {
    }

    template<class T0, class... TN>
    void
    print_n(T0 const&, TN const&...);

    template<class T0, class T1, class... TN>
    void
    print(T0 const&, T1 const&, TN const&...);

public:
    /// Copy constructor (disallowed)
    string_param(string_param const&) = delete;

    /// Copy assignment (disallowed)
    string_param& operator=(string_param const&) = delete;

    /** Constructor

        This function constructs a string as if by concatenating
        the result of streaming each argument in order into an
        output stream. It is used as a notational convenience
        at call sites which expect a parameter with the semantics
        of a @ref string_view.

        The implementation uses a small, internal static buffer
        to avoid memory allocations especially for the case where
        the list of arguments to be converted consists of a single
        integral type.

        @param args One or more arguments to convert
    */
    template<class... Args>
    string_param(Args const&... args);

    /// Returns the contained string
    string_view
    str() const
    {
        return sv_;
    }

    /// Implicit conversion to @ref string_view
    operator string_view const() const
    {
        return sv_;
    }
};

} // beast
} // boost

#include <boost/beast/core/impl/string_param.hpp>

#endif // defined(BOOST_BEAST_ALLOW_DEPRECATED) && !BOOST_BEAST_DOXYGEN

#endif

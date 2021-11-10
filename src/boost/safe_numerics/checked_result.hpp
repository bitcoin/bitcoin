#ifndef BOOST_NUMERIC_CHECKED_RESULT
#define BOOST_NUMERIC_CHECKED_RESULT

//  Copyright (c) 2012 Robert Ramey
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// contains operations for doing checked aritmetic on NATIVE
// C++ types.
#include <cassert>
#include <type_traits> // is_convertible
#include "exception.hpp"

namespace boost {
namespace safe_numerics {

template<typename R>
struct checked_result {
    const safe_numerics_error m_e;
    union contents {
        R m_r;
        char const * const m_msg;
        // contstructors for different types
        constexpr contents(const R & r) noexcept : m_r(r){}
        constexpr contents(char const * msg) noexcept : m_msg(msg) {}
        constexpr operator R () noexcept {
            return m_r;
        }
        constexpr operator char const * () noexcept {
            return m_msg;
        }
    };
    contents m_contents;

    // don't permit construction without initial value;
    checked_result() = delete;
    checked_result(const checked_result & r) = default;
    checked_result(checked_result && r) = default;
    
    constexpr /*explicit*/ checked_result(const R & r) noexcept :
        m_e(safe_numerics_error::success),
        m_contents{r}
    {}

    constexpr /*explicit*/ checked_result(
        const safe_numerics_error & e,
        const char * msg = ""
    )  noexcept :
        m_e(e),
        m_contents{msg}
    {
        assert(m_e != safe_numerics_error::success);
    }

    // permit construct from another checked result type
    template<typename T>
    constexpr /*explicit*/ checked_result(const checked_result<T> & t) noexcept :
        m_e(t.m_e)
    {
        static_assert(
            std::is_convertible<T, R>::value,
            "T must be convertible to R"
        );
        if(safe_numerics_error::success == t.m_e)
            m_contents.m_r = t.m_r;
        else
            m_contents.m_msg = t.m_msg;
    }

    constexpr bool exception() const {
        return m_e != safe_numerics_error::success;
    }

    // accesors
    constexpr operator R() const noexcept{
        // don't assert here.  Let the library catch these errors
        // assert(! exception());
        return m_contents.m_r;
    }
    
    constexpr operator safe_numerics_error () const noexcept{
        // note that this is a legitimate operation even when
        // the operation was successful - it will return success
        return m_e;
    }
    constexpr operator const char *() const noexcept{
        assert(exception());
        return m_contents.m_msg;
    }

    // disallow assignment
    checked_result & operator=(const checked_result &) = delete;
}; // checked_result

template <class R>
class make_checked_result {
public:
    template<safe_numerics_error E>
    constexpr static checked_result<R> invoke(
        char const * const & m
    ) noexcept {
        return checked_result<R>(E, m);
    }
};

} // safe_numerics
} // boost

#endif  // BOOST_NUMERIC_CHECKED_RESULT

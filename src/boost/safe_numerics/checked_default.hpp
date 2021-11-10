#ifndef BOOST_NUMERIC_CHECKED_DEFAULT_HPP
#define BOOST_NUMERIC_CHECKED_DEFAULT_HPP

//  Copyright (c) 2017 Robert Ramey
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// contains operation implementation of arithmetic operators
// on built-in types.  The default implementation is to just
// invoke the operation with no checking.  These are overloaded
// for specific types such as integer, etc.

// implement the equivant of template partial specialization for functions

// what we need is
// a) a default implementation of add, subtract, etc which just
// implements the standard operations and returns the result
// b) specific implementations to be called from safe implementation
// such as safe<int> ... and someday maybe money<T, D> ...
//
// What we need is partial function specialization - but this doesn't
// exist in C++ (yet?).  But particial specialization of structures DOES
// exist.  So put our functions into a class which can then be
// partially specialized.  Finally. add a function interface to so that
// data types can be deduced from the function call.  We now have
// the equivalent of partial function template specialization.

// usage example: checked<int>::add(t, u) ...

#include <boost/logic/tribool.hpp>
#include "checked_result.hpp"

namespace boost {
namespace safe_numerics {

// main function object which contains functions which handle
// primitives which haven't been overriden.  For now, these
// implement the default operation.  But I see this as an indicator
// that there is more work to be done.  For example float * int should
// never be called because promotions on operands should occur before
// the operation is invoked. So rather than returning the default operation
// it should trap with a static_assert. This occurs at compile time while
// calculating result interval.  This needs more investigation.

template<
    typename R,
    R Min,
    R Max,
    typename T,
    class F = make_checked_result<R>,
    class Default = void
>
struct heterogeneous_checked_operation {
    constexpr static checked_result<R>
    cast(const T & t) /* noexcept */ {
        return static_cast<R>(t);
    }
};

template<
    typename R,
    class F = make_checked_result<R>,
    class Default = void
>
struct checked_operation{
    constexpr static checked_result<R>
    minus(const R & t) noexcept {
        return - t;
    }
    constexpr static checked_result<R>
    add(const R & t, const R & u) noexcept {
        return t + u;
    }
    constexpr static checked_result<R>
    subtract(const R & t, const R & u) noexcept {
        return t - u;
    }
    constexpr static checked_result<R>
    multiply(const R & t, const R & u) noexcept {
        return t * u;
    }
    constexpr static checked_result<R>
    divide(const R & t, const R & u) noexcept {
        return t / u;
    }
    constexpr static checked_result<R>
    modulus(const R & t, const R & u) noexcept {
        return t % u;
    }
    constexpr static boost::logic::tribool
    less_than(const R & t, const R & u) noexcept {
        return t < u;
    }
    constexpr static boost::logic::tribool
    greater_than(const R & t, const R & u) noexcept {
        return t > u;
    }
    constexpr static boost::logic::tribool
    equal(const R & t, const R & u) noexcept {
        return t < u;
    }
    constexpr static checked_result<R>
    left_shift(const R & t, const R & u) noexcept {
        return t << u;
    }
    constexpr static checked_result<R>
    right_shift(const R & t, const R & u) noexcept {
        return t >> u;
    }
    constexpr static checked_result<R>
    bitwise_or(const R & t, const R & u) noexcept {
        return t | u;
    }
    constexpr static checked_result<R>
    bitwise_xor(const R & t, const R & u) noexcept {
        return t ^ u;
    }
    constexpr static checked_result<R>
    bitwise_and(const R & t, const R & u) noexcept {
        return t & u;
    }
    constexpr static checked_result<R>
    bitwise_not(const R & t) noexcept {
        return ~t;
    }
};

namespace checked {

// implement function call interface so that types other than
// the result type R can be deduced from the function parameters.

template<typename R, typename T>
constexpr inline checked_result<R> cast(const T & t) /* noexcept */ {
    return heterogeneous_checked_operation<
        R,
        std::numeric_limits<R>::min(),
        std::numeric_limits<R>::max(),
        T
    >::cast(t);
}
template<typename R>
constexpr inline checked_result<R> minus(const R & t) noexcept {
    return checked_operation<R>::minus(t);
}
template<typename R>
constexpr inline checked_result<R> add(const R & t, const R & u) noexcept {
    return checked_operation<R>::add(t, u);
}
template<typename R>
constexpr inline checked_result<R> subtract(const R & t, const R & u) noexcept {
    return checked_operation<R>::subtract(t, u);
}
template<typename R>
constexpr inline checked_result<R> multiply(const R & t, const R & u) noexcept {
    return checked_operation<R>::multiply(t, u);
}
template<typename R>
constexpr inline checked_result<R> divide(const R & t, const R & u) noexcept {
    return checked_operation<R>::divide(t, u);
}
template<typename R>
constexpr inline checked_result<R> modulus(const R & t, const R & u) noexcept {
    return checked_operation<R>::modulus(t, u);
}
template<typename R>
constexpr inline checked_result<bool> less_than(const R & t, const R & u) noexcept {
    return checked_operation<R>::less_than(t, u);
}
template<typename R>
constexpr inline checked_result<bool> greater_than_equal(const R & t, const R & u) noexcept {
    return ! checked_operation<R>::less_than(t, u);
}
template<typename R>
constexpr inline checked_result<bool> greater_than(const R & t, const R & u) noexcept {
    return checked_operation<R>::greater_than(t, u);
}
template<typename R>
constexpr inline checked_result<bool> less_than_equal(const R & t, const R & u) noexcept {
    return ! checked_operation<R>::greater_than(t, u);
}
template<typename R>
constexpr inline checked_result<bool> equal(const R & t, const R & u) noexcept {
    return checked_operation<R>::equal(t, u);
}
template<typename R>
constexpr inline checked_result<R> left_shift(const R & t, const R & u) noexcept {
    return checked_operation<R>::left_shift(t, u);
}
template<typename R>
constexpr inline checked_result<R> right_shift(const R & t, const R & u) noexcept {
    return checked_operation<R>::right_shift(t, u);
}
template<typename R>
constexpr inline checked_result<R> bitwise_or(const R & t, const R & u) noexcept {
    return checked_operation<R>::bitwise_or(t, u);
}
template<typename R>
constexpr inline checked_result<R> bitwise_xor(const R & t, const R & u) noexcept {
    return checked_operation<R>::bitwise_xor(t, u);
}
template<typename R>
constexpr inline checked_result<R> bitwise_and(const R & t, const R & u) noexcept {
    return checked_operation<R>::bitwise_and(t, u);
}
template<typename R>
constexpr inline checked_result<R> bitwise_not(const R & t) noexcept {
    return checked_operation<R>::bitwise_not(t);
}

} // checked
} // safe_numerics
} // boost

#endif // BOOST_NUMERIC_CHECKED_DEFAULT_HPP

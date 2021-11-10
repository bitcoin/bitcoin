#ifndef BOOST_NUMERIC_EXCEPTION_POLICIES_HPP
#define BOOST_NUMERIC_EXCEPTION_POLICIES_HPP

//  Copyright (c) 2015 Robert Ramey
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/mp11.hpp>
#include <boost/config.hpp> // BOOST_NO_EXCEPTIONS
#include "exception.hpp"

namespace boost {
namespace safe_numerics {

template<
    typename AE,
    typename IDB,
    typename UB,
    typename UV
>
struct exception_policy {
    constexpr static void on_arithmetic_error(
        const safe_numerics_error & e,
        const char * msg
    ){
        AE()(e, msg);
    }
    constexpr static void on_implementation_defined_behavior(
        const safe_numerics_error & e,
        const char * msg
    ){
        IDB()(e, msg);
    }
    constexpr static void on_undefined_behavior(
        const safe_numerics_error & e,
        const char * msg
    ){
        UB()(e, msg);
    }
    constexpr static void on_uninitialized_value(
        const safe_numerics_error & e,
        const char * msg
    ){
        UV()(e, msg);
    }
};

////////////////////////////////////////////////////////////////////////////////
// pre-made error action handers

// ignore any error and just return.
struct ignore_exception {
    constexpr ignore_exception() = default;
    constexpr void operator () (
        const boost::safe_numerics::safe_numerics_error &,
        const char *
    ){}
};

// emit compile time error if this is invoked.
struct trap_exception {
    constexpr trap_exception() = default;
    // error will occur on operator call.
    // hopefully this will display arguments
};

// If an exceptional condition is detected at runtime throw the exception.
struct throw_exception {
    constexpr throw_exception() = default;
    #ifndef BOOST_NO_EXCEPTIONS
    void operator()(
        const safe_numerics_error & e,
        const char * message
    ){
        throw std::system_error(std::error_code(e), message);
    }
    #else
    constexpr trap_exception()(const safe_numerics_error & e, const char * message);
    #endif
};

// given an error code - return the action code which it corresponds to.
constexpr inline safe_numerics_actions
make_safe_numerics_action(const safe_numerics_error & e){
    // we can't use standard algorithms since we want this to be constexpr
    // this brute force solution is simple and pretty fast anyway
    switch(e){
    case safe_numerics_error::negative_overflow_error:
    case safe_numerics_error::underflow_error:
    case safe_numerics_error::range_error:
    case safe_numerics_error::domain_error:
    case safe_numerics_error::positive_overflow_error:
    case safe_numerics_error::precision_overflow_error:
        return safe_numerics_actions::arithmetic_error;

    case safe_numerics_error::negative_value_shift:
    case safe_numerics_error::negative_shift:
    case safe_numerics_error::shift_too_large:
        return safe_numerics_actions::implementation_defined_behavior;

    case safe_numerics_error::uninitialized_value:
        return safe_numerics_actions::uninitialized_value;

    case safe_numerics_error::success:
        return safe_numerics_actions::no_action;
    default:
        assert(false);
    }
    // should never arrive here
    //include to suppress bogus warning
    return safe_numerics_actions::no_action;
}

////////////////////////////////////////////////////////////////////////////////
// pre-made error policy classes

// loose exception
// - throw on arithmetic errors
// - ignore other errors.
// Some applications ignore these issues and still work and we don't
// want to update them.
using loose_exception_policy = exception_policy<
    throw_exception,    // arithmetic error
    ignore_exception,   // implementation defined behavior
    ignore_exception,   // undefined behavior
    ignore_exception     // uninitialized value
>;

// loose trap
// same as above in that it doesn't check for various undefined behaviors
// but traps at compile time for hard arithmetic errors.  This policy
// would be suitable for older embedded systems which depend on
// bit manipulation operations to work.
using loose_trap_policy = exception_policy<
    trap_exception,    // arithmetic error
    ignore_exception,  // implementation defined behavior
    ignore_exception,  // undefined behavior
    ignore_exception   // uninitialized value
>;

// strict exception
// - throw at runtime on any kind of error
// recommended for new code.  Check everything at compile time
// if possible and runtime if necessary.  Trap or Throw as
// appropriate.  Should guarantee code to be portable across
// architectures.
using strict_exception_policy = exception_policy<
    throw_exception,
    throw_exception,
    throw_exception,
    ignore_exception
>;

// strict trap
// Same as above but requires code to be written in such a way as to
// make it impossible for errors to occur.  This naturally will require
// extra coding effort but might be justified for embedded and/or
// safety critical systems.
using strict_trap_policy = exception_policy<
    trap_exception,
    trap_exception,
    trap_exception,
    trap_exception
>;

// default policy
// One would use this first. After experimentation, one might
// replace some actions with ignore_exception
using default_exception_policy = strict_exception_policy;

} // namespace safe_numerics
} // namespace boost

#endif // BOOST_NUMERIC_EXCEPTION_POLICIES_HPP

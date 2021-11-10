#ifndef BOOST_NUMERIC_EXCEPTION
#define BOOST_NUMERIC_EXCEPTION

//  Copyright (c) 2012 Robert Ramey
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// contains error indicators for results of doing checked
// arithmetic on native C++ types

#include <algorithm>
#include <system_error> // error_code, system_error
#include <string>
#include <cassert>
#include <cstdint> // std::uint8_t

// Using the system_error code facility.  This facility is more complex
// than meets the eye.  To fully understand what out intent here is,
// review http://blog.think-async.com/2010/04/system-error-support-in-c0x-part-5.html
// "Giving context-specific meaning to generic error codes"

namespace boost {
namespace safe_numerics {

// errors codes for safe numerics

// in spite of the similarity, this list is distinct from the exceptions
// listed in documentation for std::exception.

// note: Don't reorder these.  Code in the file checked_result_operations.hpp
// depends upon this order !!!
enum class safe_numerics_error : std::uint8_t {
    success = 0,
    positive_overflow_error,    // result is above representational maximum
    negative_overflow_error,    // result is below representational minimum
    domain_error,               // one operand is out of valid range
    range_error,                // result cannot be produced for this operation
    precision_overflow_error,   // result lost precision
    underflow_error,            // result is too small to be represented
    negative_value_shift,       // negative value in shift operator
    negative_shift,             // shift a negative value
    shift_too_large,            // l/r shift exceeds variable size
    uninitialized_value         // creating of uninitialized value
};

constexpr inline const char * literal_string(const safe_numerics_error & e){
    switch(e){
    case safe_numerics_error::success: return "success";
    case safe_numerics_error::positive_overflow_error: return "positive_overflow_error";
    case safe_numerics_error::negative_overflow_error: return "negative_overflow_error";
    case safe_numerics_error::domain_error: return "domain_error";
    case safe_numerics_error::range_error: return "range_error";
    case safe_numerics_error::precision_overflow_error: return "precision_overflow_error";
    case safe_numerics_error::underflow_error: return "underflow_error";
    case safe_numerics_error::negative_value_shift: return "negative_value_shift";
    case safe_numerics_error::negative_shift: return "negative_shift";
    case safe_numerics_error::shift_too_large: return "shift_too_large";
    case safe_numerics_error::uninitialized_value: return "uninitialized_value";
    default:
        assert(false); // should never arrive here
    }
}

const std::uint8_t safe_numerics_casting_error_count =
    static_cast<std::uint8_t>(safe_numerics_error::domain_error) + 1;

const std::uint8_t safe_numerics_error_count =
    static_cast<std::uint8_t>(safe_numerics_error::uninitialized_value) + 1;

} // safe_numerics
} // boost

namespace std {
    template <>
    struct is_error_code_enum<boost::safe_numerics::safe_numerics_error>
        : public true_type {};
} // std

namespace boost {
namespace safe_numerics {

const class : public std::error_category {
public:
    virtual const char* name() const noexcept{
        return "safe numerics error";
    }
    virtual std::string message(int ev) const {
        switch(static_cast<safe_numerics_error>(ev)){
        case safe_numerics_error::success:
            return "success";
        case safe_numerics_error::positive_overflow_error:
            return "positive overflow error";
        case safe_numerics_error::negative_overflow_error:
            return "negative overflow error";
        case safe_numerics_error::underflow_error:
            return "underflow error";
        case safe_numerics_error::range_error:
            return "range error";
        case safe_numerics_error::precision_overflow_error:
            return "precision_overflow_error";
        case safe_numerics_error::domain_error:
            return "domain error";
        case safe_numerics_error::negative_shift:
            return "negative shift";
        case safe_numerics_error::negative_value_shift:
            return "negative value shift";
        case safe_numerics_error::shift_too_large:
            return "shift too large";
        case safe_numerics_error::uninitialized_value:
            return "uninitialized value";
        default:
            assert(false);
        }
        return ""; // suppress bogus warning
    }
} safe_numerics_error_category {};

// constexpr - damn, can't use constexpr due to std::error_code
inline std::error_code make_error_code(const safe_numerics_error & e){
    return std::error_code(static_cast<int>(e), safe_numerics_error_category);
}

// actions for error_codes for safe numerics.  I've leveraged on
// error_condition in order to do this.  I'm not sure this is a good
// idea or not.

enum class safe_numerics_actions {
    no_action = 0,
    uninitialized_value,
    arithmetic_error,
    implementation_defined_behavior,
    undefined_behavior
};

} // safe_numerics
} // boost

namespace std {
    template <>
    struct is_error_condition_enum<boost::safe_numerics::safe_numerics_actions>
        : public true_type {};
} // std

namespace boost {
namespace safe_numerics {

const class : public std::error_category {
public:
    virtual const char* name() const noexcept {
        return "safe numerics error group";
    }
    virtual std::string message(int) const {
        return "safe numerics error group";
    }
    // return true if a given error code corresponds to a
    // given safe numeric action
    virtual bool equivalent(
        const std::error_code & code,
        int condition
    ) const noexcept {
        if(code.category() != safe_numerics_error_category)
            return false;
        switch (static_cast<safe_numerics_actions>(condition)){
        case safe_numerics_actions::no_action:
            return code == safe_numerics_error::success;
        case safe_numerics_actions::uninitialized_value:
            return code == safe_numerics_error::uninitialized_value;
        case safe_numerics_actions::arithmetic_error:
            return code == safe_numerics_error::positive_overflow_error
                || code == safe_numerics_error::negative_overflow_error
                || code == safe_numerics_error::underflow_error
                || code == safe_numerics_error::range_error
                || code == safe_numerics_error::domain_error;
        case safe_numerics_actions::implementation_defined_behavior:
            return code == safe_numerics_error::negative_value_shift
                || code == safe_numerics_error::negative_shift
                || code == safe_numerics_error::shift_too_large;
        case safe_numerics_actions::undefined_behavior:
            return false;
        default:
            ;
        }
        // should never arrive here
        assert(false);
        // suppress bogus warning
        return false; 
    }
} safe_numerics_actions_category {};

// the following function is used to "finish" implementation of conversion
// of safe_numerics_error to std::error_condition.  At least for now, this
// isn't being used and defining here it can lead duplicate symbol errors
// depending on the compiler.  So suppress it until further notice
#if 0
std::error_condition make_error_condition(const safe_numerics_error & e) {
    return std::error_condition(
        static_cast<int>(e),
        safe_numerics_error_category
    );
}
#endif

} // safe_numerics
} // boost

#endif // BOOST_NUMERIC_CHECKED_RESULT

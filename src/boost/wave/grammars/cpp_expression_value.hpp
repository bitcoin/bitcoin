/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_CPP_EXPRESSION_VALUE_HPP_452FE66D_8754_4107_AF1E_E42255A0C18A_INCLUDED)
#define BOOST_CPP_EXPRESSION_VALUE_HPP_452FE66D_8754_4107_AF1E_E42255A0C18A_INCLUDED

#if defined (BOOST_SPIRIT_DEBUG)
#include <iostream>
#endif // defined(BOOST_SPIRIT_DEBUG)

#include <boost/wave/wave_config.hpp>
#include <boost/wave/grammars/cpp_value_error.hpp> // value_error

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace grammars {
namespace closures {

class closure_value;
inline bool as_bool(closure_value const& v);

///////////////////////////////////////////////////////////////////////////////
//
//  The closure_value class represents the closure type, which is used for the
//  expression grammar.
//
//      This class was introduced to allow the expression grammar to respect
//      the numeric type of a numeric literal or expression result.
//
///////////////////////////////////////////////////////////////////////////////
class closure_value {
public:

    enum value_type {
        is_int = 1,
        is_uint = 2,
        is_bool = 3
    };

    closure_value(value_error valid_ = error_noerror)
    : type(is_int), valid(valid_)
    { value.i = 0; }
    explicit closure_value(int i, value_error valid_ = error_noerror)
    : type(is_int), valid(valid_)
    { value.i = i; }
    explicit closure_value(unsigned int ui, value_error valid_ = error_noerror)
    : type(is_uint), valid(valid_)
    { value.ui = ui; }
    explicit closure_value(int_literal_type i, value_error valid_ = error_noerror)
    : type(is_int), valid(valid_)
    { value.i = i; }
    explicit closure_value(uint_literal_type ui, value_error valid_ = error_noerror)
    : type(is_uint), valid(valid_)
    { value.ui = ui; }
    explicit closure_value(bool b, value_error valid_ = error_noerror)
    : type(is_bool), valid(valid_)
    { value.b = b; }

    value_type get_type() const { return type; }
    value_error is_valid() const { return valid; }

    // explicit conversion
    friend int_literal_type as_int(closure_value const& v)
    {
        switch (v.type) {
        case is_uint:   return v.value.ui;
        case is_bool:   return v.value.b ? 1 : 0;
        case is_int:    break;
        }
        return v.value.i;
    }
    friend uint_literal_type as_uint(closure_value const& v)
    {
        switch (v.type) {
        case is_uint:   return v.value.ui;
        case is_bool:   return v.value.b ? 1 : 0;
        case is_int:    break;
        }
        return v.value.i;
    }
    friend int_literal_type as_long(closure_value const& v)
    {
        switch (v.type) {
        case is_uint:   return v.value.ui;
        case is_bool:   return v.value.b ? 1 : 0;
        case is_int:    break;
        }
        return v.value.i;
    }
    friend uint_literal_type as_ulong(closure_value const& v)
    {
        switch (v.type) {
        case is_uint:   return v.value.ui;
        case is_bool:   return v.value.b ? 1 : 0;
        case is_int:    break;
        }
        return v.value.i;
    }
    friend bool as_bool(closure_value const& v)
    {
        switch (v.type) {
        case is_uint:   return v.value.ui != 0;
        case is_bool:   return v.value.b;
        case is_int:    break;
        }
        return v.value.i != 0.0;
    }

    // assignment
    closure_value &operator= (closure_value const &rhs)
    {
        switch (rhs.get_type()) {
        case is_int:
            value.i = as_long(rhs);
            type = is_int;
            break;

        case is_uint:
            value.ui = as_ulong(rhs);
            type = is_uint;
            break;

        case is_bool:
            value.b = as_bool(rhs);
            type = is_bool;
            break;
        }
        valid = rhs.valid;
        return *this;
    }
    closure_value &operator= (int rhs)
    {
        type = is_int;
        value.i = rhs;
        valid = error_noerror;
        return *this;
    }
    closure_value &operator= (unsigned int rhs)
    {
        type = is_uint;
        value.ui = rhs;
        valid = error_noerror;
        return *this;
    }
    closure_value &operator= (int_literal_type rhs)
    {
        type = is_int;
        value.i = rhs;
        valid = error_noerror;
        return *this;
    }
    closure_value &operator= (uint_literal_type rhs)
    {
        type = is_uint;
        value.ui = rhs;
        valid = error_noerror;
        return *this;
    }
    closure_value &operator= (bool rhs)
    {
        type = is_bool;
        value.b = rhs;
        valid = error_noerror;
        return *this;
    }

    // arithmetics
    closure_value &operator+= (closure_value const &rhs)
    {
        switch (type) {
        case is_int:
            switch(rhs.type) {
            case is_bool:
                {
                    int_literal_type result = value.i + as_long(rhs);
                    if ((rhs.value.i > 0L && value.i > result) ||
                        (rhs.value.i < 0L && value.i < result))
                    {
                        valid = error_integer_overflow;
                    }
                    else {
                        value.i = result;
                    }
                }
                break;

            case is_int:
                {
                    int_literal_type result = value.i + rhs.value.i;
                    if ((rhs.value.i > 0L && value.i > result) ||
                        (rhs.value.i < 0L && value.i < result))
                    {
                        valid = error_integer_overflow;
                    }
                    else {
                        value.i = result;
                    }
                }
                break;

            case is_uint:
                {
                    uint_literal_type result = value.ui + rhs.value.ui;
                    if (result < value.ui) {
                        valid = error_integer_overflow;
                    }
                    else {
                        value.ui = result;
                        type = is_uint;
                    }
                }
                break;
            }
            break;

        case is_uint:
            {
                uint_literal_type result = value.ui + as_ulong(rhs);
                if (result < value.ui) {
                    valid = error_integer_overflow;
                }
                else {
                    value.ui = result;
                }
            }
            break;

        case is_bool:
            value.i = value.b + as_bool(rhs);
            type = is_int;
        }
        valid = (value_error)(valid | rhs.valid);
        return *this;
    }
    closure_value &operator-= (closure_value const &rhs)
    {
        switch (type) {
        case is_int:
            switch(rhs.type) {
            case is_bool:
                {
                    int_literal_type result = value.i - as_long(rhs);
                    if ((rhs.value.i > 0L && result > value.i) ||
                        (rhs.value.i < 0L && result < value.i))
                    {
                        valid = error_integer_overflow;
                    }
                    else {
                        value.i = result;
                    }
                }
                break;

            case is_int:
                {
                    int_literal_type result = value.i - rhs.value.i;
                    if ((rhs.value.i > 0L && result > value.i) ||
                        (rhs.value.i < 0L && result < value.i))
                    {
                        valid = error_integer_overflow;
                    }
                    else {
                        value.i = result;
                    }
                }
                break;

            case is_uint:
                {
                    uint_literal_type result = value.ui - rhs.value.ui;
                    if (result > value.ui) {
                        valid = error_integer_overflow;
                    }
                    else {
                        value.ui = result;
                        type = is_uint;
                    }
                }
                break;
            }
            break;

        case is_uint:
            switch(rhs.type) {
            case is_bool:
                {
                    uint_literal_type result = value.ui - as_ulong(rhs);
                    if (result > value.ui)
                    {
                        valid = error_integer_overflow;
                    }
                    else {
                        value.ui = result;
                    }
                }
                break;

            case is_int:
                {
                    uint_literal_type result = value.ui - rhs.value.i;
                    if ((rhs.value.i > 0L && result > value.ui) ||
                        (rhs.value.i < 0L && result < value.ui))
                    {
                        valid = error_integer_overflow;
                    }
                    else {
                        value.ui = result;
                    }
                }
                break;

            case is_uint:
                {
                    uint_literal_type result = value.ui - rhs.value.ui;
                    if (result > value.ui) {
                        valid = error_integer_overflow;
                    }
                    else {
                        value.ui = result;
                    }
                }
                break;
            }
            break;

        case is_bool:
            value.i = value.b - as_bool(rhs);
            type = is_int;
        }
        valid = (value_error)(valid | rhs.valid);
        return *this;
    }
    closure_value &operator*= (closure_value const &rhs)
    {
        switch (type) {
        case is_int:
            switch(rhs.type) {
            case is_bool:   value.i *= as_long(rhs); break;
            case is_int:
                {
                    int_literal_type result = value.i * rhs.value.i;
                    if (0 != value.i && 0 != rhs.value.i &&
                        (result / value.i != rhs.value.i ||
                         result / rhs.value.i != value.i)
                       )
                    {
                        valid = error_integer_overflow;
                    }
                    else {
                        value.i = result;
                    }
                }
                break;

            case is_uint:
                {
                    uint_literal_type result = value.ui * rhs.value.ui;
                    if (0 != value.ui && 0 != rhs.value.ui &&
                        (result / value.ui != rhs.value.ui ||
                         result / rhs.value.ui != value.ui)
                       )
                    {
                        valid = error_integer_overflow;
                    }
                    else {
                        value.ui = result;
                        type = is_uint;
                    }
                }
                break;
            }
            break;

        case is_uint:
            {
                uint_literal_type rhs_val = as_ulong(rhs);
                uint_literal_type result = value.ui * rhs_val;
                if (0 != value.ui && 0 != rhs_val &&
                    (result / value.ui != rhs_val ||
                      result / rhs_val != value.ui)
                    )
                {
                    valid = error_integer_overflow;
                }
                else {
                    value.ui = result;
                    type = is_uint;
                }
            }
            break;

        case is_bool:
            switch (rhs.type) {
            case is_int:
                value.i = (value.b ? 1 : 0) * rhs.value.i;
                type = is_int;
                break;

            case is_uint:
                value.ui = (value.b ? 1 : 0) * rhs.value.ui;
                type = is_uint;
                break;

            case is_bool:
                value.b = 0 != ((value.b ? 1 : 0) * (rhs.value.b ? 1 : 0));
                break;
            }
        }
        valid = (value_error)(valid | rhs.valid);
        return *this;
    }
    closure_value &operator/= (closure_value const &rhs)
    {
        switch (type) {
        case is_int:
            switch(rhs.type) {
            case is_bool:
            case is_int:
                if (as_long(rhs) != 0) {
                    if (value.i == -value.i && -1 == rhs.value.i) {
                        // LONG_MIN / -1 on two's complement
                        valid = error_integer_overflow;
                    }
                    else {
                        value.i /= as_long(rhs);
                    }
                }
                else {
                    valid = error_division_by_zero;   // division by zero
                }
                break;

            case is_uint:
                if (rhs.value.ui != 0) {
                    value.ui /= rhs.value.ui;
                    type = is_uint;
                }
                else {
                    valid = error_division_by_zero;      // division by zero
                }
                break;
            }
            break;

        case is_uint:
            if (as_ulong(rhs) != 0)
                value.ui /= as_ulong(rhs);
            else
                valid = error_division_by_zero;         // division by zero
            break;

        case is_bool:
            if (as_bool(rhs)) {
                switch(rhs.type) {
                case is_int:
                    value.i = (value.b ? 1 : 0) / rhs.value.i;
                    type = is_int;
                    break;

                case is_uint:
                    value.i = (value.b ? 1 : 0) / rhs.value.ui;
                    type = is_int;
                    break;

                case is_bool:
                    break;
                }
            }
            else {
                valid = error_division_by_zero;         // division by zero
            }
        }
        return *this;
    }
    closure_value &operator%= (closure_value const &rhs)
    {
        switch (type) {
        case is_int:
            switch(rhs.type) {
            case is_bool:
            case is_int:
                if (as_long(rhs) != 0) {
                    if (value.i == -value.i && -1 == rhs.value.i) {
                        // LONG_MIN % -1 on two's complement
                        valid = error_integer_overflow;
                    }
                    else {
                        value.i %= as_long(rhs);
                    }
                }
                else {
                    valid = error_division_by_zero;      // division by zero
                }
                break;

            case is_uint:
                if (rhs.value.ui != 0) {
                    value.ui %= rhs.value.ui;
                    type = is_uint;
                }
                else {
                    valid = error_division_by_zero;      // division by zero
                }
                break;
            }
            break;

        case is_uint:
            if (as_ulong(rhs) != 0)
                value.ui %= as_ulong(rhs);
            else
                valid = error_division_by_zero;      // division by zero
            break;

        case is_bool:
            if (as_bool(rhs)) {
                switch(rhs.type) {
                case is_int:
                    value.i = (value.b ? 1 : 0) % rhs.value.i;
                    type = is_int;
                    break;

                case is_uint:
                    value.i = (value.b ? 1 : 0) % rhs.value.ui;
                    type = is_int;
                    break;

                case is_bool:
                    break;
                }
            }
            else {
                valid = error_division_by_zero;      // division by zero
            }
        }
        return *this;
    }

    friend closure_value
    operator- (closure_value const &rhs)
    {
        switch (rhs.type) {
        case is_int:
            {
                int_literal_type value = as_long(rhs);
                if (value != 0 && value == -value)
                    return closure_value(-value, error_integer_overflow);
                return closure_value(-value, rhs.valid);
            }

        case is_bool:   return closure_value(-as_long(rhs), rhs.valid);
        case is_uint:   break;
        }

        int_literal_type value = as_ulong(rhs);
        if (value != 0 && value == -value)
            return closure_value(-value, error_integer_overflow);
        return closure_value(-value, rhs.valid);
    }
    friend closure_value
    operator~ (closure_value const &rhs)
    {
        return closure_value(~as_ulong(rhs), rhs.valid);
    }
    friend closure_value
    operator! (closure_value const &rhs)
    {
        switch (rhs.type) {
        case is_int:    return closure_value(!as_long(rhs), rhs.valid);
        case is_bool:   return closure_value(!as_bool(rhs), rhs.valid);
        case is_uint:   break;
        }
        return closure_value(!as_ulong(rhs), rhs.valid);
    }

    // comparison
    friend closure_value
    operator== (closure_value const &lhs, closure_value const &rhs)
    {
        bool cmp = false;
        switch (lhs.type) {
        case is_int:
            switch(rhs.type) {
            case is_bool:   cmp = as_bool(lhs) == rhs.value.b; break;
            case is_int:    cmp = lhs.value.i == rhs.value.i; break;
            case is_uint:   cmp = lhs.value.ui == rhs.value.ui; break;
            }
            break;

        case is_uint:   cmp = lhs.value.ui == as_ulong(rhs); break;
        case is_bool:   cmp = lhs.value.b == as_bool(rhs); break;
        }
        return closure_value(cmp, (value_error)(lhs.valid | rhs.valid));
    }
    friend closure_value
    operator!= (closure_value const &lhs, closure_value const &rhs)
    {
        return closure_value(!as_bool(lhs == rhs), (value_error)(lhs.valid | rhs.valid));
    }
    friend closure_value
    operator> (closure_value const &lhs, closure_value const &rhs)
    {
        bool cmp = false;
        switch (lhs.type) {
        case is_int:
            switch(rhs.type) {
            case is_bool:   cmp = lhs.value.i > as_long(rhs); break;
            case is_int:    cmp = lhs.value.i > rhs.value.i; break;
            case is_uint:   cmp = lhs.value.ui > rhs.value.ui; break;
            }
            break;

        case is_uint:   cmp = lhs.value.ui > as_ulong(rhs); break;
        case is_bool:   cmp = lhs.value.b > as_bool(rhs); break;
        }
        return closure_value(cmp, (value_error)(lhs.valid | rhs.valid));
    }
    friend closure_value
    operator< (closure_value const &lhs, closure_value const &rhs)
    {
        bool cmp = false;
        switch (lhs.type) {
        case is_int:
            switch(rhs.type) {
            case is_bool:   cmp = lhs.value.i < as_long(rhs); break;
            case is_int:    cmp = lhs.value.i < rhs.value.i; break;
            case is_uint:   cmp = lhs.value.ui < rhs.value.ui; break;
            }
            break;

        case is_uint:   cmp = lhs.value.ui < as_ulong(rhs); break;
        case is_bool:   cmp = as_bool(lhs) < as_bool(rhs); break;
        }
        return closure_value(cmp, (value_error)(lhs.valid | rhs.valid));
    }
    friend closure_value
    operator<= (closure_value const &lhs, closure_value const &rhs)
    {
        return closure_value(!as_bool(lhs > rhs), (value_error)(lhs.valid | rhs.valid));
    }
    friend closure_value
    operator>= (closure_value const &lhs, closure_value const &rhs)
    {
        return closure_value(!as_bool(lhs < rhs), (value_error)(lhs.valid | rhs.valid));
    }

    closure_value &
    operator<<= (closure_value const &rhs)
    {
        switch (type) {
        case is_bool:
        case is_int:
            switch (rhs.type) {
            case is_bool:
            case is_int:
                {
                int_literal_type shift_by = as_long(rhs);

                    if (shift_by > 64)
                        shift_by = 64;
                    else if (shift_by < -64)
                        shift_by = -64;
                    value.i <<= shift_by;
                }
                break;

            case is_uint:
                {
                uint_literal_type shift_by = as_ulong(rhs);

                    if (shift_by > 64)
                        shift_by = 64;
                    value.ui <<= shift_by;

                // Note: The usual arithmetic conversions are not performed on
                //       bit shift operations.
                }
                break;
            }
            break;

        case is_uint:
            switch (rhs.type) {
            case is_bool:
            case is_int:
                {
                int_literal_type shift_by = as_long(rhs);

                    if (shift_by > 64)
                        shift_by = 64;
                    else if (shift_by < -64)
                        shift_by = -64;
                    value.ui <<= shift_by;
                }
                break;

            case is_uint:
                {
                uint_literal_type shift_by = as_ulong(rhs);

                    if (shift_by > 64)
                        shift_by = 64;
                    value.ui <<= shift_by;
                }
                break;
            }
        }
        valid = (value_error)(valid | rhs.valid);
        return *this;
    }

    closure_value &
    operator>>= (closure_value const &rhs)
    {
        switch (type) {
        case is_bool:
        case is_int:
            switch (rhs.type) {
            case is_bool:
            case is_int:
                {
                int_literal_type shift_by = as_long(rhs);

                    if (shift_by > 64)
                        shift_by = 64;
                    else if (shift_by < -64)
                        shift_by = -64;
                    value.i >>= shift_by;
                }
                break;

            case is_uint:
                {
                uint_literal_type shift_by = as_ulong(rhs);

                    if (shift_by > 64)
                        shift_by = 64;
                    value.ui >>= shift_by;

                // Note: The usual arithmetic conversions are not performed on
                //       bit shift operations.
                }
                break;
            }
            break;

        case is_uint:
            switch (rhs.type) {
            case is_bool:
            case is_int:
                {
                int_literal_type shift_by = as_long(rhs);

                    if (shift_by > 64)
                        shift_by = 64;
                    else if (shift_by < -64)
                        shift_by = -64;
                    value.ui >>= shift_by;
                }
                break;

            case is_uint:
                {
                uint_literal_type shift_by = as_ulong(rhs);

                    if (shift_by > 64)
                        shift_by = 64;
                    value.ui >>= shift_by;
                }
                break;
            }
            break;
        }
        valid = (value_error)(valid | rhs.valid);
        return *this;
    }

    friend closure_value
    operator|| (closure_value const &lhs, closure_value const &rhs)
    {
        bool result = as_bool(lhs) || as_bool(rhs);
        return closure_value(result, (value_error)(lhs.valid | rhs.valid));
    }

    friend closure_value
    operator&& (closure_value const &lhs, closure_value const &rhs)
    {
        bool result = as_bool(lhs) && as_bool(rhs);
        return closure_value(result, (value_error)(lhs.valid | rhs.valid));
    }

    friend closure_value
    operator| (closure_value const &lhs, closure_value const &rhs)
    {
        uint_literal_type result = as_ulong(lhs) | as_ulong(rhs);
        return closure_value(result, (value_error)(lhs.valid | rhs.valid));
    }

    friend closure_value
    operator& (closure_value const &lhs, closure_value const &rhs)
    {
        uint_literal_type result = as_ulong(lhs) & as_ulong(rhs);
        return closure_value(result, (value_error)(lhs.valid | rhs.valid));
    }

    friend closure_value
    operator^ (closure_value const &lhs, closure_value const &rhs)
    {
        uint_literal_type result = as_ulong(lhs) ^ as_ulong(rhs);
        return closure_value(result, (value_error)(lhs.valid | rhs.valid));
    }

    // handle the ?: operator
    closure_value &
    handle_questionmark(closure_value const &cond, closure_value const &val2)
    {
        switch (type) {
        case is_int:
            switch (val2.type) {
            case is_bool: value.b = as_bool(cond) ? value.b : as_bool(val2); break;
            case is_int:  value.i = as_bool(cond) ? value.i : as_long(val2); break;
            case is_uint:
                value.ui = as_bool(cond) ? value.ui : as_ulong(val2);
                type = is_uint;   // changing type!
                break;
            }
            break;

        case is_uint:   value.ui = as_bool(cond) ? value.ui : as_ulong(val2); break;
        case is_bool:   value.b = as_bool(cond) ? value.b : as_bool(val2); break;
        }
        valid = as_bool(cond) ? valid : val2.valid;
        return *this;
    }

#if defined (BOOST_SPIRIT_DEBUG)
    friend std::ostream&
    operator<< (std::ostream &o, closure_value const &val)
    {
        switch (val.type) {
        case is_int:    o << "int(" << as_long(val) << ")"; break;
        case is_uint:   o << "unsigned int(" << as_ulong(val) << ")"; break;
        case is_bool:   o << "bool(" << as_bool(val) << ")"; break;
        }
        return o;
    }
#endif // defined(BOOST_SPIRIT_DEBUG)

private:
    value_type type;
    union {
        int_literal_type i;
        uint_literal_type ui;
        bool b;
    } value;
    value_error valid;
};

///////////////////////////////////////////////////////////////////////////////
}   // namespace closures
}   // namespace grammars
}   // namespace wave
}   // namespace boost

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !defined(BOOST_CPP_EXPRESSION_VALUE_HPP_452FE66D_8754_4107_AF1E_E42255A0C18A_INCLUDED)

#ifndef BOOST_NUMERIC_AUTOMATIC_HPP
#define BOOST_NUMERIC_AUTOMATIC_HPP

//  Copyright (c) 2012 Robert Ramey
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// policy which creates expanded results types designed
// to avoid overflows.

#include <limits>
#include <cstdint>     // (u)intmax_t,
#include <type_traits> // conditional
#include <boost/integer.hpp>

#include "safe_common.hpp"
#include "checked_result.hpp"
#include "checked_default.hpp"
#include "checked_integer.hpp"
#include "checked_result_operations.hpp"
#include "interval.hpp"
#include "utility.hpp"

namespace boost {
namespace safe_numerics {

struct automatic {
private:
    // the following returns the "true" type.  After calculating the new max and min
    // these return the minimum size type which can hold the expected result.
    struct defer_stored_signed_lazily {
        template<std::intmax_t Min, std::intmax_t Max>
        using type = utility::signed_stored_type<Min, Max>;
    };

    struct defer_stored_unsigned_lazily {
        template<std::uintmax_t Min, std::uintmax_t Max>
        using type = utility::unsigned_stored_type<Min, Max>;
    };

    template<typename T, T Min, T Max>
    struct result_type {
        using type = typename std::conditional<
            std::numeric_limits<T>::is_signed,
            defer_stored_signed_lazily,
            defer_stored_unsigned_lazily
        >::type::template type<Min, Max>;
    };

public:
    ///////////////////////////////////////////////////////////////////////
    template<typename T, typename U>
    struct addition_result {
        using temp_base_type = typename std::conditional<
            // if both arguments are unsigned
            ! std::numeric_limits<T>::is_signed
            && ! std::numeric_limits<U>::is_signed,
            // result is unsigned
            std::uintmax_t,
            // otherwise result is signed
            std::intmax_t
        >::type;

        using r_type = checked_result<temp_base_type>;
        using r_interval_type = interval<r_type>;

        constexpr static const r_interval_type t_interval{
            checked::cast<temp_base_type>(base_value(std::numeric_limits<T>::min())),
            checked::cast<temp_base_type>(base_value(std::numeric_limits<T>::max()))
        };

        constexpr static const r_interval_type u_interval{
            checked::cast<temp_base_type>(base_value(std::numeric_limits<U>::min())),
            checked::cast<temp_base_type>(base_value(std::numeric_limits<U>::max()))
        };

        constexpr static const r_interval_type r_interval = t_interval + u_interval;

        constexpr static auto rl = r_interval.l;
        constexpr static auto ru = r_interval.u;

        using type = typename result_type<
            temp_base_type,
            rl.exception()
                ? std::numeric_limits<temp_base_type>::min()
                : static_cast<temp_base_type>(rl),
            ru.exception()
                ? std::numeric_limits<temp_base_type>::max()
                : static_cast<temp_base_type>(ru)
        >::type;
    };

    ///////////////////////////////////////////////////////////////////////
    template<typename T, typename U>
    struct subtraction_result {
        // result of subtraction are always signed.
        using temp_base_type = intmax_t;

        using r_type = checked_result<temp_base_type>;
        using r_interval_type = interval<r_type>;

        constexpr static const r_interval_type t_interval{
            checked::cast<temp_base_type>(base_value(std::numeric_limits<T>::min())),
            checked::cast<temp_base_type>(base_value(std::numeric_limits<T>::max()))
        };

        constexpr static const r_interval_type u_interval{
            checked::cast<temp_base_type>(base_value(std::numeric_limits<U>::min())),
            checked::cast<temp_base_type>(base_value(std::numeric_limits<U>::max()))
        };

        constexpr static const r_interval_type r_interval = t_interval - u_interval;

        constexpr static auto rl = r_interval.l;
        constexpr static auto ru = r_interval.u;

        using type = typename result_type<
            temp_base_type,
            rl.exception()
                ? std::numeric_limits<temp_base_type>::min()
                : static_cast<temp_base_type>(rl),
            ru.exception()
                ? std::numeric_limits<temp_base_type>::max()
                : static_cast<temp_base_type>(ru)
        >::type;
    };

    ///////////////////////////////////////////////////////////////////////
    template<typename T, typename U>
    struct multiplication_result {
        using temp_base_type = typename std::conditional<
            // if both arguments are unsigned
            ! std::numeric_limits<T>::is_signed
            && ! std::numeric_limits<U>::is_signed,
            // result is unsigned
            std::uintmax_t,
            // otherwise result is signed
            std::intmax_t
        >::type;

        using r_type = checked_result<temp_base_type>;
        using r_interval_type = interval<r_type>;

        constexpr static const r_interval_type t_interval{
            checked::cast<temp_base_type>(base_value(std::numeric_limits<T>::min())),
            checked::cast<temp_base_type>(base_value(std::numeric_limits<T>::max()))
        };

        constexpr static const r_interval_type u_interval{
            checked::cast<temp_base_type>(base_value(std::numeric_limits<U>::min())),
            checked::cast<temp_base_type>(base_value(std::numeric_limits<U>::max()))
        };

        constexpr static const r_interval_type r_interval = t_interval * u_interval;

        constexpr static const auto rl = r_interval.l;
        constexpr static const auto ru = r_interval.u;

        using type = typename result_type<
            temp_base_type,
            rl.exception()
                ? std::numeric_limits<temp_base_type>::min()
                : static_cast<temp_base_type>(rl),
            ru.exception()
                ? std::numeric_limits<temp_base_type>::max()
                : static_cast<temp_base_type>(ru)
        >::type;
    };

    ///////////////////////////////////////////////////////////////////////
    template<typename T, typename U>
    struct division_result {
        using temp_base_type = typename std::conditional<
            // if both arguments are unsigned
            ! std::numeric_limits<T>::is_signed
            && ! std::numeric_limits<U>::is_signed,
            // result is unsigned
            std::uintmax_t,
            // otherwise result is signed
            std::intmax_t
        >::type;

        using r_type = checked_result<temp_base_type>;
        using r_interval_type = interval<r_type>;

        constexpr static const r_interval_type t_interval{
            checked::cast<temp_base_type>(base_value(std::numeric_limits<T>::min())),
            checked::cast<temp_base_type>(base_value(std::numeric_limits<T>::max()))
        };

        constexpr static const r_interval_type u_interval{
            checked::cast<temp_base_type>(base_value(std::numeric_limits<U>::min())),
            checked::cast<temp_base_type>(base_value(std::numeric_limits<U>::max()))
        };

        constexpr static r_interval_type rx(){
            if(u_interval.u < r_type(0)
            || u_interval.l > r_type(0))
                return t_interval / u_interval;
            return utility::minmax(
                std::initializer_list<r_type> {
                    t_interval.l / u_interval.l,
                    t_interval.l / r_type(-1),
                    t_interval.l / r_type(1),
                    t_interval.l / u_interval.u,
                    t_interval.u / u_interval.l,
                    t_interval.u / r_type(-1),
                    t_interval.u / r_type(1),
                    t_interval.u / u_interval.u,
                }
            );
        }

        constexpr static const r_interval_type r_interval = rx();

        constexpr static auto rl = r_interval.l;
        constexpr static auto ru = r_interval.u;

        using type = typename result_type<
            temp_base_type,
            rl.exception()
                ? std::numeric_limits<temp_base_type>::min()
                : static_cast<temp_base_type>(rl),
            ru.exception()
                ? std::numeric_limits<temp_base_type>::max()
                : static_cast<temp_base_type>(ru)
        >::type;
    };

    ///////////////////////////////////////////////////////////////////////
    template<typename T, typename U>
    struct modulus_result {
        using temp_base_type = typename std::conditional<
            // if both arguments are unsigned
            ! std::numeric_limits<T>::is_signed
            && ! std::numeric_limits<U>::is_signed,
            // result is unsigned
            std::uintmax_t,
            // otherwise result is signed
            std::intmax_t
        >::type;

        using r_type = checked_result<temp_base_type>;
        using r_interval_type = interval<r_type>;

        constexpr static const r_interval_type t_interval{
            checked::cast<temp_base_type>(base_value(std::numeric_limits<T>::min())),
            checked::cast<temp_base_type>(base_value(std::numeric_limits<T>::max()))
        };

        constexpr static const r_interval_type u_interval{
            checked::cast<temp_base_type>(base_value(std::numeric_limits<U>::min())),
            checked::cast<temp_base_type>(base_value(std::numeric_limits<U>::max()))
        };

        constexpr static r_interval_type rx(){
            if(u_interval.u < r_type(0)
            || u_interval.l > r_type(0))
                return t_interval / u_interval;
            return utility::minmax(
                std::initializer_list<r_type> {
                    t_interval.l % u_interval.l,
                    t_interval.l % r_type(-1),
                    t_interval.l % r_type(1),
                    t_interval.l % u_interval.u,
                    t_interval.u % u_interval.l,
                    t_interval.u % r_type(-1),
                    t_interval.u % r_type(1),
                    t_interval.u % u_interval.u,
                }
            );
        }

        constexpr static const r_interval_type r_interval = rx();

        constexpr static auto rl = r_interval.l;
        constexpr static auto ru = r_interval.u;

        using type = typename result_type<
            temp_base_type,
            rl.exception()
                ? std::numeric_limits<temp_base_type>::min()
                : static_cast<temp_base_type>(rl),
            ru.exception()
                ? std::numeric_limits<temp_base_type>::max()
                : static_cast<temp_base_type>(ru)
        >::type;
    };

    ///////////////////////////////////////////////////////////////////////
    // note: comparison_result (<, >, ...) is special.
    // The return value is always a bool.  The type returned here is
    // the intermediate type applied to make the values comparable.
    template<typename T, typename U>
    struct comparison_result {
        using temp_base_type = typename std::conditional<
            // if both arguments are unsigned
            ! std::numeric_limits<T>::is_signed
            && ! std::numeric_limits<U>::is_signed,
            // result is unsigned
            std::uintmax_t,
            // otherwise result is signed
            std::intmax_t
        >::type;

        using r_type = checked_result<temp_base_type>;
        using r_interval_type = interval<r_type>;

        constexpr static const r_interval_type t_interval{
            checked::cast<temp_base_type>(base_value(std::numeric_limits<T>::min())),
            checked::cast<temp_base_type>(base_value(std::numeric_limits<T>::max()))
        };

        constexpr static const r_interval_type u_interval{
            checked::cast<temp_base_type>(base_value(std::numeric_limits<U>::min())),
            checked::cast<temp_base_type>(base_value(std::numeric_limits<U>::max()))
        };

        // workaround some microsoft problem
        #if 0
        constexpr static r_type min(const r_type & t, const r_type & u){
            // assert(! u.exception());
            // assert(! t.exception());
            return static_cast<bool>(t < u) ? t : u;
        }

       constexpr static r_type max(const r_type & t, const r_type & u){
            // assert(! u.exception());
            // assert(! t.exception());
            return static_cast<bool>(t < u) ? u : t;
        }
        #endif

        // union of two intervals
        // note: we can't use t_interval | u_interval because it
        // depends on max and min which in turn depend on < which in turn
        // depends on implicit conversion of tribool to bool
        constexpr static r_interval_type union_interval(
            const r_interval_type & t,
            const r_interval_type & u
        ){
            //const r_type & rl = min(t.l, u.l);
            const r_type & rmin = static_cast<bool>(t.l < u.l) ? t.l : u.l;
            //const r_type & ru = max(t.u, u.u);
            const r_type & rmax = static_cast<bool>(t.u < u.u) ? u.u : t.u;
            return r_interval_type(rmin, rmax);
        }

        constexpr static const r_interval_type r_interval =
            union_interval(t_interval, u_interval);

        constexpr static auto rl = r_interval.l;
        constexpr static auto ru = r_interval.u;

        using type = typename result_type<
            temp_base_type,
            rl.exception()
                ? std::numeric_limits<temp_base_type>::min()
                : static_cast<temp_base_type>(rl),
            ru.exception()
                ? std::numeric_limits<temp_base_type>::max()
                : static_cast<temp_base_type>(ru)
        >::type;
    };

    ///////////////////////////////////////////////////////////////////////
    // shift operations
    template<typename T, typename U>
    struct left_shift_result {
        using temp_base_type = typename std::conditional<
            std::numeric_limits<T>::is_signed,
            std::intmax_t,
            std::uintmax_t
        >::type;

        using r_type = checked_result<temp_base_type>;
        using r_interval_type = interval<r_type>;

        constexpr static const r_interval_type t_interval{
            checked::cast<temp_base_type>(base_value(std::numeric_limits<T>::min())),
            checked::cast<temp_base_type>(base_value(std::numeric_limits<T>::max()))
        };

        constexpr static const r_interval_type u_interval{
            checked::cast<temp_base_type>(base_value(std::numeric_limits<U>::min())),
            checked::cast<temp_base_type>(base_value(std::numeric_limits<U>::max()))
        };

        constexpr static const r_interval_type r_interval =
            t_interval << u_interval;

        constexpr static auto rl = r_interval.l;
        constexpr static auto ru = r_interval.u;

        using type = typename result_type<
            temp_base_type,
            rl.exception()
                ? std::numeric_limits<temp_base_type>::min()
                : static_cast<temp_base_type>(rl),
            ru.exception()
                ? std::numeric_limits<temp_base_type>::max()
                : static_cast<temp_base_type>(ru)
        >::type;
    };

    ///////////////////////////////////////////////////////////////////////
    template<typename T, typename U>
    struct right_shift_result {
        using temp_base_type = typename std::conditional<
            std::numeric_limits<T>::is_signed,
            std::intmax_t,
            std::uintmax_t
        >::type;

        using r_type = checked_result<temp_base_type>;
        using r_interval_type = interval<r_type>;

        constexpr static const r_interval_type t_interval{
            checked::cast<temp_base_type>(base_value(std::numeric_limits<T>::min())),
            checked::cast<temp_base_type>(base_value(std::numeric_limits<T>::max()))
        };

        constexpr static const r_type u_min
            = checked::cast<temp_base_type>(base_value(std::numeric_limits<U>::min()));

        constexpr static const r_interval_type u_interval{
            u_min.exception()
            ? r_type(0)
            : u_min,
            checked::cast<temp_base_type>(base_value(std::numeric_limits<U>::max()))
        };

        constexpr static const r_interval_type r_interval = t_interval >> u_interval;

        constexpr static auto rl = r_interval.l;
        constexpr static auto ru = r_interval.u;

        using type = typename result_type<
            temp_base_type,
            rl.exception()
                ? std::numeric_limits<temp_base_type>::min()
                : static_cast<temp_base_type>(rl),
            ru.exception()
                ? std::numeric_limits<temp_base_type>::max()
                : static_cast<temp_base_type>(ru)
        >::type;

    };

    ///////////////////////////////////////////////////////////////////////
    template<typename T, typename U>
    struct bitwise_and_result {
        using type = decltype(
            typename base_type<T>::type()
            & typename base_type<U>::type()
        );
    };
    template<typename T, typename U>
    struct bitwise_or_result {
        using type = decltype(
            typename base_type<T>::type()
            | typename base_type<U>::type()
        );
    };
    template<typename T, typename U>
    struct bitwise_xor_result {
        using type = decltype(
            typename base_type<T>::type()
            ^ typename base_type<U>::type()
        );
    };
};

} // safe_numerics
} // boost

#endif // BOOST_NUMERIC_AUTOMATIC_HPP

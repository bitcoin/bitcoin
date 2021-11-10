#ifndef BOOST_SAFE_NUMERICS_INTERVAL_HPP
#define BOOST_SAFE_NUMERICS_INTERVAL_HPP

//  Copyright (c) 2012 Robert Ramey
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <limits>
#include <cassert>
#include <type_traits>
#include <initializer_list>
#include <algorithm> // minmax, min, max

#include <boost/logic/tribool.hpp>

#include "utility.hpp" // log

// from stack overflow
// http://stackoverflow.com/questions/23815138/implementing-variadic-min-max-functions

namespace boost {
namespace safe_numerics {

template<typename R>
struct interval {
    const R l;
    const R u;

    template<typename T>
    constexpr interval(const T & lower, const T & upper) :
        l(lower),
        u(upper)
    {
        // assert(static_cast<bool>(l <= u));
    }
    template<typename T>
    constexpr interval(const std::pair<T, T> & p) :
        l(p.first),
        u(p.second)
    {}
    template<class T>
    constexpr interval(const interval<T> & rhs) :
        l(rhs.l),
        u(rhs.u)
    {}
    constexpr interval() :
        l(std::numeric_limits<R>::min()),
        u(std::numeric_limits<R>::max())
    {}
    // return true if this interval contains the given point
    constexpr tribool includes(const R & t) const {
        return l <= t && t <= u;
    }
    // if this interval contains every point found in some other inteval t
    //  return true
    // otherwise
    //  return false or indeterminate
    constexpr tribool includes(const interval<R> & t) const {
        return u >= t.u && l <= t.l;
    }

    // return true if this interval contains the given point
    constexpr tribool excludes(const R & t) const {
        return t < l || t > u;
    }
    // if this interval excludes every point found in some other inteval t
    //  return true
    // otherwise
    //  return false or indeterminate
    constexpr tribool excludes(const interval<R> & t) const {
        return t.u < l || u < t.l;
    }

};

template<class R>
constexpr inline interval<R> make_interval(){
    return interval<R>();
}
template<class R>
constexpr  inline interval<R> make_interval(const R &){
    return interval<R>();
}

// account for the fact that for floats and doubles
// the most negative value is called "lowest" rather
// than min
template<>
constexpr inline interval<float>::interval() :
    l(std::numeric_limits<float>::lowest()),
    u(std::numeric_limits<float>::max())
{}
template<>
constexpr inline interval<double>::interval() :
    l(std::numeric_limits<double>::lowest()),
    u(std::numeric_limits<double>::max())
{}

template<typename T>
constexpr inline interval<T> operator+(const interval<T> & t, const interval<T> & u){
    // adapted from https://en.wikipedia.org/wiki/Interval_arithmetic
    return {t.l + u.l, t.u + u.u};
}

template<typename T>
constexpr inline interval<T> operator-(const interval<T> & t, const interval<T> & u){
    // adapted from https://en.wikipedia.org/wiki/Interval_arithmetic
    return {t.l - u.u, t.u - u.l};
}

template<typename T>
constexpr inline interval<T> operator*(const interval<T> & t, const interval<T> & u){
    // adapted from https://en.wikipedia.org/wiki/Interval_arithmetic
    return utility::minmax<T>(
        std::initializer_list<T> {
            t.l * u.l,
            t.l * u.u,
            t.u * u.l,
            t.u * u.u
        }
    );
}

// interval division
// note: presumes 0 is not included in the range of the denominator
template<typename T>
constexpr inline interval<T> operator/(const interval<T> & t, const interval<T> & u){
    assert(static_cast<bool>(u.excludes(T(0))));
    return utility::minmax<T>(
        std::initializer_list<T> {
            t.l / u.l,
            t.l / u.u,
            t.u / u.l,
            t.u / u.u
        }
    );
}

// modulus of two intervals.  This will give a new range of for the modulus.
// note: presumes 0 is not included in the range of the denominator
template<typename T>
constexpr inline interval<T> operator%(const interval<T> & t, const interval<T> & u){
    assert(static_cast<bool>(u.excludes(T(0))));
    return utility::minmax<T>(
        std::initializer_list<T> {
            t.l % u.l,
            t.l % u.u,
            t.u % u.l,
            t.u % u.u
        }
    );
}

template<typename T>
constexpr inline interval<T> operator<<(const interval<T> & t, const interval<T> & u){
    //return interval<T>{t.l << u.l, t.u << u.u};
    return utility::minmax<T>(
        std::initializer_list<T> {
            t.l << u.l,
            t.l << u.u,
            t.u << u.l,
            t.u << u.u
        }
    );
}

template<typename T>
constexpr inline interval<T> operator>>(const interval<T> & t, const interval<T> & u){
    //return interval<T>{t.l >> u.u, t.u >> u.l};
    return utility::minmax<T>(
        std::initializer_list<T> {
            t.l >> u.l,
            t.l >> u.u,
            t.u >> u.l,
            t.u >> u.u
        }
    );
}

// union of two intervals
template<typename T>
constexpr interval<T> operator|(const interval<T> & t, const interval<T> & u){
    const T & rl = std::min(t.l, u.l);
    const T & ru = std::max(t.u, u.u);
    return interval<T>(rl, ru);
}

// intersection of two intervals
template<typename T>
constexpr inline interval<T> operator&(const interval<T> & t, const interval<T> & u){
    const T & rl = std::max(t.l, u.l);
    const T & ru = std::min(t.u, u.u);
    return interval<T>(rl, ru);
}

// determine whether two intervals intersect
template<typename T>
constexpr inline boost::logic::tribool intersect(const interval<T> & t, const interval<T> & u){
    return t.u >= u.l || t.l <= u.u;
}

template<typename T>
constexpr inline boost::logic::tribool operator<(
    const interval<T> & t,
    const interval<T> & u
){
    return
        // if every element in t is less than every element in u
        t.u < u.l ? boost::logic::tribool(true):
        // if every element in t is greater than every element in u
        t.l > u.u ? boost::logic::tribool(false):
        // otherwise some element(s) in t are greater than some element in u
        boost::logic::indeterminate
    ;
}

template<typename T>
constexpr inline boost::logic::tribool operator>(
    const interval<T> & t,
    const interval<T> & u
){
    return
        // if every element in t is greater than every element in u
        t.l > u.u ? boost::logic::tribool(true) :
        // if every element in t is less than every element in u
        t.u < u.l ? boost::logic::tribool(false) :
        // otherwise some element(s) in t are greater than some element in u
        boost::logic::indeterminate
    ;
}

template<typename T>
constexpr inline bool operator==(
    const interval<T> & t,
    const interval<T> & u
){
    // intervals have the same limits
    return t.l == u.l && t.u == u.u;
}

template<typename T>
constexpr inline bool operator!=(
    const interval<T> & t,
    const interval<T> & u
){
    return ! (t == u);
}

template<typename T>
constexpr inline boost::logic::tribool operator<=(
    const interval<T> & t,
    const interval<T> & u
){
    return ! (t > u);
}

template<typename T>
constexpr inline boost::logic::tribool operator>=(
    const interval<T> & t,
    const interval<T> & u
){
    return ! (t < u);
}

} // safe_numerics
} // boost

#include <iosfwd>

namespace std {

template<typename CharT, typename Traits, typename T>
inline std::basic_ostream<CharT, Traits> &
operator<<(
    std::basic_ostream<CharT, Traits> & os,
    const boost::safe_numerics::interval<T> & i
){
    return os << '[' << i.l << ',' << i.u << ']';
}
template<typename CharT, typename Traits>
inline std::basic_ostream<CharT, Traits> &
operator<<(
    std::basic_ostream<CharT, Traits> & os,
    const boost::safe_numerics::interval<unsigned char> & i
){
    os << "[" << (unsigned)i.l << "," << (unsigned)i.u << "]";
    return os;
}

template<typename CharT, typename Traits>
inline std::basic_ostream<CharT, Traits> &
operator<<(
    std::basic_ostream<CharT, Traits> & os,
    const boost::safe_numerics::interval<signed char> & i
){
    os << "[" << (int)i.l << "," << (int)i.u << "]";
    return os;
}

} // std

#endif // BOOST_SAFE_NUMERICS_INTERVAL_HPP

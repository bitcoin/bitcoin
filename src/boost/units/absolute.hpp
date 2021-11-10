// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

///
/// \file
/// \brief Absolute units (points rather than vectors).
/// \details Operations between absolute units, and relative units like temperature differences.
///

#ifndef BOOST_UNITS_ABSOLUTE_HPP
#define BOOST_UNITS_ABSOLUTE_HPP

#include <iosfwd>

#include <boost/units/detail/absolute_impl.hpp>

namespace boost {

namespace units {

/// A wrapper to represent absolute units (points rather than vectors).  Intended
/// originally for temperatures, this class implements operators for absolute units 
/// so that addition of a relative unit to an absolute unit results in another
/// absolute unit : absolute<T> +/- T -> absolute<T> and subtraction of one absolute
/// unit from another results in a relative unit : absolute<T> - absolute<T> -> T.
template<class Y>
class absolute
{
    public:
        typedef absolute<Y>     this_type;
        typedef Y               value_type;
        
        BOOST_CONSTEXPR absolute() : val_() { }
        BOOST_CONSTEXPR absolute(const value_type& val) : val_(val) { }
        BOOST_CONSTEXPR absolute(const this_type& source) : val_(source.val_) { }
   
        BOOST_CXX14_CONSTEXPR this_type& operator=(const this_type& source)           { val_ = source.val_; return *this; }
        
        BOOST_CONSTEXPR const value_type& value() const                         { return val_; }
        
        BOOST_CXX14_CONSTEXPR const this_type& operator+=(const value_type& val)      { val_ += val; return *this; }
        BOOST_CXX14_CONSTEXPR const this_type& operator-=(const value_type& val)      { val_ -= val; return *this; }
        
    private:
        value_type   val_;
};

/// add a relative value to an absolute one
template<class Y>
BOOST_CONSTEXPR absolute<Y> operator+(const absolute<Y>& aval,const Y& rval)
{
    return absolute<Y>(aval.value()+rval);
}

/// add a relative value to an absolute one
template<class Y>
BOOST_CONSTEXPR absolute<Y> operator+(const Y& rval,const absolute<Y>& aval)
{
    return absolute<Y>(aval.value()+rval);
}

/// subtract a relative value from an absolute one
template<class Y>
BOOST_CONSTEXPR absolute<Y> operator-(const absolute<Y>& aval,const Y& rval)
{
    return absolute<Y>(aval.value()-rval);
}

/// subtracting two absolutes gives a difference
template<class Y>
BOOST_CONSTEXPR Y operator-(const absolute<Y>& aval1,const absolute<Y>& aval2)
{
    return Y(aval1.value()-aval2.value());
}

/// creates a quantity from an absolute unit and a raw value
template<class D, class S, class T>
BOOST_CONSTEXPR quantity<absolute<unit<D, S> >, T> operator*(const T& t, const absolute<unit<D, S> >&)
{
    return(quantity<absolute<unit<D, S> >, T>::from_value(t));
}

/// creates a quantity from an absolute unit and a raw value
template<class D, class S, class T>
BOOST_CONSTEXPR quantity<absolute<unit<D, S> >, T> operator*(const absolute<unit<D, S> >&, const T& t)
{
    return(quantity<absolute<unit<D, S> >, T>::from_value(t));
}

/// Print an absolute unit
template<class Char, class Traits, class Y>
std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& os,const absolute<Y>& aval)
{

    os << "absolute " << aval.value();
    
    return os;
}

} // namespace units

} // namespace boost

#if BOOST_UNITS_HAS_BOOST_TYPEOF

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TEMPLATE(boost::units::absolute, (class))

#endif

namespace boost {

namespace units {

/// Macro to define the offset between two absolute units.
/// Requires the value to be in the destination units e.g
/// @code
/// BOOST_UNITS_DEFINE_CONVERSION_OFFSET(celsius_base_unit, fahrenheit_base_unit, double, 32.0);
/// @endcode
/// @c BOOST_UNITS_DEFINE_CONVERSION_FACTOR is also necessary to
/// specify the conversion factor.  Like @c BOOST_UNITS_DEFINE_CONVERSION_FACTOR
/// this macro defines both forward and reverse conversions so 
/// defining, e.g., the conversion from celsius to fahrenheit as above will also
/// define the inverse conversion from fahrenheit to celsius.
#define BOOST_UNITS_DEFINE_CONVERSION_OFFSET(From, To, type_, value_)   \
    namespace boost {                                                   \
    namespace units {                                                   \
    template<>                                                          \
    struct affine_conversion_helper<                                    \
        reduce_unit<From::unit_type>::type,                             \
        reduce_unit<To::unit_type>::type>                               \
    {                                                                   \
        BOOST_STATIC_CONSTEXPR bool is_defined = true;                  \
        typedef type_ type;                                             \
        static BOOST_CONSTEXPR type value() { return(value_); }         \
    };                                                                  \
    }                                                                   \
    }                                                                   \
    void boost_units_require_semicolon()

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_ABSOLUTE_HPP

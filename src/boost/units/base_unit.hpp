// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file
/// \brief base unit (meter, kg, sec...).
/// \details base unit definition registration.

#ifndef BOOST_UNITS_BASE_UNIT_HPP
#define BOOST_UNITS_BASE_UNIT_HPP

#include <boost/units/config.hpp>
#include <boost/units/heterogeneous_system.hpp>
#include <boost/units/static_rational.hpp>
#include <boost/units/units_fwd.hpp>
#include <boost/units/unit.hpp>
#include <boost/units/detail/dimension_list.hpp>
#include <boost/units/detail/ordinal.hpp>
#include <boost/units/detail/prevent_redefinition.hpp>

namespace boost {

namespace units {

/// This must be in namespace boost::units so that ADL
/// will work with friend functions defined inline.
/// Base dimensions and base units are independent.
/// INTERNAL ONLY
template<long N> struct base_unit_ordinal { };

/// INTERNAL ONLY
template<class T, long N> struct base_unit_pair { };

/// INTERNAL ONLY
template<class T, long N>
struct check_base_unit {
    enum {
        value =
            sizeof(boost_units_unit_is_registered(units::base_unit_ordinal<N>())) == sizeof(detail::yes) &&
            sizeof(boost_units_unit_is_registered(units::base_unit_pair<T, N>())) != sizeof(detail::yes)
    };
};

/// Defines a base unit.  To define a unit you need to provide
/// the derived class (CRTP), a dimension list and a unique integer.
/// @code
/// struct my_unit : boost::units::base_unit<my_unit, length_dimension, 1> {};
/// @endcode
/// It is designed so that you will get an error message if you try
/// to use the same value in multiple definitions.
template<class Derived,
         class Dim,
         long N
#if !defined(BOOST_UNITS_DOXYGEN) && !defined(BOOST_BORLANDC)
         ,
         class = typename detail::ordinal_has_already_been_defined<
             check_base_unit<Derived, N>::value
         >::type
#endif
>
class base_unit : 
    public ordinal<N> 
{
    public:
        /// INTERNAL ONLY
        typedef void boost_units_is_base_unit_type;
        /// INTERNAL ONLY
        typedef base_unit           this_type;
        /// The dimensions of this base unit.
        typedef Dim                 dimension_type;

        /// Provided for mpl compatability.
        typedef Derived type;

        /// The unit corresponding to this base unit.
#ifndef BOOST_UNITS_DOXYGEN
        typedef unit<
            Dim,
            heterogeneous_system<
                heterogeneous_system_impl<
                    list<
                        heterogeneous_system_dim<Derived,static_rational<1> >,
                        dimensionless_type
                    >,
                    Dim,
                    no_scale
                >
            >
        > unit_type;
#else
        typedef detail::unspecified unit_type;
#endif

    private:
        /// Check for C++0x.  In C++0x, we have to have identical
        /// arguments but a different return type to trigger an
        /// error.  Note that this is only needed for clang as
        /// check_base_unit will trigger an error earlier
        /// for compilers with less strict name lookup.
        /// INTERNAL ONLY
        friend BOOST_CONSTEXPR Derived* 
        check_double_register(const units::base_unit_ordinal<N>&) 
        { return(0); }

        /// Register this ordinal
        /// INTERNAL ONLY
        friend BOOST_CONSTEXPR detail::yes 
        boost_units_unit_is_registered(const units::base_unit_ordinal<N>&) 
        { return(detail::yes()); }
        
        /// But make sure we can identify the current instantiation!
        /// INTERNAL ONLY
        friend BOOST_CONSTEXPR detail::yes 
        boost_units_unit_is_registered(const units::base_unit_pair<Derived, N>&) 
        { return(detail::yes()); }
};

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_BASE_UNIT_HPP

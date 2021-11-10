// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_CONVERSION_HPP
#define BOOST_UNITS_CONVERSION_HPP

/// \file
/// \brief Template for defining conversions between quantities.

#include <boost/units/detail/conversion_impl.hpp>

namespace boost {

namespace units {

template<class From, class To>
struct conversion_helper;

#ifdef BOOST_UNITS_DOXYGEN

/// Template for defining conversions between
/// quantities.  This template should be specialized
/// for every quantity that allows conversions.
/// For example, if you have a two units
/// called pair and dozen you would write
/// @code
/// namespace boost {
/// namespace units {
/// template<class T0, class T1>
/// struct conversion_helper<quantity<dozen, T0>, quantity<pair, T1> >
/// {
///     static quantity<pair, T1> convert(const quantity<dozen, T0>& source)
///     {
///         return(quantity<pair, T1>::from_value(6 * source.value()));
///     }
/// };
/// }
/// }
/// @endcode
///
/// In most cases, the predefined specializations for @c unit
/// and @c absolute should be sufficient, so users should rarely
/// need to use this.
template<class From, class To>
struct conversion_helper
{
    static BOOST_CONSTEXPR To convert(const From&);
};

#endif

/// Defines the conversion factor from a base unit to any unit
/// or to another base unit with the correct dimensions.  Uses
/// of this macro must appear at global scope.
/// If the destination unit is a base unit or a unit that contains
/// only one base unit which is raised to the first power (e.g. feet->meters)
/// the reverse (meters->feet in this example) need not be defined explicitly.
#define BOOST_UNITS_DEFINE_CONVERSION_FACTOR(Source, Destination, type_, value_)    \
    namespace boost {                                                       \
    namespace units {                                                       \
    template<>                                                              \
    struct select_base_unit_converter<                                      \
        unscale<Source>::type,                                              \
        unscale<reduce_unit<Destination::unit_type>::type>::type            \
    >                                                                       \
    {                                                                       \
        typedef Source source_type;                                         \
        typedef reduce_unit<Destination::unit_type>::type destination_type; \
    };                                                                      \
    template<>                                                              \
    struct base_unit_converter<Source, reduce_unit<Destination::unit_type>::type>   \
    {                                                                       \
        BOOST_STATIC_CONSTEXPR bool is_defined = true;                      \
        typedef type_ type;                                                 \
        static BOOST_CONSTEXPR type value() { return(value_); }             \
    };                                                                      \
    }                                                                       \
    }                                                                       \
    void boost_units_require_semicolon()

/// Defines the conversion factor from a base unit to any other base
/// unit with the same dimensions.  Params should be a Boost.Preprocessor
/// Seq of template parameters, such as (class T1)(class T2)
/// All uses of must appear at global scope. The reverse conversion will
/// be defined automatically.  This macro is a little dangerous, because,
/// unlike the non-template form, it will silently fail if either base
/// unit is scaled.  This is probably not an issue if both the source
/// and destination types depend on the template parameters, but be aware
/// that a generic conversion to kilograms is not going to work.
#define BOOST_UNITS_DEFINE_CONVERSION_FACTOR_TEMPLATE(Params, Source, Destination, type_, value_)   \
    namespace boost {                                                       \
    namespace units {                                                       \
    template<BOOST_PP_SEQ_ENUM(Params)>                                     \
    struct base_unit_converter<                                             \
        Source,                                                             \
        BOOST_UNITS_MAKE_HETEROGENEOUS_UNIT(Destination, typename Source::dimension_type)\
    >                                                                       \
    {                                                                       \
        BOOST_STATIC_CONSTEXPR bool is_defined = true;                      \
        typedef type_ type;                                                 \
        static BOOST_CONSTEXPR type value() { return(value_); }             \
    };                                                                      \
    }                                                                       \
    }                                                                       \
    void boost_units_require_semicolon()

/// Specifies the default conversion to be applied when
/// no direct conversion is available.
/// Source is a base unit.  Dest is any unit with the
/// same dimensions.
#define BOOST_UNITS_DEFAULT_CONVERSION(Source, Dest)                \
    namespace boost {                                               \
    namespace units {                                               \
    template<>                                                      \
    struct unscaled_get_default_conversion<unscale<Source>::type>   \
    {                                                               \
        BOOST_STATIC_CONSTEXPR bool is_defined = true;              \
        typedef Dest::unit_type type;                               \
    };                                                              \
    }                                                               \
    }                                                               \
    void boost_units_require_semicolon()

/// Specifies the default conversion to be applied when
/// no direct conversion is available.
/// Params is a PP Sequence of template arguments.
/// Source is a base unit.  Dest is any unit with the
/// same dimensions.  The source must not be a scaled
/// base unit.
#define BOOST_UNITS_DEFAULT_CONVERSION_TEMPLATE(Params, Source, Dest)   \
    namespace boost {                                                   \
    namespace units {                                                   \
    template<BOOST_PP_SEQ_ENUM(Params)>                                 \
    struct unscaled_get_default_conversion<Source>                      \
    {                                                                   \
        BOOST_STATIC_CONSTEXPR bool is_defined = true;                  \
        typedef typename Dest::unit_type type;                          \
    };                                                                  \
    }                                                                   \
    }                                                                   \
    void boost_units_require_semicolon()

/// INTERNAL ONLY
/// Users should not create their units in namespace boost::units.
/// If we want to make this public it needs to allow better control over
/// the namespaces. --SJW.
/// template that defines a base_unit and conversion to another dimensionally-consistent unit
#define BOOST_UNITS_DEFINE_BASE_UNIT_WITH_CONVERSIONS(namespace_, name_, name_string_, symbol_string_, factor, unit, id)\
namespace boost {                                                           \
namespace units {                                                           \
namespace namespace_ {                                                      \
struct name_ ## _base_unit                                                  \
  : base_unit<name_ ## _base_unit, unit::dimension_type, id> {              \
    static BOOST_CONSTEXPR const char* name() { return(name_string_); }     \
    static BOOST_CONSTEXPR const char* symbol() { return(symbol_string_); } \
};                                                                          \
}                                                                           \
}                                                                           \
}                                                                           \
BOOST_UNITS_DEFINE_CONVERSION_FACTOR(namespace_::name_ ## _base_unit, unit, double, factor); \
BOOST_UNITS_DEFAULT_CONVERSION(namespace_::name_ ## _base_unit, unit)

/// Find the conversion factor between two units.
template<class FromUnit,class ToUnit>
inline
BOOST_CONSTEXPR
typename one_to_double_type<
    typename detail::conversion_factor_helper<FromUnit, ToUnit>::type
>::type
conversion_factor(const FromUnit&,const ToUnit&)
{
    return(one_to_double(detail::conversion_factor_helper<FromUnit, ToUnit>::value()));
}

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_CONVERSION_HPP

// Boost.Units - A C++ library for zero-overhead dimensional analysis and
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2010 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_IO_HPP
#define BOOST_UNITS_IO_HPP

/// \file
/// \brief Stream input and output for rationals, units and quantities.
/// \details Functions and manipulators for output and input of units and quantities.
///   symbol and name format, and engineering and binary autoprefix.
///   Serialization output is also supported.

#include <cassert>
#include <cmath>
#include <string>
#include <iosfwd>
#include <ios>
#include <sstream>

#include <boost/assert.hpp>
#include <boost/core/nvp.hpp>

#include <boost/units/units_fwd.hpp>
#include <boost/units/heterogeneous_system.hpp>
#include <boost/units/make_scaled_unit.hpp>
#include <boost/units/quantity.hpp>
#include <boost/units/scale.hpp>
#include <boost/units/static_rational.hpp>
#include <boost/units/unit.hpp>
#include <boost/units/detail/utility.hpp>

namespace boost {

namespace serialization {

/// Boost Serialization library support for units.
template<class Archive,class System,class Dim>
inline void serialize(Archive& /*ar*/,boost::units::unit<Dim,System>&,const unsigned int /*version*/)
{ }

/// Boost Serialization library support for quantities.
template<class Archive,class Unit,class Y>
inline void serialize(Archive& ar,boost::units::quantity<Unit,Y>& q,const unsigned int /*version*/)
{
    ar & boost::serialization::make_nvp("value", units::quantity_cast<Y&>(q));
}

} // namespace serialization

namespace units {

// get string representation of arbitrary type.
template<class T> std::string to_string(const T& t)
{
    std::stringstream sstr;

    sstr << t;

    return sstr.str();
}

/// get string representation of integral-valued @c static_rational.
template<integer_type N> std::string to_string(const static_rational<N>&)
{
    return to_string(N);
}

/// get string representation of @c static_rational.
template<integer_type N, integer_type D> std::string to_string(const static_rational<N,D>&)
{
    return '(' + to_string(N) + '/' + to_string(D) + ')';
}

/// Write @c static_rational to @c std::basic_ostream.
template<class Char, class Traits, integer_type N, integer_type D>
inline std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& os,const static_rational<N,D>& r)
{
    os << to_string(r);
    return os;
}

/// traits template for unit names.
template<class BaseUnit>
struct base_unit_info
{
    /// INTERNAL ONLY
    typedef void base_unit_info_primary_template;
    /// The full name of the unit (returns BaseUnit::name() by default)
    static std::string name()
    {
        return(BaseUnit::name());
    }
    /// The symbol for the base unit (Returns BaseUnit::symbol() by default)
    static std::string symbol()
    {
        return(BaseUnit::symbol());  ///  \returns BaseUnit::symbol(), for example "m"
    }
};

/// \enum format_mode format of output of units, for example "m" or "meter".
enum format_mode
{
    symbol_fmt = 0,     /// default - reduces unit names to known symbols for both base and derived units.
    name_fmt = 1,           /// output full unit names for base and derived units, for example "meter".
    raw_fmt = 2,            /// output only symbols for base units (but not derived units), for example "m".
    typename_fmt = 3,       /// output demangled typenames (useful only for diagnosis).
    fmt_mask = 3 /// Bits used for format.
};

/// \enum autoprefix_mode automatic scaling and prefix (controlled by value of quantity) a, if any,
enum autoprefix_mode
{
    autoprefix_none = 0, /// No automatic prefix.
    autoprefix_engineering = 4, /// Scale and prefix with 10^3 multiples, 1234.5 m output as 1.2345 km.
    autoprefix_binary = 8, /// Scale and prefix with 2^10 (1024) multiples, 1024 as 1 kb.
    autoprefix_mask = 12 ///  Bits used for autoprefix.
};

namespace detail {

template<bool>
struct xalloc_key_holder
{
    static int value;
    static bool initialized;
};

template<bool b>
int xalloc_key_holder<b>::value = 0;

template<bool b>
bool xalloc_key_holder<b>::initialized = 0;

struct xalloc_key_initializer_t
{
    xalloc_key_initializer_t()
    {
        if (!xalloc_key_holder<true>::initialized)
        {
            xalloc_key_holder<true>::value = std::ios_base::xalloc();
            xalloc_key_holder<true>::initialized = true;
        }
    }
};

namespace /**/ {

xalloc_key_initializer_t xalloc_key_initializer;

} // namespace

} // namespace detail

/// returns flags controlling output.
inline long get_flags(std::ios_base& ios, long mask)
{
    return(ios.iword(detail::xalloc_key_holder<true>::value) & mask);
}

/// Set new flags controlling output format.
inline void set_flags(std::ios_base& ios, long new_flags, long mask)
{
    BOOST_ASSERT((~mask & new_flags) == 0);
    long& flags = ios.iword(detail::xalloc_key_holder<true>::value);
    flags = (flags & ~mask) | new_flags;
}

/// returns flags controlling output format.
inline format_mode get_format(std::ios_base& ios)
{
    return(static_cast<format_mode>((get_flags)(ios, fmt_mask)));
}

/// Set new flags controlling output format.
inline void set_format(std::ios_base& ios, format_mode new_mode)
{
    (set_flags)(ios, new_mode, fmt_mask);
}

/// Set new flags for type_name output format.
inline std::ios_base& typename_format(std::ios_base& ios)
{
    (set_format)(ios, typename_fmt);
    return(ios);
}

/// set new flag for raw format output, for example "m".
inline std::ios_base& raw_format(std::ios_base& ios)
{
    (set_format)(ios, raw_fmt);
    return(ios);
}

/// set new format flag for symbol output, for example "m".
inline std::ios_base& symbol_format(std::ios_base& ios)
{
    (set_format)(ios, symbol_fmt);
    return(ios);
}

/// set new format for name output, for example "meter".
inline std::ios_base& name_format(std::ios_base& ios)
{
    (set_format)(ios, name_fmt);
    return(ios);
}

/// get autoprefix flags for output.
inline autoprefix_mode get_autoprefix(std::ios_base& ios)
{
    return static_cast<autoprefix_mode>((get_flags)(ios, autoprefix_mask));
}

/// Get format for output.
inline void set_autoprefix(std::ios_base& ios, autoprefix_mode new_mode)
{
    (set_flags)(ios, new_mode, autoprefix_mask);
}

/// Clear autoprefix flags.
inline std::ios_base& no_prefix(std::ios_base& ios)
{
    (set_autoprefix)(ios, autoprefix_none);
    return ios;
}

/// Set flag for engineering prefix, so 1234.5 m displays as "1.2345 km".
inline std::ios_base& engineering_prefix(std::ios_base& ios)
{
    (set_autoprefix)(ios, autoprefix_engineering);
    return ios;
}

/// Set flag for binary prefix, so 1024 byte displays as "1 Kib".
inline std::ios_base& binary_prefix(std::ios_base& ios)
{
    (set_autoprefix)(ios, autoprefix_binary);
    return ios;
}

namespace detail {

/// \return exponent string like "^1/2".
template<integer_type N, integer_type D>
inline std::string exponent_string(const static_rational<N,D>& r)
{
    return '^' + to_string(r);
}

/// \return empty exponent string for integer rational like 2.
template<>
inline std::string exponent_string(const static_rational<1>&)
{
    return "";
}

template<class T>
inline std::string base_unit_symbol_string(const T&)
{
    return base_unit_info<typename T::tag_type>::symbol() + exponent_string(typename T::value_type());
}

template<class T>
inline std::string base_unit_name_string(const T&)
{
    return base_unit_info<typename T::tag_type>::name() + exponent_string(typename T::value_type());
}

// stringify with symbols.
template<int N>
struct symbol_string_impl
{
    template<class Begin>
    struct apply
    {
        typedef typename symbol_string_impl<N-1>::template apply<typename Begin::next> next;
        static void value(std::string& str)
        {
            str += base_unit_symbol_string(typename Begin::item()) + ' ';
            next::value(str);
        }
    };
};

template<>
struct symbol_string_impl<1>
{
    template<class Begin>
    struct apply
    {
        static void value(std::string& str)
        {
            str += base_unit_symbol_string(typename Begin::item());
        }
    };
};

template<>
struct symbol_string_impl<0>
{
    template<class Begin>
    struct apply
    {
        static void value(std::string& str)
        {
            // better shorthand for dimensionless?
            str += "dimensionless";
        }
    };
};

template<int N>
struct scale_symbol_string_impl
{
    template<class Begin>
    struct apply
    {
        static void value(std::string& str)
        {
            str += Begin::item::symbol();
            scale_symbol_string_impl<N - 1>::template apply<typename Begin::next>::value(str);
        }
    };
};

template<>
struct scale_symbol_string_impl<0>
{
    template<class Begin>
    struct apply
    {
        static void value(std::string&) { }
    };
};

// stringify with names.
template<int N>
struct name_string_impl
{
    template<class Begin>
    struct apply
    {
        typedef typename name_string_impl<N-1>::template apply<typename Begin::next> next;
        static void value(std::string& str)
        {
            str += base_unit_name_string(typename Begin::item()) + ' ';
            next::value(str);
        }
    };
};

template<>
struct name_string_impl<1>
{
    template<class Begin>
    struct apply
    {
        static void value(std::string& str)
        {
            str += base_unit_name_string(typename Begin::item());
        }
    };
};

template<>
struct name_string_impl<0>
{
    template<class Begin>
    struct apply
    {
        static void value(std::string& str)
        {
            str += "dimensionless";
        }
    };
};

template<int N>
struct scale_name_string_impl
{
    template<class Begin>
    struct apply
    {
        static void value(std::string& str)
        {
            str += Begin::item::name();
            scale_name_string_impl<N - 1>::template apply<typename Begin::next>::value(str);
        }
    };
};

template<>
struct scale_name_string_impl<0>
{
    template<class Begin>
    struct apply
    {
        static void value(std::string&) { }
    };
};

} // namespace detail

namespace detail {

// These two overloads of symbol_string and name_string will
// will pick up homogeneous_systems.  They simply call the
// appropriate function with a heterogeneous_system.
template<class Dimension,class System, class SubFormatter>
inline std::string
to_string_impl(const unit<Dimension,System>&, SubFormatter f)
{
    return f(typename reduce_unit<unit<Dimension, System> >::type());
}

/// INTERNAL ONLY
// this overload picks up heterogeneous units that are not scaled.
template<class Dimension,class Units, class Subformatter>
inline std::string
to_string_impl(const unit<Dimension, heterogeneous_system<heterogeneous_system_impl<Units, Dimension, dimensionless_type> > >&, Subformatter f)
{
    std::string str;
    f.template append_units_to<Units>(str);
    return(str);
}

// This overload is a special case for heterogeneous_system which
// is really unitless
/// INTERNAL ONLY
template<class Subformatter>
inline std::string
to_string_impl(const unit<dimensionless_type, heterogeneous_system<heterogeneous_system_impl<dimensionless_type, dimensionless_type, dimensionless_type> > >&, Subformatter)
{
    return("dimensionless");
}

// this overload deals with heterogeneous_systems which are unitless
// but scaled.
/// INTERNAL ONLY
template<class Scale, class Subformatter>
inline std::string
to_string_impl(const unit<dimensionless_type, heterogeneous_system<heterogeneous_system_impl<dimensionless_type, dimensionless_type, Scale> > >&, Subformatter f)
{
    std::string str;
    f.template append_scale_to<Scale>(str);
    return(str);
}

// this overload deals with scaled units.
/// INTERNAL ONLY
template<class Dimension,class Units,class Scale, class Subformatter>
inline std::string
to_string_impl(const unit<Dimension, heterogeneous_system<heterogeneous_system_impl<Units, Dimension, Scale> > >&, Subformatter f)
{
    std::string str;

    f.template append_scale_to<Scale>(str);

    std::string without_scale = f(unit<Dimension, heterogeneous_system<heterogeneous_system_impl<Units, Dimension, dimensionless_type> > >());

    if (f.is_default_string(without_scale, unit<Dimension, heterogeneous_system<heterogeneous_system_impl<Units, Dimension, dimensionless_type> > >()))
    {
        str += "(";
        str += without_scale;
        str += ")";
    }
    else
    {
        str += without_scale;
    }

    return(str);
}

// This overload catches scaled units that have a single base unit
// raised to the first power.  It causes si::nano * si::meters to not
// put parentheses around the meters.  i.e. nm rather than n(m)
/// INTERNAL ONLY
template<class Dimension,class Unit,class Scale, class Subformatter>
inline std::string
to_string_impl(const unit<Dimension, heterogeneous_system<heterogeneous_system_impl<list<heterogeneous_system_dim<Unit, static_rational<1> >,dimensionless_type>, Dimension, Scale> > >&, Subformatter f)
{
    std::string str;

    f.template append_scale_to<Scale>(str);
    str += f(unit<Dimension, heterogeneous_system<heterogeneous_system_impl<list<heterogeneous_system_dim<Unit, static_rational<1> >, dimensionless_type>, Dimension, dimensionless_type> > >());

    return(str);
}

// This overload is necessary to disambiguate.
// it catches units that are unscaled and have a single
// base unit raised to the first power.  It is treated the
// same as any other unscaled unit.
/// INTERNAL ONLY
template<class Dimension,class Unit,class Subformatter>
inline std::string
to_string_impl(const unit<Dimension, heterogeneous_system<heterogeneous_system_impl<list<heterogeneous_system_dim<Unit, static_rational<1> >,dimensionless_type>, Dimension, dimensionless_type> > >&, Subformatter f)
{
    std::string str;
    f.template append_units_to<list<heterogeneous_system_dim<Unit, static_rational<1> >,dimensionless_type> >(str);
    return(str);
}

// This overload catches scaled units that have a single scaled base unit
// raised to the first power.  It moves that scaling on the base unit
// to the unit level scaling and recurses.  By doing this we make sure that
// si::milli * si::kilograms will print g rather than mkg.
//
// This transformation will not be applied if base_unit_info is specialized
// for the scaled base unit.
//
/// INTERNAL ONLY
template<class Dimension,class Unit,class UnitScale, class Scale, class Subformatter>
inline std::string
to_string_impl(
    const unit<
        Dimension,
        heterogeneous_system<
            heterogeneous_system_impl<
                list<heterogeneous_system_dim<scaled_base_unit<Unit, UnitScale>, static_rational<1> >, dimensionless_type>,
                Dimension,
                Scale
            >
        >
    >&,
    Subformatter f,
    typename base_unit_info<scaled_base_unit<Unit, UnitScale> >::base_unit_info_primary_template* = 0)
{
    return(f(
        unit<
            Dimension,
            heterogeneous_system<
                heterogeneous_system_impl<
                    list<heterogeneous_system_dim<Unit, static_rational<1> >, dimensionless_type>,
                    Dimension,
                    typename mpl::times<Scale, list<scale_list_dim<UnitScale>, dimensionless_type> >::type
                >
            >
        >()));
}

// this overload disambuguates between the overload for an unscaled unit
// and the overload for a scaled base unit raised to the first power.
/// INTERNAL ONLY
template<class Dimension,class Unit,class UnitScale,class Subformatter>
inline std::string
to_string_impl(
    const unit<
        Dimension,
        heterogeneous_system<
            heterogeneous_system_impl<
                list<heterogeneous_system_dim<scaled_base_unit<Unit, UnitScale>, static_rational<1> >, dimensionless_type>,
                Dimension,
                dimensionless_type
            >
        >
    >&,
    Subformatter f,
    typename base_unit_info<scaled_base_unit<Unit, UnitScale> >::base_unit_info_primary_template* = 0)
{
    std::string str;
    f.template append_units_to<list<heterogeneous_system_dim<scaled_base_unit<Unit, UnitScale>, static_rational<1> >, dimensionless_type> >(str);
    return(str);
}

struct format_raw_symbol_impl {
    template<class Units>
    void append_units_to(std::string& str) {
        detail::symbol_string_impl<Units::size::value>::template apply<Units>::value(str);
    }
    template<class Scale>
    void append_scale_to(std::string& str) {
        detail::scale_symbol_string_impl<Scale::size::value>::template apply<Scale>::value(str);
    }
    template<class Unit>
    std::string operator()(const Unit& u) {
        return(to_string_impl(u, *this));
    }
    template<class Unit>
    bool is_default_string(const std::string&, const Unit&) {
        return(true);
    }
};

struct format_symbol_impl : format_raw_symbol_impl {
    template<class Unit>
    std::string operator()(const Unit& u) {
        return(symbol_string(u));
    }
    template<class Unit>
    bool is_default_string(const std::string& str, const Unit& u) {
        return(str == to_string_impl(u, format_raw_symbol_impl()));
    }
};

struct format_raw_name_impl {
    template<class Units>
    void append_units_to(std::string& str) {
        detail::name_string_impl<(Units::size::value)>::template apply<Units>::value(str);
    }
    template<class Scale>
    void append_scale_to(std::string& str) {
        detail::scale_name_string_impl<Scale::size::value>::template apply<Scale>::value(str);
    }
    template<class Unit>
    std::string operator()(const Unit& u) {
        return(to_string_impl(u, *this));
    }
    template<class Unit>
    bool is_default_string(const std::string&, const Unit&) {
        return(true);
    }
};

struct format_name_impl : format_raw_name_impl {
    template<class Unit>
    std::string operator()(const Unit& u) {
        return(name_string(u));
    }
    template<class Unit>
    bool is_default_string(const std::string& str, const Unit& u) {
        return(str == to_string_impl(u, format_raw_name_impl()));
    }
};

template<class Char, class Traits>
inline void do_print(std::basic_ostream<Char, Traits>& os, const std::string& s)
{
    os << s.c_str();
}

inline void do_print(std::ostream& os, const std::string& s)
{
    os << s;
}

template<class Char, class Traits>
inline void do_print(std::basic_ostream<Char, Traits>& os, const char* s)
{
    os << s;
}

// For automatically applying the appropriate prefixes.

}

#ifdef BOOST_UNITS_DOXYGEN

/// ADL customization point for automatic prefixing.
/// Returns a non-negative value.  Implemented as std::abs
/// for built-in types.
template<class T>
double autoprefix_norm(const T& arg);

#else

template<class T, bool C = boost::is_arithmetic<T>::value>
struct autoprefix_norm_impl;

template<class T>
struct autoprefix_norm_impl<T, true>
{
    typedef double type;
    static BOOST_CONSTEXPR double call(const T& arg) { return std::abs(arg); }
};

template<class T>
struct autoprefix_norm_impl<T, false>
{
    typedef one type;
    static BOOST_CONSTEXPR one call(const T&) { return one(); }
};

template<class T>
BOOST_CONSTEXPR
typename autoprefix_norm_impl<T>::type autoprefix_norm(const T& arg)
{
    return autoprefix_norm_impl<T>::call(arg);
}

#endif

namespace detail {

template<class End, class Prev, class T, class F>
BOOST_CONSTEXPR
bool find_matching_scale_impl(End, End, Prev, T, double, F)
{
    return false;
}

template<class Begin, class End, class Prev, class T, class F>
BOOST_CXX14_CONSTEXPR
bool find_matching_scale_impl(Begin, End end, Prev prev, T t, double x, F f)
{
    if(Begin::item::value() > x) {
        f(prev, t);
        return true;
    } else {
        return detail::find_matching_scale_impl(
            typename Begin::next(),
            end,
            typename Begin::item(),
            t,
            x,
            f
        );
    }
}

template<class End, class T, class F>
BOOST_CONSTEXPR
bool find_matching_scale_i(End, End, T, double, F)
{
    return false;
}

template<class Begin, class End, class T, class F>
BOOST_CXX14_CONSTEXPR
bool find_matching_scale_i(Begin, End end, T t, double x, F f)
{
    if(Begin::item::value() > x) {
        return false;
    } else {
        return detail::find_matching_scale_impl(typename Begin::next(), end, typename Begin::item(), t, x, f);
    }
}

template<class Scales, class T, class F>
BOOST_CXX14_CONSTEXPR
bool find_matching_scale(T t, double x, F f)
{
    return detail::find_matching_scale_i(Scales(), dimensionless_type(), t, x, f);
}

typedef list<scale<10, static_rational<-24> >,
        list<scale<10, static_rational<-21> >,
        list<scale<10, static_rational<-18> >,
        list<scale<10, static_rational<-15> >,
        list<scale<10, static_rational<-12> >,
        list<scale<10, static_rational<-9> >,
        list<scale<10, static_rational<-6> >,
        list<scale<10, static_rational<-3> >,
        list<scale<10, static_rational<0> >,
        list<scale<10, static_rational<3> >,
        list<scale<10, static_rational<6> >,
        list<scale<10, static_rational<9> >,
        list<scale<10, static_rational<12> >,
        list<scale<10, static_rational<15> >,
        list<scale<10, static_rational<18> >,
        list<scale<10, static_rational<21> >,
        list<scale<10, static_rational<24> >,
        list<scale<10, static_rational<27> >,
        dimensionless_type> > > > > > > > > > > > > > > > > > engineering_prefixes;

typedef list<scale<2, static_rational<10> >,
        list<scale<2, static_rational<20> >,
        list<scale<2, static_rational<30> >,
        list<scale<2, static_rational<40> >,
        list<scale<2, static_rational<50> >,
        list<scale<2, static_rational<60> >,
        list<scale<2, static_rational<70> >,
        list<scale<2, static_rational<80> >,
        list<scale<2, static_rational<90> >,
        dimensionless_type> > > > > > > > > binary_prefixes;

template<class Os, class Quantity>
struct print_default_t {
    typedef void result_type;
    void operator()() const
    {
        *os << q->value() << ' ' << typename Quantity::unit_type();
    }
    Os* os;
    const Quantity* q;
};

template<class Os, class Quantity>
print_default_t<Os, Quantity> print_default(Os& os, const Quantity& q)
{
    print_default_t<Os, Quantity> result = { &os, &q };
    return result;
}

template<class Os>
struct print_scale_t {
    typedef void result_type;
    template<class Prefix, class T>
    void operator()(Prefix, const T& t) const
    {
        *prefixed = true;
        *os << t / Prefix::value() << ' ';
        switch(units::get_format(*os)) {
            case name_fmt: do_print(*os, Prefix::name()); break;
            case raw_fmt:
            case symbol_fmt: do_print(*os, Prefix::symbol()); break;
            case typename_fmt: do_print(*os, units::simplify_typename(Prefix())); *os << ' '; break;
        }
    }
    template<long N, class T>
    void operator()(scale<N, static_rational<0> >, const T& t) const
    {
        *prefixed = false;
        *os << t << ' ';
    }
    Os* os;
    bool* prefixed;
};

template<class Os>
print_scale_t<Os> print_scale(Os& os, bool& prefixed)
{
    print_scale_t<Os> result = { &os, &prefixed };
    return result;
}

// puts parentheses around a unit
/// INTERNAL ONLY
template<class Dimension,class Units,class Scale, class Subformatter>
inline std::string
maybe_parenthesize(const unit<Dimension, heterogeneous_system<heterogeneous_system_impl<Units, Dimension, Scale> > >&, Subformatter f)
{
    std::string str;

    std::string without_scale = f(unit<Dimension, heterogeneous_system<heterogeneous_system_impl<Units, Dimension, dimensionless_type> > >());

    if (f.is_default_string(without_scale, unit<Dimension, heterogeneous_system<heterogeneous_system_impl<Units, Dimension, dimensionless_type> > >()))
    {
        str += "(";
        str += without_scale;
        str += ")";
    }
    else
    {
        str += without_scale;
    }

    return(str);
}

// This overload catches scaled units that have a single base unit
// raised to the first power.  It causes si::nano * si::meters to not
// put parentheses around the meters.  i.e. nm rather than n(m)
/// INTERNAL ONLY
template<class Dimension,class Unit,class Scale, class Subformatter>
inline std::string
maybe_parenthesize(const unit<Dimension, heterogeneous_system<heterogeneous_system_impl<list<heterogeneous_system_dim<Unit, static_rational<1> >,dimensionless_type>, Dimension, Scale> > >&, Subformatter f)
{
    return f(unit<Dimension, heterogeneous_system<heterogeneous_system_impl<list<heterogeneous_system_dim<Unit, static_rational<1> >, dimensionless_type>, Dimension, dimensionless_type> > >());
}

template<class Prefixes, class CharT, class Traits, class Unit, class T, class F>
void do_print_prefixed_impl(std::basic_ostream<CharT, Traits>& os, const quantity<Unit, T>& q, F default_)
{
    bool prefixed;
    if(detail::find_matching_scale<Prefixes>(q.value(), autoprefix_norm(q.value()), detail::print_scale(os, prefixed))) {
        if(prefixed) {
            switch(units::get_format(os)) {
                case symbol_fmt: do_print(os, maybe_parenthesize(Unit(), format_symbol_impl())); break;
                case raw_fmt: do_print(os, maybe_parenthesize(Unit(), format_raw_symbol_impl())); break;
                case name_fmt: do_print(os, maybe_parenthesize(Unit(), format_name_impl())); break;
                case typename_fmt: do_print(os, simplify_typename(Unit())); break;
            }
        } else {
            os << Unit();
        }
    } else {
        default_();
    }
}

// Handle units like si::kilograms that have a scale embedded in the
// base unit.  This overload is disabled if the scaled base unit has
// a user-defined string representation.
template<class Prefixes, class CharT, class Traits, class Dimension, class BaseUnit, class BaseScale, class Scale, class T>
typename base_unit_info<
    scaled_base_unit<BaseUnit, Scale>
>::base_unit_info_primary_template
do_print_prefixed(
    std::basic_ostream<CharT, Traits>& os,
    const quantity<
        unit<
            Dimension,
            heterogeneous_system<
                heterogeneous_system_impl<
                    list<
                        heterogeneous_system_dim<
                            scaled_base_unit<BaseUnit, BaseScale>,
                            static_rational<1>
                        >,
                        dimensionless_type
                    >,
                    Dimension,
                    Scale
                >
            >
        >,
        T
    >& q)
{
    quantity<
        unit<
            Dimension,
            heterogeneous_system<
                heterogeneous_system_impl<
                    list<
                        heterogeneous_system_dim<BaseUnit, static_rational<1> >,
                        dimensionless_type
                    >,
                    Dimension,
                    dimensionless_type
                >
            >
        >,
        T
    > unscaled(q);
    detail::do_print_prefixed_impl<Prefixes>(os, unscaled, detail::print_default(os, q));
}

template<class Prefixes, class CharT, class Traits, class Dimension, class L, class Scale, class T>
void do_print_prefixed(
    std::basic_ostream<CharT, Traits>& os,
    const quantity<
        unit<
            Dimension,
            heterogeneous_system<
                heterogeneous_system_impl<
                    L,
                    Dimension,
                    Scale
                >
            >
        >,
        T
    >& q)
{
    quantity<
        unit<
            Dimension,
            heterogeneous_system<
                heterogeneous_system_impl<
                    L,
                    Dimension,
                    dimensionless_type
                >
            >
        >,
        T
    > unscaled(q);
    detail::do_print_prefixed_impl<Prefixes>(os, unscaled, detail::print_default(os, q));
}

template<class Prefixes, class CharT, class Traits, class Dimension, class System, class T>
void do_print_prefixed(std::basic_ostream<CharT, Traits>& os, const quantity<unit<Dimension, System>, T>& q)
{
    detail::do_print_prefixed<Prefixes>(os, quantity<unit<Dimension, typename make_heterogeneous_system<Dimension, System>::type>, T>(q));
}

template<class Prefixes, class CharT, class Traits, class Unit, class T>
void do_print_prefixed(std::basic_ostream<CharT, Traits>& os, const quantity<Unit, T>& q)
{
    detail::print_default(os, q)();
}

template<class Prefixes, class CharT, class Traits, class Unit, class T>
void maybe_print_prefixed(std::basic_ostream<CharT, Traits>& os, const quantity<Unit, T>& q, mpl::true_)
{
    detail::do_print_prefixed<Prefixes>(os, q);
}

template<class Prefixes, class CharT, class Traits, class Unit, class T>
void maybe_print_prefixed(std::basic_ostream<CharT, Traits>& os, const quantity<Unit, T>& q, mpl::false_)
{
    detail::print_default(os, q)();
}

inline BOOST_CONSTEXPR mpl::true_ test_norm(double) { return mpl::true_(); }
inline BOOST_CONSTEXPR mpl::false_ test_norm(one) { return mpl::false_(); }

} // namespace detail

template<class Dimension,class System>
inline std::string
typename_string(const unit<Dimension, System>&)
{
    return simplify_typename(typename reduce_unit< unit<Dimension,System> >::type());
}

template<class Dimension,class System>
inline std::string
symbol_string(const unit<Dimension, System>&)
{
    return detail::to_string_impl(unit<Dimension,System>(), detail::format_symbol_impl());
}

template<class Dimension,class System>
inline std::string
name_string(const unit<Dimension, System>&)
{
    return detail::to_string_impl(unit<Dimension,System>(), detail::format_name_impl());
}

/// Print a @c unit as a list of base units and their exponents.
///
///     for @c symbol_format outputs e.g. "m s^-1" or "J".
///     for @c name_format  outputs e.g. "meter second^-1" or "joule".
///     for @c raw_format  outputs e.g. "m s^-1" or "meter kilogram^2 second^-2".
///     for @c typename_format  outputs the typename itself (currently demangled only on GCC).
template<class Char, class Traits, class Dimension, class System>
inline std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& os, const unit<Dimension, System>& u)
{
    if (units::get_format(os) == typename_fmt)
    {
        detail::do_print(os, typename_string(u));
    }
    else if (units::get_format(os) == raw_fmt)
    {
        detail::do_print(os, detail::to_string_impl(u, detail::format_raw_symbol_impl()));
    }
    else if (units::get_format(os) == symbol_fmt)
    {
        detail::do_print(os, symbol_string(u));
    }
    else if (units::get_format(os) == name_fmt)
    {
        detail::do_print(os, name_string(u));
    }
    else
    {
        BOOST_ASSERT_MSG(false, "The format mode must be one of: typename_format, raw_format, name_format, symbol_format");
    }

    return(os);
}

/// \brief Print a @c quantity.
/// \details Prints the value followed by the unit.
/// If the engineering_prefix, or binary_prefix is set,
/// tries to scale the value appropriately.
/// For example, it might print 12.345 km instead of 12345 m.
/// (Note does @b not attempt to automatically scale scalars like double, float...)
template<class Char, class Traits, class Unit, class T>
inline std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& os, const quantity<Unit, T>& q)
{
    if (units::get_autoprefix(os) == autoprefix_none)
    {
        os << q.value() << ' ' << Unit();
    }
    else if (units::get_autoprefix(os) == autoprefix_engineering)
    {
        detail::maybe_print_prefixed<detail::engineering_prefixes>(os, q, detail::test_norm(autoprefix_norm(q.value())));
    }
    else if (units::get_autoprefix(os) == autoprefix_binary)
    {
        detail::maybe_print_prefixed<detail::binary_prefixes>(os, q, detail::test_norm(autoprefix_norm(q.value())));
    }
    else
    {
        BOOST_ASSERT_MSG(false, "Autoprefixing must be one of: no_prefix, engineering_prefix, binary_prefix");
    }
    return(os);
}

} // namespace units

} // namespace boost

#endif

// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_QUANTITY_HPP
#define BOOST_UNITS_QUANTITY_HPP

#include <algorithm>

#include <boost/config.hpp>
#include <boost/static_assert.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/units/conversion.hpp>
#include <boost/units/dimensionless_type.hpp>
#include <boost/units/homogeneous_system.hpp>
#include <boost/units/operators.hpp>
#include <boost/units/static_rational.hpp>
#include <boost/units/units_fwd.hpp>
#include <boost/units/detail/dimensionless_unit.hpp>

namespace boost {

namespace units {

namespace detail {

template<class T, class Enable = void>
struct is_base_unit : mpl::false_ {};

template<class T>
struct is_base_unit<T, typename T::boost_units_is_base_unit_type> : mpl::true_ {};

template<class Source, class Destination>
struct is_narrowing_conversion_impl : mpl::bool_<(sizeof(Source) > sizeof(Destination))> {};

template<class Source, class Destination>
struct is_non_narrowing_conversion :
    mpl::and_<
        boost::is_convertible<Source, Destination>,
        mpl::not_<
            mpl::and_<
                boost::is_arithmetic<Source>,
                boost::is_arithmetic<Destination>,
                mpl::or_<
                    mpl::and_<
                        is_integral<Destination>,
                        mpl::not_<is_integral<Source> >
                    >,
                    is_narrowing_conversion_impl<Source, Destination>
                >
            >
        >
    >
{};

template<>
struct is_non_narrowing_conversion<long double, double> : mpl::false_ {};

// msvc 7.1 needs extra disambiguation
template<class T, class U>
struct disable_if_is_same
{
    typedef void type;
};

template<class T>
struct disable_if_is_same<T, T> {};

}
 
/// class declaration
template<class Unit,class Y>
class quantity
{
        // base units are not the same as units.
        BOOST_MPL_ASSERT_NOT((detail::is_base_unit<Unit>));
        enum { force_instantiation_of_unit = sizeof(Unit) };
        typedef void (quantity::*unspecified_null_pointer_constant_type)(int*******);
    public:
        typedef quantity<Unit,Y>                        this_type;
        
        typedef Y                                       value_type;
        typedef Unit        unit_type;
 
        BOOST_CONSTEXPR quantity() : val_() 
        { 
            BOOST_UNITS_CHECK_LAYOUT_COMPATIBILITY(this_type, Y);
        }

        BOOST_CONSTEXPR quantity(unspecified_null_pointer_constant_type) : val_() 
        { 
            BOOST_UNITS_CHECK_LAYOUT_COMPATIBILITY(this_type, Y);
        }
        
        BOOST_CONSTEXPR quantity(const this_type& source) : val_(source.val_) 
        {
            BOOST_UNITS_CHECK_LAYOUT_COMPATIBILITY(this_type, Y);
        }
        
        // Need to make sure that the destructor of
        // Unit which contains the checking is instantiated,
        // on sun.
        #ifdef __SUNPRO_CC
        ~quantity() {
            unit_type force_unit_instantiation;
        }
        #endif
        
        //~quantity() { }
        
        BOOST_CXX14_CONSTEXPR this_type& operator=(const this_type& source) 
        { 
             val_ = source.val_; 
             
             return *this; 
        }

        #ifndef BOOST_NO_SFINAE

        /// implicit conversion between value types is allowed if allowed for value types themselves
        template<class YY>
        BOOST_CONSTEXPR quantity(const quantity<Unit,YY>& source,
            typename boost::enable_if<detail::is_non_narrowing_conversion<YY, Y> >::type* = 0) :
            val_(source.value())
        { 
            BOOST_UNITS_CHECK_LAYOUT_COMPATIBILITY(this_type, Y);
        }

        /// implicit conversion between value types is not allowed if not allowed for value types themselves
        template<class YY>
        explicit BOOST_CONSTEXPR quantity(const quantity<Unit,YY>& source,
            typename boost::disable_if<detail::is_non_narrowing_conversion<YY, Y> >::type* = 0) :
            val_(static_cast<Y>(source.value()))
        { 
            BOOST_UNITS_CHECK_LAYOUT_COMPATIBILITY(this_type, Y);
        }

        #else

        /// implicit conversion between value types is allowed if allowed for value types themselves
        template<class YY>
        BOOST_CONSTEXPR quantity(const quantity<Unit,YY>& source) :
            val_(source.value())
        { 
            BOOST_UNITS_CHECK_LAYOUT_COMPATIBILITY(this_type, Y);
            BOOST_STATIC_ASSERT((boost::is_convertible<YY, Y>::value == true));
        }

        #endif
        
        /// implicit assignment between value types is allowed if allowed for value types themselves
        template<class YY>
        BOOST_CXX14_CONSTEXPR this_type& operator=(const quantity<Unit,YY>& source)
        {
            BOOST_STATIC_ASSERT((boost::is_convertible<YY, Y>::value == true));

            *this = this_type(source);
            
            return *this;
        }

        #ifndef BOOST_NO_SFINAE

        /// explicit conversion between different unit systems is allowed if implicit conversion is disallowed
        template<class Unit2,class YY> 
        explicit
        BOOST_CONSTEXPR quantity(const quantity<Unit2,YY>& source, 
                 typename boost::disable_if<
                    mpl::and_<
                        //is_implicitly_convertible should be undefined when the
                        //units are not convertible at all
                        typename is_implicitly_convertible<Unit2,Unit>::type,
                        detail::is_non_narrowing_conversion<YY, Y>
                    >,
                    typename detail::disable_if_is_same<Unit, Unit2>::type
                 >::type* = 0)
             : val_(conversion_helper<quantity<Unit2,YY>,this_type>::convert(source).value())
        {
            BOOST_UNITS_CHECK_LAYOUT_COMPATIBILITY(this_type, Y);
            BOOST_STATIC_ASSERT((boost::is_convertible<YY,Y>::value == true));
        }

        /// implicit conversion between different unit systems is allowed if each fundamental dimension is implicitly convertible
        template<class Unit2,class YY> 
        BOOST_CONSTEXPR quantity(const quantity<Unit2,YY>& source, 
                 typename boost::enable_if<
                     mpl::and_<
                         typename is_implicitly_convertible<Unit2,Unit>::type,
                         detail::is_non_narrowing_conversion<YY, Y>
                     >,
                     typename detail::disable_if_is_same<Unit, Unit2>::type
                 >::type* = 0)
             : val_(conversion_helper<quantity<Unit2,YY>,this_type>::convert(source).value())
        {
            BOOST_UNITS_CHECK_LAYOUT_COMPATIBILITY(this_type, Y);
            BOOST_STATIC_ASSERT((boost::is_convertible<YY,Y>::value == true));
        }

        #else

        /// without SFINAE we can't distinguish between explicit and implicit conversions so 
        /// the conversion is always explicit
        template<class Unit2,class YY> 
        explicit BOOST_CONSTEXPR quantity(const quantity<Unit2,YY>& source)
             : val_(conversion_helper<quantity<Unit2,YY>,this_type>::convert(source).value())
        {
            BOOST_UNITS_CHECK_LAYOUT_COMPATIBILITY(this_type, Y);
            BOOST_STATIC_ASSERT((boost::is_convertible<YY,Y>::value == true));
        }

        #endif
        
        /// implicit assignment between different unit systems is allowed if each fundamental dimension is implicitly convertible 
        template<class Unit2,class YY>
        BOOST_CXX14_CONSTEXPR this_type& operator=(const quantity<Unit2,YY>& source)
        {
            
            BOOST_STATIC_ASSERT((is_implicitly_convertible<Unit2,unit_type>::value == true));
            BOOST_STATIC_ASSERT((boost::is_convertible<YY,Y>::value == true));
            
            *this = this_type(source);
            
            return *this;
        }

        BOOST_CONSTEXPR const value_type& value() const     { return val_; }                        ///< constant accessor to value
        
        ///< can add a quantity of the same type if add_typeof_helper<value_type,value_type>::type is convertible to value_type
        template<class Unit2, class YY>
        BOOST_CXX14_CONSTEXPR this_type& operator+=(const quantity<Unit2, YY>& source)
        {
            BOOST_STATIC_ASSERT((boost::is_same<typename add_typeof_helper<Unit, Unit2>::type, Unit>::value));
            val_ += source.value();
            return *this;
        }  

        ///< can subtract a quantity of the same type if subtract_typeof_helper<value_type,value_type>::type is convertible to value_type
        template<class Unit2, class YY>
        BOOST_CXX14_CONSTEXPR this_type& operator-=(const quantity<Unit2, YY>& source)
        {
            BOOST_STATIC_ASSERT((boost::is_same<typename subtract_typeof_helper<Unit, Unit2>::type, Unit>::value));
            val_ -= source.value();
            return *this;
        }  

        template<class Unit2, class YY>
        BOOST_CXX14_CONSTEXPR this_type& operator*=(const quantity<Unit2, YY>& source)
        {
            BOOST_STATIC_ASSERT((boost::is_same<typename multiply_typeof_helper<Unit, Unit2>::type, Unit>::value));
            val_ *= source.value();
            return *this;
        }  
        
        template<class Unit2, class YY>
        BOOST_CXX14_CONSTEXPR this_type& operator/=(const quantity<Unit2, YY>& source)
        {
            BOOST_STATIC_ASSERT((boost::is_same<typename divide_typeof_helper<Unit, Unit2>::type, Unit>::value));
            val_ /= source.value();
            return *this;
        }

        ///< can multiply a quantity by a scalar value_type if multiply_typeof_helper<value_type,value_type>::type is convertible to value_type
        BOOST_CXX14_CONSTEXPR this_type& operator*=(const value_type& source) { val_ *= source; return *this; }
        ///< can divide a quantity by a scalar value_type if divide_typeof_helper<value_type,value_type>::type is convertible to value_type
        BOOST_CXX14_CONSTEXPR this_type& operator/=(const value_type& source) { val_ /= source; return *this; }
    
        /// Construct quantity directly from @c value_type (potentially dangerous).
        static BOOST_CONSTEXPR this_type from_value(const value_type& val)  { return this_type(val, 0); }

    protected:
        explicit BOOST_CONSTEXPR quantity(const value_type& val, int) : val_(val) { }
        
    private:
        value_type    val_;
};

/// Specialization for dimensionless quantities. Implicit conversions between 
/// unit systems are allowed because all dimensionless quantities are equivalent.
/// Implicit construction and assignment from and conversion to @c value_type is
/// also allowed.
template<class System,class Y>
class quantity<BOOST_UNITS_DIMENSIONLESS_UNIT(System),Y>
{
    public:
        typedef quantity<unit<dimensionless_type,System>,Y>     this_type;
                                   
        typedef Y                                               value_type;
        typedef System                                          system_type;
        typedef dimensionless_type                              dimension_type;
        typedef unit<dimension_type,system_type>                unit_type;
         
        BOOST_CONSTEXPR quantity() : val_() 
        {
            BOOST_UNITS_CHECK_LAYOUT_COMPATIBILITY(this_type, Y);
        }
        
        /// construction from raw @c value_type is allowed
        BOOST_CONSTEXPR quantity(value_type val) : val_(val) 
        {
            BOOST_UNITS_CHECK_LAYOUT_COMPATIBILITY(this_type, Y);
        } 
                           
        BOOST_CONSTEXPR quantity(const this_type& source) : val_(source.val_) 
        {
            BOOST_UNITS_CHECK_LAYOUT_COMPATIBILITY(this_type, Y);
        }
        
        //~quantity() { }
        
        BOOST_CXX14_CONSTEXPR this_type& operator=(const this_type& source) 
        { 
            val_ = source.val_; 
                
            return *this; 
        }
        
        #ifndef BOOST_NO_SFINAE

        /// implicit conversion between value types is allowed if allowed for value types themselves
        template<class YY>
        BOOST_CONSTEXPR quantity(const quantity<unit<dimension_type,system_type>,YY>& source,
            typename boost::enable_if<detail::is_non_narrowing_conversion<YY, Y> >::type* = 0) :
            val_(source.value())
        { 
            BOOST_UNITS_CHECK_LAYOUT_COMPATIBILITY(this_type, Y);
        }

        /// implicit conversion between value types is not allowed if not allowed for value types themselves
        template<class YY>
        explicit BOOST_CONSTEXPR quantity(const quantity<unit<dimension_type,system_type>,YY>& source,
            typename boost::disable_if<detail::is_non_narrowing_conversion<YY, Y> >::type* = 0) :
            val_(static_cast<Y>(source.value()))
        { 
            BOOST_UNITS_CHECK_LAYOUT_COMPATIBILITY(this_type, Y);
        }

        #else

        /// implicit conversion between value types is allowed if allowed for value types themselves
        template<class YY>
        BOOST_CONSTEXPR quantity(const quantity<unit<dimension_type,system_type>,YY>& source) :
            val_(source.value())
        { 
            BOOST_UNITS_CHECK_LAYOUT_COMPATIBILITY(this_type, Y);
            BOOST_STATIC_ASSERT((boost::is_convertible<YY, Y>::value == true));
        }

        #endif
        
        /// implicit assignment between value types is allowed if allowed for value types themselves
        template<class YY>
        BOOST_CXX14_CONSTEXPR this_type& operator=(const quantity<unit<dimension_type,system_type>,YY>& source)
        {
            BOOST_STATIC_ASSERT((boost::is_convertible<YY,Y>::value == true));

            *this = this_type(source);
            
            return *this;
        }

        #if 1

        /// implicit conversion between different unit systems is allowed
        template<class System2, class Y2> 
        BOOST_CONSTEXPR quantity(const quantity<unit<dimensionless_type, System2>,Y2>& source,
        #ifdef __SUNPRO_CC
            typename boost::enable_if<
                boost::mpl::and_<
                    detail::is_non_narrowing_conversion<Y2, Y>,
                    detail::is_dimensionless_system<System2>
                > 
            >::type* = 0
        #else
            typename boost::enable_if<detail::is_non_narrowing_conversion<Y2, Y> >::type* = 0,
            typename detail::disable_if_is_same<System, System2>::type* = 0,
            typename boost::enable_if<detail::is_dimensionless_system<System2> >::type* = 0
        #endif
            ) :
            val_(source.value()) 
        {
            BOOST_UNITS_CHECK_LAYOUT_COMPATIBILITY(this_type, Y);
        }

        /// implicit conversion between different unit systems is allowed
        template<class System2, class Y2> 
        explicit BOOST_CONSTEXPR quantity(const quantity<unit<dimensionless_type, System2>,Y2>& source,
        #ifdef __SUNPRO_CC
            typename boost::enable_if<
                boost::mpl::and_<
                    boost::mpl::not_<detail::is_non_narrowing_conversion<Y2, Y> >,
                    detail::is_dimensionless_system<System2>
                > 
            >::type* = 0
        #else
            typename boost::disable_if<detail::is_non_narrowing_conversion<Y2, Y> >::type* = 0,
            typename detail::disable_if_is_same<System, System2>::type* = 0,
            typename boost::enable_if<detail::is_dimensionless_system<System2> >::type* = 0
        #endif
            ) :
            val_(static_cast<Y>(source.value())) 
        {
            BOOST_UNITS_CHECK_LAYOUT_COMPATIBILITY(this_type, Y);
        }

        #else

        /// implicit conversion between different unit systems is allowed
        template<class System2, class Y2> 
        BOOST_CONSTEXPR quantity(const quantity<unit<dimensionless_type,homogeneous_system<System2> >,Y2>& source) :
            val_(source.value()) 
        {
            BOOST_UNITS_CHECK_LAYOUT_COMPATIBILITY(this_type, Y);
            BOOST_STATIC_ASSERT((boost::is_convertible<Y2, Y>::value == true));
        }

        #endif

        /// conversion between different unit systems is explicit when
        /// the units are not equivalent.
        template<class System2, class Y2> 
        explicit BOOST_CONSTEXPR quantity(const quantity<unit<dimensionless_type, System2>,Y2>& source,
            typename boost::disable_if<detail::is_dimensionless_system<System2> >::type* = 0) :
            val_(conversion_helper<quantity<unit<dimensionless_type, System2>,Y2>, this_type>::convert(source).value()) 
        {
            BOOST_UNITS_CHECK_LAYOUT_COMPATIBILITY(this_type, Y);
        }
        
        #ifndef __SUNPRO_CC

        /// implicit assignment between different unit systems is allowed
        template<class System2>
        BOOST_CXX14_CONSTEXPR this_type& operator=(const quantity<BOOST_UNITS_DIMENSIONLESS_UNIT(System2),Y>& source)
        {
            *this = this_type(source);
            
            return *this;
        }
        
        #endif
        
        /// implicit conversion to @c value_type is allowed
        BOOST_CONSTEXPR operator value_type() const                               { return val_; }
        
        BOOST_CONSTEXPR const value_type& value() const                           { return val_; }  ///< constant accessor to value
        
        ///< can add a quantity of the same type if add_typeof_helper<value_type,value_type>::type is convertible to value_type
        BOOST_CXX14_CONSTEXPR this_type& operator+=(const this_type& source)      { val_ += source.val_; return *this; }  
        
        ///< can subtract a quantity of the same type if subtract_typeof_helper<value_type,value_type>::type is convertible to value_type
        BOOST_CXX14_CONSTEXPR this_type& operator-=(const this_type& source)      { val_ -= source.val_; return *this; }  
        
        ///< can multiply a quantity by a scalar value_type if multiply_typeof_helper<value_type,value_type>::type is convertible to value_type
        BOOST_CXX14_CONSTEXPR this_type& operator*=(const value_type& val)        { val_ *= val; return *this; }          

        ///< can divide a quantity by a scalar value_type if divide_typeof_helper<value_type,value_type>::type is convertible to value_type
        BOOST_CXX14_CONSTEXPR this_type& operator/=(const value_type& val)        { val_ /= val; return *this; }          

        /// Construct quantity directly from @c value_type.
        static BOOST_CONSTEXPR this_type from_value(const value_type& val)        { return this_type(val); }

   private:
        value_type    val_;
};

#ifdef BOOST_MSVC
// HACK: For some obscure reason msvc 8.0 needs these specializations
template<class System, class T>
class quantity<unit<int, System>, T> {};
template<class T>
class quantity<int, T> {};
#endif

} // namespace units

} // namespace boost

#if BOOST_UNITS_HAS_BOOST_TYPEOF

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TEMPLATE(boost::units::quantity, 2)

#endif

namespace boost {

namespace units {

namespace detail {

/// helper class for quantity_cast
template<class X,class Y> struct quantity_cast_helper;

/// specialization for casting to the value type
template<class Y,class X,class Unit>
struct quantity_cast_helper<Y,quantity<Unit,X> >
{
    typedef Y type;
    
    BOOST_CONSTEXPR type operator()(quantity<Unit,X>& source) const         { return const_cast<X&>(source.value()); }
};

/// specialization for casting to the value type
template<class Y,class X,class Unit>
struct quantity_cast_helper<Y,const quantity<Unit,X> >
{
    typedef Y type;
    
    BOOST_CONSTEXPR type operator()(const quantity<Unit,X>& source) const   { return source.value(); }
};

} // namespace detail

/// quantity_cast provides mutating access to underlying quantity value_type
template<class X,class Y>
inline 
BOOST_CONSTEXPR
X
quantity_cast(Y& source)
{
    return detail::quantity_cast_helper<X,Y>()(source);
}

template<class X,class Y>
inline 
BOOST_CONSTEXPR
X
quantity_cast(const Y& source)
{
    return detail::quantity_cast_helper<X,const Y>()(source);
}

/// swap quantities
template<class Unit,class Y>
inline void swap(quantity<Unit,Y>& lhs, quantity<Unit,Y>& rhs)
{
    using std::swap;
    swap(quantity_cast<Y&>(lhs),quantity_cast<Y&>(rhs));
}

/// specialize unary plus typeof helper
/// INTERNAL ONLY
template<class Unit,class Y>
struct unary_plus_typeof_helper< quantity<Unit,Y> >
{
    typedef typename unary_plus_typeof_helper<Y>::type      value_type;
    typedef typename unary_plus_typeof_helper<Unit>::type   unit_type;
    typedef quantity<unit_type,value_type>                  type;
};

/// specialize unary minus typeof helper
/// INTERNAL ONLY
template<class Unit,class Y>
struct unary_minus_typeof_helper< quantity<Unit,Y> >
{
    typedef typename unary_minus_typeof_helper<Y>::type     value_type;
    typedef typename unary_minus_typeof_helper<Unit>::type  unit_type;
    typedef quantity<unit_type,value_type>                  type;
};

/// specialize add typeof helper
/// INTERNAL ONLY
template<class Unit1,
         class Unit2,
         class X,
         class Y>
struct add_typeof_helper< quantity<Unit1,X>,quantity<Unit2,Y> >
{
    typedef typename add_typeof_helper<X,Y>::type           value_type;
    typedef typename add_typeof_helper<Unit1,Unit2>::type   unit_type;
    typedef quantity<unit_type,value_type>                  type;
};

/// for sun CC we need to invoke SFINAE at
/// the top level, otherwise it will silently
/// return int.
template<class Dim1, class System1,
         class Dim2, class System2,
         class X,
         class Y>
struct add_typeof_helper< quantity<unit<Dim1, System1>,X>,quantity<unit<Dim2, System2>,Y> >
{
};

template<class Dim,
         class System,
         class X,
         class Y>
struct add_typeof_helper< quantity<unit<Dim, System>,X>,quantity<unit<Dim, System>,Y> >
{
    typedef typename add_typeof_helper<X,Y>::type  value_type;
    typedef unit<Dim, System>                      unit_type;
    typedef quantity<unit_type,value_type>         type;
};

/// specialize subtract typeof helper
/// INTERNAL ONLY
template<class Unit1,
         class Unit2,
         class X,
         class Y>
struct subtract_typeof_helper< quantity<Unit1,X>,quantity<Unit2,Y> >
{
    typedef typename subtract_typeof_helper<X,Y>::type          value_type;
    typedef typename subtract_typeof_helper<Unit1,Unit2>::type  unit_type;
    typedef quantity<unit_type,value_type>                      type;
};

// Force adding different units to fail on sun.
template<class Dim1, class System1,
         class Dim2, class System2,
         class X,
         class Y>
struct subtract_typeof_helper< quantity<unit<Dim1, System1>,X>,quantity<unit<Dim2, System2>,Y> >
{
};

template<class Dim,
         class System,
         class X,
         class Y>
struct subtract_typeof_helper< quantity<unit<Dim, System>,X>,quantity<unit<Dim, System>,Y> >
{
    typedef typename subtract_typeof_helper<X,Y>::type  value_type;
    typedef unit<Dim, System>                           unit_type;
    typedef quantity<unit_type,value_type>              type;
};

/// scalar times unit typeof helper
/// INTERNAL ONLY
template<class System,
         class Dim,
         class X>
struct multiply_typeof_helper< X,unit<Dim,System> >
{
    typedef X                               value_type;
    typedef unit<Dim,System>                unit_type;
    typedef quantity<unit_type,value_type>  type;
};

/// unit times scalar typeof helper
/// INTERNAL ONLY
template<class System,
         class Dim,
         class X>
struct multiply_typeof_helper< unit<Dim,System>,X >
{
    typedef X                               value_type;
    typedef unit<Dim,System>                unit_type;
    typedef quantity<unit_type,value_type>  type;
};

/// scalar times quantity typeof helper
/// INTERNAL ONLY
template<class Unit,
         class X,
         class Y>
struct multiply_typeof_helper< X,quantity<Unit,Y> >
{
    typedef typename multiply_typeof_helper<X,Y>::type  value_type;
    typedef Unit                                        unit_type;
    typedef quantity<unit_type,value_type>              type;
};

/// disambiguate
/// INTERNAL ONLY
template<class Unit,
         class Y>
struct multiply_typeof_helper< one,quantity<Unit,Y> >
{
    typedef quantity<Unit,Y> type;
};

/// quantity times scalar typeof helper
/// INTERNAL ONLY
template<class Unit,
         class X,
         class Y>
struct multiply_typeof_helper< quantity<Unit,X>,Y >
{
    typedef typename multiply_typeof_helper<X,Y>::type  value_type;
    typedef Unit                                        unit_type;
    typedef quantity<unit_type,value_type>              type;
};

/// disambiguate
/// INTERNAL ONLY
template<class Unit,
         class X>
struct multiply_typeof_helper< quantity<Unit,X>,one >
{
    typedef quantity<Unit,X> type;
};

/// unit times quantity typeof helper
/// INTERNAL ONLY
template<class Unit,
         class System,
         class Dim,
         class X>
struct multiply_typeof_helper< unit<Dim,System>,quantity<Unit,X> >
{
    typedef X                                                               value_type;
    typedef typename multiply_typeof_helper< unit<Dim,System>,Unit >::type  unit_type;
    typedef quantity<unit_type,value_type>                                  type;
};

/// quantity times unit typeof helper
/// INTERNAL ONLY
template<class Unit,
         class System,
         class Dim,
         class X>
struct multiply_typeof_helper< quantity<Unit,X>,unit<Dim,System> >
{
    typedef X                                                               value_type;
    typedef typename multiply_typeof_helper< Unit,unit<Dim,System> >::type  unit_type;
    typedef quantity<unit_type,value_type>                                  type;
};

/// quantity times quantity typeof helper
/// INTERNAL ONLY
template<class Unit1,
         class Unit2,
         class X,
         class Y>
struct multiply_typeof_helper< quantity<Unit1,X>,quantity<Unit2,Y> >
{
    typedef typename multiply_typeof_helper<X,Y>::type          value_type;
    typedef typename multiply_typeof_helper<Unit1,Unit2>::type  unit_type;
    typedef quantity<unit_type,value_type>                      type;
};

/// scalar divided by unit typeof helper
/// INTERNAL ONLY
template<class System,
         class Dim,
         class X>
struct divide_typeof_helper< X,unit<Dim,System> >
{
    typedef X                                                                           value_type;
    typedef typename power_typeof_helper< unit<Dim,System>,static_rational<-1> >::type  unit_type;
    typedef quantity<unit_type,value_type>                                              type;
};

/// unit divided by scalar typeof helper
/// INTERNAL ONLY
template<class System,
         class Dim,
         class X>
struct divide_typeof_helper< unit<Dim,System>,X >
{
    typedef typename divide_typeof_helper<X,X>::type    value_type;
    typedef unit<Dim,System>                            unit_type;
    typedef quantity<unit_type,value_type>              type;
};

/// scalar divided by quantity typeof helper
/// INTERNAL ONLY
template<class Unit,
         class X,
         class Y>
struct divide_typeof_helper< X,quantity<Unit,Y> >
{
    typedef typename divide_typeof_helper<X,Y>::type                        value_type;
    typedef typename power_typeof_helper< Unit,static_rational<-1> >::type  unit_type;
    typedef quantity<unit_type,value_type>                                  type;
};

/// disambiguate
/// INTERNAL ONLY
template<class Unit,
         class Y>
struct divide_typeof_helper< one,quantity<Unit,Y> >
{
    typedef quantity<Unit,Y> type;
};

/// quantity divided by scalar typeof helper
/// INTERNAL ONLY
template<class Unit,
         class X,
         class Y>
struct divide_typeof_helper< quantity<Unit,X>,Y >
{
    typedef typename divide_typeof_helper<X,Y>::type    value_type;
    typedef Unit                                        unit_type;
    typedef quantity<unit_type,value_type>              type;
};

/// disambiguate
/// INTERNAL ONLY
template<class Unit,
         class X>
struct divide_typeof_helper< quantity<Unit,X>,one >
{
    typedef quantity<Unit,X> type;
};

/// unit divided by quantity typeof helper
/// INTERNAL ONLY
template<class Unit,
         class System,
         class Dim,
         class X>
struct divide_typeof_helper< unit<Dim,System>,quantity<Unit,X> >
{
    typedef typename divide_typeof_helper<X,X>::type                        value_type;
    typedef typename divide_typeof_helper< unit<Dim,System>,Unit >::type    unit_type;
    typedef quantity<unit_type,value_type>                                  type;
};

/// quantity divided by unit typeof helper
/// INTERNAL ONLY
template<class Unit,
         class System,
         class Dim,
         class X>
struct divide_typeof_helper< quantity<Unit,X>,unit<Dim,System> >
{
    typedef X                                                               value_type;
    typedef typename divide_typeof_helper< Unit,unit<Dim,System> >::type    unit_type;
    typedef quantity<unit_type,value_type>                                  type;
};

/// quantity divided by quantity typeof helper
/// INTERNAL ONLY
template<class Unit1,
         class Unit2,
         class X,
         class Y>
struct divide_typeof_helper< quantity<Unit1,X>,quantity<Unit2,Y> >
{
    typedef typename divide_typeof_helper<X,Y>::type            value_type;
    typedef typename divide_typeof_helper<Unit1,Unit2>::type    unit_type;
    typedef quantity<unit_type,value_type>                      type;
};

/// specialize power typeof helper
/// INTERNAL ONLY
template<class Unit,long N,long D,class Y> 
struct power_typeof_helper< quantity<Unit,Y>,static_rational<N,D> >                
{ 
    typedef typename power_typeof_helper<Y,static_rational<N,D> >::type     value_type;
    typedef typename power_typeof_helper<Unit,static_rational<N,D> >::type  unit_type;
    typedef quantity<unit_type,value_type>                                  type; 
    
    static BOOST_CONSTEXPR type value(const quantity<Unit,Y>& x)  
    { 
        return type::from_value(power_typeof_helper<Y,static_rational<N,D> >::value(x.value()));
    }
};

/// specialize root typeof helper
/// INTERNAL ONLY
template<class Unit,long N,long D,class Y> 
struct root_typeof_helper< quantity<Unit,Y>,static_rational<N,D> >                
{ 
    typedef typename root_typeof_helper<Y,static_rational<N,D> >::type      value_type;
    typedef typename root_typeof_helper<Unit,static_rational<N,D> >::type   unit_type;
    typedef quantity<unit_type,value_type>                                  type;
    
    static BOOST_CONSTEXPR type value(const quantity<Unit,Y>& x)  
    { 
        return type::from_value(root_typeof_helper<Y,static_rational<N,D> >::value(x.value()));
    }
};

/// runtime unit times scalar
/// INTERNAL ONLY
template<class System,
         class Dim,
         class Y>
inline
BOOST_CONSTEXPR
typename multiply_typeof_helper< unit<Dim,System>,Y >::type
operator*(const unit<Dim,System>&,const Y& rhs)
{
    typedef typename multiply_typeof_helper< unit<Dim,System>,Y >::type type;
    
    return type::from_value(rhs);
}

/// runtime unit divided by scalar
template<class System,
         class Dim,
         class Y>
inline
BOOST_CONSTEXPR
typename divide_typeof_helper< unit<Dim,System>,Y >::type
operator/(const unit<Dim,System>&,const Y& rhs)
{
    typedef typename divide_typeof_helper<unit<Dim,System>,Y>::type type;
    
    return type::from_value(Y(1)/rhs);
}

/// runtime scalar times unit
template<class System,
         class Dim,
         class Y>
inline
BOOST_CONSTEXPR
typename multiply_typeof_helper< Y,unit<Dim,System> >::type
operator*(const Y& lhs,const unit<Dim,System>&)
{
    typedef typename multiply_typeof_helper< Y,unit<Dim,System> >::type type;
    
    return type::from_value(lhs);
}

/// runtime scalar divided by unit
template<class System,
         class Dim,
         class Y>
inline
BOOST_CONSTEXPR
typename divide_typeof_helper< Y,unit<Dim,System> >::type
operator/(const Y& lhs,const unit<Dim,System>&)
{
    typedef typename divide_typeof_helper< Y,unit<Dim,System> >::type   type;
    
    return type::from_value(lhs);
}

///// runtime quantity times scalar
//template<class Unit,
//         class X,
//         class Y>
//inline
//BOOST_CONSTEXPR
//typename multiply_typeof_helper< quantity<Unit,X>,Y >::type
//operator*(const quantity<Unit,X>& lhs,const Y& rhs)
//{
//    typedef typename multiply_typeof_helper< quantity<Unit,X>,Y >::type type;
//    
//    return type::from_value(lhs.value()*rhs);
//}
//
///// runtime scalar times quantity
//template<class Unit,
//         class X,
//         class Y>
//inline
//BOOST_CONSTEXPR
//typename multiply_typeof_helper< X,quantity<Unit,Y> >::type
//operator*(const X& lhs,const quantity<Unit,Y>& rhs)
//{
//    typedef typename multiply_typeof_helper< X,quantity<Unit,Y> >::type type;
//    
//    return type::from_value(lhs*rhs.value());
//}

/// runtime quantity times scalar
template<class Unit,
         class X>
inline
BOOST_CONSTEXPR
typename multiply_typeof_helper< quantity<Unit,X>,X >::type
operator*(const quantity<Unit,X>& lhs,const X& rhs)
{
    typedef typename multiply_typeof_helper< quantity<Unit,X>,X >::type type;
    
    return type::from_value(lhs.value()*rhs);
}

/// runtime scalar times quantity
template<class Unit,
         class X>
inline
BOOST_CONSTEXPR
typename multiply_typeof_helper< X,quantity<Unit,X> >::type
operator*(const X& lhs,const quantity<Unit,X>& rhs)
{
    typedef typename multiply_typeof_helper< X,quantity<Unit,X> >::type type;
    
    return type::from_value(lhs*rhs.value());
}

///// runtime quantity divided by scalar
//template<class Unit,
//         class X,
//         class Y>
//inline
//BOOST_CONSTEXPR
//typename divide_typeof_helper< quantity<Unit,X>,Y >::type
//operator/(const quantity<Unit,X>& lhs,const Y& rhs)
//{
//    typedef typename divide_typeof_helper< quantity<Unit,X>,Y >::type   type;
//    
//    return type::from_value(lhs.value()/rhs);
//}
//
///// runtime scalar divided by quantity
//template<class Unit,
//         class X,
//         class Y>
//inline
//BOOST_CONSTEXPR
//typename divide_typeof_helper< X,quantity<Unit,Y> >::type
//operator/(const X& lhs,const quantity<Unit,Y>& rhs)
//{
//    typedef typename divide_typeof_helper< X,quantity<Unit,Y> >::type   type;
//    
//    return type::from_value(lhs/rhs.value());
//}

/// runtime quantity divided by scalar
template<class Unit,
         class X>
inline
BOOST_CONSTEXPR
typename divide_typeof_helper< quantity<Unit,X>,X >::type
operator/(const quantity<Unit,X>& lhs,const X& rhs)
{
    typedef typename divide_typeof_helper< quantity<Unit,X>,X >::type   type;
    
    return type::from_value(lhs.value()/rhs);
}

/// runtime scalar divided by quantity
template<class Unit,
         class X>
inline
BOOST_CONSTEXPR
typename divide_typeof_helper< X,quantity<Unit,X> >::type
operator/(const X& lhs,const quantity<Unit,X>& rhs)
{
    typedef typename divide_typeof_helper< X,quantity<Unit,X> >::type   type;
    
    return type::from_value(lhs/rhs.value());
}

/// runtime unit times quantity
template<class System1,
         class Dim1,
         class Unit2,
         class Y>
inline
BOOST_CONSTEXPR
typename multiply_typeof_helper< unit<Dim1,System1>,quantity<Unit2,Y> >::type
operator*(const unit<Dim1,System1>&,const quantity<Unit2,Y>& rhs)
{
    typedef typename multiply_typeof_helper< unit<Dim1,System1>,quantity<Unit2,Y> >::type  type;
    
    return type::from_value(rhs.value());
}

/// runtime unit divided by quantity
template<class System1,
         class Dim1,
         class Unit2,
         class Y>
inline
BOOST_CONSTEXPR
typename divide_typeof_helper< unit<Dim1,System1>,quantity<Unit2,Y> >::type
operator/(const unit<Dim1,System1>&,const quantity<Unit2,Y>& rhs)
{
    typedef typename divide_typeof_helper< unit<Dim1,System1>,quantity<Unit2,Y> >::type    type;
    
    return type::from_value(Y(1)/rhs.value());
}

/// runtime quantity times unit
template<class Unit1,
         class System2,
         class Dim2,
         class Y>
inline
BOOST_CONSTEXPR
typename multiply_typeof_helper< quantity<Unit1,Y>,unit<Dim2,System2> >::type
operator*(const quantity<Unit1,Y>& lhs,const unit<Dim2,System2>&)
{
    typedef typename multiply_typeof_helper< quantity<Unit1,Y>,unit<Dim2,System2> >::type  type;
    
    return type::from_value(lhs.value());
}

/// runtime quantity divided by unit
template<class Unit1,
         class System2,
         class Dim2,
         class Y>
inline
BOOST_CONSTEXPR
typename divide_typeof_helper< quantity<Unit1,Y>,unit<Dim2,System2> >::type
operator/(const quantity<Unit1,Y>& lhs,const unit<Dim2,System2>&)
{
    typedef typename divide_typeof_helper< quantity<Unit1,Y>,unit<Dim2,System2> >::type    type;
    
    return type::from_value(lhs.value());
}

/// runtime unary plus quantity
template<class Unit,class Y>
BOOST_CONSTEXPR
typename unary_plus_typeof_helper< quantity<Unit,Y> >::type
operator+(const quantity<Unit,Y>& val)
{ 
    typedef typename unary_plus_typeof_helper< quantity<Unit,Y> >::type     type;
    
    return type::from_value(+val.value());
}

/// runtime unary minus quantity
template<class Unit,class Y>
BOOST_CONSTEXPR
typename unary_minus_typeof_helper< quantity<Unit,Y> >::type
operator-(const quantity<Unit,Y>& val)
{ 
    typedef typename unary_minus_typeof_helper< quantity<Unit,Y> >::type    type;
    
    return type::from_value(-val.value());
}

/// runtime quantity plus quantity
template<class Unit1,
         class Unit2,
         class X,
         class Y>
inline
BOOST_CONSTEXPR
typename add_typeof_helper< quantity<Unit1,X>,quantity<Unit2,Y> >::type
operator+(const quantity<Unit1,X>& lhs,
          const quantity<Unit2,Y>& rhs)
{
    typedef typename add_typeof_helper< quantity<Unit1,X>,quantity<Unit2,Y> >::type     type;
    
    return type::from_value(lhs.value()+rhs.value());
}

/// runtime quantity minus quantity
template<class Unit1,
         class Unit2,
         class X,
         class Y>
inline
BOOST_CONSTEXPR
typename subtract_typeof_helper< quantity<Unit1,X>,quantity<Unit2,Y> >::type
operator-(const quantity<Unit1,X>& lhs,
          const quantity<Unit2,Y>& rhs)
{
    typedef typename subtract_typeof_helper< quantity<Unit1,X>,quantity<Unit2,Y> >::type    type;
    
    return type::from_value(lhs.value()-rhs.value());
}

/// runtime quantity times quantity
template<class Unit1,
         class Unit2,
         class X,
         class Y>
inline
BOOST_CONSTEXPR
typename multiply_typeof_helper< quantity<Unit1,X>,quantity<Unit2,Y> >::type
operator*(const quantity<Unit1,X>& lhs,
          const quantity<Unit2,Y>& rhs)
{
    typedef typename multiply_typeof_helper< quantity<Unit1,X>,
                                             quantity<Unit2,Y> >::type type;
    
    return type::from_value(lhs.value()*rhs.value());
}

/// runtime quantity divided by quantity
template<class Unit1,
         class Unit2,
         class X,
         class Y>
inline
BOOST_CONSTEXPR
typename divide_typeof_helper< quantity<Unit1,X>,quantity<Unit2,Y> >::type
operator/(const quantity<Unit1,X>& lhs,
          const quantity<Unit2,Y>& rhs)
{
    typedef typename divide_typeof_helper< quantity<Unit1,X>,
                                           quantity<Unit2,Y> >::type   type;
    
    return type::from_value(lhs.value()/rhs.value());
}

/// runtime operator==
template<class Unit,
         class X,
         class Y>
inline
BOOST_CONSTEXPR
bool 
operator==(const quantity<Unit,X>& val1,
           const quantity<Unit,Y>& val2)
{
    return val1.value() == val2.value();
}

/// runtime operator!=
template<class Unit,
         class X,
         class Y>
inline
BOOST_CONSTEXPR
bool 
operator!=(const quantity<Unit,X>& val1,
           const quantity<Unit,Y>& val2)
{
    return val1.value() != val2.value();
}

/// runtime operator<
template<class Unit,
         class X,
         class Y>
inline
BOOST_CONSTEXPR
bool 
operator<(const quantity<Unit,X>& val1,
          const quantity<Unit,Y>& val2)
{
    return val1.value() < val2.value();
}

/// runtime operator<=
template<class Unit,
         class X,
         class Y>
inline
BOOST_CONSTEXPR
bool 
operator<=(const quantity<Unit,X>& val1,
           const quantity<Unit,Y>& val2)
{
    return val1.value() <= val2.value();
}

/// runtime operator>
template<class Unit,
         class X,
         class Y>
inline
BOOST_CONSTEXPR
bool 
operator>(const quantity<Unit,X>& val1,
          const quantity<Unit,Y>& val2)
{
    return val1.value() > val2.value();
}

/// runtime operator>=
template<class Unit,
         class X,
         class Y>
inline
BOOST_CONSTEXPR
bool 
operator>=(const quantity<Unit,X>& val1,
           const quantity<Unit,Y>& val2)
{
    return val1.value() >= val2.value();
}

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_QUANTITY_HPP

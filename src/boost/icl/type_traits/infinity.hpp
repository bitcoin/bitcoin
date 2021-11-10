/*-----------------------------------------------------------------------------+
Copyright (c) 2010-2011: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TYPE_TRAITS_INFINITY_HPP_JOFA_100322
#define BOOST_ICL_TYPE_TRAITS_INFINITY_HPP_JOFA_100322

#include <string>
#include <boost/static_assert.hpp>
#include <boost/icl/type_traits/is_numeric.hpp>
#include <boost/icl/type_traits/rep_type_of.hpp>
#include <boost/icl/type_traits/size_type_of.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/if.hpp>

namespace boost{ namespace icl
{

template<class Type> struct has_std_infinity
{
    typedef has_std_infinity type;
    BOOST_STATIC_CONSTANT(bool, 
        value = (     is_numeric<Type>::value
                   && std::numeric_limits<Type>::has_infinity
                )
       );
};

template<class Type> struct has_max_infinity
{
    typedef has_max_infinity type;
    BOOST_STATIC_CONSTANT(bool, 
        value = (     is_numeric<Type>::value
                   && ! std::numeric_limits<Type>::has_infinity
                )
       );
};

//------------------------------------------------------------------------------
template <class Type, bool has_std_inf=false, bool has_std_max=false> 
struct get_numeric_infinity;

template <class Type, bool has_std_max> 
struct get_numeric_infinity<Type, true, has_std_max>
{
    typedef get_numeric_infinity type;
    static Type value()
    {
        return (std::numeric_limits<Type>::infinity)();
    }
};

template <class Type> 
struct get_numeric_infinity<Type, false, true>
{
    typedef get_numeric_infinity type;
    static Type value()
    {
        return (std::numeric_limits<Type>::max)();
    }
};

template <class Type> 
struct get_numeric_infinity<Type, false, false>
{
    typedef get_numeric_infinity type;
    static Type value()
    {
        return Type();
    }
};

template <class Type> 
struct numeric_infinity
{
    typedef numeric_infinity type;
    static Type value()
    {
        return get_numeric_infinity< Type
                                   , has_std_infinity<Type>::value
                                   , has_max_infinity<Type>::value >::value();
    }
};


//------------------------------------------------------------------------------
template<class Type, bool has_numeric_inf, bool has_repr_inf, bool has_size, bool has_diff>
struct get_infinity;

template<class Type, bool has_repr_inf, bool has_size, bool has_diff>
struct get_infinity<Type, true, has_repr_inf, has_size, has_diff>
{
    typedef get_infinity type;

    static Type value()
    {
        return  numeric_infinity<Type>::value();
    }
};

template<class Type, bool has_size, bool has_diff>
struct get_infinity<Type, false, true, has_size, has_diff>
{
    typedef get_infinity type;

    static Type value()
    {
        return Type(numeric_infinity<typename Type::rep>::value());
    }
};

template<class Type, bool has_diff>
struct get_infinity<Type, false, false, true, has_diff>
{
    typedef get_infinity type;
    typedef typename Type::size_type size_type;

    static Type value()
    {
        return Type(numeric_infinity<size_type>::value());
    }
};

template<class Type>
struct get_infinity<Type, false, false, false, true>
{
    typedef get_infinity type;
    typedef typename Type::difference_type difference_type;

    static Type value()
    {
        return identity_element<difference_type>::value();
    }
};

template<class Type>
struct get_infinity<Type, false, false, false, false>
{
    typedef get_infinity type;

    static Type value()
    {
        return identity_element<Type>::value();
    }
};

template <class Type> struct infinity
{
    typedef infinity type;

    static Type value()
    {
        return
            get_infinity< Type
                        , is_numeric<Type>::value
                        , has_rep_type<Type>::value
                        , has_size_type<Type>::value
                        , has_difference_type<Type>::value
                        >::value();
    }
};

template <> 
struct infinity<std::string>
{
    typedef infinity type;

    static std::string value()
    {
        return std::string();
    }
};

}} // namespace boost icl

#endif



// Copyright (c) 2009-2020 Vladimir Batov.
// Use, modification and distribution are subject to the Boost Software License,
// Version 1.0. See http://www.boost.org/LICENSE_1_0.txt.

#ifndef BOOST_CONVERT_IS_CONVERTER_HPP
#define BOOST_CONVERT_IS_CONVERTER_HPP

#include <boost/convert/detail/config.hpp>
#include <boost/convert/detail/is_callable.hpp>
#include <boost/type_traits/function_traits.hpp>
#include <boost/core/ref.hpp>

namespace boost { namespace cnv
{
    template<typename, typename, typename, typename =void>
    struct is_cnv { BOOST_STATIC_CONSTANT(bool, value = false); };

    template<typename Class, typename TypeIn, typename TypeOut>
    struct is_cnv<Class, TypeIn, TypeOut, typename std::enable_if<is_class<Class>::value, void>::type>
    {
        typedef typename ::boost::unwrap_reference<Class>::type class_type;
        typedef void signature_type(TypeIn const&, optional<TypeOut>&);

        BOOST_DECLARE_IS_CALLABLE(is_callable, operator());

        BOOST_STATIC_CONSTANT(bool, value = (is_callable<class_type, signature_type>::value));
    };

    template<typename Function, typename TypeIn, typename TypeOut>
    struct is_cnv<Function, TypeIn, TypeOut,
        typename enable_if_c<is_function<Function>::value && function_types::function_arity<Function>::value == 2,
        void>::type>
    {
        using       in_type = TypeIn;
        using      out_type = optional<TypeOut>&;
        using  func_in_type = typename function_traits<Function>::arg1_type;
        using func_out_type = typename function_traits<Function>::arg2_type;

        BOOST_STATIC_CONSTANT(bool,  in_good = (is_convertible<in_type, func_in_type>::value));
        BOOST_STATIC_CONSTANT(bool, out_good = (is_same<out_type, func_out_type>::value));
        BOOST_STATIC_CONSTANT(bool,    value = (in_good && out_good));
    };
}}

#endif // BOOST_CONVERT_IS_CONVERTER_HPP


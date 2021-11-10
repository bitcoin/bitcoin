// Copyright 2016 Klemens Morgenstern
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef BOOST_DLL_DETAIL_GET_MEM_FN_TYPE_HPP_
#define BOOST_DLL_DETAIL_GET_MEM_FN_TYPE_HPP_

namespace boost { namespace dll { namespace detail {

template<typename Class, typename Func>
struct get_mem_fn_type;

template<typename Class, typename Return, typename ...Args>
struct get_mem_fn_type<Class, Return(Args...)> {
    typedef Return (Class::*mem_fn)(Args...);
};

template<typename Class, typename Return, typename ...Args>
struct get_mem_fn_type<const Class, Return(Args...)> {
    typedef Return (Class::*mem_fn)(Args...) const ;
};

template<typename Class, typename Return, typename ...Args>
struct get_mem_fn_type<volatile Class, Return(Args...)> {
    typedef Return (Class::*mem_fn)(Args...) volatile;
};

template<typename Class, typename Return, typename ...Args>
struct get_mem_fn_type<const volatile Class, Return(Args...)> {
    typedef Return (Class::*mem_fn)(Args...) const volatile ;
};

}}} // namespace boost::dll::detail


#endif /* BOOST_DLL_SMART_LIBRARY_HPP_ */

// Boost.TypeErasure library
//
// Copyright 2015 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_DETAIL_DYNAMIC_VTABLE_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_DETAIL_DYNAMIC_VTABLE_HPP_INCLUDED

#include <boost/type_erasure/detail/get_placeholders.hpp>
#include <boost/type_erasure/detail/rebind_placeholders.hpp>
#include <boost/type_erasure/detail/normalize.hpp>
#include <boost/type_erasure/detail/adapt_to_vtable.hpp>
#include <boost/type_erasure/detail/vtable.hpp>
#include <boost/type_erasure/static_binding.hpp>
#include <boost/type_erasure/register_binding.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/set.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/index_of.hpp>
#include <typeinfo>

namespace boost {
namespace type_erasure {
namespace detail {

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && !defined(BOOST_NO_CXX11_CONSTEXPR) && !defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS)

template<class P>
struct dynamic_binding_impl {
    const std::type_info * type;
    dynamic_binding_impl() = default;
    constexpr dynamic_binding_impl(const std::type_info * t) : type(t) {}
};

template<class T>
struct dynamic_binding_element {
    typedef const std::type_info * type;
};

template<class Table>
struct append_to_key {
    template<class P>
    void operator()(P) {
        key->push_back(static_cast<const dynamic_binding_impl<P>*>(table)->type);
    }
    const Table * table;
    key_type * key;
};

template<class... P>
struct dynamic_vtable : dynamic_binding_impl<P>... {
    dynamic_vtable() = default;
    constexpr dynamic_vtable(typename dynamic_binding_element<P>::type ...t) : dynamic_binding_impl<P>(t)... {}
    template<class F>
    typename F::type lookup(F*) const {
        key_type key;
#ifndef BOOST_TYPE_ERASURE_USE_MP11
        typedef typename ::boost::type_erasure::detail::get_placeholders<F, ::boost::mpl::set0<> >::type placeholders;
#else
        typedef typename ::boost::type_erasure::detail::get_placeholders<F, ::boost::mp11::mp_list<> >::type placeholders;
#endif
        typedef typename ::boost::mpl::fold<
            placeholders,
            ::boost::mpl::map0<>,
            ::boost::type_erasure::detail::counting_map_appender
        >::type placeholder_map;
        key.push_back(&typeid(typename ::boost::type_erasure::detail::rebind_placeholders<F, placeholder_map>::type));
        ::boost::mpl::for_each<placeholders>(append_to_key<dynamic_vtable>{this, &key});
        return reinterpret_cast<typename F::type>(lookup_function_impl(key));
    }
    template<class Bindings>
    void init() {
        *this = dynamic_vtable(&typeid(typename boost::mpl::at<Bindings, P>::type)...);
    }
    template<class Bindings, class Src>
    void convert_from(const Src& src) {
#ifndef BOOST_TYPE_ERASURE_USE_MP11
        *this = dynamic_vtable(
            (&src.lookup((::boost::type_erasure::typeid_<typename ::boost::mpl::at<Bindings, P>::type>*)0)())...);
#else
        *this = dynamic_vtable(
            (&src.lookup((::boost::type_erasure::typeid_< ::boost::mp11::mp_second< ::boost::mp11::mp_map_find<Bindings, P> > >*)0)())...);
#endif
    }
};

template<class L>
struct make_dynamic_vtable_impl;

template<class... P>
struct make_dynamic_vtable_impl<stored_arg_pack<P...> >
{
    typedef dynamic_vtable<P...> type;
};

template<class PlaceholderList>
struct make_dynamic_vtable
    : make_dynamic_vtable_impl<typename make_arg_pack<PlaceholderList>::type>
{};

#else

template<class Bindings>
struct dynamic_vtable_initializer
{
    dynamic_vtable_initializer(const std::type_info**& ptr) : types(&ptr) {}
    template<class P>
    void operator()(P)
    {
        *(*types)++ = &typeid(typename ::boost::mpl::at<Bindings, P>::type);
    }
    const ::std::type_info*** types;
};

template<class Placeholders>
struct dynamic_vtable
{
    const ::std::type_info * types[(::boost::mpl::size<Placeholders>::value)];
    struct append_to_key
    {
        append_to_key(const std::type_info * const * t, key_type* k) : types(t), key(k) {}
        template<class P>
        void operator()(P)
        {
            key->push_back(types[(::boost::mpl::index_of<Placeholders, P>::type::value)]);
        }
        const std::type_info * const * types;
        key_type * key;
    };
    template<class F>
    typename F::type lookup(F*) const {
        key_type key;
        typedef typename ::boost::type_erasure::detail::get_placeholders<F, ::boost::mpl::set0<> >::type placeholders;
        typedef typename ::boost::mpl::fold<
            placeholders,
            ::boost::mpl::map0<>,
            ::boost::type_erasure::detail::counting_map_appender
        >::type placeholder_map;
        key.push_back(&typeid(typename ::boost::type_erasure::detail::rebind_placeholders<F, placeholder_map>::type));
        ::boost::mpl::for_each<placeholders>(append_to_key(types, &key));
        return reinterpret_cast<typename F::type>(lookup_function_impl(key));
    }
    template<class Bindings>
    void init()
    {
        const std::type_info* ptr = types;
        ::boost::mpl::for_each<Placeholders>(dynamic_vtable_initializer<Bindings>(ptr));
    }
    template<class Bindings, class Src>
    struct converter
    {
        converter(const std::type_info**& t, const Src& s) : types(&t), src(&s) {}
        template<class P>
        void operator()(P)
        {
            *(*types)++ = &src->lookup((::boost::type_erasure::typeid_<typename ::boost::mpl::at<Bindings, P>::type>*)0)();
        }
        const std::type_info*** types;
        const Src * src;
    };
    template<class Bindings, class Src>
    void convert_from(const Src& src) {
        const ::std::type_info** ptr = types;
        ::boost::mpl::for_each<Placeholders>(converter<Bindings, Src>(ptr, src));
    }
};

template<class Placeholders>
struct make_dynamic_vtable
{
    typedef dynamic_vtable<Placeholders> type;
};

#endif

}
}
}

#endif

// Boost.TypeErasure library
//
// Copyright 2015 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_REGISTER_BINDING_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_REGISTER_BINDING_HPP_INCLUDED

#include <boost/type_erasure/detail/check_map.hpp>
#include <boost/type_erasure/detail/get_placeholders.hpp>
#include <boost/type_erasure/detail/rebind_placeholders.hpp>
#include <boost/type_erasure/detail/normalize.hpp>
#include <boost/type_erasure/detail/adapt_to_vtable.hpp>
#include <boost/type_erasure/detail/auto_link.hpp>
#include <boost/type_erasure/static_binding.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/remove_if.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/has_key.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/set.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/pair.hpp>
#include <boost/mpl/back_inserter.hpp>
#include <boost/mpl/for_each.hpp>
#include <vector>
#include <typeinfo>

namespace boost {
namespace type_erasure {
namespace detail {

typedef std::vector<const std::type_info*> key_type;
typedef void (*value_type)();
BOOST_TYPE_ERASURE_DECL void register_function_impl(const key_type& key, value_type fn);
BOOST_TYPE_ERASURE_DECL value_type lookup_function_impl(const key_type& key);

template<class Map>
struct append_to_key_static {
    append_to_key_static(key_type* k) : key(k) {} 
    template<class P>
    void operator()(P) {
#ifndef BOOST_TYPE_ERASURE_USE_MP11
        key->push_back(&typeid(typename ::boost::mpl::at<Map, P>::type));
#else
        key->push_back(&typeid(::boost::mp11::mp_second< ::boost::mp11::mp_map_find<Map, P> >));
#endif
    }
    key_type* key;
};

// This placeholder exists solely to create a normalized
// representation of a primitive concept.  For the moment
// I'm going to be conservative and require a bijection
// between the original placeholders and the normalized
// placeholders.  It should be safe to map everything
// to a single placeholder, though, as long as the
// key includes every instance of each placeholder
// as a separate element.  i.e. we should be able to
// turn addable<_a, _b> into addable<_, _> and
// addable<_a, _a> into addable<_, _> as well if we always
// add typeids for both arguments to the search key.
template<int N>
struct _ : ::boost::type_erasure::placeholder {};

struct counting_map_appender
{
    template<class State, class Key>
    struct apply
    {
        typedef typename ::boost::mpl::insert<
            State,
            ::boost::mpl::pair<
                Key,
                ::boost::type_erasure::detail::_<
                    ::boost::mpl::size<State>::value
                >
            >
        >::type type;
    };
};

template<class Map>
struct register_function {
    template<class F>
    void operator()(F) {
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
        ::boost::mpl::for_each<placeholders>(append_to_key_static<Map>(&key));
        value_type fn = reinterpret_cast<value_type>(&::boost::type_erasure::detail::rebind_placeholders<F, Map>::type::value);
        ::boost::type_erasure::detail::register_function_impl(key, fn);
    }
};

}

/**
 * Registers a model of a concept to allow downcasting @ref any
 * via \dynamic_any_cast.
 */
template<class Concept, class Map>
void register_binding(const static_binding<Map>&)
{
    typedef typename ::boost::type_erasure::detail::normalize_concept<
        Concept
    >::type normalized;
    typedef typename ::boost::mpl::transform<normalized,
        ::boost::type_erasure::detail::maybe_adapt_to_vtable< ::boost::mpl::_1>
    >::type actual_concept;
    typedef typename ::boost::type_erasure::detail::get_placeholder_normalization_map<
        Concept
    >::type placeholder_subs;
    typedef typename ::boost::type_erasure::detail::add_deductions<Map, placeholder_subs>::type actual_map;
    ::boost::mpl::for_each<actual_concept>(::boost::type_erasure::detail::register_function<actual_map>());
}

/**
 * \overload
 */
template<class Concept, class T>
void register_binding()
{
    // Find all placeholders
    typedef typename ::boost::type_erasure::detail::normalize_concept_impl<Concept>::type normalized;
    typedef typename normalized::first basic;
    typedef typename ::boost::mpl::fold<
        basic,
#ifndef BOOST_TYPE_ERASURE_USE_MP11
        ::boost::mpl::set0<>,
#else
        ::boost::mp11::mp_list<>,
#endif
        ::boost::type_erasure::detail::get_placeholders< ::boost::mpl::_2, ::boost::mpl::_1>
    >::type all_placeholders;
    // remove deduced placeholders
    typedef typename ::boost::mpl::fold<
        typename normalized::second,
        ::boost::mpl::set0<>,
        ::boost::mpl::insert< ::boost::mpl::_1, ::boost::mpl::second< ::boost::mpl::_2> >
    >::type xtra_deduced;
    typedef typename ::boost::mpl::remove_if<
        all_placeholders,
        ::boost::mpl::or_<
            ::boost::type_erasure::detail::is_deduced< ::boost::mpl::_1>,
            ::boost::mpl::has_key<xtra_deduced, ::boost::mpl::_1>
        >,
        ::boost::mpl::back_inserter< ::boost::mpl::vector0<> >
    >::type unknown_placeholders;
    // Bind the single remaining placeholder to T
    BOOST_MPL_ASSERT((boost::mpl::equal_to<boost::mpl::size<unknown_placeholders>, boost::mpl::int_<1> >));
    register_binding<Concept>(::boost::type_erasure::make_binding<
        ::boost::mpl::map< ::boost::mpl::pair<typename ::boost::mpl::front<unknown_placeholders>::type, T> > >());
}

}
}

#endif

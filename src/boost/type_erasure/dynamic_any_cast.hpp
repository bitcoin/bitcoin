// Boost.TypeErasure library
//
// Copyright 2015 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_DYNAMIC_ANY_CAST_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_DYNAMIC_ANY_CAST_HPP_INCLUDED

#include <boost/type_erasure/detail/normalize.hpp>
#include <boost/type_erasure/binding_of.hpp>
#include <boost/type_erasure/static_binding.hpp>
#include <boost/type_erasure/dynamic_binding.hpp>
#include <boost/type_erasure/concept_of.hpp>
#include <boost/type_erasure/placeholder_of.hpp>
#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/binding.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/set.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/remove_reference.hpp>

namespace boost {
namespace type_erasure {

namespace detail {

template<class P, class P2, class Any>
struct make_ref_placeholder;

template<class P, class P2, class Any>
struct make_ref_placeholder<P, P2, const Any&> { typedef const P& type; };
template<class P, class P2, class Any>
struct make_ref_placeholder<P, P2, Any&> { typedef P& type; };
template<class P, class P2, class Any>
struct make_ref_placeholder<P, P2&, const Any&> { typedef P& type; };
template<class P, class P2, class Any>
struct make_ref_placeholder<P, P2&, Any&> { typedef P& type; };
template<class P, class P2, class Any>
struct make_ref_placeholder<P, const P2&, const Any&> { typedef const P& type; };
template<class P, class P2, class Any>
struct make_ref_placeholder<P, const P2&, Any&> { typedef const P& type; };

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
template<class P, class P2, class Any>
struct make_ref_placeholder { typedef P&& type; };
template<class P, class P2, class Any>
struct make_ref_placeholder<P, P2&, Any> { typedef P& type; };
template<class P, class P2, class Any>
struct make_ref_placeholder<P, const P2&, Any> { typedef const P& type; };
template<class P, class P2, class Any>
struct make_ref_placeholder<P, P2&&, Any> { typedef P&& type; };
template<class P, class P2, class Any>
struct make_ref_placeholder<P, P2&&, const Any&> { typedef const P& type; };
template<class P, class P2, class Any>
struct make_ref_placeholder<P, P2&&, Any&> { typedef P& type; };
#endif

template<class R, class Tag>
struct make_result_placeholder_map
{
    typedef ::boost::mpl::map<
        ::boost::mpl::pair<
            typename ::boost::remove_const<
                typename ::boost::remove_reference<
                    typename ::boost::type_erasure::placeholder_of<R>::type
                >::type
            >::type,
            typename ::boost::remove_const<
                typename ::boost::remove_reference<
                    Tag
                >::type
            >::type
        >
    > type;
};

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
template<class R, class Any, class Map>
R dynamic_any_cast_impl(Any&& arg, const static_binding<Map>& map)
#else
template<class R, class Any, class Map>
R dynamic_any_cast_impl(Any& arg, const static_binding<Map>& map)
#endif
{
    typedef typename ::boost::remove_const<typename ::boost::remove_reference<Any>::type>::type src_type;
    typedef typename ::boost::type_erasure::detail::normalize_concept<
        typename ::boost::type_erasure::concept_of<src_type>::type
    >::type normalized;
    typedef typename ::boost::mpl::fold<
        normalized,
#ifndef BOOST_TYPE_ERASURE_USE_MP11
        ::boost::mpl::set0<>,
#else
        ::boost::mp11::mp_list<>,
#endif
        ::boost::type_erasure::detail::get_placeholders<
            ::boost::mpl::_2,
            ::boost::mpl::_1
        >
    >::type placeholders;
#ifndef BOOST_TYPE_ERASURE_USE_MP11
    typedef ::boost::type_erasure::detail::substitution_map< ::boost::mpl::map0<> > identity_map;
#else
    typedef ::boost::type_erasure::detail::make_identity_placeholder_map<normalized> identity_map;
#endif
    ::boost::type_erasure::dynamic_binding<placeholders> my_binding(
        ::boost::type_erasure::binding_of(arg),
        ::boost::type_erasure::make_binding<identity_map>());
    typedef typename ::boost::remove_const<
        typename ::boost::remove_reference<
            typename ::boost::type_erasure::placeholder_of<R>::type
        >::type
    >::type result_placeholder;
    ::boost::type_erasure::binding< typename ::boost::type_erasure::concept_of<R>::type> new_binding(
        my_binding,
        map);
    typedef ::boost::type_erasure::any<
        typename ::boost::type_erasure::concept_of<R>::type,
        typename ::boost::type_erasure::detail::make_ref_placeholder<
            result_placeholder,
            typename ::boost::type_erasure::placeholder_of<src_type>::type,
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
            Any
#else
            Any&
#endif
        >::type
    > result_ref_type;
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    return result_ref_type(std::forward<Any>(arg), new_binding);
#else
    return result_ref_type(arg, new_binding);
#endif
}

}

#ifdef BOOST_TYPE_ERASURE_DOXYGEN

/**
 * Downcasts or crosscasts an @ref any.
 *
 * \pre @c R and @c Any must both be specializations of @ref any.
 * \pre PlaceholderMap must be an MPL map with a key
 *      for every non-deduced placeholder used by R.
 *      The value associated with each key should
 *      be the corresponding placeholder in Any.
 * \pre The concept of Any must include @ref typeid_, for every
 *      @ref placeholder which is used by R.
 * 
 * The single argument form can only be used when @c R uses
 * a single non-deduced placeholder.
 *
 * \throws bad_any_cast if the concepts used by R were
 *         not previously registered via a call to
 *         \register_binding.
 *
 * Example:
 * \code
 * // Assume that typeid_<>, copy_constructible<>, and incrementable<>
 * // have all been registered for int.
 * any<mpl::vector<typeid_<>, copy_constructible<> > > x(1);
 * typedef any<
 *     mpl::vector<
 *         typeid_<>,
 *         copy_constructible<>,
 *         incrementable<>
 *     >
 * > incrementable_any;
 * auto y = dynamic_any_cast<incrementable_any>(x);
 * ++y;
 * assert(any_cast<int>(y) == 2);
 * \endcode
 */
template<class R, class Any>
R dynamic_any_cast(Any&& arg);

/**
 * \overload
 */
template<class R, class Any, class Map>
R dynamic_any_cast(Any&& arg, const static_binding<Map>&);

#else

template<class R, class Concept, class Tag>
R dynamic_any_cast(const any<Concept, Tag>& arg)
{
    return ::boost::type_erasure::detail::dynamic_any_cast_impl<R>(arg,
        ::boost::type_erasure::make_binding<typename ::boost::type_erasure::detail::make_result_placeholder_map<R, Tag>::type>());
}

template<class R, class Concept, class Tag>
R dynamic_any_cast(any<Concept, Tag>& arg)
{
    return ::boost::type_erasure::detail::dynamic_any_cast_impl<R>(arg,
        ::boost::type_erasure::make_binding<typename ::boost::type_erasure::detail::make_result_placeholder_map<R, Tag>::type>());
}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
template<class R, class Concept, class Tag>
R dynamic_any_cast(any<Concept, Tag>&& arg)
{
    return ::boost::type_erasure::detail::dynamic_any_cast_impl<R>(::std::move(arg),
        ::boost::type_erasure::make_binding<typename ::boost::type_erasure::detail::make_result_placeholder_map<R, Tag>::type>());
}
#endif

template<class R, class Concept, class Tag, class Map>
R dynamic_any_cast(const any<Concept, Tag>& arg, const static_binding<Map>& map)
{
    return ::boost::type_erasure::detail::dynamic_any_cast_impl<R>(arg, map);
}

template<class R, class Concept, class Tag, class Map>
R dynamic_any_cast(any<Concept, Tag>& arg, const static_binding<Map>& map)
{
    return ::boost::type_erasure::detail::dynamic_any_cast_impl<R>(arg, map);
}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
template<class R, class Concept, class Tag, class Map>
R dynamic_any_cast(any<Concept, Tag>&& arg, const static_binding<Map>& map)
{
    return ::boost::type_erasure::detail::dynamic_any_cast_impl<R>(::std::move(arg), map);
}
#endif

#endif

}
}

#endif

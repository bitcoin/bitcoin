// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_DEDUCED_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_DEDUCED_HPP_INCLUDED

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/set.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/type_erasure/detail/get_placeholders.hpp>
#include <boost/type_erasure/placeholder.hpp>

namespace boost {
namespace type_erasure {

/**
 * A placeholder for an associated type.  The type corresponding
 * to this placeholder is deduced by substituting placeholders
 * in the arguments of the metafunction and then evaluating it.
 * 
 * When using @ref deduced in a template context, if it is possible for
 * Metafunction to contain no placeholders at all, use the nested type,
 * to automatically evaluate it early as needed.
 */
template<class Metafunction>
struct deduced : ::boost::type_erasure::placeholder
{
    typedef typename ::boost::mpl::eval_if<
        ::boost::mpl::empty<
            typename ::boost::type_erasure::detail::get_placeholders<
                Metafunction,
#ifndef BOOST_TYPE_ERASURE_USE_MP11
                ::boost::mpl::set0<>
#else
                ::boost::mp11::mp_list<>
#endif
            >::type
        >,
        Metafunction,
        ::boost::mpl::identity<
            ::boost::type_erasure::deduced<Metafunction>
        >
    >::type type;
};

}
}

#endif

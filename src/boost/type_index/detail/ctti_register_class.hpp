//
// Copyright 2013-2021 Antony Polukhin.
//
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_TYPE_INDEX_CTTI_REGISTER_CLASS_HPP
#define BOOST_TYPE_INDEX_CTTI_REGISTER_CLASS_HPP

/// \file ctti_register_class.hpp
/// \brief Contains BOOST_TYPE_INDEX_REGISTER_CLASS macro implementation that uses boost::typeindex::ctti_type_index.
/// Not intended for inclusion from user's code.

#include <boost/type_index/ctti_type_index.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
# pragma once
#endif

namespace boost { namespace typeindex { namespace detail {

template <class T>
inline const ctti_data& ctti_construct_typeid_ref(const T*) BOOST_NOEXCEPT {
    return ctti_construct<T>();
}

}}} // namespace boost::typeindex::detail

/// @cond
#define BOOST_TYPE_INDEX_REGISTER_CLASS                                                                             \
    virtual const boost::typeindex::detail::ctti_data& boost_type_index_type_id_runtime_() const BOOST_NOEXCEPT {   \
        return boost::typeindex::detail::ctti_construct_typeid_ref(this);                                           \
    }                                                                                                               \
/**/
/// @endcond

#endif // BOOST_TYPE_INDEX_CTTI_REGISTER_CLASS_HPP


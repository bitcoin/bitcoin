// ----------------------------------------------------------------------------
// Copyright (C) 2009 Sebastian Redl
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------

#ifndef BOOST_PROPERTY_TREE_ID_TRANSLATOR_HPP_INCLUDED
#define BOOST_PROPERTY_TREE_ID_TRANSLATOR_HPP_INCLUDED

#include <boost/property_tree/ptree_fwd.hpp>

#include <boost/optional.hpp>
#include <string>

namespace boost { namespace property_tree
{

    /// Simple implementation of the Translator concept. It does no translation.
    template <typename T>
    struct id_translator
    {
        typedef T internal_type;
        typedef T external_type;

        boost::optional<T> get_value(const T &v) { return v; }
        boost::optional<T> put_value(const T &v) { return v; }
    };

    // This is the default translator whenever you get two equal types.
    template <typename T>
    struct translator_between<T, T>
    {
        typedef id_translator<T> type;
    };

    // A more specific specialization for std::basic_string. Otherwise,
    // stream_translator's specialization wins.
    template <typename Ch, typename Traits, typename Alloc>
    struct translator_between< std::basic_string<Ch, Traits, Alloc>,
                               std::basic_string<Ch, Traits, Alloc> >
    {
        typedef id_translator< std::basic_string<Ch, Traits, Alloc> > type;
    };

}}

#endif

// ----------------------------------------------------------------------------
// Copyright (C) 2002-2006 Marcin Kalicinski
// Copyright (C) 2009 Sebastian Redl
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------
#ifndef BOOST_PROPERTY_TREE_PTREE_FWD_HPP_INCLUDED
#define BOOST_PROPERTY_TREE_PTREE_FWD_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/optional/optional_fwd.hpp>
#include <boost/throw_exception.hpp>
#include <functional>           // for std::less
#include <memory>               // for std::allocator
#include <string>

namespace boost { namespace property_tree
{
    namespace detail {
        template <typename T> struct less_nocase;
    }

    // Classes

    template < class Key, class Data, class KeyCompare = std::less<Key> >
    class basic_ptree;

    template <typename T>
    struct id_translator;

    template <typename String, typename Translator>
    class string_path;

    // Texas-style concepts for documentation only.
#if 0
    concept PropertyTreePath<class Path> {
        // The key type for which this path works.
        typename key_type;
        // Return the key that the first segment of the path names.
        // Split the head off the state.
        key_type Path::reduce();

        // Return true if the path is empty.
        bool Path::empty() const;

        // Return true if the path contains a single element.
        bool Path::single() const;

        // Dump as a std::string, for exception messages.
        std::string Path::dump() const;
    }
    concept PropertyTreeKey<class Key> {
        PropertyTreePath path;
        requires SameType<Key, PropertyTreePath<path>::key_type>;
    }
    concept PropertyTreeTranslator<class Tr> {
        typename internal_type;
        typename external_type;

        boost::optional<external_type> Tr::get_value(internal_type);
        boost::optional<internal_type> Tr::put_value(external_type);
    }
#endif
    /// If you want to use a custom key type, specialize this struct for it
    /// and give it a 'type' typedef that specifies your path type. The path
    /// type must conform to the Path concept described in the documentation.
    /// This is already specialized for std::basic_string.
    template <typename Key>
    struct path_of;

    /// Specialize this struct to specify a default translator between the data
    /// in a tree whose data_type is Internal, and the external data_type
    /// specified in a get_value, get, put_value or put operation.
    /// This is already specialized for Internal being std::basic_string.
    template <typename Internal, typename External>
    struct translator_between;

    class ptree_error;
    class ptree_bad_data;
    class ptree_bad_path;

    // Typedefs

    /** Implements a path using a std::string as the key. */
    typedef string_path<std::string, id_translator<std::string> > path;

    /**
     * A property tree with std::string for key and data, and default
     * comparison.
     */
    typedef basic_ptree<std::string, std::string> ptree;

    /**
     * A property tree with std::string for key and data, and case-insensitive
     * comparison.
     */
    typedef basic_ptree<std::string, std::string,
                        detail::less_nocase<std::string> >
        iptree;

#ifndef BOOST_NO_STD_WSTRING
    /** Implements a path using a std::wstring as the key. */
    typedef string_path<std::wstring, id_translator<std::wstring> > wpath;

    /**
     * A property tree with std::wstring for key and data, and default
     * comparison.
     * @note The type only exists if the platform supports @c wchar_t.
     */
    typedef basic_ptree<std::wstring, std::wstring> wptree;

    /**
     * A property tree with std::wstring for key and data, and case-insensitive
     * comparison.
     * @note The type only exists if the platform supports @c wchar_t.
     */
    typedef basic_ptree<std::wstring, std::wstring,
                        detail::less_nocase<std::wstring> >
        wiptree;
#endif

    // Free functions

    /**
     * Swap two property tree instances.
     */
    template<class K, class D, class C>
    void swap(basic_ptree<K, D, C> &pt1,
              basic_ptree<K, D, C> &pt2);

} }


#if !defined(BOOST_PROPERTY_TREE_DOXYGEN_INVOKED)
    // Throwing macro to avoid no return warnings portably
#   define BOOST_PROPERTY_TREE_THROW(e) BOOST_THROW_EXCEPTION(e)
#endif

#endif

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

#ifndef BOOST_PROPERTY_TREE_PTREE_HPP_INCLUDED
#define BOOST_PROPERTY_TREE_PTREE_HPP_INCLUDED

#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/property_tree/string_path.hpp>
#include <boost/property_tree/stream_translator.hpp>
#include <boost/property_tree/exceptions.hpp>
#include <boost/property_tree/detail/ptree_utils.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/indexed_by.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/throw_exception.hpp>
#include <boost/optional.hpp>
#include <utility>                  // for std::pair

namespace boost { namespace property_tree
{

    /**
     * Property tree main structure. A property tree is a hierarchical data
     * structure which has one element of type @p Data in each node, as well
     * as an ordered sequence of sub-nodes, which are additionally identified
     * by a non-unique key of type @p Key.
     *
     * Key equivalency is defined by @p KeyCompare, a predicate defining a
     * strict weak ordering.
     *
     * Property tree defines a Container-like interface to the (key-node) pairs
     * of its direct sub-nodes. The iterators are bidirectional. The sequence
     * of nodes is held in insertion order, not key order.
     */
    template<class Key, class Data, class KeyCompare>
    class basic_ptree
    {
#if defined(BOOST_PROPERTY_TREE_DOXYGEN_INVOKED)
    public:
#endif
        // Internal types
        /**
         * Simpler way to refer to this basic_ptree\<C,K,P,A\> type.
         * Note that this is private, and made public only for doxygen.
         */
        typedef basic_ptree<Key, Data, KeyCompare> self_type;

    public:
        // Basic types
        typedef Key                                  key_type;
        typedef Data                                 data_type;
        typedef KeyCompare                           key_compare;

        // Container view types
        typedef std::pair<const Key, self_type>      value_type;
        typedef std::size_t                          size_type;

        // The problem with the iterators is that I can't make them complete
        // until the container is complete. Sucks. Especially for the reverses.
        class iterator;
        class const_iterator;
        class reverse_iterator;
        class const_reverse_iterator;

        // Associative view types
        class assoc_iterator;
        class const_assoc_iterator;

        // Property tree view types
        typedef typename path_of<Key>::type          path_type;


        // The big five

        /** Creates a node with no children and default-constructed data. */
        basic_ptree();
        /** Creates a node with no children and a copy of the given data. */
        explicit basic_ptree(const data_type &data);
        basic_ptree(const self_type &rhs);
        ~basic_ptree();
        /** Basic guarantee only. */
        self_type &operator =(const self_type &rhs);

        /** Swap with other tree. Only constant-time and nothrow if the
         * data type's swap is.
         */
        void swap(self_type &rhs);

        // Container view functions

        /** The number of direct children of this node. */
        size_type size() const;
        size_type max_size() const;
        /** Whether there are any direct children. */
        bool empty() const;

        iterator begin();
        const_iterator begin() const;
        iterator end();
        const_iterator end() const;
        reverse_iterator rbegin();
        const_reverse_iterator rbegin() const;
        reverse_iterator rend();
        const_reverse_iterator rend() const;

        value_type &front();
        const value_type &front() const;
        value_type &back();
        const value_type &back() const;

        /** Insert a copy of the given tree with its key just before the given
         * position in this node. This operation invalidates no iterators.
         * @return An iterator to the newly created child.
         */
        iterator insert(iterator where, const value_type &value);

        /** Range insert. Equivalent to:
         * @code
         * for(; first != last; ++first) insert(where, *first);
         * @endcode
         */
        template<class It> void insert(iterator where, It first, It last);

        /** Erase the child pointed at by the iterator. This operation
         * invalidates the given iterator, as well as its equivalent
         * assoc_iterator.
         * @return A valid iterator pointing to the element after the erased.
         */
        iterator erase(iterator where);

        /** Range erase. Equivalent to:
         * @code
         * while(first != last;) first = erase(first);
         * @endcode
         */
        iterator erase(iterator first, iterator last);

        /** Equivalent to insert(begin(), value). */
        iterator push_front(const value_type &value);

        /** Equivalent to insert(end(), value). */
        iterator push_back(const value_type &value);

        /** Equivalent to erase(begin()). */
        void pop_front();

        /** Equivalent to erase(boost::prior(end())). */
        void pop_back();

        /** Reverses the order of direct children in the property tree. */
        void reverse();

        /** Sorts the direct children of this node according to the predicate.
         * The predicate is passed the whole pair of key and child.
         */
        template<class Compare> void sort(Compare comp);

        /** Sorts the direct children of this node according to key order. */
        void sort();

        // Equality

        /** Two property trees are the same if they have the same data, the keys
         * and order of their children are the same, and the children compare
         * equal, recursively.
         */
        bool operator ==(const self_type &rhs) const;
        bool operator !=(const self_type &rhs) const;

        // Associative view

        /** Returns an iterator to the first child, in key order. */
        assoc_iterator ordered_begin();
        /** Returns an iterator to the first child, in key order. */
        const_assoc_iterator ordered_begin() const;

        /** Returns the not-found iterator. Equivalent to end() in a real
         * associative container.
         */
        assoc_iterator not_found();
        /** Returns the not-found iterator. Equivalent to end() in a real
         * associative container.
         */
        const_assoc_iterator not_found() const;

        /** Find a child with the given key, or not_found() if there is none.
         * There is no guarantee about which child is returned if multiple have
         * the same key.
         */
        assoc_iterator find(const key_type &key);

        /** Find a child with the given key, or not_found() if there is none.
         * There is no guarantee about which child is returned if multiple have
         * the same key.
         */
        const_assoc_iterator find(const key_type &key) const;

        /** Find the range of children that have the given key. */
        std::pair<assoc_iterator, assoc_iterator>
            equal_range(const key_type &key);

        /** Find the range of children that have the given key. */
        std::pair<const_assoc_iterator, const_assoc_iterator>
            equal_range(const key_type &key) const;

        /** Count the number of direct children with the given key. */
        size_type count(const key_type &key) const;

        /** Erase all direct children with the given key and return the count.
         */
        size_type erase(const key_type &key);

        /** Get the iterator that points to the same element as the argument.
         * @note A valid assoc_iterator range (a, b) does not imply that
         *       (to_iterator(a), to_iterator(b)) is a valid range.
         */
        iterator to_iterator(assoc_iterator it);

        /** Get the iterator that points to the same element as the argument.
         * @note A valid const_assoc_iterator range (a, b) does not imply that
         *       (to_iterator(a), to_iterator(b)) is a valid range.
         */
        const_iterator to_iterator(const_assoc_iterator it) const;

        // Property tree view

        /** Reference to the actual data in this node. */
        data_type &data();

        /** Reference to the actual data in this node. */
        const data_type &data() const;

        /** Clear this tree completely, of both data and children. */
        void clear();

        /** Get the child at the given path, or throw @c ptree_bad_path.
         * @note Depending on the path, the result at each level may not be
         *       completely deterministic, i.e. if the same key appears multiple
         *       times, which child is chosen is not specified. This can lead
         *       to the path not being resolved even though there is a
         *       descendant with this path. Example:
         * @code
         *   a -> b -> c
         *     -> b
         * @endcode
         *       The path "a.b.c" will succeed if the resolution of "b" chooses
         *       the first such node, but fail if it chooses the second.
         */
        self_type &get_child(const path_type &path);

        /** Get the child at the given path, or throw @c ptree_bad_path. */
        const self_type &get_child(const path_type &path) const;

        /** Get the child at the given path, or return @p default_value. */
        self_type &get_child(const path_type &path, self_type &default_value);

        /** Get the child at the given path, or return @p default_value. */
        const self_type &get_child(const path_type &path,
                                   const self_type &default_value) const;

        /** Get the child at the given path, or return boost::null. */
        optional<self_type &> get_child_optional(const path_type &path);

        /** Get the child at the given path, or return boost::null. */
        optional<const self_type &>
          get_child_optional(const path_type &path) const;

        /** Set the node at the given path to the given value. Create any
         * missing parents. If the node at the path already exists, replace it.
         * @return A reference to the inserted subtree.
         * @note Because of the way paths work, it is not generally guaranteed
         *       that a node newly created can be accessed using the same path.
         * @note If the path could refer to multiple nodes, it is unspecified
         *       which one gets replaced.
         */
        self_type &put_child(const path_type &path, const self_type &value);

        /** Add the node at the given path. Create any missing parents. If there
         * already is a node at the path, add another one with the same key.
         * @param path Path to the child. The last fragment must not have an
         *             index.
         * @return A reference to the inserted subtree.
         * @note Because of the way paths work, it is not generally guaranteed
         *       that a node newly created can be accessed using the same path.
         */
        self_type &add_child(const path_type &path, const self_type &value);

        /** Take the value of this node and attempt to translate it to a
         * @c Type object using the supplied translator.
         * @throw ptree_bad_data if the conversion fails.
         */
        template<class Type, class Translator>
        typename boost::enable_if<detail::is_translator<Translator>, Type>::type
        get_value(Translator tr) const;

        /** Take the value of this node and attempt to translate it to a
         * @c Type object using the default translator.
         * @throw ptree_bad_data if the conversion fails.
         */
        template<class Type>
        Type get_value() const;

        /** Take the value of this node and attempt to translate it to a
         * @c Type object using the supplied translator. Return @p default_value
         * if this fails.
         */
        template<class Type, class Translator>
        Type get_value(const Type &default_value, Translator tr) const;

        /** Make get_value do the right thing for string literals. */
        template <class Ch, class Translator>
        typename boost::enable_if<
            detail::is_character<Ch>,
            std::basic_string<Ch>
        >::type
        get_value(const Ch *default_value, Translator tr) const;

        /** Take the value of this node and attempt to translate it to a
         * @c Type object using the default translator. Return @p default_value
         * if this fails.
         */
        template<class Type>
        typename boost::disable_if<detail::is_translator<Type>, Type>::type
        get_value(const Type &default_value) const;

        /** Make get_value do the right thing for string literals. */
        template <class Ch>
        typename boost::enable_if<
            detail::is_character<Ch>,
            std::basic_string<Ch>
        >::type
        get_value(const Ch *default_value) const;

        /** Take the value of this node and attempt to translate it to a
         * @c Type object using the supplied translator. Return boost::null if
         * this fails.
         */
        template<class Type, class Translator>
        optional<Type> get_value_optional(Translator tr) const;

        /** Take the value of this node and attempt to translate it to a
         * @c Type object using the default translator. Return boost::null if
         * this fails.
         */
        template<class Type>
        optional<Type> get_value_optional() const;

        /** Replace the value at this node with the given value, translated
         * to the tree's data type using the supplied translator.
         * @throw ptree_bad_data if the conversion fails.
        */
        template<class Type, class Translator>
        void put_value(const Type &value, Translator tr);

        /** Replace the value at this node with the given value, translated
         * to the tree's data type using the default translator.
         * @throw ptree_bad_data if the conversion fails.
        */
        template<class Type>
        void put_value(const Type &value);

        /** Shorthand for get_child(path).get_value(tr). */
        template<class Type, class Translator>
        typename boost::enable_if<detail::is_translator<Translator>, Type>::type
        get(const path_type &path, Translator tr) const;

        /** Shorthand for get_child(path).get_value\<Type\>(). */
        template<class Type>
        Type get(const path_type &path) const;

        /** Shorthand for get_child(path, empty_ptree())
         *                    .get_value(default_value, tr).
         * That is, return the translated value if possible, and the default
         * value if the node doesn't exist or conversion fails.
         */
        template<class Type, class Translator>
        Type get(const path_type &path,
                 const Type &default_value,
                 Translator tr) const;

        /** Make get do the right thing for string literals. */
        template <class Ch, class Translator>
        typename boost::enable_if<
            detail::is_character<Ch>,
            std::basic_string<Ch>
        >::type
        get(const path_type &path, const Ch *default_value, Translator tr)const;

        /** Shorthand for get_child(path, empty_ptree())
         *                    .get_value(default_value).
         * That is, return the translated value if possible, and the default
         * value if the node doesn't exist or conversion fails.
         */
        template<class Type>
        typename boost::disable_if<detail::is_translator<Type>, Type>::type
        get(const path_type &path, const Type &default_value) const;

        /** Make get do the right thing for string literals. */
        template <class Ch>
        typename boost::enable_if<
            detail::is_character<Ch>,
            std::basic_string<Ch>
        >::type
        get(const path_type &path, const Ch *default_value) const;

        /** Shorthand for:
         * @code
         * if(optional\<self_type&\> node = get_child_optional(path))
         *   return node->get_value_optional(tr);
         * return boost::null;
         * @endcode
         * That is, return the value if it exists and can be converted, or nil.
        */
        template<class Type, class Translator>
        optional<Type> get_optional(const path_type &path, Translator tr) const;

        /** Shorthand for:
         * @code
         * if(optional\<const self_type&\> node = get_child_optional(path))
         *   return node->get_value_optional();
         * return boost::null;
         * @endcode
         * That is, return the value if it exists and can be converted, or nil.
        */
        template<class Type>
        optional<Type> get_optional(const path_type &path) const;

        /** Set the value of the node at the given path to the supplied value,
         * translated to the tree's data type. If the node doesn't exist, it is
         * created, including all its missing parents.
         * @return The node that had its value changed.
         * @throw ptree_bad_data if the conversion fails.
        */
        template<class Type, class Translator>
        self_type &put(const path_type &path, const Type &value, Translator tr);

        /** Set the value of the node at the given path to the supplied value,
         * translated to the tree's data type. If the node doesn't exist, it is
         * created, including all its missing parents.
         * @return The node that had its value changed.
         * @throw ptree_bad_data if the conversion fails.
        */
        template<class Type>
        self_type &put(const path_type &path, const Type &value);

        /** If the node identified by the path does not exist, create it,
         * including all its missing parents.
         * If the node already exists, add a sibling with the same key.
         * Set the newly created node's value to the given paremeter,
         * translated with the supplied translator.
         * @param path Path to the child. The last fragment must not have an
         *             index.
         * @param value The value to add.
         * @param tr The translator to use.
         * @return The node that was added.
         * @throw ptree_bad_data if the conversion fails.
        */
        template<class Type, class Translator>
        self_type &add(const path_type &path,
                       const Type &value,
                       Translator tr);

        /** If the node identified by the path does not exist, create it,
         * including all its missing parents.
         * If the node already exists, add a sibling with the same key.
         * Set the newly created node's value to the given paremeter,
         * translated with the supplied translator.
         * @param path Path to the child. The last fragment must not have an
         *             index.
         * @param value The value to add.
         * @return The node that was added.
         * @throw ptree_bad_data if the conversion fails.
        */
        template<class Type>
        self_type &add(const path_type &path, const Type &value);

    private:
        // Hold the data of this node
        data_type m_data;
        // Hold the children - this is a void* because we can't complete the
        // container type within the class.
        void* m_children;

        // Getter tree-walk. Not const-safe! Gets the node the path refers to,
        // or null. Destroys p's value.
        self_type* walk_path(path_type& p) const;

        // Modifer tree-walk. Gets the parent of the node referred to by the
        // path, creating nodes as necessary. p is the path to the remaining
        // child.
        self_type& force_path(path_type& p);

        // This struct contains typedefs for the concrete types.
        struct subs;
        friend struct subs;
        friend class iterator;
        friend class const_iterator;
        friend class reverse_iterator;
        friend class const_reverse_iterator;
    };

}}

#include <boost/property_tree/detail/ptree_implementation.hpp>

#endif

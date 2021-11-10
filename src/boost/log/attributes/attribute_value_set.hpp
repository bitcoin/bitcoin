/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   attribute_value_set.hpp
 * \author Andrey Semashev
 * \date   21.04.2007
 *
 * This header file contains definition of attribute value set. The set is constructed from
 * three attribute sets (global, thread-specific and source-specific) and contains attribute
 * values.
 */

#ifndef BOOST_LOG_ATTRIBUTE_VALUE_SET_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTE_VALUE_SET_HPP_INCLUDED_

#include <cstddef>
#include <utility>
#include <iterator>
#include <boost/move/core.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/attributes/attribute_name.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/attribute_value.hpp>
#include <boost/log/attributes/attribute_set.hpp>
#include <boost/log/expressions/keyword_fwd.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

/*!
 * \brief A set of attribute values
 *
 * The set of attribute values is an associative container with attribute name as a key and
 * a pointer to attribute value object as a mapped type. This is a collection of elements with unique
 * keys, that is, there can be only one attribute value with a given name in the set. With respect to
 * read-only capabilities, the set interface is close to \c std::unordered_map.
 *
 * The set is designed to be only capable of adding elements to it. Once added, the attribute value
 * cannot be removed from the set.
 *
 * An instance of attribute value set can be constructed from three attribute sets. The constructor attempts to
 * accommodate values of all attributes from the sets. The situation when a same-named attribute is found
 * in more than one attribute set is possible. This problem is solved on construction of the value set: the three
 * attribute sets have different priorities when it comes to solving conflicts.
 *
 * From the library perspective the three source attribute sets are global, thread-specific and source-specific
 * attributes, with the latter having the highest priority. This feature allows to override attributes of wider scopes
 * with the more specific ones.
 *
 * For sake of performance, the attribute values are not immediately acquired from attribute sets at construction.
 * Instead, on-demand acquisition is performed either on iterator dereferencing or on call to the \c freeze method.
 * Once acquired, the attribute value stays within the set until its destruction. This nuance does not affect
 * other set properties, such as size or lookup ability. The logging core automatically freezes the set
 * at the right point, so users should not be bothered unless they manually create attribute value sets.
 *
 * \note The attribute sets that were used for the value set construction must not be modified or destroyed
 *       until the value set is frozen. Otherwise the behavior is undefined.
 */
class attribute_value_set
{
    BOOST_COPYABLE_AND_MOVABLE_ALT(attribute_value_set)

public:
    //! Key type
    typedef attribute_name key_type;
    //! Mapped attribute type
    typedef attribute_value mapped_type;

    //! Value type
    typedef std::pair< const key_type, mapped_type > value_type;
    //! Reference type
    typedef value_type& reference;
    //! Const reference type
    typedef value_type const& const_reference;
    //! Pointer type
    typedef value_type* pointer;
    //! Const pointer type
    typedef value_type const* const_pointer;
    //! Size type
    typedef std::size_t size_type;
    //! Pointer difference type
    typedef std::ptrdiff_t difference_type;

#ifndef BOOST_LOG_DOXYGEN_PASS

private:
    struct implementation;
    friend struct implementation;

    //! A base class for the container nodes
    struct node_base
    {
        node_base* m_pPrev;
        node_base* m_pNext;

        node_base();

        BOOST_DELETED_FUNCTION(node_base(node_base const&))
        BOOST_DELETED_FUNCTION(node_base& operator= (node_base const&))
    };

    //! Container elements
    struct node;
    friend struct node;
    struct node :
        public node_base
    {
        value_type m_Value;
        bool m_DynamicallyAllocated;

        node(key_type const& key, mapped_type& data, bool dynamic);
    };

public:
    class const_iterator;
    friend class const_iterator;
    class const_iterator
    {
    public:
        //  Standard typedefs
        typedef attribute_value_set::difference_type difference_type;
        typedef attribute_value_set::value_type value_type;
        typedef attribute_value_set::const_reference reference;
        typedef attribute_value_set::const_pointer pointer;
        typedef std::bidirectional_iterator_tag iterator_category;

    public:
        //  Constructors
        BOOST_CONSTEXPR const_iterator() : m_pNode(NULL), m_pContainer(NULL) {}
        explicit const_iterator(node_base* n, attribute_value_set* cont) BOOST_NOEXCEPT :
            m_pNode(n),
            m_pContainer(cont)
        {
        }

        //  Comparison
        bool operator== (const_iterator const& that) const BOOST_NOEXCEPT
        {
            return (m_pNode == that.m_pNode);
        }
        bool operator!= (const_iterator const& that) const BOOST_NOEXCEPT
        {
            return (m_pNode != that.m_pNode);
        }

        //  Modification
        const_iterator& operator++ ()
        {
            m_pContainer->freeze();
            m_pNode = m_pNode->m_pNext;
            return *this;
        }
        const_iterator& operator-- ()
        {
            m_pContainer->freeze();
            m_pNode = m_pNode->m_pPrev;
            return *this;
        }
        const_iterator operator++ (int)
        {
            const_iterator tmp(*this);
            m_pContainer->freeze();
            m_pNode = m_pNode->m_pNext;
            return tmp;
        }
        const_iterator operator-- (int)
        {
            const_iterator tmp(*this);
            m_pContainer->freeze();
            m_pNode = m_pNode->m_pPrev;
            return tmp;
        }

        //  Dereferencing
        pointer operator-> () const BOOST_NOEXCEPT { return &(static_cast< node* >(m_pNode)->m_Value); }
        reference operator* () const BOOST_NOEXCEPT { return static_cast< node* >(m_pNode)->m_Value; }

    private:
        node_base* m_pNode;
        attribute_value_set* m_pContainer;
    };

#else

    /*!
     * Constant iterator type with bidirectional capabilities.
     */
    typedef implementation_defined const_iterator;

#endif // BOOST_LOG_DOXYGEN_PASS

private:
    //! Pointer to the container implementation
    implementation* m_pImpl;

public:
    /*!
     * Default constructor
     *
     * The constructor creates an empty set which can be filled later by subsequent
     * calls of \c insert method. Optionally, the amount of storage reserved for elements
     * to be inserted may be passed to the constructor.
     * The constructed set is frozen.
     *
     * \param reserve_count Number of elements to reserve space for.
     */
    BOOST_LOG_API explicit attribute_value_set(size_type reserve_count = 8);

    /*!
     * Move constructor
     */
    attribute_value_set(BOOST_RV_REF(attribute_value_set) that) BOOST_NOEXCEPT : m_pImpl(that.m_pImpl)
    {
        that.m_pImpl = NULL;
    }

    /*!
     * The constructor adopts three attribute sets into the value set.
     * The \a source_attrs attributes have the greatest preference when a same-named
     * attribute is found in several sets, \a global_attrs has the least.
     * The constructed set is not frozen.
     *
     * \param source_attrs A set of source-specific attributes.
     * \param thread_attrs A set of thread-specific attributes.
     * \param global_attrs A set of global attributes.
     * \param reserve_count Amount of elements to reserve space for, in addition to the elements in the three attribute sets provided.
     */
    BOOST_LOG_API attribute_value_set(
        attribute_set const& source_attrs,
        attribute_set const& thread_attrs,
        attribute_set const& global_attrs,
        size_type reserve_count = 8);

    /*!
     * The constructor adopts three attribute sets into the value set.
     * The \a source_attrs attributes have the greatest preference when a same-named
     * attribute is found in several sets, \a global_attrs has the least.
     * The constructed set is not frozen.
     *
     * \pre The \a source_attrs set is frozen.
     *
     * \param source_attrs A set of source-specific attributes.
     * \param thread_attrs A set of thread-specific attributes.
     * \param global_attrs A set of global attributes.
     * \param reserve_count Amount of elements to reserve space for, in addition to the elements in the three attribute sets provided.
     */
    BOOST_LOG_API attribute_value_set(
        attribute_value_set const& source_attrs,
        attribute_set const& thread_attrs,
        attribute_set const& global_attrs,
        size_type reserve_count = 8);

    /*!
     * The constructor adopts three attribute sets into the value set.
     * The \a source_attrs attributes have the greatest preference when a same-named
     * attribute is found in several sets, \a global_attrs has the least.
     * The constructed set is not frozen.
     *
     * \pre The \a source_attrs set is frozen.
     *
     * \param source_attrs A set of source-specific attributes.
     * \param thread_attrs A set of thread-specific attributes.
     * \param global_attrs A set of global attributes.
     * \param reserve_count Amount of elements to reserve space for, in addition to the elements in the three attribute sets provided.
     */
    attribute_value_set(
        BOOST_RV_REF(attribute_value_set) source_attrs,
        attribute_set const& thread_attrs,
        attribute_set const& global_attrs,
        size_type reserve_count = 8) : m_pImpl(NULL)
    {
        construct(static_cast< attribute_value_set& >(source_attrs), thread_attrs, global_attrs, reserve_count);
    }

    /*!
     * Copy constructor.
     *
     * \pre The original set is frozen.
     * \post The constructed set is frozen, <tt>std::equal(begin(), end(), that.begin()) == true</tt>
     */
    BOOST_LOG_API attribute_value_set(attribute_value_set const& that);

    /*!
     * Destructor. Releases all referenced attribute values.
     */
    BOOST_LOG_API ~attribute_value_set() BOOST_NOEXCEPT;

    /*!
     * Assignment operator
     */
    attribute_value_set& operator= (attribute_value_set that) BOOST_NOEXCEPT
    {
        this->swap(that);
        return *this;
    }

    /*!
     * Swaps two sets
     *
     * \b Throws: Nothing.
     */
    void swap(attribute_value_set& that) BOOST_NOEXCEPT
    {
        implementation* const p = m_pImpl;
        m_pImpl = that.m_pImpl;
        that.m_pImpl = p;
    }

    /*!
     * \return Iterator to the first element of the set.
     */
    BOOST_LOG_API const_iterator begin() const;
    /*!
     * \return Iterator to the after-the-last element of the set.
     */
    BOOST_LOG_API const_iterator end() const;

    /*!
     * \return Number of elements in the set.
     */
    BOOST_LOG_API size_type size() const;
    /*!
     * \return \c true if there are no elements in the container, \c false otherwise.
     */
    bool empty() const { return (this->size() == 0); }

    /*!
     * The method finds the attribute value by name.
     *
     * \param key Attribute name.
     * \return Iterator to the found element or \c end() if the attribute with such name is not found.
     */
    BOOST_LOG_API const_iterator find(key_type key) const;

    /*!
     * Alternative lookup syntax.
     *
     * \param key Attribute name.
     * \return A pointer to the attribute value if it is found with \a key, default-constructed mapped value otherwise.
     */
    mapped_type operator[] (key_type key) const
    {
        const_iterator it = this->find(key);
        if (it != this->end())
            return it->second;
        else
            return mapped_type();
    }

    /*!
     * Alternative lookup syntax.
     *
     * \param keyword Attribute keyword.
     * \return A \c value_ref with extracted attribute value if it is found, empty \c value_ref otherwise.
     */
    template< typename DescriptorT, template< typename > class ActorT >
    typename result_of::extract< typename expressions::attribute_keyword< DescriptorT, ActorT >::value_type, DescriptorT >::type
    operator[] (expressions::attribute_keyword< DescriptorT, ActorT > const& keyword) const
    {
        typedef typename expressions::attribute_keyword< DescriptorT, ActorT >::value_type attr_value_type;
        typedef typename result_of::extract< attr_value_type, DescriptorT >::type result_type;
        const_iterator it = this->find(keyword.get_name());
        if (it != this->end())
            return it->second.extract< attr_value_type, DescriptorT >();
        else
            return result_type();
    }

    /*!
     * The method counts the number of the attribute value occurrences in the set. Since there can be only one
     * attribute value with a particular key, the method always return 0 or 1.
     *
     * \param key Attribute name.
     * \return The number of times the attribute value is found in the container.
     */
    size_type count(key_type key) const { return size_type(this->find(key) != this->end()); }

    /*!
     * The method acquires values of all adopted attributes.
     *
     * \post The set is frozen.
     */
    BOOST_LOG_API void freeze();

    /*!
     * Inserts an element into the set. The complexity of the operation is amortized constant.
     *
     * \pre The set is frozen.
     *
     * \param key The attribute name.
     * \param mapped The attribute value.
     *
     * \returns An iterator to the inserted element and \c true if insertion succeeded. Otherwise,
     *          if the set already contains a same-named attribute value, iterator to the
     *          existing element and \c false.
     */
    BOOST_LOG_API std::pair< const_iterator, bool > insert(key_type key, mapped_type const& mapped);

    /*!
     * Inserts an element into the set. The complexity of the operation is amortized constant.
     *
     * \pre The set is frozen.
     *
     * \param value The attribute name and value.
     *
     * \returns An iterator to the inserted element and \c true if insertion succeeded. Otherwise,
     *          if the set already contains a same-named attribute value, iterator to the
     *          existing element and \c false.
     */
    std::pair< const_iterator, bool > insert(const_reference value) { return this->insert(value.first, value.second); }

    /*!
     * Mass insertion method. The complexity of the operation is linear to the number of elements inserted.
     *
     * \pre The set is frozen.
     *
     * \param begin A forward iterator that points to the first element to be inserted.
     * \param end A forward iterator that points to the after-the-last element to be inserted.
     */
    template< typename FwdIteratorT >
    void insert(FwdIteratorT begin, FwdIteratorT end)
    {
        for (; begin != end; ++begin)
            this->insert(*begin);
    }

    /*!
     * Mass insertion method with ability to acquire iterators to the inserted elements.
     * The complexity of the operation is linear to the number of elements inserted times the complexity
     * of filling the \a out iterator.
     *
     * \pre The set is frozen.
     *
     * \param begin A forward iterator that points to the first element to be inserted.
     * \param end A forward iterator that points to the after-the-last element to be inserted.
     * \param out An output iterator that receives results of insertion of the elements.
     */
    template< typename FwdIteratorT, typename OutputIteratorT >
    void insert(FwdIteratorT begin, FwdIteratorT end, OutputIteratorT out)
    {
        for (; begin != end; ++begin, ++out)
            *out = this->insert(*begin);
    }

#ifndef BOOST_LOG_DOXYGEN_PASS
private:
    //! Constructs the object by moving from \a source_attrs. This function is mostly needed to maintain ABI stable between C++03 and C++11.
    BOOST_LOG_API void construct(
        attribute_value_set& source_attrs,
        attribute_set const& thread_attrs,
        attribute_set const& global_attrs,
        size_type reserve_count);
#endif // BOOST_LOG_DOXYGEN_PASS
};

/*!
 * Free swap overload
 */
inline void swap(attribute_value_set& left, attribute_value_set& right) BOOST_NOEXCEPT
{
    left.swap(right);
}

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_ATTRIBUTE_VALUE_SET_HPP_INCLUDED_

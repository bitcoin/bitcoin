// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file container_adaptor/associative_container_adaptor.hpp
/// \brief Container adaptor to build a type that is compliant to the concept of an associative container.

#ifndef BOOST_BIMAP_CONTAINER_ADAPTOR_ASSOCIATIVE_CONTAINER_ADAPTOR_HPP
#define BOOST_BIMAP_CONTAINER_ADAPTOR_ASSOCIATIVE_CONTAINER_ADAPTOR_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>

#include <utility>

#include <boost/mpl/vector.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/aux_/na.hpp>
#include <boost/bimap/container_adaptor/detail/identity_converters.hpp>
#include <boost/bimap/container_adaptor/container_adaptor.hpp>
#include <boost/call_traits.hpp>

namespace boost {
namespace bimaps {
namespace container_adaptor {

#ifndef BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES

template
<
    class Base, class Iterator, class ConstIterator, class KeyType,
    class IteratorToBaseConverter, class IteratorFromBaseConverter,
    class ValueToBaseConverter, class ValueFromBaseConverter, class KeyToBaseConverter,
    class FunctorsFromDerivedClasses
>
struct associative_container_adaptor_base
{
    typedef container_adaptor
    <
        Base,

        Iterator, ConstIterator,

        IteratorToBaseConverter, IteratorFromBaseConverter,
        ValueToBaseConverter   , ValueFromBaseConverter,

        BOOST_DEDUCED_TYPENAME mpl::push_front<

            FunctorsFromDerivedClasses,

            BOOST_DEDUCED_TYPENAME mpl::if_< ::boost::mpl::is_na<KeyToBaseConverter>,
            // {
                    detail::key_to_base_identity
                    <
                        BOOST_DEDUCED_TYPENAME Base::key_type, KeyType
                    >,
            // }
            // else
            // {
                    KeyToBaseConverter
            // }

            >::type

        >::type

    > type;
};

#endif // BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES


/// \brief Container adaptor to build a type that is compliant to the concept of an associative container.

template
<
    class Base,

    class Iterator,
    class ConstIterator,

    class KeyType,

    class IteratorToBaseConverter   = ::boost::mpl::na,
    class IteratorFromBaseConverter = ::boost::mpl::na,
    class ValueToBaseConverter      = ::boost::mpl::na,
    class ValueFromBaseConverter    = ::boost::mpl::na,
    class KeyToBaseConverter        = ::boost::mpl::na,

    class FunctorsFromDerivedClasses = mpl::vector<>
>
class associative_container_adaptor :

    public associative_container_adaptor_base
    <
        Base, Iterator, ConstIterator, KeyType,
        IteratorToBaseConverter, IteratorFromBaseConverter,
        ValueToBaseConverter, ValueFromBaseConverter, KeyToBaseConverter,
        FunctorsFromDerivedClasses

    >::type
{

    // MetaData -------------------------------------------------------------

    typedef typename associative_container_adaptor_base
    <
        Base, Iterator, ConstIterator, KeyType,
        IteratorToBaseConverter, IteratorFromBaseConverter,
        ValueToBaseConverter, ValueFromBaseConverter, KeyToBaseConverter,
        FunctorsFromDerivedClasses

    >::type base_;

    public:

    typedef KeyType key_type;

    protected:

    typedef BOOST_DEDUCED_TYPENAME mpl::if_< ::boost::mpl::is_na<KeyToBaseConverter>,
    // {
            detail::key_to_base_identity
            <
                BOOST_DEDUCED_TYPENAME Base::key_type, KeyType
            >,
    // }
    // else
    // {
            KeyToBaseConverter
    // }

    >::type key_to_base;

    public:

    explicit associative_container_adaptor(Base & c)
        : base_(c) {}

    protected:


    typedef associative_container_adaptor associative_container_adaptor_;

    // Interface --------------------------------------------------------------

    public:

    template< class CompatibleKey >
    BOOST_DEDUCED_TYPENAME base_::size_type erase(const CompatibleKey & k)
    {
        return this->base().erase
        (
            this->template functor<key_to_base>()(k)
        );
    }

    // As we redefine erase, the other overloads need to be manually routed

    BOOST_DEDUCED_TYPENAME base_::iterator erase(
        BOOST_DEDUCED_TYPENAME base_::iterator pos)
    {
        return base_::container_adaptor_::erase(pos);
    }

    BOOST_DEDUCED_TYPENAME base_::iterator erase(
        BOOST_DEDUCED_TYPENAME base_::iterator first,
        BOOST_DEDUCED_TYPENAME base_::iterator last)
    {
        return base_::container_adaptor_::erase(first,last);
    }

    template< class CompatibleKey >
    BOOST_DEDUCED_TYPENAME base_::size_type count(const CompatibleKey & k) const
    {
        return this->base().count(
            this->template functor<key_to_base>()(k)
        );
    }

    template< class CompatibleKey >
    BOOST_DEDUCED_TYPENAME base_::iterator find(const CompatibleKey & k)
    {
        return this->template functor<typename base_::iterator_from_base>()
        (
            this->base().find(
                this->template functor<key_to_base>()(k)
            )
        );
    }

    template< class CompatibleKey >
    BOOST_DEDUCED_TYPENAME base_::const_iterator
        find(const CompatibleKey & k) const
    {
        return this->template functor<
            BOOST_DEDUCED_TYPENAME base_::iterator_from_base>()
        (
            this->base().find(
                this->template functor<key_to_base>()(k)
            )
        );
    }

    template< class CompatibleKey >
    std::pair
    <
        BOOST_DEDUCED_TYPENAME base_::iterator,
        BOOST_DEDUCED_TYPENAME base_::iterator
    >
        equal_range(const CompatibleKey & k)
    {
        std::pair<

            BOOST_DEDUCED_TYPENAME Base::iterator,
            BOOST_DEDUCED_TYPENAME Base::iterator

        > r( this->base().equal_range(
                this->template functor<key_to_base>()(k)
            )
        );

        return std::pair
        <
            BOOST_DEDUCED_TYPENAME base_::iterator,
            BOOST_DEDUCED_TYPENAME base_::iterator
        >(
            this->template functor<
                BOOST_DEDUCED_TYPENAME base_::iterator_from_base
            >()                                         ( r.first ),
            this->template functor<
                BOOST_DEDUCED_TYPENAME base_::iterator_from_base
            >()                                         ( r.second )
        );
    }

    template< class CompatibleKey >
    std::pair
    <
        BOOST_DEDUCED_TYPENAME base_::const_iterator,
        BOOST_DEDUCED_TYPENAME base_::const_iterator
    >
        equal_range(const CompatibleKey & k) const
    {
        std::pair<

            BOOST_DEDUCED_TYPENAME Base::const_iterator,
            BOOST_DEDUCED_TYPENAME Base::const_iterator

        > r( this->base().equal_range(
                this->template functor<key_to_base>()(k)
            )
        );

        return std::pair
        <
            BOOST_DEDUCED_TYPENAME base_::const_iterator,
            BOOST_DEDUCED_TYPENAME base_::const_iterator
        >(
            this->template functor<
                BOOST_DEDUCED_TYPENAME base_::iterator_from_base
            >()                                         ( r.first ),
            this->template functor<
                BOOST_DEDUCED_TYPENAME base_::iterator_from_base
            >()                                         ( r.second )
        );
    }

};


} // namespace container_adaptor
} // namespace bimaps
} // namespace boost


#endif // BOOST_BIMAP_CONTAINER_ADAPTOR_ASSOCIATIVE_CONTAINER_ADAPTOR_HPP




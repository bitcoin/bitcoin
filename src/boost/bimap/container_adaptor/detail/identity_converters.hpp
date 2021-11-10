// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file container_adaptor/detail/identity_converters.hpp
/// \brief Value and iterators identity converters.

#ifndef BOOST_BIMAP_CONTAINER_ADAPTOR_DETAIL_IDENTITY_CONVERTERS_HPP
#define BOOST_BIMAP_CONTAINER_ADAPTOR_DETAIL_IDENTITY_CONVERTERS_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>

namespace boost {
namespace bimaps {
namespace container_adaptor {

/// \brief Details of the container adaptor toolbox

namespace detail {

/// \brief Iterator identity converter used by default in container adaptors.
/**
If Iterator and ConstIterator are of the same type one of the convert function is not
included.
                                                                                    **/

template
<
    class BaseIterator              , class Iterator,
    class BaseConstIterator         , class ConstIterator
>
struct iterator_to_base_identity
{
    BaseIterator operator()(Iterator iter) const
    {
        return BaseIterator(iter);
    }

    BaseConstIterator operator()(ConstIterator iter) const
    {
        return BaseConstIterator(iter);
    }
};

#ifndef BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES

template< class BaseIterator, class Iterator >
struct iterator_to_base_identity<BaseIterator,Iterator,BaseIterator,Iterator>
{
    BaseIterator operator()(Iterator iter) const
    {
        return BaseIterator(iter);
    }
};

#endif // BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES

/// \brief Iterator from base identity converter used by default in container adaptors.
/**
If Iterator and ConstIterator are of the same type one of the convert function is not
included.
                                                                                    **/

template
<
    class BaseIterator              , class Iterator,
    class BaseConstIterator         , class ConstIterator
>
struct iterator_from_base_identity
{
    Iterator operator()(BaseIterator iter) const
    {
        return Iterator(iter);
    }
    ConstIterator operator()(BaseConstIterator iter) const
    {
        return ConstIterator(iter);
    }
};

#ifndef BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES

template< class BaseIterator, class Iterator, class ConstIterator >
struct iterator_from_base_identity<BaseIterator,Iterator,BaseIterator,ConstIterator>
{
    Iterator operator()(BaseIterator iter) const
    {
        return Iterator(iter);
    }
};

#endif // BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES

/// \brief Value to base identity converter used by default in container adaptors.

template< class BaseValue, class Value >
struct value_to_base_identity
{
    BaseValue operator()(const Value & val) const
    {
        return BaseValue(val);
    }
};

#ifndef BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES

template< class Value >
struct value_to_base_identity< Value, Value >
{
    const Value & operator()(const Value & val) const
    {
        return val;
    }
};

#endif // BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES

/// \brief Value from base identity converter used by default in container adaptors.

template< class BaseValue, class Value >
struct value_from_base_identity
{
    Value operator()(const BaseValue & val) const
    {
        return Value(val);
    }
};

#ifndef BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES

template< class Value >
struct value_from_base_identity<Value,Value>
{
    Value & operator()(Value & val) const
    {
        return val;
    }

    const Value & operator()(const Value & val) const
    {
        return val;
    }
};

#endif // BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES

/// \brief Key to base identity converter used by default in container adaptors.

template< class BaseKey, class Key >
struct key_to_base_identity
{
    BaseKey operator()(const Key & k) const
    {
        return BaseKey(k);
    }
};

#ifndef BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES

template< class Key >
struct key_to_base_identity< Key, Key >
{
    // As default accept any type as key in order to allow container
    // adaptors to work with compatible key types

    template< class CompatibleKey >
    const CompatibleKey & operator()(const CompatibleKey & k) const
    {
        return k;
    }
};

#endif // BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES

} // namespace detail
} // namespace container_adaptor
} // namespace bimaps
} // namespace boost


#endif // BOOST_BIMAP_CONTAINER_ADAPTOR_DETAIL_IDENTITY_CONVERTERS_HPP



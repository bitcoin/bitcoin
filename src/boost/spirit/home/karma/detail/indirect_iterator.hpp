//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SPIRIT_KARMA_DETAIL_INDIRECT_ITERATOR_HPP
#define BOOST_SPIRIT_KARMA_DETAIL_INDIRECT_ITERATOR_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/unused.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <iterator> // for std::iterator_traits

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma { namespace detail
{
    ///////////////////////////////////////////////////////////////////////
    // This is a wrapper for any iterator allowing to pass a reference of it
    // to the components of the sequence
    template <typename Iterator>
    class indirect_iterator
      : public boost::iterator_facade<
            indirect_iterator<Iterator>
          , typename std::iterator_traits<Iterator>::value_type
          , boost::forward_traversal_tag
          , typename std::iterator_traits<Iterator>::reference>
    {
        typedef typename std::iterator_traits<Iterator>::value_type
            base_value_type;
        typedef typename std::iterator_traits<Iterator>::reference
            base_reference_type;

        typedef boost::iterator_facade<
            indirect_iterator<Iterator>, base_value_type
          , boost::forward_traversal_tag, base_reference_type
        > base_type;

    public:
        indirect_iterator(Iterator& iter)
          : iter_(&iter)
        {}
        indirect_iterator(indirect_iterator const& iter)
          : iter_(iter.iter_)
        {}

    private:
        friend class boost::iterator_core_access;

        void increment()
        {
            ++*iter_;
        }

        bool equal(indirect_iterator const& other) const
        {
            return *iter_ == *other.iter_;
        }

        base_reference_type dereference() const
        {
            return **iter_;
        }

    private:
        Iterator* iter_;
    };
}}}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace traits
{
    template <typename Iterator>
    struct make_indirect_iterator
    {
        typedef karma::detail::indirect_iterator<Iterator> type;
    };

    template <typename Iterator>
    struct make_indirect_iterator<karma::detail::indirect_iterator<Iterator> >
    {
        typedef karma::detail::indirect_iterator<Iterator> type;
    };

    template <>
    struct make_indirect_iterator<unused_type const*>
    {
        typedef unused_type const* type;
    };

    template <typename Iterator>
    struct make_indirect_iterator<Iterator const&>
      : make_indirect_iterator<Iterator const>
    {};
}}}

#endif

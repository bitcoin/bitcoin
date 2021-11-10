///////////////////////////////////////////////////////////////////////////////
// sub_match_vector.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_CORE_SUB_MATCH_VECTOR_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_CORE_SUB_MATCH_VECTOR_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/noncopyable.hpp>
#include <boost/iterator_adaptors.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/core/sub_match_impl.hpp>

namespace boost { namespace xpressive { namespace detail
{

#if BOOST_ITERATOR_ADAPTORS_VERSION >= 0x0200

//////////////////////////////////////////////////////////////////////////
// sub_match_iterator
//
template<typename Value, typename MainIter>
struct sub_match_iterator
  : iterator_adaptor
    <
        sub_match_iterator<Value, MainIter>
      , MainIter
      , Value
      , std::random_access_iterator_tag
    >
{
    typedef iterator_adaptor
    <
        sub_match_iterator<Value, MainIter>
      , MainIter
      , Value
      , std::random_access_iterator_tag
    > base_t;

    sub_match_iterator(MainIter baseiter)
      : base_t(baseiter)
    {
    }
};

#endif

//////////////////////////////////////////////////////////////////////////
// sub_match_vector
//
template<typename BidiIter>
struct sub_match_vector
  : private noncopyable
{
private:
    struct dummy { int i_; };
    typedef int dummy::*bool_type;

public:
    typedef sub_match<BidiIter> value_type;
    typedef std::size_t size_type;
    typedef value_type const &const_reference;
    typedef const_reference reference;
    typedef typename iterator_difference<BidiIter>::type difference_type;
    typedef typename iterator_value<BidiIter>::type char_type;
    typedef typename sub_match<BidiIter>::string_type string_type;

#if BOOST_ITERATOR_ADAPTORS_VERSION >= 0x0200

    typedef sub_match_iterator
    <
        value_type const
      , sub_match_impl<BidiIter> const *
    > const_iterator;

#else

    typedef iterator_adaptor
    <
        sub_match_impl<BidiIter> const *
      , default_iterator_policies
      , value_type
      , value_type const &
      , value_type const *
    > const_iterator;

#endif // BOOST_ITERATOR_ADAPTORS_VERSION < 0x0200

    typedef const_iterator iterator;

    sub_match_vector()
      : size_(0)
      , sub_matches_(0)
    {
    }

    const_reference operator [](size_type index) const
    {
        static value_type const s_null;
        return (index >= this->size_)
            ? s_null
            : *static_cast<value_type const *>(&this->sub_matches_[ index ]);
    }

    size_type size() const
    {
        return this->size_;
    }

    bool empty() const
    {
        return 0 == this->size();
    }

    const_iterator begin() const
    {
        return const_iterator(this->sub_matches_);
    }

    const_iterator end() const
    {
        return const_iterator(this->sub_matches_ + this->size_);
    }

    operator bool_type() const
    {
        return (!this->empty() && (*this)[0].matched) ? &dummy::i_ : 0;
    }

    bool operator !() const
    {
        return this->empty() || !(*this)[0].matched;
    }

    void swap(sub_match_vector<BidiIter> &that)
    {
        std::swap(this->size_, that.size_);
        std::swap(this->sub_matches_, that.sub_matches_);
    }

private:
    friend struct detail::core_access<BidiIter>;

    void init_(sub_match_impl<BidiIter> *sub_matches, size_type size)
    {
        this->size_ = size;
        this->sub_matches_ = sub_matches;
    }

    void init_(sub_match_impl<BidiIter> *sub_matches, size_type size, sub_match_vector<BidiIter> const &that)
    {
        BOOST_ASSERT(size == that.size_);
        this->size_ = size;
        this->sub_matches_ = sub_matches;
        std::copy(that.sub_matches_, that.sub_matches_ + that.size_, this->sub_matches_);
    }

    size_type size_;
    sub_match_impl<BidiIter> *sub_matches_;
};

}}} // namespace boost::xpressive::detail

#endif

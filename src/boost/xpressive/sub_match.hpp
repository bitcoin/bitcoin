///////////////////////////////////////////////////////////////////////////////
/// \file sub_match.hpp
/// Contains the definition of the class template sub_match\<\>
/// and associated helper functions
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_SUB_MATCH_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_SUB_MATCH_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <iosfwd>
#include <string>
#include <utility>
#include <iterator>
#include <algorithm>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/iterator/iterator_traits.hpp>
#include <boost/range/const_iterator.hpp>
#include <boost/range/mutable_iterator.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>

//{{AFX_DOC_COMMENT
///////////////////////////////////////////////////////////////////////////////
// This is a hack to get Doxygen to show the inheritance relation between
// sub_match<T> and std::pair<T,T>.
#ifdef BOOST_XPRESSIVE_DOXYGEN_INVOKED
/// INTERNAL ONLY
namespace std
{
    /// INTERNAL ONLY
    template<typename, typename> struct pair {};
}
#endif
//}}AFX_DOC_COMMENT

namespace boost { namespace xpressive
{

///////////////////////////////////////////////////////////////////////////////
// sub_match
//
/// \brief Class template \c sub_match denotes the sequence of characters matched by a particular
/// marked sub-expression.
///
/// When the marked sub-expression denoted by an object of type \c sub_match\<\> participated in a
/// regular expression match then member \c matched evaluates to \c true, and members \c first and \c second
/// denote the range of characters <tt>[first,second)</tt> which formed that match. Otherwise \c matched is \c false,
/// and members \c first and \c second contained undefined values.
///
/// If an object of type \c sub_match\<\> represents sub-expression 0 - that is to say the whole match -
/// then member \c matched is always \c true, unless a partial match was obtained as a result of the flag
/// \c match_partial being passed to a regular expression algorithm, in which case member \c matched is
/// \c false, and members \c first and \c second represent the character range that formed the partial match.
template<typename BidiIter>
struct sub_match
  : std::pair<BidiIter, BidiIter>
{
private:
    /// INTERNAL ONLY
    ///
    struct dummy { int i_; };
    typedef int dummy::*bool_type;

public:
    typedef typename iterator_value<BidiIter>::type value_type;
    typedef typename iterator_difference<BidiIter>::type difference_type;
    typedef typename detail::string_type<value_type>::type string_type;
    typedef BidiIter iterator;

    sub_match()
      : std::pair<BidiIter, BidiIter>()
      , matched(false)
    {
    }

    sub_match(BidiIter first, BidiIter second, bool matched_ = false)
      : std::pair<BidiIter, BidiIter>(first, second)
      , matched(matched_)
    {
    }

    string_type str() const
    {
        return this->matched ? string_type(this->first, this->second) : string_type();
    }

    operator string_type() const
    {
        return this->matched ? string_type(this->first, this->second) : string_type();
    }

    difference_type length() const
    {
        return this->matched ? std::distance(this->first, this->second) : 0;
    }

    operator bool_type() const
    {
        return this->matched ? &dummy::i_ : 0;
    }

    bool operator !() const
    {
        return !this->matched;
    }

    /// \brief Performs a lexicographic string comparison
    /// \param str the string against which to compare
    /// \return the results of <tt>(*this).str().compare(str)</tt>
    int compare(string_type const &str) const
    {
        return this->str().compare(str);
    }

    /// \overload
    ///
    int compare(sub_match const &sub) const
    {
        return this->str().compare(sub.str());
    }

    /// \overload
    ///
    int compare(value_type const *ptr) const
    {
        return this->str().compare(ptr);
    }

    /// \brief true if this sub-match participated in the full match.
    bool matched;
};

///////////////////////////////////////////////////////////////////////////////
/// \brief \c range_begin() to make \c sub_match\<\> a valid range
/// \param sub the \c sub_match\<\> object denoting the range
/// \return \c sub.first
/// \pre \c sub.first is not singular
template<typename BidiIter>
inline BidiIter range_begin(sub_match<BidiIter> &sub)
{
    return sub.first;
}

/// \overload
///
template<typename BidiIter>
inline BidiIter range_begin(sub_match<BidiIter> const &sub)
{
    return sub.first;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief \c range_end() to make \c sub_match\<\> a valid range
/// \param sub the \c sub_match\<\> object denoting the range
/// \return \c sub.second
/// \pre \c sub.second is not singular
template<typename BidiIter>
inline BidiIter range_end(sub_match<BidiIter> &sub)
{
    return sub.second;
}

/// \overload
///
template<typename BidiIter>
inline BidiIter range_end(sub_match<BidiIter> const &sub)
{
    return sub.second;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief insertion operator for sending sub-matches to ostreams
/// \param sout output stream.
/// \param sub sub_match object to be written to the stream.
/// \return sout \<\< sub.str()
template<typename BidiIter, typename Char, typename Traits>
inline std::basic_ostream<Char, Traits> &operator <<
(
    std::basic_ostream<Char, Traits> &sout
  , sub_match<BidiIter> const &sub
)
{
    typedef typename iterator_value<BidiIter>::type char_type;
    BOOST_MPL_ASSERT_MSG(
        (boost::is_same<Char, char_type>::value)
      , CHARACTER_TYPES_OF_STREAM_AND_SUB_MATCH_MUST_MATCH
      , (Char, char_type)
    );
    if(sub.matched)
    {
        std::ostream_iterator<char_type, Char, Traits> iout(sout);
        std::copy(sub.first, sub.second, iout);
    }
    return sout;
}


// BUGBUG make these more efficient

template<typename BidiIter>
bool operator == (sub_match<BidiIter> const &lhs, sub_match<BidiIter> const &rhs)
{
    return lhs.compare(rhs) == 0;
}

template<typename BidiIter>
bool operator != (sub_match<BidiIter> const &lhs, sub_match<BidiIter> const &rhs)
{
    return lhs.compare(rhs) != 0;
}

template<typename BidiIter>
bool operator < (sub_match<BidiIter> const &lhs, sub_match<BidiIter> const &rhs)
{
    return lhs.compare(rhs) < 0;
}

template<typename BidiIter>
bool operator <= (sub_match<BidiIter> const &lhs, sub_match<BidiIter> const &rhs)
{
    return lhs.compare(rhs) <= 0;
}

template<typename BidiIter>
bool operator >= (sub_match<BidiIter> const &lhs, sub_match<BidiIter> const &rhs)
{
    return lhs.compare(rhs) >= 0;
}

template<typename BidiIter>
bool operator > (sub_match<BidiIter> const &lhs, sub_match<BidiIter> const &rhs)
{
    return lhs.compare(rhs) > 0;
}

template<typename BidiIter>
bool operator == (typename iterator_value<BidiIter>::type const *lhs, sub_match<BidiIter> const &rhs)
{
    return lhs == rhs.str();
}

template<typename BidiIter>
bool operator != (typename iterator_value<BidiIter>::type const *lhs, sub_match<BidiIter> const &rhs)
{
    return lhs != rhs.str();
}

template<typename BidiIter>
bool operator < (typename iterator_value<BidiIter>::type const *lhs, sub_match<BidiIter> const &rhs)
{
    return lhs < rhs.str();
}

template<typename BidiIter>
bool operator > (typename iterator_value<BidiIter>::type const *lhs, sub_match<BidiIter> const &rhs)
{
    return lhs> rhs.str();
}

template<typename BidiIter>
bool operator >= (typename iterator_value<BidiIter>::type const *lhs, sub_match<BidiIter> const &rhs)
{
    return lhs >= rhs.str();
}

template<typename BidiIter>
bool operator <= (typename iterator_value<BidiIter>::type const *lhs, sub_match<BidiIter> const &rhs)
{
    return lhs <= rhs.str();
}

template<typename BidiIter>
bool operator == (sub_match<BidiIter> const &lhs, typename iterator_value<BidiIter>::type const *rhs)
{
    return lhs.str() == rhs;
}

template<typename BidiIter>
bool operator != (sub_match<BidiIter> const &lhs, typename iterator_value<BidiIter>::type const *rhs)
{
    return lhs.str() != rhs;
}

template<typename BidiIter>
bool operator < (sub_match<BidiIter> const &lhs, typename iterator_value<BidiIter>::type const *rhs)
{
    return lhs.str() < rhs;
}

template<typename BidiIter>
bool operator > (sub_match<BidiIter> const &lhs, typename iterator_value<BidiIter>::type const *rhs)
{
    return lhs.str() > rhs;
}

template<typename BidiIter>
bool operator >= (sub_match<BidiIter> const &lhs, typename iterator_value<BidiIter>::type const *rhs)
{
    return lhs.str() >= rhs;
}

template<typename BidiIter>
bool operator <= (sub_match<BidiIter> const &lhs, typename iterator_value<BidiIter>::type const *rhs)
{
    return lhs.str() <= rhs;
}

template<typename BidiIter>
bool operator == (typename iterator_value<BidiIter>::type const &lhs, sub_match<BidiIter> const &rhs)
{
    return lhs == rhs.str();
}

template<typename BidiIter>
bool operator != (typename iterator_value<BidiIter>::type const &lhs, sub_match<BidiIter> const &rhs)
{
    return lhs != rhs.str();
}

template<typename BidiIter>
bool operator < (typename iterator_value<BidiIter>::type const &lhs, sub_match<BidiIter> const &rhs)
{
    return lhs < rhs.str();
}

template<typename BidiIter>
bool operator > (typename iterator_value<BidiIter>::type const &lhs, sub_match<BidiIter> const &rhs)
{
    return lhs> rhs.str();
}

template<typename BidiIter>
bool operator >= (typename iterator_value<BidiIter>::type const &lhs, sub_match<BidiIter> const &rhs)
{
    return lhs >= rhs.str();
}

template<typename BidiIter>
bool operator <= (typename iterator_value<BidiIter>::type const &lhs, sub_match<BidiIter> const &rhs)
{
    return lhs <= rhs.str();
}

template<typename BidiIter>
bool operator == (sub_match<BidiIter> const &lhs, typename iterator_value<BidiIter>::type const &rhs)
{
    return lhs.str() == rhs;
}

template<typename BidiIter>
bool operator != (sub_match<BidiIter> const &lhs, typename iterator_value<BidiIter>::type const &rhs)
{
    return lhs.str() != rhs;
}

template<typename BidiIter>
bool operator < (sub_match<BidiIter> const &lhs, typename iterator_value<BidiIter>::type const &rhs)
{
    return lhs.str() < rhs;
}

template<typename BidiIter>
bool operator > (sub_match<BidiIter> const &lhs, typename iterator_value<BidiIter>::type const &rhs)
{
    return lhs.str() > rhs;
}

template<typename BidiIter>
bool operator >= (sub_match<BidiIter> const &lhs, typename iterator_value<BidiIter>::type const &rhs)
{
    return lhs.str() >= rhs;
}

template<typename BidiIter>
bool operator <= (sub_match<BidiIter> const &lhs, typename iterator_value<BidiIter>::type const &rhs)
{
    return lhs.str() <= rhs;
}

// Operator+ convenience function
template<typename BidiIter>
typename sub_match<BidiIter>::string_type
operator + (sub_match<BidiIter> const &lhs, sub_match<BidiIter> const &rhs)
{
    return lhs.str() + rhs.str();
}

template<typename BidiIter>
typename sub_match<BidiIter>::string_type
operator + (sub_match<BidiIter> const &lhs, typename iterator_value<BidiIter>::type const &rhs)
{
    return lhs.str() + rhs;
}

template<typename BidiIter>
typename sub_match<BidiIter>::string_type
operator + (typename iterator_value<BidiIter>::type const &lhs, sub_match<BidiIter> const &rhs)
{
    return lhs + rhs.str();
}

template<typename BidiIter>
typename sub_match<BidiIter>::string_type
operator + (sub_match<BidiIter> const &lhs, typename iterator_value<BidiIter>::type const *rhs)
{
    return lhs.str() + rhs;
}

template<typename BidiIter>
typename sub_match<BidiIter>::string_type
operator + (typename iterator_value<BidiIter>::type const *lhs, sub_match<BidiIter> const &rhs)
{
    return lhs + rhs.str();
}

template<typename BidiIter>
typename sub_match<BidiIter>::string_type
operator + (sub_match<BidiIter> const &lhs, typename sub_match<BidiIter>::string_type const &rhs)
{
    return lhs.str() + rhs;
}

template<typename BidiIter>
typename sub_match<BidiIter>::string_type
operator + (typename sub_match<BidiIter>::string_type const &lhs, sub_match<BidiIter> const &rhs)
{
    return lhs + rhs.str();
}

}} // namespace boost::xpressive

// Hook the Boost.Range customization points to make sub_match a valid range.
namespace boost
{
    /// INTERNAL ONLY
    ///
    template<typename BidiIter>
    struct range_mutable_iterator<xpressive::sub_match<BidiIter> >
    {
        typedef BidiIter type;
    };

    /// INTERNAL ONLY
    ///
    template<typename BidiIter>
    struct range_const_iterator<xpressive::sub_match<BidiIter> >
    {
        typedef BidiIter type;
    };
}

#endif

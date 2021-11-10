///////////////////////////////////////////////////////////////////////////////
// optimize.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_CORE_OPTIMIZE_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_CORE_OPTIMIZE_HPP_EAN_10_04_2005

#include <string>
#include <utility>
#include <boost/mpl/bool.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/iterator/iterator_traits.hpp>
#include <boost/xpressive/detail/core/finder.hpp>
#include <boost/xpressive/detail/core/linker.hpp>
#include <boost/xpressive/detail/core/peeker.hpp>
#include <boost/xpressive/detail/core/regex_impl.hpp>

namespace boost { namespace xpressive { namespace detail
{

///////////////////////////////////////////////////////////////////////////////
// optimize_regex
//
template<typename BidiIter, typename Traits>
intrusive_ptr<finder<BidiIter> > optimize_regex
(
    xpression_peeker<typename iterator_value<BidiIter>::type> const &peeker
  , Traits const &tr
  , mpl::false_
)
{
    if(peeker.line_start())
    {
        return intrusive_ptr<finder<BidiIter> >
        (
            new line_start_finder<BidiIter, Traits>(tr)
        );
    }
    else if(peeker.leading_simple_repeat())
    {
        return intrusive_ptr<finder<BidiIter> >
        (
            new leading_simple_repeat_finder<BidiIter>()
        );
    }
    else if(256 != peeker.bitset().count())
    {
        return intrusive_ptr<finder<BidiIter> >
        (
            new hash_peek_finder<BidiIter, Traits>(peeker.bitset())
        );
    }

    return intrusive_ptr<finder<BidiIter> >();
}

///////////////////////////////////////////////////////////////////////////////
// optimize_regex
//
template<typename BidiIter, typename Traits>
intrusive_ptr<finder<BidiIter> > optimize_regex
(
    xpression_peeker<typename iterator_value<BidiIter>::type> const &peeker
  , Traits const &tr
  , mpl::true_
)
{
    typedef typename iterator_value<BidiIter>::type char_type;

    // if we have a leading string literal, initialize a boyer-moore struct with it
    peeker_string<char_type> const &str = peeker.get_string();
    if(str.begin_ != str.end_)
    {
        BOOST_ASSERT(1 == peeker.bitset().count());
        return intrusive_ptr<finder<BidiIter> >
        (
            new boyer_moore_finder<BidiIter, Traits>(str.begin_, str.end_, tr, str.icase_)
        );
    }

    return optimize_regex<BidiIter>(peeker, tr, mpl::false_());
}

///////////////////////////////////////////////////////////////////////////////
// common_compile
//
template<typename BidiIter, typename Traits>
void common_compile
(
    intrusive_ptr<matchable_ex<BidiIter> const> const &regex
  , regex_impl<BidiIter> &impl
  , Traits const &tr
)
{
    typedef typename iterator_value<BidiIter>::type char_type;

    // "link" the regex
    xpression_linker<char_type> linker(tr);
    regex->link(linker);

    // "peek" into the compiled regex to see if there are optimization opportunities
    hash_peek_bitset<char_type> bset;
    xpression_peeker<char_type> peeker(bset, tr, linker.has_backrefs());
    regex->peek(peeker);

    // optimization: get the peek chars OR the boyer-moore search string
    impl.finder_ = optimize_regex<BidiIter>(peeker, tr, is_random<BidiIter>());
    impl.xpr_ = regex;
}

}}} // namespace boost::xpressive

#endif

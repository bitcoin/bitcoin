///////////////////////////////////////////////////////////////////////////////
// access.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_CORE_ACCESS_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_CORE_ACCESS_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/proto/traits.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/dynamic/matchable.hpp>
#include <boost/xpressive/match_results.hpp> // for type_info_less

namespace boost { namespace xpressive { namespace detail
{

///////////////////////////////////////////////////////////////////////////////
// core_access
//
template<typename BidiIter>
struct core_access
{
    typedef typename iterator_value<BidiIter>::type char_type;

    static std::size_t get_hidden_mark_count(basic_regex<BidiIter> const &rex)
    {
        return proto::value(rex)->hidden_mark_count_;
    }

    static bool match(basic_regex<BidiIter> const &rex, match_state<BidiIter> &state)
    {
        return rex.match_(state);
    }

    static shared_ptr<detail::regex_impl<BidiIter> > const &
    get_regex_impl(basic_regex<BidiIter> const &rex)
    {
        return proto::value(rex).get();
    }

    static void init_sub_match_vector
    (
        sub_match_vector<BidiIter> &subs_vect
      , sub_match_impl<BidiIter> *subs_ptr
      , std::size_t size
    )
    {
        subs_vect.init_(subs_ptr, size);
    }

    static void init_sub_match_vector
    (
        sub_match_vector<BidiIter> &subs_vect
      , sub_match_impl<BidiIter> *subs_ptr
      , std::size_t size
      , sub_match_vector<BidiIter> const &that
    )
    {
        subs_vect.init_(subs_ptr, size, that);
    }

    static void init_match_results
    (
        match_results<BidiIter> &what
      , regex_id_type regex_id
      , intrusive_ptr<traits<char_type> const> const &tr
      , sub_match_impl<BidiIter> *sub_matches
      , std::size_t size
      , std::vector<named_mark<char_type> > const &named_marks
    )
    {
        what.init_(regex_id, tr, sub_matches, size, named_marks);
    }

    static sub_match_vector<BidiIter> &get_sub_match_vector(match_results<BidiIter> &what)
    {
        return what.sub_matches_;
    }

    static sub_match_impl<BidiIter> *get_sub_matches(sub_match_vector<BidiIter> &subs)
    {
        return subs.sub_matches_;
    }

    static results_extras<BidiIter> &get_extras(match_results<BidiIter> &what)
    {
        return what.get_extras_();
    }

    static nested_results<BidiIter> &get_nested_results(match_results<BidiIter> &what)
    {
        return what.nested_results_;
    }

    static action_args_type &get_action_args(match_results<BidiIter> &what)
    {
        return what.args_;
    }

    static void set_prefix_suffix(match_results<BidiIter> &what, BidiIter begin, BidiIter end)
    {
        what.set_prefix_suffix_(begin, end);
    }

    static void reset(match_results<BidiIter> &what)
    {
        what.reset_();
    }

    static void set_base(match_results<BidiIter> &what, BidiIter base)
    {
        what.set_base_(base);
    }

    static BidiIter get_base(match_results<BidiIter> &what)
    {
        return *what.base_;
    }
};

}}} // namespace boost::xpressive::detail

#endif

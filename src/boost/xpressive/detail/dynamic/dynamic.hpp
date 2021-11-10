///////////////////////////////////////////////////////////////////////////////
// dynamic.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_DYNAMIC_DYNAMIC_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_DYNAMIC_DYNAMIC_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <vector>
#include <utility>
#include <algorithm>
#include <boost/assert.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/throw_exception.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/core/quant_style.hpp>
#include <boost/xpressive/detail/dynamic/matchable.hpp>
#include <boost/xpressive/detail/dynamic/sequence.hpp>
#include <boost/xpressive/detail/core/icase.hpp>

namespace boost { namespace xpressive { namespace detail
{

///////////////////////////////////////////////////////////////////////////////
// invalid_xpression
template<typename BidiIter>
struct invalid_xpression
  : matchable_ex<BidiIter>
{
    invalid_xpression()
      : matchable_ex<BidiIter>()
    {
        intrusive_ptr_add_ref(this); // keep alive forever
    }

    bool match(match_state<BidiIter> &) const
    {
        BOOST_ASSERT(false);
        return false;
    }
};

///////////////////////////////////////////////////////////////////////////////
// get_invalid_xpression
template<typename BidiIter>
inline shared_matchable<BidiIter> const &get_invalid_xpression()
{
    static invalid_xpression<BidiIter> const invalid_xpr;
    static intrusive_ptr<matchable_ex<BidiIter> const> const invalid_ptr(&invalid_xpr);
    static shared_matchable<BidiIter> const invalid_matchable(invalid_ptr);
    return invalid_matchable;
}

///////////////////////////////////////////////////////////////////////////////
// dynamic_xpression
template<typename Matcher, typename BidiIter>
struct dynamic_xpression
  : Matcher
  , matchable_ex<BidiIter>
{
    typedef typename iterator_value<BidiIter>::type char_type;

    dynamic_xpression(Matcher const &matcher = Matcher())
      : Matcher(matcher)
      , next_(get_invalid_xpression<BidiIter>())
    {
    }

    virtual bool match(match_state<BidiIter> &state) const
    {
        return this->Matcher::match(state, *this->next_.matchable());
    }

    virtual void link(xpression_linker<char_type> &linker) const
    {
        linker.accept(*static_cast<Matcher const *>(this), this->next_.matchable().get());
        this->next_.link(linker);
    }

    virtual void peek(xpression_peeker<char_type> &peeker) const
    {
        this->peek_next_(peeker.accept(*static_cast<Matcher const *>(this)), peeker);
    }

    virtual void repeat(quant_spec const &spec, sequence<BidiIter> &seq) const
    {
        this->repeat_(spec, seq, quant_type<Matcher>(), is_same<Matcher, mark_begin_matcher>());
    }

private:
    friend struct sequence<BidiIter>;

    void peek_next_(mpl::true_, xpression_peeker<char_type> &peeker) const
    {
        this->next_.peek(peeker);
    }

    void peek_next_(mpl::false_, xpression_peeker<char_type> &) const
    {
        // no-op
    }

    void repeat_(quant_spec const &spec, sequence<BidiIter> &seq, mpl::int_<quant_none>, mpl::false_) const
    {
        if(quant_none == seq.quant())
        {
            BOOST_THROW_EXCEPTION(
                regex_error(regex_constants::error_badrepeat, "expression cannot be quantified")
            );
        }
        else
        {
            this->repeat_(spec, seq, mpl::int_<quant_variable_width>(), mpl::false_());
        }
    }

    void repeat_(quant_spec const &spec, sequence<BidiIter> &seq, mpl::int_<quant_fixed_width>, mpl::false_) const
    {
        if(this->next_ == get_invalid_xpression<BidiIter>())
        {
            make_simple_repeat(spec, seq, matcher_wrapper<Matcher>(*this));
        }
        else
        {
            this->repeat_(spec, seq, mpl::int_<quant_variable_width>(), mpl::false_());
        }
    }

    void repeat_(quant_spec const &spec, sequence<BidiIter> &seq, mpl::int_<quant_variable_width>, mpl::false_) const
    {
        if(!is_unknown(seq.width()) && seq.pure())
        {
            make_simple_repeat(spec, seq);
        }
        else
        {
            make_repeat(spec, seq);
        }
    }

    void repeat_(quant_spec const &spec, sequence<BidiIter> &seq, mpl::int_<quant_fixed_width>, mpl::true_) const
    {
        make_repeat(spec, seq, this->mark_number_);
    }

    shared_matchable<BidiIter> next_;
};

///////////////////////////////////////////////////////////////////////////////
// make_dynamic
template<typename BidiIter, typename Matcher>
inline sequence<BidiIter> make_dynamic(Matcher const &matcher)
{
    typedef dynamic_xpression<Matcher, BidiIter> xpression_type;
    intrusive_ptr<xpression_type> xpr(new xpression_type(matcher));
    return sequence<BidiIter>(xpr);
}

///////////////////////////////////////////////////////////////////////////////
// alternates_vector
template<typename BidiIter>
struct alternates_vector
  : std::vector<shared_matchable<BidiIter> >
{
    BOOST_STATIC_CONSTANT(std::size_t, width = unknown_width::value);
    BOOST_STATIC_CONSTANT(bool, pure = false);
};

///////////////////////////////////////////////////////////////////////////////
// matcher_wrapper
template<typename Matcher>
struct matcher_wrapper
  : Matcher
{
    matcher_wrapper(Matcher const &matcher = Matcher())
      : Matcher(matcher)
    {
    }

    template<typename BidiIter>
    bool match(match_state<BidiIter> &state) const
    {
        return this->Matcher::match(state, matcher_wrapper<true_matcher>());
    }

    template<typename Char>
    void link(xpression_linker<Char> &linker) const
    {
        linker.accept(*static_cast<Matcher const *>(this), 0);
    }

    template<typename Char>
    void peek(xpression_peeker<Char> &peeker) const
    {
        peeker.accept(*static_cast<Matcher const *>(this));
    }
};

//////////////////////////////////////////////////////////////////////////
// make_simple_repeat
template<typename BidiIter, typename Xpr>
inline void
make_simple_repeat(quant_spec const &spec, sequence<BidiIter> &seq, Xpr const &xpr)
{
    if(spec.greedy_)
    {
        simple_repeat_matcher<Xpr, mpl::true_> quant(xpr, spec.min_, spec.max_, seq.width().value());
        seq = make_dynamic<BidiIter>(quant);
    }
    else
    {
        simple_repeat_matcher<Xpr, mpl::false_> quant(xpr, spec.min_, spec.max_, seq.width().value());
        seq = make_dynamic<BidiIter>(quant);
    }
}

//////////////////////////////////////////////////////////////////////////
// make_simple_repeat
template<typename BidiIter>
inline void
make_simple_repeat(quant_spec const &spec, sequence<BidiIter> &seq)
{
    seq += make_dynamic<BidiIter>(true_matcher());
    make_simple_repeat(spec, seq, seq.xpr());
}

//////////////////////////////////////////////////////////////////////////
// make_optional
template<typename BidiIter>
inline void
make_optional(quant_spec const &spec, sequence<BidiIter> &seq)
{
    typedef shared_matchable<BidiIter> xpr_type;
    seq += make_dynamic<BidiIter>(alternate_end_matcher());
    if(spec.greedy_)
    {
        optional_matcher<xpr_type, mpl::true_> opt(seq.xpr());
        seq = make_dynamic<BidiIter>(opt);
    }
    else
    {
        optional_matcher<xpr_type, mpl::false_> opt(seq.xpr());
        seq = make_dynamic<BidiIter>(opt);
    }
}

//////////////////////////////////////////////////////////////////////////
// make_optional
template<typename BidiIter>
inline void
make_optional(quant_spec const &spec, sequence<BidiIter> &seq, int mark_nbr)
{
    typedef shared_matchable<BidiIter> xpr_type;
    seq += make_dynamic<BidiIter>(alternate_end_matcher());
    if(spec.greedy_)
    {
        optional_mark_matcher<xpr_type, mpl::true_> opt(seq.xpr(), mark_nbr);
        seq = make_dynamic<BidiIter>(opt);
    }
    else
    {
        optional_mark_matcher<xpr_type, mpl::false_> opt(seq.xpr(), mark_nbr);
        seq = make_dynamic<BidiIter>(opt);
    }
}

//////////////////////////////////////////////////////////////////////////
// make_repeat
template<typename BidiIter>
inline void
make_repeat(quant_spec const &spec, sequence<BidiIter> &seq)
{
    // only bother creating a repeater if max is greater than one
    if(1 < spec.max_)
    {
        // create a hidden mark so this expression can be quantified
        int mark_nbr = -static_cast<int>(++*spec.hidden_mark_count_);
        seq = make_dynamic<BidiIter>(mark_begin_matcher(mark_nbr)) + seq
            + make_dynamic<BidiIter>(mark_end_matcher(mark_nbr));
        make_repeat(spec, seq, mark_nbr);
        return;
    }

    // if min is 0, the repeat must be made optional
    if(0 == spec.min_)
    {
        make_optional(spec, seq);
    }
}

//////////////////////////////////////////////////////////////////////////
// make_repeat
template<typename BidiIter>
inline void
make_repeat(quant_spec const &spec, sequence<BidiIter> &seq, int mark_nbr)
{
    BOOST_ASSERT(spec.max_); // we should never get here if max is 0

    // only bother creating a repeater if max is greater than one
    if(1 < spec.max_)
    {
        // TODO: statically bind the repeat matchers to the mark matchers for better perf
        unsigned int min = spec.min_ ? spec.min_ : 1U;
        repeat_begin_matcher repeat_begin(mark_nbr);
        if(spec.greedy_)
        {
            repeat_end_matcher<mpl::true_> repeat_end(mark_nbr, min, spec.max_);
            seq = make_dynamic<BidiIter>(repeat_begin) + seq
                + make_dynamic<BidiIter>(repeat_end);
        }
        else
        {
            repeat_end_matcher<mpl::false_> repeat_end(mark_nbr, min, spec.max_);
            seq = make_dynamic<BidiIter>(repeat_begin) + seq
                + make_dynamic<BidiIter>(repeat_end);
        }
    }

    // if min is 0, the repeat must be made optional
    if(0 == spec.min_)
    {
        make_optional(spec, seq, mark_nbr);
    }
}

}}} // namespace boost::xpressive::detail

#endif

///////////////////////////////////////////////////////////////////////////////
// static.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_STATIC_STATIC_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_STATIC_STATIC_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/mpl/assert.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/core/state.hpp>
#include <boost/xpressive/detail/core/linker.hpp>
#include <boost/xpressive/detail/core/peeker.hpp>
#include <boost/xpressive/detail/static/placeholders.hpp>
#include <boost/xpressive/detail/utility/width.hpp>

// Random thoughts:
// - must support indirect repeat counts {$n,$m}
// - add ws to eat whitespace (make *ws illegal)
// - a{n,m}    -> repeat<n,m>(a)
// - a{$n,$m}  -> repeat(n,m)(a)
// - add nil to match nothing
// - instead of s1, s2, etc., how about s[1], s[2], etc.? Needlessly verbose?

namespace boost { namespace xpressive { namespace detail
{

///////////////////////////////////////////////////////////////////////////////
// stacked_xpression
//
template<typename Top, typename Next>
struct stacked_xpression
  : Next
{
    // match
    //  delegates to Next
    template<typename BidiIter>
    bool match(match_state<BidiIter> &state) const
    {
        return static_cast<Next const *>(this)->
            BOOST_NESTED_TEMPLATE push_match<Top>(state);
    }

    // top_match
    //   jump back to the xpression on top of the xpression stack,
    //   and keep the xpression on the stack.
    template<typename BidiIter>
    static bool top_match(match_state<BidiIter> &state, void const *top)
    {
        return static_cast<Top const *>(top)->
            BOOST_NESTED_TEMPLATE push_match<Top>(state);
    }

    // pop_match
    //   jump back to the xpression on top of the xpression stack,
    //   pop the xpression off the stack.
    template<typename BidiIter>
    static bool pop_match(match_state<BidiIter> &state, void const *top)
    {
        return static_cast<Top const *>(top)->match(state);
    }

    // skip_match
    //   pop the xpression off the top of the stack and ignore it; call
    //   match on next.
    template<typename BidiIter>
    bool skip_match(match_state<BidiIter> &state) const
    {
        // could be static_xpression::skip_impl or stacked_xpression::skip_impl
        // depending on if there is 1 or more than 1 xpression on the
        // xpression stack
        return Top::skip_impl(*static_cast<Next const *>(this), state);
    }

//protected:

    // skip_impl
    //   implementation of skip_match.
    template<typename That, typename BidiIter>
    static bool skip_impl(That const &that, match_state<BidiIter> &state)
    {
        return that.BOOST_NESTED_TEMPLATE push_match<Top>(state);
    }
};

///////////////////////////////////////////////////////////////////////////////
// stacked_xpression_cast
//
template<typename Top, typename Next>
inline stacked_xpression<Top, Next> const &stacked_xpression_cast(Next const &next)
{
    // NOTE: this is a little white lie. The "next" object doesn't really have
    // the type to which we're casting it. It is harmless, though. We are only using
    // the cast to decorate the next object with type information. It is done
    // this way to save stack space.
    BOOST_MPL_ASSERT_RELATION(sizeof(stacked_xpression<Top, Next>), ==, sizeof(Next));
    return *static_cast<stacked_xpression<Top, Next> const *>(&next);
}

///////////////////////////////////////////////////////////////////////////////
// static_xpression
//
template<typename Matcher, typename Next>
struct static_xpression
  : Matcher
{
    Next next_;

    BOOST_STATIC_CONSTANT(bool, pure = Matcher::pure && Next::pure);
    BOOST_STATIC_CONSTANT(
        std::size_t
      , width =
            Matcher::width != unknown_width::value && Next::width != unknown_width::value
          ? Matcher::width + Next::width
          : unknown_width::value
    );

    static_xpression(Matcher const &matcher = Matcher(), Next const &next = Next())
      : Matcher(matcher)
      , next_(next)
    {
    }

    // match
    //  delegates to the Matcher
    template<typename BidiIter>
    bool match(match_state<BidiIter> &state) const
    {
        return this->Matcher::match(state, this->next_);
    }

    // push_match
    //   call match on this, but also push "Top" onto the xpression
    //   stack so we know what we are jumping back to later.
    template<typename Top, typename BidiIter>
    bool push_match(match_state<BidiIter> &state) const
    {
        return this->Matcher::match(state, stacked_xpression_cast<Top>(this->next_));
    }

    // skip_impl
    //   implementation of skip_match, called from stacked_xpression::skip_match
    template<typename That, typename BidiIter>
    static bool skip_impl(That const &that, match_state<BidiIter> &state)
    {
        return that.match(state);
    }

    // for linking a compiled regular xpression
    template<typename Char>
    void link(xpression_linker<Char> &linker) const
    {
        linker.accept(*static_cast<Matcher const *>(this), &this->next_);
        this->next_.link(linker);
    }

    // for building a lead-follow
    template<typename Char>
    void peek(xpression_peeker<Char> &peeker) const
    {
        this->peek_next_(peeker.accept(*static_cast<Matcher const *>(this)), peeker);
    }

    // for getting xpression width
    detail::width get_width() const
    {
        return this->get_width_(mpl::size_t<width>());
    }

private:

    static_xpression &operator =(static_xpression const &);

    template<typename Char>
    void peek_next_(mpl::true_, xpression_peeker<Char> &peeker) const
    {
        this->next_.peek(peeker);
    }

    template<typename Char>
    void peek_next_(mpl::false_, xpression_peeker<Char> &) const
    {
        // no-op
    }

    template<std::size_t Width>
    detail::width get_width_(mpl::size_t<Width>) const
    {
        return Width;
    }

    detail::width get_width_(unknown_width) const
    {
        // Should only be called in contexts where the width is
        // known to be fixed.
        return this->Matcher::get_width() + this->next_.get_width();
    }
};

///////////////////////////////////////////////////////////////////////////////
// make_static
//
template<typename Matcher>
inline static_xpression<Matcher> const
make_static(Matcher const &matcher)
{
    return static_xpression<Matcher>(matcher);
}

template<typename Matcher, typename Next>
inline static_xpression<Matcher, Next> const
make_static(Matcher const &matcher, Next const &next)
{
    return static_xpression<Matcher, Next>(matcher, next);
}

///////////////////////////////////////////////////////////////////////////////
// no_next
//
struct no_next
{
    BOOST_STATIC_CONSTANT(std::size_t, width = 0);
    BOOST_STATIC_CONSTANT(bool, pure = true);

    template<typename Char>
    void link(xpression_linker<Char> &) const
    {
    }

    template<typename Char>
    void peek(xpression_peeker<Char> &peeker) const
    {
        peeker.fail();
    }

    detail::width get_width() const
    {
        return 0;
    }
};

///////////////////////////////////////////////////////////////////////////////
// get_mark_number
//
inline int get_mark_number(basic_mark_tag const &mark)
{
    return proto::value(mark).mark_number_;
}

}}} // namespace boost::xpressive::detail

#endif

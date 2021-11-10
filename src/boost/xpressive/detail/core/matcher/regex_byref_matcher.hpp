///////////////////////////////////////////////////////////////////////////////
// regex_byref_matcher.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_REGEX_BYREF_MATCHER_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_REGEX_BYREF_MATCHER_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/assert.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/xpressive/regex_error.hpp>
#include <boost/xpressive/regex_constants.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/core/quant_style.hpp>
#include <boost/xpressive/detail/core/state.hpp>
#include <boost/xpressive/detail/core/regex_impl.hpp>
#include <boost/xpressive/detail/core/adaptor.hpp>

namespace boost { namespace xpressive { namespace detail
{
    ///////////////////////////////////////////////////////////////////////////////
    // regex_byref_matcher
    //
    template<typename BidiIter>
    struct regex_byref_matcher
      : quant_style<quant_variable_width, unknown_width::value, false>
    {
        // avoid cyclic references by holding a weak_ptr to the
        // regex_impl struct
        weak_ptr<regex_impl<BidiIter> > wimpl_;

        // the basic_regex object holds a ref-count to this regex_impl, so
        // we don't have to worry about it going away.
        regex_impl<BidiIter> const *pimpl_;

        regex_byref_matcher(shared_ptr<regex_impl<BidiIter> > const &impl)
          : wimpl_(impl)
          , pimpl_(impl.get())
        {
            BOOST_ASSERT(this->pimpl_);
        }

        template<typename Next>
        bool match(match_state<BidiIter> &state, Next const &next) const
        {
            BOOST_ASSERT(this->pimpl_ == this->wimpl_.lock().get());
            BOOST_XPR_ENSURE_(this->pimpl_->xpr_, regex_constants::error_badref, "bad regex reference");

            return push_context_match(*this->pimpl_, state, this->wrap_(next, is_static_xpression<Next>()));
        }

    private:
        template<typename Next>
        static xpression_adaptor<reference_wrapper<Next const>, matchable<BidiIter> > wrap_(Next const &next, mpl::true_)
        {
            // wrap the static xpression in a matchable interface
            return xpression_adaptor<reference_wrapper<Next const>, matchable<BidiIter> >(boost::cref(next));
        }

        template<typename Next>
        static Next const &wrap_(Next const &next, mpl::false_)
        {
            return next;
        }
    };

}}}

#endif

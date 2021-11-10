///////////////////////////////////////////////////////////////////////////////
// adaptor.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_CORE_ADAPTOR_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_CORE_ADAPTOR_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/ref.hpp>
#include <boost/implicit_cast.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/dynamic/matchable.hpp>

namespace boost { namespace xpressive { namespace detail
{

///////////////////////////////////////////////////////////////////////////////
// xpression_adaptor
//
//   wrap a static xpression in a matchable interface so it can be stored
//   in and invoked from a basic_regex object.
template<typename Xpr, typename Base>
struct xpression_adaptor
  : Base // either matchable or matchable_ex
{
    typedef typename Base::iterator_type iterator_type;
    typedef typename iterator_value<iterator_type>::type char_type;

    Xpr xpr_;

    xpression_adaptor(Xpr const &xpr)
    #if BOOST_WORKAROUND(__GNUC__, BOOST_TESTED_AT(4))
        // Ugh, gcc has an optimizer bug which elides this c'tor call
        // resulting in pure virtual function calls.
        __attribute__((__noinline__))
    #endif
      : xpr_(xpr)
    {
    }

    virtual bool match(match_state<iterator_type> &state) const
    {
        typedef typename boost::unwrap_reference<Xpr const>::type xpr_type;
        return implicit_cast<xpr_type &>(this->xpr_).match(state);
    }

    void link(xpression_linker<char_type> &linker) const
    {
        this->xpr_.link(linker);
    }

    void peek(xpression_peeker<char_type> &peeker) const
    {
        this->xpr_.peek(peeker);
    }

private:
    xpression_adaptor &operator =(xpression_adaptor const &);
};

///////////////////////////////////////////////////////////////////////////////
// make_adaptor
//
template<typename Base, typename Xpr>
inline intrusive_ptr<Base const> make_adaptor(Xpr const &xpr)
{
    return intrusive_ptr<Base const>(new xpression_adaptor<Xpr, Base>(xpr));
}

}}} // namespace boost::xpressive::detail

#endif

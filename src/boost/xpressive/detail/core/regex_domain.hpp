///////////////////////////////////////////////////////////////////////////////
/// \file regex_domain.hpp
/// Contains the definition of the regex_domain type
//
//  Copyright 2009 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_CORE_REGEX_DOMAIN_HPP_EAN_12_12_2009
#define BOOST_XPRESSIVE_DETAIL_CORE_REGEX_DOMAIN_HPP_EAN_12_12_2009

#include <boost/xpressive/xpressive_fwd.hpp>
#include <boost/proto/traits.hpp>
#include <boost/proto/domain.hpp>
#include <boost/proto/generate.hpp>

namespace boost { namespace xpressive { namespace detail
{
    struct regex_domain
      : proto::domain<proto::default_generator, proto::not_<proto::address_of<proto::_> > >
    {};
}}}

#endif

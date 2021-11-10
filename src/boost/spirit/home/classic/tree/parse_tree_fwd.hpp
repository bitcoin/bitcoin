/*=============================================================================
    Copyright (c) 2006 Tobias Schwinger
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_TREE_PARSE_TREE_FWD_HPP)
#define BOOST_SPIRIT_TREE_PARSE_TREE_FWD_HPP

#include <boost/spirit/home/classic/namespace.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    template <
        typename MatchPolicyT, 
        typename NodeFactoryT,
        typename T = nil_t
    >
    struct pt_tree_policy;

    template <
        typename IteratorT,
        typename NodeFactoryT = node_val_data_factory<nil_t>,
        typename T = nil_t
    >
    struct pt_match_policy;

    template <typename T>
    struct gen_pt_node_parser;

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif


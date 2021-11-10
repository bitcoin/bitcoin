// ----------------------------------------------------------------------------
// Copyright (C) 2002-2006 Marcin Kalicinski
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------
#ifndef BOOST_PROPERTY_TREE_DETAIL_XML_PARSER_FLAGS_HPP_INCLUDED
#define BOOST_PROPERTY_TREE_DETAIL_XML_PARSER_FLAGS_HPP_INCLUDED

namespace boost { namespace property_tree { namespace xml_parser
{

    /// Text elements should be put in separate keys,
    /// not concatenated in parent data.
    static const int no_concat_text  = 0x1;
    /// Comments should be omitted.
    static const int no_comments     = 0x2;
    /// Whitespace should be collapsed and trimmed.
    static const int trim_whitespace = 0x4;

    inline bool validate_flags(int flags)
    {
        return (flags & ~(no_concat_text | no_comments | trim_whitespace)) == 0;
    }

} } }

#endif

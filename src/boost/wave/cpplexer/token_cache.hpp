/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_TOKEN_CACHE_HPP_4D2320B7_1D56_4113_A114_397E70FA438C_INCLUDED)
#define BOOST_TOKEN_CACHE_HPP_4D2320B7_1D56_4113_A114_397E70FA438C_INCLUDED

#include <vector>

#include <boost/wave/wave_config.hpp>
#include <boost/wave/token_ids.hpp>

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace cpplexer {

///////////////////////////////////////////////////////////////////////////////
//
//  The token_cache template is used to cache the tokens corresponding to the
//  keywords, operators and other constant language elements.
//
//  This avoids repeated construction of these tokens, which is especially
//  effective when used in conjunction with a copy on write string
//  implementation (COW string).
//
///////////////////////////////////////////////////////////////////////////////
template <typename StringT>
class token_cache
{
public:
    token_cache()
    :   cache(T_LAST_TOKEN - T_FIRST_TOKEN)
    {
        typename std::vector<StringT>::iterator it = cache.begin();
        for (unsigned int i = T_FIRST_TOKEN; i < T_LAST_TOKEN;  ++i, ++it)
        {
            *it = StringT(boost::wave::get_token_value(token_id(i)));
        }
    }

    StringT const &get_token_value(token_id id) const
    {
        return cache[BASEID_FROM_TOKEN(id) - T_FIRST_TOKEN];
    }

private:
    std::vector<StringT> cache;
};

///////////////////////////////////////////////////////////////////////////////
}   // namespace cpplexer
}   // namespace wave
}   // namespace boost

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !defined(BOOST_TOKEN_CACHE_HPP_4D2320B7_1D56_4113_A114_397E70FA438C_INCLUDED)

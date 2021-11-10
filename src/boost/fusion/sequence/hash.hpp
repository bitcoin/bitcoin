/*=============================================================================
    Copyright (c) 2014 Christoph Weiss

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_HASH_23072014_1017)
#define FUSION_HASH_23072014_1017

#include <boost/functional/hash.hpp>
#include <boost/fusion/algorithm/iteration/fold.hpp>
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/utility/enable_if.hpp>

namespace boost { namespace fusion
{
    namespace hashing
    {
        struct hash_combine_fold
        {
            typedef std::size_t result_type;
            template<typename T>
            inline std::size_t operator()(std::size_t seed, T const& v)
            {
                boost::hash_combine(seed, v);
                return seed;
            }
        };

        template <typename Seq>
        inline typename
        boost::enable_if<traits::is_sequence<Seq>, std::size_t>::type
        hash_value(Seq const& seq)
        {
            return fold(seq, 0, hash_combine_fold());
        }
    }

    using hashing::hash_value;
}}

#endif

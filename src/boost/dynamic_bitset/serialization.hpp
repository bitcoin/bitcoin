// -----------------------------------------------------------
//
// Copyright (c) 2015 Seth Heeren
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// -----------------------------------------------------------

#ifndef BOOST_DYNAMIC_BITSET_SERIALIZATION_HPP
#define BOOST_DYNAMIC_BITSET_SERIALIZATION_HPP

#include "boost/dynamic_bitset/dynamic_bitset.hpp"
#include <boost/core/nvp.hpp>

namespace boost {

    // implementation for optional zero-copy serialization support
    template <typename Block, typename Allocator>
        class dynamic_bitset<Block, Allocator>::serialize_impl
        {
            public:
                template <typename Ar> 
                static void serialize(Ar& ar, dynamic_bitset<Block, Allocator>& bs, unsigned) {
                    ar & boost::make_nvp("m_num_bits", bs.m_num_bits)
                       & boost::make_nvp("m_bits", bs.m_bits);
                }
        };

}

// ADL hook to Boost Serialization library
namespace boost {
    namespace serialization {

        template <typename Ar, typename Block, typename Allocator>
            void serialize(Ar& ar, dynamic_bitset<Block, Allocator>& bs, unsigned version) {
                dynamic_bitset<Block, Allocator>::serialize_impl::serialize(ar, bs, version);
            }

    } // namespace serialization
} // namespace boost

#endif // include guard


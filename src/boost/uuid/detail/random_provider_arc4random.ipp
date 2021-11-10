//
// Copyright (c) 2017 James E. King III
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//   https://www.boost.org/LICENSE_1_0.txt)
//
// "A Replacement Call for Random"
// https://man.openbsd.org/arc4random.3
//

#include <cstddef>
#include <stdlib.h>

namespace boost {
namespace uuids {
namespace detail {

class random_provider_base
{
  public:
    //! Obtain entropy and place it into a memory location
    //! \param[in]  buf  the location to write entropy
    //! \param[in]  siz  the number of bytes to acquire
    void get_random_bytes(void *buf, std::size_t siz)
    {
        arc4random_buf(buf, siz);
    }
};

} // detail
} // uuids
} // boost

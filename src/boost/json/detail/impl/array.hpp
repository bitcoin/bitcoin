//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_DETAIL_IMPL_ARRAY_HPP
#define BOOST_JSON_DETAIL_IMPL_ARRAY_HPP

BOOST_JSON_NS_BEGIN
namespace detail {

unchecked_array::
~unchecked_array()
{
    if(! data_ ||
        sp_.is_not_shared_and_deallocate_is_trivial())
        return;
    for(unsigned long i = 0;
        i < size_; ++i)
        data_[i].~value();
}

void
unchecked_array::
relocate(value* dest) noexcept
{
    if(size_ > 0)
        std::memcpy(
            static_cast<void*>(dest),
            data_, size_ * sizeof(value));
    data_ = nullptr;
}

} // detail
BOOST_JSON_NS_END

#endif

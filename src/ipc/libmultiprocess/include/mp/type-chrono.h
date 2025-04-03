// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPE_CHRONO_H
#define MP_PROXY_TYPE_CHRONO_H

#include <mp/util.h>

#include <chrono>

namespace mp {
//! Overload CustomBuildField and CustomReadField to serialize std::chrono
//! parameters and return values as numbers.
template <class Rep, class Period, typename Value, typename Output>
void CustomBuildField(TypeList<std::chrono::duration<Rep, Period>>, Priority<1>, InvokeContext& invoke_context, Value&& value,
                      Output&& output)
{
    static_assert(std::numeric_limits<decltype(output.get())>::lowest() <= std::numeric_limits<Rep>::lowest(),
                  "capnp type does not have enough range to hold lowest std::chrono::duration value");
    static_assert(std::numeric_limits<decltype(output.get())>::max() >= std::numeric_limits<Rep>::max(),
                  "capnp type does not have enough range to hold highest std::chrono::duration value");
    output.set(value.count());
}

template <class Rep, class Period, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<std::chrono::duration<Rep, Period>>, Priority<1>, InvokeContext& invoke_context,
                               Input&& input, ReadDest&& read_dest)
{
    return read_dest.construct(input.get());
}
} // namespace mp

#endif // MP_PROXY_TYPE_CHRONO_H

// Copyright (c) The Bitcoin Core developers
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

//! Overload CustomBuildField and CustomReadField to serialize
//! std::chrono::time_point parameters and return values as integer tick counts.
//! The capnp field type must be an integer type with enough range to hold the
//! time_since_epoch() count for the given Duration (e.g. Int64 for nanoseconds).
template <class Clock, class Duration, typename Value, typename Output>
void CustomBuildField(TypeList<std::chrono::time_point<Clock, Duration>>, Priority<1>, InvokeContext& invoke_context,
                      Value&& value, Output&& output)
{
    using Rep = typename Duration::rep;
    static_assert(std::cmp_less_equal(std::numeric_limits<decltype(output.get())>::lowest(), std::numeric_limits<Rep>::lowest()),
                  "capnp type does not have enough range to hold lowest std::chrono::time_point value");
    static_assert(std::cmp_greater_equal(std::numeric_limits<decltype(output.get())>::max(), std::numeric_limits<Rep>::max()),
                  "capnp type does not have enough range to hold highest std::chrono::time_point value");
    output.set(value.time_since_epoch().count());
}

template <class Clock, class Duration, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<std::chrono::time_point<Clock, Duration>>, Priority<1>,
                               InvokeContext& invoke_context, Input&& input, ReadDest&& read_dest)
{
    return read_dest.construct(std::chrono::time_point<Clock, Duration>{Duration{input.get()}});
}
} // namespace mp

#endif // MP_PROXY_TYPE_CHRONO_H

// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPE_MAP_H
#define MP_PROXY_TYPE_MAP_H

#include <mp/proxy-types.h>
#include <mp/type-pair.h>
#include <mp/util.h>

namespace mp {
template <typename KeyLocalType, typename ValueLocalType, typename Value, typename Output>
void CustomBuildField(TypeList<std::map<KeyLocalType, ValueLocalType>>,
    Priority<1>,
    InvokeContext& invoke_context,
    Value&& value,
    Output&& output)
{
    // FIXME dededup with vector handler above
    auto list = output.init(value.size());
    size_t i = 0;
    for (const auto& elem : value) {
        BuildField(TypeList<std::pair<KeyLocalType, ValueLocalType>>(), invoke_context,
            ListOutput<typename decltype(list)::Builds>(list, i), elem);
        ++i;
    }
}

// Replacement for `m.emplace(piecewise_construct, t1, t2)` to work around a
// Clang 22 regression triggered by libc++'s std::map piecewise emplace: when
// the key constructor argument tuple is empty (std::tuple<>), libc++'s internal
// "try key extraction" SFINAE probe instantiates std::tuple_element<0,
// std::tuple<>>, which Clang 22 diagnoses as an out-of-bounds pack access ("a
// parameter pack may not be accessed at an out of bounds index") instead of
// treating it as substitution failure. See LLVM issue #167709 and the upstream
// fix in llvm/llvm-project PR #183614.
// https://github.com/llvm/llvm-project/issues/167709
// https://github.com/llvm/llvm-project/pull/183614
template <class Map, class Tuple1, class Tuple2>
auto EmplacePiecewiseSafe(
    Map& m,
    const std::piecewise_construct_t&,
    Tuple1&& t1,
    Tuple2&& t2)
{
    if constexpr (std::tuple_size_v<std::remove_reference_t<Tuple1>> == 0) {
        // Avoid tuple<> / tuple<> (LLVM 22 libc++ regression path)
        return m.emplace(std::piecewise_construct,
                         std::forward_as_tuple(typename Map::key_type{}),
                         std::forward<Tuple2>(t2));
    } else {
        return m.emplace(std::piecewise_construct,
                         std::forward<Tuple1>(t1),
                         std::forward<Tuple2>(t2));
    }
}

template <typename KeyLocalType, typename ValueLocalType, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<std::map<KeyLocalType, ValueLocalType>>,
    Priority<1>,
    InvokeContext& invoke_context,
    Input&& input,
    ReadDest&& read_dest)
{
    return read_dest.update([&](auto& value) {
        auto data = input.get();
        value.clear();
        for (auto item : data) {
            ReadField(TypeList<std::pair<const KeyLocalType, ValueLocalType>>(), invoke_context,
                Make<ValueField>(item),
                ReadDestEmplace(
                    TypeList<std::pair<const KeyLocalType, ValueLocalType>>(), [&](auto&&... args) -> auto& {
                        return *EmplacePiecewiseSafe(value, std::forward<decltype(args)>(args)...).first;
                    }));
        }
    });
}
} // namespace mp

#endif // MP_PROXY_TYPE_MAP_H

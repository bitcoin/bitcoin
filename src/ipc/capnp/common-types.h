// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_CAPNP_COMMON_TYPES_H
#define BITCOIN_IPC_CAPNP_COMMON_TYPES_H

#include <clientversion.h>
#include <interfaces/types.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <streams.h>
#include <univalue.h>

#include <cstddef>
#include <mp/proxy-types.h>
#include <type_traits>
#include <utility>

namespace ipc {
namespace capnp {
//! Construct a ParamStream wrapping a data stream with serialization parameters
//! needed to pass transaction objects between bitcoin processes.
//! In the future, more params may be added here to serialize other objects that
//! require serialization parameters. Params should just be chosen to serialize
//! objects completely and ensure that serializing and deserializing objects
//! with the specified parameters produces equivalent objects. It's also
//! harmless to specify serialization parameters here that are not used.
template <typename S>
auto Wrap(S& s)
{
    return ParamsStream{s, TX_WITH_WITNESS};
}

//! Detect if type has a deserialize_type constructor, which is
//! used to deserialize types like CTransaction that can't be unserialized into
//! existing objects because they are immutable.
template <typename T>
concept Deserializable = std::is_constructible_v<T, ::deserialize_type, ::DataStream&>;
} // namespace capnp
} // namespace ipc

//! Functions to serialize / deserialize common bitcoin types.
namespace mp {
//! Overload multiprocess library's CustomBuildField hook to allow any
//! serializable object to be stored in a capnproto Data field or passed to a
//! capnproto interface. Use Priority<1> so this hook has medium priority, and
//! higher priority hooks could take precedence over this one.
template <typename LocalType, typename Value, typename Output>
void CustomBuildField(TypeList<LocalType>, Priority<1>, InvokeContext& invoke_context, Value&& value, Output&& output)
// Enable if serializeable and if LocalType is not cv or reference qualified. If
// LocalType is cv or reference qualified, it is important to fall back to
// lower-priority Priority<0> implementation of this function that strips cv
// references, to prevent this CustomBuildField overload from taking precedence
// over more narrow overloads for specific LocalTypes.
requires Serializable<LocalType, DataStream> && std::is_same_v<LocalType, std::remove_cv_t<std::remove_reference_t<LocalType>>>
{
    DataStream stream;
    auto wrapper{ipc::capnp::Wrap(stream)};
    value.Serialize(wrapper);
    auto result = output.init(stream.size());
    memcpy(result.begin(), stream.data(), stream.size());
}

//! Overload multiprocess library's CustomReadField hook to allow any object
//! with an Unserialize method to be read from a capnproto Data field or
//! returned from capnproto interface. Use Priority<1> so this hook has medium
//! priority, and higher priority hooks could take precedence over this one.
template <typename LocalType, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<LocalType>, Priority<1>, InvokeContext& invoke_context, Input&& input, ReadDest&& read_dest)
requires Unserializable<LocalType, DataStream> && (!ipc::capnp::Deserializable<LocalType>)
{
    return read_dest.update([&](auto& value) {
        if (!input.has()) return;
        auto data = input.get();
        SpanReader stream({data.begin(), data.end()});
        auto wrapper{ipc::capnp::Wrap(stream)};
        value.Unserialize(wrapper);
    });
}

//! Overload multiprocess library's CustomReadField hook to allow any object
//! with a deserialize constructor to be read from a capnproto Data field or
//! returned from capnproto interface. Use Priority<1> so this hook has medium
//! priority, and higher priority hooks could take precedence over this one.
template <typename LocalType, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<LocalType>, Priority<1>, InvokeContext& invoke_context, Input&& input, ReadDest&& read_dest)
requires ipc::capnp::Deserializable<LocalType>
{
    assert(input.has());
    auto data = input.get();
    SpanReader stream({data.begin(), data.end()});
    auto wrapper{ipc::capnp::Wrap(stream)};
    return read_dest.construct(::deserialize, wrapper);
}

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

//! Overload CustomBuildField and CustomReadField to serialize UniValue
//! parameters and return values as JSON strings.
template <typename Value, typename Output>
void CustomBuildField(TypeList<UniValue>, Priority<1>, InvokeContext& invoke_context, Value&& value, Output&& output)
{
    std::string str = value.write();
    auto result = output.init(str.size());
    memcpy(result.begin(), str.data(), str.size());
}

template <typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<UniValue>, Priority<1>, InvokeContext& invoke_context, Input&& input,
                               ReadDest&& read_dest)
{
    return read_dest.update([&](auto& value) {
        auto data = input.get();
        value.read(std::string_view{data.begin(), data.size()});
    });
}

//! Generic ::capnp::Data field builder for any C++ type that can be converted
//! to a span of bytes, like std::vector<char> or std::array<uint8_t>, or custom
//! blob types like uint256 or PKHash with data() and size() methods pointing to
//! bytes.
//!
//! Note: it might make sense to move this function into libmultiprocess, since
//! it is fairly generic. However this would require decreasing its priority so
//! it can be overridden, which would require more changes inside
//! libmultiprocess to avoid conflicting with the Priority<1> CustomBuildField
//! function it already provides for std::vector. Also, it might make sense to
//! provide a CustomReadField counterpart to this function, which could be
//! called to read C++ types that can be constructed from spans of bytes from
//! ::capnp::Data fields. But so far there hasn't been a need for this.
template <typename LocalType, typename Value, typename Output>
void CustomBuildField(TypeList<LocalType>, Priority<2>, InvokeContext& invoke_context, Value&& value, Output&& output)
requires
    (std::is_same_v<decltype(output.get()), ::capnp::Data::Builder>) &&
    (std::convertible_to<Value, std::span<const std::byte>> ||
     std::convertible_to<Value, std::span<const char>> ||
     std::convertible_to<Value, std::span<const unsigned char>> ||
     std::convertible_to<Value, std::span<const signed char>>)
{
    auto data = std::span{value};
    auto result = output.init(data.size());
    memcpy(result.begin(), data.data(), data.size());
}
} // namespace mp

#endif // BITCOIN_IPC_CAPNP_COMMON_TYPES_H

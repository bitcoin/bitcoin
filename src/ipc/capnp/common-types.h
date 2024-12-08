// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_CAPNP_COMMON_TYPES_H
#define BITCOIN_IPC_CAPNP_COMMON_TYPES_H

#include <chainparams.h>
#include <clientversion.h>
#include <consensus/validation.h>
#include <interfaces/types.h>
#include <ipc/capnp/common.capnp.proxy.h>
#include <netbase.h>
#include <net_processing.h>
#include <net_types.h>
#include <primitives/transaction.h>
#include <protocol.h>
#include <serialize.h>
#include <streams.h>
#include <univalue.h>
#include <util/result.h>
#include <util/translation.h>
#include <validation.h>
#include <wallet/coincontrol.h>

#include <cstddef>
#include <mp/proxy-types.h>
#include <type_traits>
#include <unordered_set>
#include <utility>

namespace ipc {
namespace capnp {
//! Convert kj::StringPtr to std::string.
inline std::string ToString(const kj::StringPtr& str) { return {str.cStr(), str.size()}; }

//! Convert kj::ArrayPtr to std::string.
inline std::string ToString(const kj::ArrayPtr<const kj::byte>& array)
{
    return {reinterpret_cast<const char*>(array.begin()), array.size()};
}

//! Convert kj::ArrayPtr to base_blob.
template <typename T>
inline T ToBlob(const kj::ArrayPtr<const kj::byte>& array)
{
    return T({array.begin(), array.begin() + array.size()});
}

//! Convert base_blob to kj::ArrayPtr.
template <typename T>
inline kj::ArrayPtr<const kj::byte> ToArray(const T& blob)
{
    return {reinterpret_cast<const kj::byte*>(blob.data()), blob.size()};
}

//! Construct a ParamStream wrapping data stream with serialization parameters
//! needed to pass transaction and address objects between bitcoin processes.
//! In the future, more params may be added here to serialize other objects that
//! require serialization parameters. Params should just be chosen to serialize
//! objects completely and ensure that serializing and deserializing objects
//! with the specified parameters produces equivalent objects. It's also
//! harmless to specify serialization parameters here that are not used.
template <typename S>
auto Wrap(S& s)
{
    return ParamsStream{s, TX_WITH_WITNESS, CAddress::V2_NETWORK};
}

//! Serialize bitcoin value.
template <typename T>
DataStream Serialize(const T& value)
{
    DataStream stream;
    auto wrapper{Wrap(stream)};
    value.Serialize(wrapper);
    return stream;
}

//! Deserialize bitcoin value.
template <typename T>
T Unserialize(const kj::ArrayPtr<const kj::byte>& data)
{
    SpanReader stream{{data.begin(), data.end()}};
    T value;
    auto wrapper{Wrap(stream)};
    value.Unserialize(wrapper);
    return value;
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

template <typename Value, typename Output>
void CustomBuildField(TypeList<fs::path>, Priority<1>, InvokeContext& invoke_context, Value&& path, Output&& output)
{
    std::string str = fs::PathToString(path);
    auto result = output.init(str.size());
    memcpy(result.begin(), str.data(), str.size());
}

template <typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<fs::path>, Priority<1>, InvokeContext& invoke_context, Input&& input,
                               ReadDest&& read_dest)
{
    auto data = input.get();
    return read_dest.construct(fs::PathFromString({CharCast(data.begin()), data.size()}));
}

template <typename Value, typename Output>
void CustomBuildField(TypeList<SecureString>, Priority<1>, InvokeContext& invoke_context, Value&& str, Output&& output)
{
    auto result = output.init(str.size());
    // Copy SecureString into output. Caller needs to be responsible for calling
    // memory_cleanse later on the output after it is sent.
    memcpy(result.begin(), str.data(), str.size());
}

template <typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<SecureString>, Priority<1>, InvokeContext& invoke_context, Input&& input,
                               ReadDest&& read_dest)
{
    auto data = input.get();
    // Copy input into SecureString. Caller needs to be responsible for calling
    // memory_cleanse on the input.
    return read_dest.construct(CharCast(data.begin()), data.size());
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

//! Overload CustomBuildField and CustomReadField to serialize
//! UniValue::type_error exceptions as text strings.
template <typename Value, typename Output>
void CustomBuildField(TypeList<UniValue::type_error>, Priority<1>, InvokeContext& invoke_context,
                      Value&& value, Output&& output)
{
    BuildField(TypeList<std::string>(), invoke_context, output, std::string(value.what()));
}

template <typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<UniValue::type_error>, Priority<1>, InvokeContext& invoke_context,
                               Input&& input, ReadDest&& read_dest)
{
    read_dest.construct(ReadField(TypeList<std::string>(), invoke_context, input, mp::ReadDestTemp<std::string>()));
}

template <typename Output>
void CustomBuildField(
    TypeList<>, Priority<1>, InvokeContext& invoke_context, Output&& output,
    typename std::enable_if<std::is_same<decltype(output.get()),
                                         ipc::capnp::messages::GlobalArgs::Builder>::value>::type* enable = nullptr)
{
    ipc::capnp::BuildGlobalArgs(invoke_context, output.init());
}

template <typename Accessor, typename ServerContext, typename Fn, typename... Args>
auto CustomPassField(TypeList<>, ServerContext& server_context, const Fn& fn, Args&&... args) ->
    typename std::enable_if<std::is_same<decltype(Accessor::get(server_context.call_context.getParams())),
                                         ipc::capnp::messages::GlobalArgs::Reader>::value>::type
{
    ipc::capnp::ReadGlobalArgs(server_context, Accessor::get(server_context.call_context.getParams()));
    return fn.invoke(server_context, std::forward<Args>(args)...);
}

//! Overload CustomBuildField and CustomReadField to serialize util::Result
//! return values as common.capnp Result and ResultVoid structs
template <typename LocalType, typename Value, typename Output>
void CustomBuildField(TypeList<util::Result<LocalType>>, Priority<1>, InvokeContext& invoke_context, Value&& value,
                      Output&& output)
{
    auto result = output.init();
    if (value) {
        if constexpr (!std::is_same_v<LocalType, void>) {
            using ValueAccessor = typename ProxyStruct<typename decltype(result)::Builds>::ValueAccessor;
            BuildField(TypeList<LocalType>(), invoke_context, Make<StructField, ValueAccessor>(result), *value);
        }
    } else {
        BuildField(TypeList<bilingual_str>(), invoke_context, Make<ValueField>(result.initError()),
                   util::ErrorString(value));
    }
}

template <typename LocalType, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<util::Result<LocalType>>, Priority<1>, InvokeContext& invoke_context,
                               Input&& input, ReadDest&& read_dest)
{
    auto result = input.get();
    if (result.hasError()) {
        bilingual_str error;
        ReadField(mp::TypeList<bilingual_str>(), invoke_context, mp::Make<mp::ValueField>(result.getError()),
                    mp::ReadDestValue(error));
        read_dest.construct(util::Error{std::move(error)});
    } else {
        if constexpr (!std::is_same_v<LocalType, void>) {
            assert (result.hasValue());
            ReadField(mp::TypeList<LocalType>(), invoke_context, mp::Make<mp::ValueField>(result.getValue()),
                mp::ReadDestEmplace(
                    mp::TypeList<LocalType>(), [&](auto&&... args) -> auto& {
                        return *read_dest.construct(LocalType{std::forward<decltype(args)>(args)...});
                    }));
        } else {
            read_dest.construct();
        }
    }
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

// libmultiprocess only provides read/build functions for std::set, not
// std::unordered_set, so copy and paste those functions here.
// TODO: Move these to libmultiprocess and dedup std::set, std::unordered_set,
// and std::vector implementations.
template <typename LocalType, typename Hash, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<std::unordered_set<LocalType, Hash>>, Priority<1>,
                               InvokeContext& invoke_context, Input&& input, ReadDest&& read_dest)
{
    return read_dest.update([&](auto& value) {
        auto data = input.get();
        value.clear();
        for (auto item : data) {
            ReadField(TypeList<LocalType>(), invoke_context, Make<ValueField>(item),
                      ReadDestEmplace(
                          TypeList<const LocalType>(), [&](auto&&... args) -> auto& {
                              return *value.emplace(std::forward<decltype(args)>(args)...).first;
                          }));
        }
    });
}

template <typename LocalType, typename Hash, typename Value, typename Output>
void CustomBuildField(TypeList<std::unordered_set<LocalType, Hash>>, Priority<1>, InvokeContext& invoke_context,
                      Value&& value, Output&& output)
{
    auto list = output.init(value.size());
    size_t i = 0;
    for (const auto& elem : value) {
        BuildField(TypeList<LocalType>(), invoke_context, ListOutput<typename decltype(list)::Builds>(list, i), elem);
        ++i;
    }
}
} // namespace mp

#endif // BITCOIN_IPC_CAPNP_COMMON_TYPES_H

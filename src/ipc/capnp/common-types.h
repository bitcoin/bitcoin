// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_CAPNP_COMMON_TYPES_H
#define BITCOIN_IPC_CAPNP_COMMON_TYPES_H

#include <clientversion.h>
#include <protocol.h>
#include <streams.h>
#include <univalue.h>
#include <util/result.h>
#include <util/translation.h>

#include <cstddef>
#include <mp/proxy-types.h>
#include <type_traits>
#include <unordered_set>
#include <utility>

namespace ipc {
namespace capnp {
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
template<typename S>
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

//! Use SFINAE to define Serializeable<T> trait which is true if type T has a
//! Serialize(stream) method, false otherwise.
template <typename T>
struct Serializable {
private:
    template <typename C>
    static std::true_type test(decltype(std::declval<C>().Serialize(std::declval<std::nullptr_t&>()))*);
    template <typename>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(nullptr))::value;
};

//! Use SFINAE to define Unserializeable<T> trait which is true if type T has
//! an Unserialize(stream) method, false otherwise.
template <typename T>
struct Unserializable {
private:
    template <typename C>
    static std::true_type test(decltype(std::declval<C>().Unserialize(std::declval<std::nullptr_t&>()))*);
    template <typename>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(nullptr))::value;
};

//! Trait which is true if type has a deserialize_type constructor, which is
//! used to deserialize types like CTransaction that can't be unserialized into
//! existing objects because they are immutable.
template <typename T>
using Deserializable = std::is_constructible<T, ::deserialize_type, ::DataStream&>;
} // namespace capnp
} // namespace ipc

//! Functions to serialize / deserialize common bitcoin types.
namespace mp {
//! Overload multiprocess library's CustomBuildField hook to allow any
//! serializable object to be stored in a capnproto Data field or passed to a
//! canproto interface. Use Priority<1> so this hook has medium priority, and
//! higher priority hooks could take precedence over this one.
template <typename LocalType, typename Value, typename Output>
void CustomBuildField(
    TypeList<LocalType>, Priority<1>, InvokeContext& invoke_context, Value&& value, Output&& output,
    // Enable if serializeable and if LocalType is not cv or reference
    // qualified. If LocalType is cv or reference qualified, it is important to
    // fall back to lower-priority Priority<0> implementation of this function
    // that strips cv references, to prevent this CustomBuildField overload from
    // taking precedence over more narrow overloads for specific LocalTypes.
    std::enable_if_t<ipc::capnp::Serializable<LocalType>::value &&
                     std::is_same_v<LocalType, std::remove_cv_t<std::remove_reference_t<LocalType>>>>* enable = nullptr)
{
    DataStream stream;
    auto wrapper{ipc::capnp::Wrap(stream)};
    value.Serialize(wrapper);
    auto result = output.init(stream.size());
    memcpy(result.begin(), stream.data(), stream.size());
}

//! Overload multiprocess library's CustomReadField hook to allow any object
//! with an Unserialize method to be read from a capnproto Data field or
//! returned from canproto interface. Use Priority<1> so this hook has medium
//! priority, and higher priority hooks could take precedence over this one.
template <typename LocalType, typename Input, typename ReadDest>
decltype(auto)
CustomReadField(TypeList<LocalType>, Priority<1>, InvokeContext& invoke_context, Input&& input, ReadDest&& read_dest,
                std::enable_if_t<ipc::capnp::Unserializable<LocalType>::value &&
                                 !ipc::capnp::Deserializable<LocalType>::value>* enable = nullptr)
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
//! with an deserialize constructor to be read from a capnproto Data field or
//! returned from canproto interface. Use Priority<1> so this hook has medium
//! priority, and higher priority hooks could take precedence over this one.
template <typename LocalType, typename Input, typename ReadDest>
decltype(auto)
CustomReadField(TypeList<LocalType>, Priority<1>, InvokeContext& invoke_context, Input&& input, ReadDest&& read_dest,
                std::enable_if_t<ipc::capnp::Deserializable<LocalType>::value>* enable = nullptr)
{
    assert(input.has());
    auto data = input.get();
    SpanReader stream({data.begin(), data.end()});
    // TODO: instead of always preferring Deserialize implementation over
    // Unserialize should prefer Deserializing when emplacing, unserialize when
    // updating. Can implement by adding read_dest.alreadyConstructed()
    // constexpr bool method in libmultiprocess.
    auto wrapper{ipc::capnp::Wrap(stream)};
    return read_dest.construct(deserialize, wrapper);
}

//! Overload CustomBuildField and CustomReadField to serialize::chrono::seconds
//! parameters and return values as integers.
template <typename Value, typename Output>
void CustomBuildField(TypeList<std::chrono::seconds>, Priority<1>, InvokeContext& invoke_context, Value&& value,
                      Output&& output)
{
    output.set(value.count());
}

template <typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<std::chrono::seconds>, Priority<1>, InvokeContext& invoke_context,
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

//! Overload CustomBuildField and CustomReadField to serialize other types of
//! objects that have CustomBuildMessage and CustomReadMessage overloads.
//! Defining BuildMessage and ReadMessage overloads is simpler than defining
//! BuildField and ReadField overloads because these overloads can be normal
//! functions instead of template functions, and defined in a .cpp file instead
//! of a header.
template <typename LocalType, typename Value, typename Output>
void CustomBuildField(TypeList<LocalType>, Priority<2>, InvokeContext& invoke_context, Value&& value, Output&& output,
                      decltype(CustomBuildMessage(invoke_context, value, std::move(output.get())))* enable = nullptr)
{
    CustomBuildMessage(invoke_context, value, std::move(output.init()));
}

template <typename LocalType, typename Reader, typename ReadDest>
decltype(auto) CustomReadField(TypeList<LocalType>, Priority<2>, InvokeContext& invoke_context, Reader&& reader,
                               ReadDest&& read_dest,
                               decltype(CustomReadMessage(invoke_context, reader.get(),
                                                          std::declval<LocalType&>()))* enable = nullptr)
{
    return read_dest.update([&](auto& value) { CustomReadMessage(invoke_context, reader.get(), value); });
}

//! Helper for CustomPassField below. Call Accessor::get method if it has one,
//! otherwise return capnp::Void.
template <typename Accessor, typename Message>
decltype(auto) MaybeGet(Message&& message, decltype(Accessor::get(message))* enable = nullptr)
{
    return Accessor::get(message);
}

template <typename Accessor>
::capnp::Void MaybeGet(...)
{
    return {};
}

//! Helper for CustomPassField below. Call Accessor::init method if it has one,
//! otherwise do nothing.
template <typename Accessor, typename Message>
decltype(auto) MaybeInit(Message&& message, decltype(Accessor::get(message))* enable = nullptr)
{
    return Accessor::init(message);
}

template <typename Accessor>
::capnp::Void MaybeInit(...)
{
    return {};
}

//! Overload CustomPassField to serialize types of objects that have
//! CustomPassMessage overloads. Defining PassMessage overloads is simpler than
//! defining PassField overloads, because PassMessage overloads can be normal
//! functions instead of template functions, and defined in a .cpp file instead
//! of a header.
//!
//! Defining a PassField or PassMessage overload is useful for input/output
//! parameters. If an overload is not defined these parameters will just be
//! deserialized on the server side with ReadField into a temporary variable,
//! then the server method will be called passing the temporary variable as a
//! parameter, then the temporary variable will be serialized and sent back to
//! the client with BuildField. But if a PassField or PassMessage overload is
//! defined, the overload is called with a callback to invoke and pass
//! parameters to the server side function, and run arbitrary code before and
//! after invoking the function.
template <typename Accessor, typename... LocalTypes, typename ServerContext, typename Fn, typename... Args>
auto CustomPassField(TypeList<LocalTypes...>, ServerContext& server_context, Fn&& fn, Args&&... args)
    -> decltype(CustomPassMessage(server_context, MaybeGet<Accessor>(server_context.call_context.getParams()),
                                  MaybeGet<Accessor>(server_context.call_context.getResults()), nullptr))
{
    CustomPassMessage(server_context, MaybeGet<Accessor>(server_context.call_context.getParams()),
                      MaybeInit<Accessor>(server_context.call_context.getResults()),
                      [&](LocalTypes... param) { fn.invoke(server_context, std::forward<Args>(args)..., param...); });
}

//! Generic ::capnp::Data field builder for any class that a Span can be
//! constructed from, particularly BaseHash and base_blob classes and
//! subclasses. It's also used to serialize vector<unsigned char>
//! set elements in GCSFilter::ElementSet.
//!
//! There is currently no corresponding ::capnp::Data CustomReadField function
//! that works using Spans, because the bitcoin classes in the codebase like
//! BaseHash and blob_blob that can converted /to/ Span don't currently have
//! Span constructors that allow them to be constructed /from/ Span. For example
//! CustomReadField function could be written that would allow dropping
//! specialized CustomReadField functions for types like PKHash.
//!
//! For the LocalType = vector<unsigned char> case, it's also not necessary to
//! have ::capnp::Data CustomReadField function corresponding to this
//! CustomBuildField function because ::capnp::Data inherits from
//! ::capnp::ArrayPtr, and libmultiprocess already provides a generic
//! CustomReadField function that can read from ::capnp::ArrayPtr into
//! std::vector.
template <typename LocalType, typename Value, typename Output>
void CustomBuildField(
    TypeList<LocalType>, Priority<2>, InvokeContext& invoke_context, Value&& value, Output&& output,
    typename std::enable_if_t<!ipc::capnp::Serializable<typename std::remove_cv<
        typename std::remove_reference<Value>::type>::type>::value>* enable_not_serializable = nullptr,
    typename std::enable_if_t<std::is_same_v<decltype(output.get()), ::capnp::Data::Builder>>* enable_output =
        nullptr,
    decltype(Span{value})* enable_value = nullptr)
{
    auto data = Span{value};
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

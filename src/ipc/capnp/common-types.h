// Copyright (c) 2023-present The Bitcoin Core developers
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
#include <kj/async.h>
#include <memory>
#include <mp/proxy-io.h>
#include <mp/proxy-types.h>
#include <mp/type-chrono.h>
#include <mp/type-context.h>
#include <mp/type-data.h>
#include <mp/type-decay.h>
#include <mp/type-interface.h>
#include <mp/type-message.h>
#include <mp/type-number.h>
#include <mp/type-optional.h>
#include <mp/type-pointer.h>
#include <mp/type-string.h>
#include <mp/type-struct.h>
#include <mp/type-threadmap.h>
#include <mp/type-vector.h>
#include <mp/util.h>
#include <stdexcept>
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

//! Client-side state shared between a cancelable IPC request and the
//! `interfaces::CancelFn` handed to the caller.
class CancelState
{
public:
    explicit CancelState(mp::EventLoop& loop) : m_loop(loop) {}

    void setCanceler(kj::Canceler* canceler)
    {
        const mp::Lock lock{m_mutex};
        m_canceler = canceler;
        if (m_canceler && m_canceled) m_canceler->cancel("canceled by client");
    }

    //! Cancel the request. Callable from any thread.
    void cancel()
    {
        {
            const mp::Lock lock{m_mutex};
            if (m_canceled) return;
            m_canceled = true;
            if (!m_canceler) return;
        }
        m_loop->sync([&] {
            const mp::Lock lock{m_mutex};
            if (m_canceler) m_canceler->cancel("canceled by client");
        });
    }

    bool canceled()
    {
        const mp::Lock lock{m_mutex};
        return m_canceled;
    }

private:
    mp::EventLoopRef m_loop;
    mp::Mutex m_mutex;
    bool m_canceled MP_GUARDED_BY(m_mutex){false};
    kj::Canceler* m_canceler MP_GUARDED_BY(m_mutex){nullptr};
};
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
        if (!value.read(std::string_view{data.begin(), data.size()})) {
            throw std::runtime_error{"invalid JSON received over IPC"};
        }
    });
}

//! Interpret empty Data fields as null CTransactionRef values. This is safe to
//! do because no CTransaction is ever serialized as empty Data, and it is
//! convenient because this allows std::vector<CTransactionRef> to be passed as
//! List(Data) even if the vector contains null values, and even though Cap'n
//! Proto does not (currently) allow distinguishing between null and empty Data
//! values in a List. Interpreting empty Data values as null CTransactionRef
//! values works well for this purpose.
template <typename Input>
bool CustomHasField(TypeList<CTransaction>, InvokeContext& invoke_context, const Input& input)
{
    return input.get().size() > 0;
}

//! Overload multiprocess library's CustomBuildExtraParam hook so an
//! `interfaces::CancelArg` parameter declared with `$Proxy.extraParam` in a
//! capnp schema makes the client request cancelable. The caller's `CancelArg`
//! receives a `CancelFn` that cancels the request from any thread, after which
//! the method call throws `InterruptException` instead of returning.
inline void CustomBuildExtraParam(TypeList<interfaces::CancelArg>, ClientInvokeContext& invoke_context, interfaces::CancelArg&& value)
{
    if (!value) return;
    auto state{std::make_shared<ipc::capnp::CancelState>(*invoke_context.connection.m_loop)};
    invoke_context.set_canceler = [state](kj::Canceler* canceler) { state->setCanceler(canceler); };
    invoke_context.handle_error = [state](const kj::Exception&) {
        if (state->canceled()) throw InterruptException{"canceled"};
    };
    // The client-side guard has nothing to unregister.
    static_cast<void>(value([state] { state->cancel(); }));
}

//! Overload multiprocess library's CustomReadExtraParam hook to build the
//! `interfaces::CancelArg` passed to a wrapped server method. It registers one
//! `CancelFn` per request that the event loop runs if the client cancels the
//! request or disconnects, or runs immediately if the request was already
//! canceled before the method registered it. The returned `CancelGuard` clears
//! the callback at destruction, so the event-loop thread can't run the callback
//! after the wrapped method goes out of scope and locals are freed.
template <typename ServerContext>
interfaces::CancelArg CustomReadExtraParam(TypeList<interfaces::CancelArg>, ServerContext& server_context)
{
    if (!server_context.request_mutex) {
        // The method runs on the event loop thread, which is where
        // cancellations are dispatched, so it can't be canceled mid-execution.
        return [](interfaces::CancelFn) { return interfaces::CancelGuard{}; };
    }
    return [&server_context](interfaces::CancelFn fn) {
        {
            const Lock lock{*server_context.request_mutex};
            if (!server_context.request_canceled) {
                server_context.cancel_fn = std::move(fn);
                return interfaces::CancelGuard{[&server_context] {
                    const Lock lock{*server_context.request_mutex};
                    server_context.cancel_fn = nullptr;
                }};
            }
        }
        fn();
        return interfaces::CancelGuard{};
    };
}
} // namespace mp

#endif // BITCOIN_IPC_CAPNP_COMMON_TYPES_H

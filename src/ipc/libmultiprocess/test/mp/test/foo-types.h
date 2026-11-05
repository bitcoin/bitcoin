// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_TEST_FOO_TYPES_H
#define MP_TEST_FOO_TYPES_H

#include <mp/proxy.h>
#include <mp/proxy-types.h>

// IWYU pragma: begin_exports
#include <any>
#include <capnp/common.h>
#include <cstddef>
#include <mp/test/foo.capnp.h>
#include <mp/type-context.h>
#include <mp/type-data.h>
#include <mp/type-decay.h>
#include <mp/type-function.h>
#include <mp/type-interface.h>
#include <mp/type-map.h>
#include <mp/type-message.h>
#include <mp/type-number.h>
#include <mp/type-optional.h>
#include <mp/type-pointer.h>
#include <mp/type-set.h>
#include <mp/type-string.h>
#include <mp/type-struct.h>
#include <mp/type-threadmap.h>
#include <mp/type-unordered-set.h>
#include <mp/type-vector.h>
#include <string>
#include <type_traits>
// IWYU pragma: end_exports

#include <kj/async.h>
#include <kj/exception.h>
#include <memory>

namespace mp {
namespace test {
namespace messages {
struct ExtendedCallback; // IWYU pragma: export
struct FooCallback; // IWYU pragma: export
struct FooFn; // IWYU pragma: export
struct FooInterface; // IWYU pragma: export
struct BarInterface; // IWYU pragma: export
} // namespace messages

class CancelFnState
{
public:
    explicit CancelFnState(EventLoop& loop) : m_loop(loop) {}

    void setCanceler(kj::Canceler* canceler)
    {
        const Lock lock{m_mutex};
        m_canceler = canceler;
        if (m_canceler && m_canceled) m_canceler->cancel("canceled by client");
    }

    //! Cancel the request. Callable from any thread.
    void cancel()
    {
        {
            const Lock lock{m_mutex};
            if (m_canceled) return;
            m_canceled = true;
            if (!m_canceler) return;
        }
        m_loop->sync([&] {
            const Lock lock{m_mutex};
            if (m_canceler) m_canceler->cancel("canceled by client");
        });
    }

    bool canceled()
    {
        const Lock lock{m_mutex};
        return m_canceled;
    }

private:
    EventLoopRef m_loop;
    Mutex m_mutex;
    bool m_canceled MP_GUARDED_BY(m_mutex){false};
    kj::Canceler* m_canceler MP_GUARDED_BY(m_mutex){nullptr};
};

template <typename Output>
void CustomBuildField(TypeList<FooCustom>, Priority<1>, InvokeContext& invoke_context, const FooCustom& value, Output&& output)
{
    BuildField(TypeList<std::string>(), invoke_context, output, value.v1);
    output.setV2(value.v2);
    BuildField(TypeList<std::vector<int>>(), invoke_context, output, value.v3);
}

template <typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<FooCustom>, Priority<1>, InvokeContext& invoke_context, Input&& input, ReadDest&& read_dest)
{
    messages::FooCustom::Reader custom = input.get();
    return read_dest.update([&](FooCustom& value) {
        value.v1 = ReadField(TypeList<std::string>(), invoke_context, mp::Make<mp::ValueField>(custom.getV1()), ReadDestTemp<std::string>());
        value.v2 = custom.getV2();
        value.v3 = ReadField(TypeList<std::vector<int>>(), invoke_context, mp::Make<mp::ValueField>(custom.getV3()), ReadDestTemp<std::vector<int>>());
    });
}

} // namespace test

template <typename Input>
bool CustomHasField(TypeList<test::FooData>, InvokeContext& invoke_context, const Input& input)
{
    // Cap'n Proto C++ cannot distinguish null vs empty Data in List(Data), so
    // interpret empty Data as null for this specific type.
    return input.get().size() != 0;
}

inline void CustomBuildMessage(InvokeContext& invoke_context,
                        const test::FooMessage& src,
                        test::messages::FooMessage::Builder&& builder)
{
    const auto& hook{invoke_context.connection.m_loop->testing_hook_misc};
    if (hook) hook("build FooMessage");
    builder.setMessage(src.message + " build");
}

inline void CustomReadMessage(InvokeContext& invoke_context,
                       const test::messages::FooMessage::Reader& reader,
                       test::FooMessage& dest)
{
    dest.message = std::string{reader.getMessage()} + " read";
}

inline void CustomBuildMessage(InvokeContext& invoke_context,
                        const test::FooMutable& src,
                        test::messages::FooMutable::Builder&& builder)
{
    builder.setMessage(src.message + " build");
}

inline void CustomReadMessage(InvokeContext& invoke_context,
                       const test::messages::FooMutable::Reader& reader,
                       test::FooMutable& dest)
{
    dest.message = std::string{reader.getMessage()} + " read";
}

inline void CustomPassMessage(InvokeContext& invoke_context,
                       const test::messages::FooMutable::Reader& reader,
                       test::messages::FooMutable::Builder builder,
                       std::function<void(test::FooMutable&)>&& fn)
{
    test::FooMutable mut;
    mut.message = std::string{reader.getMessage()} + " pass";
    fn(mut);
    builder.setMessage(mut.message + " return");
}

inline void CustomBuildExtraParam(TypeList<int>, ClientInvokeContext& invoke_context, int&& value)
{
    auto& loop = *invoke_context.connection.m_loop;
    if (loop.testing_hook_misc) loop.testing_hook_misc(value);
}

template <typename ServerContext>
int CustomReadExtraParam(TypeList<int>, ServerContext& server_context)
{
    return 1;
}

inline void CustomBuildExtraParam(TypeList<test::CancelArg>, ClientInvokeContext& invoke_context, test::CancelArg&& value)
{
    if (!value) return;
    auto state{std::make_shared<test::CancelFnState>(*invoke_context.connection.m_loop)};
    invoke_context.set_canceler = [state](kj::Canceler* canceler) { state->setCanceler(canceler); };
    invoke_context.handle_error = [state](const kj::Exception&) {
        if (state->canceled()) throw InterruptException{"canceled"};
    };
    // The `CancelGuard` object returned on the client side has nothing to unregister.
    static_cast<void>(value([state] { state->cancel(); }));
}

template <typename ServerContext>
test::CancelArg CustomReadExtraParam(TypeList<test::CancelArg>, ServerContext& server_context)
{
    if (!server_context.request_mutex) {
        // Not an async method means this method will run on the event loop
        // thread which fires cancellations, just return a dummy function.
        return [](test::CancelFn) { return test::CancelGuard{}; };
    }
    return [&server_context](test::CancelFn fn) {
        {
            const Lock lock{*server_context.request_mutex};
            if (!server_context.request_canceled) {
                server_context.cancel_fn = std::move(fn);
                return test::CancelGuard{[&server_context] {
                    const Lock lock{*server_context.request_mutex};
                    server_context.cancel_fn = nullptr;
                }};
            }
        }
        fn();
        return test::CancelGuard{};
    };
}
} // namespace mp

#endif // MP_TEST_FOO_TYPES_H

// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPE_FUNCTION_H
#define MP_PROXY_TYPE_FUNCTION_H

#include <mp/util.h>

namespace mp {
//! Adapter to convert ProxyCallback object call to function object call.
template <typename Result, typename... Args>
class ProxyCallbackImpl final : public ProxyCallback<std::function<Result(Args...)>>
{
    using Fn = std::function<Result(Args...)>;
    Fn m_fn;

public:
    ProxyCallbackImpl(Fn fn) : m_fn(std::move(fn)) {}
    Result call(Args&&... args) override { return m_fn(std::forward<Args>(args)...); }
};

template <typename Value, typename FnR, typename... FnParams, typename Output>
void CustomBuildField(TypeList<std::function<FnR(FnParams...)>>,
    Priority<1>,
    InvokeContext& invoke_context,
    Value& value,
    Output&& output)
{
    if (value) {
        using Interface = typename decltype(output.get())::Calls;
        using Callback = ProxyCallbackImpl<FnR, FnParams...>;
        output.set(kj::heap<ProxyServer<Interface>>(
            std::make_shared<Callback>(std::forward<Value>(value)), invoke_context.connection));
    }
}

// ProxyCallFn class is needed because c++11 doesn't support auto lambda parameters.
// It's equivalent c++14: [invoke_context](auto&& params) {
// invoke_context->call(std::forward<decltype(params)>(params)...)
template <typename InvokeContext>
struct ProxyCallFn
{
    InvokeContext m_proxy;

    template <typename... CallParams>
    decltype(auto) operator()(CallParams&&... params) { return this->m_proxy->call(std::forward<CallParams>(params)...); }
};

template <typename FnR, typename... FnParams, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<std::function<FnR(FnParams...)>>,
    Priority<1>,
    InvokeContext& invoke_context,
    Input&& input,
    ReadDest&& read_dest)
{
    if (input.has()) {
        using Interface = typename Decay<decltype(input.get())>::Calls;
        auto client = std::make_shared<ProxyClient<Interface>>(
            input.get(), &invoke_context.connection, /* destroy_connection= */ false);
        return read_dest.construct(ProxyCallFn<decltype(client)>{std::move(client)});
    }
    return read_dest.construct();
};
} // namespace mp

#endif // MP_PROXY_TYPE_FUNCTION_H

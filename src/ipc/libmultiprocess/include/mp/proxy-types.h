// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPES_H
#define MP_PROXY_TYPES_H

#include <mp/proxy-io.h>

#include <exception>
#include <optional>
#include <set>
#include <typeindex>
#include <vector>

namespace mp {

template <typename Value>
class ValueField
{
public:
    ValueField(Value& value) : m_value(value) {}
    ValueField(Value&& value) : m_value(value) {}
    Value& m_value;

    Value& get() { return m_value; }
    Value& init() { return m_value; }
    bool has() { return true; }
};

template <typename Accessor, typename Struct>
struct StructField
{
    template <typename S>
    StructField(S& struct_) : m_struct(struct_)
    {
    }
    Struct& m_struct;

    decltype(auto) get() const { return Accessor::get(this->m_struct); }

    bool has() const {
      if constexpr (Accessor::optional) {
        return Accessor::getHas(m_struct);
      } else if constexpr (Accessor::boxed) {
        return Accessor::has(m_struct);
      } else {
        return true;
      }
    }

    bool want() const {
      if constexpr (Accessor::requested) {
        return Accessor::getWant(m_struct);
      } else {
        return true;
      }
    }

    template <typename... Args> decltype(auto) set(Args &&...args) const {
      return Accessor::set(this->m_struct, std::forward<Args>(args)...);
    }

    template <typename... Args> decltype(auto) init(Args &&...args) const {
      return Accessor::init(this->m_struct, std::forward<Args>(args)...);
    }

    void setHas() const {
      if constexpr (Accessor::optional) {
        Accessor::setHas(m_struct);
      }
    }

    void setWant() const {
      if constexpr (Accessor::requested) {
        Accessor::setWant(m_struct);
      }
    }
};



// Destination parameter type that can be passed to ReadField function as an
// alternative to ReadDestUpdate. It allows the ReadField implementation to call
// the provided emplace_fn function with constructor arguments, so it only needs
// to determine the arguments, and can let the emplace function decide how to
// actually construct the read destination object. For example, if a std::string
// is being read, the ReadField call will call the custom emplace_fn with char*
// and size_t arguments, and the emplace function can decide whether to call the
// constructor via the operator or make_shared or emplace or just return a
// temporary string that is moved from.
template <typename LocalType, typename EmplaceFn>
struct ReadDestEmplace
{
    ReadDestEmplace(TypeList<LocalType>, EmplaceFn emplace_fn) : m_emplace_fn(std::move(emplace_fn)) {}

    //! Simple case. If ReadField implementation calls this construct() method
    //! with constructor arguments, just pass them on to the emplace function.
    template <typename... Args>
    decltype(auto) construct(Args&&... args)
    {
        return m_emplace_fn(std::forward<Args>(args)...);
    }

    //! More complicated case. If ReadField implementation works by calling this
    //! update() method, adapt it call construct() instead. This requires
    //! LocalType to have a default constructor to create new object that can be
    //! passed to update()
    template <typename UpdateFn>
    decltype(auto) update(UpdateFn&& update_fn)
    {
        if constexpr (std::is_const_v<std::remove_reference_t<std::invoke_result_t<EmplaceFn>>>) {
            // If destination type is const, default construct temporary
            // to pass to update, then call move constructor via construct() to
            // move from that temporary.
            std::remove_cv_t<LocalType> temp;
            update_fn(temp);
            return construct(std::move(temp));
        } else {
            // Default construct object and pass it to update_fn.
            decltype(auto) temp = construct();
            update_fn(temp);
            return temp;
        }
    }
    EmplaceFn m_emplace_fn;
};

//! Helper function to create a ReadDestEmplace object that constructs a
//! temporary, ReadField can return.
template <typename LocalType>
auto ReadDestTemp()
{
    return ReadDestEmplace{TypeList<LocalType>(), [](auto&&... args) -> decltype(auto) {
        return LocalType{std::forward<decltype(args)>(args)...};
    }};
}

//! Destination parameter type that can be passed to ReadField function as an
//! alternative to ReadDestEmplace. Instead of requiring an emplace callback to
//! construct a new value, it just takes a reference to an existing value and
//! assigns a new value to it.
template <typename Value>
struct ReadDestUpdate
{
    ReadDestUpdate(Value& value) : m_value(value) {}

    //! Simple case. If ReadField works by calling update() just forward arguments to update_fn.
    template <typename UpdateFn>
    Value& update(UpdateFn&& update_fn)
    {
        update_fn(m_value);
        return m_value;
    }

    //! More complicated case. If ReadField works by calling construct(), need
    //! to reconstruct m_value in place.
    template <typename... Args>
    Value& construct(Args&&... args)
    {
        m_value.~Value();
        new (&m_value) Value(std::forward<Args>(args)...);
        return m_value;
    }

    Value& m_value;
};

template <typename... LocalTypes, typename... Args>
decltype(auto) ReadField(TypeList<LocalTypes...>, Args&&... args)
{
    return CustomReadField(TypeList<RemoveCvRef<LocalTypes>...>(), Priority<2>(), std::forward<Args>(args)...);
}

template <typename LocalType, typename Input>
void ThrowField(TypeList<LocalType>, InvokeContext& invoke_context, Input&& input)
{
    ReadField(
        TypeList<LocalType>(), invoke_context, input, ReadDestEmplace(TypeList<LocalType>(),
            [](auto&& ...args) -> const LocalType& { throw LocalType{std::forward<decltype(args)>(args)...}; }));
}

//! Special case for generic std::exception. It's an abstract type so it can't
//! be created directly. Rethrow as std::runtime_error so callers expecting it
//! will still catch it.
template <typename Input>
void ThrowField(TypeList<std::exception>, InvokeContext& invoke_context, Input&& input)
{
    auto data = input.get();
    throw std::runtime_error(std::string(CharCast(data.begin()), data.size()));
}

template <typename... Values>
bool CustomHasValue(InvokeContext& invoke_context, const Values&... value)
{
    return true;
}

template <typename... LocalTypes, typename Context, typename... Values, typename Output>
void BuildField(TypeList<LocalTypes...>, Context& context, Output&& output, Values&&... values)
{
    if (CustomHasValue(context, values...)) {
        CustomBuildField(TypeList<LocalTypes...>(), Priority<3>(), context, std::forward<Values>(values)...,
            std::forward<Output>(output));
    }
}

// Adapter to let BuildField overloads methods work set & init list elements as
// if they were fields of a struct. If BuildField is changed to use some kind of
// accessor class instead of calling method pointers, then then maybe this could
// go away or be simplified, because would no longer be a need to return
// ListOutput method pointers emulating capnp struct method pointers..
template <typename ListType>
struct ListOutput;

template <typename T, ::capnp::Kind kind>
struct ListOutput<::capnp::List<T, kind>>
{
    using Builder = typename ::capnp::List<T, kind>::Builder;

    ListOutput(Builder& builder, size_t index) : m_builder(builder), m_index(index) {}
    Builder& m_builder;
    size_t m_index;

    // clang-format off
    decltype(auto) get() const { return this->m_builder[this->m_index]; }
    decltype(auto) init() const { return this->m_builder[this->m_index]; }
    template<typename B = Builder, typename Arg> decltype(auto) set(Arg&& arg) const { return static_cast<B&>(this->m_builder).set(m_index, std::forward<Arg>(arg)); }
    template<typename B = Builder, typename Arg> decltype(auto) init(Arg&& arg) const { return static_cast<B&>(this->m_builder).init(m_index, std::forward<Arg>(arg)); }
    // clang-format on
};

template <typename LocalType, typename Value, typename Output>
void CustomBuildField(TypeList<LocalType>, Priority<0>, InvokeContext& invoke_context, Value&& value, Output&& output)
{
    output.set(BuildPrimitive(invoke_context, std::forward<Value>(value), TypeList<decltype(output.get())>()));
}

//! PassField override for callable interface reference arguments.
template <typename Accessor, typename LocalType, typename ServerContext, typename Fn, typename... Args>
auto PassField(Priority<1>, TypeList<LocalType&>, ServerContext& server_context, Fn&& fn, Args&&... args)
    -> Require<typename decltype(Accessor::get(server_context.call_context.getParams()))::Calls>
{
    // Just create a temporary ProxyClient if argument is a reference to an
    // interface client. If argument needs to have a longer lifetime and not be
    // destroyed after this call, a CustomPassField overload can be implemented
    // to bypass this code, and a custom ProxyServerMethodTraits overload can be
    // implemented in order to read the capability pointer out of params and
    // construct a ProxyClient with a longer lifetime.
    const auto& params = server_context.call_context.getParams();
    const auto& input = Make<StructField, Accessor>(params);
    using Interface = typename Decay<decltype(input.get())>::Calls;
    auto param = std::make_unique<ProxyClient<Interface>>(input.get(), server_context.proxy_server.m_context.connection, false);
    fn.invoke(server_context, std::forward<Args>(args)..., *param);
}

template <typename... Args>
void MaybeBuildField(std::true_type, Args&&... args)
{
    BuildField(std::forward<Args>(args)...);
}
template <typename... Args>
void MaybeBuildField(std::false_type, Args&&...)
{
}
template <typename... Args>
void MaybeReadField(std::true_type, Args&&... args)
{
    ReadField(std::forward<Args>(args)...);
}
template <typename... Args>
void MaybeReadField(std::false_type, Args&&...)
{
}

template <typename LocalType, typename Value, typename Output>
void MaybeSetWant(TypeList<LocalType*>, Priority<1>, const Value& value, Output&& output)
{
    if (value) {
        output.setWant();
    }
}

template <typename LocalTypes, typename... Args>
void MaybeSetWant(LocalTypes, Priority<0>, const Args&...)
{
}

//! Default PassField implementation calling MaybeReadField/MaybeBuildField.
template <typename Accessor, typename LocalType, typename ServerContext, typename Fn, typename... Args>
void PassField(Priority<0>, TypeList<LocalType>, ServerContext& server_context, Fn&& fn, Args&&... args)
{
    InvokeContext& invoke_context = server_context;
    using ArgType = RemoveCvRef<LocalType>;
    std::optional<ArgType> param;
    const auto& params = server_context.call_context.getParams();
    MaybeReadField(std::integral_constant<bool, Accessor::in>(), TypeList<ArgType>(), invoke_context,
        Make<StructField, Accessor>(params), ReadDestEmplace(TypeList<ArgType>(), [&](auto&&... args) -> auto& {
            param.emplace(std::forward<decltype(args)>(args)...);
            return *param;
        }));
    if constexpr (Accessor::in) {
        assert(param);
    } else {
        if (!param) param.emplace();
    }
    fn.invoke(server_context, std::forward<Args>(args)..., static_cast<LocalType&&>(*param));
    auto&& results = server_context.call_context.getResults();
    MaybeBuildField(std::integral_constant<bool, Accessor::out>(), TypeList<LocalType>(), invoke_context,
        Make<StructField, Accessor>(results), *param);
}

//! Default PassField implementation for count(0) arguments, calling ReadField/BuildField
template <typename Accessor, typename ServerContext, typename Fn, typename... Args>
void PassField(Priority<0>, TypeList<>, ServerContext& server_context, const Fn& fn, Args&&... args)
{
    const auto& params = server_context.call_context.getParams();
    const auto& input = Make<StructField, Accessor>(params);
    ReadField(TypeList<>(), server_context, input);
    fn.invoke(server_context, std::forward<Args>(args)...);
    auto&& results = server_context.call_context.getResults();
    BuildField(TypeList<>(), server_context, Make<StructField, Accessor>(results));
}

template <typename Derived, size_t N = 0>
struct IterateFieldsHelper
{
    template <typename Arg1, typename Arg2, typename ParamList, typename NextFn, typename... NextFnArgs>
    void handleChain(Arg1& arg1, Arg2& arg2, ParamList, NextFn&& next_fn, NextFnArgs&&... next_fn_args)
    {
        using S = Split<N, ParamList>;
        handleChain(arg1, arg2, typename S::First());
        next_fn.handleChain(arg1, arg2, typename S::Second(),
            std::forward<NextFnArgs>(next_fn_args)...);
    }

    template <typename Arg1, typename Arg2, typename ParamList>
    void handleChain(Arg1& arg1, Arg2& arg2, ParamList)
    {
        static_cast<Derived*>(this)->handleField(arg1, arg2, ParamList());
    }
private:
    IterateFieldsHelper() = default;
    friend Derived;
};

struct IterateFields : IterateFieldsHelper<IterateFields, 0>
{
    template <typename Arg1, typename Arg2, typename ParamList>
    void handleField(Arg1&&, Arg2&&, ParamList)
    {
    }
};

template <typename Exception, typename Accessor>
struct ClientException
{
    struct BuildParams : IterateFieldsHelper<BuildParams, 0>
    {
        template <typename Params, typename ParamList>
        void handleField(InvokeContext& invoke_context, Params& params, ParamList)
        {
        }

        BuildParams(ClientException* client_exception) : m_client_exception(client_exception) {}
        ClientException* m_client_exception;
    };

    struct ReadResults : IterateFieldsHelper<ReadResults, 0>
    {
        template <typename Results, typename ParamList>
        void handleField(InvokeContext& invoke_context, Results& results, ParamList)
        {
            StructField<Accessor, Results> input(results);
            if (input.has()) {
                ThrowField(TypeList<Exception>(), invoke_context, input);
            }
        }

        ReadResults(ClientException* client_exception) : m_client_exception(client_exception) {}
        ClientException* m_client_exception;
    };
};

template <typename Accessor, typename... Types>
struct ClientParam
{
    ClientParam(Types&&... values) : m_values{std::forward<Types>(values)...} {}

    struct BuildParams : IterateFieldsHelper<BuildParams, sizeof...(Types)>
    {
        template <typename Params, typename ParamList>
        void handleField(ClientInvokeContext& invoke_context, Params& params, ParamList)
        {
            auto const fun = [&]<typename... Values>(Values&&... values) {
                MaybeSetWant(
                    ParamList(), Priority<1>(), values..., Make<StructField, Accessor>(params));
                MaybeBuildField(std::integral_constant<bool, Accessor::in>(), ParamList(), invoke_context,
                    Make<StructField, Accessor>(params), std::forward<Values>(values)...);
            };

            // Note: The m_values tuple just consists of lvalue and rvalue
            // references, so calling std::move doesn't change the tuple, it
            // just causes std::apply to call the std::get overload that returns
            // && instead of &, so rvalue references are preserved and not
            // turned into lvalue references. This allows the BuildField call to
            // move from the argument if it is an rvalue reference or was passed
            // by value.
            std::apply(fun, std::move(m_client_param->m_values));
        }

        BuildParams(ClientParam* client_param) : m_client_param(client_param) {}
        ClientParam* m_client_param;
    };

    struct ReadResults : IterateFieldsHelper<ReadResults, sizeof...(Types)>
    {
        template <typename Results, typename... Params>
        void handleField(ClientInvokeContext& invoke_context, Results& results, TypeList<Params...>)
        {
            auto const fun = [&]<typename... Values>(Values&&... values) {
                MaybeReadField(std::integral_constant<bool, Accessor::out>(), TypeList<Decay<Params>...>(), invoke_context,
                    Make<StructField, Accessor>(results), ReadDestUpdate(values)...);
            };

            std::apply(fun, m_client_param->m_values);
        }

        ReadResults(ClientParam* client_param) : m_client_param(client_param) {}
        ClientParam* m_client_param;
    };

    std::tuple<Types&&...> m_values;
};

template <typename Accessor, typename... Types>
ClientParam<Accessor, Types...> MakeClientParam(Types&&... values)
{
    return {std::forward<Types>(values)...};
}

struct ServerCall
{
    // FIXME: maybe call call_context.releaseParams()
    template <typename ServerContext, typename... Args>
    decltype(auto) invoke(ServerContext& server_context, TypeList<>, Args&&... args) const
    {
        return ProxyServerMethodTraits<typename decltype(server_context.call_context.getParams())::Reads>::invoke(
            server_context,
            std::forward<Args>(args)...);
    }
};

struct ServerDestroy
{
    template <typename ServerContext, typename... Args>
    void invoke(ServerContext& server_context, TypeList<>, Args&&... args) const
    {
        server_context.proxy_server.invokeDestroy(std::forward<Args>(args)...);
    }
};

template <typename Accessor, typename Parent>
struct ServerRet : Parent
{
    ServerRet(Parent parent) : Parent(parent) {}

    template <typename ServerContext, typename... Args>
    void invoke(ServerContext& server_context, TypeList<>, Args&&... args) const
    {
        auto&& result = Parent::invoke(server_context, TypeList<>(), std::forward<Args>(args)...);
        auto&& results = server_context.call_context.getResults();
        InvokeContext& invoke_context = server_context;
        BuildField(TypeList<decltype(result)>(), invoke_context, Make<StructField, Accessor>(results),
            std::forward<decltype(result)>(result));
    }
};

template <typename Exception, typename Accessor, typename Parent>
struct ServerExcept : Parent
{
    ServerExcept(Parent parent) : Parent(parent) {}

    template <typename ServerContext, typename... Args>
    void invoke(ServerContext& server_context, TypeList<>, Args&&... args) const
    {
        try {
            return Parent::invoke(server_context, TypeList<>(), std::forward<Args>(args)...);
        } catch (const Exception& exception) {
            auto&& results = server_context.call_context.getResults();
            BuildField(TypeList<Exception>(), server_context, Make<StructField, Accessor>(results), exception);
        }
    }
};

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

template <class Accessor>
void CustomPassField();

//! PassField override calling CustomPassField function, if it exists.
//! Defining a CustomPassField or CustomPassMessage overload is useful for
//! input/output parameters. If an overload is not defined these parameters will
//! just be deserialized on the server side with ReadField into a temporary
//! variable, then the server method will be called passing the temporary
//! variable as a parameter, then the temporary variable will be serialized and
//! sent back to the client with BuildField. But if a PassField or PassMessage
//! overload is defined, the overload is called with a callback to invoke and
//! pass parameters to the server side function, and run arbitrary code before
//! and after invoking the function.
template <typename Accessor, typename... Args>
auto PassField(Priority<2>, Args&&... args) -> decltype(CustomPassField<Accessor>(std::forward<Args>(args)...))
{
    return CustomPassField<Accessor>(std::forward<Args>(args)...);
};

template <int argc, typename Accessor, typename Parent>
struct ServerField : Parent
{
    ServerField(Parent parent) : Parent(parent) {}

    const Parent& parent() const { return *this; }

    template <typename ServerContext, typename ArgTypes, typename... Args>
    decltype(auto) invoke(ServerContext& server_context, ArgTypes, Args&&... args) const
    {
        return PassField<Accessor>(Priority<2>(),
            typename Split<argc, ArgTypes>::First(),
            server_context,
            this->parent(),
            typename Split<argc, ArgTypes>::Second(),
            std::forward<Args>(args)...);
    }
};

template <int argc, typename Accessor, typename Parent>
ServerField<argc, Accessor, Parent> MakeServerField(Parent parent)
{
    return {parent};
}

template <typename Request>
struct CapRequestTraits;

template <typename _Params, typename _Results>
struct CapRequestTraits<::capnp::Request<_Params, _Results>>
{
    using Params = _Params;
    using Results = _Results;
};

//! Entry point called by all generated ProxyClient destructors. This only logs
//! the object destruction. The actual cleanup happens in the ProxyClient base
//! destructor.
template <typename Client>
void clientDestroy(Client& client)
{
    if (client.m_context.connection) {
        MP_LOG(*client.m_context.loop, Log::Info) << "IPC client destroy " << typeid(client).name();
    } else {
        KJ_LOG(INFO, "IPC interrupted client destroy", typeid(client).name());
    }
}

template <typename Server>
void serverDestroy(Server& server)
{
    MP_LOG(*server.m_context.loop, Log::Info) << "IPC server destroy " << typeid(server).name();
}

//! Entry point called by generated client code that looks like:
//!
//! ProxyClient<ClassName>::M0::Result ProxyClient<ClassName>::methodName(M0::Param<0> arg0, M0::Param<1> arg1) {
//!     typename M0::Result result;
//!     clientInvoke(*this, &InterfaceName::Client::methodNameRequest, MakeClientParam<...>(M0::Fwd<0>(arg0)), MakeClientParam<...>(M0::Fwd<1>(arg1)), MakeClientParam<...>(result));
//!     return result;
//! }
//!
//! Ellipses above are where generated Accessor<> type declarations are inserted.
template <typename ProxyClient, typename GetRequest, typename... FieldObjs>
void clientInvoke(ProxyClient& proxy_client, const GetRequest& get_request, FieldObjs&&... fields)
{
    if (!g_thread_context.waiter) {
        assert(g_thread_context.thread_name.empty());
        g_thread_context.thread_name = ThreadName(proxy_client.m_context.loop->m_exe_name);
        // If next assert triggers, it means clientInvoke is being called from
        // the capnp event loop thread. This can happen when a ProxyServer
        // method implementation that runs synchronously on the event loop
        // thread tries to make a blocking callback to the client. Any server
        // method that makes a blocking callback or blocks in general needs to
        // run asynchronously off the event loop thread. This is easy to fix by
        // just adding a 'context :Proxy.Context' argument to the capnp method
        // declaration so the server method runs in a dedicated thread.
        assert(!g_thread_context.loop_thread);
        g_thread_context.waiter = std::make_unique<Waiter>();
        MP_LOGPLAIN(*proxy_client.m_context.loop, Log::Info)
            << "{" << g_thread_context.thread_name
            << "} IPC client first request from current thread, constructing waiter";
    }
    ThreadContext& thread_context{g_thread_context};
    std::optional<ClientInvokeContext> invoke_context; // Must outlive waiter->wait() call below
    std::exception_ptr exception;
    std::string kj_exception;
    bool done = false;
    const char* disconnected = nullptr;
    proxy_client.m_context.loop->sync([&]() {
        if (!proxy_client.m_context.connection) {
            const Lock lock(thread_context.waiter->m_mutex);
            done = true;
            disconnected = "IPC client method called after disconnect.";
            thread_context.waiter->m_cv.notify_all();
            return;
        }

        auto request = (proxy_client.m_client.*get_request)(nullptr);
        using Request = CapRequestTraits<decltype(request)>;
        using FieldList = typename ProxyClientMethodTraits<typename Request::Params>::Fields;
        invoke_context.emplace(*proxy_client.m_context.connection, thread_context);
        IterateFields().handleChain(*invoke_context, request, FieldList(), typename FieldObjs::BuildParams{&fields}...);
        MP_LOGPLAIN(*proxy_client.m_context.loop, Log::Debug)
            << "{" << thread_context.thread_name << "} IPC client send "
            << TypeName<typename Request::Params>();
        MP_LOGPLAIN(*proxy_client.m_context.loop, Log::Trace)
            << "send data: " << LogEscape(request.toString(), proxy_client.m_context.loop->m_log_opts.max_chars);

        proxy_client.m_context.loop->m_task_set->add(request.send().then(
            [&](::capnp::Response<typename Request::Results>&& response) {
                MP_LOGPLAIN(*proxy_client.m_context.loop, Log::Debug)
                    << "{" << thread_context.thread_name << "} IPC client recv "
                    << TypeName<typename Request::Results>();
                MP_LOGPLAIN(*proxy_client.m_context.loop, Log::Trace)
                    << "recv data: " << LogEscape(response.toString(), proxy_client.m_context.loop->m_log_opts.max_chars);
                try {
                    IterateFields().handleChain(
                        *invoke_context, response, FieldList(), typename FieldObjs::ReadResults{&fields}...);
                } catch (...) {
                    exception = std::current_exception();
                }
                const Lock lock(thread_context.waiter->m_mutex);
                done = true;
                thread_context.waiter->m_cv.notify_all();
            },
            [&](const ::kj::Exception& e) {
                if (e.getType() == ::kj::Exception::Type::DISCONNECTED) {
                    disconnected = "IPC client method call interrupted by disconnect.";
                } else {
                    kj_exception = kj::str("kj::Exception: ", e).cStr();
                    MP_LOGPLAIN(*proxy_client.m_context.loop, Log::Info)
                        << "{" << thread_context.thread_name << "} IPC client exception " << kj_exception;
                }
                const Lock lock(thread_context.waiter->m_mutex);
                done = true;
                thread_context.waiter->m_cv.notify_all();
            }));
    });

    Lock lock(thread_context.waiter->m_mutex);
    thread_context.waiter->wait(lock, [&done]() { return done; });
    if (exception) std::rethrow_exception(exception);
    if (!kj_exception.empty()) MP_LOGPLAIN(*proxy_client.m_context.loop, Log::Raise) << kj_exception;
    if (disconnected) MP_LOGPLAIN(*proxy_client.m_context.loop, Log::Raise) << disconnected;
}

//! Invoke callable `fn()` that may return void. If it does return void, replace
//! return value with value of `ret()`. This is useful for avoiding code
//! duplication and branching in generic code that forwards calls to functions.
template <typename Fn, typename Ret>
auto ReplaceVoid(Fn&& fn, Ret&& ret)
{
    if constexpr (std::is_same_v<decltype(fn()), void>) {
        fn();
        return ret();
    } else {
        return fn();
    }
}

extern std::atomic<int> server_reqs;

//! Entry point called by generated server code that looks like:
//!
//! kj::Promise<void> ProxyServer<InterfaceName>::methodName(CallContext call_context) {
//!     return serverInvoke(*this, call_context, MakeServerField<0, ...>(MakeServerField<1, ...>(Make<ServerRet, ...>(ServerCall()))));
//! }
//!
//! Ellipses above are where generated Accessor<> type declarations are inserted.
template <typename Server, typename CallContext, typename Fn>
kj::Promise<void> serverInvoke(Server& server, CallContext& call_context, Fn fn)
{
    auto params = call_context.getParams();
    using Params = decltype(params);
    using Results = typename decltype(call_context.getResults())::Builds;

    int req = ++server_reqs;
    MP_LOG(*server.m_context.loop, Log::Debug) << "IPC server recv request  #" << req << " "
                                     << TypeName<typename Params::Reads>();
    MP_LOG(*server.m_context.loop, Log::Trace) << "request data: "
        << LogEscape(params.toString(), server.m_context.loop->m_log_opts.max_chars);

    try {
        using ServerContext = ServerInvokeContext<Server, CallContext>;
        using ArgList = typename ProxyClientMethodTraits<typename Params::Reads>::Params;
        ServerContext server_context{server, call_context, req};
        // ReplaceVoid is used to support fn.invoke implementations that
        // execute asynchronously and return promises, as well as
        // implementations that execute synchronously and return void. The
        // invoke function will be synchronous by default, but asynchronous if
        // an mp.Context argument is passed, and the mp.Context PassField
        // overload returns a promise executing the request in a worker thread
        // and waiting for it to complete.
        return ReplaceVoid([&]() { return fn.invoke(server_context, ArgList()); },
            [&]() { return kj::Promise<CallContext>(kj::mv(call_context)); })
            .then([&server, req](CallContext call_context) {
                MP_LOG(*server.m_context.loop, Log::Debug) << "IPC server send response #" << req << " " << TypeName<Results>();
                MP_LOG(*server.m_context.loop, Log::Trace) << "response data: "
                    << LogEscape(call_context.getResults().toString(), server.m_context.loop->m_log_opts.max_chars);
            });
    } catch (const std::exception& e) {
        MP_LOG(*server.m_context.loop, Log::Error) << "IPC server unhandled exception: " << e.what();
        throw;
    } catch (...) {
        MP_LOG(*server.m_context.loop, Log::Error) << "IPC server unhandled exception";
        throw;
    }
}

//! Map to convert client interface pointers to ProxyContext struct references
//! at runtime using typeids.
struct ProxyTypeRegister {
    template<typename Interface>
    ProxyTypeRegister(TypeList<Interface>) {
        types().emplace(typeid(Interface), [](void* iface) -> ProxyContext& { return static_cast<typename mp::ProxyType<Interface>::Client&>(*static_cast<Interface*>(iface)).m_context; });
    }
    using Types = std::map<std::type_index, ProxyContext&(*)(void*)>;
    static Types& types() { static Types types; return types; }
};

} // namespace mp

#endif // MP_PROXY_TYPES_H

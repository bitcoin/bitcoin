// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_UTIL_H
#define MP_UTIL_H

#include <capnp/schema.h>
#include <cstddef>
#include <functional>
#include <future>
#include <kj/common.h>
#include <kj/exception.h>
#include <kj/string-tree.h>
#include <memory>
#include <string.h>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#ifdef WIN32
#include <winsock2.h>
#endif

namespace mp {

//! Generic utility functions used by capnp code.

//! Type holding a list of types.
//!
//! Example:
//!   TypeList<int, bool, void>
template <typename... Types>
struct TypeList
{
    static constexpr size_t size = sizeof...(Types);
};

//! Construct a template class value by deducing template arguments from the
//! types of constructor arguments, so they don't need to be specified manually.
//!
//! Uses of this can go away with class template deduction in C++17
//! (https://en.cppreference.com/w/cpp/language/class_template_argument_deduction)
//!
//! Example:
//!   Make<std::pair>(5, true) // Constructs std::pair<int, bool>(5, true);
template <template <typename...> class Class, typename... Types, typename... Args>
Class<Types..., std::remove_reference_t<Args>...> Make(Args&&... args)
{
    return Class<Types..., std::remove_reference_t<Args>...>{std::forward<Args>(args)...};
}

//! Type helper splitting a TypeList into two halves at position index.
//!
//! Example:
//!   is_same<TypeList<int, double>, Split<2, TypeList<int, double, float, bool>>::First>
//!   is_same<TypeList<float, bool>, Split<2, TypeList<int, double, float, bool>>::Second>
template <std::size_t index, typename List, typename _First = TypeList<>, bool done = index == 0>
struct Split;

//! Specialization of above (base case)
template <typename _Second, typename _First>
struct Split<0, _Second, _First, true>
{
    using First = _First;
    using Second = _Second;
};

//! Specialization of above (recursive case)
template <std::size_t index, typename Type, typename... _Second, typename... _First>
struct Split<index, TypeList<Type, _Second...>, TypeList<_First...>, false>
{
    using _Next = Split<index - 1, TypeList<_Second...>, TypeList<_First..., Type>>;
    using First = typename _Next::First;
    using Second = typename _Next::Second;
};

//! Type helper giving return type of a callable type.
template <typename Callable>
using ResultOf = decltype(std::declval<Callable>()());

//! Substitutue for std::remove_cvref_t
template <typename T>
using RemoveCvRef = std::remove_cv_t<std::remove_reference_t<T>>;

//! Type helper abbreviating std::decay.
template <typename T>
using Decay = std::decay_t<T>;

//! SFINAE helper, see using Require below.
template <typename SfinaeExpr, typename Result_>
struct _Require
{
    using Result = Result_;
};

//! SFINAE helper, basically the same as to C++17's void_t, but allowing types other than void to be returned.
template <typename SfinaeExpr, typename Result = void>
using Require = typename _Require<SfinaeExpr, Result>::Result;

//! Function parameter type for prioritizing overloaded function calls that
//! would otherwise be ambiguous.
//!
//! Example:
//!   auto foo(Priority<1>) -> std::enable_if<>;
//!   auto foo(Priority<0>) -> void;
//!
//!   foo(Priority<1>());   // Calls higher priority overload if enabled.
template <int priority>
struct Priority : Priority<priority - 1>
{
};

//! Specialization of above (base case)
template <>
struct Priority<0>
{
};

//! Return capnp type name with filename prefix removed.
template <typename T>
const char* TypeName()
{
    // DisplayName string looks like
    // "interfaces/capnp/common.capnp:ChainNotifications.resendWalletTransactions$Results"
    // This discards the part of the string before the first ':' character.
    // Another alternative would be to use the displayNamePrefixLength field,
    // but this discards everything before the last '.' character, throwing away
    // the object name, which is useful.
    const char* display_name = ::capnp::Schema::from<T>().getProto().getDisplayName().cStr();
    const char* short_name = strchr(display_name, ':');
    return short_name ? short_name + 1 : display_name;
}

//! Analog to std::lock_guard that unlocks instead of locks.
template <typename Lock>
struct UnlockGuard
{
    UnlockGuard(Lock& lock) : m_lock(lock) { m_lock.unlock(); }
    ~UnlockGuard() { m_lock.lock(); }
    Lock& m_lock;
};

template <typename Lock, typename Callback>
void Unlock(Lock& lock, Callback&& callback)
{
    const UnlockGuard<Lock> unlock(lock);
    callback();
}

//! Needed for libc++/macOS compatibility. Lets code work with shared_ptr nothrow declaration
//! https://github.com/capnproto/capnproto/issues/553#issuecomment-328554603
template <typename T>
struct DestructorCatcher
{
    T value;
    template <typename... Params>
    DestructorCatcher(Params&&... params) : value(kj::fwd<Params>(params)...)
    {
    }
    ~DestructorCatcher() noexcept try {
    } catch (const kj::Exception& e) { // NOLINT(bugprone-empty-catch)
    }
};

//! Wrapper around callback function for compatibility with std::async.
//!
//! std::async requires callbacks to be copyable and requires noexcept
//! destructors, but this doesn't work well with kj types which are generally
//! move-only and not noexcept.
template <typename Callable>
struct AsyncCallable
{
    AsyncCallable(Callable&& callable) : m_callable(std::make_shared<DestructorCatcher<Callable>>(std::move(callable)))
    {
    }
    AsyncCallable(const AsyncCallable&) = default;
    AsyncCallable(AsyncCallable&&) = default;
    ~AsyncCallable() noexcept = default;
    ResultOf<Callable> operator()() const { return (m_callable->value)(); }
    mutable std::shared_ptr<DestructorCatcher<Callable>> m_callable;
};

//! Construct AsyncCallable object.
template <typename Callable>
AsyncCallable<std::remove_reference_t<Callable>> MakeAsyncCallable(Callable&& callable)
{
    return std::move(callable);
}

//! Format current thread name as "{exe_name}-{$pid}/{thread_name}-{$tid}".
std::string ThreadName(const char* exe_name);

//! Escape binary string for use in log so it doesn't trigger unicode decode
//! errors in python unit tests.
std::string LogEscape(const kj::StringTree& string);

#ifdef WIN32
using ProcessId = uintptr_t;
using SocketId = uintptr_t;
constexpr SocketId SocketError{INVALID_SOCKET};
#else
using ProcessId = int;
using SocketId = int;
constexpr SocketId SocketError{-1};
#endif

//! Information about parent process passed to child process.  On unix this is
//! just the inherited int file descriptor formatted as a string. On windows,
//! this is a path to a named path pipe the parent process will write
//! WSADuplicateSocket info to.
using ConnectInfo = std::string;

//! Callback type used by SpawnProcess below.
using ConnectInfoToArgsFn = std::function<std::vector<std::string>(const ConnectInfo&)>;

//! Create a socket pair that can be used to communicate within a process or
//! between parent and child processes.
std::array<SocketId, 2> SocketPair();

//! Spawn a new process that communicates with the current process over provided
//! socket argument. Calls connect_info_to_args callback with a connection
//! string that needs to be passed to the child process, and executes the
//! argv command line it returns. Returns child process id.
ProcessId SpawnProcess(SocketId socket, ConnectInfoToArgsFn&& connect_info_to_args);

//! Initialize spawned child process using the ConnectInfo string passed to it,
//! returning a socket id for communicating with the parent process.
SocketId StartSpawned(const ConnectInfo& connect_info);

//! Call execvp with vector args.
void ExecProcess(const std::vector<std::string>& args);

//! Wait for a process to exit and return its exit code.
int WaitProcess(ProcessId pid);

inline char* CharCast(char* c) { return c; }
inline char* CharCast(unsigned char* c) { return (char*)c; }
inline const char* CharCast(const char* c) { return c; }
inline const char* CharCast(const unsigned char* c) { return (const char*)c; }

} // namespace mp

#endif // MP_UTIL_H

// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_UTIL_H
#define MP_UTIL_H

#include <capnp/schema.h>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <functional>
#include <kj/string-tree.h>
#include <mutex>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

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

//! Convenient wrapper around std::variant<T*, T>
template <typename T>
struct PtrOrValue {
    std::variant<T*, T> data;

    template <typename... Args>
    PtrOrValue(T* ptr, Args&&... args) : data(ptr ? ptr : std::variant<T*, T>{std::in_place_type<T>, std::forward<Args>(args)...}) {}

    T& operator*() { return data.index() ? std::get<T>(data) : *std::get<T*>(data); }
    T* operator->() { return &**this; }
    T& operator*() const { return data.index() ? std::get<T>(data) : *std::get<T*>(data); }
    T* operator->() const { return &**this; }
};

// Annotated mutex and lock class (https://clang.llvm.org/docs/ThreadSafetyAnalysis.html)
#if defined(__clang__) && (!defined(SWIG))
#define MP_TSA(x)   __attribute__((x))
#else
#define MP_TSA(x)   // no-op
#endif

#define MP_CAPABILITY(x)        MP_TSA(capability(x))
#define MP_SCOPED_CAPABILITY    MP_TSA(scoped_lockable)
#define MP_REQUIRES(x)          MP_TSA(requires_capability(x))
#define MP_ACQUIRE(...)         MP_TSA(acquire_capability(__VA_ARGS__))
#define MP_RELEASE(...)         MP_TSA(release_capability(__VA_ARGS__))
#define MP_ASSERT_CAPABILITY(x) MP_TSA(assert_capability(x))
#define MP_GUARDED_BY(x)        MP_TSA(guarded_by(x))
#define MP_NO_TSA               MP_TSA(no_thread_safety_analysis)

class MP_CAPABILITY("mutex") Mutex {
public:
    void lock() MP_ACQUIRE() { m_mutex.lock(); }
    void unlock() MP_RELEASE() { m_mutex.unlock(); }

    std::mutex m_mutex;
};

class MP_SCOPED_CAPABILITY Lock {
public:
    explicit Lock(Mutex& m) MP_ACQUIRE(m) : m_lock(m.m_mutex) {}
    ~Lock() MP_RELEASE() = default;
    void unlock() MP_RELEASE() { m_lock.unlock(); }
    void lock() MP_ACQUIRE() { m_lock.lock(); }
    void assert_locked(Mutex& mutex) MP_ASSERT_CAPABILITY() MP_ASSERT_CAPABILITY(mutex)
    {
        assert(m_lock.mutex() == &mutex.m_mutex);
        assert(m_lock);
    }

    std::unique_lock<std::mutex> m_lock;
};

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

//! Format current thread name as "{exe_name}-{$pid}/{thread_name}-{$tid}".
std::string ThreadName(const char* exe_name);

//! Escape binary string for use in log so it doesn't trigger unicode decode
//! errors in python unit tests.
std::string LogEscape(const kj::StringTree& string);

//! Callback type used by SpawnProcess below.
using FdToArgsFn = std::function<std::vector<std::string>(int fd)>;

//! Spawn a new process that communicates with the current process over a socket
//! pair. Returns pid through an output argument, and file descriptor for the
//! local side of the socket. Invokes fd_to_args callback with the remote file
//! descriptor number which returns the command line arguments that should be
//! used to execute the process, and which should have the remote file
//! descriptor embedded in whatever format the child process expects.
int SpawnProcess(int& pid, FdToArgsFn&& fd_to_args);

//! Call execvp with vector args.
void ExecProcess(const std::vector<std::string>& args);

//! Wait for a process to exit and return its exit code.
int WaitProcess(int pid);

inline char* CharCast(char* c) { return c; }
inline char* CharCast(unsigned char* c) { return (char*)c; }
inline const char* CharCast(const char* c) { return c; }
inline const char* CharCast(const unsigned char* c) { return (const char*)c; }

} // namespace mp

#endif // MP_UTIL_H

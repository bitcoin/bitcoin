// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SYNC_H
#define BITCOIN_SYNC_H

#ifdef DEBUG_LOCKCONTENTION
#include <logging.h>
#include <logging/timer.h>
#endif

#include <threadsafety.h> // IWYU pragma: export
#include <util/macros.h>

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

////////////////////////////////////////////////
//                                            //
// THE SIMPLE DEFINITION, EXCLUDING DEBUG CODE //
//                                            //
////////////////////////////////////////////////

/*
RecursiveMutex mutex;
    std::recursive_mutex mutex;

LOCK(mutex);
    std::unique_lock<std::recursive_mutex> criticalblock(mutex);

LOCK2(mutex1, mutex2);
    std::unique_lock<std::recursive_mutex> criticalblock1(mutex1);
    std::unique_lock<std::recursive_mutex> criticalblock2(mutex2);

TRY_LOCK(mutex, name);
    std::unique_lock<std::recursive_mutex> name(mutex, std::try_to_lock_t);

ENTER_CRITICAL_SECTION(mutex); // no RAII
    mutex.lock();

LEAVE_CRITICAL_SECTION(mutex); // no RAII
    mutex.unlock();
 */

///////////////////////////////
//                           //
// THE ACTUAL IMPLEMENTATION //
//                           //
///////////////////////////////

#ifdef DEBUG_LOCKORDER
template <typename MutexType>
void EnterCritical(const char* pszName, const char* pszFile, int nLine, MutexType* cs, bool fTry = false);
void LeaveCritical();
void CheckLastCritical(void* cs, std::string& lockname, const char* guardname, const char* file, int line);
template <typename MutexType>
void AssertLockHeldInternal(const char* pszName, const char* pszFile, int nLine, MutexType* cs) EXCLUSIVE_LOCKS_REQUIRED(cs);
template <typename MutexType>
void AssertLockNotHeldInternal(const char* pszName, const char* pszFile, int nLine, MutexType* cs) LOCKS_EXCLUDED(cs);
void DeleteLock(void* cs);
bool LockStackEmpty();

/**
 * Call abort() if a potential lock order deadlock bug is detected, instead of
 * just logging information and throwing a logic_error. Defaults to true, and
 * set to false in DEBUG_LOCKORDER unit tests.
 */
extern bool g_debug_lockorder_abort;
#else
template <typename MutexType>
inline void EnterCritical(const char* pszName, const char* pszFile, int nLine, MutexType* cs, bool fTry = false) {}
inline void LeaveCritical() {}
inline void CheckLastCritical(void* cs, std::string& lockname, const char* guardname, const char* file, int line) {}
template <typename MutexType>
inline void AssertLockHeldInternal(const char* pszName, const char* pszFile, int nLine, MutexType* cs) EXCLUSIVE_LOCKS_REQUIRED(cs) {}
template <typename MutexType>
void AssertLockNotHeldInternal(const char* pszName, const char* pszFile, int nLine, MutexType* cs) LOCKS_EXCLUDED(cs) {}
inline void DeleteLock(void* cs) {}
inline bool LockStackEmpty() { return true; }
#endif

/**
 * Template mixin that adds -Wthread-safety locking annotations and lock order
 * checking to a subset of the mutex API.
 */
template <typename PARENT>
class LOCKABLE AnnotatedMixin : public PARENT
{
public:
    ~AnnotatedMixin() {
        DeleteLock((void*)this);
    }

    void lock() EXCLUSIVE_LOCK_FUNCTION()
    {
        PARENT::lock();
    }

    void unlock() UNLOCK_FUNCTION()
    {
        PARENT::unlock();
    }

    bool try_lock() EXCLUSIVE_TRYLOCK_FUNCTION(true)
    {
        return PARENT::try_lock();
    }

    using unique_lock = std::unique_lock<PARENT>;
#ifdef __clang__
    //! For negative capabilities in the Clang Thread Safety Analysis.
    //! A negative requirement uses the EXCLUSIVE_LOCKS_REQUIRED attribute, in conjunction
    //! with the ! operator, to indicate that a mutex should not be held.
    const AnnotatedMixin& operator!() const { return *this; }
#endif // __clang__
};

/**
 * Wrapped mutex: supports recursive locking, but no waiting
 * TODO: We should move away from using the recursive lock by default.
 */
using RecursiveMutex = AnnotatedMixin<std::recursive_mutex>;

/** Wrapped mutex: supports waiting but not recursive locking */
using Mutex = AnnotatedMixin<std::mutex>;

/** Different type to mark Mutex at global scope
 *
 * Thread safety analysis can't handle negative assertions about mutexes
 * with global scope well, so mark them with a separate type, and
 * eventually move all the mutexes into classes so they are not globally
 * visible.
 *
 * See: https://github.com/bitcoin/bitcoin/pull/20272#issuecomment-720755781
 */
class GlobalMutex : public Mutex { };

#define AssertLockHeld(cs) AssertLockHeldInternal(#cs, __FILE__, __LINE__, &cs)

inline void AssertLockNotHeldInline(const char* name, const char* file, int line, Mutex* cs) EXCLUSIVE_LOCKS_REQUIRED(!cs) { AssertLockNotHeldInternal(name, file, line, cs); }
inline void AssertLockNotHeldInline(const char* name, const char* file, int line, RecursiveMutex* cs) LOCKS_EXCLUDED(cs) { AssertLockNotHeldInternal(name, file, line, cs); }
inline void AssertLockNotHeldInline(const char* name, const char* file, int line, GlobalMutex* cs) LOCKS_EXCLUDED(cs) { AssertLockNotHeldInternal(name, file, line, cs); }
#define AssertLockNotHeld(cs) AssertLockNotHeldInline(#cs, __FILE__, __LINE__, &cs)

/** Wrapper around std::unique_lock style lock for MutexType. */
template <typename MutexType>
class SCOPED_LOCKABLE UniqueLock : public MutexType::unique_lock
{
private:
    using Base = typename MutexType::unique_lock;

    void Enter(const char* pszName, const char* pszFile, int nLine)
    {
        EnterCritical(pszName, pszFile, nLine, Base::mutex());
#ifdef DEBUG_LOCKCONTENTION
        if (Base::try_lock()) return;
        LOG_TIME_MICROS_WITH_CATEGORY(strprintf("lock contention %s, %s:%d", pszName, pszFile, nLine), BCLog::LOCK);
#endif
        Base::lock();
    }

    bool TryEnter(const char* pszName, const char* pszFile, int nLine)
    {
        EnterCritical(pszName, pszFile, nLine, Base::mutex(), true);
        if (Base::try_lock()) {
            return true;
        }
        LeaveCritical();
        return false;
    }

public:
    UniqueLock(MutexType& mutexIn, const char* pszName, const char* pszFile, int nLine, bool fTry = false) EXCLUSIVE_LOCK_FUNCTION(mutexIn) : Base(mutexIn, std::defer_lock)
    {
        if (fTry)
            TryEnter(pszName, pszFile, nLine);
        else
            Enter(pszName, pszFile, nLine);
    }

    UniqueLock(MutexType* pmutexIn, const char* pszName, const char* pszFile, int nLine, bool fTry = false) EXCLUSIVE_LOCK_FUNCTION(pmutexIn)
    {
        if (!pmutexIn) return;

        *static_cast<Base*>(this) = Base(*pmutexIn, std::defer_lock);
        if (fTry)
            TryEnter(pszName, pszFile, nLine);
        else
            Enter(pszName, pszFile, nLine);
    }

    ~UniqueLock() UNLOCK_FUNCTION()
    {
        if (Base::owns_lock())
            LeaveCritical();
    }

    operator bool()
    {
        return Base::owns_lock();
    }

protected:
    // needed for reverse_lock
    UniqueLock() = default;

public:
    /**
     * An RAII-style reverse lock. Unlocks on construction and locks on destruction.
     */
    class reverse_lock {
    public:
        explicit reverse_lock(UniqueLock& _lock, const char* _guardname, const char* _file, int _line) : lock(_lock), file(_file), line(_line) {
            CheckLastCritical((void*)lock.mutex(), lockname, _guardname, _file, _line);
            lock.unlock();
            LeaveCritical();
            lock.swap(templock);
        }

        ~reverse_lock() {
            templock.swap(lock);
            EnterCritical(lockname.c_str(), file.c_str(), line, lock.mutex());
            lock.lock();
        }

     private:
        reverse_lock(reverse_lock const&);
        reverse_lock& operator=(reverse_lock const&);

        UniqueLock& lock;
        UniqueLock templock;
        std::string lockname;
        const std::string file;
        const int line;
     };
     friend class reverse_lock;
};

#define REVERSE_LOCK(g) typename std::decay<decltype(g)>::type::reverse_lock UNIQUE_NAME(revlock)(g, #g, __FILE__, __LINE__)

// When locking a Mutex, require negative capability to ensure the lock
// is not already held
inline Mutex& MaybeCheckNotHeld(Mutex& cs) EXCLUSIVE_LOCKS_REQUIRED(!cs) LOCK_RETURNED(cs) { return cs; }
inline Mutex* MaybeCheckNotHeld(Mutex* cs) EXCLUSIVE_LOCKS_REQUIRED(!cs) LOCK_RETURNED(cs) { return cs; }

// When locking a GlobalMutex or RecursiveMutex, just check it is not
// locked in the surrounding scope.
template <typename MutexType>
inline MutexType& MaybeCheckNotHeld(MutexType& m) LOCKS_EXCLUDED(m) LOCK_RETURNED(m) { return m; }
template <typename MutexType>
inline MutexType* MaybeCheckNotHeld(MutexType* m) LOCKS_EXCLUDED(m) LOCK_RETURNED(m) { return m; }

#define LOCK(cs) UniqueLock UNIQUE_NAME(criticalblock)(MaybeCheckNotHeld(cs), #cs, __FILE__, __LINE__)
#define LOCK2(cs1, cs2)                                               \
    UniqueLock criticalblock1(MaybeCheckNotHeld(cs1), #cs1, __FILE__, __LINE__); \
    UniqueLock criticalblock2(MaybeCheckNotHeld(cs2), #cs2, __FILE__, __LINE__)
#define LOCK_ARGS(cs) MaybeCheckNotHeld(cs), #cs, __FILE__, __LINE__
#define TRY_LOCK(cs, name) UniqueLock name(LOCK_ARGS(cs), true)
#define WAIT_LOCK(cs, name) UniqueLock name(LOCK_ARGS(cs))

#define ENTER_CRITICAL_SECTION(cs)                            \
    {                                                         \
        EnterCritical(#cs, __FILE__, __LINE__, &cs); \
        (cs).lock();                                          \
    }

#define LEAVE_CRITICAL_SECTION(cs)                                          \
    {                                                                       \
        std::string lockname;                                               \
        CheckLastCritical((void*)(&cs), lockname, #cs, __FILE__, __LINE__); \
        (cs).unlock();                                                      \
        LeaveCritical();                                                    \
    }

//! Run code while locking a mutex.
//!
//! Examples:
//!
//!   WITH_LOCK(cs, shared_val = shared_val + 1);
//!
//!   int val = WITH_LOCK(cs, return shared_val);
//!
//! Note:
//!
//! Since the return type deduction follows that of decltype(auto), while the
//! deduced type of:
//!
//!   WITH_LOCK(cs, return {int i = 1; return i;});
//!
//! is int, the deduced type of:
//!
//!   WITH_LOCK(cs, return {int j = 1; return (j);});
//!
//! is &int, a reference to a local variable
//!
//! The above is detectable at compile-time with the -Wreturn-local-addr flag in
//! gcc and the -Wreturn-stack-address flag in clang, both enabled by default.
#define WITH_LOCK(cs, code) (MaybeCheckNotHeld(cs), [&]() -> decltype(auto) { LOCK(cs); code; }())

#endif // BITCOIN_SYNC_H

/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file Guards.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <mutex>
#include <condition_variable>
#include <atomic>
#pragma warning(push)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <boost/thread.hpp>
#pragma warning(pop)
#pragma GCC diagnostic pop

namespace dev
{

using Mutex = std::mutex;
using RecursiveMutex = std::recursive_mutex;
using SharedMutex = boost::shared_mutex;

using Guard = std::lock_guard<std::mutex>;
using UniqueGuard = std::unique_lock<std::mutex>;
using RecursiveGuard = std::lock_guard<std::recursive_mutex>;
using ReadGuard = boost::shared_lock<boost::shared_mutex>;
using UpgradableGuard = boost::upgrade_lock<boost::shared_mutex>;
using UpgradeGuard = boost::upgrade_to_unique_lock<boost::shared_mutex>;
using WriteGuard = boost::unique_lock<boost::shared_mutex>;

template <class GuardType, class MutexType>
struct GenericGuardBool: GuardType
{
	GenericGuardBool(MutexType& _m): GuardType(_m) {}
	bool b = true;
};
template <class MutexType>
struct GenericUnguardBool
{
	GenericUnguardBool(MutexType& _m): m(_m) { m.unlock(); }
	~GenericUnguardBool() { m.lock(); }
	bool b = true;
	MutexType& m;
};
template <class MutexType>
struct GenericUnguardSharedBool
{
	GenericUnguardSharedBool(MutexType& _m): m(_m) { m.unlock_shared(); }
	~GenericUnguardSharedBool() { m.lock_shared(); }
	bool b = true;
	MutexType& m;
};

template <class N>
class Notified
{
public:
	Notified() {}
	Notified(N const& _v): m_value(_v) {}
	Notified(Notified const&) = delete;
	Notified& operator=(N const& _v) { UniqueGuard l(m_mutex); m_value = _v; m_cv.notify_all(); return *this; }

	operator N() const { UniqueGuard l(m_mutex); return m_value; }

	void wait() const { N old; { UniqueGuard l(m_mutex); old = m_value; } waitNot(old); }
	void wait(N const& _v) const { UniqueGuard l(m_mutex); m_cv.wait(l, [&](){return m_value == _v;}); }
	void waitNot(N const& _v) const { UniqueGuard l(m_mutex); m_cv.wait(l, [&](){return m_value != _v;}); }
	template <class F> void wait(F const& _f) const { UniqueGuard l(m_mutex); m_cv.wait(l, _f); }

	template <class R, class P> void wait(std::chrono::duration<R, P> _d) const { N old; { UniqueGuard l(m_mutex); old = m_value; } waitNot(_d, old); }
	template <class R, class P> void wait(std::chrono::duration<R, P> _d, N const& _v) const { UniqueGuard l(m_mutex); m_cv.wait_for(l, _d, [&](){return m_value == _v;}); }
	template <class R, class P> void waitNot(std::chrono::duration<R, P> _d, N const& _v) const { UniqueGuard l(m_mutex); m_cv.wait_for(l, _d, [&](){return m_value != _v;}); }
	template <class R, class P, class F> void wait(std::chrono::duration<R, P> _d, F const& _f) const { UniqueGuard l(m_mutex); m_cv.wait_for(l, _d, _f); }

private:
	mutable Mutex m_mutex;
	mutable std::condition_variable m_cv;
	N m_value;
};

/** @brief Simple block guard.
 * The expression/block following is guarded though the given mutex.
 * Usage:
 * @code
 * Mutex m;
 * unsigned d;
 * ...
 * ETH_(m) d = 1;
 * ...
 * ETH_(m) { for (auto d = 10; d > 0; --d) foo(d); d = 0; }
 * @endcode
 *
 * There are several variants of this basic mechanism for different Mutex types and Guards.
 *
 * There is also the UNGUARD variant which allows an unguarded expression/block to exist within a
 * guarded expression. eg:
 *
 * @code
 * Mutex m;
 * int d;
 * ...
 * ETH_GUARDED(m)
 * {
 *   for (auto d = 50; d > 25; --d)
 *     foo(d);
 *   ETH_UNGUARDED(m)
 *     bar();
 *   for (; d > 0; --d)
 *     foo(d);
 * }
 * @endcode
 */

#define DEV_GUARDED(MUTEX) \
	for (GenericGuardBool<Guard, Mutex> __eth_l(MUTEX); __eth_l.b; __eth_l.b = false)
#define DEV_READ_GUARDED(MUTEX) \
	for (GenericGuardBool<ReadGuard, SharedMutex> __eth_l(MUTEX); __eth_l.b; __eth_l.b = false)
#define DEV_WRITE_GUARDED(MUTEX) \
	for (GenericGuardBool<WriteGuard, SharedMutex> __eth_l(MUTEX); __eth_l.b; __eth_l.b = false)
#define DEV_RECURSIVE_GUARDED(MUTEX) \
	for (GenericGuardBool<RecursiveGuard, RecursiveMutex> __eth_l(MUTEX); __eth_l.b; __eth_l.b = false)
#define DEV_UNGUARDED(MUTEX) \
	for (GenericUnguardBool<Mutex> __eth_l(MUTEX); __eth_l.b; __eth_l.b = false)
#define DEV_READ_UNGUARDED(MUTEX) \
	for (GenericUnguardSharedBool<SharedMutex> __eth_l(MUTEX); __eth_l.b; __eth_l.b = false)
#define DEV_WRITE_UNGUARDED(MUTEX) \
	for (GenericUnguardBool<SharedMutex> __eth_l(MUTEX); __eth_l.b; __eth_l.b = false)

}

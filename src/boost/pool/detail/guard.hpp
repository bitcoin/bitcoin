// Copyright (C) 2000 Stephen Cleary
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org for updates, documentation, and revision history.

#ifndef BOOST_POOL_GUARD_HPP
#define BOOST_POOL_GUARD_HPP

/*!
  \file
  \brief Extremely Light-Weight guard class.
  \details Auto-lock/unlock-er
  detail/guard.hpp provides a type guard<Mutex>
  that allows scoped access to the Mutex's locking and unlocking operations.
  It is used to ensure that a Mutex is unlocked, even if an exception is thrown.
*/

namespace boost {

namespace details {
namespace pool {

template <typename Mutex> //!< \tparam Mutex (platform-specific) mutex class.
class guard
{ //! Locks the mutex, binding guard<Mutex> to Mutex.
	/*! Example:
	Given a (platform-specific) mutex class, we can wrap code as follows:

	extern mutex global_lock;

	static void f()
	{
		boost::details::pool::guard<mutex> g(global_lock);
		// g's constructor locks "global_lock"

		... // do anything:
				//   throw exceptions
				//   return
				//   or just fall through
	} // g's destructor unlocks "global_lock"
	*/
  private:
    Mutex & mtx;

    guard(const guard &); //!< Guards the mutex, ensuring unlocked on destruction, even if exception is thrown.
    void operator=(const guard &);

  public:
    explicit guard(Mutex & nmtx)
    :mtx(nmtx)
    { //! Locks the mutex of the guard class.
			mtx.lock();
		}

    ~guard()
    { //! destructor unlocks the mutex of the guard class.
			mtx.unlock();
		}
}; // class guard

} // namespace pool
} // namespace details

} // namespace boost

#endif

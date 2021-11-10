// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/**
 * \file boost/process/group.hpp
 *
 * Defines a group process class.
 * For additional information see the platform specific implementations:
 *
 *   - [windows - job object](https://msdn.microsoft.com/en-us/library/windows/desktop/ms684161.aspx)
 *   - [posix - process group](http://pubs.opengroup.org/onlinepubs/009695399/functions/setpgid.html)
 *
 */

#ifndef BOOST_PROCESS_GROUP_HPP
#define BOOST_PROCESS_GROUP_HPP

#include <boost/process/detail/config.hpp>
#include <boost/process/child.hpp>
#include <chrono>
#include <memory>

#include <boost/none.hpp>
#include <atomic>


#if defined(BOOST_POSIX_API)
#include <boost/process/detail/posix/group_handle.hpp>
#include <boost/process/detail/posix/group_ref.hpp>
#include <boost/process/detail/posix/wait_group.hpp>
#elif defined(BOOST_WINDOWS_API)
#include <boost/process/detail/windows/group_handle.hpp>
#include <boost/process/detail/windows/group_ref.hpp>
#include <boost/process/detail/windows/wait_group.hpp>
#endif

namespace boost {

namespace process {

namespace detail {
    struct group_builder;
}

/**
 * Represents a process group.
 *
 * Groups are movable but non-copyable. The destructor
 * automatically closes handles to the group process.
 *
 * The group will have the same interface as std::thread.
 *
 * \note If the destructor is called without a previous detach or wait, the group will be terminated.
 *
 * \attention If a default-constructed group is used before being used in a process launch, the behaviour is undefined.
 *
 * \attention Waiting for groups is currently broken on windows and will most likely result in a dead-lock.
 */
class group
{
    ::boost::process::detail::api::group_handle _group_handle;
    bool _attached = true;
public:
    typedef ::boost::process::detail::api::group_handle group_handle;
    ///Native representation of the handle.
    typedef group_handle::handle_t native_handle_t;
    explicit group(group_handle &&ch) : _group_handle(std::move(ch)) {}
    ///Construct the group from a native_handle
    explicit group(native_handle_t & handle) : _group_handle(handle) {};
    group(const group&) = delete;
    ///Move constructor
    group(group && lhs)
        : _group_handle(std::move(lhs._group_handle)),
          _attached (lhs._attached)
    {
        lhs._attached = false;
    }
    ///Default constructor
    group() = default;
    group& operator=(const group&) = delete;
    ///Move assign
    group& operator=(group && lhs)
    {
        _group_handle= std::move(lhs._group_handle);
        _attached    = lhs._attached;

        return *this;
    };

    ///Detach the group
    void detach() {_attached = false; }

    /** Join the child. This just calls wait, but that way the naming is similar to std::thread */
    void join() {wait();}
    /** Check if the child is joinable. */
    bool joinable() {return _attached;}

    /** Destructor
     *
     * \note If the destructor is called without a previous detach or wait, the group will be terminated.
     *
     */
    ~group()
    {
        std::error_code ec;
        if ( _attached && valid())
            terminate(ec);
    }

    ///Obtain the native handle of the group.
    native_handle_t native_handle() const { return _group_handle.handle(); }

    ///Wait for the process group to exit.
    void wait()
    {
        boost::process::detail::api::wait(_group_handle);
    }
    ///\overload void wait()
    void wait(std::error_code & ec) noexcept
    {
        boost::process::detail::api::wait(_group_handle, ec);
    }
    /** Wait for the process group to exit for period of time.
      *  \return True if all child processes exited while waiting.*/
    template< class Rep, class Period >
    bool wait_for  (const std::chrono::duration<Rep, Period>& rel_time)
    {
        return boost::process::detail::api::wait_for(_group_handle, rel_time);
    }

    /** \overload bool wait_for(const std::chrono::duration<Rep, Period>& timeout_time ) */
    template< class Rep, class Period >
    bool wait_for  (const std::chrono::duration<Rep, Period>& rel_time, std::error_code & ec) noexcept
    {
        return boost::process::detail::api::wait_for(_group_handle, rel_time, ec);
    }

    /** Wait for the process group to exit until a point in time.
      *  \return True if all child processes exited while waiting.*/
    template< class Clock, class Duration >
    bool wait_until(const std::chrono::time_point<Clock, Duration>& timeout_time )
    {
        return boost::process::detail::api::wait_until(_group_handle, timeout_time);
    }
    /** \overload bool wait_until(const std::chrono::time_point<Clock, Duration>& timeout_time ) */
    template< class Clock, class Duration >
    bool wait_until(const std::chrono::time_point<Clock, Duration>& timeout_time, std::error_code & ec) noexcept
    {
        return boost::process::detail::api::wait_until(_group_handle, timeout_time, ec);
    }

    ///Check if the group has a valid handle.
    bool valid() const
    {
        return _group_handle.valid();
    }
    ///Convenience to call valid.
    explicit operator bool() const {return valid();}

    ///Terminate the process group, i.e. all processes in the group
    void terminate()
    {
        ::boost::process::detail::api::terminate(_group_handle);
    }
    ///\overload void terminate()
    void terminate(std::error_code & ec) noexcept
    {
        ::boost::process::detail::api::terminate(_group_handle, ec);
    }

    ///Assign a child process to the group
    void add(const child &c)
    {
        _group_handle.add(c.native_handle());
    }
    ///\overload void assign(const child & c)
    void add(const child &c, std::error_code & ec) noexcept
    {
        _group_handle.add(c.native_handle(), ec);
    }

    ///Check if the child process is in the group
    bool has(const child &c)
    {
        return _group_handle.has(c.native_handle());
    }
    ///\overload bool has(const child &)
    bool has(const child &c, std::error_code & ec) noexcept
    {
        return _group_handle.has(c.native_handle(), ec);

    }

    friend struct detail::group_builder;
};

namespace detail
{

struct group_tag;
struct group_builder
{
    group * group_p;

    void operator()(group & grp) {this->group_p = &grp;};

    typedef api::group_ref result_type;
    api::group_ref get_initializer() {return api::group_ref (group_p->_group_handle);};
};

template<>
struct initializer_tag<group>
{
    typedef group_tag type;
};

template<>
struct initializer_builder<group_tag>
{
    typedef group_builder type;
};

}
}}
#endif


// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_WINDOWS_WAIT_GROUP_HPP_
#define BOOST_PROCESS_DETAIL_WINDOWS_WAIT_GROUP_HPP_

#include <boost/process/detail/config.hpp>
#include <boost/process/detail/windows/group_handle.hpp>
#include <boost/winapi/jobs.hpp>
#include <boost/winapi/wait.hpp>
#include <chrono>

namespace boost { namespace process { namespace detail { namespace windows {

struct group_handle;


inline bool wait_impl(const group_handle & p, std::error_code & ec, std::chrono::system_clock::rep wait_time)
{
    ::boost::winapi::DWORD_ completion_code;
    ::boost::winapi::ULONG_PTR_ completion_key;
    ::boost::winapi::LPOVERLAPPED_ overlapped;

    auto start_time = std::chrono::system_clock::now();

    while (workaround::get_queued_completion_status(
                                       p._io_port, &completion_code,
                                       &completion_key, &overlapped, static_cast<::boost::winapi::DWORD_>(wait_time)))
    {
        if (reinterpret_cast<::boost::winapi::HANDLE_>(completion_key) == p._job_object &&
             completion_code == workaround::JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO_)
        {

            //double check, could be a different handle from a child
            workaround::JOBOBJECT_BASIC_ACCOUNTING_INFORMATION_ info;
            if (!workaround::query_information_job_object(
                    p._job_object,
                    workaround::JobObjectBasicAccountingInformation_,
                    static_cast<void *>(&info),
                    sizeof(info), nullptr))
            {
                ec = get_last_error();
                return false;
            }
            else if (info.ActiveProcesses == 0)
                return false; //correct, nothing left.
        }
        //reduce the remaining wait time -> in case interrupted by something else
        if (wait_time != static_cast<int>(::boost::winapi::infinite))
        {
            auto now = std::chrono::system_clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
            wait_time -= static_cast<std::chrono::system_clock::rep>(diff.count());
            start_time = now;
            if (wait_time <= 0)
                return true; //timeout with other source
        }

    }

    auto ec_ = get_last_error();
    if (ec_.value() == ::boost::winapi::wait_timeout)
        return true; //timeout

    ec = ec_;
    return false;
}

inline void wait(const group_handle &p, std::error_code &ec)
{
    wait_impl(p, ec, ::boost::winapi::infinite);
}

inline void wait(const group_handle &p)
{
    std::error_code ec;
    wait(p, ec);
    boost::process::detail::throw_error(ec, "wait error");
}

template< class Clock, class Duration >
inline bool wait_until(
        const group_handle &p,
        const std::chrono::time_point<Clock, Duration>& timeout_time,
        std::error_code &ec)
{
    std::chrono::milliseconds ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    timeout_time - Clock::now());

    auto timeout = wait_impl(p, ec, ms.count());
    return !ec && !timeout;
}

template< class Clock, class Duration >
inline bool wait_until(
        const group_handle &p,
        const std::chrono::time_point<Clock, Duration>& timeout_time)
{
    std::error_code ec;
    bool b = wait_until(p, timeout_time, ec);
    boost::process::detail::throw_error(ec, "wait_until error");
    return b;
}

template< class Rep, class Period >
inline bool wait_for(
        const group_handle &p,
        const std::chrono::duration<Rep, Period>& rel_time,
        std::error_code &ec)
{
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(rel_time);
    auto timeout = wait_impl(p, ec, ms.count());
    return !ec && !timeout;
}

template< class Rep, class Period >
inline bool wait_for(
        const group_handle &p,
        const std::chrono::duration<Rep, Period>& rel_time)
{
    std::error_code ec;
    bool b = wait_for(p, rel_time, ec);
    boost::process::detail::throw_error(ec, "wait_for error");
    return b;
}

}}}}

#endif /* BOOST_PROCESS_DETAIL_WINDOWS_WAIT_GROUP_HPP_ */

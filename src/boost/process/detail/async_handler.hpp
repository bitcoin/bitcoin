/*
 * async_handler.hpp
 *
 *  Created on: 12.06.2016
 *      Author: Klemens
 */

#ifndef BOOST_PROCESS_DETAIL_ASYNC_HANDLER_HPP_
#define BOOST_PROCESS_DETAIL_ASYNC_HANDLER_HPP_

#include <type_traits>

#if defined(BOOST_POSIX_API)
#include <boost/process/posix.hpp>
#include <boost/process/detail/posix/async_handler.hpp>
#include <boost/process/detail/posix/asio_fwd.hpp>
#else
#include <boost/process/detail/windows/async_handler.hpp>
#include <boost/process/detail/windows/asio_fwd.hpp>
#endif

namespace boost {

namespace process {

namespace detail {

#if defined(BOOST_POSIX_API)
using ::boost::process::detail::posix::is_async_handler;
using ::boost::process::detail::posix::does_require_io_context;
#else
using ::boost::process::detail::windows::is_async_handler;
using ::boost::process::detail::windows::does_require_io_context;
#endif

template<typename ...Args>
struct has_io_context;

template<typename T, typename ...Args>
struct has_io_context<T, Args...>
{
    typedef typename has_io_context<Args...>::type next;
    typedef typename std::is_same<
                typename std::remove_reference<T>::type,
                boost::asio::io_context>::type is_ios;
    typedef typename std::conditional<is_ios::value,
            std::true_type,
            next>::type type;
};

template<typename T>
struct has_io_context<T>
{
    typedef typename std::is_same<
            typename std::remove_reference<T>::type,
            boost::asio::io_context>::type type;
};

template<typename ...Args>
using has_io_context_t = typename has_io_context<Args...>::type;

template<typename ...Args>
struct has_async_handler;

template<typename T, typename ...Args>
struct has_async_handler<T, Args...>
{
    typedef typename has_async_handler<Args...>::type next;
    typedef typename is_async_handler<T>::type is_ios;
    typedef typename std::conditional<is_ios::value,
            std::true_type,
            next>::type type;
};

template<typename T>
struct has_async_handler<T>
{
    typedef typename is_async_handler<T>::type type;
};

template<typename ...Args>
struct needs_io_context;

template<typename T, typename ...Args>
struct needs_io_context<T, Args...>
{
    typedef typename needs_io_context<Args...>::type next;
    typedef typename does_require_io_context<T>::type is_ios;
    typedef typename std::conditional<is_ios::value,
            std::true_type,
            next>::type type;
};

template<typename T>
struct needs_io_context<T>
{
    typedef typename does_require_io_context<T>::type type;
};

template<typename ...Args>
boost::asio::io_context &get_io_context_var(boost::asio::io_context & f, Args&...)
{
    return f;
}

template<typename First, typename ...Args>
boost::asio::io_context &get_io_context_var(First&, Args&...args)
{
    return get_io_context_var(args...);
}

}
}
}


#endif /* BOOST_PROCESS_DETAIL_ASYNC_HANDLER_HPP_ */

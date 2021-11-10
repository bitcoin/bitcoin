// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_ON_EXIT_HPP_
#define BOOST_PROCESS_DETAIL_ON_EXIT_HPP_

#include <boost/process/detail/config.hpp>

#if defined(BOOST_POSIX_API)
#include <boost/process/detail/posix/on_exit.hpp>
#elif defined(BOOST_WINDOWS_API)
#include <boost/process/detail/windows/on_exit.hpp>
#endif

#include <future>
#include <memory>

namespace boost { namespace process { namespace detail {

inline std::function<void(int, const std::error_code &)> on_exit_from_future(std::future<int> &f)
{
    std::shared_ptr<std::promise<int>> promise = std::make_shared<std::promise<int>>();
    f = promise->get_future();
    return [promise](int code, const std::error_code & ec)
            {
                if (ec)
                    promise->set_exception(
                        std::make_exception_ptr(process_error(ec, "on_exit failed with error"))
                        );
                else
                    promise->set_value(code);
            };
}


struct on_exit_
{
    api::on_exit_ operator= (const std::function<void(int, const std::error_code&)> & f) const {return f;}
    api::on_exit_ operator()(const std::function<void(int, const std::error_code&)> & f) const {return f;}

    api::on_exit_ operator= (std::future<int> &f) const {return on_exit_from_future(f);}
    api::on_exit_ operator()(std::future<int> &f) const {return on_exit_from_future(f);}
};

}

constexpr static ::boost::process::detail::on_exit_ on_exit{};


}}
#endif /* INCLUDE_BOOST_PROCESS_WINDOWS_ON_EXIT_HPP_ */

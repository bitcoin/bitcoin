// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_POSIX_IO_CONTEXT_REF_HPP_
#define BOOST_PROCESS_POSIX_IO_CONTEXT_REF_HPP_

#include <boost/process/detail/posix/handler.hpp>
#include <boost/process/detail/posix/async_handler.hpp>
#include <boost/asio/io_context.hpp>

#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/algorithm/transformation/filter_if.hpp>
#include <boost/fusion/algorithm/transformation/transform.hpp>
#include <boost/fusion/view/transform_view.hpp>
#include <boost/fusion/container/vector/convert.hpp>


#include <boost/process/detail/posix/sigchld_service.hpp>
#include <boost/process/detail/posix/is_running.hpp>

#include <functional>
#include <type_traits>
#include <memory>
#include <vector>
#include <sys/wait.h>

namespace boost { namespace process { namespace detail { namespace posix {

template<typename Executor>
struct on_exit_handler_transformer
{
    Executor & exec;
    on_exit_handler_transformer(Executor & exec) : exec(exec) {}
    template<typename Sig>
    struct result;

    template<typename T>
    struct result<on_exit_handler_transformer<Executor>(T&)>
    {
        typedef typename T::on_exit_handler_t type;
    };

    template<typename T>
    auto operator()(T& t) const -> typename T::on_exit_handler_t
    {
        return t.on_exit_handler(exec);
    }
};

template<typename Executor>
struct async_handler_collector
{
    Executor & exec;
    std::vector<std::function<void(int, const std::error_code & ec)>> &handlers;


    async_handler_collector(Executor & exec,
            std::vector<std::function<void(int, const std::error_code & ec)>> &handlers)
                : exec(exec), handlers(handlers) {}

    template<typename T>
    void operator()(T & t) const
    {
        handlers.push_back(t.on_exit_handler(exec));
    }
};

//Also set's up waiting for the exit, so it can close async stuff.
struct io_context_ref : handler_base_ext
{
    io_context_ref(boost::asio::io_context & ios) : ios(ios)
    {

    }
    boost::asio::io_context &get() {return ios;};
    
    template <class Executor>
    void on_success(Executor& exec)
    {
        ios.notify_fork(boost::asio::io_context::fork_parent);
        //must be on the heap so I can move it into the lambda.
        auto asyncs = boost::fusion::filter_if<
                        is_async_handler<
                        typename std::remove_reference< boost::mpl::_ > ::type
                        >>(exec.seq);

        //ok, check if there are actually any.
        if (boost::fusion::empty(asyncs))
            return;

        std::vector<std::function<void(int, const std::error_code & ec)>> funcs;
        funcs.reserve(boost::fusion::size(asyncs));
        boost::fusion::for_each(asyncs, async_handler_collector<Executor>(exec, funcs));

        auto & es = exec.exit_status;

        auto wh = [funcs, es](int val, const std::error_code & ec)
                {
                    es->store(val);
                    for (auto & func : funcs)
                        func(::boost::process::detail::posix::eval_exit_status(val), ec);
                };

        sigchld_service.async_wait(exec.pid, std::move(wh));
    }

    template<typename Executor>
    void on_setup (Executor &) const {/*ios.notify_fork(boost::asio::io_context::fork_prepare);*/}

    template<typename Executor>
    void on_exec_setup  (Executor &) const {/*ios.notify_fork(boost::asio::io_context::fork_child);*/}

    template <class Executor>
    void on_error(Executor&, const std::error_code &) const {/*ios.notify_fork(boost::asio::io_context::fork_parent);*/}

private:
    boost::asio::io_context &ios;
    boost::process::detail::posix::sigchld_service &sigchld_service = boost::asio::use_service<boost::process::detail::posix::sigchld_service>(ios);
};

}}}}

#endif /* BOOST_PROCESS_WINDOWS_IO_CONTEXT_REF_HPP_ */

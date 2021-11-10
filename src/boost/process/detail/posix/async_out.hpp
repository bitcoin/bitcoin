// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_POSIX_ASYNC_OUT_HPP
#define BOOST_PROCESS_DETAIL_POSIX_ASYNC_OUT_HPP


#include <boost/process/detail/posix/handler.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/read.hpp>
#include <boost/process/async_pipe.hpp>
#include <istream>
#include <memory>
#include <exception>
#include <future>
#include <array>
#include <boost/process/detail/used_handles.hpp>

namespace boost { namespace process { namespace detail { namespace posix {


inline int apply_out_handles(int handle, std::integral_constant<int, 1>, std::integral_constant<int, -1>)
{
    return ::dup2(handle, STDOUT_FILENO);
}

inline int apply_out_handles(int handle, std::integral_constant<int, 2>, std::integral_constant<int, -1>)
{
    return ::dup2(handle, STDERR_FILENO);
}

inline int apply_out_handles(int handle, std::integral_constant<int, 1>, std::integral_constant<int, 2>)
{
    if (::dup2(handle, STDOUT_FILENO) == -1)
        return -1;
    if (::dup2(handle, STDERR_FILENO) == -1)
        return -1;

    return 0;
}

template<int p1, int p2, typename Buffer>
struct async_out_buffer : ::boost::process::detail::posix::handler_base_ext,
                          ::boost::process::detail::posix::require_io_context,
                          ::boost::process::detail::uses_handles
{
    Buffer & buf;

    std::shared_ptr<boost::process::async_pipe> pipe;

    std::array<int, 4> get_used_handles()
    {
        const auto pp1 = p1 != -1 ? p1 : p2;
        const auto pp2 = p2 != -1 ? p2 : p1;

        if (pipe)
            return {pipe->native_source(), pipe->native_sink(), pp1, pp2};
        else  //if pipe is not constructed, limit_ds is invoked before -> this also means on_exec_setup gets invoked before.
            return {pp1, pp2, pp1, pp2};
    }


    async_out_buffer(Buffer & buf) : buf(buf)
    {
    }

    template <typename Executor>
    inline void on_success(Executor &exec)
    {
        auto  pipe              = this->pipe;
        boost::asio::async_read(*pipe, buf,
                [pipe](const boost::system::error_code&, std::size_t){});

        this->pipe = nullptr;
        std::move(*pipe).sink().close();
    }

    template<typename Executor>
    void on_error(Executor &, const std::error_code &) const
    {
        std::move(*pipe).sink().close();
    }

    template<typename Executor>
    void on_setup(Executor & exec)
    {
        pipe = std::make_shared<boost::process::async_pipe>(get_io_context(exec.seq));
    }


    template <typename Executor>
    void on_exec_setup(Executor &exec)
    {
        int res = apply_out_handles(pipe->native_sink(),
                      std::integral_constant<int, p1>(), std::integral_constant<int, p2>());
        if (res == -1)
            exec.set_error(::boost::process::detail::get_last_error(), "dup2() failed");

        ::close(pipe->native_sink());
        ::close(pipe->native_source());

    }
};




template<int p1, int p2, typename Type>
struct async_out_future : ::boost::process::detail::posix::handler_base_ext,
                          ::boost::process::detail::posix::require_io_context
{
    std::shared_ptr<std::promise<Type>> promise = std::make_shared<std::promise<Type>>();

    std::shared_ptr<boost::asio::streambuf> buffer = std::make_shared<boost::asio::streambuf>();

    std::shared_ptr<boost::process::async_pipe> pipe;

    async_out_future(std::future<Type> & fut)
    {
        fut = promise->get_future();
    }
    template <typename Executor>
    inline void on_success(Executor &)
    {
        auto pipe_ = this->pipe;

        auto buffer_  = this->buffer;
        auto promise_ = this->promise;

        boost::asio::async_read(*pipe_, *buffer_,
                [pipe_, buffer_, promise_](const boost::system::error_code& ec, std::size_t)
                {
                    if (ec && (ec.value() != ENOENT))
                    {
                        std::error_code e(ec.value(), std::system_category());
                        promise_->set_exception(std::make_exception_ptr(process_error(e)));
                    }
                    else
                    {
                        std::istream is (buffer_.get());
                        Type arg;
                        if (buffer_->size() > 0)
                        {
                            arg.resize(buffer_->size());
                            is.read(&*arg.begin(), buffer_->size());
                        }
                        promise_->set_value(std::move(arg));
                    }
                });

        std::move(*pipe_).sink().close();
        this->pipe = nullptr;
    }

    template<typename Executor>
    void on_error(Executor &, const std::error_code &) const
    {
        std::move(*pipe).sink().close();
    }

    template<typename Executor>
    void on_setup(Executor & exec)
    {
        pipe = std::make_shared<boost::process::async_pipe>(get_io_context(exec.seq));
    }

    template <typename Executor>
    void on_exec_setup(Executor &exec)
    {

        int res = apply_out_handles(pipe->native_sink(),
                      std::integral_constant<int, p1>(), std::integral_constant<int, p2>());
        if (res == -1)
            exec.set_error(::boost::process::detail::get_last_error(), "dup2() failed");

        ::close(pipe->native_sink());
        ::close(pipe->native_source());
    }

};

}}}}

#endif

// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_WINDOWS_INITIALIZERS_ASYNC_OUT_HPP
#define BOOST_PROCESS_WINDOWS_INITIALIZERS_ASYNC_OUT_HPP

#include <boost/winapi/process.hpp>
#include <boost/winapi/handles.hpp>
#include <boost/winapi/handle_info.hpp>
#include <boost/winapi/error_codes.hpp>
#include <boost/asio/read.hpp>
#include <boost/process/detail/handler_base.hpp>
#include <boost/process/detail/used_handles.hpp>
#include <boost/process/detail/windows/asio_fwd.hpp>

#include <istream>
#include <memory>
#include <exception>
#include <future>


namespace boost { namespace process { namespace detail { namespace windows {


template <typename Executor>
inline void apply_out_handles(Executor &e, void* handle, std::integral_constant<int, 1>, std::integral_constant<int, -1>)
{
    boost::winapi::SetHandleInformation(handle,
            boost::winapi::HANDLE_FLAG_INHERIT_,
            boost::winapi::HANDLE_FLAG_INHERIT_);

    e.startup_info.hStdOutput = handle;
    e.startup_info.dwFlags   |= ::boost::winapi::STARTF_USESTDHANDLES_;
    e.inherit_handles = true;
}

template <typename Executor>
inline void apply_out_handles(Executor &e, void* handle, std::integral_constant<int, 2>, std::integral_constant<int, -1>)
{
    boost::winapi::SetHandleInformation(handle,
            boost::winapi::HANDLE_FLAG_INHERIT_,
            boost::winapi::HANDLE_FLAG_INHERIT_);


    e.startup_info.hStdError = handle;
    e.startup_info.dwFlags  |= ::boost::winapi::STARTF_USESTDHANDLES_;
    e.inherit_handles = true;
}

template <typename Executor>
inline void apply_out_handles(Executor &e, void* handle, std::integral_constant<int, 1>, std::integral_constant<int, 2>)
{
    boost::winapi::SetHandleInformation(handle,
            boost::winapi::HANDLE_FLAG_INHERIT_,
            boost::winapi::HANDLE_FLAG_INHERIT_);

    e.startup_info.hStdOutput = handle;
    e.startup_info.hStdError  = handle;
    e.startup_info.dwFlags   |= ::boost::winapi::STARTF_USESTDHANDLES_;
    e.inherit_handles = true;
}

template<int p1, int p2, typename Buffer>
struct async_out_buffer : ::boost::process::detail::windows::handler_base_ext,
                          ::boost::process::detail::windows::require_io_context
{
    Buffer & buf;

    std::shared_ptr<boost::process::async_pipe> pipe;


    async_out_buffer(Buffer & buf) : buf(buf)
    {
    }
    template <typename Executor>
    inline void on_success(Executor&)
    {
        auto pipe_ = this->pipe;
        boost::asio::async_read(*pipe_, buf,
                [pipe_](const boost::system::error_code&, std::size_t){});
        std::move(*pipe_).sink().close();
        this->pipe       = nullptr;

    }

    template<typename Executor>
    void on_error(Executor &, const std::error_code &) const
    {
        std::move(*pipe).sink().close();
    }

    template <typename WindowsExecutor>
    void on_setup(WindowsExecutor &exec)
    {
        if (!pipe)
            pipe = std::make_shared<boost::process::async_pipe>(get_io_context(exec.seq));
        apply_out_handles(exec, std::move(*pipe).sink().native_handle(),
                std::integral_constant<int, p1>(), std::integral_constant<int, p2>());
    }
};



template<int p1, int p2, typename Type>
struct async_out_future : ::boost::process::detail::windows::handler_base_ext,
                          ::boost::process::detail::windows::require_io_context,
                          ::boost::process::detail::uses_handles
{
    std::shared_ptr<boost::process::async_pipe> pipe;
    std::shared_ptr<std::promise<Type>> promise = std::make_shared<std::promise<Type>>();
    std::shared_ptr<boost::asio::streambuf> buffer = std::make_shared<boost::asio::streambuf>();

    ::boost::winapi::HANDLE_ get_used_handles() const
    {
        return std::move(*pipe).sink().native_handle();
    }


    async_out_future(std::future<Type> & fut)
    {
        fut = promise->get_future();
    }
    template <typename Executor>
    inline void on_success(Executor&)
    {
        auto pipe_    = this->pipe;
        auto buffer_  = this->buffer;
        auto promise_ = this->promise;
        std::move(*pipe_).sink().close();
        boost::asio::async_read(*pipe_, *buffer_,
                [pipe_, buffer_, promise_](const boost::system::error_code& ec, std::size_t)
                {
                    if (ec && (ec.value() != ::boost::winapi::ERROR_BROKEN_PIPE_))
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
        this->pipe    = nullptr;
        this->buffer  = nullptr;
        this->promise = nullptr;


    }

    template<typename Executor>
    void on_error(Executor &, const std::error_code &) const
    {
        std::move(*pipe).sink().close();
    }

    template <typename WindowsExecutor>
    void on_setup(WindowsExecutor &exec)
    {
        if (!pipe)
            pipe = std::make_shared<boost::process::async_pipe>(get_io_context(exec.seq));

        apply_out_handles(exec, std::move(*pipe).sink().native_handle(),
                std::integral_constant<int, p1>(), std::integral_constant<int, p2>());
    }
};


}}}}

#endif

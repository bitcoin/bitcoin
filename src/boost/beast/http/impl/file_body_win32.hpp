//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_HTTP_IMPL_FILE_BODY_WIN32_HPP
#define BOOST_BEAST_HTTP_IMPL_FILE_BODY_WIN32_HPP

#if BOOST_BEAST_USE_WIN32_FILE

#include <boost/beast/core/async_base.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/buffers_range.hpp>
#include <boost/beast/core/detail/clamp.hpp>
#include <boost/beast/core/detail/is_invocable.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/windows/overlapped_ptr.hpp>
#include <boost/make_unique.hpp>
#include <boost/smart_ptr/make_shared_array.hpp>
#include <boost/winapi/basic_types.hpp>
#include <boost/winapi/error_codes.hpp>
#include <boost/winapi/get_last_error.hpp>
#include <algorithm>
#include <cstring>

namespace boost {
namespace beast {
namespace http {

namespace detail {
template<class, class, bool, class, class>
class write_some_win32_op;
} // detail

template<>
struct basic_file_body<file_win32>
{
    using file_type = file_win32;

    class writer;
    class reader;

    //--------------------------------------------------------------------------

    class value_type
    {
        friend class writer;
        friend class reader;
        friend struct basic_file_body<file_win32>;

        template<class, class, bool, class, class>
        friend class detail::write_some_win32_op;
        template<
            class Protocol, class Executor,
            bool isRequest, class Fields>
        friend
        std::size_t
        write_some(
            net::basic_stream_socket<Protocol, Executor>& sock,
            serializer<isRequest,
                basic_file_body<file_win32>, Fields>& sr,
            error_code& ec);

        file_win32 file_;
        std::uint64_t size_ = 0;    // cached file size
        std::uint64_t first_;       // starting offset of the range
        std::uint64_t last_;        // ending offset of the range

    public:
        ~value_type() = default;
        value_type() = default;
        value_type(value_type&& other) = default;
        value_type& operator=(value_type&& other) = default;

        file_win32& file()
        {
            return file_;
        }

        bool
        is_open() const
        {
            return file_.is_open();
        }

        std::uint64_t
        size() const
        {
            return size_;
        }

        void
        close();

        void
        open(char const* path, file_mode mode, error_code& ec);

        void
        reset(file_win32&& file, error_code& ec);
    };

    //--------------------------------------------------------------------------

    class writer
    {
        template<class, class, bool, class, class>
        friend class detail::write_some_win32_op;
        template<
            class Protocol, class Executor,
            bool isRequest, class Fields>
        friend
        std::size_t
        write_some(
            net::basic_stream_socket<Protocol, Executor>& sock,
            serializer<isRequest,
                basic_file_body<file_win32>, Fields>& sr,
            error_code& ec);

        value_type& body_;  // The body we are reading from
        std::uint64_t pos_; // The current position in the file
        char buf_[4096];    // Small buffer for reading

    public:
        using const_buffers_type =
            net::const_buffer;

        template<bool isRequest, class Fields>
        writer(header<isRequest, Fields>&, value_type& b)
            : body_(b)
            , pos_(body_.first_)
        {
            BOOST_ASSERT(body_.file_.is_open());
        }

        void
        init(error_code& ec)
        {
            BOOST_ASSERT(body_.file_.is_open());
            ec.clear();
        }

        boost::optional<std::pair<const_buffers_type, bool>>
        get(error_code& ec)
        {
            std::size_t const n = (std::min)(sizeof(buf_),
                beast::detail::clamp(body_.last_ - pos_));
            if(n == 0)
            {
                ec = {};
                return boost::none;
            }
            auto const nread = body_.file_.read(buf_, n, ec);
            if(ec)
                return boost::none;
            if (nread == 0)
            {
                ec = error::short_read;
                return boost::none;
            }
            BOOST_ASSERT(nread != 0);
            pos_ += nread;
            ec = {};
            return {{
                {buf_, nread},          // buffer to return.
                pos_ < body_.last_}};   // `true` if there are more buffers.
        }
    };

    //--------------------------------------------------------------------------

    class reader
    {
        value_type& body_;

    public:
        template<bool isRequest, class Fields>
        explicit
        reader(header<isRequest, Fields>&, value_type& b)
            : body_(b)
        {
        }

        void
        init(boost::optional<
            std::uint64_t> const& content_length,
                error_code& ec)
        {
            // VFALCO We could reserve space in the file
            boost::ignore_unused(content_length);
            BOOST_ASSERT(body_.file_.is_open());
            ec = {};
        }

        template<class ConstBufferSequence>
        std::size_t
        put(ConstBufferSequence const& buffers,
            error_code& ec)
        {
            std::size_t nwritten = 0;
            for(auto buffer : beast::buffers_range_ref(buffers))
            {
                nwritten += body_.file_.write(
                    buffer.data(), buffer.size(), ec);
                if(ec)
                    return nwritten;
            }
            ec = {};
            return nwritten;
        }

        void
        finish(error_code& ec)
        {
            ec = {};
        }
    };

    //--------------------------------------------------------------------------

    static
    std::uint64_t
    size(value_type const& body)
    {
        return body.size();
    }
};

//------------------------------------------------------------------------------

inline
void
basic_file_body<file_win32>::
value_type::
close()
{
    error_code ignored;
    file_.close(ignored);
}

inline
void
basic_file_body<file_win32>::
value_type::
open(char const* path, file_mode mode, error_code& ec)
{
    file_.open(path, mode, ec);
    if(ec)
        return;
    size_ = file_.size(ec);
    if(ec)
    {
        close();
        return;
    }
    first_ = 0;
    last_ = size_;
}

inline
void
basic_file_body<file_win32>::
value_type::
reset(file_win32&& file, error_code& ec)
{
    if(file_.is_open())
    {
        error_code ignored;
        file_.close(ignored);
    }
    file_ = std::move(file);
    if(file_.is_open())
    {
        size_ = file_.size(ec);
        if(ec)
        {
            close();
            return;
        }
        first_ = 0;
        last_ = size_;
    }
}

//------------------------------------------------------------------------------

namespace detail {

template<class Unsigned>
boost::winapi::DWORD_
lowPart(Unsigned n)
{
    return static_cast<
        boost::winapi::DWORD_>(
            n & 0xffffffff);
}

template<class Unsigned>
boost::winapi::DWORD_
highPart(Unsigned n, std::true_type)
{
    return static_cast<
        boost::winapi::DWORD_>(
            (n>>32)&0xffffffff);
}

template<class Unsigned>
boost::winapi::DWORD_
highPart(Unsigned, std::false_type)
{
    return 0;
}

template<class Unsigned>
boost::winapi::DWORD_
highPart(Unsigned n)
{
    return highPart(n, std::integral_constant<
        bool, (sizeof(Unsigned)>4)>{});
}

class null_lambda
{
public:
    template<class ConstBufferSequence>
    void
    operator()(error_code&,
        ConstBufferSequence const&) const
    {
        BOOST_ASSERT(false);
    }
};

// https://github.com/boostorg/beast/issues/1815
// developer commentary:
// This function mimics the behaviour of ASIO.
// Perhaps the correct fix is to insist on the use
// of an appropriate error_condition to detect
// connection_reset and connection_refused?
inline
error_code
make_win32_error(
    boost::winapi::DWORD_ dwError) noexcept
{
    // from
    // https://github.com/boostorg/asio/blob/6534af41b471288091ae39f9ab801594189b6fc9/include/boost/asio/detail/impl/socket_ops.ipp#L842
    switch(dwError)
    {
    case boost::winapi::ERROR_NETNAME_DELETED_:
        return net::error::connection_reset;
    case boost::winapi::ERROR_PORT_UNREACHABLE_:
        return net::error::connection_refused;
    case boost::winapi::WSAEMSGSIZE_:
    case boost::winapi::ERROR_MORE_DATA_:
        return {};
    }
    return error_code(
        static_cast<int>(dwError),
        system_category());
}

inline
error_code
make_win32_error(
    error_code ec) noexcept
{
    if(ec.category() !=
        system_category())
        return ec;
    return make_win32_error(
        static_cast<boost::winapi::DWORD_>(
            ec.value()));
}

//------------------------------------------------------------------------------

#if BOOST_ASIO_HAS_WINDOWS_OVERLAPPED_PTR

template<
    class Protocol, class Executor,
    bool isRequest, class Fields,
    class Handler>
class write_some_win32_op
    : public beast::async_base<Handler, Executor>
{
    net::basic_stream_socket<
        Protocol, Executor>& sock_;
    serializer<isRequest,
        basic_file_body<file_win32>, Fields>& sr_;
    bool header_ = false;

public:
    template<class Handler_>
    write_some_win32_op(
        Handler_&& h,
        net::basic_stream_socket<
            Protocol, Executor>& s,
        serializer<isRequest,
            basic_file_body<file_win32>,Fields>& sr)
        : async_base<
            Handler, Executor>(
                std::forward<Handler_>(h),
                s.get_executor())
        , sock_(s)
        , sr_(sr)
    {
        (*this)();
    }

    void
    operator()()
    {
        if(! sr_.is_header_done())
        {
            header_ = true;
            sr_.split(true);
            return detail::async_write_some_impl(
                sock_, sr_, std::move(*this));
        }
        if(sr_.get().chunked())
        {
            return detail::async_write_some_impl(
                sock_, sr_, std::move(*this));
        }
        auto& w = sr_.writer_impl();
        boost::winapi::DWORD_ const nNumberOfBytesToWrite =
            static_cast<boost::winapi::DWORD_>(
            (std::min<std::uint64_t>)(
                (std::min<std::uint64_t>)(w.body_.last_ - w.pos_, sr_.limit()),
                (std::numeric_limits<boost::winapi::DWORD_>::max)()));
        net::windows::overlapped_ptr overlapped{
            sock_.get_executor(), std::move(*this)};
        // Note that we have moved *this, so we cannot access
        // the handler since it is now moved-from. We can still
        // access simple things like references and built-in types.
        auto& ov = *overlapped.get();
        ov.Offset = lowPart(w.pos_);
        ov.OffsetHigh = highPart(w.pos_);
        auto const bSuccess = ::TransmitFile(
            sock_.native_handle(),
            sr_.get().body().file_.native_handle(),
            nNumberOfBytesToWrite,
            0,
            overlapped.get(),
            nullptr,
            0);
        auto const dwError = boost::winapi::GetLastError();
        if(! bSuccess && dwError !=
            boost::winapi::ERROR_IO_PENDING_)
        {
            // VFALCO This needs review, is 0 the right number?
            // completed immediately (with error?)
            overlapped.complete(
                make_win32_error(dwError), 0);
            return;
        }
        overlapped.release();
    }

    void
    operator()(
        error_code ec,
        std::size_t bytes_transferred = 0)
    {
        if(ec)
        {
            ec = make_win32_error(ec);
        }
        else if(! ec && ! header_)
        {
            auto& w = sr_.writer_impl();
            w.pos_ += bytes_transferred;
            BOOST_ASSERT(w.pos_ <= w.body_.last_);
            if(w.pos_ >= w.body_.last_)
            {
                sr_.next(ec, null_lambda{});
                BOOST_ASSERT(! ec);
                BOOST_ASSERT(sr_.is_done());
            }
        }
        this->complete_now(ec, bytes_transferred);
    }
};

struct run_write_some_win32_op
{
    template<
        class Protocol, class Executor,
        bool isRequest, class Fields,
        class WriteHandler>
    void
    operator()(
        WriteHandler&& h,
        net::basic_stream_socket<
            Protocol, Executor>* s,
        serializer<isRequest,
            basic_file_body<file_win32>, Fields>* sr)
    {
        // If you get an error on the following line it means
        // that your handler does not meet the documented type
        // requirements for the handler.

        static_assert(
            beast::detail::is_invocable<WriteHandler,
            void(error_code, std::size_t)>::value,
            "WriteHandler type requirements not met");

        write_some_win32_op<
            Protocol, Executor,
            isRequest, Fields,
            typename std::decay<WriteHandler>::type>(
                std::forward<WriteHandler>(h), *s, *sr);
    }
};

#endif

} // detail

//------------------------------------------------------------------------------

template<
    class Protocol, class Executor,
    bool isRequest, class Fields>
std::size_t
write_some(
    net::basic_stream_socket<
        Protocol, Executor>& sock,
    serializer<isRequest,
        basic_file_body<file_win32>, Fields>& sr,
    error_code& ec)
{
    if(! sr.is_header_done())
    {
        sr.split(true);
        auto const bytes_transferred =
            detail::write_some_impl(sock, sr, ec);
        if(ec)
            return bytes_transferred;
        return bytes_transferred;
    }
    if(sr.get().chunked())
    {
        auto const bytes_transferred =
            detail::write_some_impl(sock, sr, ec);
        if(ec)
            return bytes_transferred;
        return bytes_transferred;
    }
    auto& w = sr.writer_impl();
    w.body_.file_.seek(w.pos_, ec);
    if(ec)
        return 0;
    boost::winapi::DWORD_ const nNumberOfBytesToWrite =
        static_cast<boost::winapi::DWORD_>(
        (std::min<std::uint64_t>)(
            (std::min<std::uint64_t>)(w.body_.last_ - w.pos_, sr.limit()),
            (std::numeric_limits<boost::winapi::DWORD_>::max)()));
    auto const bSuccess = ::TransmitFile(
        sock.native_handle(),
        w.body_.file_.native_handle(),
        nNumberOfBytesToWrite,
        0,
        nullptr,
        nullptr,
        0);
    if(! bSuccess)
    {
        ec = detail::make_win32_error(
            boost::winapi::GetLastError());
        return 0;
    }
    w.pos_ += nNumberOfBytesToWrite;
    BOOST_ASSERT(w.pos_ <= w.body_.last_);
    if(w.pos_ < w.body_.last_)
    {
        ec = {};
    }
    else
    {
        sr.next(ec, detail::null_lambda{});
        BOOST_ASSERT(! ec);
        BOOST_ASSERT(sr.is_done());
    }
    return nNumberOfBytesToWrite;
}

#if BOOST_ASIO_HAS_WINDOWS_OVERLAPPED_PTR

template<
    class Protocol, class Executor,
    bool isRequest, class Fields,
    BOOST_BEAST_ASYNC_TPARAM2 WriteHandler>
BOOST_BEAST_ASYNC_RESULT2(WriteHandler)
async_write_some(
    net::basic_stream_socket<
        Protocol, Executor>& sock,
    serializer<isRequest,
        basic_file_body<file_win32>, Fields>& sr,
    WriteHandler&& handler)
{  
    return net::async_initiate<
        WriteHandler,
        void(error_code, std::size_t)>(
            detail::run_write_some_win32_op{},
            handler,
            &sock,
            &sr);
}

#endif

} // http
} // beast
} // boost

#endif

#endif

// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_WINDOWS_ASYNC_PIPE_HPP_
#define BOOST_PROCESS_DETAIL_WINDOWS_ASYNC_PIPE_HPP_

#include <boost/winapi/basic_types.hpp>
#include <boost/winapi/pipes.hpp>
#include <boost/winapi/handles.hpp>
#include <boost/winapi/file_management.hpp>
#include <boost/winapi/get_last_error.hpp>
#include <boost/winapi/access_rights.hpp>
#include <boost/winapi/process.hpp>
#include <boost/process/detail/windows/basic_pipe.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/windows/stream_handle.hpp>
#include <atomic>
#include <system_error>
#include <string>

namespace boost { namespace process { namespace detail { namespace windows {

inline std::string make_pipe_name()
{
    std::string name = "\\\\.\\pipe\\boost_process_auto_pipe_";

    auto pid = ::boost::winapi::GetCurrentProcessId();

    static std::atomic_size_t cnt{0};
    name += std::to_string(pid);
    name += "_";
    name += std::to_string(cnt++);

    return name;
}

class async_pipe
{
    ::boost::asio::windows::stream_handle _source;
    ::boost::asio::windows::stream_handle _sink  ;

    inline async_pipe(boost::asio::io_context & ios_source,
                      boost::asio::io_context & ios_sink,
                      const std::string & name, bool private_);

public:
    typedef ::boost::winapi::HANDLE_ native_handle_type;
    typedef ::boost::asio::windows::stream_handle   handle_type;
    typedef typename handle_type::executor_type executor_type;

    async_pipe(boost::asio::io_context & ios) : async_pipe(ios, ios, make_pipe_name(), true) {}
    async_pipe(boost::asio::io_context & ios_source, boost::asio::io_context & ios_sink)
                : async_pipe(ios_source, ios_sink, make_pipe_name(), true) {}

    async_pipe(boost::asio::io_context & ios, const std::string & name)
                : async_pipe(ios, ios, name, false) {}

    async_pipe(boost::asio::io_context & ios_source, boost::asio::io_context & ios_sink, const std::string & name)
            : async_pipe(ios_source, ios_sink, name, false) {}



    inline async_pipe(const async_pipe& rhs);
    async_pipe(async_pipe&& rhs)  : _source(std::move(rhs._source)), _sink(std::move(rhs._sink))
    {
    }
    template<class CharT, class Traits = std::char_traits<CharT>>
    explicit async_pipe(::boost::asio::io_context & ios_source,
                        ::boost::asio::io_context & ios_sink,
                         const basic_pipe<CharT, Traits> & p)
            : _source(ios_source, p.native_source()), _sink(ios_sink, p.native_sink())
    {
    }

    template<class CharT, class Traits = std::char_traits<CharT>>
    explicit async_pipe(boost::asio::io_context & ios, const basic_pipe<CharT, Traits> & p)
            : async_pipe(ios, ios, p)
    {
    }

    template<class CharT, class Traits = std::char_traits<CharT>>
    inline async_pipe& operator=(const basic_pipe<CharT, Traits>& p);
    inline async_pipe& operator=(const async_pipe& rhs);

    inline async_pipe& operator=(async_pipe&& rhs);

    ~async_pipe()
    {
        boost::system::error_code ec;
        close(ec);
    }

    template<class CharT, class Traits = std::char_traits<CharT>>
    inline explicit operator basic_pipe<CharT, Traits>() const;

    void cancel()
    {
        if (_sink.is_open())
            _sink.cancel();
        if (_source.is_open())
            _source.cancel();
    }

    void close()
    {
        if (_sink.is_open())
        {
            _sink.close();
            _sink = handle_type(_sink.get_executor());
        }
        if (_source.is_open())
        {
            _source.close();
            _source = handle_type(_source.get_executor());
        }
    }
    void close(boost::system::error_code & ec)
    {
        if (_sink.is_open())
        {
            _sink.close(ec);
            _sink = handle_type(_sink.get_executor());
        }
        if (_source.is_open())
        {
            _source.close(ec);
            _source = handle_type(_source.get_executor());
        }
    }

    bool is_open() const
    {
        return  _sink.is_open() || _source.is_open();
    }
    void async_close()
    {
        if (_sink.is_open())
            boost::asio::post(_sink.get_executor(),   [this]{_sink.close();});
        if (_source.is_open())
            boost::asio::post(_source.get_executor(), [this]{_source.close();});
    }

    template<typename MutableBufferSequence>
    std::size_t read_some(const MutableBufferSequence & buffers)
    {
        return _source.read_some(buffers);
    }
    template<typename MutableBufferSequence>
    std::size_t write_some(const MutableBufferSequence & buffers)
    {
        return _sink.write_some(buffers);
    }


    template<typename MutableBufferSequence>
    std::size_t read_some(const MutableBufferSequence & buffers, boost::system::error_code & ec) noexcept
    {
        return _source.read_some(buffers, ec);
    }
    template<typename MutableBufferSequence>
    std::size_t write_some(const MutableBufferSequence & buffers, boost::system::error_code & ec) noexcept
    {
        return _sink.write_some(buffers, ec);
    }

    native_handle_type native_source() const {return const_cast<boost::asio::windows::stream_handle&>(_source).native_handle();}
    native_handle_type native_sink  () const {return const_cast<boost::asio::windows::stream_handle&>(_sink  ).native_handle();}

    template<typename MutableBufferSequence,
             typename ReadHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(
          ReadHandler, void(boost::system::error_code, std::size_t))
      async_read_some(
        const MutableBufferSequence & buffers,
              ReadHandler &&handler)
    {
        return _source.async_read_some(buffers, std::forward<ReadHandler>(handler));
    }

    template<typename ConstBufferSequence,
             typename WriteHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(
              WriteHandler, void(boost::system::error_code, std::size_t))
      async_write_some(
        const ConstBufferSequence & buffers,
        WriteHandler && handler)
    {
        return _sink.async_write_some(buffers,  std::forward<WriteHandler>(handler));
    }

    const handle_type & sink  () const & {return _sink;}
    const handle_type & source() const & {return _source;}

    handle_type && source() && { return std::move(_source); }
    handle_type && sink()   && { return std::move(_sink); }

    handle_type source(::boost::asio::io_context& ios) &&
    {
        ::boost::asio::windows::stream_handle stolen(ios.get_executor(), _source.native_handle());
        boost::system::error_code ec;
        _source.assign(::boost::winapi::INVALID_HANDLE_VALUE_, ec);
        return stolen;
    }
    handle_type sink  (::boost::asio::io_context& ios) &&
    {
        ::boost::asio::windows::stream_handle stolen(ios.get_executor(), _sink.native_handle());
        boost::system::error_code ec;
        _sink.assign(::boost::winapi::INVALID_HANDLE_VALUE_, ec);
        return stolen;
    }

    handle_type source(::boost::asio::io_context& ios) const &
    {
        auto proc = ::boost::winapi::GetCurrentProcess();

        ::boost::winapi::HANDLE_ source;
        auto source_in = const_cast<handle_type&>(_source).native_handle();
        if (source_in == ::boost::winapi::INVALID_HANDLE_VALUE_)
            source = ::boost::winapi::INVALID_HANDLE_VALUE_;
        else if (!::boost::winapi::DuplicateHandle(
                proc, source_in, proc, &source, 0,
                static_cast<::boost::winapi::BOOL_>(true),
                 ::boost::winapi::DUPLICATE_SAME_ACCESS_))
            throw_last_error("Duplicate Pipe Failed");

        return ::boost::asio::windows::stream_handle(ios.get_executor(), source);
    }
    handle_type sink  (::boost::asio::io_context& ios) const &
    {
        auto proc = ::boost::winapi::GetCurrentProcess();

        ::boost::winapi::HANDLE_ sink;
        auto sink_in = const_cast<handle_type&>(_sink).native_handle();
        if (sink_in == ::boost::winapi::INVALID_HANDLE_VALUE_)
            sink = ::boost::winapi::INVALID_HANDLE_VALUE_;
        else if (!::boost::winapi::DuplicateHandle(
                proc, sink_in, proc, &sink, 0,
                static_cast<::boost::winapi::BOOL_>(true),
                 ::boost::winapi::DUPLICATE_SAME_ACCESS_))
            throw_last_error("Duplicate Pipe Failed");

        return ::boost::asio::windows::stream_handle(ios.get_executor(), sink);
    }
};

async_pipe::async_pipe(const async_pipe& p)  :
    _source(const_cast<handle_type&>(p._source).get_executor()),
    _sink  (const_cast<handle_type&>(p._sink).get_executor())
{

    auto proc = ::boost::winapi::GetCurrentProcess();

    ::boost::winapi::HANDLE_ source;
    ::boost::winapi::HANDLE_ sink;

    //cannot get the handle from a const object.
    auto source_in = const_cast<handle_type&>(p._source).native_handle();
    auto sink_in   = const_cast<handle_type&>(p._sink).native_handle();

    if (source_in == ::boost::winapi::INVALID_HANDLE_VALUE_)
        source = ::boost::winapi::INVALID_HANDLE_VALUE_;
    else if (!::boost::winapi::DuplicateHandle(
            proc, source_in, proc, &source, 0,
            static_cast<::boost::winapi::BOOL_>(true),
             ::boost::winapi::DUPLICATE_SAME_ACCESS_))
        throw_last_error("Duplicate Pipe Failed");

    if (sink_in   == ::boost::winapi::INVALID_HANDLE_VALUE_)
        sink = ::boost::winapi::INVALID_HANDLE_VALUE_;
    else if (!::boost::winapi::DuplicateHandle(
            proc, sink_in, proc, &sink, 0,
            static_cast<::boost::winapi::BOOL_>(true),
             ::boost::winapi::DUPLICATE_SAME_ACCESS_))
        throw_last_error("Duplicate Pipe Failed");

    if (source != ::boost::winapi::INVALID_HANDLE_VALUE_)
        _source.assign(source);
    if (sink != ::boost::winapi::INVALID_HANDLE_VALUE_)
        _sink.  assign(sink);
}


async_pipe::async_pipe(boost::asio::io_context & ios_source,
                       boost::asio::io_context & ios_sink,
                       const std::string & name, bool private_) : _source(ios_source), _sink(ios_sink)
{
    static constexpr int FILE_FLAG_OVERLAPPED_  = 0x40000000; //temporary

    ::boost::winapi::HANDLE_ source = ::boost::winapi::create_named_pipe(
#if defined(BOOST_NO_ANSI_APIS)
            ::boost::process::detail::convert(name).c_str(),
#else
            name.c_str(),
#endif
            ::boost::winapi::PIPE_ACCESS_INBOUND_
            | FILE_FLAG_OVERLAPPED_, //write flag
            0, private_ ? 1 : ::boost::winapi::PIPE_UNLIMITED_INSTANCES_, 8192, 8192, 0, nullptr);


    if (source == boost::winapi::INVALID_HANDLE_VALUE_)
        ::boost::process::detail::throw_last_error("create_named_pipe(" + name + ") failed");

    _source.assign(source);

    ::boost::winapi::HANDLE_ sink = boost::winapi::create_file(
#if defined(BOOST_NO_ANSI_APIS)
            ::boost::process::detail::convert(name).c_str(),
#else
            name.c_str(),
#endif
            ::boost::winapi::GENERIC_WRITE_, 0, nullptr,
            ::boost::winapi::OPEN_EXISTING_,
            FILE_FLAG_OVERLAPPED_, //to allow read
            nullptr);

    if (sink == ::boost::winapi::INVALID_HANDLE_VALUE_)
        ::boost::process::detail::throw_last_error("create_file() failed");

    _sink.assign(sink);
}

template<class CharT, class Traits>
async_pipe& async_pipe::operator=(const basic_pipe<CharT, Traits> & p)
{
    auto proc = ::boost::winapi::GetCurrentProcess();

    ::boost::winapi::HANDLE_ source;
    ::boost::winapi::HANDLE_ sink;

    //cannot get the handle from a const object.
    auto source_in = p.native_source();
    auto sink_in   = p.native_sink();

    if (source_in == ::boost::winapi::INVALID_HANDLE_VALUE_)
        source = ::boost::winapi::INVALID_HANDLE_VALUE_;
    else if (!::boost::winapi::DuplicateHandle(
            proc, source_in.native_handle(), proc, &source, 0,
            static_cast<::boost::winapi::BOOL_>(true),
            ::boost::winapi::DUPLICATE_SAME_ACCESS_))
        throw_last_error("Duplicate Pipe Failed");

    if (sink_in   == ::boost::winapi::INVALID_HANDLE_VALUE_)
        sink = ::boost::winapi::INVALID_HANDLE_VALUE_;
    else if (!::boost::winapi::DuplicateHandle(
            proc, sink_in.native_handle(), proc, &sink, 0,
            static_cast<::boost::winapi::BOOL_>(true),
            ::boost::winapi::DUPLICATE_SAME_ACCESS_))
        throw_last_error("Duplicate Pipe Failed");

    //so we also assign the io_context
    if (source != ::boost::winapi::INVALID_HANDLE_VALUE_)
        _source.assign(source);

    if (sink != ::boost::winapi::INVALID_HANDLE_VALUE_)
        _sink.assign(sink);

    return *this;
}

async_pipe& async_pipe::operator=(const async_pipe & p)
{
    auto proc = ::boost::winapi::GetCurrentProcess();

    ::boost::winapi::HANDLE_ source;
    ::boost::winapi::HANDLE_ sink;

    //cannot get the handle from a const object.
    auto &source_in = const_cast<::boost::asio::windows::stream_handle &>(p._source);
    auto &sink_in   = const_cast<::boost::asio::windows::stream_handle &>(p._sink);

    source_in.get_executor();

    if (source_in.native_handle() == ::boost::winapi::INVALID_HANDLE_VALUE_)
        source = ::boost::winapi::INVALID_HANDLE_VALUE_;
    else if (!::boost::winapi::DuplicateHandle(
            proc, source_in.native_handle(), proc, &source, 0,
            static_cast<::boost::winapi::BOOL_>(true),
             ::boost::winapi::DUPLICATE_SAME_ACCESS_))
        throw_last_error("Duplicate Pipe Failed");

    if (sink_in.native_handle()   == ::boost::winapi::INVALID_HANDLE_VALUE_)
        sink = ::boost::winapi::INVALID_HANDLE_VALUE_;
    else if (!::boost::winapi::DuplicateHandle(
            proc, sink_in.native_handle(), proc, &sink, 0,
            static_cast<::boost::winapi::BOOL_>(true),
             ::boost::winapi::DUPLICATE_SAME_ACCESS_))
        throw_last_error("Duplicate Pipe Failed");

    //so we also assign the io_context
    if (source != ::boost::winapi::INVALID_HANDLE_VALUE_)
        _source = ::boost::asio::windows::stream_handle(source_in.get_executor(), source);
    else
        _source = ::boost::asio::windows::stream_handle(source_in.get_executor());

    if (sink != ::boost::winapi::INVALID_HANDLE_VALUE_)
        _sink   = ::boost::asio::windows::stream_handle(source_in.get_executor(), sink);
    else
        _sink   = ::boost::asio::windows::stream_handle(source_in.get_executor());

    return *this;
}

async_pipe& async_pipe::operator=(async_pipe && rhs)
{
    _source = std::move(rhs._source);
    _sink = std::move(rhs._sink);
    return *this;
}

template<class CharT, class Traits>
async_pipe::operator basic_pipe<CharT, Traits>() const
{
    auto proc = ::boost::winapi::GetCurrentProcess();

    ::boost::winapi::HANDLE_ source;
    ::boost::winapi::HANDLE_ sink;

    //cannot get the handle from a const object.
    auto source_in = const_cast<::boost::asio::windows::stream_handle &>(_source).native_handle();
    auto sink_in   = const_cast<::boost::asio::windows::stream_handle &>(_sink).native_handle();

    if (source_in == ::boost::winapi::INVALID_HANDLE_VALUE_)
        source = ::boost::winapi::INVALID_HANDLE_VALUE_;
    else if (!::boost::winapi::DuplicateHandle(
            proc, source_in, proc, &source, 0,
            static_cast<::boost::winapi::BOOL_>(true),
             ::boost::winapi::DUPLICATE_SAME_ACCESS_))
        throw_last_error("Duplicate Pipe Failed");

    if (sink_in == ::boost::winapi::INVALID_HANDLE_VALUE_)
        sink = ::boost::winapi::INVALID_HANDLE_VALUE_;
    else if (!::boost::winapi::DuplicateHandle(
            proc, sink_in, proc, &sink, 0,
            static_cast<::boost::winapi::BOOL_>(true),
             ::boost::winapi::DUPLICATE_SAME_ACCESS_))
        throw_last_error("Duplicate Pipe Failed");

    return basic_pipe<CharT, Traits>{source, sink};
}

inline bool operator==(const async_pipe & lhs, const async_pipe & rhs)
{
    return compare_handles(lhs.native_source(), rhs.native_source()) &&
           compare_handles(lhs.native_sink(),   rhs.native_sink());
}

inline bool operator!=(const async_pipe & lhs, const async_pipe & rhs)
{
    return !compare_handles(lhs.native_source(), rhs.native_source()) ||
           !compare_handles(lhs.native_sink(),   rhs.native_sink());
}

template<class Char, class Traits>
inline bool operator==(const async_pipe & lhs, const basic_pipe<Char, Traits> & rhs)
{
    return compare_handles(lhs.native_source(), rhs.native_source()) &&
           compare_handles(lhs.native_sink(),   rhs.native_sink());
}

template<class Char, class Traits>
inline bool operator!=(const async_pipe & lhs, const basic_pipe<Char, Traits> & rhs)
{
    return !compare_handles(lhs.native_source(), rhs.native_source()) ||
           !compare_handles(lhs.native_sink(),   rhs.native_sink());
}

template<class Char, class Traits>
inline bool operator==(const basic_pipe<Char, Traits> & lhs, const async_pipe & rhs)
{
    return compare_handles(lhs.native_source(), rhs.native_source()) &&
           compare_handles(lhs.native_sink(),   rhs.native_sink());
}

template<class Char, class Traits>
inline bool operator!=(const basic_pipe<Char, Traits> & lhs, const async_pipe & rhs)
{
    return !compare_handles(lhs.native_source(), rhs.native_source()) ||
           !compare_handles(lhs.native_sink(),   rhs.native_sink());
}

}}}}

#endif /* INCLUDE_BOOST_PIPE_DETAIL_WINDOWS_ASYNC_PIPE_HPP_ */

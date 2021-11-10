// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_POSIX_PIPE_HPP
#define BOOST_PROCESS_POSIX_PIPE_HPP


#include <boost/filesystem.hpp>
#include <boost/process/detail/posix/compare_handles.hpp>
#include <system_error>
#include <array>
#include <unistd.h>
#include <fcntl.h>
#include <memory>

namespace boost { namespace process { namespace detail { namespace posix {


template<class CharT, class Traits = std::char_traits<CharT>>
class basic_pipe
{
    int _source = -1;
    int _sink   = -1;
public:
    explicit basic_pipe(int source, int sink) : _source(source), _sink(sink) {}
    explicit basic_pipe(int source, int sink, const std::string&) : _source(source), _sink(sink) {}
    typedef CharT                      char_type  ;
    typedef          Traits            traits_type;
    typedef typename Traits::int_type  int_type   ;
    typedef typename Traits::pos_type  pos_type   ;
    typedef typename Traits::off_type  off_type   ;
    typedef          int               native_handle_type;

    basic_pipe()
    {
        int fds[2];
        if (::pipe(fds) == -1)
            boost::process::detail::throw_last_error("pipe(2) failed");

        _source = fds[0];
        _sink   = fds[1];
    }
    inline basic_pipe(const basic_pipe& rhs);
    explicit inline basic_pipe(const std::string& name);
    basic_pipe(basic_pipe&& lhs)  : _source(lhs._source), _sink(lhs._sink)
    {
        lhs._source = -1;
        lhs._sink   = -1;
    }
    inline basic_pipe& operator=(const basic_pipe& );
    basic_pipe& operator=(basic_pipe&& lhs)
    {
        _source = lhs._source;
        _sink   = lhs._sink ;

        lhs._source = -1;
        lhs._sink   = -1;

        return *this;
    }
    ~basic_pipe()
    {
        if (_sink   != -1)
            ::close(_sink);
        if (_source != -1)
            ::close(_source);
    }
    native_handle_type native_source() const {return _source;}
    native_handle_type native_sink  () const {return _sink;}

    void assign_source(native_handle_type h) { _source = h;}
    void assign_sink  (native_handle_type h) { _sink = h;}




    int_type write(const char_type * data, int_type count)
    {
        int_type write_len;
        while ((write_len = ::write(_sink, data, count * sizeof(char_type))) == -1)
        {
            //Try again if interrupted
            auto err = errno;
            if (err != EINTR)
                ::boost::process::detail::throw_last_error();
        }
        return write_len;
    }
    int_type read(char_type * data, int_type count)
    {
        int_type read_len;
        while ((read_len = ::read(_source, data, count * sizeof(char_type))) == -1)
        {
            //Try again if interrupted
            auto err = errno;
            if (err != EINTR)
                ::boost::process::detail::throw_last_error();
        }
        return read_len;
    }

    bool is_open() const
    {
        return (_source != -1) ||
               (_sink   != -1);
    }

    void close()
    {
        if (_source != -1)
            ::close(_source);
        if (_sink != -1)
            ::close(_sink);
        _source = -1;
        _sink   = -1;
    }
};

template<class CharT, class Traits>
basic_pipe<CharT, Traits>::basic_pipe(const basic_pipe & rhs)
{
       if (rhs._source != -1)
       {
           _source = ::dup(rhs._source);
           if (_source == -1)
               ::boost::process::detail::throw_last_error("dup() failed");
       }
    if (rhs._sink != -1)
    {
        _sink = ::dup(rhs._sink);
        if (_sink == -1)
            ::boost::process::detail::throw_last_error("dup() failed");

    }
}

template<class CharT, class Traits>
basic_pipe<CharT, Traits> &basic_pipe<CharT, Traits>::operator=(const basic_pipe & rhs)
{
       if (rhs._source != -1)
       {
           _source = ::dup(rhs._source);
           if (_source == -1)
               ::boost::process::detail::throw_last_error("dup() failed");
       }
    if (rhs._sink != -1)
    {
        _sink = ::dup(rhs._sink);
        if (_sink == -1)
            ::boost::process::detail::throw_last_error("dup() failed");

    }
    return *this;
}


template<class CharT, class Traits>
basic_pipe<CharT, Traits>::basic_pipe(const std::string & name)
{
    auto fifo = mkfifo(name.c_str(), 0666 );
            
    if (fifo != 0) 
        boost::process::detail::throw_last_error("mkfifo() failed");

    
    int  read_fd = open(name.c_str(), O_RDWR);
        
    if (read_fd == -1)
        boost::process::detail::throw_last_error();
    
    int write_fd = dup(read_fd);
    
    if (write_fd == -1)
        boost::process::detail::throw_last_error();

    _sink = write_fd;
    _source = read_fd;
    ::unlink(name.c_str());
}

template<class Char, class Traits>
inline bool operator==(const basic_pipe<Char, Traits> & lhs, const basic_pipe<Char, Traits> & rhs)
{
    return compare_handles(lhs.native_source(), rhs.native_source()) &&
           compare_handles(lhs.native_sink(),   rhs.native_sink());
}

template<class Char, class Traits>
inline bool operator!=(const basic_pipe<Char, Traits> & lhs, const basic_pipe<Char, Traits> & rhs)
{
    return !compare_handles(lhs.native_source(), rhs.native_source()) ||
           !compare_handles(lhs.native_sink(),   rhs.native_sink());
}

}}}}

#endif

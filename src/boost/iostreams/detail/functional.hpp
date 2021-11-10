/*
 * Distributed under the Boost Software License, Version 1.0.(See accompanying 
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)
 * 
 * See http://www.boost.org/libs/iostreams for documentation.

 * File:        boost/iostreams/detail/functional.hpp
 * Date:        Sun Dec 09 05:38:03 MST 2007
 * Copyright:   2007-2008 CodeRage, LLC
 * Author:      Jonathan Turkanis
 * Contact:     turkanis at coderage dot com

 * Defines several function objects and object generators for use with 
 * execute_all()
 */

#ifndef BOOST_IOSTREAMS_DETAIL_FUNCTIONAL_HPP_INCLUDED
#define BOOST_IOSTREAMS_DETAIL_FUNCTIONAL_HPP_INCLUDED

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/iostreams/close.hpp>
#include <boost/iostreams/detail/ios.hpp> // BOOST_IOS

namespace boost { namespace iostreams { namespace detail {

    // Function objects and object generators for invoking
    // boost::iostreams::close

template<typename T>
class device_close_operation {
public:
    typedef void result_type;
    device_close_operation(T& t, BOOST_IOS::openmode which) 
        : t_(t), which_(which) 
        { }
    void operator()() const { boost::iostreams::close(t_, which_); }
private:
    BOOST_DELETED_FUNCTION(device_close_operation& operator=(const device_close_operation&));
    T&                   t_;
    BOOST_IOS::openmode  which_;
};

template<typename T, typename Sink>
class filter_close_operation {
public:
    typedef void result_type;
    filter_close_operation(T& t, Sink& snk, BOOST_IOS::openmode which)
        : t_(t), snk_(snk), which_(which)
        { }
    void operator()() const { boost::iostreams::close(t_, snk_, which_); }
private:
    BOOST_DELETED_FUNCTION(filter_close_operation& operator=(const filter_close_operation&));
    T&                   t_;
    Sink&                snk_;
    BOOST_IOS::openmode  which_;
};

template<typename T>
device_close_operation<T> 
call_close(T& t, BOOST_IOS::openmode which) 
{ return device_close_operation<T>(t, which); }

template<typename T, typename Sink>
filter_close_operation<T, Sink> 
call_close(T& t, Sink& snk, BOOST_IOS::openmode which) 
{ return filter_close_operation<T, Sink>(t, snk, which); }

    // Function objects and object generators for invoking
    // boost::iostreams::detail::close_all

template<typename T>
class device_close_all_operation {
public:
    typedef void result_type;
    device_close_all_operation(T& t) : t_(t) { }
    void operator()() const { detail::close_all(t_); }
private:
    BOOST_DELETED_FUNCTION(device_close_all_operation& operator=(const device_close_all_operation&));
    T& t_;
};

template<typename T, typename Sink>
class filter_close_all_operation {
public:
    typedef void result_type;
    filter_close_all_operation(T& t, Sink& snk) : t_(t), snk_(snk) { }
    void operator()() const { detail::close_all(t_, snk_); }
private:
    BOOST_DELETED_FUNCTION(filter_close_all_operation& operator=(const filter_close_all_operation&));
    T&     t_;
    Sink&  snk_;
};

template<typename T>
device_close_all_operation<T> call_close_all(T& t) 
{ return device_close_all_operation<T>(t); }

template<typename T, typename Sink>
filter_close_all_operation<T, Sink> 
call_close_all(T& t, Sink& snk) 
{ return filter_close_all_operation<T, Sink>(t, snk); }

    // Function object and object generator for invoking a
    // member function void close(std::ios_base::openmode)

template<typename T>
class member_close_operation {
public:
    typedef void result_type;
    member_close_operation(T& t, BOOST_IOS::openmode which) 
        : t_(t), which_(which) 
        { }
    void operator()() const { t_.close(which_); }
private:
    BOOST_DELETED_FUNCTION(member_close_operation& operator=(const member_close_operation&));
    T&                   t_;
    BOOST_IOS::openmode  which_;
};

template<typename T>
member_close_operation<T> call_member_close(T& t, BOOST_IOS::openmode which) 
{ return member_close_operation<T>(t, which); }

    // Function object and object generator for invoking a
    // member function void reset()

template<typename T>
class reset_operation {
public:
    reset_operation(T& t) : t_(t) { }
    void operator()() const { t_.reset(); }
private:
    BOOST_DELETED_FUNCTION(reset_operation& operator=(const reset_operation&));
    T& t_;
};

template<typename T>
reset_operation<T> call_reset(T& t) { return reset_operation<T>(t); }

    // Function object and object generator for clearing a flag

template<typename T>
class clear_flags_operation {
public:
    typedef void result_type;
    clear_flags_operation(T& t) : t_(t) { }
    void operator()() const { t_ = 0; }
private:
    BOOST_DELETED_FUNCTION(clear_flags_operation& operator=(const clear_flags_operation&));
    T& t_;
};

template<typename T>
clear_flags_operation<T> clear_flags(T& t) 
{ return clear_flags_operation<T>(t); }

    // Function object and generator for flushing a buffer

// Function object for use with execute_all()
template<typename Buffer, typename Device>
class flush_buffer_operation {
public:
    typedef void result_type;
    flush_buffer_operation(Buffer& buf, Device& dev, bool flush)
        : buf_(buf), dev_(dev), flush_(flush)
        { }
    void operator()() const
    {
        if (flush_) 
            buf_.flush(dev_);
    }
private:
    BOOST_DELETED_FUNCTION(flush_buffer_operation& operator=(const flush_buffer_operation&));
    Buffer&  buf_;
    Device&  dev_;
    bool     flush_;
};

template<typename Buffer, typename Device>
flush_buffer_operation<Buffer, Device> 
flush_buffer(Buffer& buf, Device& dev, bool flush)
{ return flush_buffer_operation<Buffer, Device>(buf, dev, flush); }

} } } // End namespaces detail, iostreams, boost.

#endif // #ifndef BOOST_IOSTREAMS_DETAIL_FUNCTIONAL_HPP_INCLUDED

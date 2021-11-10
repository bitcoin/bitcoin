// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2005-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_DETAIL_NON_BLOCKING_ADAPTER_HPP_INCLUDED
#define BOOST_IOSTREAMS_DETAIL_NON_BLOCKING_ADAPTER_HPP_INCLUDED

#include <boost/iostreams/detail/ios.hpp>  // streamsize, seekdir, openmode.
#include <boost/iostreams/read.hpp>
#include <boost/iostreams/seek.hpp>
#include <boost/iostreams/traits.hpp>
#include <boost/iostreams/write.hpp>

namespace boost { namespace iostreams {

template<typename Device>
class non_blocking_adapter {
public:
    typedef typename char_type_of<Device>::type char_type;
    struct category
        : mode_of<Device>::type, device_tag
        { };
    explicit non_blocking_adapter(Device& dev) : device_(dev) { }
    std::streamsize read(char_type* s, std::streamsize n)
    { 
        std::streamsize result = 0;
        while (result < n) {
            std::streamsize amt = iostreams::read(device_, s + result, n - result);
            if (amt == -1)
                break;
            result += amt;
        }
        return result != 0 ? result : -1;
    }
    std::streamsize write(const char_type* s, std::streamsize n)
    { 
        std::streamsize result = 0;
        while (result < n) {
            std::streamsize amt = 
                iostreams::write(device_, s + result, n - result);
            // write errors, like EOF on read, need to be handled.
            if (amt == -1)
                break;
            result += amt;
        }
        return result;    
    }
    std::streampos seek( stream_offset off, BOOST_IOS::seekdir way,
                         BOOST_IOS::openmode which = 
                             BOOST_IOS::in | BOOST_IOS::out )
    { return iostreams::seek(device_, off, way, which); }
public:
    non_blocking_adapter& operator=(const non_blocking_adapter&);
    Device& device_;
};

} } // End namespace iostreams.

#endif // #ifndef BOOST_IOSTREAMS_DETAIL_NON_BLOCKING_ADAPTER_HPP_INCLUDED

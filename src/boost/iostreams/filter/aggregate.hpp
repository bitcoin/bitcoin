// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_AGGREGATE_FILTER_HPP_INCLUDED
#define BOOST_IOSTREAMS_AGGREGATE_FILTER_HPP_INCLUDED

#if defined(_MSC_VER)
# pragma once
#endif              

#include <algorithm>                          // copy, min.
#include <boost/assert.hpp>
#include <iterator>                           // back_inserter
#include <vector>
#include <boost/iostreams/constants.hpp>      // default_device_buffer_size 
#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/detail/char_traits.hpp>
#include <boost/iostreams/detail/ios.hpp>     // openmode, streamsize.
#include <boost/iostreams/pipeline.hpp>
#include <boost/iostreams/read.hpp>           // check_eof 
#include <boost/iostreams/write.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/type_traits/is_convertible.hpp>

// Must come last.
#include <boost/iostreams/detail/config/disable_warnings.hpp>  // MSVC.

namespace boost { namespace iostreams {

//
// Template name: aggregate_filter.
// Template parameters:
//      Ch - The character type.
//      Alloc - The allocator type.
// Description: Utility for defining DualUseFilters which filter an
//      entire stream at once. To use, override the protected virtual
//      member do_filter.
// Note: This filter should not be copied while it is in use.
//
template<typename Ch, typename Alloc = std::allocator<Ch> >
class aggregate_filter  {
public:
    typedef Ch char_type;
    struct category
        : dual_use,
          filter_tag,
          multichar_tag,
          closable_tag
        { };
    aggregate_filter() : ptr_(0), state_(0) { }
    virtual ~aggregate_filter() { }

    template<typename Source>
    std::streamsize read(Source& src, char_type* s, std::streamsize n)
    {
        using namespace std;
        BOOST_ASSERT(!(state_ & f_write));
        state_ |= f_read;
        if (!(state_ & f_eof))
            do_read(src);
        std::streamsize amt =
            (std::min)(n, static_cast<std::streamsize>(data_.size() - ptr_));
        if (amt) {
            BOOST_IOSTREAMS_CHAR_TRAITS(char_type)::copy(s, &data_[ptr_], amt);
            ptr_ += amt;
        }
        return detail::check_eof(amt);
    }

    template<typename Sink>
    std::streamsize write(Sink&, const char_type* s, std::streamsize n)
    {
        BOOST_ASSERT(!(state_ & f_read));
        state_ |= f_write;
        data_.insert(data_.end(), s, s + n);
        return n;
    }

    template<typename Sink>
    void close(Sink& sink, BOOST_IOS::openmode which)
    {
        if ((state_ & f_read) != 0 && which == BOOST_IOS::in)
            close_impl();
        if ((state_ & f_write) != 0 && which == BOOST_IOS::out) {
            try {
                vector_type filtered;
                do_filter(data_, filtered);
                do_write( 
                    sink, &filtered[0],
                    static_cast<std::streamsize>(filtered.size())
                );
            } catch (...) {
                close_impl();
                throw;
            }
            close_impl();
        }
    }

protected:
    typedef std::vector<Ch, Alloc>           vector_type;
    typedef typename vector_type::size_type  size_type;
private:
    virtual void do_filter(const vector_type& src, vector_type& dest) = 0;
    virtual void do_close() { }

    template<typename Source>
    void do_read(Source& src)
    {
        using std::streamsize;
        vector_type data;
        while (true) {
            const std::streamsize  size = default_device_buffer_size;
            Ch                     buf[size];
            std::streamsize        amt;
            if ((amt = boost::iostreams::read(src, buf, size)) == -1)
                break;
            data.insert(data.end(), buf, buf + amt);
        }
        do_filter(data, data_);
        state_ |= f_eof;
    }

    template<typename Sink>
    void do_write(Sink& sink, const char_type* s, std::streamsize n) 
    { 
        typedef typename iostreams::category_of<Sink>::type  category;
        typedef is_convertible<category, output>             can_write;
        do_write(sink, s, n, can_write()); 
    }

    template<typename Sink>
    void do_write(Sink& sink, const char_type* s, std::streamsize n, mpl::true_) 
    { iostreams::write(sink, s, n); }

    template<typename Sink>
    void do_write(Sink&, const char_type*, std::streamsize, mpl::false_) { }

    void close_impl()
    {
        data_.clear();
        ptr_ = 0;
        state_ = 0;
        do_close();
    }

    enum flag_type {
        f_read   = 1,
        f_write  = f_read << 1,
        f_eof    = f_write << 1
    };

    // Note: typically will not be copied while vector contains data.
    vector_type  data_;
    size_type    ptr_;
    int          state_;
};
BOOST_IOSTREAMS_PIPABLE(aggregate_filter, 1)

} } // End namespaces iostreams, boost.

#include <boost/iostreams/detail/config/enable_warnings.hpp>  // MSVC.

#endif // #ifndef BOOST_IOSTREAMS_AGGREGATE_FILTER_HPP_INCLUDED

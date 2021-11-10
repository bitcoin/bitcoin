// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_DETAIL_RANGE_ADAPTER_HPP_INCLUDED
#define BOOST_IOSTREAMS_DETAIL_RANGE_ADAPTER_HPP_INCLUDED

#if defined(_MSC_VER)
# pragma once
#endif

#include <algorithm>                             // min.
#include <boost/assert.hpp>
#include <cstddef>                               // ptrdiff_t.
#include <iosfwd>                                // streamsize, streamoff.
#include <iterator>                              // iterator_traits.
#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/detail/error.hpp>
#include <boost/iostreams/positioning.hpp>
#include <boost/mpl/if.hpp>
#include <boost/throw_exception.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/core/enable_if.hpp>

// Must come last.
#include <boost/iostreams/detail/config/disable_warnings.hpp>  // MSVC.

namespace boost { namespace iostreams { namespace detail {

// Used for simulated tag dispatch.
template<typename Traversal> struct range_adapter_impl;

//
// Template name: range_adapter
// Description: Device based on an instance of boost::iterator_range.
// Template parameters:
//     Mode - A mode tag.
//     Range - An instance of iterator_range.
//
template<typename Mode, typename Range>
class range_adapter {
private:
    typedef typename Range::iterator                  iterator;
    typedef std::iterator_traits<iterator>            iter_traits;
    typedef typename iter_traits::iterator_category   iter_cat;
public:
    typedef typename Range::value_type                char_type;
    struct category : Mode, device_tag { };
    typedef typename
            mpl::if_<
                is_convertible<
                    iter_cat,
                    std::random_access_iterator_tag
                >,
                std::random_access_iterator_tag,
                std::forward_iterator_tag
            >::type                                   tag;
    typedef range_adapter_impl<tag>                   impl;

    explicit range_adapter(const Range& rng);
    range_adapter(iterator first, iterator last);
    std::streamsize read(char_type* s, std::streamsize n);
    std::streamsize write(const char_type* s, std::streamsize n);
    std::streampos seek(stream_offset off, BOOST_IOS::seekdir way);
private:
    iterator first_, cur_, last_;
};

//------------------Implementation of range_adapter---------------------------//

template<typename Mode, typename Range>
range_adapter<Mode, Range>::range_adapter(const Range& rng)
    : first_(rng.begin()), cur_(rng.begin()), last_(rng.end()) { }

template<typename Mode, typename Range>
range_adapter<Mode, Range>::range_adapter(iterator first, iterator last)
    : first_(first), cur_(first), last_(last) { }

template<typename Mode, typename Range>
inline std::streamsize range_adapter<Mode, Range>::read
    (char_type* s, std::streamsize n)
{ return impl::read(cur_, last_, s, n); }

template<typename Mode, typename Range>
inline std::streamsize range_adapter<Mode, Range>::write
    (const char_type* s, std::streamsize n)
{ return impl::write(cur_, last_, s, n); }


template<typename Mode, typename Range>
std::streampos range_adapter<Mode, Range>::seek
    (stream_offset off, BOOST_IOS::seekdir way)
{ 
    impl::seek(first_, cur_, last_, off, way); 
    return offset_to_position(cur_ - first_);
}

//------------------Implementation of range_adapter_impl----------------------//

template<>
struct range_adapter_impl<std::forward_iterator_tag> {
    template<typename Iter, typename Ch>
    static std::streamsize read
        (Iter& cur, Iter& last, Ch* s,std::streamsize n)
    {
        std::streamsize rem = n; // No. of chars remaining.
        while (cur != last && rem-- > 0) *s++ = *cur++;
        return n - rem != 0 ? n - rem : -1;
    }

    template<typename Iter, typename Ch>
    static std::streamsize write
        (Iter& cur, Iter& last, const Ch* s, std::streamsize n)
    {
        while (cur != last && n-- > 0) *cur++ = *s++;
        if (cur == last && n > 0)
            boost::throw_exception(write_area_exhausted());
        return n;
    }
};

template<>
struct range_adapter_impl<std::random_access_iterator_tag> {
    template<typename Iter, typename Ch>
    static std::streamsize read
        (Iter& cur, Iter& last, Ch* s,std::streamsize n)
    {
        std::streamsize result = 
            (std::min)(static_cast<std::streamsize>(last - cur), n);
        if (result)
            std::copy(cur, cur + result, s);
        cur += result;
        return result != 0 ? result : -1;
    }

    template<typename Iter, typename Ch>
    static std::streamsize write
        (Iter& cur, Iter& last, const Ch* s, std::streamsize n)
    {
        std::streamsize count =
            (std::min)(static_cast<std::streamsize>(last - cur), n);
        std::copy(s, s + count, cur);
        cur += count;
        if (count < n) 
            boost::throw_exception(write_area_exhausted());
        return n;
    }

    template<typename Iter>
    static void seek
        ( Iter& first, Iter& cur, Iter& last, stream_offset off,
          BOOST_IOS::seekdir way )
    {
        using namespace std;
        switch (way) {
        case BOOST_IOS::beg:
            if (off > last - first || off < 0)
                boost::throw_exception(bad_seek());
            cur = first + off;
            break;
        case BOOST_IOS::cur:
            {
                std::ptrdiff_t newoff = cur - first + off;
                if (newoff > last - first || newoff < 0)
                    boost::throw_exception(bad_seek());
                cur += off;
                break;
            }
        case BOOST_IOS::end:
            if (last - first + off < 0 || off > 0)
                boost::throw_exception(bad_seek());
            cur = last + off;
            break;
        default:
            BOOST_ASSERT(0);
        }
    }
};

} } } // End namespaces detail, iostreams, boost.

#include <boost/iostreams/detail/config/enable_warnings.hpp>  // MSVC.

#endif // #ifndef BOOST_IOSTREAMS_DETAIL_RANGE_ADAPTER_HPP_INCLUDED //---------------//

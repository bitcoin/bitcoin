// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2005-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

// Note: bidirectional streams are not supported.

#ifndef BOOST_IOSTREAMS_COMPOSE_HPP_INCLUDED
#define BOOST_IOSTREAMS_COMPOSE_HPP_INCLUDED

#if defined(_MSC_VER)
# pragma once
#endif

#include <algorithm>          // min.
#include <utility>            // pair.
#include <boost/config.hpp>   // DEDUCED_TYPENAME.
#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/detail/adapter/direct_adapter.hpp>
#include <boost/iostreams/detail/call_traits.hpp>
#include <boost/iostreams/detail/enable_if_stream.hpp>
#include <boost/iostreams/detail/execute.hpp>
#include <boost/iostreams/detail/functional.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/iostreams/traits.hpp>      // mode_of, is_direct.
#include <boost/mpl/if.hpp>
#include <boost/ref.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_convertible.hpp>

// Must come last.
#include <boost/iostreams/detail/config/disable_warnings.hpp>  // MSVC.

namespace boost { namespace iostreams {

namespace detail {

template< typename First, 
          typename Second,
          typename FirstMode = 
              BOOST_DEDUCED_TYPENAME mode_of<First>::type,
          typename SecondMode = 
              BOOST_DEDUCED_TYPENAME mode_of<Second>::type >
struct composite_mode
    : select<
          is_convertible<SecondMode, FirstMode>, FirstMode,
          is_convertible<FirstMode, SecondMode>, SecondMode,
          is_convertible<SecondMode, input>,     input,
          else_,                                 output
      >
    { };

//
// Template name: composite_device.
// Description: Provides a Device view of a Filter, Device pair.
// Template parameters:
//      Filter - A model of Filter.
//      Device - An indirect model of Device.
//
template< typename Filter,
          typename Device,
          typename Mode =
              BOOST_DEDUCED_TYPENAME composite_mode<Filter, Device>::type >
class composite_device {
private:
    typedef typename detail::param_type<Device>::type       param_type;
    typedef typename mode_of<Filter>::type                  filter_mode;
    typedef typename mode_of<Device>::type                  device_mode;
    typedef typename
            iostreams::select<  // Disambiguation for Tru64.
                is_direct<Device>,  direct_adapter<Device>,
                is_std_io<Device>,  Device&,
                else_,              Device
            >::type                                         value_type;
    BOOST_STATIC_ASSERT(is_filter<Filter>::value);
    BOOST_STATIC_ASSERT(is_device<Device>::value);
public:
    typedef typename char_type_of<Filter>::type             char_type;
    struct category
        : Mode,
          device_tag,
          closable_tag,
          flushable_tag,
          localizable_tag,
          optimally_buffered_tag
        { };
    composite_device(const Filter& flt, param_type dev);
    std::streamsize read(char_type* s, std::streamsize n);
    std::streamsize write(const char_type* s, std::streamsize n);
    std::streampos seek( stream_offset off, BOOST_IOS::seekdir way,
                         BOOST_IOS::openmode which =
                             BOOST_IOS::in | BOOST_IOS::out );

    void close();
    void close(BOOST_IOS::openmode which);
    bool flush();
    std::streamsize optimal_buffer_size() const;

    template<typename Locale> // Avoid dependency on <locale>
    void imbue(const Locale& loc)
    {
        iostreams::imbue(filter_, loc);
        iostreams::imbue(device_, loc);
    }

    Filter& first() { return filter_; }
    Device& second() { return device_; }
private:
    Filter      filter_;
    value_type  device_;
};

//
// Template name: composite_device.
// Description: Provides a Device view of a Filter, Device pair.
// Template parameters:
//      Filter - A model of Filter.
//      Device - An indirect model of Device.
//
template< typename Filter1, 
          typename Filter2,
          typename Mode =
              BOOST_DEDUCED_TYPENAME composite_mode<Filter1, Filter2>::type >
class composite_filter {
private:
    typedef reference_wrapper<Filter2>           filter_ref;
    typedef typename mode_of<Filter1>::type      first_mode;
    typedef typename mode_of<Filter2>::type      second_mode;

    // A dual-use filter cannot be composed with a read-write filter
    BOOST_STATIC_ASSERT(
        !(is_convertible<first_mode, dual_use>::value) ||
        !(is_convertible<second_mode, input>::value) ||
        !(is_convertible<second_mode, output>::value) ||
         (is_convertible<second_mode, dual_use>::value)
    );
    BOOST_STATIC_ASSERT(
        !(is_convertible<second_mode, dual_use>::value) ||
        !(is_convertible<first_mode, input>::value) ||
        !(is_convertible<first_mode, output>::value) ||
         (is_convertible<first_mode, dual_use>::value)
    );
    BOOST_STATIC_ASSERT(is_filter<Filter1>::value);
    BOOST_STATIC_ASSERT(is_filter<Filter2>::value);
public:
    typedef typename char_type_of<Filter1>::type  char_type;
    struct category
        : Mode,
          filter_tag,
          multichar_tag,
          closable_tag,
          flushable_tag,
          localizable_tag,
          optimally_buffered_tag
        { };
    composite_filter(const Filter1& filter1, const Filter2& filter2)
        : filter1_(filter1), filter2_(filter2)
        { }

    template<typename Source>
    std::streamsize read(Source& src, char_type* s, std::streamsize n)
    {
        composite_device<filter_ref, Source> cmp(boost::ref(filter2_), src);
        return iostreams::read(filter1_, cmp, s, n);
    }

    template<typename Sink>
    std::streamsize write(Sink& snk, const char_type* s, std::streamsize n)
    {
        composite_device<filter_ref, Sink> cmp(boost::ref(filter2_), snk);
        return iostreams::write(filter1_, cmp, s, n);
    }

    template<typename Device>
    std::streampos seek( Device& dev, stream_offset off, BOOST_IOS::seekdir way,
                         BOOST_IOS::openmode which =
                             BOOST_IOS::in | BOOST_IOS::out )
    {
        composite_device<filter_ref, Device> cmp(boost::ref(filter2_), dev);
        return iostreams::seek(filter1_, cmp, off, way, which);
    }

    template<typename Device>
    void close(Device& dev)
    {
        BOOST_STATIC_ASSERT((!is_convertible<category, two_sequence>::value));
        BOOST_STATIC_ASSERT((!is_convertible<category, dual_use>::value));

        // Create a new device by composing the second filter2_ with dev.
        composite_device<filter_ref, Device> cmp(boost::ref(filter2_), dev);

        // Close input sequences in reverse order and output sequences in 
        // forward order
        if (!is_convertible<first_mode, dual_use>::value) {
            detail::execute_all(
                detail::call_close(filter2_, dev, BOOST_IOS::in),
                detail::call_close(filter1_, cmp, BOOST_IOS::in),
                detail::call_close(filter1_, cmp, BOOST_IOS::out),
                detail::call_close(filter2_, dev, BOOST_IOS::out)
            );
        } else if (is_convertible<second_mode, input>::value) {
            detail::execute_all(
                detail::call_close(filter2_, dev, BOOST_IOS::in),
                detail::call_close(filter1_, cmp, BOOST_IOS::in)
            );
        } else {
            detail::execute_all(
                detail::call_close(filter1_, cmp, BOOST_IOS::out),
                detail::call_close(filter2_, dev, BOOST_IOS::out)
            );
        }
    }

    template<typename Device>
    void close(Device& dev, BOOST_IOS::openmode which)
    {
        BOOST_STATIC_ASSERT(
            (is_convertible<category, two_sequence>::value) ||
            (is_convertible<category, dual_use>::value)
        );

        // Create a new device by composing the second filter2_ with dev.
        composite_device<filter_ref, Device> cmp(boost::ref(filter2_), dev);

        // Close input sequences in reverse order
        if ( which == BOOST_IOS::in &&
             ( !is_convertible<first_mode, dual_use>::value ||
                is_convertible<second_mode, input>::value ) )
        {
            detail::execute_all(
                detail::call_close(filter2_, dev, BOOST_IOS::in),
                detail::call_close(filter1_, cmp, BOOST_IOS::in)
            );
        }

        // Close output sequences in forward order
        if ( which == BOOST_IOS::out &&
             ( !is_convertible<first_mode, dual_use>::value ||
                is_convertible<second_mode, output>::value ) )
        {
            detail::execute_all(
                detail::call_close(filter1_, cmp, BOOST_IOS::out),
                detail::call_close(filter2_, dev, BOOST_IOS::out)
            );
        }
    }

    template<typename Device>
    bool flush(Device& dev)
    {
        composite_device<Filter2, Device> cmp(filter2_, dev);
        return iostreams::flush(filter1_, cmp);
    }

    std::streamsize optimal_buffer_size() const
    {
        std::streamsize first = iostreams::optimal_buffer_size(filter1_);
        std::streamsize second = iostreams::optimal_buffer_size(filter2_);
        return first < second ? second : first;
    }

    template<typename Locale> // Avoid dependency on <locale>
    void imbue(const Locale& loc)
    {   // To do: consider using RAII.
        iostreams::imbue(filter1_, loc);
        iostreams::imbue(filter2_, loc);
    }

    Filter1& first() { return filter1_; }
    Filter2& second() { return filter2_; }
private:
    Filter1  filter1_;
    Filter2  filter2_;
};

template<typename Filter, typename FilterOrDevice>
struct composite_traits
    : mpl::if_<
          is_device<FilterOrDevice>,
          composite_device<Filter, FilterOrDevice>,
          composite_filter<Filter, FilterOrDevice>
      >
    { };

} // End namespace detail.

template<typename Filter, typename FilterOrDevice>
struct composite : detail::composite_traits<Filter, FilterOrDevice>::type {
    typedef typename detail::param_type<FilterOrDevice>::type param_type;
    typedef typename detail::composite_traits<Filter, FilterOrDevice>::type base;
    composite(const Filter& flt, param_type dev)
        : base(flt, dev)
        { }
};

//--------------Implementation of compose-------------------------------------//

// Note: The following workarounds are patterned after resolve.hpp. It has not
// yet been confirmed that they are necessary.

#ifndef BOOST_IOSTREAMS_BROKEN_OVERLOAD_RESOLUTION //-------------------------//
# ifndef BOOST_IOSTREAMS_NO_STREAM_TEMPLATES //-------------------------------//

template<typename Filter, typename FilterOrDevice>
composite<Filter, FilterOrDevice>
compose( const Filter& filter, const FilterOrDevice& fod
         BOOST_IOSTREAMS_DISABLE_IF_STREAM(FilterOrDevice) )
{ return composite<Filter, FilterOrDevice>(filter, fod); }

template<typename Filter, typename Ch, typename Tr>
composite< Filter, std::basic_streambuf<Ch, Tr> >
compose(const Filter& filter, std::basic_streambuf<Ch, Tr>& sb)
{ return composite< Filter, std::basic_streambuf<Ch, Tr> >(filter, sb); }

template<typename Filter, typename Ch, typename Tr>
composite< Filter, std::basic_istream<Ch, Tr> >
compose(const Filter& filter, std::basic_istream<Ch, Tr>& is)
{ return composite< Filter, std::basic_istream<Ch, Tr> >(filter, is); }

template<typename Filter, typename Ch, typename Tr>
composite< Filter, std::basic_ostream<Ch, Tr> >
compose(const Filter& filter, std::basic_ostream<Ch, Tr>& os)
{ return composite< Filter, std::basic_ostream<Ch, Tr> >(filter, os); }

template<typename Filter, typename Ch, typename Tr>
composite< Filter, std::basic_iostream<Ch, Tr> >
compose(const Filter& filter, std::basic_iostream<Ch, Tr>& io)
{ return composite< Filter, std::basic_iostream<Ch, Tr> >(filter, io); }

# else // # ifndef BOOST_IOSTREAMS_NO_STREAM_TEMPLATES //---------------------//

template<typename Filter, typename FilterOrDevice>
composite<Filter, FilterOrDevice>
compose( const Filter& filter, const FilterOrDevice& fod
         BOOST_IOSTREAMS_DISABLE_IF_STREAM(FilterOrDevice) )
{ return composite<Filter, FilterOrDevice>(filter, fod); }

template<typename Filter>
composite<Filter, std::streambuf>
compose(const Filter& filter, std::streambuf& sb)
{ return composite<Filter, std::streambuf>(filter, sb); }

template<typename Filter>
composite<Filter, std::istream>
compose(const Filter& filter, std::istream& is)
{ return composite<Filter, std::istream>(filter, is); }

template<typename Filter>
composite<Filter, std::ostream>
compose(const Filter& filter, std::ostream& os)
{ return composite<Filter, std::ostream>(filter, os); }

template<typename Filter>
composite<Filter, std::iostream>
compose(const Filter& filter, std::iostream& io)
{ return composite<Filter, std::iostream>(filter, io); }

# endif // # ifndef BOOST_IOSTREAMS_NO_STREAM_TEMPLATES //--------------------//
#else // #ifndef BOOST_IOSTREAMS_BROKEN_OVERLOAD_RESOLUTION //----------------//

template<typename Filter, typename Stream>
composite<Filter, Stream>
compose(const Filter& flt, const Stream& strm, mpl::true_)
{   // Bad overload resolution.
    return composite<Filter, Stream>(flt, const_cast<Stream&>(strm));
}

template<typename Filter, typename FilterOrDevice>
composite<Filter, FilterOrDevice>
compose(const Filter& flt, const FilterOrDevice& fod, mpl::false_)
{ return composite<Filter, FilterOrDevice>(flt, fod); }

template<typename Filter, typename FilterOrDevice>
composite<Filter, FilterOrDevice>
compose( const Filter& flt, const FilterOrDevice& fod
         BOOST_IOSTREAMS_DISABLE_IF_STREAM(T) )
{ return compose(flt, fod, is_std_io<FilterOrDevice>()); }

# if !BOOST_WORKAROUND(BOOST_BORLANDC, < 0x600) && \
     !defined(__GNUC__) // ---------------------------------------------------//

template<typename Filter, typename FilterOrDevice>
composite<Filter, FilterOrDevice>
compose (const Filter& filter, FilterOrDevice& fod)
{ return composite<Filter, FilterOrDevice>(filter, fod); }

# endif // Borland 5.x or GCC //--------------------------------//
#endif // #ifndef BOOST_IOSTREAMS_BROKEN_OVERLOAD_RESOLUTION //---------------//

//----------------------------------------------------------------------------//

namespace detail {

//--------------Implementation of composite_device---------------------------//

template<typename Filter, typename Device, typename Mode>
composite_device<Filter, Device, Mode>::composite_device
    (const Filter& flt, param_type dev)
    : filter_(flt), device_(dev)
    { }

template<typename Filter, typename Device, typename Mode>
inline std::streamsize composite_device<Filter, Device, Mode>::read
    (char_type* s, std::streamsize n)
{ return iostreams::read(filter_, device_, s, n); }

template<typename Filter, typename Device, typename Mode>
inline std::streamsize composite_device<Filter, Device, Mode>::write
    (const char_type* s, std::streamsize n)
{ return iostreams::write(filter_, device_, s, n); }

template<typename Filter, typename Device, typename Mode>
std::streampos composite_device<Filter, Device, Mode>::seek
    (stream_offset off, BOOST_IOS::seekdir way, BOOST_IOS::openmode which)
{ return iostreams::seek(filter_, device_, off, way, which); }

template<typename Filter, typename Device, typename Mode>
void composite_device<Filter, Device, Mode>::close()
{
    BOOST_STATIC_ASSERT((!is_convertible<Mode, two_sequence>::value));
    BOOST_STATIC_ASSERT(
        !(is_convertible<filter_mode, dual_use>::value) ||
        !(is_convertible<device_mode, input>::value) ||
        !(is_convertible<device_mode, output>::value)
    );

    // Close input sequences in reverse order and output sequences 
    // in forward order
    if (!is_convertible<filter_mode, dual_use>::value) {
        detail::execute_all(
            detail::call_close(device_, BOOST_IOS::in),
            detail::call_close(filter_, device_, BOOST_IOS::in),
            detail::call_close(filter_, device_, BOOST_IOS::out),
            detail::call_close(device_, BOOST_IOS::out)
        );
    } else if (is_convertible<device_mode, input>::value) {
        detail::execute_all(
            detail::call_close(device_, BOOST_IOS::in),
            detail::call_close(filter_, device_, BOOST_IOS::in)
        );
    } else {
        detail::execute_all(
            detail::call_close(filter_, device_, BOOST_IOS::out),
            detail::call_close(device_, BOOST_IOS::out)
        );
    }
}

template<typename Filter, typename Device, typename Mode>
void composite_device<Filter, Device, Mode>::close(BOOST_IOS::openmode which)
{
    BOOST_STATIC_ASSERT((is_convertible<Mode, two_sequence>::value));
    BOOST_STATIC_ASSERT(!(is_convertible<filter_mode, dual_use>::value));

    // Close input sequences in reverse order
    if (which == BOOST_IOS::in) {
        detail::execute_all(
            detail::call_close(device_, BOOST_IOS::in),
            detail::call_close(filter_, device_, BOOST_IOS::in) 
        );
    }

    // Close output sequences in forward order
    if (which == BOOST_IOS::out) {
        detail::execute_all(
            detail::call_close(filter_, device_, BOOST_IOS::out),
            detail::call_close(device_, BOOST_IOS::out)
        );
    }
}

template<typename Filter, typename Device, typename Mode>
bool composite_device<Filter, Device, Mode>::flush()
{
    bool r1 = iostreams::flush(filter_, device_);
    bool r2 = iostreams::flush(device_);
    return r1 && r2;
}

template<typename Filter, typename Device, typename Mode>
std::streamsize
composite_device<Filter, Device, Mode>::optimal_buffer_size() const
{ return iostreams::optimal_buffer_size(device_); }

} // End namespace detail.

} } // End namespaces iostreams, boost.

#include <boost/iostreams/detail/config/enable_warnings.hpp>

#endif // #ifndef BOOST_IOSTREAMS_COMPOSE_HPP_INCLUDED

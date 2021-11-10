/*
 * Distributed under the Boost Software License, Version 1.0.(See accompanying 
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)
 * 
 * See http://www.boost.org/libs/iostreams for documentation.

 * File:        boost/iostreams/detail/restrict_impl.hpp
 * Date:        Sun Jan 06 12:57:30 MST 2008
 * Copyright:   2007-2008 CodeRage, LLC
 * Author:      Jonathan Turkanis
 * Contact:     turkanis at coderage dot com
 *
 * If included with the macro BOOST_IOSTREAMS_RESTRICT undefined, defines the 
 * class template boost::iostreams::restriction. If included with the macro
 * BOOST_IOSTREAMS_RESTRICT defined as an identifier, defines the overloaded
 * function template boost::iostreams::BOOST_IOSTREAMS_RESTRICT, and object 
 * generator for boost::iostreams::restriction.
 *
 * This design allows <boost/iostreams/restrict.hpp> and 
 * <boost/iostreams/slice.hpp> to share an implementation.
 */

#if !defined(BOOST_IOSTREAMS_RESTRICT_IMPL_HPP_INCLUDED) && \
    !defined(BOOST_IOSTREAMS_RESTRICT)
# define BOOST_IOSTREAMS_RESTRICT_IMPL_HPP_INCLUDED
                    
//------------------Implementation of restriction-----------------------------//

# include <algorithm>          // min.
# include <utility>            // pair.
# include <boost/cstdint.hpp>  // intmax_t.
# include <boost/config.hpp>   // DEDUCED_TYPENAME.
# include <boost/iostreams/categories.hpp>
# include <boost/iostreams/char_traits.hpp>
# include <boost/iostreams/detail/adapter/device_adapter.hpp>
# include <boost/iostreams/detail/adapter/filter_adapter.hpp>
# include <boost/iostreams/detail/call_traits.hpp>
# include <boost/iostreams/detail/enable_if_stream.hpp>
# include <boost/iostreams/detail/error.hpp>
# include <boost/iostreams/detail/ios.hpp>     // failure.
# include <boost/iostreams/detail/select.hpp>
# include <boost/iostreams/operations.hpp>
# include <boost/iostreams/skip.hpp>
# include <boost/iostreams/traits.hpp>         // mode_of, is_direct.
# include <boost/mpl/bool.hpp>
# include <boost/static_assert.hpp>
# include <boost/throw_exception.hpp>
# include <boost/type_traits/is_convertible.hpp>

# include <boost/iostreams/detail/config/disable_warnings.hpp>

namespace boost { namespace iostreams {

namespace detail {

//
// Template name: restricted_indirect_device.
// Description: Provides an restricted view of an indirect Device.
// Template parameters:
//      Device - An indirect model of Device that models either Source or
//          SeekableDevice.
//
template<typename Device>
class restricted_indirect_device : public device_adapter<Device> {
private:
    typedef typename detail::param_type<Device>::type  param_type;
public:
    typedef typename char_type_of<Device>::type  char_type;
    typedef typename mode_of<Device>::type       mode;
    BOOST_STATIC_ASSERT(!(is_convertible<mode, detail::two_sequence>::value));
    struct category
        : mode,
          device_tag,
          closable_tag,
          flushable_tag,
          localizable_tag,
          optimally_buffered_tag
        { };
    restricted_indirect_device( param_type dev, stream_offset off,
                                stream_offset len = -1 );
    std::streamsize read(char_type* s, std::streamsize n);
    std::streamsize write(const char_type* s, std::streamsize n);
    std::streampos seek(stream_offset off, BOOST_IOS::seekdir way);
private:
    stream_offset beg_, pos_, end_;
};

//
// Template name: restricted_direct_device.
// Description: Provides an restricted view of a Direct Device.
// Template parameters:
//      Device - A model of Direct and Device.
//
template<typename Device>
class restricted_direct_device : public device_adapter<Device> {
public:
    typedef typename char_type_of<Device>::type  char_type;
    typedef std::pair<char_type*, char_type*>    pair_type;
    typedef typename mode_of<Device>::type       mode;
    BOOST_STATIC_ASSERT(!(is_convertible<mode, detail::two_sequence>::value));
    struct category
        : mode_of<Device>::type,
          device_tag,
          direct_tag,
          closable_tag,
          localizable_tag
        { };
    restricted_direct_device( const Device& dev, stream_offset off,
                              stream_offset len = -1 );
    pair_type input_sequence();
    pair_type output_sequence();
private:
    pair_type sequence(mpl::true_);
    pair_type sequence(mpl::false_);
    char_type *beg_, *end_;
};

//
// Template name: restricted_filter.
// Description: Provides an restricted view of a Filter.
// Template parameters:
//      Filter - An indirect model of Filter.
//
template<typename Filter>
class restricted_filter : public filter_adapter<Filter> {
public:
    typedef typename char_type_of<Filter>::type char_type;
    typedef typename mode_of<Filter>::type      mode;
    BOOST_STATIC_ASSERT(!(is_convertible<mode, detail::two_sequence>::value));
    struct category
        : mode,
          filter_tag,
          multichar_tag,
          closable_tag,
          localizable_tag,
          optimally_buffered_tag
        { };
    restricted_filter( const Filter& flt, stream_offset off, 
                       stream_offset len = -1 );

    template<typename Source>
    std::streamsize read(Source& src, char_type* s, std::streamsize n)
    {
        using namespace std;
        if (!open_)
            open(src, BOOST_IOS::in);
        std::streamsize amt =
            end_ != -1 ?
                (std::min) (n, static_cast<std::streamsize>(end_ - pos_)) :
                n;
        std::streamsize result = 
            iostreams::read(this->component(), src, s, amt);
        if (result != -1)
            pos_ += result;
        return result;
    }

    template<typename Sink>
    std::streamsize write(Sink& snk, const char_type* s, std::streamsize n)
    {
        if (!open_)
            open(snk, BOOST_IOS::out);
        if (end_ != -1 && pos_ + n >= end_) {
            if(pos_ < end_)
                pos_ += iostreams::write(this->component(),
                    snk, s, end_ - pos_);
            boost::throw_exception(bad_write());
        }
        std::streamsize result = 
            iostreams::write(this->component(), snk, s, n);
        pos_ += result;
        return result;
    }

    template<typename Device>
    std::streampos seek(Device& dev, stream_offset off, BOOST_IOS::seekdir way)
    {
        stream_offset next;
        if (way == BOOST_IOS::beg) {
            next = beg_ + off;
        } else if (way == BOOST_IOS::cur) {
            next = pos_ + off;
        } else if (end_ != -1) {
            next = end_ + off;
        } else {
            // Restriction is half-open; seek relative to the actual end.
            pos_ = this->component().seek(dev, off, BOOST_IOS::end);
            if (pos_ < beg_)
                boost::throw_exception(bad_seek());
            return offset_to_position(pos_ - beg_);
        }
        if (next < beg_ || (end_ != -1 && next >= end_))
            boost::throw_exception(bad_seek());
        pos_ = this->component().seek(dev, next, BOOST_IOS::cur);
        return offset_to_position(pos_ - beg_);
    }

    template<typename Device>
    void close(Device& dev) 
    { 
        open_ = false;
        detail::close_all(this->component(), dev); 
    }

    template<typename Device>
    void close(Device& dev, BOOST_IOS::openmode which) 
    { 
        open_ = false;
        iostreams::close(this->component(), dev, which); 
    }
private:
    template<typename Device>
    void open(Device& dev, BOOST_IOS::openmode which)
    {
        typedef typename is_convertible<mode, dual_use>::type is_dual_use;
        open_ = true;
        which = is_dual_use() ? which : (BOOST_IOS::in | BOOST_IOS::out);
        iostreams::skip(this->component(), dev, beg_, which);
    }

    stream_offset  beg_, pos_, end_;
    bool           open_;
};

template<typename T>
struct restriction_traits
    : iostreams::select<  // Disambiguation for Tru64.
          is_filter<T>,  restricted_filter<T>,
          is_direct<T>,  restricted_direct_device<T>,
          else_,         restricted_indirect_device<T>
      >
    { };

} // End namespace detail.

template<typename T>
struct restriction : public detail::restriction_traits<T>::type {
    typedef typename detail::param_type<T>::type          param_type;
    typedef typename detail::restriction_traits<T>::type  base_type;
    restriction(param_type t, stream_offset off, stream_offset len = -1)
        : base_type(t, off, len)
        { }
};

namespace detail {

//--------------Implementation of restricted_indirect_device------------------//

template<typename Device>
restricted_indirect_device<Device>::restricted_indirect_device
    (param_type dev, stream_offset off, stream_offset len)
    : device_adapter<Device>(dev), beg_(off), pos_(off), 
      end_(len != -1 ? off + len : -1)
{
    if (len < -1 || off < 0)
        boost::throw_exception(BOOST_IOSTREAMS_FAILURE("bad offset"));
    iostreams::skip(this->component(), off);
}

template<typename Device>
inline std::streamsize restricted_indirect_device<Device>::read
    (char_type* s, std::streamsize n)
{
    using namespace std;
    std::streamsize amt =
        end_ != -1 ?
            (std::min) (n, static_cast<std::streamsize>(end_ - pos_)) :
            n;
    std::streamsize result = iostreams::read(this->component(), s, amt);
    if (result != -1)
        pos_ += result;
    return result;
}

template<typename Device>
inline std::streamsize restricted_indirect_device<Device>::write
    (const char_type* s, std::streamsize n)
{
    if (end_ != -1 && pos_ + n >= end_) {
        if(pos_ < end_)
            pos_ += iostreams::write(this->component(), s, end_ - pos_);
        boost::throw_exception(bad_write());
    }
    std::streamsize result = iostreams::write(this->component(), s, n);
    pos_ += result;
    return result;
}

template<typename Device>
std::streampos restricted_indirect_device<Device>::seek
    (stream_offset off, BOOST_IOS::seekdir way)
{
    stream_offset next;
    if (way == BOOST_IOS::beg) {
        next = beg_ + off;
    } else if (way == BOOST_IOS::cur) {
        next = pos_ + off;
    } else if (end_ != -1) {
        next = end_ + off;
    } else {
        // Restriction is half-open; seek relative to the actual end.
        pos_ = iostreams::seek(this->component(), off, BOOST_IOS::end);
        if (pos_ < beg_)
            boost::throw_exception(bad_seek());
        return offset_to_position(pos_ - beg_);
    }
    if (next < beg_ || (end_ != -1 && next > end_))
        boost::throw_exception(bad_seek());
    pos_ = iostreams::seek(this->component(), next - pos_, BOOST_IOS::cur);
    return offset_to_position(pos_ - beg_);
}

//--------------Implementation of restricted_direct_device--------------------//

template<typename Device>
restricted_direct_device<Device>::restricted_direct_device
    (const Device& dev, stream_offset off, stream_offset len)
    : device_adapter<Device>(dev), beg_(0), end_(0)
{
    std::pair<char_type*, char_type*> seq =
        sequence(is_convertible<category, input>());
    if ( off < 0 || len < -1 || 
         (len != -1 && off + len > seq.second - seq.first) )
    {
        boost::throw_exception(BOOST_IOSTREAMS_FAILURE("bad offset"));
    }
    beg_ = seq.first + off;
    end_ = len != -1 ? 
        seq.first + off + len :
        seq.second;
}

template<typename Device>
typename restricted_direct_device<Device>::pair_type
restricted_direct_device<Device>::input_sequence()
{
    BOOST_STATIC_ASSERT((is_convertible<category, input>::value));
    return std::make_pair(beg_, end_);
}

template<typename Device>
typename restricted_direct_device<Device>::pair_type
restricted_direct_device<Device>::output_sequence()
{
    BOOST_STATIC_ASSERT((is_convertible<category, output>::value));
    return std::make_pair(beg_, end_);
}

template<typename Device>
typename restricted_direct_device<Device>::pair_type
restricted_direct_device<Device>::sequence(mpl::true_)
{ return iostreams::input_sequence(this->component()); }

template<typename Device>
typename restricted_direct_device<Device>::pair_type
restricted_direct_device<Device>::sequence(mpl::false_)
{ return iostreams::output_sequence(this->component()); }

//--------------Implementation of restricted_filter---------------------------//

template<typename Filter>
restricted_filter<Filter>::restricted_filter
    (const Filter& flt, stream_offset off, stream_offset len)
    : filter_adapter<Filter>(flt), beg_(off),
      pos_(off), end_(len != -1 ? off + len : -1), open_(false)
{
    if (len < -1 || off < 0)
        boost::throw_exception(BOOST_IOSTREAMS_FAILURE("bad offset"));
}

} // End namespace detail.

} } // End namespaces iostreams, boost.

#elif defined(BOOST_IOSTREAMS_RESTRICT)

namespace boost { namespace iostreams {

//--------------Implementation of restrict/slice------------------------------//

// Note: The following workarounds are patterned after resolve.hpp. It has not
// yet been confirmed that they are necessary.

# ifndef BOOST_IOSTREAMS_BROKEN_OVERLOAD_RESOLUTION //------------------------//
#  ifndef BOOST_IOSTREAMS_NO_STREAM_TEMPLATES //------------------------------//

template<typename T>
restriction<T> 
BOOST_IOSTREAMS_RESTRICT( const T& t, stream_offset off, stream_offset len = -1
                          BOOST_IOSTREAMS_DISABLE_IF_STREAM(T) )
{ return restriction<T>(t, off, len); }

template<typename Ch, typename Tr>
restriction< std::basic_streambuf<Ch, Tr> >
BOOST_IOSTREAMS_RESTRICT( std::basic_streambuf<Ch, Tr>& sb, stream_offset off, 
                          stream_offset len = -1 )
{ return restriction< std::basic_streambuf<Ch, Tr> >(sb, off, len); }

template<typename Ch, typename Tr>
restriction< std::basic_istream<Ch, Tr> >
BOOST_IOSTREAMS_RESTRICT
    (std::basic_istream<Ch, Tr>& is, stream_offset off, stream_offset len = -1)
{ return restriction< std::basic_istream<Ch, Tr> >(is, off, len); }

template<typename Ch, typename Tr>
restriction< std::basic_ostream<Ch, Tr> >
BOOST_IOSTREAMS_RESTRICT
    (std::basic_ostream<Ch, Tr>& os, stream_offset off, stream_offset len = -1)
{ return restriction< std::basic_ostream<Ch, Tr> >(os, off, len); }

template<typename Ch, typename Tr>
restriction< std::basic_iostream<Ch, Tr> >
BOOST_IOSTREAMS_RESTRICT
    (std::basic_iostream<Ch, Tr>& io, stream_offset off, stream_offset len = -1)
{ return restriction< std::basic_iostream<Ch, Tr> >(io, off, len); }

#  else // # ifndef BOOST_IOSTREAMS_NO_STREAM_TEMPLATES //--------------------//

template<typename T>
restriction<T> 
BOOST_IOSTREAMS_RESTRICT( const T& t, stream_offset off, stream_offset len = -1
                          BOOST_IOSTREAMS_DISABLE_IF_STREAM(T) )
{ return restriction<T>(t, off, len); }

restriction<std::streambuf> 
BOOST_IOSTREAMS_RESTRICT
    (std::streambuf& sb, stream_offset off, stream_offset len = -1)
{ return restriction<std::streambuf>(sb, off, len); }

restriction<std::istream> 
BOOST_IOSTREAMS_RESTRICT
    (std::istream<Ch, Tr>& is, stream_offset off, stream_offset len = -1)
{ return restriction<std::istream>(is, off, len); }

restriction<std::ostream> 
BOOST_IOSTREAMS_RESTRICT
    (std::ostream<Ch, Tr>& os, stream_offset off, stream_offset len = -1)
{ return restriction<std::ostream>(os, off, len); }

restriction<std::iostream> 
BOOST_IOSTREAMS_RESTRICT
    (std::iostream<Ch, Tr>& io, stream_offset off, stream_offset len = -1)
{ return restriction<std::iostream>(io, off, len); }

#  endif // # ifndef BOOST_IOSTREAMS_NO_STREAM_TEMPLATES //-------------------//
# else // #ifndef BOOST_IOSTREAMS_BROKEN_OVERLOAD_RESOLUTION //---------------//

template<typename T>
restriction<T> 
BOOST_IOSTREAMS_RESTRICT
    (const T& t, stream_offset off, stream_offset len, mpl::true_)
{   // Bad overload resolution.
    return restriction<T>(const_cast<T&>(t, off, len));
}

template<typename T>
restriction<T> 
BOOST_IOSTREAMS_RESTRICT
    (const T& t, stream_offset off, stream_offset len, mpl::false_)
{ return restriction<T>(t, off, len); }

template<typename T>
restriction<T> 
BOOST_IOSTREAMS_RESTRICT( const T& t, stream_offset off, stream_offset len = -1
                          BOOST_IOSTREAMS_DISABLE_IF_STREAM(T) )
{ return BOOST_IOSTREAMS_RESTRICT(t, off, len, is_std_io<T>()); }

# if !BOOST_WORKAROUND(BOOST_BORLANDC, < 0x600) && \
     !defined(__GNUC__) // ---------------------------------------------------//

template<typename T>
restriction<T>
BOOST_IOSTREAMS_RESTRICT(T& t, stream_offset off, stream_offset len = -1)
{ return restriction<T>(t, off, len); }

#  endif // Borland 5.x or GCC //-------------------------------//
# endif // #ifndef BOOST_IOSTREAMS_BROKEN_OVERLOAD_RESOLUTION //--------------//

} } // End namespaces iostreams, boost.

# include <boost/iostreams/detail/config/enable_warnings.hpp>

#endif // #if !defined(BOOST_IOSTREAMS_RESTRICT_IMPL_HPP_INCLUDED) ...

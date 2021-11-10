/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// basic_text_oprimitive.ipp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <cstddef> // NULL
#include <algorithm> // std::copy
#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::size_t; 
} // namespace std
#endif

#include <boost/core/uncaught_exceptions.hpp>

#include <boost/archive/basic_text_oprimitive.hpp>

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>

namespace boost {
namespace archive {

// translate to base64 and copy in to buffer.
template<class OStream>
BOOST_ARCHIVE_OR_WARCHIVE_DECL void
basic_text_oprimitive<OStream>::save_binary(
    const void *address, 
    std::size_t count
){
    typedef typename OStream::char_type CharType;
    
    if(0 == count)
        return;
    
    if(os.fail())
        boost::serialization::throw_exception(
            archive_exception(archive_exception::output_stream_error)
        );
        
    os.put('\n');
    
    typedef 
        boost::archive::iterators::insert_linebreaks<
            boost::archive::iterators::base64_from_binary<
                boost::archive::iterators::transform_width<
                    const char *,
                    6,
                    8
                >
            > 
            ,76
            ,const char // cwpro8 needs this
        > 
        base64_text;

    boost::archive::iterators::ostream_iterator<CharType> oi(os);
    std::copy(
        base64_text(static_cast<const char *>(address)),
        base64_text(
            static_cast<const char *>(address) + count
        ),
        oi
    );
    
    std::size_t tail = count % 3;
    if(tail > 0){
        *oi++ = '=';
        if(tail < 2)
            *oi = '=';
    }
}

template<class OStream>
BOOST_ARCHIVE_OR_WARCHIVE_DECL
basic_text_oprimitive<OStream>::basic_text_oprimitive(
    OStream & os_,
    bool no_codecvt
) : 
    os(os_),
    flags_saver(os_),
#ifndef BOOST_NO_STD_LOCALE
    precision_saver(os_),
    codecvt_null_facet(1),
    archive_locale(os.getloc(), & codecvt_null_facet),
    locale_saver(os)
{
    if(! no_codecvt){
        os_.flush();
        os_.imbue(archive_locale);
    }
    os_ << std::noboolalpha;
}
#else
    precision_saver(os_)
{}
#endif


template<class OStream>
BOOST_ARCHIVE_OR_WARCHIVE_DECL
basic_text_oprimitive<OStream>::~basic_text_oprimitive(){
    if(boost::core::uncaught_exceptions() > 0)
        return;
    os << std::endl;
}

} //namespace boost 
} //namespace archive 

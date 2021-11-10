// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

// Thanks to Gareth Sylvester-Bradley for the Dinkumware versions of the
// positioning functions.

#ifndef BOOST_IOSTREAMS_POSITIONING_HPP_INCLUDED
#define BOOST_IOSTREAMS_POSITIONING_HPP_INCLUDED

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/integer_traits.hpp>
#include <boost/iostreams/detail/config/codecvt.hpp> // mbstate_t.
#include <boost/iostreams/detail/config/fpos.hpp>
#include <boost/iostreams/detail/ios.hpp> // streamoff, streampos.

// Must come last.
#include <boost/iostreams/detail/config/disable_warnings.hpp> 

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std { using ::fpos_t; }
#endif

namespace boost { namespace iostreams {
                    
//------------------Definition of stream_offset-------------------------------//

typedef boost::intmax_t stream_offset;

//------------------Definition of stream_offset_to_streamoff------------------//

inline std::streamoff stream_offset_to_streamoff(stream_offset off)
{ return static_cast<stream_offset>(off); }

//------------------Definition of offset_to_position--------------------------//

# ifndef BOOST_IOSTREAMS_HAS_DINKUMWARE_FPOS

inline std::streampos offset_to_position(stream_offset off) { return off; }

# else // # ifndef BOOST_IOSTREAMS_HAS_DINKUMWARE_FPOS

inline std::streampos offset_to_position(stream_offset off)
{ return std::streampos(std::mbstate_t(), off); }

# endif // # ifndef BOOST_IOSTREAMS_HAS_DINKUMWARE_FPOS

//------------------Definition of position_to_offset--------------------------//

// Hande custom pos_type's
template<typename PosType> 
inline stream_offset position_to_offset(PosType pos)
{ return std::streamoff(pos); }

# ifndef BOOST_IOSTREAMS_HAS_DINKUMWARE_FPOS

inline stream_offset position_to_offset(std::streampos pos) { return pos; }

# else // # ifndef BOOST_IOSTREAMS_HAS_DINKUMWARE_FPOS

// In the Dinkumware standard library, a std::streampos consists of two stream
// offsets -- _Fpos, of type std::fpos_t, and _Myoff, of type std::streamoff --
// together with a conversion state. A std::streampos is converted to a 
// boost::iostreams::stream_offset by extracting the two stream offsets and
// summing them. The value of _Fpos can be extracted using the implementation-
// defined member functions seekpos() or get_fpos_t(), depending on the 
// Dinkumware version. The value of _Myoff cannot be extracted directly, but can
// be calculated as the difference between the result of converting the 
// std::fpos to a std::streamoff and the result of converting the member _Fpos
// to a long. The latter operation is accomplished with the macro BOOST_IOSTREAMS_FPOSOFF,
// which works correctly on platforms where std::fpos_t is an integral type and 
// platforms where it is a struct

// Converts a std::fpos_t to a stream_offset
inline stream_offset fpos_t_to_offset(std::fpos_t pos)
{
#  if defined(_POSIX_) || (_INTEGRAL_MAX_BITS >= 64) || defined(__IBMCPP__)
    return pos;
#  else
    return BOOST_IOSTREAMS_FPOSOFF(pos);
#  endif
}

// Extracts the member _Fpos from a std::fpos
inline std::fpos_t streampos_to_fpos_t(std::streampos pos)
{
#  if defined (_CPPLIB_VER) || defined(__IBMCPP__)
    return pos.seekpos();
#  else
    return pos.get_fpos_t();
#  endif
}

inline stream_offset position_to_offset(std::streampos pos)
{
    return fpos_t_to_offset(streampos_to_fpos_t(pos)) +
        static_cast<stream_offset>(
            static_cast<std::streamoff>(pos) -
            BOOST_IOSTREAMS_FPOSOFF(streampos_to_fpos_t(pos))
        );
}

# endif // # ifndef BOOST_IOSTREAMS_HAS_DINKUMWARE_FPOS 

} } // End namespaces iostreams, boost.

#include <boost/iostreams/detail/config/enable_warnings.hpp> 

#endif // #ifndef BOOST_IOSTREAMS_POSITIONING_HPP_INCLUDED

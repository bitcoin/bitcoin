//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//
// This is a derivative work based on Zlib, copyright below:
/*
    Copyright (C) 1995-2013 Jean-loup Gailly and Mark Adler

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.

    Jean-loup Gailly        Mark Adler
    jloup@gzip.org          madler@alumni.caltech.edu

    The data format used by the zlib library is described by RFCs (Request for
    Comments) 1950 to 1952 in the files http://tools.ietf.org/html/rfc1950
    (zlib format), rfc1951 (deflate format) and rfc1952 (gzip format).
*/

#ifndef BOOST_BEAST_ZLIB_DETAIL_INFLATE_STREAM_HPP
#define BOOST_BEAST_ZLIB_DETAIL_INFLATE_STREAM_HPP

#include <boost/beast/zlib/error.hpp>
#include <boost/beast/zlib/zlib.hpp>
#include <boost/beast/zlib/detail/bitstream.hpp>
#include <boost/beast/zlib/detail/ranges.hpp>
#include <boost/beast/zlib/detail/window.hpp>
#if 0
#include <boost/beast/core/detail/type_traits.hpp>
#include <boost/throw_exception.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#endif

namespace boost {
namespace beast {
namespace zlib {
namespace detail {

class inflate_stream
{
protected:
    inflate_stream()
    {
        w_.reset(15);
    }

    BOOST_BEAST_DECL
    void
    doClear();

    BOOST_BEAST_DECL
    void
    doReset(int windowBits);

    BOOST_BEAST_DECL
    void
    doWrite(z_params& zs, Flush flush, error_code& ec);

    void
    doReset()
    {
        doReset(w_.bits());
    }

private:
    enum Mode
    {
        HEAD,       // i: waiting for magic header
        FLAGS,      // i: waiting for method and flags (gzip)
        TIME,       // i: waiting for modification time (gzip)
        OS,         // i: waiting for extra flags and operating system (gzip)
        EXLEN,      // i: waiting for extra length (gzip)
        EXTRA,      // i: waiting for extra bytes (gzip)
        NAME,       // i: waiting for end of file name (gzip)
        COMMENT,    // i: waiting for end of comment (gzip)
        HCRC,       // i: waiting for header crc (gzip)
        TYPE,       // i: waiting for type bits, including last-flag bit
        TYPEDO,     // i: same, but skip check to exit inflate on new block
        STORED,     // i: waiting for stored size (length and complement)
        COPY_,      // i/o: same as COPY below, but only first time in
        COPY,       // i/o: waiting for input or output to copy stored block
        TABLE,      // i: waiting for dynamic block table lengths
        LENLENS,    // i: waiting for code length code lengths
        CODELENS,   // i: waiting for length/lit and distance code lengths
            LEN_,   // i: same as LEN below, but only first time in
            LEN,    // i: waiting for length/lit/eob code
            LENEXT, // i: waiting for length extra bits
            DIST,   // i: waiting for distance code
            DISTEXT,// i: waiting for distance extra bits
            MATCH,  // o: waiting for output space to copy string
            LIT,    // o: waiting for output space to write literal
        CHECK,      // i: waiting for 32-bit check value
        LENGTH,     // i: waiting for 32-bit length (gzip)
        DONE,       // finished check, done -- remain here until reset
        BAD,        // got a data error -- remain here until reset
        SYNC        // looking for synchronization bytes to restart inflate()
    };

    /*  Structure for decoding tables.  Each entry provides either the
        information needed to do the operation requested by the code that
        indexed that table entry, or it provides a pointer to another
        table that indexes more bits of the code.  op indicates whether
        the entry is a pointer to another table, a literal, a length or
        distance, an end-of-block, or an invalid code.  For a table
        pointer, the low four bits of op is the number of index bits of
        that table.  For a length or distance, the low four bits of op
        is the number of extra bits to get after the code.  bits is
        the number of bits in this code or part of the code to drop off
        of the bit buffer.  val is the actual byte to output in the case
        of a literal, the base length or distance, or the offset from
        the current table to the next table.  Each entry is four bytes.

        op values as set by inflate_table():

        00000000 - literal
        0000tttt - table link, tttt != 0 is the number of table index bits
        0001eeee - length or distance, eeee is the number of extra bits
        01100000 - end of block
        01000000 - invalid code
    */
    struct code
    {
        std::uint8_t  op;   // operation, extra bits, table bits
        std::uint8_t  bits; // bits in this part of the code
        std::uint16_t val;  // offset in table or code value
    };

    /*  Maximum size of the dynamic table.  The maximum number of code
        structures is 1444, which is the sum of 852 for literal/length codes
        and 592 for distance codes.  These values were found by exhaustive
        searches using the program examples/enough.c found in the zlib
        distribtution.  The arguments to that program are the number of
        symbols, the initial root table size, and the maximum bit length
        of a code.  "enough 286 9 15" for literal/length codes returns
        returns 852, and "enough 30 6 15" for distance codes returns 592.
        The initial root table size (9 or 6) is found in the fifth argument
        of the inflate_table() calls in inflate.c and infback.c.  If the
        root table size is changed, then these maximum sizes would be need
        to be recalculated and updated.
    */
    static std::uint16_t constexpr kEnoughLens = 852;
    static std::uint16_t constexpr kEnoughDists = 592;
    static std::uint16_t constexpr kEnough = kEnoughLens + kEnoughDists;

    struct codes
    {
        code const* lencode;
        code const* distcode;
        unsigned lenbits; // VFALCO use std::uint8_t
        unsigned distbits;
    };

    // Type of code to build for inflate_table()
    enum class build
    {
        codes,
        lens,
        dists
    };

    BOOST_BEAST_DECL
    static
    void
    inflate_table(
        build type,
        std::uint16_t* lens,
        std::size_t codes,
        code** table,
        unsigned *bits,
        std::uint16_t* work,
        error_code& ec);

    BOOST_BEAST_DECL
    static
    codes const&
    get_fixed_tables();

    BOOST_BEAST_DECL
    void
    fixedTables();

    BOOST_BEAST_DECL
    void
    inflate_fast(ranges& r, error_code& ec);

    bitstream bi_;

    Mode mode_ = HEAD;              // current inflate mode
    int last_ = 0;                  // true if processing last block
    unsigned dmax_ = 32768U;        // zlib header max distance (INFLATE_STRICT)

    // sliding window
    window w_;

    // for string and stored block copying
    unsigned length_;               // literal or length of data to copy
    unsigned offset_;               // distance back to copy string from

    // for table and code decoding
    unsigned extra_;                // extra bits needed

    // dynamic table building
    unsigned ncode_;                // number of code length code lengths
    unsigned nlen_;                 // number of length code lengths
    unsigned ndist_;                // number of distance code lengths
    unsigned have_;                 // number of code lengths in lens[]
    unsigned short lens_[320];      // temporary storage for code lengths
    unsigned short work_[288];      // work area for code table building
    code codes_[kEnough];           // space for code tables
    code *next_ = codes_;           // next available space in codes[]
    int back_ = -1;                 // bits back of last unprocessed length/lit
    unsigned was_;                  // initial length of match

    // fixed and dynamic code tables
    code const* lencode_ = codes_   ; // starting table for length/literal codes
    code const* distcode_ = codes_; // starting table for distance codes
    unsigned lenbits_;              // index bits for lencode
    unsigned distbits_;             // index bits for distcode
};

} // detail
} // zlib
} // beast
} // boost

#ifdef BOOST_BEAST_HEADER_ONLY
#include <boost/beast/zlib/detail/inflate_stream.ipp>
#endif

#endif

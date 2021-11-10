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

#ifndef BOOST_BEAST_ZLIB_DETAIL_INFLATE_STREAM_IPP
#define BOOST_BEAST_ZLIB_DETAIL_INFLATE_STREAM_IPP

#include <boost/beast/zlib/detail/inflate_stream.hpp>
#include <boost/throw_exception.hpp>
#include <array>

namespace boost {
namespace beast {
namespace zlib {
namespace detail {

void
inflate_stream::
doClear()
{
}

void
inflate_stream::
doReset(int windowBits)
{
    if(windowBits < 8 || windowBits > 15)
        BOOST_THROW_EXCEPTION(std::domain_error{
            "windowBits out of range"});
    w_.reset(windowBits);

    bi_.flush();
    mode_ = HEAD;
    last_ = 0;
    dmax_ = 32768U;
    lencode_ = codes_;
    distcode_ = codes_;
    next_ = codes_;
    back_ = -1;
}

void
inflate_stream::
doWrite(z_params& zs, Flush flush, error_code& ec)
{
    ranges r;
    r.in.first = static_cast<
        std::uint8_t const*>(zs.next_in);
    r.in.last = r.in.first + zs.avail_in;
    r.in.next = r.in.first;
    r.out.first = static_cast<
        std::uint8_t*>(zs.next_out);
    r.out.last = r.out.first + zs.avail_out;
    r.out.next = r.out.first;

    auto const done =
        [&]
        {
            /*
               Return from inflate(), updating the total counts and the check value.
               If there was no progress during the inflate() call, return a buffer
               error.  Call updatewindow() to create and/or update the window state.
               Note: a memory error from inflate() is non-recoverable.
             */


            // VFALCO TODO Don't allocate update the window unless necessary
            if(/*wsize_ ||*/ (r.out.used() && mode_ < BAD &&
                    (mode_ < CHECK || flush != Flush::finish)))
                w_.write(r.out.first, r.out.used());

            zs.next_in = r.in.next;
            zs.avail_in = r.in.avail();
            zs.next_out = r.out.next;
            zs.avail_out = r.out.avail();
            zs.total_in += r.in.used();
            zs.total_out += r.out.used();
            zs.data_type = bi_.size() + (last_ ? 64 : 0) +
                (mode_ == TYPE ? 128 : 0) +
                (mode_ == LEN_ || mode_ == COPY_ ? 256 : 0);

            if(((! r.in.used() && ! r.out.used()) ||
                    flush == Flush::finish) && ! ec)
                ec = error::need_buffers;
        };
    auto const err =
        [&](error e)
        {
            ec = e;
            mode_ = BAD;
        };

    if(mode_ == TYPE)
        mode_ = TYPEDO;

    for(;;)
    {
        switch(mode_)
        {
        case HEAD:
            mode_ = TYPEDO;
            break;

        case TYPE:
            if(flush == Flush::block || flush == Flush::trees)
                return done();
            // fall through

        case TYPEDO:
        {
            if(last_)
            {
                bi_.flush_byte();
                mode_ = CHECK;
                break;
            }
            if(! bi_.fill(3, r.in.next, r.in.last))
                return done();
            std::uint8_t v;
            bi_.read(v, 1);
            last_ = v != 0;
            bi_.read(v, 2);
            switch(v)
            {
            case 0:
                // uncompressed block
                mode_ = STORED;
                break;
            case 1:
                // fixed Huffman table
                fixedTables();
                mode_ = LEN_;             /* decode codes */
                if(flush == Flush::trees)
                    return done();
                break;
            case 2:
                // dynamic Huffman table
                mode_ = TABLE;
                break;

            default:
                return err(error::invalid_block_type);
            }
            break;
        }

        case STORED:
        {
            bi_.flush_byte();
            std::uint32_t v;
            if(! bi_.fill(32, r.in.next, r.in.last))
                return done();
            bi_.peek(v, 32);
            length_ = v & 0xffff;
            if(length_ != ((v >> 16) ^ 0xffff))
                return err(error::invalid_stored_length);
            // flush instead of read, otherwise
            // undefined right shift behavior.
            bi_.flush();
            mode_ = COPY_;
            if(flush == Flush::trees)
                return done();
            BOOST_FALLTHROUGH;
        }

        case COPY_:
            mode_ = COPY;
            BOOST_FALLTHROUGH;

        case COPY:
        {
            auto copy = length_;
            if(copy == 0)
            {
                mode_ = TYPE;
                break;
            }
            copy = clamp(copy, r.in.avail());
            copy = clamp(copy, r.out.avail());
            if(copy == 0)
                return done();
            std::memcpy(r.out.next, r.in.next, copy);
            r.in.next += copy;
            r.out.next += copy;
            length_ -= copy;
            break;
        }

        case TABLE:
            if(! bi_.fill(5 + 5 + 4, r.in.next, r.in.last))
                return done();
            bi_.read(nlen_, 5);
            nlen_ += 257;
            bi_.read(ndist_, 5);
            ndist_ += 1;
            bi_.read(ncode_, 4);
            ncode_ += 4;
            if(nlen_ > 286 || ndist_ > 30)
                return err(error::too_many_symbols);
            have_ = 0;
            mode_ = LENLENS;
            BOOST_FALLTHROUGH;

        case LENLENS:
        {
            static std::array<std::uint8_t, 19> constexpr order = {{
                16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15}};
            while(have_ < ncode_)
            {
                if(! bi_.fill(3, r.in.next, r.in.last))
                    return done();
                bi_.read(lens_[order[have_]], 3);
                ++have_;
            }
            while(have_ < order.size())
                lens_[order[have_++]] = 0;

            next_ = &codes_[0];
            lencode_ = next_;
            lenbits_ = 7;
            inflate_table(build::codes, &lens_[0],
                order.size(), &next_, &lenbits_, work_, ec);
            if(ec)
            {
                mode_ = BAD;
                break;
            }
            have_ = 0;
            mode_ = CODELENS;
            BOOST_FALLTHROUGH;
        }

        case CODELENS:
        {
            while(have_ < nlen_ + ndist_)
            {
                std::uint16_t v;
                if(! bi_.fill(lenbits_, r.in.next, r.in.last))
                    return done();
                bi_.peek(v, lenbits_);
                auto cp = &lencode_[v];
                if(cp->val < 16)
                {
                    bi_.drop(cp->bits);
                    lens_[have_++] = cp->val;
                }
                else
                {
                    std::uint16_t len;
                    std::uint16_t copy;
                    if(cp->val == 16)
                    {
                        if(! bi_.fill(cp->bits + 2, r.in.next, r.in.last))
                            return done();
                        bi_.drop(cp->bits);
                        if(have_ == 0)
                            return err(error::invalid_bit_length_repeat);
                        bi_.read(copy, 2);
                        len = lens_[have_ - 1];
                        copy += 3;

                    }
                    else if(cp->val == 17)
                    {
                        if(! bi_.fill(cp->bits + 3, r.in.next, r.in.last))
                            return done();
                        bi_.drop(cp->bits);
                        bi_.read(copy, 3);
                        len = 0;
                        copy += 3;
                    }
                    else
                    {
                        if(! bi_.fill(cp->bits + 7, r.in.next, r.in.last))
                            return done();
                        bi_.drop(cp->bits);
                        bi_.read(copy, 7);
                        len = 0;
                        copy += 11;
                    }
                    if(have_ + copy > nlen_ + ndist_)
                        return err(error::invalid_bit_length_repeat);
                    std::fill(&lens_[have_], &lens_[have_ + copy], len);
                    have_ += copy;
                    copy = 0;
                }
            }
            // handle error breaks in while
            if(mode_ == BAD)
                break;
            // check for end-of-block code (better have one)
            if(lens_[256] == 0)
                return err(error::missing_eob);
            /* build code tables -- note: do not change the lenbits or distbits
               values here (9 and 6) without reading the comments in inftrees.hpp
               concerning the kEnough constants, which depend on those values */
            next_ = &codes_[0];
            lencode_ = next_;
            lenbits_ = 9;
            inflate_table(build::lens, &lens_[0],
                nlen_, &next_, &lenbits_, work_, ec);
            if(ec)
            {
                mode_ = BAD;
                return;
            }
            distcode_ = next_;
            distbits_ = 6;
            inflate_table(build::dists, lens_ + nlen_,
                ndist_, &next_, &distbits_, work_, ec);
            if(ec)
            {
                mode_ = BAD;
                return;
            }
            mode_ = LEN_;
            if(flush == Flush::trees)
                return done();
            BOOST_FALLTHROUGH;
        }

        case LEN_:
            mode_ = LEN;
            BOOST_FALLTHROUGH;

        case LEN:
        {
            if(r.in.avail() >= 6 && r.out.avail() >= 258)
            {
                inflate_fast(r, ec);
                if(ec)
                {
                    mode_ = BAD;
                    return;
                }
                if(mode_ == TYPE)
                    back_ = -1;
                break;
            }
            if(! bi_.fill(lenbits_, r.in.next, r.in.last))
                return done();
            std::uint16_t v;
            back_ = 0;
            bi_.peek(v, lenbits_);
            auto cp = &lencode_[v];
            if(cp->op && (cp->op & 0xf0) == 0)
            {
                auto prev = cp;
                if(! bi_.fill(prev->bits + prev->op, r.in.next, r.in.last))
                    return done();
                bi_.peek(v, prev->bits + prev->op);
                cp = &lencode_[prev->val + (v >> prev->bits)];
                bi_.drop(prev->bits + cp->bits);
                back_ += prev->bits + cp->bits;
            }
            else
            {
                bi_.drop(cp->bits);
                back_ += cp->bits;
            }
            length_ = cp->val;
            if(cp->op == 0)
            {
                mode_ = LIT;
                break;
            }
            if(cp->op & 32)
            {
                back_ = -1;
                mode_ = TYPE;
                break;
            }
            if(cp->op & 64)
                return err(error::invalid_literal_length);
            extra_ = cp->op & 15;
            mode_ = LENEXT;
            BOOST_FALLTHROUGH;
        }

        case LENEXT:
            if(extra_)
            {
                if(! bi_.fill(extra_, r.in.next, r.in.last))
                    return done();
                std::uint16_t v;
                bi_.read(v, extra_);
                length_ += v;
                back_ += extra_;
            }
            was_ = length_;
            mode_ = DIST;
            BOOST_FALLTHROUGH;

        case DIST:
        {
            if(! bi_.fill(distbits_, r.in.next, r.in.last))
                return done();
            std::uint16_t v;
            bi_.peek(v, distbits_);
            auto cp = &distcode_[v];
            if((cp->op & 0xf0) == 0)
            {
                auto prev = cp;
                if(! bi_.fill(prev->bits + prev->op, r.in.next, r.in.last))
                    return done();
                bi_.peek(v, prev->bits + prev->op);
                cp = &distcode_[prev->val + (v >> prev->bits)];
                bi_.drop(prev->bits + cp->bits);
                back_ += prev->bits + cp->bits;
            }
            else
            {
                bi_.drop(cp->bits);
                back_ += cp->bits;
            }
            if(cp->op & 64)
                return err(error::invalid_distance_code);
            offset_ = cp->val;
            extra_ = cp->op & 15;
            mode_ = DISTEXT;
            BOOST_FALLTHROUGH;
        }

        case DISTEXT:
            if(extra_)
            {
                std::uint16_t v;
                if(! bi_.fill(extra_, r.in.next, r.in.last))
                    return done();
                bi_.read(v, extra_);
                offset_ += v;
                back_ += extra_;
            }
#ifdef INFLATE_STRICT
            if(offset_ > dmax_)
                return err(error::invalid_distance);
#endif
            mode_ = MATCH;
            BOOST_FALLTHROUGH;

        case MATCH:
        {
            if(! r.out.avail())
                return done();
            if(offset_ > r.out.used())
            {
                // copy from window
                auto offset = static_cast<std::uint16_t>(
                    offset_ - r.out.used());
                if(offset > w_.size())
                    return err(error::invalid_distance);
                auto const n = clamp(clamp(
                    length_, offset), r.out.avail());
                w_.read(r.out.next, offset, n);
                r.out.next += n;
                length_ -= n;
            }
            else
            {
                // copy from output
                auto in = r.out.next - offset_;
                auto n = clamp(length_, r.out.avail());
                length_ -= n;
                while(n--)
                    *r.out.next++ = *in++;
            }
            if(length_ == 0)
                mode_ = LEN;
            break;
        }

        case LIT:
        {
            if(! r.out.avail())
                return done();
            auto const v = static_cast<std::uint8_t>(length_);
            *r.out.next++ = v;
            mode_ = LEN;
            break;
        }

        case CHECK:
            mode_ = DONE;
            BOOST_FALLTHROUGH;

        case DONE:
            ec = error::end_of_stream;
            return done();

        case BAD:
            return done();

        case SYNC:
        default:
            BOOST_THROW_EXCEPTION(std::logic_error{
                "stream error"});
        }
    }
}

//------------------------------------------------------------------------------

/*  Build a set of tables to decode the provided canonical Huffman code.
    The code lengths are lens[0..codes-1].  The result starts at *table,
    whose indices are 0..2^bits-1.  work is a writable array of at least
    lens shorts, which is used as a work area.  type is the type of code
    to be generated, build::codes, build::lens, or build::dists.  On
    return, zero is success, -1 is an invalid code, and +1 means that
    kEnough isn't enough.  table on return points to the next available
    entry's address.  bits is the requested root table index bits, and
    on return it is the actual root table index bits.  It will differ if
    the request is greater than the longest code or if it is less than
    the shortest code.
*/
void
inflate_stream::
inflate_table(
    build type,
    std::uint16_t* lens,
    std::size_t codes,
    code** table,
    unsigned *bits,
    std::uint16_t* work,
    error_code& ec)
{
    unsigned len;                   // a code's length in bits
    unsigned sym;                   // index of code symbols
    unsigned min, max;              // minimum and maximum code lengths
    unsigned root;                  // number of index bits for root table
    unsigned curr;                  // number of index bits for current table
    unsigned drop;                  // code bits to drop for sub-table
    int left;                       // number of prefix codes available
    unsigned used;                  // code entries in table used
    unsigned huff;                  // Huffman code
    unsigned incr;                  // for incrementing code, index
    unsigned fill;                  // index for replicating entries
    unsigned low;                   // low bits for current root entry
    unsigned mask;                  // mask for low root bits
    code here;                      // table entry for duplication
    code *next;                     // next available space in table
    std::uint16_t const* base;      // base value table to use
    std::uint16_t const* extra;     // extra bits table to use
    int end;                        // use base and extra for symbol > end
    std::uint16_t count[15+1];      // number of codes of each length
    std::uint16_t offs[15+1];       // offsets in table for each length

    // Length codes 257..285 base
    static std::uint16_t constexpr lbase[31] = {
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0};

    // Length codes 257..285 extra
    static std::uint16_t constexpr lext[31] = {
        16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 18, 18, 18, 18,
        19, 19, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 16, 72, 78};

    // Distance codes 0..29 base
    static std::uint16_t constexpr dbase[32] = {
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
        8193, 12289, 16385, 24577, 0, 0};

    // Distance codes 0..29 extra
    static std::uint16_t constexpr dext[32] = {
        16, 16, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22,
        23, 23, 24, 24, 25, 25, 26, 26, 27, 27,
        28, 28, 29, 29, 64, 64};

    /*
       Process a set of code lengths to create a canonical Huffman code.  The
       code lengths are lens[0..codes-1].  Each length corresponds to the
       symbols 0..codes-1.  The Huffman code is generated by first sorting the
       symbols by length from short to long, and retaining the symbol order
       for codes with equal lengths.  Then the code starts with all zero bits
       for the first code of the shortest length, and the codes are integer
       increments for the same length, and zeros are appended as the length
       increases.  For the deflate format, these bits are stored backwards
       from their more natural integer increment ordering, and so when the
       decoding tables are built in the large loop below, the integer codes
       are incremented backwards.

       This routine assumes, but does not check, that all of the entries in
       lens[] are in the range 0..15.  The caller must assure this.
       1..15 is interpreted as that code length.  zero means that that
       symbol does not occur in this code.

       The codes are sorted by computing a count of codes for each length,
       creating from that a table of starting indices for each length in the
       sorted table, and then entering the symbols in order in the sorted
       table.  The sorted table is work[], with that space being provided by
       the caller.

       The length counts are used for other purposes as well, i.e. finding
       the minimum and maximum length codes, determining if there are any
       codes at all, checking for a valid set of lengths, and looking ahead
       at length counts to determine sub-table sizes when building the
       decoding tables.
     */

    /* accumulate lengths for codes (assumes lens[] all in 0..15) */
    for (len = 0; len <= 15; len++)
        count[len] = 0;
    for (sym = 0; sym < codes; sym++)
        count[lens[sym]]++;

    /* bound code lengths, force root to be within code lengths */
    root = *bits;
    for (max = 15; max >= 1; max--)
        if (count[max] != 0)
            break;
    if (root > max)
        root = max;
    if (max == 0)
    {                     /* no symbols to code at all */
        here.op = (std::uint8_t)64;    /* invalid code marker */
        here.bits = (std::uint8_t)1;
        here.val = (std::uint16_t)0;
        *(*table)++ = here;             /* make a table to force an error */
        *(*table)++ = here;
        *bits = 1;
        return;       /* no symbols, but wait for decoding to report error */
    }
    for (min = 1; min < max; min++)
        if (count[min] != 0)
            break;
    if (root < min)
        root = min;

    /* check for an over-subscribed or incomplete set of lengths */
    left = 1;
    for (len = 1; len <= 15; len++)
    {
        left <<= 1;
        left -= count[len];
        if (left < 0)
        {
            ec = error::over_subscribed_length;
            return;
        }
    }
    if (left > 0 && (type == build::codes || max != 1))
    {
        ec = error::incomplete_length_set;
        return;
    }

    /* generate offsets into symbol table for each length for sorting */
    offs[1] = 0;
    for (len = 1; len < 15; len++)
        offs[len + 1] = offs[len] + count[len];

    /* sort symbols by length, by symbol order within each length */
    for (sym = 0; sym < codes; sym++)
        if (lens[sym] != 0)
            work[offs[lens[sym]]++] = (std::uint16_t)sym;

    /*
       Create and fill in decoding tables.  In this loop, the table being
       filled is at next and has curr index bits.  The code being used is huff
       with length len.  That code is converted to an index by dropping drop
       bits off of the bottom.  For codes where len is less than drop + curr,
       those top drop + curr - len bits are incremented through all values to
       fill the table with replicated entries.

       root is the number of index bits for the root table.  When len exceeds
       root, sub-tables are created pointed to by the root entry with an index
       of the low root bits of huff.  This is saved in low to check for when a
       new sub-table should be started.  drop is zero when the root table is
       being filled, and drop is root when sub-tables are being filled.

       When a new sub-table is needed, it is necessary to look ahead in the
       code lengths to determine what size sub-table is needed.  The length
       counts are used for this, and so count[] is decremented as codes are
       entered in the tables.

       used keeps track of how many table entries have been allocated from the
       provided *table space.  It is checked for build::lens and DIST tables against
       the constants kEnoughLens and kEnoughDists to guard against changes in
       the initial root table size constants.  See the comments in inftrees.hpp
       for more information.

       sym increments through all symbols, and the loop terminates when
       all codes of length max, i.e. all codes, have been processed.  This
       routine permits incomplete codes, so another loop after this one fills
       in the rest of the decoding tables with invalid code markers.
     */

    /* set up for code type */
    switch (type)
    {
    case build::codes:
        base = extra = work;    /* dummy value--not used */
        end = 19;
        break;
    case build::lens:
        base = lbase;
        base -= 257;
        extra = lext;
        extra -= 257;
        end = 256;
        break;
    default:            /* build::dists */
        base = dbase;
        extra = dext;
        end = -1;
    }

    /* initialize state for loop */
    huff = 0;                   /* starting code */
    sym = 0;                    /* starting code symbol */
    len = min;                  /* starting code length */
    next = *table;              /* current table to fill in */
    curr = root;                /* current table index bits */
    drop = 0;                   /* current bits to drop from code for index */
    low = (unsigned)(-1);       /* trigger new sub-table when len > root */
    used = 1U << root;          /* use root table entries */
    mask = used - 1;            /* mask for comparing low */

    auto const not_enough = []
    {
        BOOST_THROW_EXCEPTION(std::logic_error{
            "insufficient output size when inflating tables"});
    };

    // check available table space
    if ((type == build::lens && used > kEnoughLens) ||
            (type == build::dists && used > kEnoughDists))
        return not_enough();

    /* process all codes and make table entries */
    for (;;)
    {
        /* create table entry */
        here.bits = (std::uint8_t)(len - drop);
        if ((int)(work[sym]) < end)
        {
            here.op = (std::uint8_t)0;
            here.val = work[sym];
        }
        else if ((int)(work[sym]) > end)
        {
            here.op = (std::uint8_t)(extra[work[sym]]);
            here.val = base[work[sym]];
        }
        else
        {
            here.op = (std::uint8_t)(32 + 64);         /* end of block */
            here.val = 0;
        }

        /* replicate for those indices with low len bits equal to huff */
        incr = 1U << (len - drop);
        fill = 1U << curr;
        min = fill;                 /* save offset to next table */
        do
        {
            fill -= incr;
            next[(huff >> drop) + fill] = here;
        } while (fill != 0);

        /* backwards increment the len-bit code huff */
        incr = 1U << (len - 1);
        while (huff & incr)
            incr >>= 1;
        if (incr != 0)
        {
            huff &= incr - 1;
            huff += incr;
        }
        else
            huff = 0;

        /* go to next symbol, update count, len */
        sym++;
        if (--(count[len]) == 0)
        {
            if (len == max) break;
            len = lens[work[sym]];
        }

        /* create new sub-table if needed */
        if (len > root && (huff & mask) != low)
        {
            /* if first time, transition to sub-tables */
            if (drop == 0)
                drop = root;

            /* increment past last table */
            next += min;            /* here min is 1 << curr */

            /* determine length of next table */
            curr = len - drop;
            left = (int)(1 << curr);
            while (curr + drop < max)
            {
                left -= count[curr + drop];
                if (left <= 0) break;
                curr++;
                left <<= 1;
            }

            /* check for enough space */
            used += 1U << curr;
            if ((type == build::lens && used > kEnoughLens) ||
                    (type == build::dists && used > kEnoughDists))
                return not_enough();

            /* point entry in root table to sub-table */
            low = huff & mask;
            (*table)[low].op = (std::uint8_t)curr;
            (*table)[low].bits = (std::uint8_t)root;
            (*table)[low].val = (std::uint16_t)(next - *table);
        }
    }

    /* fill in remaining table entry if code is incomplete (guaranteed to have
       at most one remaining entry, since if the code is incomplete, the
       maximum code length that was allowed to get this far is one bit) */
    if (huff != 0)
    {
        here.op = 64;   // invalid code marker
        here.bits = (std::uint8_t)(len - drop);
        here.val = 0;
        next[huff] = here;
    }

    *table += used;
    *bits = root;
}

auto
inflate_stream::
get_fixed_tables() ->
    codes const&
{
    struct fixed_codes : codes
    {
        code len_[512];
        code dist_[32];

        fixed_codes()
        {
            lencode = len_;
            lenbits = 9;
            distcode = dist_;
            distbits = 5;

            std::uint16_t lens[320];
            std::uint16_t work[288];

            std::fill(&lens[  0], &lens[144], std::uint16_t{8});
            std::fill(&lens[144], &lens[256], std::uint16_t{9});
            std::fill(&lens[256], &lens[280], std::uint16_t{7});
            std::fill(&lens[280], &lens[288], std::uint16_t{8});

            {
                error_code ec;
                auto next = &len_[0];
                inflate_table(build::lens,
                    lens, 288, &next, &lenbits, work, ec);
                if(ec)
                    BOOST_THROW_EXCEPTION(std::logic_error{ec.message()});
            }

            // VFALCO These fixups are from ZLib
            len_[ 99].op = 64;
            len_[227].op = 64;
            len_[355].op = 64;
            len_[483].op = 64;

            {
                error_code ec;
                auto next = &dist_[0];
                std::fill(&lens[0], &lens[32], std::uint16_t{5});
                inflate_table(build::dists,
                    lens, 32, &next, &distbits, work, ec);
                if(ec)
                    BOOST_THROW_EXCEPTION(std::logic_error{ec.message()});
            }
        }
    };

    static fixed_codes const fc;
    return fc;
}

void
inflate_stream::
fixedTables()
{
    auto const fc = get_fixed_tables();
    lencode_ = fc.lencode;
    lenbits_ = fc.lenbits;
    distcode_ = fc.distcode;
    distbits_ = fc.distbits;
}

/*
   Decode literal, length, and distance codes and write out the resulting
   literal and match bytes until either not enough input or output is
   available, an end-of-block is encountered, or a data error is encountered.
   When large enough input and output buffers are supplied to inflate(), for
   example, a 16K input buffer and a 64K output buffer, more than 95% of the
   inflate execution time is spent in this routine.

   Entry assumptions:

        state->mode_ == LEN
        zs.avail_in >= 6
        zs.avail_out >= 258
        start >= zs.avail_out
        state->bits_ < 8

   On return, state->mode_ is one of:

        LEN -- ran out of enough output space or enough available input
        TYPE -- reached end of block code, inflate() to interpret next block
        BAD -- error in block data

   Notes:

    - The maximum input bits used by a length/distance pair is 15 bits for the
      length code, 5 bits for the length extra, 15 bits for the distance code,
      and 13 bits for the distance extra.  This totals 48 bits, or six bytes.
      Therefore if zs.avail_in >= 6, then there is enough input to avoid
      checking for available input while decoding.

    - The maximum bytes that a single length/distance pair can output is 258
      bytes, which is the maximum length that can be coded.  inflate_fast()
      requires zs.avail_out >= 258 for each loop to avoid checking for
      output space.

  inflate_fast() speedups that turned out slower (on a PowerPC G3 750CXe):
   - Using bit fields for code structure
   - Different op definition to avoid & for extra bits (do & for table bits)
   - Three separate decoding do-loops for direct, window, and wnext == 0
   - Special case for distance > 1 copies to do overlapped load and store copy
   - Explicit branch predictions (based on measured branch probabilities)
   - Deferring match copy and interspersed it with decoding subsequent codes
   - Swapping literal/length else
   - Swapping window/direct else
   - Larger unrolled copy loops (three is about right)
   - Moving len -= 3 statement into middle of loop
 */
void
inflate_stream::
inflate_fast(ranges& r, error_code& ec)
{
    unsigned char const* last;  // have enough input while in < last
    unsigned char *end;         // while out < end, enough space available
    std::size_t op;             // code bits, operation, extra bits, or window position, window bytes to copy
    unsigned len;               // match length, unused bytes
    unsigned dist;              // match distance
    unsigned const lmask =
        (1U << lenbits_) - 1;   // mask for first level of length codes
    unsigned const dmask =
        (1U << distbits_) - 1;  // mask for first level of distance codes

    last = r.in.next + (r.in.avail() - 5);
    end = r.out.next + (r.out.avail() - 257);

    /* decode literals and length/distances until end-of-block or not enough
       input data or output space */
    do
    {
        if(bi_.size() < 15)
            bi_.fill_16(r.in.next);
        auto cp = &lencode_[bi_.peek_fast() & lmask];
    dolen:
        bi_.drop(cp->bits);
        op = (unsigned)(cp->op);
        if(op == 0)
        {
            // literal
            *r.out.next++ = (unsigned char)(cp->val);
        }
        else if(op & 16)
        {
            // length base
            len = (unsigned)(cp->val);
            op &= 15; // number of extra bits
            if(op)
            {
                if(bi_.size() < op)
                    bi_.fill_8(r.in.next);
                len += (unsigned)bi_.peek_fast() & ((1U << op) - 1);
                bi_.drop(op);
            }
            if(bi_.size() < 15)
                bi_.fill_16(r.in.next);
            cp = &distcode_[bi_.peek_fast() & dmask];
        dodist:
            bi_.drop(cp->bits);
            op = (unsigned)(cp->op);
            if(op & 16)
            {
                // distance base
                dist = (unsigned)(cp->val);
                op &= 15; // number of extra bits
                if(bi_.size() < op)
                {
                    bi_.fill_8(r.in.next);
                    if(bi_.size() < op)
                        bi_.fill_8(r.in.next);
                }
                dist += (unsigned)bi_.peek_fast() & ((1U << op) - 1);
#ifdef INFLATE_STRICT
                if(dist > dmax_)
                {
                    ec = error::invalid_distance;
                    mode_ = BAD;
                    break;
                }
#endif
                bi_.drop(op);

                op = r.out.used();
                if(dist > op)
                {
                    // copy from window
                    op = dist - op; // distance back in window
                    if(op > w_.size())
                    {
                        ec = error::invalid_distance;
                        mode_ = BAD;
                        break;
                    }
                    auto const n = clamp(len, op);
                    w_.read(r.out.next, op, n);
                    r.out.next += n;
                    len -= n;
                }
                if(len > 0)
                {
                    // copy from output
                    auto in = r.out.next - dist;
                    auto n = clamp(len, r.out.avail());
                    len -= n;
                    while(n--)
                        *r.out.next++ = *in++;
                }
            }
            else if((op & 64) == 0)
            {
                // 2nd level distance code
                cp = &distcode_[cp->val + (bi_.peek_fast() & ((1U << op) - 1))];
                goto dodist;
            }
            else
            {
                ec = error::invalid_distance_code;
                mode_ = BAD;
                break;
            }
        }
        else if((op & 64) == 0)
        {
            // 2nd level length code
            cp = &lencode_[cp->val + (bi_.peek_fast() & ((1U << op) - 1))];
            goto dolen;
        }
        else if(op & 32)
        {
            // end-of-block
            mode_ = TYPE;
            break;
        }
        else
        {
            ec = error::invalid_literal_length;
            mode_ = BAD;
            break;
        }
    }
    while(r.in.next < last && r.out.next < end);

    // return unused bytes (on entry, bits < 8, so in won't go too far back)
    bi_.rewind(r.in.next);
}

} // detail
} // zlib
} // beast
} // boost

#endif

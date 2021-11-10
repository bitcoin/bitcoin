//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_ZLIB_DEFLATE_STREAM_HPP
#define BOOST_BEAST_ZLIB_DEFLATE_STREAM_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/zlib/error.hpp>
#include <boost/beast/zlib/zlib.hpp>
#include <boost/beast/zlib/detail/deflate_stream.hpp>
#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <memory>

namespace boost {
namespace beast {
namespace zlib {

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

/** Raw deflate compressor.

    This is a port of zlib's "deflate" functionality to C++.
*/
class deflate_stream
    : private detail::deflate_stream
{
public:
    /** Construct a default deflate stream.

        Upon construction, the stream settings will be set
        to these default values:

        @li `level = 6`

        @li `windowBits = 15`

        @li `memLevel = 8`

        @li `strategy = Strategy::normal`

        Although the stream is ready to be used immediately
        after construction, any required internal buffers are
        not dynamically allocated until needed.
    */
    deflate_stream()
    {
        reset(6, 15, DEF_MEM_LEVEL, Strategy::normal);
    }

    /** Reset the stream and compression settings.

        This function initializes the stream to the specified
        compression settings.

        Although the stream is ready to be used immediately
        after a reset, any required internal buffers are not
        dynamically allocated until needed.

        @note Any unprocessed input or pending output from
        previous calls are discarded.
    */
    void
    reset(
        int level,
        int windowBits,
        int memLevel,
        Strategy strategy)
    {
        doReset(level, windowBits, memLevel, strategy);
    }

    /** Reset the stream without deallocating memory.

        This function performs the equivalent of calling `clear`
        followed by `reset` with the same compression settings,
        without deallocating the internal buffers.

        @note Any unprocessed input or pending output from
        previous calls are discarded.
    */
    void
    reset()
    {
        doReset();
    }

    /** Clear the stream.

        This function resets the stream and frees all dynamically
        allocated internal buffers. The compression settings are
        left unchanged.

        @note Any unprocessed input or pending output from
        previous calls are discarded.
    */
    void
    clear()
    {
        doClear();
    }

    /** Returns the upper limit on the size of a compressed block.

        This function makes a conservative estimate of the maximum number
        of bytes needed to store the result of compressing a block of
        data based on the current compression level and strategy.

        @param sourceLen The size of the uncompressed data.

        @return The maximum number of resulting compressed bytes.
    */
    std::size_t
    upper_bound(std::size_t sourceLen) const
    {
        return doUpperBound(sourceLen);
    }

    /** Fine tune internal compression parameters.

        Compression parameters should only be tuned by someone who
        understands the algorithm used by zlib's deflate for searching
        for the best matching string, and even then only by the most
        fanatic optimizer trying to squeeze out the last compressed bit
        for their specific input data. Read the deflate.c source code
        (ZLib) for the meaning of the max_lazy, good_length, nice_length,
        and max_chain parameters.
    */
    void
    tune(
        int good_length,
        int max_lazy,
        int nice_length,
        int max_chain)
    {
        doTune(good_length, max_lazy, nice_length, max_chain);
    }

    /** Compress input and write output.

        This function compresses as much data as possible, and stops when
        the input buffer becomes empty or the output buffer becomes full.
        It may introduce some output latency (reading input without
        producing any output) except when forced to flush.

        In each call, one or both of these actions are performed:

        @li Compress more input starting at `zs.next_in` and update
        `zs.next_in` and `zs.avail_in` accordingly. If not all
        input can be processed (because there is not enough room in
        the output buffer), `zs.next_in` and `zs.avail_in` are updated
        and processing will resume at this point for the next call.

        @li Provide more output starting at `zs.next_out` and update
        `zs.next_out` and `zs.avail_out` accordingly. This action is
        forced if the parameter flush is not `Flush::none`. Forcing
        flush frequently degrades the compression ratio, so this parameter
        should be set only when necessary (in interactive applications).
        Some output may be provided even if flush is not set.

        Before the call, the application must ensure that at least one
        of the actions is possible, by providing more input and/or
        consuming more output, and updating `zs.avail_in` or `zs.avail_out`
        accordingly; `zs.avail_out` should never be zero before the call.
        The application can consume the compressed output when it wants,
        for example when the output buffer is full (`zs.avail_out == 0`),
        or after each call of `write`. If `write` returns no error
        with zero `zs.avail_out`, it must be called again after making
        room in the output buffer because there might be more output
        pending.

        Normally the parameter flush is set to `Flush::none`, which allows
        deflate to decide how much data to accumulate before producing
        output, in order to maximize compression.

        If the parameter flush is set to `Flush::sync`, all pending output
        is flushed to the output buffer and the output is aligned on a
        byte boundary, so that the decompressor can get all input data
        available so far. In particular `zs.avail_in` is zero after the
        call if enough output space has been provided before the call.
        Flushing may degrade compression for some compression algorithms
        and so it should be used only when necessary. This completes the
        current deflate block and follows it with an empty stored block
        that is three bits plus filler bits to the next byte, followed
        by the four bytes `{ 0x00, 0x00 0xff 0xff }`.

        If flush is set to `Flush::partial`, all pending output is flushed
        to the output buffer, but the output is not aligned to a byte
        boundary. All of the input data so far will be available to the
        decompressor, as for Z_SYNC_FLUSH. This completes the current
        deflate block and follows it with an empty fixed codes block that
        is 10 bits long. This assures that enough bytes are output in order
        for the decompressor to finish the block before the empty fixed
        code block.

        If flush is set to `Flush::block`, a deflate block is completed
        and emitted, as for `Flush::sync`, but the output is not aligned
        on a byte boundary, and up to seven bits of the current block are
        held to be written as the next byte after the next deflate block
        is completed. In this case, the decompressor may not be provided
        enough bits at this point in order to complete decompression of
        the data provided so far to the compressor. It may need to wait
        for the next block to be emitted. This is for advanced applications
        that need to control the emission of deflate blocks.

        If flush is set to `Flush::full`, all output is flushed as with
        `Flush::sync`, and the compression state is reset so that
        decompression can restart from this point if previous compressed
        data has been damaged or if random access is desired. Using
        `Flush::full` too often can seriously degrade compression.

        If `write` returns with `zs.avail_out == 0`, this function must
        be called again with the same value of the flush parameter and
        more output space (updated `zs.avail_out`), until the flush is
        complete (`write` returns with non-zero `zs.avail_out`). In the
        case of a `Flush::full`or `Flush::sync`, make sure that
        `zs.avail_out` is greater than six to avoid repeated flush markers
        due to `zs.avail_out == 0` on return.

        If the parameter flush is set to `Flush::finish`, pending input
        is processed, pending output is flushed and deflate returns the
        error `error::end_of_stream` if there was enough output space;
        if deflate returns with no error, this function must be called
        again with `Flush::finish` and more output space (updated
        `zs.avail_out`) but no more input data, until it returns the
        error `error::end_of_stream` or another error. After `write` has
        returned the `error::end_of_stream` error, the only possible
        operations on the stream are to reset or destroy.

        `Flush::finish` can be used immediately after initialization
        if all the compression is to be done in a single step. In this
        case, `zs.avail_out` must be at least value returned by
        `upper_bound` (see below). Then `write` is guaranteed to return
        the `error::end_of_stream` error. If not enough output space
        is provided, deflate will not return `error::end_of_stream`,
        and it must be called again as described above.

        `write` returns no error if some progress has been made (more
        input processed or more output produced), `error::end_of_stream`
        if all input has been consumed and all output has been produced
        (only when flush is set to `Flush::finish`), `error::stream_error`
        if the stream state was inconsistent (for example if `zs.next_in`
        or `zs.next_out` was `nullptr`), `error::need_buffers` if no
        progress is possible (for example `zs.avail_in` or `zs.avail_out`
        was zero). Note that `error::need_buffers` is not fatal, and
        `write` can be called again with more input and more output space
        to continue compressing.
    */
    void
    write(
        z_params& zs,
        Flush flush,
        error_code& ec)
    {
        doWrite(zs, flush, ec);
    }

    /** Update the compression level and strategy.

        This function dynamically updates the compression level and
        compression strategy. The interpretation of level and strategy
        is as in @ref reset. This can be used to switch between compression
        and straight copy of the input data, or to switch to a different kind
        of input data requiring a different strategy. If the compression level
        is changed, the input available so far is compressed with the old level
        (and may be flushed); the new level will take effect only at the next
        call of @ref write.

        Before the call of `params`, the stream state must be set as for a
        call of @ref write, since the currently available input may have to be
        compressed and flushed. In particular, `zs.avail_out` must be non-zero.

        @return `Z_OK` if success, `Z_STREAM_ERROR` if the source stream state
        was inconsistent or if a parameter was invalid, `error::need_buffers`
        if `zs.avail_out` was zero.
    */
    void
    params(
        z_params& zs,
        int level,
        Strategy strategy,
        error_code& ec)
    {
        doParams(zs, level, strategy, ec);
    }

    /** Return bits pending in the output.

        This function returns the number of bytes and bits of output
        that have been generated, but not yet provided in the available
        output. The bytes not provided would be due to the available
        output space having being consumed. The number of bits of output
        not provided are between 0 and 7, where they await more bits to
        join them in order to fill out a full byte. If pending or bits
        are `nullptr`, then those values are not set.

        @return `Z_OK` if success, or `Z_STREAM_ERROR` if the source
        stream state was inconsistent.
    */
    void
    pending(unsigned *value, int *bits)
    {
        doPending(value, bits);
    }

    /** Insert bits into the compressed output stream.

        This function inserts bits in the deflate output stream. The
        intent is that this function is used to start off the deflate
        output with the bits leftover from a previous deflate stream when
        appending to it. As such, this function can only be used for raw
        deflate, and must be used before the first `write` call after an
        initialization. `bits` must be less than or equal to 16, and that
        many of the least significant bits of `value` will be inserted in
        the output.

        @return `error::need_buffers` if there was not enough room in
        the internal buffer to insert the bits.
    */
    void
    prime(int bits, int value, error_code& ec)
    {
        return doPrime(bits, value, ec);
    }
};

/** Returns the upper limit on the size of a compressed block.

    This function makes a conservative estimate of the maximum number
    of bytes needed to store the result of compressing a block of
    data.

    @param bytes The size of the uncompressed data.

    @return The maximum number of resulting compressed bytes.
*/
std::size_t
deflate_upper_bound(std::size_t bytes);

/*  For the default windowBits of 15 and memLevel of 8, this function returns
    a close to exact, as well as small, upper bound on the compressed size.
    They are coded as constants here for a reason--if the #define's are
    changed, then this function needs to be changed as well.  The return
    value for 15 and 8 only works for those exact settings.

    For any setting other than those defaults for windowBits and memLevel,
    the value returned is a conservative worst case for the maximum expansion
    resulting from using fixed blocks instead of stored blocks, which deflate
    can emit on compressed data for some combinations of the parameters.

    This function could be more sophisticated to provide closer upper bounds for
    every combination of windowBits and memLevel.  But even the conservative
    upper bound of about 14% expansion does not seem onerous for output buffer
    allocation.
*/
inline
std::size_t
deflate_upper_bound(std::size_t bytes)
{
    return bytes +
        ((bytes + 7) >> 3) +
        ((bytes + 63) >> 6) + 5 +
        6;
}

} // zlib
} // beast
} // boost

#endif

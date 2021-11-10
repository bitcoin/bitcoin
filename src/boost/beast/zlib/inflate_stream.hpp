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

#ifndef BOOST_BEAST_ZLIB_INFLATE_STREAM_HPP
#define BOOST_BEAST_ZLIB_INFLATE_STREAM_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/zlib/detail/inflate_stream.hpp>

namespace boost {
namespace beast {
namespace zlib {

/** Raw deflate stream decompressor.

    This implements a raw deflate stream decompressor. The deflate
    protocol is a compression protocol described in
    "DEFLATE Compressed Data Format Specification version 1.3"
    located here: https://tools.ietf.org/html/rfc1951

    The implementation is a refactored port to C++ of ZLib's "inflate".
    A more detailed description of ZLib is at http://zlib.net/.

    Compression can be done in a single step if the buffers are large
    enough (for example if an input file is memory mapped), or can be done
    by repeated calls of the compression function. In the latter case, the
    application must provide more input and/or consume the output (providing
    more output space) before each call.
*/
class inflate_stream
    : private detail::inflate_stream
{
public:
    /** Construct a raw deflate decompression stream.

        The window size is set to the default of 15 bits.
    */
    inflate_stream() = default;

    /** Reset the stream.

        This puts the stream in a newly constructed state with
        the previously specified window size, but without de-allocating
        any dynamically created structures.
    */
    void
    reset()
    {
        doReset();
    }

    /** Reset the stream.

        This puts the stream in a newly constructed state with the
        specified window size, but without de-allocating any dynamically
        created structures.
    */
    void
    reset(int windowBits)
    {
        doReset(windowBits);
    }

    /** Put the stream in a newly constructed state.

        All dynamically allocated memory is de-allocated.
    */
    void
    clear()
    {
        doClear();
    }

    /** Decompress input and produce output.

        This function decompresses as much data as possible, and stops when
        the input buffer becomes empty or the output buffer becomes full. It
        may introduce some output latency (reading input without producing any
        output) except when forced to flush.

        One or both of the following actions are performed:

        @li Decompress more input starting at `zs.next_in` and update `zs.next_in`
        and `zs.avail_in` accordingly. If not all input can be processed (because
        there is not enough room in the output buffer), `zs.next_in` is updated
        and processing will resume at this point for the next call.

        @li Provide more output starting at `zs.next_out` and update `zs.next_out`
        and `zs.avail_out` accordingly. `write` provides as much output as
        possible, until there is no more input data or no more space in the output
        buffer (see below about the flush parameter).

        Before the call, the application should ensure that at least one of the
        actions is possible, by providing more input and/or consuming more output,
        and updating the values in `zs` accordingly. The application can consume
        the uncompressed output when it wants, for example when the output buffer
        is full (`zs.avail_out == 0`), or after each call. If `write` returns no
        error and with zero `zs.avail_out`, it must be called again after making
        room in the output buffer because there might be more output pending.

        The flush parameter may be `Flush::none`, `Flush::sync`, `Flush::finish`,
        `Flush::block`, or `Flush::trees`. `Flush::sync` requests to flush as much
        output as possible to the output buffer. `Flush::block` requests to stop if
        and when it gets to the next deflate block boundary. When decoding the
        zlib or gzip format, this will cause `write` to return immediately after
        the header and before the first block. When doing a raw inflate, `write` will
        go ahead and process the first block, and will return when it gets to the
        end of that block, or when it runs out of data.

        The `Flush::block` option assists in appending to or combining deflate
        streams. Also to assist in this, on return `write` will set `zs.data_type`
        to the number of unused bits in the last byte taken from `zs.next_in`, plus
        64 if `write` is currently decoding the last block in the deflate stream,
        plus 128 if `write` returned immediately after decoding an end-of-block code
        or decoding the complete header up to just before the first byte of the
        deflate stream. The end-of-block will not be indicated until all of the
        uncompressed data from that block has been written to `zs.next_out`. The
        number of unused bits may in general be greater than seven, except when
        bit 7 of `zs.data_type` is set, in which case the number of unused bits
        will be less than eight. `zs.data_type` is set as noted here every time
        `write` returns for all flush options, and so can be used to determine the
        amount of currently consumed input in bits.

        The `Flush::trees` option behaves as `Flush::block` does, but it also returns
        when the end of each deflate block header is reached, before any actual data
        in that block is decoded. This allows the caller to determine the length of
        the deflate block header for later use in random access within a deflate block.
        256 is added to the value of `zs.data_type` when `write` returns immediately
        after reaching the end of the deflate block header.

        `write` should normally be called until it returns `error::end_of_stream` or
        another error. However if all decompression is to be performed in a single
        step (a single call of `write`), the parameter flush should be set to
        `Flush::finish`. In this case all pending input is processed and all pending
        output is flushed; `zs.avail_out` must be large enough to hold all of the
        uncompressed data for the operation to complete. (The size of the uncompressed
        data may have been saved by the compressor for this purpose.) The use of
        `Flush::finish` is not required to perform an inflation in one step. However
        it may be used to inform inflate that a faster approach can be used for the
        single call. `Flush::finish` also informs inflate to not maintain a sliding
        window if the stream completes, which reduces inflate's memory footprint.
        If the stream does not complete, either because not all of the stream is
        provided or not enough output space is provided, then a sliding window will be
        allocated and `write` can be called again to continue the operation as if
        `Flush::none` had been used.

        In this implementation, `write` always flushes as much output as possible to
        the output buffer, and always uses the faster approach on the first call. So
        the effects of the flush parameter in this implementation are on the return value
        of `write` as noted below, when `write` returns early when `Flush::block` or
        `Flush::trees` is used, and when `write` avoids the allocation of memory for a
        sliding window when `Flush::finish` is used.

        If a preset dictionary is needed after this call,
        `write` sets `zs.adler` to the Adler-32 checksum of the dictionary chosen by
        the compressor and returns `error::need_dictionary`; otherwise it sets
        `zs.adler` to the Adler-32 checksum of all output produced so far (that is,
        `zs.total_out bytes`) and returns no error, `error::end_of_stream`, or an
        error code as described below. At the end of the stream, `write` checks that
        its computed adler32 checksum is equal to that saved by the compressor and
        returns `error::end_of_stream` only if the checksum is correct.

        This function returns no error if some progress has been made (more input
        processed or more output produced), `error::end_of_stream` if the end of the
        compressed data has been reached and all uncompressed output has been produced,
        `error::need_dictionary` if a preset dictionary is needed at this point,
        `error::invalid_data` if the input data was corrupted (input stream not
        conforming to the zlib format or incorrect check value), `error::stream_error`
        if the stream structure was inconsistent (for example if `zs.next_in` or
        `zs.next_out` was null), `error::need_buffers` if no progress is possible or
        if there was not enough room in the output buffer when `Flush::finish` is
        used. Note that `error::need_buffers` is not fatal, and `write` can be called
        again with more input and more output space to continue decompressing.
    */
    void
    write(z_params& zs, Flush flush, error_code& ec)
    {
        doWrite(zs, flush, ec);
    }
};

} // zlib
} // beast
} // boost

#endif

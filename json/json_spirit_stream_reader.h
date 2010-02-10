#ifndef JSON_SPIRIT_READ_STREAM
#define JSON_SPIRIT_READ_STREAM

//          Copyright John W. Wilkinson 2007 - 2009.
// Distributed under the MIT License, see accompanying file LICENSE.txt

// json spirit version 4.03

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include "json_spirit_reader_template.h"

namespace json_spirit
{
    // these classes allows you to read multiple top level contiguous values from a stream,
    // the normal stream read functions have a bug that prevent multiple top level values 
    // from being read unless they are separated by spaces

    template< class Istream_type, class Value_type >
    class Stream_reader
    {
    public:

        Stream_reader( Istream_type& is )
        :   iters_( is )
        {
        }

        bool read_next( Value_type& value )
        {
            return read_range( iters_.begin_, iters_.end_, value );
        }

    private:

        typedef Multi_pass_iters< Istream_type > Mp_iters;

        Mp_iters iters_;
    };

    template< class Istream_type, class Value_type >
    class Stream_reader_thrower
    {
    public:

        Stream_reader_thrower( Istream_type& is )
        :   iters_( is )
        ,    posn_begin_( iters_.begin_, iters_.end_ )
        ,    posn_end_( iters_.end_, iters_.end_ )
        {
        }

        void read_next( Value_type& value )
        {
            posn_begin_ = read_range_or_throw( posn_begin_, posn_end_, value );
        }

    private:

        typedef Multi_pass_iters< Istream_type > Mp_iters;
        typedef spirit_namespace::position_iterator< typename Mp_iters::Mp_iter > Posn_iter_t;

        Mp_iters iters_;
        Posn_iter_t posn_begin_, posn_end_;
    };
}

#endif

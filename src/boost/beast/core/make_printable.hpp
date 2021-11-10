//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_MAKE_PRINTABLE_HPP
#define BOOST_BEAST_MAKE_PRINTABLE_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/buffer_traits.hpp>
#include <boost/asio/buffer.hpp>
#include <ostream>

namespace boost {
namespace beast {

namespace detail {

template<class Buffers>
class make_printable_adaptor
{
    Buffers b_;

public:
    explicit
    make_printable_adaptor(Buffers const& b)
        : b_(b)
    {
    }

    template<class B>
    friend
    std::ostream&
    operator<<(std::ostream& os,
        make_printable_adaptor<B> const& v);
};

template<class Buffers>
std::ostream&
operator<<(std::ostream& os,
    make_printable_adaptor<Buffers> const& v)
{
    for(
        auto it = net::buffer_sequence_begin(v.b_),
        end = net::buffer_sequence_end(v.b_);
        it != end;
        ++it)
    {
        net::const_buffer cb = *it;
        os.write(static_cast<char const*>(
            cb.data()), cb.size());
    }
    return os;
}

} // detail

/** Helper to permit a buffer sequence to be printed to a std::ostream

    This function is used to wrap a buffer sequence to allow it to
    be interpreted as characters and written to a `std::ostream` such
    as `std::cout`. No character translation is performed; unprintable
    and null characters will be transferred as-is to the output stream.

    @par Example
    This function prints the size and contents of a buffer sequence
    to standard output:
    @code
    template <class ConstBufferSequence>
    void
    print (ConstBufferSequence const& buffers)
    {
        std::cout <<
            "Buffer size: " << buffer_bytes(buffers) << " bytes\n"
            "Buffer data: '" << make_printable(buffers) << "'\n";
    }
    @endcode

    @param buffers An object meeting the requirements of
    <em>ConstBufferSequence</em> to be streamed. The implementation
    will make a copy of this object. Ownership of the underlying
    memory is not transferred, the application is still responsible
    for managing its lifetime.
*/
template<class ConstBufferSequence>
#if BOOST_BEAST_DOXYGEN
__implementation_defined__
#else
detail::make_printable_adaptor<ConstBufferSequence>
#endif
make_printable(ConstBufferSequence const& buffers)
{
    static_assert(net::is_const_buffer_sequence<
        ConstBufferSequence>::value,
            "ConstBufferSequence type requirements not met");
    return detail::make_printable_adaptor<
        ConstBufferSequence>{buffers};
}

} // beast
} // boost

#endif

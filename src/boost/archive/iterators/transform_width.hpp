#ifndef BOOST_ARCHIVE_ITERATORS_TRANSFORM_WIDTH_HPP
#define BOOST_ARCHIVE_ITERATORS_TRANSFORM_WIDTH_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// transform_width.hpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

// iterator which takes elements of x bits and returns elements of y bits.
// used to change streams of 8 bit characters into streams of 6 bit characters.
// and vice-versa for implementing base64 encodeing/decoding. Be very careful
// when using and end iterator.  end is only reliable detected when the input
// stream length is some common multiple of x and y.  E.G. Base64 6 bit
// character and 8 bit bytes. Lowest common multiple is 24 => 4 6 bit characters
// or 3 8 bit characters

#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/iterator/iterator_traits.hpp>

#include <algorithm> // std::min

namespace boost {
namespace archive {
namespace iterators {

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// class used by text archives to translate char strings to wchar_t
// strings of the currently selected locale
template<
    class Base,
    int BitsOut,
    int BitsIn,
    class CharType = typename boost::iterator_value<Base>::type // output character
>
class transform_width :
    public boost::iterator_adaptor<
        transform_width<Base, BitsOut, BitsIn, CharType>,
        Base,
        CharType,
        single_pass_traversal_tag,
        CharType
    >
{
    friend class boost::iterator_core_access;
    typedef typename boost::iterator_adaptor<
        transform_width<Base, BitsOut, BitsIn, CharType>,
        Base,
        CharType,
        single_pass_traversal_tag,
        CharType
    > super_t;

    typedef transform_width<Base, BitsOut, BitsIn, CharType> this_t;
    typedef typename iterator_value<Base>::type base_value_type;

    void fill();

    CharType dereference() const {
        if(!m_buffer_out_full)
            const_cast<this_t *>(this)->fill();
        return m_buffer_out;
    }

    bool equal_impl(const this_t & rhs){
        if(BitsIn < BitsOut) // discard any left over bits
            return this->base_reference() == rhs.base_reference();
        else{
            // BitsIn > BitsOut  // zero fill
            if(this->base_reference() == rhs.base_reference()){
                m_end_of_sequence = true;
                return 0 == m_remaining_bits;
            }
            return false;
        }
    }

    // standard iterator interface
    bool equal(const this_t & rhs) const {
        return const_cast<this_t *>(this)->equal_impl(rhs);
    }

    void increment(){
        m_buffer_out_full = false;
    }

    bool m_buffer_out_full;
    CharType m_buffer_out;

    // last read element from input
    base_value_type m_buffer_in;

    // number of bits to left in the input buffer.
    unsigned int m_remaining_bits;

    // flag to indicate we've reached end of data.
    bool m_end_of_sequence;

public:
    // make composible buy using templated constructor
    template<class T>
    transform_width(T start) :
        super_t(Base(static_cast< T >(start))),
        m_buffer_out_full(false),
        m_buffer_out(0),
        // To disable GCC warning, but not truly necessary
	    //(m_buffer_in will be initialized later before being
	    //used because m_remaining_bits == 0)
        m_buffer_in(0),
        m_remaining_bits(0),
        m_end_of_sequence(false)
    {}
    // intel 7.1 doesn't like default copy constructor
    transform_width(const transform_width & rhs) :
        super_t(rhs.base_reference()),
        m_buffer_out_full(rhs.m_buffer_out_full),
        m_buffer_out(rhs.m_buffer_out),
        m_buffer_in(rhs.m_buffer_in),
        m_remaining_bits(rhs.m_remaining_bits),
        m_end_of_sequence(false)
    {}
};

template<
    class Base,
    int BitsOut,
    int BitsIn,
    class CharType
>
void transform_width<Base, BitsOut, BitsIn, CharType>::fill() {
    unsigned int missing_bits = BitsOut;
    m_buffer_out = 0;
    do{
        if(0 == m_remaining_bits){
            if(m_end_of_sequence){
                m_buffer_in = 0;
                m_remaining_bits = missing_bits;
            }
            else{
                m_buffer_in = * this->base_reference()++;
                m_remaining_bits = BitsIn;
            }
        }

        // append these bits to the next output
        // up to the size of the output
        unsigned int i = (std::min)(missing_bits, m_remaining_bits);
        // shift interesting bits to least significant position
        base_value_type j = m_buffer_in >> (m_remaining_bits - i);
        // and mask off the un interesting higher bits
        // note presumption of twos complement notation
        j &= (1 << i) - 1;
        // append then interesting bits to the output value
        m_buffer_out <<= i;
        m_buffer_out |= j;

        // and update counters
        missing_bits -= i;
        m_remaining_bits -= i;
    }while(0 < missing_bits);
    m_buffer_out_full = true;
}

} // namespace iterators
} // namespace archive
} // namespace boost

#endif // BOOST_ARCHIVE_ITERATORS_TRANSFORM_WIDTH_HPP

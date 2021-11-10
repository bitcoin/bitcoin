//
// Copyright 2005-2007 Adobe Systems Incorporated
// Copyright 2019 Mateusz Loskot <mateusz at loskot dot net>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_BIT_ALIGNED_PIXEL_REFERENCE_HPP
#define BOOST_GIL_BIT_ALIGNED_PIXEL_REFERENCE_HPP

#include <boost/gil/pixel.hpp>
#include <boost/gil/channel.hpp>
#include <boost/gil/detail/mp11.hpp>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <functional>
#include <type_traits>

namespace boost { namespace gil {

/// A model of a heterogeneous pixel that is not byte aligned.
/// Examples are bitmap (1-bit pixels) or 6-bit RGB (222).

/////////////////////////////
//  bit_range
//
//  Represents a range of bits that can span multiple consecutive bytes. The range has a size fixed at compile time, but the offset is specified at run time.
/////////////////////////////

template <int RangeSize, bool IsMutable>
class bit_range {
public:
    using byte_t = mp11::mp_if_c<IsMutable, unsigned char, unsigned char const>;
    using difference_type = std::ptrdiff_t;
    template <int RS, bool M> friend class bit_range;
private:
    byte_t* _current_byte;   // the starting byte of the bit range
    int     _bit_offset;     // offset from the beginning of the current byte. 0<=_bit_offset<=7

public:
    bit_range() : _current_byte(nullptr), _bit_offset(0) {}
    bit_range(byte_t* current_byte, int bit_offset)
        : _current_byte(current_byte)
        , _bit_offset(bit_offset)
    {
        BOOST_ASSERT(bit_offset >= 0 && bit_offset < 8);
    }

    bit_range(const bit_range& br) : _current_byte(br._current_byte), _bit_offset(br._bit_offset) {}
    template <bool M> bit_range(const bit_range<RangeSize,M>& br) : _current_byte(br._current_byte), _bit_offset(br._bit_offset) {}

    bit_range& operator=(const bit_range& br) { _current_byte = br._current_byte; _bit_offset=br._bit_offset; return *this; }
    bool operator==(const bit_range& br) const { return  _current_byte==br._current_byte && _bit_offset==br._bit_offset; }

    bit_range& operator++() {
        _current_byte += (_bit_offset+RangeSize) / 8;
        _bit_offset    = (_bit_offset+RangeSize) % 8;
        return *this;
    }
    bit_range& operator--() { bit_advance(-RangeSize); return *this; }

    void bit_advance(difference_type num_bits) {
        int new_offset = int(_bit_offset+num_bits);
        _current_byte += new_offset / 8;
        _bit_offset    = new_offset % 8;
        if (_bit_offset<0) {
            _bit_offset+=8;
            --_current_byte;
        }
    }
    difference_type bit_distance_to(const bit_range& b) const {
        return (b.current_byte() - current_byte())*8 + b.bit_offset()-bit_offset();
    }
    byte_t* current_byte() const { return _current_byte; }
    int     bit_offset()   const { return _bit_offset; }
};

/// \defgroup ColorBaseModelNonAlignedPixel bit_aligned_pixel_reference
/// \ingroup ColorBaseModel
/// \brief A heterogeneous color base representing pixel that may not be byte aligned, i.e. it may correspond to a bit range that does not start/end at a byte boundary. Models ColorBaseConcept.
///
/// \defgroup PixelModelNonAlignedPixel bit_aligned_pixel_reference
/// \ingroup PixelModel
/// \brief A heterogeneous pixel reference used to represent non-byte-aligned pixels. Models PixelConcept
///
/// Example:
/// \code
/// unsigned char data=0;
///
/// // A mutable reference to a 6-bit BGR pixel in "123" format (1 bit for red, 2 bits for green, 3 bits for blue)
/// using rgb123_ref_t = bit_aligned_pixel_reference<unsigned char, mp11::mp_list_c<int,1,2,3>, rgb_layout_t, true> const;
///
/// // create the pixel reference at bit offset 2
/// // (i.e. red = [2], green = [3,4], blue = [5,6,7] bits)
/// rgb123_ref_t ref(&data, 2);
/// get_color(ref, red_t()) = 1;
/// assert(data == 0x04);
/// get_color(ref, green_t()) = 3;
/// assert(data == 0x1C);
/// get_color(ref, blue_t()) = 7;
/// assert(data == 0xFC);
/// \endcode
///
/// \ingroup ColorBaseModelNonAlignedPixel PixelModelNonAlignedPixel PixelBasedModel
/// \brief Heterogeneous pixel reference corresponding to non-byte-aligned bit range. Models ColorBaseConcept, PixelConcept, PixelBasedConcept
///
/// \tparam BitField
/// \tparam ChannelBitSizes Boost.MP11-compatible list of integral types defining the number of bits for each channel. For example, for 565RGB, mp_list_c<int,5,6,5>
/// \tparam Layout
/// \tparam IsMutable
template <typename BitField, typename ChannelBitSizes, typename Layout, bool IsMutable>
struct bit_aligned_pixel_reference
{
    static constexpr int bit_size =
            mp11::mp_fold
            <
                ChannelBitSizes,
                std::integral_constant<int, 0>,
                mp11::mp_plus
            >::value;

    using bit_range_t = boost::gil::bit_range<bit_size,IsMutable>;
    using bitfield_t = BitField;
    using data_ptr_t = mp11::mp_if_c<IsMutable, unsigned char*, const unsigned char*>;

    using layout_t = Layout;

    using value_type = typename packed_pixel_type<bitfield_t,ChannelBitSizes,Layout>::type;
    using reference = const bit_aligned_pixel_reference<BitField, ChannelBitSizes, Layout, IsMutable>;
    using const_reference = bit_aligned_pixel_reference<BitField,ChannelBitSizes,Layout,false> const;

    static constexpr bool is_mutable = IsMutable;

    bit_aligned_pixel_reference(){}
    bit_aligned_pixel_reference(data_ptr_t data_ptr, int bit_offset)   : _bit_range(data_ptr, bit_offset) {}
    explicit bit_aligned_pixel_reference(const bit_range_t& bit_range) : _bit_range(bit_range) {}
    template <bool IsMutable2> bit_aligned_pixel_reference(const bit_aligned_pixel_reference<BitField,ChannelBitSizes,Layout,IsMutable2>& p) : _bit_range(p._bit_range) {}

    // Grayscale references can be constructed from the channel reference
    explicit bit_aligned_pixel_reference(typename kth_element_type<bit_aligned_pixel_reference,0>::type const channel0)
        : _bit_range(static_cast<data_ptr_t>(&channel0), channel0.first_bit())
    {
        static_assert(num_channels<bit_aligned_pixel_reference>::value == 1, "");
    }

    // Construct from another compatible pixel type
    bit_aligned_pixel_reference(bit_aligned_pixel_reference const& p)
        : _bit_range(p._bit_range) {}

    // TODO: Why p by non-const reference?
    template <typename BF, typename CR>
    bit_aligned_pixel_reference(packed_pixel<BF, CR, Layout>& p)
        : _bit_range(static_cast<data_ptr_t>(&gil::at_c<0>(p)), gil::at_c<0>(p).first_bit())
    {
        check_compatible<packed_pixel<BF, CR, Layout>>();
    }

    auto operator=(bit_aligned_pixel_reference const& p) const
        -> bit_aligned_pixel_reference const&
    {
        static_copy(p, *this);
        return *this;
    }

    template <typename P>
    auto operator=(P const& p) const -> bit_aligned_pixel_reference const&
    {
        assign(p, is_pixel<P>());
        return *this;
    }

    template <typename P>
    bool operator==(P const& p) const
    {
        return equal(p, is_pixel<P>());
    }

    template <typename P>
    bool operator!=(P const& p) const { return !(*this==p); }

    auto operator->() const -> bit_aligned_pixel_reference const* { return this; }

    bit_range_t const& bit_range() const { return _bit_range; }

private:
    mutable bit_range_t _bit_range;
    template <typename B, typename C, typename L, bool M> friend struct bit_aligned_pixel_reference;

    template <typename Pixel> static void check_compatible() { gil_function_requires<PixelsCompatibleConcept<Pixel,bit_aligned_pixel_reference> >(); }

    template <typename Pixel>
    void assign(Pixel const& p, std::true_type) const
    {
        check_compatible<Pixel>();
        static_copy(p, *this);
    }

    template <typename Pixel>
    bool equal(Pixel const& p, std::true_type) const
    {
        check_compatible<Pixel>();
        return static_equal(*this, p);
    }

private:
    static void check_gray()
    {
        static_assert(std::is_same<typename Layout::color_space_t, gray_t>::value, "");
    }

    template <typename Channel>
    void assign(Channel const& channel, std::false_type) const
    {
        check_gray();
        gil::at_c<0>(*this) = channel;
    }

    template <typename Channel>
    bool equal (Channel const& channel, std::false_type) const
    {
        check_gray();
        return gil::at_c<0>(*this) == channel;
    }
};

/////////////////////////////
//  ColorBasedConcept
/////////////////////////////

template <typename BitField, typename ChannelBitSizes, typename L, bool IsMutable, int K>
struct kth_element_type
<
    bit_aligned_pixel_reference<BitField, ChannelBitSizes, L, IsMutable>,
    K
>
{
    using type = packed_dynamic_channel_reference
        <
            BitField,
            mp11::mp_at_c<ChannelBitSizes, K>::value,
            IsMutable
        > const;
};

template <typename B, typename C, typename L, bool M, int K>
struct kth_element_reference_type<bit_aligned_pixel_reference<B,C,L,M>, K>
    : public kth_element_type<bit_aligned_pixel_reference<B,C,L,M>, K> {};

template <typename B, typename C, typename L, bool M, int K>
struct kth_element_const_reference_type<bit_aligned_pixel_reference<B,C,L,M>, K>
    : public kth_element_type<bit_aligned_pixel_reference<B,C,L,M>, K> {};

namespace detail {

// returns sum of IntegralVector[0] ... IntegralVector[K-1]
template <typename IntegralVector, int K>
struct sum_k
    : mp11::mp_plus
        <
            sum_k<IntegralVector, K - 1>,
            typename mp11::mp_at_c<IntegralVector, K - 1>::type
        >
{};

template <typename IntegralVector>
struct sum_k<IntegralVector, 0> : std::integral_constant<int, 0> {};

} // namespace detail

// at_c required by MutableColorBaseConcept
template <int K, typename BitField, typename ChannelBitSizes, typename L, bool IsMutable>
inline
auto at_c(const bit_aligned_pixel_reference<BitField, ChannelBitSizes, L, IsMutable>& p)
    -> typename kth_element_reference_type<bit_aligned_pixel_reference<BitField, ChannelBitSizes, L, IsMutable>, K>::type
{
    using pixel_t = bit_aligned_pixel_reference<BitField, ChannelBitSizes, L, IsMutable>;
    using channel_t = typename kth_element_reference_type<pixel_t, K>::type;
    using bit_range_t = typename pixel_t::bit_range_t;

    bit_range_t bit_range(p.bit_range());
    bit_range.bit_advance(detail::sum_k<ChannelBitSizes, K>::value);

    return channel_t(bit_range.current_byte(), bit_range.bit_offset());
}

/////////////////////////////
//  PixelConcept
/////////////////////////////

/// Metafunction predicate that flags bit_aligned_pixel_reference as a model of PixelConcept. Required by PixelConcept
template <typename B, typename C, typename L, bool M>
struct is_pixel<bit_aligned_pixel_reference<B, C, L, M> > : std::true_type {};

/////////////////////////////
//  PixelBasedConcept
/////////////////////////////

template <typename B, typename C, typename L, bool M>
struct color_space_type<bit_aligned_pixel_reference<B, C, L, M>>
{
    using type = typename L::color_space_t;
};

template <typename B, typename C, typename L, bool M>
struct channel_mapping_type<bit_aligned_pixel_reference<B, C, L, M>>
{
    using type = typename L::channel_mapping_t;
};

template <typename B, typename C, typename L, bool M>
struct is_planar<bit_aligned_pixel_reference<B, C, L, M>> : std::false_type {};

/////////////////////////////
//  pixel_reference_type
/////////////////////////////

// Constructs a homogeneous bit_aligned_pixel_reference given a channel reference
template <typename BitField, int NumBits, typename Layout>
struct pixel_reference_type
    <
        packed_dynamic_channel_reference<BitField, NumBits, false> const,
        Layout, false, false
    >
{
private:
    using channel_bit_sizes_t = mp11::mp_repeat
        <
            mp11::mp_list<std::integral_constant<unsigned, NumBits>>,
            mp11::mp_size<typename Layout::color_space_t>
        >;

public:
    using type =
        bit_aligned_pixel_reference<BitField, channel_bit_sizes_t, Layout, false>;
};

// Same but for the mutable case. We cannot combine the mutable
// and read-only cases because this triggers ambiguity
template <typename BitField, int NumBits, typename Layout>
struct pixel_reference_type
    <
        packed_dynamic_channel_reference<BitField, NumBits, true> const,
        Layout, false, true
    >
{
private:
    using channel_bit_sizes_t = mp11::mp_repeat
        <
            mp11::mp_list<std::integral_constant<unsigned, NumBits>>,
            mp11::mp_size<typename Layout::color_space_t>
        >;

public:
    using type = bit_aligned_pixel_reference<BitField, channel_bit_sizes_t, Layout, true>;
};

} }  // namespace boost::gil

namespace std {

// We are forced to define swap inside std namespace because on some platforms (Visual Studio 8) STL calls swap qualified.
// swap with 'left bias':
// - swap between proxy and anything
// - swap between value type and proxy
// - swap between proxy and proxy
// Having three overloads allows us to swap between different (but compatible) models of PixelConcept

template <typename B, typename C, typename L, typename R> inline
void swap(const boost::gil::bit_aligned_pixel_reference<B,C,L,true> x, R& y) {
    boost::gil::swap_proxy<typename boost::gil::bit_aligned_pixel_reference<B,C,L,true>::value_type>(x,y);
}


template <typename B, typename C, typename L> inline
void swap(typename boost::gil::bit_aligned_pixel_reference<B,C,L,true>::value_type& x, const boost::gil::bit_aligned_pixel_reference<B,C,L,true> y) {
    boost::gil::swap_proxy<typename boost::gil::bit_aligned_pixel_reference<B,C,L,true>::value_type>(x,y);
}


template <typename B, typename C, typename L> inline
void swap(const boost::gil::bit_aligned_pixel_reference<B,C,L,true> x, const boost::gil::bit_aligned_pixel_reference<B,C,L,true> y) {
    boost::gil::swap_proxy<typename boost::gil::bit_aligned_pixel_reference<B,C,L,true>::value_type>(x,y);
}

} // namespace std

#endif

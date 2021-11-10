//
// Copyright 2005-2007 Adobe Systems Incorporated
// Copyright 2019 Mateusz Loskot <mateusz at loskot dot net>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_PIXEL_HPP
#define BOOST_GIL_PIXEL_HPP

#include <boost/gil/channel.hpp>
#include <boost/gil/color_base.hpp>
#include <boost/gil/color_base_algorithm.hpp>
#include <boost/gil/concepts.hpp>
#include <boost/gil/metafunctions.hpp>
#include <boost/gil/utilities.hpp>
#include <boost/gil/detail/mp11.hpp>

#include <functional>
#include <type_traits>

namespace boost { namespace gil {

// Forward-declare gray_t
struct gray_color_t;
using gray_t = mp11::mp_list<gray_color_t>;
template <typename PixelBased> struct color_space_type;
template <typename PixelBased> struct channel_mapping_type;
template <typename PixelBased> struct channel_type;
template <typename PixelBased> struct is_planar;

template <typename PixelBased>
struct color_space_type<PixelBased const> : color_space_type<PixelBased> {};

template <typename PixelBased>
struct channel_mapping_type<PixelBased const> : channel_mapping_type<PixelBased> {};

template <typename PixelBased>
struct channel_type<PixelBased const> : channel_type<PixelBased> {};

template <typename PixelBased>
struct is_planar : std::false_type {};

template <typename PixelBased>
struct is_planar<PixelBased const> : is_planar<PixelBased> {};

template <typename T> struct is_pixel : std::false_type {};
template <typename T> struct is_pixel<T const> : is_pixel<T> {};

/// \ingroup PixelBasedAlgorithm
/// \brief Returns the number of channels of a pixel-based GIL construct
template <typename PixelBased>
struct num_channels : mp11::mp_size<typename color_space_type<PixelBased>::type>::type {};

/**
\addtogroup PixelBasedAlgorithm

Example:
\code
static_assert(num_channels<rgb8_view_t>::value == 3, "");
static_assert(num_channels<cmyk16_planar_ptr_t>::value == 4, "");

static_assert(is_planar<rgb16_planar_image_t>::value));
static_assert(std::is_same<color_space_type<rgb8_planar_ref_t>::type, rgb_t>::value, "");
static_assert(std::is_same<channel_mapping_type<cmyk8_pixel_t>::type,
                             channel_mapping_type<rgba8_pixel_t>::type>::value, "");
static_assert(std::is_same<channel_type<bgr8_pixel_t>::type, uint8_t>::value, "");
\endcode
*/

/// \defgroup ColorBaseModelPixel pixel
/// \ingroup ColorBaseModel
/// \brief A homogeneous color base whose element is a channel value. Models HomogeneousColorBaseValueConcept

/// \defgroup PixelModelPixel pixel
/// \ingroup PixelModel
/// \brief A homogeneous pixel value. Models HomogeneousPixelValueConcept

/// \ingroup PixelModelPixel ColorBaseModelPixel PixelBasedModel
/// \brief Represents a pixel value (a container of channels). Models: HomogeneousColorBaseValueConcept, PixelValueConcept, HomogeneousPixelBasedConcept
///
/// A pixel is a set of channels defining the color at a given point in an image. Conceptually, a pixel is little more than a color base whose elements
/// model \p ChannelConcept. The class \p pixel defines a simple, homogeneous pixel value. It is used to store
/// the value of a color. The built-in C++ references to \p pixel, \p pixel& and \p const \p pixel& are used to represent a reference to a pixel
/// inside an interleaved image view (a view in which all channels are together in memory). Similarly, built-in pointer types \p pixel* and \p const \p pixel*
/// are used as the standard iterator over a row of interleaved homogeneous pixels.
///
/// Since \p pixel inherits the properties of color base, assigning, equality comparison and copy-construcion are allowed between compatible pixels.
/// This means that an 8-bit RGB pixel may be assigned to an 8-bit BGR pixel, or to an 8-bit planar reference. The channels are properly paired semantically.
///
/// The single-channel (grayscale) instantiation of the class pixel, (i.e. \p pixel<T,gray_layout_t>) is also convertible to/from a channel value.
/// This allows grayscale pixels to be used in simpler expressions like *gray_pix1 = *gray_pix2  instead of more complicated at_c<0>(gray_pix1) = at_c<0>(gray_pix2)
/// or get_color<gray_color_t>(gray_pix1) = get_color<gray_color_t>(gray_pix2)
///
/// \tparam ChannelValue TODO
/// \tparam Layout mp11::make_integer_sequence<int, ColorSpace::size>
template <typename ChannelValue, typename Layout>
struct pixel :
    detail::homogeneous_color_base
    <
        ChannelValue,
        Layout,
        mp11::mp_size<typename Layout::color_space_t>::value
    >
{
private:
    using channel_t = ChannelValue;
    using parent_t = detail::homogeneous_color_base
        <
            ChannelValue,
            Layout,
            mp11::mp_size<typename Layout::color_space_t>::value
        >;
public:
    using value_type = pixel<ChannelValue, Layout>;
    using reference = value_type&;
    using const_reference = value_type const&;
    static constexpr bool is_mutable = channel_traits<channel_t>::is_mutable;

    pixel() = default;
    explicit pixel(channel_t v) : parent_t(v) {}  // sets all channels to v
    pixel(channel_t v0, channel_t v1) : parent_t(v0, v1) {}
    pixel(channel_t v0, channel_t v1, channel_t v2) : parent_t(v0, v1, v2) {}

    pixel(channel_t v0, channel_t v1, channel_t v2, channel_t v3)
        : parent_t(v0, v1, v2, v3)
    {}

    pixel(channel_t v0, channel_t v1, channel_t v2, channel_t v3, channel_t v4)
        : parent_t(v0, v1, v2, v3, v4)
    {}

    pixel(channel_t v0, channel_t v1, channel_t v2, channel_t v3, channel_t v4, channel_t v5)
        : parent_t(v0, v1, v2, v3, v4, v5)
    {}

    pixel(const pixel& p) : parent_t(p) {}

    pixel& operator=(pixel const& p)
    {
        static_copy(p,*this);
        return *this;
    }

    // Construct from another compatible pixel type
    template <typename Pixel>
    pixel(Pixel const& p,
        typename std::enable_if<is_pixel<Pixel>::value>::type* /*dummy*/ = nullptr)
        : parent_t(p)
    {
        check_compatible<Pixel>();
    }

    template <typename Pixel>
    pixel& operator=(Pixel const& p)
    {
        assign(p, is_pixel<Pixel>());
        return *this;
    }

    template <typename Pixel>
    bool operator==(Pixel const& p) const { return equal(p, is_pixel<Pixel>()); }

    template <typename Pixel>
    bool operator!=(Pixel const& p) const { return !(*this == p); }

    // homogeneous pixels have operator[]
    auto operator[](std::size_t index)
        -> typename channel_traits<channel_t>::reference
    {
        return dynamic_at_c(*this, index);
    }

    auto operator[](std::size_t index) const
        -> typename channel_traits<channel_t>::const_reference
    {
        return dynamic_at_c(*this, index);
    }

private:
    template <typename Pixel>
    void assign(Pixel const& p, std::true_type)
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

    template <typename Pixel>
    void check_compatible() const
    {
        gil_function_requires<PixelsCompatibleConcept<Pixel, pixel>>();
    }

// Support for assignment/equality comparison of a channel with a grayscale pixel

private:
    static void check_gray()
    {
        static_assert(std::is_same<typename Layout::color_space_t, gray_t>::value, "");
    }

    template <typename Channel>
    void assign(Channel const& channel, std::false_type)
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

public:
    pixel& operator= (channel_t channel)
    {
        check_gray();
        gil::at_c<0>(*this) = channel;
        return *this;
    }

    bool operator==(channel_t channel) const
    {
        check_gray();
        return gil::at_c<0>(*this) == channel;
    }
};

/////////////////////////////
//  ColorBasedConcept
/////////////////////////////

template <typename ChannelValue, typename Layout, int K>
struct kth_element_type<pixel<ChannelValue,Layout>, K>
{
    using type = ChannelValue;
};

template <typename ChannelValue, typename Layout, int K>
struct kth_element_reference_type<pixel<ChannelValue,Layout>, K>
{
    using type = typename channel_traits<ChannelValue>::reference;
};

template <typename ChannelValue, typename Layout, int K>
struct kth_element_reference_type<const pixel<ChannelValue,Layout>, K>
{
    using type = typename channel_traits<ChannelValue>::const_reference;
};

template <typename ChannelValue, typename Layout, int K>
struct kth_element_const_reference_type<pixel<ChannelValue,Layout>, K>
{
    using type = typename channel_traits<ChannelValue>::const_reference;
};

/////////////////////////////
//  PixelConcept
/////////////////////////////

template <typename ChannelValue, typename Layout>
struct is_pixel<pixel<ChannelValue,Layout>> : std::true_type {};

/////////////////////////////
//  HomogeneousPixelBasedConcept
/////////////////////////////

template <typename ChannelValue, typename Layout>
struct color_space_type<pixel<ChannelValue, Layout>>
{
    using type = typename Layout::color_space_t;
};

template <typename ChannelValue, typename Layout>
struct channel_mapping_type<pixel<ChannelValue, Layout>>
{
    using type = typename Layout::channel_mapping_t;
};

template <typename ChannelValue, typename Layout>
struct is_planar<pixel<ChannelValue, Layout>> : std::false_type {};

template <typename ChannelValue, typename Layout>
struct channel_type<pixel<ChannelValue, Layout>>
{
    using type = ChannelValue;
};

}}  // namespace boost::gil

#endif

//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_METAFUNCTIONS_HPP
#define BOOST_GIL_METAFUNCTIONS_HPP

#include <boost/gil/channel.hpp>
#include <boost/gil/dynamic_step.hpp>
#include <boost/gil/concepts.hpp>
#include <boost/gil/concepts/detail/type_traits.hpp>
#include <boost/gil/detail/mp11.hpp>

#include <iterator>
#include <type_traits>

namespace boost { namespace gil {

// forward declarations
template <typename T, typename L> struct pixel;
template <typename BitField,typename ChannelRefs,typename Layout> struct packed_pixel;
template <typename T, typename C> struct planar_pixel_reference;
template <typename IC, typename C> struct planar_pixel_iterator;
template <typename I> class memory_based_step_iterator;
template <typename I> class memory_based_2d_locator;
template <typename L> class image_view;
template <typename Pixel, bool IsPlanar, typename Alloc> class image;
template <typename T> struct channel_type;
template <typename T> struct color_space_type;
template <typename T> struct channel_mapping_type;
template <typename It> struct is_iterator_adaptor;
template <typename It> struct iterator_adaptor_get_base;
template <typename BitField, typename ChannelBitSizes, typename Layout, bool IsMutable> struct bit_aligned_pixel_reference;

//////////////////////////////////////////////////
///
///  TYPE ANALYSIS METAFUNCTIONS
///  Predicate metafunctions determining properties of GIL types
///
//////////////////////////////////////////////////


/// \defgroup GILIsBasic xxx_is_basic
/// \ingroup TypeAnalysis
/// \brief Determines if GIL constructs are basic.
///    Basic constructs are the ones that can be generated with the type
///    factory methods pixel_reference_type, iterator_type, locator_type, view_type and image_type
///    They can be mutable/immutable, planar/interleaved, step/nonstep. They must use GIL-provided models.

/// \brief Determines if a given pixel reference is basic
///    Basic references must use gil::pixel& (if interleaved), gil::planar_pixel_reference (if planar). They must use the standard constness rules.
/// \ingroup GILIsBasic
template <typename PixelRef>
struct pixel_reference_is_basic : public std::false_type {};

template <typename T, typename L>
struct pixel_reference_is_basic<pixel<T, L>&> : std::true_type {};

template <typename T, typename L>
struct pixel_reference_is_basic<const pixel<T, L>&> : std::true_type {};

template <typename TR, typename CS>
struct pixel_reference_is_basic<planar_pixel_reference<TR, CS>> : std::true_type {};

template <typename TR, typename CS>
struct pixel_reference_is_basic<const planar_pixel_reference<TR, CS>> : std::true_type {};

/// \brief Determines if a given pixel iterator is basic
///    Basic iterators must use gil::pixel (if interleaved), gil::planar_pixel_iterator (if planar) and gil::memory_based_step_iterator (if step). They must use the standard constness rules.
/// \ingroup GILIsBasic
template <typename Iterator>
struct iterator_is_basic : std::false_type {};

/// \tparam T mutable interleaved pixel type
template <typename T, typename L>
struct iterator_is_basic<pixel<T, L>*> : std::true_type {};

/// \tparam T immutable interleaved pixel type
template <typename T, typename L>
struct iterator_is_basic<pixel<T, L> const*> : std::true_type {};

/// \tparam T mutable planar pixel type
template <typename T, typename CS>
struct iterator_is_basic<planar_pixel_iterator<T*, CS>> : std::true_type {};

/// \tparam T immutable planar pixel type
template <typename T, typename CS>
struct iterator_is_basic<planar_pixel_iterator<T const*, CS>> : std::true_type {};

/// \tparam T mutable interleaved step
template <typename T, typename L>
struct iterator_is_basic<memory_based_step_iterator<pixel<T, L>*>> : std::true_type {};

/// \tparam T immutable interleaved step
template <typename T, typename L>
struct iterator_is_basic<memory_based_step_iterator<pixel<T, L> const*>> : std::true_type {};

/// \tparam T mutable planar step
template <typename T, typename CS>
struct iterator_is_basic<memory_based_step_iterator<planar_pixel_iterator<T*, CS>>>
    : std::true_type
{};

/// \tparam T immutable planar step
template <typename T, typename CS>
struct iterator_is_basic<memory_based_step_iterator<planar_pixel_iterator<T const*, CS>>>
    : std::true_type
{};


/// \ingroup GILIsBasic
/// \brief Determines if a given locator is basic. A basic locator is memory-based and has basic x_iterator and y_iterator
template <typename Loc>
struct locator_is_basic : std::false_type {};

template <typename Iterator>
struct locator_is_basic
    <
        memory_based_2d_locator<memory_based_step_iterator<Iterator>>
    > : iterator_is_basic<Iterator>
{};

/// \ingroup GILIsBasic
/// \brief Basic views must be over basic locators
template <typename View>
struct view_is_basic : std::false_type {};

template <typename Loc>
struct view_is_basic<image_view<Loc>> : locator_is_basic<Loc> {};

/// \ingroup GILIsBasic
/// \brief Basic images must use basic views and std::allocator
template <typename Img>
struct image_is_basic : std::false_type {};

template <typename Pixel, bool IsPlanar, typename Alloc>
struct image_is_basic<image<Pixel, IsPlanar, Alloc>> : std::true_type {};


/// \defgroup GILIsStep xxx_is_step
/// \ingroup TypeAnalysis
/// \brief Determines if the given iterator/locator/view has a step that could be set dynamically

template <typename I>
struct iterator_is_step;

namespace detail {

template <typename It, bool IsBase, bool EqualsStepType>
struct iterator_is_step_impl;

// iterator that has the same type as its dynamic_x_step_type must be a step iterator
template <typename It, bool IsBase>
struct iterator_is_step_impl<It, IsBase, true> : std::true_type {};

// base iterator can never be a step iterator
template <typename It>
struct iterator_is_step_impl<It, true, false> : std::false_type {};

// for an iterator adaptor, see if its base is step
template <typename It>
struct iterator_is_step_impl<It, false, false>
    : public iterator_is_step<typename iterator_adaptor_get_base<It>::type> {};

} // namespace detail

/// \ingroup GILIsStep
/// \brief Determines if the given iterator has a step that could be set dynamically
template <typename I>
struct iterator_is_step
    : detail::iterator_is_step_impl
    <
        I,
        !is_iterator_adaptor<I>::value,
        std::is_same<I, typename dynamic_x_step_type<I>::type
    >::value
>
{};

/// \ingroup GILIsStep
/// \brief Determines if the given locator has a horizontal step that could be set dynamically
template <typename L> struct locator_is_step_in_x : public iterator_is_step<typename L::x_iterator> {};

/// \ingroup GILIsStep
/// \brief Determines if the given locator has a vertical step that could be set dynamically
template <typename L> struct locator_is_step_in_y : public iterator_is_step<typename L::y_iterator> {};

/// \ingroup GILIsStep
/// \brief Determines if the given view has a horizontal step that could be set dynamically
template <typename V> struct view_is_step_in_x : public locator_is_step_in_x<typename V::xy_locator> {};

/// \ingroup GILIsStep
/// \brief Determines if the given view has a vertical step that could be set dynamically
template <typename V> struct view_is_step_in_y : public locator_is_step_in_y<typename V::xy_locator> {};

/// \brief Determines whether the given pixel reference is a proxy class or a native C++ reference
/// \ingroup TypeAnalysis
template <typename PixelReference>
struct pixel_reference_is_proxy
    : mp11::mp_not
    <
        std::is_same
        <
            typename detail::remove_const_and_reference<PixelReference>::type,
            typename detail::remove_const_and_reference<PixelReference>::type::value_type
        >
    >
{};

/// \brief Given a model of a pixel, determines whether the model represents a pixel reference (as opposed to pixel value)
/// \ingroup TypeAnalysis
template <typename Pixel>
struct pixel_is_reference
    : mp11::mp_or<is_reference<Pixel>, pixel_reference_is_proxy<Pixel>> {};

/// \defgroup GILIsMutable xxx_is_mutable
/// \ingroup TypeAnalysis
/// \brief Determines if the given pixel reference/iterator/locator/view is mutable (i.e. its pixels can be changed)

/// \ingroup GILIsMutable
/// \brief Determines if the given pixel reference is mutable (i.e. its channels can be changed)
///
/// Note that built-in C++ references obey the const qualifier but reference proxy classes do not.
template <typename R>
struct pixel_reference_is_mutable
    : std::integral_constant<bool, std::remove_reference<R>::type::is_mutable>
{};

template <typename R>
struct pixel_reference_is_mutable<R const&>
    : mp11::mp_and<pixel_reference_is_proxy<R>, pixel_reference_is_mutable<R>>
{};

/// \ingroup GILIsMutable
/// \brief Determines if the given locator is mutable (i.e. its pixels can be changed)
template <typename L> struct locator_is_mutable : public iterator_is_mutable<typename L::x_iterator> {};
/// \ingroup GILIsMutable
/// \brief Determines if the given view is mutable (i.e. its pixels can be changed)
template <typename V> struct view_is_mutable : public iterator_is_mutable<typename V::x_iterator> {};


//////////////////////////////////////////////////
///
///  TYPE FACTORY METAFUNCTIONS
///  Metafunctions returning GIL types from other GIL types
///
//////////////////////////////////////////////////

/// \defgroup TypeFactoryFromElements xxx_type
/// \ingroup TypeFactory
/// \brief Returns the type of a homogeneous GIL construct given its elements (channel, layout, whether it is planar, step, mutable, etc.)

/// \defgroup TypeFactoryFromPixel xxx_type_from_pixel
/// \ingroup TypeFactory
/// \brief Returns the type of a GIL construct given its pixel type, whether it is planar, step, mutable, etc.

/// \defgroup TypeFactoryDerived derived_xxx_type
/// \ingroup TypeFactory
/// \brief Returns the type of a homogeneous GIL construct given a related construct by changing some of its properties

/// \ingroup TypeFactoryFromElements
/// \brief Returns the type of a homogeneous pixel reference given the channel type, layout, whether it operates on planar data and whether it is mutable
template <typename T, typename L, bool IsPlanar=false, bool IsMutable=true> struct pixel_reference_type{};
template <typename T, typename L> struct pixel_reference_type<T,L,false,true > { using type = pixel<T,L>&; };
template <typename T, typename L> struct pixel_reference_type<T,L,false,false> { using type = pixel<T,L> const&; };
template <typename T, typename L> struct pixel_reference_type<T,L,true,true> { using type = planar_pixel_reference<typename channel_traits<T>::reference,typename color_space_type<L>::type> const; };       // TODO: Assert M=identity
template <typename T, typename L> struct pixel_reference_type<T,L,true,false> { using type = planar_pixel_reference<typename channel_traits<T>::const_reference,typename color_space_type<L>::type> const; };// TODO: Assert M=identity

/// \ingroup TypeFactoryFromPixel
/// \brief Returns the type of a pixel iterator given the pixel type, whether it operates on planar data, whether it is a step iterator, and whether it is mutable
template <typename Pixel, bool IsPlanar=false, bool IsStep=false, bool IsMutable=true> struct iterator_type_from_pixel{};
template <typename Pixel> struct iterator_type_from_pixel<Pixel,false,false,true > { using type = Pixel *; };
template <typename Pixel> struct iterator_type_from_pixel<Pixel,false,false,false> { using type = const Pixel *; };
template <typename Pixel> struct iterator_type_from_pixel<Pixel,true,false,true> {
    using type = planar_pixel_iterator<typename channel_traits<typename channel_type<Pixel>::type>::pointer,typename color_space_type<Pixel>::type>;
};
template <typename Pixel> struct iterator_type_from_pixel<Pixel,true,false,false> {
    using type = planar_pixel_iterator<typename channel_traits<typename channel_type<Pixel>::type>::const_pointer,typename color_space_type<Pixel>::type>;
};
template <typename Pixel, bool IsPlanar, bool IsMutable> struct iterator_type_from_pixel<Pixel,IsPlanar,true,IsMutable> {
    using type = memory_based_step_iterator<typename iterator_type_from_pixel<Pixel,IsPlanar,false,IsMutable>::type>;
};

/// \ingroup TypeFactoryFromElements
/// \brief Returns the type of a homogeneous iterator given the channel type, layout, whether it operates on planar data, whether it is a step iterator, and whether it is mutable
template <typename T, typename L, bool IsPlanar=false, bool IsStep=false, bool IsMutable=true> struct iterator_type{};
template <typename T, typename L> struct iterator_type<T,L,false,false,true > { using type = pixel<T,L>*; };
template <typename T, typename L> struct iterator_type<T,L,false,false,false> { using type = pixel<T,L> const*; };
template <typename T, typename L> struct iterator_type<T,L,true,false,true> { using type = planar_pixel_iterator<T*,typename L::color_space_t>; };               // TODO: Assert M=identity
template <typename T, typename L> struct iterator_type<T,L,true,false,false> { using type = planar_pixel_iterator<const T*,typename L::color_space_t>; };        // TODO: Assert M=identity
template <typename T, typename L, bool IsPlanar, bool IsMutable> struct iterator_type<T,L,IsPlanar,true,IsMutable> {
    using type = memory_based_step_iterator<typename iterator_type<T,L,IsPlanar,false,IsMutable>::type>;
};

/// \brief Given a pixel iterator defining access to pixels along a row, returns the types of the corresponding built-in step_iterator, xy_locator, image_view
/// \ingroup TypeFactory
template <typename XIterator>
struct type_from_x_iterator
{
    using step_iterator_t = memory_based_step_iterator<XIterator>;
    using xy_locator_t = memory_based_2d_locator<step_iterator_t>;
    using view_t = image_view<xy_locator_t>;
};

namespace detail {

template <typename BitField, typename FirstBit, typename NumBits>
struct packed_channel_reference_type
{
    using type = packed_channel_reference
        <
            BitField, FirstBit::value, NumBits::value, true
        > const;
};

template <typename BitField, typename ChannelBitSizes>
class packed_channel_references_vector_type
{
    template <typename FirstBit, typename NumBits>
    using reference_type = typename packed_channel_reference_type<BitField, FirstBit, NumBits>::type;

    // If ChannelBitSizesVector is mp11::mp_list_c<int,7,7,2>
    // Then first_bits_vector will be mp11::mp_list_c<int,0,7,14,16>
    using first_bit_list = mp11::mp_fold_q
        <
            ChannelBitSizes,
            mp11::mp_list<std::integral_constant<int, 0>>,
            mp11::mp_bind
            <
                mp11::mp_push_back,
                mp11::_1,
                mp11::mp_bind
                <
                    mp11::mp_plus,
                    mp11::mp_bind<mp_back, mp11::_1>,
                    mp11::_2
                >
            >
        >;

    static_assert(mp11::mp_at_c<first_bit_list, 0>::value == 0, "packed channel first bit must be 0");

public:
    using type = mp11::mp_transform
        <
            reference_type,
            mp_pop_back<first_bit_list>,
            ChannelBitSizes
        >;
};

} // namespace detail

/// \ingroup TypeFactoryFromElements
/// \brief Returns the type of a packed pixel given its bitfield type, the bit size of its channels and its layout.
///
/// A packed pixel has channels that cover bit ranges but itself is byte aligned. RGB565 pixel is an example.
///
/// The size of ChannelBitSizes must equal the number of channels in the given layout
/// The sum of bit sizes for all channels must be less than or equal to the number of bits in BitField (and cannot exceed 64).
///  If it is less than the number of bits in BitField, the last bits will be unused.
template <typename BitField, typename ChannelBitSizes, typename Layout>
struct packed_pixel_type
{
    using type = packed_pixel
        <
            BitField,
            typename detail::packed_channel_references_vector_type
            <
                BitField,
                ChannelBitSizes
            >::type,
        Layout>;
};

/// \defgroup TypeFactoryPacked packed_image_type,bit_aligned_image_type
/// \ingroup TypeFactoryFromElements
/// \brief Returns the type of an image whose channels are not byte-aligned.
///
/// A packed image is an image whose pixels are byte aligned, such as "rgb565". <br>
/// A bit-aligned image is an image whose pixels are not byte aligned, such as "rgb222". <br>
///
/// The sum of the bit sizes of all channels cannot exceed 64.

/// \ingroup TypeFactoryPacked
/// \brief Returns the type of an interleaved packed image: an image whose channels may not be byte-aligned, but whose pixels are byte aligned.
template <typename BitField, typename ChannelBitSizes, typename Layout, typename Alloc=std::allocator<unsigned char>>
struct packed_image_type
{
    using type = image<typename packed_pixel_type<BitField,ChannelBitSizes,Layout>::type,false,Alloc>;
};

/// \ingroup TypeFactoryPacked
/// \brief Returns the type of a single-channel image given its bitfield type, the bit size of its channel and its layout
template <typename BitField, unsigned Size1, typename Layout, typename Alloc = std::allocator<unsigned char>>
struct packed_image1_type
    : packed_image_type<BitField, mp11::mp_list_c<unsigned, Size1>, Layout, Alloc>
{};

/// \ingroup TypeFactoryPacked
/// \brief Returns the type of a two channel image given its bitfield type, the bit size of its channels and its layout
template <typename BitField, unsigned Size1, unsigned Size2, typename Layout, typename Alloc = std::allocator<unsigned char>>
struct packed_image2_type
    : packed_image_type<BitField, mp11::mp_list_c<unsigned, Size1, Size2>, Layout, Alloc>
{};

/// \ingroup TypeFactoryPacked
/// \brief Returns the type of a three channel image given its bitfield type, the bit size of its channels and its layout
template <typename BitField, unsigned Size1, unsigned Size2, unsigned Size3, typename Layout, typename Alloc = std::allocator<unsigned char>>
struct packed_image3_type
    : packed_image_type<BitField, mp11::mp_list_c<unsigned, Size1, Size2, Size3>, Layout, Alloc>
{};

/// \ingroup TypeFactoryPacked
/// \brief Returns the type of a four channel image given its bitfield type, the bit size of its channels and its layout
template <typename BitField, unsigned Size1, unsigned Size2, unsigned Size3, unsigned Size4, typename Layout, typename Alloc = std::allocator<unsigned char>>
struct packed_image4_type
    : packed_image_type<BitField, mp11::mp_list_c<unsigned, Size1, Size2, Size3, Size4>, Layout, Alloc>
{};

/// \ingroup TypeFactoryPacked
/// \brief Returns the type of a five channel image given its bitfield type, the bit size of its channels and its layout
template <typename BitField, unsigned Size1, unsigned Size2, unsigned Size3, unsigned Size4, unsigned Size5, typename Layout, typename Alloc = std::allocator<unsigned char>>
struct packed_image5_type
    : packed_image_type<BitField, mp11::mp_list_c<unsigned, Size1, Size2, Size3, Size4, Size5>, Layout, Alloc> {};


/// \ingroup TypeFactoryPacked
/// \brief Returns the type of a packed image whose pixels may not be byte aligned. For example, an "rgb222" image is bit-aligned because its pixel spans six bits.
///
/// Note that the alignment parameter in the constructor of bit-aligned images is in bit units. For example, if you want to construct a bit-aligned
/// image whose rows are byte-aligned, use 8 as the alignment parameter, not 1.
///
template
<
    typename ChannelBitSizes,
    typename Layout,
    typename Alloc = std::allocator<unsigned char>
>
struct bit_aligned_image_type
{
private:

    static constexpr int bit_size =
        mp11::mp_fold
        <
            ChannelBitSizes,
            std::integral_constant<int, 0>,
            mp11::mp_plus
        >::value;

    using bitfield_t = typename detail::min_fast_uint<bit_size + 7>::type;
    using bit_alignedref_t = bit_aligned_pixel_reference<bitfield_t, ChannelBitSizes, Layout, true> const;

public:
    using type = image<bit_alignedref_t,false,Alloc>;
};

/// \ingroup TypeFactoryPacked
/// \brief Returns the type of a single-channel bit-aligned image given the bit size of its channel and its layout
template <unsigned Size1, typename Layout, typename Alloc = std::allocator<unsigned char>>
struct bit_aligned_image1_type : bit_aligned_image_type<mp11::mp_list_c<unsigned, Size1>, Layout, Alloc> {};

/// \ingroup TypeFactoryPacked
/// \brief Returns the type of a two channel bit-aligned image given the bit size of its channels and its layout
template <unsigned Size1, unsigned Size2, typename Layout, typename Alloc = std::allocator<unsigned char>>
struct bit_aligned_image2_type : bit_aligned_image_type<mp11::mp_list_c<unsigned, Size1, Size2>, Layout, Alloc> {};

/// \ingroup TypeFactoryPacked
/// \brief Returns the type of a three channel bit-aligned image given the bit size of its channels and its layout
template <unsigned Size1, unsigned Size2, unsigned Size3, typename Layout, typename Alloc = std::allocator<unsigned char>>
struct bit_aligned_image3_type : bit_aligned_image_type<mp11::mp_list_c<unsigned, Size1, Size2, Size3>, Layout, Alloc> {};

/// \ingroup TypeFactoryPacked
/// \brief Returns the type of a four channel bit-aligned image given the bit size of its channels and its layout
template <unsigned Size1, unsigned Size2, unsigned Size3, unsigned Size4, typename Layout, typename Alloc = std::allocator<unsigned char>>
struct bit_aligned_image4_type : bit_aligned_image_type<mp11::mp_list_c<unsigned, Size1, Size2, Size3, Size4>, Layout, Alloc> {};

/// \ingroup TypeFactoryPacked
/// \brief Returns the type of a five channel bit-aligned image given the bit size of its channels and its layout
template <unsigned Size1, unsigned Size2, unsigned Size3, unsigned Size4, unsigned Size5, typename Layout, typename Alloc = std::allocator<unsigned char>>
struct bit_aligned_image5_type : bit_aligned_image_type<mp11::mp_list_c<unsigned, Size1, Size2, Size3, Size4, Size5>, Layout, Alloc> {};


/// \ingroup TypeFactoryFromElements
/// \brief Returns the type of a homogeneous pixel given the channel type and layout
template <typename Channel, typename Layout>
struct pixel_value_type
{
    // by default use gil::pixel. Specializations are provided for
    using type = pixel<Channel, Layout>;
};

// Specializations for packed channels
template <typename BitField, int NumBits, bool IsMutable, typename Layout>
struct pixel_value_type<packed_dynamic_channel_reference<BitField, NumBits, IsMutable>, Layout>
    : packed_pixel_type<BitField, mp11::mp_list_c<unsigned, NumBits>, Layout>
{};

template <typename BitField, int NumBits, bool IsMutable, typename Layout>
struct pixel_value_type<packed_dynamic_channel_reference<BitField, NumBits, IsMutable> const, Layout>
    : packed_pixel_type<BitField, mp11::mp_list_c<unsigned, NumBits>, Layout>
{};

template <typename BitField, int FirstBit, int NumBits, bool IsMutable, typename Layout>
struct pixel_value_type<packed_channel_reference<BitField, FirstBit, NumBits, IsMutable>, Layout>
    : packed_pixel_type<BitField, mp11::mp_list_c<unsigned, NumBits>, Layout>
{};

template <typename BitField, int FirstBit, int NumBits, bool IsMutable, typename Layout>
struct pixel_value_type<packed_channel_reference<BitField, FirstBit, NumBits, IsMutable> const, Layout>
    : packed_pixel_type<BitField, mp11::mp_list_c<unsigned, NumBits>, Layout>
{};

template <int NumBits, typename Layout>
struct pixel_value_type<packed_channel_value<NumBits>, Layout>
    : packed_pixel_type<typename detail::min_fast_uint<NumBits>::type, mp11::mp_list_c<unsigned, NumBits>, Layout>
{};

/// \ingroup TypeFactoryFromElements
/// \brief Returns the type of a homogeneous locator given the channel type, layout, whether it operates on planar data and whether it has a step horizontally
template <typename T, typename L, bool IsPlanar = false, bool IsStepX = false, bool IsMutable = true>
struct locator_type
{
    using type = typename type_from_x_iterator
        <
            typename iterator_type<T, L, IsPlanar, IsStepX, IsMutable>::type
        >::xy_locator_type;
};

/// \ingroup TypeFactoryFromElements
/// \brief Returns the type of a homogeneous view given the channel type, layout, whether it operates on planar data and whether it has a step horizontally
template <typename T, typename L, bool IsPlanar = false, bool IsStepX = false, bool IsMutable = true>
struct view_type
{
    using type = typename type_from_x_iterator
        <
            typename iterator_type<T, L, IsPlanar, IsStepX, IsMutable>::type
        >::view_t;
};

/// \ingroup TypeFactoryFromElements
/// \brief Returns the type of a homogeneous image given the channel type, layout, and whether it operates on planar data
template <typename T, typename L, bool IsPlanar = false, typename Alloc = std::allocator<unsigned char>>
struct image_type
{
    using type = image<pixel<T, L>, IsPlanar, Alloc>;
};

/// \ingroup TypeFactoryFromPixel
/// \brief Returns the type of a view the pixel type, whether it operates on planar data and whether it has a step horizontally
template <typename Pixel, bool IsPlanar=false, bool IsStepX=false, bool IsMutable=true>
struct view_type_from_pixel {
    using type = typename type_from_x_iterator<typename iterator_type_from_pixel<Pixel,IsPlanar,IsStepX,IsMutable>::type>::view_t;
};


/// \brief Constructs a pixel reference type from a source pixel reference type by changing some of the properties.
/// \ingroup TypeFactoryDerived
///  Use use_default for the properties of the source view that you want to keep
template
<
        typename Ref,
        typename T = use_default,
        typename L = use_default,
        typename IsPlanar = use_default,
        typename IsMutable = use_default>
class derived_pixel_reference_type
{
    using pixel_t = typename std::remove_reference<Ref>::type;

    using channel_t = typename mp11::mp_if
        <
            std::is_same<T, use_default>,
            typename channel_type<pixel_t>::type,
            T
        >::type;

    using layout_t = typename  mp11::mp_if
        <
            std::is_same<L, use_default>,
            layout
            <
                typename color_space_type<pixel_t>::type,
                typename channel_mapping_type<pixel_t>::type
            >,
            L
        >::type;

    static bool const mut = mp11::mp_if
        <
            std::is_same<IsMutable, use_default>,
            pixel_reference_is_mutable<Ref>,
            IsMutable
        >::value;

    static bool const planar = mp11::mp_if
        <
            std::is_same<IsPlanar, use_default>,
            is_planar<pixel_t>,
            IsPlanar
        >::value;

public:
    using type = typename pixel_reference_type<channel_t, layout_t, planar, mut>::type;
};

/// \brief Constructs a pixel iterator type from a source pixel iterator type by changing some of the properties.
/// \ingroup TypeFactoryDerived
///  Use use_default for the properties of the source view that you want to keep
template
<
    typename Iterator,
    typename T = use_default,
    typename L = use_default,
    typename IsPlanar = use_default,
    typename IsStep = use_default,
    typename IsMutable = use_default
>
class derived_iterator_type
{
    using channel_t = typename mp11::mp_if
        <
            std::is_same<T, use_default>,
            typename channel_type<Iterator>::type,
            T
        >::type;

    using layout_t = typename mp11::mp_if
        <
            std::is_same<L, use_default>,
            layout
            <
                typename color_space_type<Iterator>::type,
                typename channel_mapping_type<Iterator>::type
            >,
            L
        >::type;

    static const bool mut = mp11::mp_if
        <
            std::is_same<IsMutable, use_default>,
            iterator_is_mutable<Iterator>,
            IsMutable
        >::value;

    static bool const planar = mp11::mp_if
        <
            std::is_same<IsPlanar, use_default>,
            is_planar<Iterator>,
            IsPlanar
        >::value;

    static bool const step = mp11::mp_if
        <
            std::is_same<IsStep, use_default>,
            iterator_is_step<Iterator>,
            IsStep
        >::type::value;

public:
    using type = typename iterator_type<channel_t, layout_t, planar, step, mut>::type;
};

/// \brief Constructs an image view type from a source view type by changing some of the properties.
/// \ingroup TypeFactoryDerived
///  Use use_default for the properties of the source view that you want to keep
template <typename View, typename T = use_default, typename L = use_default, typename IsPlanar = use_default, typename StepX = use_default, typename IsMutable = use_default>
class derived_view_type
{
    using channel_t = typename mp11::mp_if
        <
            std::is_same<T, use_default>,
            typename channel_type<View>::type,
            T
        >;

    using layout_t = typename mp11::mp_if
        <
            std::is_same<L, use_default>,
            layout
            <
                typename color_space_type<View>::type,
                typename channel_mapping_type<View>::type
            >,
            L
        >;

    static bool const mut = mp11::mp_if
        <
            std::is_same<IsMutable, use_default>,
            view_is_mutable<View>,
            IsMutable
        >::value;

    static bool const planar = mp11::mp_if
        <
            std::is_same<IsPlanar, use_default>,
            is_planar<View>,
            IsPlanar
        >::value;

    static bool const step = mp11::mp_if
        <
            std::is_same<StepX, use_default>,
            view_is_step_in_x<View>,
            StepX
        >::value;

public:
    using type = typename view_type<channel_t, layout_t, planar, step, mut>::type;
};

/// \brief Constructs a homogeneous image type from a source image type by changing some of the properties.
/// \ingroup TypeFactoryDerived
///  Use use_default for the properties of the source image that you want to keep
template <typename Image, typename T = use_default, typename L = use_default, typename IsPlanar = use_default>
class derived_image_type
{
    using channel_t = typename mp11::mp_if
        <
            std::is_same<T, use_default>,
            typename channel_type<Image>::type,
            T
        >::type;

    using layout_t = typename mp11::mp_if
        <
            std::is_same<L, use_default>,
            layout
            <
                typename color_space_type<Image>::type,
                typename channel_mapping_type<Image>::type>,
                L
            >::type;

    static bool const planar = mp11::mp_if
        <
            std::is_same<IsPlanar, use_default>,
            is_planar<Image>,
            IsPlanar
        >::value;

public:
    using type = typename image_type<channel_t, layout_t, planar>::type;
};

}}  // namespace boost::gil

#endif

//
// Copyright 2005-2007 Adobe Systems Incorporated
// Copyright 2019 Mateusz Loskot <mateusz at loskot dot net>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_COLOR_BASE_ALGORITHM_HPP
#define BOOST_GIL_COLOR_BASE_ALGORITHM_HPP

#include <boost/gil/concepts.hpp>
#include <boost/gil/utilities.hpp>
#include <boost/gil/detail/mp11.hpp>

#include <boost/config.hpp>

#include <algorithm>
#include <type_traits>

namespace boost { namespace gil {

///////////////////////////////////////
/// size:   Semantic channel size
///////////////////////////////////////

/**
\defgroup ColorBaseAlgorithmSize size
\ingroup ColorBaseAlgorithm
\brief Returns an integral constant type specifying the number of elements in a color base

Example:
\code
static_assert(size<rgb8_pixel_t>::value == 3, "");
static_assert(size<cmyk8_planar_ptr_t>::value == 4, "");
\endcode
*/

/// \brief Returns an integral constant type specifying the number of elements in a color base
/// \ingroup ColorBaseAlgorithmSize
template <typename ColorBase>
struct size : public mp11::mp_size<typename ColorBase::layout_t::color_space_t> {};

///////////////////////////////////////
/// semantic_at_c:   Semantic channel accessors
///////////////////////////////////////

/**
\defgroup ColorBaseAlgorithmSemanticAtC kth_semantic_element_type, kth_semantic_element_reference_type, kth_semantic_element_const_reference_type, semantic_at_c
\ingroup ColorBaseAlgorithm
\brief Support for accessing the elements of a color base by semantic index

The semantic index of an element is the index of its color in the color space. Semantic indexing allows for proper pairing of elements of color bases
independent on their layout. For example, red is the first semantic element of a color base regardless of whether it has an RGB layout or a BGR layout.
All GIL color base algorithms taking multiple color bases use semantic indexing to access their elements.

Example:
\code
// 16-bit BGR pixel, 4 bits for the blue, 3 bits for the green, 2 bits for the red channel and 7 unused bits
using bgr432_pixel_t = packed_pixel_type<uint16_t, mp11::mp_list_c<unsigned,4,3,2>, bgr_layout_t>::type;

// A reference to its red channel. Although the red channel is the third, its semantic index is 0 in the RGB color space
using red_channel_reference_t = kth_semantic_element_reference_type<bgr432_pixel_t, 0>::type;

// Initialize the pixel to black
bgr432_pixel_t red_pixel(0,0,0);

// Set the red channel to 100%
red_channel_reference_t red_channel = semantic_at_c<0>(red_pixel);
red_channel = channel_traits<red_channel_reference_t>::max_value();

\endcode
*/
/// \brief Specifies the type of the K-th semantic element of a color base
/// \ingroup ColorBaseAlgorithmSemanticAtC
template <typename ColorBase, int K>
struct kth_semantic_element_type
{
    using channel_mapping_t = typename ColorBase::layout_t::channel_mapping_t;
    static_assert(K < mp11::mp_size<channel_mapping_t>::value,
        "K index should be less than size of channel_mapping_t sequence");

    static constexpr int semantic_index = mp11::mp_at_c<channel_mapping_t, K>::type::value;
    using type = typename kth_element_type<ColorBase, semantic_index>::type;
};

/// \brief Specifies the return type of the mutable semantic_at_c<K>(color_base);
/// \ingroup ColorBaseAlgorithmSemanticAtC
template <typename ColorBase, int K>
struct kth_semantic_element_reference_type
{
    using channel_mapping_t = typename ColorBase::layout_t::channel_mapping_t;
    static_assert(K < mp11::mp_size<channel_mapping_t>::value,
        "K index should be less than size of channel_mapping_t sequence");

    static constexpr int semantic_index = mp11::mp_at_c<channel_mapping_t, K>::type::value;
    using type = typename kth_element_reference_type<ColorBase, semantic_index>::type;
    static type get(ColorBase& cb) { return gil::at_c<semantic_index>(cb); }
};

/// \brief Specifies the return type of the constant semantic_at_c<K>(color_base);
/// \ingroup ColorBaseAlgorithmSemanticAtC
template <typename ColorBase, int K>
struct kth_semantic_element_const_reference_type
{
    using channel_mapping_t = typename ColorBase::layout_t::channel_mapping_t;
    static_assert(K < mp11::mp_size<channel_mapping_t>::value,
        "K index should be less than size of channel_mapping_t sequence");

    static constexpr int semantic_index = mp11::mp_at_c<channel_mapping_t, K>::type::value;
    using type = typename kth_element_const_reference_type<ColorBase,semantic_index>::type;
    static type get(const ColorBase& cb) { return gil::at_c<semantic_index>(cb); }
};

/// \brief A mutable accessor to the K-th semantic element of a color base
/// \ingroup ColorBaseAlgorithmSemanticAtC
template <int K, typename ColorBase>
inline
auto semantic_at_c(ColorBase& p)
    -> typename std::enable_if
    <
        !std::is_const<ColorBase>::value,
        typename kth_semantic_element_reference_type<ColorBase, K>::type
    >::type
{
    return kth_semantic_element_reference_type<ColorBase, K>::get(p);
}

/// \brief A constant accessor to the K-th semantic element of a color base
/// \ingroup ColorBaseAlgorithmSemanticAtC
template <int K, typename ColorBase>
inline
auto semantic_at_c(ColorBase const& p)
    -> typename kth_semantic_element_const_reference_type<ColorBase, K>::type
{
    return kth_semantic_element_const_reference_type<ColorBase, K>::get(p);
}

///////////////////////////////////////
/// get_color:   Named channel accessors
///////////////////////////////////////

/**
\defgroup ColorBaseAlgorithmColor color_element_type, color_element_reference_type, color_element_const_reference_type, get_color, contains_color
\ingroup ColorBaseAlgorithm
\brief Support for accessing the elements of a color base by color name

Example: A function that takes a generic pixel containing a red channel and sets it to 100%:

\code
template <typename Pixel>
void set_red_to_max(Pixel& pixel) {
    boost::function_requires<MutablePixelConcept<Pixel> >();
    static_assert(contains_color<Pixel, red_t>::value, "");

    using red_channel_t = typename color_element_type<Pixel, red_t>::type;
    get_color(pixel, red_t()) = channel_traits<red_channel_t>::max_value();
}
\endcode
*/

/// \brief A predicate metafunction determining whether a given color base contains a given color
/// \ingroup ColorBaseAlgorithmColor
template <typename ColorBase, typename Color>
struct contains_color
    : mp11::mp_contains<typename ColorBase::layout_t::color_space_t, Color>
{};

template <typename ColorBase, typename Color>
struct color_index_type : public detail::type_to_index<typename ColorBase::layout_t::color_space_t,Color> {};

/// \brief Specifies the type of the element associated with a given color tag
/// \ingroup ColorBaseAlgorithmColor
template <typename ColorBase, typename Color>
struct color_element_type : public kth_semantic_element_type<ColorBase,color_index_type<ColorBase,Color>::value> {};

/// \brief Specifies the return type of the mutable element accessor by color name, get_color(color_base, Color());
/// \ingroup ColorBaseAlgorithmColor
template <typename ColorBase, typename Color>
struct color_element_reference_type : public kth_semantic_element_reference_type<ColorBase,color_index_type<ColorBase,Color>::value> {};

/// \brief Specifies the return type of the constant element accessor by color name, get_color(color_base, Color());
/// \ingroup ColorBaseAlgorithmColor
template <typename ColorBase, typename Color>
struct color_element_const_reference_type : public kth_semantic_element_const_reference_type<ColorBase,color_index_type<ColorBase,Color>::value> {};

/// \brief Mutable accessor to the element associated with a given color name
/// \ingroup ColorBaseAlgorithmColor
template <typename ColorBase, typename Color>
typename color_element_reference_type<ColorBase,Color>::type get_color(ColorBase& cb, Color=Color()) {
    return color_element_reference_type<ColorBase,Color>::get(cb);
}

/// \brief Constant accessor to the element associated with a given color name
/// \ingroup ColorBaseAlgorithmColor
template <typename ColorBase, typename Color>
typename color_element_const_reference_type<ColorBase,Color>::type get_color(const ColorBase& cb, Color=Color()) {
    return color_element_const_reference_type<ColorBase,Color>::get(cb);
}

///////////////////////////////////////
///
/// element_type, element_reference_type, element_const_reference_type: Support for homogeneous color bases
///
///////////////////////////////////////

/**
\defgroup ColorBaseAlgorithmHomogeneous element_type, element_reference_type, element_const_reference_type
\ingroup ColorBaseAlgorithm
\brief Types for homogeneous color bases

Example:
\code
using element_t = element_type<rgb8c_planar_ptr_t>::type;
static_assert(std::is_same<element_t, const uint8_t*>::value, "");
\endcode
*/
/// \brief Specifies the element type of a homogeneous color base
/// \ingroup ColorBaseAlgorithmHomogeneous
template <typename ColorBase>
struct element_type : public kth_element_type<ColorBase, 0> {};

/// \brief Specifies the return type of the mutable element accessor at_c of a homogeneous color base
/// \ingroup ColorBaseAlgorithmHomogeneous
template <typename ColorBase>
struct element_reference_type : public kth_element_reference_type<ColorBase, 0> {};

/// \brief Specifies the return type of the constant element accessor at_c of a homogeneous color base
/// \ingroup ColorBaseAlgorithmHomogeneous
template <typename ColorBase>
struct element_const_reference_type : public kth_element_const_reference_type<ColorBase, 0> {};


namespace detail {

// compile-time recursion for per-element operations on color bases
template <int N>
struct element_recursion
{

#if defined(BOOST_GCC) && (BOOST_GCC >= 40900)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

    template <typename P1,typename P2>
    static bool static_equal(const P1& p1, const P2& p2)
    {
        return element_recursion<N-1>::static_equal(p1,p2) &&
               semantic_at_c<N-1>(p1)==semantic_at_c<N-1>(p2);
    }

    template <typename P1,typename P2>
    static void static_copy(const P1& p1, P2& p2)
    {
        element_recursion<N-1>::static_copy(p1,p2);
        semantic_at_c<N-1>(p2)=semantic_at_c<N-1>(p1);
    }

    template <typename P,typename T2>
    static void static_fill(P& p, T2 v)
    {
        element_recursion<N-1>::static_fill(p,v);
        semantic_at_c<N-1>(p)=v;
    }

    template <typename Dst,typename Op>
    static void static_generate(Dst& dst, Op op)
    {
        element_recursion<N-1>::static_generate(dst,op);
        semantic_at_c<N-1>(dst)=op();
    }

#if defined(BOOST_GCC) && (BOOST_GCC >= 40900)
#pragma GCC diagnostic pop
#endif

    //static_for_each with one source
    template <typename P1,typename Op>
    static Op static_for_each(P1& p1, Op op) {
        Op op2(element_recursion<N-1>::static_for_each(p1,op));
        op2(semantic_at_c<N-1>(p1));
        return op2;
    }
    template <typename P1,typename Op>
    static Op static_for_each(const P1& p1, Op op) {
        Op op2(element_recursion<N-1>::static_for_each(p1,op));
        op2(semantic_at_c<N-1>(p1));
        return op2;
    }
    //static_for_each with two sources
    template <typename P1,typename P2,typename Op>
    static Op static_for_each(P1& p1, P2& p2, Op op) {
        Op op2(element_recursion<N-1>::static_for_each(p1,p2,op));
        op2(semantic_at_c<N-1>(p1), semantic_at_c<N-1>(p2));
        return op2;
    }
    template <typename P1,typename P2,typename Op>
    static Op static_for_each(P1& p1, const P2& p2, Op op) {
        Op op2(element_recursion<N-1>::static_for_each(p1,p2,op));
        op2(semantic_at_c<N-1>(p1), semantic_at_c<N-1>(p2));
        return op2;
    }
    template <typename P1,typename P2,typename Op>
    static Op static_for_each(const P1& p1, P2& p2, Op op) {
        Op op2(element_recursion<N-1>::static_for_each(p1,p2,op));
        op2(semantic_at_c<N-1>(p1), semantic_at_c<N-1>(p2));
        return op2;
    }
    template <typename P1,typename P2,typename Op>
    static Op static_for_each(const P1& p1, const P2& p2, Op op) {
        Op op2(element_recursion<N-1>::static_for_each(p1,p2,op));
        op2(semantic_at_c<N-1>(p1), semantic_at_c<N-1>(p2));
        return op2;
    }
    //static_for_each with three sources
    template <typename P1,typename P2,typename P3,typename Op>
    static Op static_for_each(P1& p1, P2& p2, P3& p3, Op op) {
        Op op2(element_recursion<N-1>::static_for_each(p1,p2,p3,op));
        op2(semantic_at_c<N-1>(p1), semantic_at_c<N-1>(p2), semantic_at_c<N-1>(p3));
        return op2;
    }
    template <typename P1,typename P2,typename P3,typename Op>
    static Op static_for_each(P1& p1, P2& p2, const P3& p3, Op op) {
        Op op2(element_recursion<N-1>::static_for_each(p1,p2,p3,op));
        op2(semantic_at_c<N-1>(p1), semantic_at_c<N-1>(p2), semantic_at_c<N-1>(p3));
        return op2;
    }
    template <typename P1,typename P2,typename P3,typename Op>
    static Op static_for_each(P1& p1, const P2& p2, P3& p3, Op op) {
        Op op2(element_recursion<N-1>::static_for_each(p1,p2,p3,op));
        op2(semantic_at_c<N-1>(p1), semantic_at_c<N-1>(p2), semantic_at_c<N-1>(p3));
        return op2;
    }
    template <typename P1,typename P2,typename P3,typename Op>
    static Op static_for_each(P1& p1, const P2& p2, const P3& p3, Op op) {
        Op op2(element_recursion<N-1>::static_for_each(p1,p2,p3,op));
        op2(semantic_at_c<N-1>(p1), semantic_at_c<N-1>(p2), semantic_at_c<N-1>(p3));
        return op2;
    }
    template <typename P1,typename P2,typename P3,typename Op>
    static Op static_for_each(const P1& p1, P2& p2, P3& p3, Op op) {
        Op op2(element_recursion<N-1>::static_for_each(p1,p2,p3,op));
        op2(semantic_at_c<N-1>(p1), semantic_at_c<N-1>(p2), semantic_at_c<N-1>(p3));
        return op2;
    }
    template <typename P1,typename P2,typename P3,typename Op>
    static Op static_for_each(const P1& p1, P2& p2, const P3& p3, Op op) {
        Op op2(element_recursion<N-1>::static_for_each(p1,p2,p3,op));
        op2(semantic_at_c<N-1>(p1), semantic_at_c<N-1>(p2), semantic_at_c<N-1>(p3));
        return op2;
    }
    template <typename P1,typename P2,typename P3,typename Op>
    static Op static_for_each(const P1& p1, const P2& p2, P3& p3, Op op) {
        Op op2(element_recursion<N-1>::static_for_each(p1,p2,p3,op));
        op2(semantic_at_c<N-1>(p1), semantic_at_c<N-1>(p2), semantic_at_c<N-1>(p3));
        return op2;
    }
    template <typename P1,typename P2,typename P3,typename Op>
    static Op static_for_each(const P1& p1, const P2& p2, const P3& p3, Op op) {
        Op op2(element_recursion<N-1>::static_for_each(p1,p2,p3,op));
        op2(semantic_at_c<N-1>(p1), semantic_at_c<N-1>(p2), semantic_at_c<N-1>(p3));
        return op2;
    }
    //static_transform with one source
    template <typename P1,typename Dst,typename Op>
    static Op static_transform(P1& src, Dst& dst, Op op) {
        Op op2(element_recursion<N-1>::static_transform(src,dst,op));
        semantic_at_c<N-1>(dst)=op2(semantic_at_c<N-1>(src));
        return op2;
    }
    template <typename P1,typename Dst,typename Op>
    static Op static_transform(const P1& src, Dst& dst, Op op) {
        Op op2(element_recursion<N-1>::static_transform(src,dst,op));
        semantic_at_c<N-1>(dst)=op2(semantic_at_c<N-1>(src));
        return op2;
    }
    //static_transform with two sources
    template <typename P1,typename P2,typename Dst,typename Op>
    static Op static_transform(P1& src1, P2& src2, Dst& dst, Op op) {
        Op op2(element_recursion<N-1>::static_transform(src1,src2,dst,op));
        semantic_at_c<N-1>(dst)=op2(semantic_at_c<N-1>(src1), semantic_at_c<N-1>(src2));
        return op2;
    }
    template <typename P1,typename P2,typename Dst,typename Op>
    static Op static_transform(P1& src1, const P2& src2, Dst& dst, Op op) {
        Op op2(element_recursion<N-1>::static_transform(src1,src2,dst,op));
        semantic_at_c<N-1>(dst)=op2(semantic_at_c<N-1>(src1), semantic_at_c<N-1>(src2));
        return op2;
    }
    template <typename P1,typename P2,typename Dst,typename Op>
    static Op static_transform(const P1& src1, P2& src2, Dst& dst, Op op) {
        Op op2(element_recursion<N-1>::static_transform(src1,src2,dst,op));
        semantic_at_c<N-1>(dst)=op2(semantic_at_c<N-1>(src1), semantic_at_c<N-1>(src2));
        return op2;
    }
    template <typename P1,typename P2,typename Dst,typename Op>
    static Op static_transform(const P1& src1, const P2& src2, Dst& dst, Op op) {
        Op op2(element_recursion<N-1>::static_transform(src1,src2,dst,op));
        semantic_at_c<N-1>(dst)=op2(semantic_at_c<N-1>(src1), semantic_at_c<N-1>(src2));
        return op2;
    }
};

// Termination condition of the compile-time recursion for element operations on a color base
template<> struct element_recursion<0> {
    //static_equal
    template <typename P1,typename P2>
    static bool static_equal(const P1&, const P2&) { return true; }
    //static_copy
    template <typename P1,typename P2>
    static void static_copy(const P1&, const P2&) {}
    //static_fill
    template <typename P, typename T2>
    static void static_fill(const P&, T2) {}
    //static_generate
    template <typename Dst,typename Op>
    static void static_generate(const Dst&,Op){}
    //static_for_each with one source
    template <typename P1,typename Op>
    static Op static_for_each(const P1&,Op op){return op;}
    //static_for_each with two sources
    template <typename P1,typename P2,typename Op>
    static Op static_for_each(const P1&,const P2&,Op op){return op;}
    //static_for_each with three sources
    template <typename P1,typename P2,typename P3,typename Op>
    static Op static_for_each(const P1&,const P2&,const P3&,Op op){return op;}
    //static_transform with one source
    template <typename P1,typename Dst,typename Op>
    static Op static_transform(const P1&,const Dst&,Op op){return op;}
    //static_transform with two sources
    template <typename P1,typename P2,typename Dst,typename Op>
    static Op static_transform(const P1&,const P2&,const Dst&,Op op){return op;}
};

// std::min and std::max don't have the mutable overloads...
template <typename Q> inline const Q& mutable_min(const Q& x, const Q& y) { return x<y ? x : y; }
template <typename Q> inline       Q& mutable_min(      Q& x,       Q& y) { return x<y ? x : y; }
template <typename Q> inline const Q& mutable_max(const Q& x, const Q& y) { return x<y ? y : x; }
template <typename Q> inline       Q& mutable_max(      Q& x,       Q& y) { return x<y ? y : x; }


// compile-time recursion for min/max element
template <int N>
struct min_max_recur {
    template <typename P> static typename element_const_reference_type<P>::type max_(const P& p) {
        return mutable_max(min_max_recur<N-1>::max_(p),semantic_at_c<N-1>(p));
    }
    template <typename P> static typename element_reference_type<P>::type       max_(      P& p) {
        return mutable_max(min_max_recur<N-1>::max_(p),semantic_at_c<N-1>(p));
    }
    template <typename P> static typename element_const_reference_type<P>::type min_(const P& p) {
        return mutable_min(min_max_recur<N-1>::min_(p),semantic_at_c<N-1>(p));
    }
    template <typename P> static typename element_reference_type<P>::type       min_(      P& p) {
        return mutable_min(min_max_recur<N-1>::min_(p),semantic_at_c<N-1>(p));
    }
};

// termination condition of the compile-time recursion for min/max element
template <>
struct min_max_recur<1> {
    template <typename P> static typename element_const_reference_type<P>::type max_(const P& p) { return semantic_at_c<0>(p); }
    template <typename P> static typename element_reference_type<P>::type       max_(      P& p) { return semantic_at_c<0>(p); }
    template <typename P> static typename element_const_reference_type<P>::type min_(const P& p) { return semantic_at_c<0>(p); }
    template <typename P> static typename element_reference_type<P>::type       min_(      P& p) { return semantic_at_c<0>(p); }
};
}  // namespace detail

/// \defgroup ColorBaseAlgorithmMinMax static_min, static_max
/// \ingroup ColorBaseAlgorithm
/// \brief Equivalents to std::min_element and std::max_element for homogeneous color bases
///
/// Example:
/// \code
/// rgb8_pixel_t pixel(10,20,30);
/// assert(pixel[2] == 30);
/// static_max(pixel) = static_min(pixel);
/// assert(pixel[2] == 10);
/// \endcode
/// \{

template <typename P>
BOOST_FORCEINLINE
typename element_const_reference_type<P>::type static_max(const P& p) { return detail::min_max_recur<size<P>::value>::max_(p); }

template <typename P>
BOOST_FORCEINLINE
typename element_reference_type<P>::type       static_max(      P& p) { return detail::min_max_recur<size<P>::value>::max_(p); }

template <typename P>
BOOST_FORCEINLINE
typename element_const_reference_type<P>::type static_min(const P& p) { return detail::min_max_recur<size<P>::value>::min_(p); }

template <typename P>
BOOST_FORCEINLINE
typename element_reference_type<P>::type       static_min(      P& p) { return detail::min_max_recur<size<P>::value>::min_(p); }
/// \}

/// \defgroup ColorBaseAlgorithmEqual static_equal
/// \ingroup ColorBaseAlgorithm
/// \brief Equivalent to std::equal. Pairs the elements semantically
///
/// Example:
/// \code
/// rgb8_pixel_t rgb_red(255,0,0);
/// bgr8_pixel_t bgr_red(0,0,255);
/// assert(rgb_red[0]==255 && bgr_red[0]==0);
///
/// assert(static_equal(rgb_red,bgr_red));
/// assert(rgb_red==bgr_red);  // operator== invokes static_equal
/// \endcode
/// \{

template <typename P1,typename P2>
BOOST_FORCEINLINE
bool static_equal(const P1& p1, const P2& p2) { return detail::element_recursion<size<P1>::value>::static_equal(p1,p2); }

/// \}

/// \defgroup ColorBaseAlgorithmCopy static_copy
/// \ingroup ColorBaseAlgorithm
/// \brief Equivalent to std::copy. Pairs the elements semantically
///
/// Example:
/// \code
/// rgb8_pixel_t rgb_red(255,0,0);
/// bgr8_pixel_t bgr_red;
/// static_copy(rgb_red, bgr_red);  // same as bgr_red = rgb_red
///
/// assert(rgb_red[0] == 255 && bgr_red[0] == 0);
/// assert(rgb_red == bgr_red);
/// \endcode
/// \{

template <typename Src,typename Dst>
BOOST_FORCEINLINE
void static_copy(const Src& src, Dst& dst)
{
    detail::element_recursion<size<Dst>::value>::static_copy(src, dst);
}

/// \}

/// \defgroup ColorBaseAlgorithmFill static_fill
/// \ingroup ColorBaseAlgorithm
/// \brief Equivalent to std::fill.
///
/// Example:
/// \code
/// rgb8_pixel_t p;
/// static_fill(p, 10);
/// assert(p == rgb8_pixel_t(10,10,10));
/// \endcode
/// \{

template <typename P,typename V>
BOOST_FORCEINLINE
void static_fill(P& p, const V& v)
{
    detail::element_recursion<size<P>::value>::static_fill(p,v);
}

/// \}

/// \defgroup ColorBaseAlgorithmGenerate static_generate
/// \ingroup ColorBaseAlgorithm
/// \brief Equivalent to std::generate.
///
/// Example: Set each channel of a pixel to its semantic index. The channels must be assignable from an integer.
/// \code
/// struct consecutive_fn {
///     int& _current;
///     consecutive_fn(int& start) : _current(start) {}
///     int operator()() { return _current++; }
/// };
/// rgb8_pixel_t p;
/// int start=0;
/// static_generate(p, consecutive_fn(start));
/// assert(p == rgb8_pixel_t(0,1,2));
/// \endcode
///
/// \{

template <typename P1,typename Op>
BOOST_FORCEINLINE
void static_generate(P1& dst,Op op)                      { detail::element_recursion<size<P1>::value>::static_generate(dst,op); }
/// \}

/// \defgroup ColorBaseAlgorithmTransform static_transform
/// \ingroup ColorBaseAlgorithm
/// \brief Equivalent to std::transform. Pairs the elements semantically
///
/// Example: Write a generic function that adds two pixels into a homogeneous result pixel.
/// \code
/// template <typename Result>
/// struct my_plus {
///     template <typename T1, typename T2>
///     Result operator()(T1 f1, T2 f2) const { return f1+f2; }
/// };
///
/// template <typename Pixel1, typename Pixel2, typename Pixel3>
/// void sum_channels(const Pixel1& p1, const Pixel2& p2, Pixel3& result) {
///     using result_channel_t = typename channel_type<Pixel3>::type;
///     static_transform(p1,p2,result,my_plus<result_channel_t>());
/// }
///
/// rgb8_pixel_t p1(1,2,3);
/// bgr8_pixel_t p2(3,2,1);
/// rgb8_pixel_t result;
/// sum_channels(p1,p2,result);
/// assert(result == rgb8_pixel_t(2,4,6));
/// \endcode
/// \{

//static_transform with one source
template <typename Src,typename Dst,typename Op>
BOOST_FORCEINLINE
Op static_transform(Src& src,Dst& dst,Op op)              { return detail::element_recursion<size<Dst>::value>::static_transform(src,dst,op); }
template <typename Src,typename Dst,typename Op>
BOOST_FORCEINLINE
Op static_transform(const Src& src,Dst& dst,Op op)              { return detail::element_recursion<size<Dst>::value>::static_transform(src,dst,op); }
//static_transform with two sources
template <typename P2,typename P3,typename Dst,typename Op>
BOOST_FORCEINLINE
Op static_transform(P2& p2,P3& p3,Dst& dst,Op op) { return detail::element_recursion<size<Dst>::value>::static_transform(p2,p3,dst,op); }
template <typename P2,typename P3,typename Dst,typename Op>
BOOST_FORCEINLINE
Op static_transform(P2& p2,const P3& p3,Dst& dst,Op op) { return detail::element_recursion<size<Dst>::value>::static_transform(p2,p3,dst,op); }
template <typename P2,typename P3,typename Dst,typename Op>
BOOST_FORCEINLINE
Op static_transform(const P2& p2,P3& p3,Dst& dst,Op op) { return detail::element_recursion<size<Dst>::value>::static_transform(p2,p3,dst,op); }
template <typename P2,typename P3,typename Dst,typename Op>
BOOST_FORCEINLINE
Op static_transform(const P2& p2,const P3& p3,Dst& dst,Op op) { return detail::element_recursion<size<Dst>::value>::static_transform(p2,p3,dst,op); }
/// \}

/// \defgroup ColorBaseAlgorithmForEach static_for_each
/// \ingroup ColorBaseAlgorithm
/// \brief Equivalent to std::for_each. Pairs the elements semantically
///
/// Example: Use static_for_each to increment a planar pixel iterator
/// \code
/// struct increment {
///     template <typename Incrementable>
///     void operator()(Incrementable& x) const { ++x; }
/// };
///
/// template <typename ColorBase>
/// void increment_elements(ColorBase& cb) {
///     static_for_each(cb, increment());
/// }
///
/// uint8_t red[2], green[2], blue[2];
/// rgb8c_planar_ptr_t p1(red,green,blue);
/// rgb8c_planar_ptr_t p2=p1;
/// increment_elements(p1);
/// ++p2;
/// assert(p1 == p2);
/// \endcode
/// \{

//static_for_each with one source
template <typename P1,typename Op>
BOOST_FORCEINLINE
Op static_for_each(      P1& p1, Op op)                          { return detail::element_recursion<size<P1>::value>::static_for_each(p1,op); }
template <typename P1,typename Op>
BOOST_FORCEINLINE
Op static_for_each(const P1& p1, Op op)                          { return detail::element_recursion<size<P1>::value>::static_for_each(p1,op); }
//static_for_each with two sources
template <typename P1,typename P2,typename Op>
BOOST_FORCEINLINE
Op static_for_each(P1& p1,      P2& p2, Op op)             { return detail::element_recursion<size<P1>::value>::static_for_each(p1,p2,op); }
template <typename P1,typename P2,typename Op>
BOOST_FORCEINLINE
Op static_for_each(P1& p1,const P2& p2, Op op)             { return detail::element_recursion<size<P1>::value>::static_for_each(p1,p2,op); }
template <typename P1,typename P2,typename Op>
BOOST_FORCEINLINE
Op static_for_each(const P1& p1,      P2& p2, Op op)             { return detail::element_recursion<size<P1>::value>::static_for_each(p1,p2,op); }
template <typename P1,typename P2,typename Op>
BOOST_FORCEINLINE
Op static_for_each(const P1& p1,const P2& p2, Op op)             { return detail::element_recursion<size<P1>::value>::static_for_each(p1,p2,op); }
//static_for_each with three sources
template <typename P1,typename P2,typename P3,typename Op>
BOOST_FORCEINLINE
Op static_for_each(P1& p1,P2& p2,P3& p3,Op op) { return detail::element_recursion<size<P1>::value>::static_for_each(p1,p2,p3,op); }
template <typename P1,typename P2,typename P3,typename Op>
BOOST_FORCEINLINE
Op static_for_each(P1& p1,P2& p2,const P3& p3,Op op) { return detail::element_recursion<size<P1>::value>::static_for_each(p1,p2,p3,op); }
template <typename P1,typename P2,typename P3,typename Op>
BOOST_FORCEINLINE
Op static_for_each(P1& p1,const P2& p2,P3& p3,Op op) { return detail::element_recursion<size<P1>::value>::static_for_each(p1,p2,p3,op); }
template <typename P1,typename P2,typename P3,typename Op>
BOOST_FORCEINLINE
Op static_for_each(P1& p1,const P2& p2,const P3& p3,Op op) { return detail::element_recursion<size<P1>::value>::static_for_each(p1,p2,p3,op); }
template <typename P1,typename P2,typename P3,typename Op>
BOOST_FORCEINLINE
Op static_for_each(const P1& p1,P2& p2,P3& p3,Op op) { return detail::element_recursion<size<P1>::value>::static_for_each(p1,p2,p3,op); }
template <typename P1,typename P2,typename P3,typename Op>
BOOST_FORCEINLINE
Op static_for_each(const P1& p1,P2& p2,const P3& p3,Op op) { return detail::element_recursion<size<P1>::value>::static_for_each(p1,p2,p3,op); }
template <typename P1,typename P2,typename P3,typename Op>
BOOST_FORCEINLINE
Op static_for_each(const P1& p1,const P2& p2,P3& p3,Op op) { return detail::element_recursion<size<P1>::value>::static_for_each(p1,p2,p3,op); }
template <typename P1,typename P2,typename P3,typename Op>
BOOST_FORCEINLINE
Op static_for_each(const P1& p1,const P2& p2,const P3& p3,Op op) { return detail::element_recursion<size<P1>::value>::static_for_each(p1,p2,p3,op); }
///\}

} }  // namespace boost::gil

#endif

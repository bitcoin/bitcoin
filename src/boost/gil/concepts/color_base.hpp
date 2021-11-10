//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_CONCEPTS_COLOR_BASE_HPP
#define BOOST_GIL_CONCEPTS_COLOR_BASE_HPP

#include <boost/gil/concepts/basic.hpp>
#include <boost/gil/concepts/color.hpp>
#include <boost/gil/concepts/concept_check.hpp>
#include <boost/gil/concepts/fwd.hpp>

#include <boost/core/ignore_unused.hpp>
#include <type_traits>

#if defined(BOOST_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wunused-local-typedefs"
#endif

#if defined(BOOST_GCC) && (BOOST_GCC >= 40900)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif

namespace boost { namespace gil {

// Forward declarations of at_c
namespace detail {

template <typename Element, typename Layout, int K>
struct homogeneous_color_base;

} // namespace detail

template <int K, typename E, typename L, int N>
auto at_c(detail::homogeneous_color_base<E, L, N>& p)
    -> typename std::add_lvalue_reference<E>::type;

template <int K, typename E, typename L, int N>
auto at_c(detail::homogeneous_color_base<E, L, N> const& p)
    -> typename std::add_lvalue_reference<typename std::add_const<E>::type>::type;

template <typename P, typename C, typename L>
struct packed_pixel;

template <int K, typename P, typename C, typename L>
auto at_c(packed_pixel<P, C, L>& p)
    -> typename kth_element_reference_type<packed_pixel<P, C, L>, K>::type;

template <int K, typename P, typename C, typename L>
auto at_c(packed_pixel<P, C, L> const& p)
    -> typename kth_element_const_reference_type<packed_pixel<P, C, L>, K>::type;

template <typename B, typename C, typename L, bool M>
struct bit_aligned_pixel_reference;

template <int K, typename B, typename C, typename L, bool M>
inline auto at_c(bit_aligned_pixel_reference<B, C, L, M> const& p)
    -> typename kth_element_reference_type
        <
            bit_aligned_pixel_reference<B, C, L, M>,
            K
        >::type;

// Forward declarations of semantic_at_c
template <int K, typename ColorBase>
auto semantic_at_c(ColorBase& p)
    -> typename std::enable_if
        <
            !std::is_const<ColorBase>::value,
            typename kth_semantic_element_reference_type<ColorBase, K>::type
        >::type;

template <int K, typename ColorBase>
auto semantic_at_c(ColorBase const& p)
    -> typename kth_semantic_element_const_reference_type<ColorBase, K>::type;

/// \ingroup ColorBaseConcept
/// \brief A color base is a container of color elements (such as channels, channel references or channel pointers).
///
/// The most common use of color base is in the implementation of a pixel,
/// in which case the color elements are channel values. The color base concept,
/// however, can be used in other scenarios. For example, a planar pixel has
/// channels that are not contiguous in memory. Its reference is a proxy class
/// that uses a color base whose elements are channel references. Its iterator
/// uses a color base whose elements are channel iterators.
///
/// A color base must have an associated layout (which consists of a color space,
/// as well as an ordering of the channels).
/// There are two ways to index the elements of a color base: A physical index
/// corresponds to the way they are ordered in memory, and a semantic index
/// corresponds to the way the elements are ordered in their color space.
/// For example, in the RGB color space the elements are ordered as
/// {red_t, green_t, blue_t}. For a color base with a BGR layout, the first element
/// in physical ordering is the blue element, whereas the first semantic element
/// is the red one.
/// Models of \p ColorBaseConcept are required to provide the \p at_c<K>(ColorBase)
/// function, which allows for accessing the elements based on their physical order.
/// GIL provides a \p semantic_at_c<K>(ColorBase) function (described later)
/// which can operate on any model of ColorBaseConcept and returns the corresponding
/// semantic element.
///
/// \code
/// concept ColorBaseConcept<typename T> : CopyConstructible<T>, EqualityComparable<T>
/// {
///     // a GIL layout (the color space and element permutation)
///     typename layout_t;
///
///     // The type of K-th element
///     template <int K>
///     struct kth_element_type;
///         where Metafunction<kth_element_type>;
///
///     // The result of at_c
///     template <int K>
///     struct kth_element_const_reference_type;
///         where Metafunction<kth_element_const_reference_type>;
///
///     template <int K>
///     kth_element_const_reference_type<T,K>::type at_c(T);
///
///     // Copy-constructible and equality comparable with other compatible color bases
///     template <ColorBaseConcept T2> where { ColorBasesCompatibleConcept<T,T2> }
///         T::T(T2);
///     template <ColorBaseConcept T2> where { ColorBasesCompatibleConcept<T,T2> }
///         bool operator==(const T&, const T2&);
///     template <ColorBaseConcept T2> where { ColorBasesCompatibleConcept<T,T2> }
///         bool operator!=(const T&, const T2&);
/// };
/// \endcode
template <typename ColorBase>
struct ColorBaseConcept
{
    void constraints()
    {
        gil_function_requires<CopyConstructible<ColorBase>>();
        gil_function_requires<EqualityComparable<ColorBase>>();

        using color_space_t = typename ColorBase::layout_t::color_space_t;
        gil_function_requires<ColorSpaceConcept<color_space_t>>();

        using channel_mapping_t = typename ColorBase::layout_t::channel_mapping_t;
        // TODO: channel_mapping_t must be an Boost.MP11-compatible random access sequence

        static const int num_elements = size<ColorBase>::value;

        using TN = typename kth_element_type<ColorBase, num_elements - 1>::type;
        using RN = typename kth_element_const_reference_type<ColorBase, num_elements - 1>::type;

        RN r = gil::at_c<num_elements - 1>(cb);
        boost::ignore_unused(r);

        // functions that work for every pixel (no need to require them)
        semantic_at_c<0>(cb);
        semantic_at_c<num_elements-1>(cb);
        // also static_max(cb), static_min(cb), static_fill(cb,value),
        // and all variations of static_for_each(), static_generate(), static_transform()
    }
    ColorBase cb;
};

/// \ingroup ColorBaseConcept
/// \brief Color base which allows for modifying its elements
/// \code
/// concept MutableColorBaseConcept<ColorBaseConcept T> : Assignable<T>, Swappable<T>
/// {
///     template <int K>
///     struct kth_element_reference_type; where Metafunction<kth_element_reference_type>;
///
///     template <int K>
///     kth_element_reference_type<kth_element_type<T,K>::type>::type at_c(T);
///
///     template <ColorBaseConcept T2> where { ColorBasesCompatibleConcept<T,T2> }
///         T& operator=(T&, const T2&);
/// };
/// \endcode
template <typename ColorBase>
struct MutableColorBaseConcept
{
    void constraints()
    {
        gil_function_requires<ColorBaseConcept<ColorBase>>();
        gil_function_requires<Assignable<ColorBase>>();
        gil_function_requires<Swappable<ColorBase>>();

        using R0 = typename kth_element_reference_type<ColorBase, 0>::type;

        R0 r = gil::at_c<0>(cb);
        gil::at_c<0>(cb) = r;
    }
    ColorBase cb;
};

/// \ingroup ColorBaseConcept
/// \brief Color base that also has a default-constructor. Refines Regular
/// \code
/// concept ColorBaseValueConcept<typename T> : MutableColorBaseConcept<T>, Regular<T>
/// {
/// };
/// \endcode
template <typename ColorBase>
struct ColorBaseValueConcept
{
    void constraints()
    {
        gil_function_requires<MutableColorBaseConcept<ColorBase>>();
        gil_function_requires<Regular<ColorBase>>();
    }
};

/// \ingroup ColorBaseConcept
/// \brief Color base whose elements all have the same type
/// \code
/// concept HomogeneousColorBaseConcept<ColorBaseConcept CB>
/// {
///     // For all K in [0 ... size<C1>::value-1):
///     //     where SameType<kth_element_type<CB,K>::type, kth_element_type<CB,K+1>::type>;
///     kth_element_const_reference_type<CB,0>::type dynamic_at_c(CB const&, std::size_t n) const;
/// };
/// \endcode
template <typename ColorBase>
struct HomogeneousColorBaseConcept
{
    void constraints()
    {
        gil_function_requires<ColorBaseConcept<ColorBase>>();

        static const int num_elements = size<ColorBase>::value;

        using T0 = typename kth_element_type<ColorBase, 0>::type;
        using TN = typename kth_element_type<ColorBase, num_elements - 1>::type;

        static_assert(std::is_same<T0, TN>::value, "");   // better than nothing

        using R0 = typename kth_element_const_reference_type<ColorBase, 0>::type;
        R0 r = dynamic_at_c(cb, 0);
        boost::ignore_unused(r);
    }
    ColorBase cb;
};

/// \ingroup ColorBaseConcept
/// \brief Homogeneous color base that allows for modifying its elements
/// \code
/// concept MutableHomogeneousColorBaseConcept<ColorBaseConcept CB>
///     : HomogeneousColorBaseConcept<CB>
/// {
///     kth_element_reference_type<CB, 0>::type dynamic_at_c(CB&, std::size_t n);
/// };
/// \endcode
template <typename ColorBase>
struct MutableHomogeneousColorBaseConcept
{
    void constraints()
    {
        gil_function_requires<ColorBaseConcept<ColorBase>>();
        gil_function_requires<HomogeneousColorBaseConcept<ColorBase>>();
        using R0 = typename kth_element_reference_type<ColorBase, 0>::type;
        R0 r = dynamic_at_c(cb, 0);
        boost::ignore_unused(r);
        dynamic_at_c(cb, 0) = dynamic_at_c(cb, 0);
    }
    ColorBase cb;
};

/// \ingroup ColorBaseConcept
/// \brief Homogeneous color base that also has a default constructor.
/// Refines Regular.
///
/// \code
/// concept HomogeneousColorBaseValueConcept<typename T>
///     : MutableHomogeneousColorBaseConcept<T>, Regular<T>
/// {
/// };
/// \endcode
template <typename ColorBase>
struct HomogeneousColorBaseValueConcept
{
    void constraints()
    {
        gil_function_requires<MutableHomogeneousColorBaseConcept<ColorBase>>();
        gil_function_requires<Regular<ColorBase>>();
    }
};

/// \ingroup ColorBaseConcept
/// \brief Two color bases are compatible if they have the same color space and their elements are compatible, semantic-pairwise.
/// \code
/// concept ColorBasesCompatibleConcept<ColorBaseConcept C1, ColorBaseConcept C2>
/// {
///     where SameType<C1::layout_t::color_space_t, C2::layout_t::color_space_t>;
///     // also, for all K in [0 ... size<C1>::value):
///     //     where Convertible<kth_semantic_element_type<C1,K>::type, kth_semantic_element_type<C2,K>::type>;
///     //     where Convertible<kth_semantic_element_type<C2,K>::type, kth_semantic_element_type<C1,K>::type>;
/// };
/// \endcode
template <typename ColorBase1, typename ColorBase2>
struct ColorBasesCompatibleConcept
{
    void constraints()
    {
        static_assert(std::is_same
            <
                typename ColorBase1::layout_t::color_space_t,
                typename ColorBase2::layout_t::color_space_t
            >::value, "");

//        using e1 = typename kth_semantic_element_type<ColorBase1,0>::type;
//        using e2 = typename kth_semantic_element_type<ColorBase2,0>::type;
//        "e1 is convertible to e2"
    }
};

}} // namespace boost::gil

#if defined(BOOST_CLANG)
#pragma clang diagnostic pop
#endif

#if defined(BOOST_GCC) && (BOOST_GCC >= 40900)
#pragma GCC diagnostic pop
#endif

#endif

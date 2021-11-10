//
// Copyright 2005-2007 Adobe Systems Incorporated
// Copyright 2019 Mateusz Loskot <mateusz at loskot dot net>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_COLOR_BASE_HPP
#define BOOST_GIL_COLOR_BASE_HPP

#include <boost/gil/utilities.hpp>
#include <boost/gil/concepts.hpp>
#include <boost/gil/detail/mp11.hpp>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <type_traits>

namespace boost { namespace gil {

// Forward-declare
template <typename P> P* memunit_advanced(const P* p, std::ptrdiff_t diff);

// Forward-declare semantic_at_c
template <int K, typename ColorBase>
auto semantic_at_c(ColorBase& p)
    -> typename std::enable_if
    <
        !std::is_const<ColorBase>::value,
        typename kth_semantic_element_reference_type<ColorBase, K>::type
    >::type;


template <int K, typename ColorBase>
typename kth_semantic_element_const_reference_type<ColorBase,K>::type semantic_at_c(const ColorBase& p);

// Forward declare element_reference_type
template <typename ColorBase> struct element_reference_type;
template <typename ColorBase> struct element_const_reference_type;
template <typename ColorBase, int K> struct kth_element_type;
template <typename ColorBase, int K> struct kth_element_type<const ColorBase,K> : public kth_element_type<ColorBase,K> {};
template <typename ColorBase, int K> struct kth_element_reference_type;
template <typename ColorBase, int K> struct kth_element_reference_type<const ColorBase,K> : public kth_element_reference_type<ColorBase,K> {};
template <typename ColorBase, int K> struct kth_element_const_reference_type;
template <typename ColorBase, int K> struct kth_element_const_reference_type<const ColorBase,K> : public kth_element_const_reference_type<ColorBase,K> {};

namespace detail {

template <typename DstLayout, typename SrcLayout, int K>
struct mapping_transform : mp11::mp_at
    <
        typename SrcLayout::channel_mapping_t,
        typename detail::type_to_index
        <
            typename DstLayout::channel_mapping_t,
            std::integral_constant<int, K>
        >
    >::type
{};

/// \defgroup ColorBaseModelHomogeneous detail::homogeneous_color_base
/// \ingroup ColorBaseModel
/// \brief A homogeneous color base holding one color element.
/// Models HomogeneousColorBaseConcept or HomogeneousColorBaseValueConcept
/// If the element type models Regular, this class models HomogeneousColorBaseValueConcept.

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#endif

/// \brief A homogeneous color base holding one color element.
/// Models HomogeneousColorBaseConcept or HomogeneousColorBaseValueConcept
/// \ingroup ColorBaseModelHomogeneous
template <typename Element, typename Layout>
struct homogeneous_color_base<Element, Layout, 1>
{
    using layout_t = Layout;

    homogeneous_color_base() = default;
    homogeneous_color_base(Element v) : v0_(v) {}

    template <typename E2, typename L2>
    homogeneous_color_base(homogeneous_color_base<E2, L2, 1> const& c)
        : v0_(gil::at_c<0>(c))
    {}

    auto at(std::integral_constant<int, 0>)
        -> typename element_reference_type<homogeneous_color_base>::type
    { return v0_; }

    auto at(std::integral_constant<int, 0>) const
        -> typename element_const_reference_type<homogeneous_color_base>::type
    { return v0_; }

    // grayscale pixel values are convertible to channel type
    // FIXME: explicit?
    operator Element() const { return v0_; }

private:
    Element v0_{};
};

/// \brief A homogeneous color base holding two color elements
/// Models HomogeneousColorBaseConcept or HomogeneousColorBaseValueConcept
/// \ingroup ColorBaseModelHomogeneous
template <typename Element, typename Layout>
struct homogeneous_color_base<Element, Layout, 2>
{
    using layout_t = Layout;

    homogeneous_color_base() = default;
    explicit homogeneous_color_base(Element v) : v0_(v), v1_(v) {}
    homogeneous_color_base(Element v0, Element v1) : v0_(v0), v1_(v1) {}

    template <typename E2, typename L2>
    homogeneous_color_base(homogeneous_color_base<E2, L2, 2> const& c)
        : v0_(gil::at_c<mapping_transform<Layout, L2, 0>::value>(c))
        , v1_(gil::at_c<mapping_transform<Layout, L2, 1>::value>(c))
    {}

    // Support for l-value reference proxy copy construction
    template <typename E2, typename L2>
    homogeneous_color_base(homogeneous_color_base<E2, L2, 2>& c)
        : v0_(gil::at_c<mapping_transform<Layout, L2, 0>::value>(c))
        , v1_(gil::at_c<mapping_transform<Layout, L2, 1>::value>(c))
    {}

    // Support for planar_pixel_iterator construction and dereferencing
    template <typename P>
    homogeneous_color_base(P* p, bool)
        : v0_(&semantic_at_c<0>(*p))
        , v1_(&semantic_at_c<1>(*p))
    {}

    // Support for planar_pixel_reference offset constructor
    template <typename Ptr>
    homogeneous_color_base(Ptr const& ptr, std::ptrdiff_t diff)
        : v0_(*memunit_advanced(semantic_at_c<0>(ptr), diff))
        , v1_(*memunit_advanced(semantic_at_c<1>(ptr), diff))
    {}

    template <typename Ref>
    Ref deref() const
    {
        return Ref(*semantic_at_c<0>(*this), *semantic_at_c<1>(*this));
    }

    auto at(std::integral_constant<int, 0>)
        -> typename element_reference_type<homogeneous_color_base>::type
    { return v0_; }

    auto at(std::integral_constant<int, 0>) const
        -> typename element_const_reference_type<homogeneous_color_base>::type
    { return v0_; }

    auto at(std::integral_constant<int, 1>)
        -> typename element_reference_type<homogeneous_color_base>::type
    { return v1_; }

    auto at(std::integral_constant<int, 1>) const
        -> typename element_const_reference_type<homogeneous_color_base>::type
    { return v1_; }

    // Support for planar_pixel_reference operator[]
    Element at_c_dynamic(std::size_t i) const
    {
        if (i == 0)
            return v0_;
        else
            return v1_;
    }

private:
    Element v0_{};
    Element v1_{};
};

/// \brief A homogeneous color base holding three color elements.
/// Models HomogeneousColorBaseConcept or HomogeneousColorBaseValueConcept
/// \ingroup ColorBaseModelHomogeneous
template <typename Element, typename Layout>
struct homogeneous_color_base<Element, Layout, 3>
{
    using layout_t = Layout;

    homogeneous_color_base() = default;
    explicit homogeneous_color_base(Element v) : v0_(v), v1_(v), v2_(v) {}
    homogeneous_color_base(Element v0, Element v1, Element v2)
        : v0_(v0), v1_(v1), v2_(v2)
    {}

    template <typename E2, typename L2>
    homogeneous_color_base(homogeneous_color_base<E2, L2, 3> const& c)
        : v0_(gil::at_c<mapping_transform<Layout, L2, 0>::value>(c))
        , v1_(gil::at_c<mapping_transform<Layout, L2, 1>::value>(c))
        , v2_(gil::at_c<mapping_transform<Layout, L2, 2>::value>(c))
    {}

    // Support for l-value reference proxy copy construction
    template <typename E2, typename L2>
    homogeneous_color_base(homogeneous_color_base<E2, L2, 3>& c)
        : v0_(gil::at_c<mapping_transform<Layout, L2, 0>::value>(c))
        , v1_(gil::at_c<mapping_transform<Layout, L2, 1>::value>(c))
        , v2_(gil::at_c<mapping_transform<Layout, L2, 2>::value>(c))
    {}

    // Support for planar_pixel_iterator construction and dereferencing
    template <typename P>
    homogeneous_color_base(P* p, bool)
        : v0_(&semantic_at_c<0>(*p))
        , v1_(&semantic_at_c<1>(*p))
        , v2_(&semantic_at_c<2>(*p))
    {}

    // Support for planar_pixel_reference offset constructor
    template <typename Ptr>
    homogeneous_color_base(Ptr const& ptr, std::ptrdiff_t diff)
        : v0_(*memunit_advanced(semantic_at_c<0>(ptr), diff))
        , v1_(*memunit_advanced(semantic_at_c<1>(ptr), diff))
        , v2_(*memunit_advanced(semantic_at_c<2>(ptr), diff))
    {}

    template <typename Ref>
    Ref deref() const
    {
        return Ref(
            *semantic_at_c<0>(*this),
            *semantic_at_c<1>(*this),
            *semantic_at_c<2>(*this));
    }

    auto at(std::integral_constant<int, 0>)
        -> typename element_reference_type<homogeneous_color_base>::type
    { return v0_; }

    auto at(std::integral_constant<int, 0>) const
        -> typename element_const_reference_type<homogeneous_color_base>::type
    { return v0_; }

    auto at(std::integral_constant<int, 1>)
        -> typename element_reference_type<homogeneous_color_base>::type
    { return v1_; }

    auto at(std::integral_constant<int, 1>) const
        -> typename element_const_reference_type<homogeneous_color_base>::type
    { return v1_; }

    auto at(std::integral_constant<int, 2>)
        -> typename element_reference_type<homogeneous_color_base>::type
    { return v2_; }

    auto at(std::integral_constant<int, 2>) const
        -> typename element_const_reference_type<homogeneous_color_base>::type
    { return v2_; }

    // Support for planar_pixel_reference operator[]
    Element at_c_dynamic(std::size_t i) const
    {
        switch (i)
        {
        case 0: return v0_;
        case 1: return v1_;
        }
        return v2_;
    }

private:
    Element v0_{};
    Element v1_{};
    Element v2_{};
};

/// \brief A homogeneous color base holding four color elements.
/// Models HomogeneousColorBaseConcept or HomogeneousColorBaseValueConcept
/// \ingroup ColorBaseModelHomogeneous
template <typename Element, typename Layout>
struct homogeneous_color_base<Element, Layout, 4>
{
    using layout_t = Layout;

    homogeneous_color_base() = default;
    explicit homogeneous_color_base(Element v) : v0_(v), v1_(v), v2_(v), v3_(v) {}
    homogeneous_color_base(Element v0, Element v1, Element v2, Element v3)
        : v0_(v0), v1_(v1), v2_(v2), v3_(v3)
    {}

    template <typename E2, typename L2>
    homogeneous_color_base(homogeneous_color_base<E2, L2, 4> const& c)
        : v0_(gil::at_c<mapping_transform<Layout, L2, 0>::value>(c))
        , v1_(gil::at_c<mapping_transform<Layout, L2, 1>::value>(c))
        , v2_(gil::at_c<mapping_transform<Layout, L2, 2>::value>(c))
        , v3_(gil::at_c<mapping_transform<Layout, L2, 3>::value>(c))
    {}

    // Support for l-value reference proxy copy construction
    template <typename E2, typename L2>
    homogeneous_color_base(homogeneous_color_base<E2, L2, 4>& c)
        : v0_(gil::at_c<mapping_transform<Layout, L2, 0>::value>(c))
        , v1_(gil::at_c<mapping_transform<Layout, L2, 1>::value>(c))
        , v2_(gil::at_c<mapping_transform<Layout, L2, 2>::value>(c))
        , v3_(gil::at_c<mapping_transform<Layout, L2, 3>::value>(c))
    {}

    // Support for planar_pixel_iterator construction and dereferencing
    template <typename P>
    homogeneous_color_base(P * p, bool)
        : v0_(&semantic_at_c<0>(*p))
        , v1_(&semantic_at_c<1>(*p))
        , v2_(&semantic_at_c<2>(*p))
        , v3_(&semantic_at_c<3>(*p))
    {}

    // Support for planar_pixel_reference offset constructor
    template <typename Ptr>
    homogeneous_color_base(Ptr const& ptr, std::ptrdiff_t diff)
        : v0_(*memunit_advanced(semantic_at_c<0>(ptr), diff))
        , v1_(*memunit_advanced(semantic_at_c<1>(ptr), diff))
        , v2_(*memunit_advanced(semantic_at_c<2>(ptr), diff))
        , v3_(*memunit_advanced(semantic_at_c<3>(ptr), diff))
    {}

    template <typename Ref>
    Ref deref() const
    {
        return Ref(
            *semantic_at_c<0>(*this),
            *semantic_at_c<1>(*this),
            *semantic_at_c<2>(*this),
            *semantic_at_c<3>(*this));
    }

    auto at(std::integral_constant<int, 0>)
        -> typename element_reference_type<homogeneous_color_base>::type
    { return v0_; }

    auto at(std::integral_constant<int, 0>) const
        -> typename element_const_reference_type<homogeneous_color_base>::type
    { return v0_; }

    auto at(std::integral_constant<int, 1>)
        -> typename element_reference_type<homogeneous_color_base>::type
    { return v1_; }

    auto at(std::integral_constant<int, 1>) const
        -> typename element_const_reference_type<homogeneous_color_base>::type
    { return v1_; }

    auto at(std::integral_constant<int, 2>)
        -> typename element_reference_type<homogeneous_color_base>::type
    { return v2_; }

    auto at(std::integral_constant<int, 2>) const
        -> typename element_const_reference_type<homogeneous_color_base>::type
    { return v2_; }

    auto at(std::integral_constant<int, 3>)
        -> typename element_reference_type<homogeneous_color_base>::type
    { return v3_; }

    auto at(std::integral_constant<int, 3>) const
        -> typename element_const_reference_type<homogeneous_color_base>::type
    { return v3_; }

    // Support for planar_pixel_reference operator[]
    Element at_c_dynamic(std::size_t i) const
    {
        switch (i)
        {
        case 0: return v0_;
        case 1: return v1_;
        case 2: return v2_;
        }
        return v3_;
    }

private:
    Element v0_{};
    Element v1_{};
    Element v2_{};
    Element v3_{};
};

/// \brief A homogeneous color base holding five color elements.
/// Models HomogeneousColorBaseConcept or HomogeneousColorBaseValueConcept
/// \ingroup ColorBaseModelHomogeneous
template <typename Element, typename Layout>
struct homogeneous_color_base<Element, Layout, 5>
{
    using layout_t = Layout;

    homogeneous_color_base() = default;
    explicit homogeneous_color_base(Element v)
        : v0_(v), v1_(v), v2_(v), v3_(v), v4_(v)
    {}

    homogeneous_color_base(Element v0, Element v1, Element v2, Element v3, Element v4)
        : v0_(v0), v1_(v1), v2_(v2), v3_(v3), v4_(v4)
    {}

    template <typename E2, typename L2>
    homogeneous_color_base(homogeneous_color_base<E2, L2, 5> const& c)
        : v0_(gil::at_c<mapping_transform<Layout, L2, 0>::value>(c))
        , v1_(gil::at_c<mapping_transform<Layout, L2, 1>::value>(c))
        , v2_(gil::at_c<mapping_transform<Layout, L2, 2>::value>(c))
        , v3_(gil::at_c<mapping_transform<Layout, L2, 3>::value>(c))
        , v4_(gil::at_c<mapping_transform<Layout, L2, 4>::value>(c))
    {}

    // Support for l-value reference proxy copy construction
    template <typename E2, typename L2>
    homogeneous_color_base(homogeneous_color_base<E2, L2, 5>& c)
        : v0_(gil::at_c<mapping_transform<Layout, L2, 0>::value>(c))
        , v1_(gil::at_c<mapping_transform<Layout, L2, 1>::value>(c))
        , v2_(gil::at_c<mapping_transform<Layout, L2, 2>::value>(c))
        , v3_(gil::at_c<mapping_transform<Layout, L2, 3>::value>(c))
        , v4_(gil::at_c<mapping_transform<Layout, L2, 4>::value>(c))
    {}

    // Support for planar_pixel_iterator construction and dereferencing
    template <typename P>
    homogeneous_color_base(P* p, bool)
        : v0_(&semantic_at_c<0>(*p))
        , v1_(&semantic_at_c<1>(*p))
        , v2_(&semantic_at_c<2>(*p))
        , v3_(&semantic_at_c<3>(*p))
        , v4_(&semantic_at_c<4>(*p))
    {}

    // Support for planar_pixel_reference offset constructor
    template <typename Ptr>
    homogeneous_color_base(Ptr const& ptr, std::ptrdiff_t diff)
        : v0_(*memunit_advanced(semantic_at_c<0>(ptr), diff))
        , v1_(*memunit_advanced(semantic_at_c<1>(ptr), diff))
        , v2_(*memunit_advanced(semantic_at_c<2>(ptr), diff))
        , v3_(*memunit_advanced(semantic_at_c<3>(ptr), diff))
        , v4_(*memunit_advanced(semantic_at_c<4>(ptr), diff))
    {}


    auto at(std::integral_constant<int, 0>)
        -> typename element_reference_type<homogeneous_color_base>::type
    { return v0_; }

    auto at(std::integral_constant<int, 0>) const
        -> typename element_const_reference_type<homogeneous_color_base>::type
    { return v0_; }

    auto at(std::integral_constant<int, 1>)
        -> typename element_reference_type<homogeneous_color_base>::type
    { return v1_; }

    auto at(std::integral_constant<int, 1>) const
        -> typename element_const_reference_type<homogeneous_color_base>::type
    { return v1_; }

    auto at(std::integral_constant<int, 2>)
        -> typename element_reference_type<homogeneous_color_base>::type
    { return v2_; }

    auto at(std::integral_constant<int, 2>) const
        -> typename element_const_reference_type<homogeneous_color_base>::type
    { return v2_; }

    auto at(std::integral_constant<int, 3>)
        -> typename element_reference_type<homogeneous_color_base>::type
    { return v3_; }

    auto at(std::integral_constant<int, 3>) const
        -> typename element_const_reference_type<homogeneous_color_base>::type
    { return v3_; }

    auto at(std::integral_constant<int, 4>)
        -> typename element_reference_type<homogeneous_color_base>::type
    { return v4_; }

    auto at(std::integral_constant<int, 4>) const
        -> typename element_const_reference_type<homogeneous_color_base>::type
    { return v4_; }

    template <typename Ref>
    Ref deref() const
    {
        return Ref(
            *semantic_at_c<0>(*this),
            *semantic_at_c<1>(*this),
            *semantic_at_c<2>(*this),
            *semantic_at_c<3>(*this),
            *semantic_at_c<4>(*this));
    }

    // Support for planar_pixel_reference operator[]
    Element at_c_dynamic(std::size_t i) const
    {
        switch (i)
        {
        case 0: return v0_;
        case 1: return v1_;
        case 2: return v2_;
        case 3: return v3_;
        }
        return v4_;
    }

private:
    Element v0_{};
    Element v1_{};
    Element v2_{};
    Element v3_{};
    Element v4_{};
};

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(pop)
#endif

// The following way of casting adjacent channels (the contents of color_base) into an array appears to be unsafe
// -- there is no guarantee that the compiler won't add any padding between adjacent channels.
// Note, however, that GIL _must_ be compiled with compiler settings ensuring there is no padding in the color base structs.
// This is because the color base structs must model the interleaved organization in memory. In other words, the client may
// have existing RGB image in the form "RGBRGBRGB..." and we must be able to represent it with an array of RGB color bases (i.e. RGB pixels)
// with no padding. We have tested with char/int/float/double channels on gcc and VC and have so far discovered no problem.
// We have even tried using strange channels consisting of short + char (3 bytes). With the default 4-byte alignment on VC, the size
// of this channel is padded to 4 bytes, so an RGB pixel of it will be 4x3=12 bytes. The code below will still work properly.
// However, the client must nevertheless ensure that proper compiler settings are used for their compiler and their channel types.

template <typename Element, typename Layout, int K>
auto dynamic_at_c(homogeneous_color_base<Element,Layout,K>& cb, std::size_t i)
    -> typename element_reference_type<homogeneous_color_base<Element, Layout, K>>::type
{
    BOOST_ASSERT(i < K);
    return (gil_reinterpret_cast<Element*>(&cb))[i];
}

template <typename Element, typename Layout, int K>
auto dynamic_at_c(homogeneous_color_base<Element, Layout, K> const& cb, std::size_t i)
    -> typename element_const_reference_type
        <
            homogeneous_color_base<Element, Layout, K>
        >::type
{
    BOOST_ASSERT(i < K);
    return (gil_reinterpret_cast_c<const Element*>(&cb))[i];
}

template <typename Element, typename Layout, int K>
auto dynamic_at_c(homogeneous_color_base<Element&, Layout, K> const& cb, std::size_t i)
    -> typename element_reference_type
        <
            homogeneous_color_base<Element&, Layout, K>
        >::type
{
    BOOST_ASSERT(i < K);
    return cb.at_c_dynamic(i);
}

template <typename Element, typename Layout, int K>
auto dynamic_at_c(
    homogeneous_color_base<Element const&, Layout, K>const& cb, std::size_t i)
    -> typename element_const_reference_type
        <
            homogeneous_color_base<Element const&, Layout, K>
        >::type
{
    BOOST_ASSERT(i < K);
    return cb.at_c_dynamic(i);
}

} // namespace detail

template <typename Element, typename Layout, int K1, int K>
struct kth_element_type<detail::homogeneous_color_base<Element, Layout, K1>, K>
{
    using type = Element;
};

template <typename Element, typename Layout, int K1, int K>
struct kth_element_reference_type<detail::homogeneous_color_base<Element, Layout, K1>, K>
    : std::add_lvalue_reference<Element>
{};

template <typename Element, typename Layout, int K1, int K>
struct kth_element_const_reference_type
    <
        detail::homogeneous_color_base<Element, Layout, K1>,
        K
    >
    : std::add_lvalue_reference<typename std::add_const<Element>::type>
{};

/// \brief Provides mutable access to the K-th element, in physical order
/// \ingroup ColorBaseModelHomogeneous
template <int K, typename E, typename L, int N>
inline
auto at_c(detail::homogeneous_color_base<E, L, N>& p)
    -> typename std::add_lvalue_reference<E>::type
{
    return p.at(std::integral_constant<int, K>());
}

/// \brief Provides constant access to the K-th element, in physical order
/// \ingroup ColorBaseModelHomogeneous
template <int K, typename E, typename L, int N>
inline
auto at_c(const detail::homogeneous_color_base<E, L, N>& p)
    -> typename std::add_lvalue_reference<typename std::add_const<E>::type>::type
{
    return p.at(std::integral_constant<int, K>());
}

namespace detail
{

struct swap_fn
{
    template <typename T>
    void operator()(T& x, T& y) const
    {
        using std::swap;
        swap(x, y);
    }
};

} // namespace detail

template <typename E, typename L, int N>
inline
void swap(
    detail::homogeneous_color_base<E, L, N>& x,
    detail::homogeneous_color_base<E, L, N>& y)
{
    static_for_each(x, y, detail::swap_fn());
}

}}  // namespace boost::gil

#endif

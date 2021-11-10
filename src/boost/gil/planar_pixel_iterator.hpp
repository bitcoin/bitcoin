//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_PLANAR_PIXEL_ITERATOR_HPP
#define BOOST_GIL_PLANAR_PIXEL_ITERATOR_HPP

#include <boost/gil/pixel.hpp>
#include <boost/gil/step_iterator.hpp>
#include <boost/gil/detail/mp11.hpp>

#include <boost/iterator/iterator_facade.hpp>

#include <iterator>
#include <type_traits>

namespace boost { namespace gil {

//forward declaration (as this file is included in planar_pixel_reference.hpp)
template <typename ChannelReference, typename ColorSpace>
struct planar_pixel_reference;

/// \defgroup ColorBaseModelPlanarPtr planar_pixel_iterator
/// \ingroup ColorBaseModel
/// \brief A homogeneous color base whose element is a channel iterator. Models HomogeneousColorBaseValueConcept
/// This class is used as an iterator to a planar pixel.

/// \defgroup PixelIteratorModelPlanarPtr planar_pixel_iterator
/// \ingroup PixelIteratorModel
/// \brief An iterator over planar pixels. Models PixelIteratorConcept, HomogeneousPixelBasedConcept, MemoryBasedIteratorConcept, HasDynamicXStepTypeConcept

////////////////////////////////////////////////////////////////////////////////////////
/// \brief An iterator over planar pixels. Models HomogeneousColorBaseConcept, PixelIteratorConcept, HomogeneousPixelBasedConcept, MemoryBasedIteratorConcept, HasDynamicXStepTypeConcept
///
/// Planar pixels have channel data that is not consecutive in memory.
/// To abstract this we use classes to represent references and pointers to planar pixels.
///
/// \ingroup PixelIteratorModelPlanarPtr ColorBaseModelPlanarPtr PixelBasedModel
template <typename ChannelPtr, typename ColorSpace>
struct planar_pixel_iterator
    :
    iterator_facade
    <
        planar_pixel_iterator<ChannelPtr, ColorSpace>,
        pixel<typename std::iterator_traits<ChannelPtr>::value_type,layout<ColorSpace>>,
        std::random_access_iterator_tag,
        planar_pixel_reference<typename std::iterator_traits<ChannelPtr>::reference, ColorSpace> const
    >,
    detail::homogeneous_color_base
    <
        ChannelPtr,
        layout<ColorSpace>,
        mp11::mp_size<ColorSpace>::value
    >
{
private:
    using parent_t = iterator_facade
        <
            planar_pixel_iterator<ChannelPtr, ColorSpace>,
            pixel<typename std::iterator_traits<ChannelPtr>::value_type,layout<ColorSpace>>,
            std::random_access_iterator_tag,
            planar_pixel_reference<typename std::iterator_traits<ChannelPtr>::reference, ColorSpace> const
        >;

    using color_base_parent_t = detail::homogeneous_color_base
        <
            ChannelPtr,
            layout<ColorSpace>,
            mp11::mp_size<ColorSpace>::value
        >;

    using channel_t = typename std::iterator_traits<ChannelPtr>::value_type;

public:
    using value_type = typename parent_t::value_type;
    using reference = typename parent_t::reference;
    using difference_type = typename parent_t::difference_type;

    planar_pixel_iterator() : color_base_parent_t(0) {}
    planar_pixel_iterator(bool) {}        // constructor that does not fill with zero (for performance)

    planar_pixel_iterator(const ChannelPtr& v0, const ChannelPtr& v1) : color_base_parent_t(v0,v1) {}
    planar_pixel_iterator(const ChannelPtr& v0, const ChannelPtr& v1, const ChannelPtr& v2) : color_base_parent_t(v0,v1,v2) {}
    planar_pixel_iterator(const ChannelPtr& v0, const ChannelPtr& v1, const ChannelPtr& v2, const ChannelPtr& v3) : color_base_parent_t(v0,v1,v2,v3) {}
    planar_pixel_iterator(const ChannelPtr& v0, const ChannelPtr& v1, const ChannelPtr& v2, const ChannelPtr& v3, const ChannelPtr& v4) : color_base_parent_t(v0,v1,v2,v3,v4) {}

    template <typename IC1,typename C1>
    planar_pixel_iterator(const planar_pixel_iterator<IC1,C1>& ptr) : color_base_parent_t(ptr) {}

    /// Copy constructor and operator= from pointers to compatible planar pixels or planar pixel references.
    /// That allow constructs like pointer = &value or pointer = &reference
    /// Since we should not override operator& that's the best we can do.
    template <typename P>
    planar_pixel_iterator(P* pix) : color_base_parent_t(pix, true) {
        function_requires<PixelsCompatibleConcept<P,value_type> >();
    }

    struct address_of { template <typename T> T* operator()(T& t) { return &t; } };
    template <typename P>
    planar_pixel_iterator& operator=(P* pix) {
        function_requires<PixelsCompatibleConcept<P,value_type> >();
        static_transform(*pix,*this, address_of());

        // PERFORMANCE_CHECK: Compare to this:
        //this->template semantic_at_c<0>()=&pix->template semantic_at_c<0>();
        //this->template semantic_at_c<1>()=&pix->template semantic_at_c<1>();
        //this->template semantic_at_c<2>()=&pix->template semantic_at_c<2>();
        return *this;
    }

    /// For some reason operator[] provided by iterator_facade returns a custom class that is convertible to reference
    /// We require our own reference because it is registered in iterator_traits
    reference operator[](difference_type d)       const { return memunit_advanced_ref(*this,d*sizeof(channel_t));}

    reference operator->()                        const { return **this; }

    // PERFORMANCE_CHECK: Remove?
    bool operator< (const planar_pixel_iterator& ptr)   const { return gil::at_c<0>(*this)< gil::at_c<0>(ptr); }
    bool operator!=(const planar_pixel_iterator& ptr)   const { return gil::at_c<0>(*this)!=gil::at_c<0>(ptr); }
private:
    friend class boost::iterator_core_access;

    void increment()            { static_transform(*this,*this,detail::inc<ChannelPtr>()); }
    void decrement()            { static_transform(*this,*this,detail::dec<ChannelPtr>()); }
    void advance(std::ptrdiff_t d){ static_transform(*this,*this,std::bind(detail::plus_asymmetric<ChannelPtr,std::ptrdiff_t>(),std::placeholders::_1,d)); }
    reference dereference() const { return this->template deref<reference>(); }

    std::ptrdiff_t distance_to(const planar_pixel_iterator& it) const { return gil::at_c<0>(it)-gil::at_c<0>(*this); }
    bool equal(const planar_pixel_iterator& it) const { return gil::at_c<0>(*this)==gil::at_c<0>(it); }
};

namespace detail {
template <typename I>
struct channel_iterator_is_mutable : std::true_type {};

template <typename I>
struct channel_iterator_is_mutable<I const*> : std::false_type {};

} // namespace detail

template <typename IC, typename C>
struct const_iterator_type<planar_pixel_iterator<IC,C> > {
private:
    using channel_t = typename std::iterator_traits<IC>::value_type;
public:
    using type = planar_pixel_iterator<typename channel_traits<channel_t>::const_pointer,C>;
};

// The default implementation when the iterator is a C pointer is to use the standard constness semantics
template <typename IC, typename C>
struct iterator_is_mutable<planar_pixel_iterator<IC,C> > : public detail::channel_iterator_is_mutable<IC> {};

/////////////////////////////
//  ColorBasedConcept
/////////////////////////////

template <typename IC, typename C, int K>
struct kth_element_type<planar_pixel_iterator<IC, C>, K>
{
    using type = IC;
};

template <typename IC, typename C, int K>
struct kth_element_reference_type<planar_pixel_iterator<IC, C>, K>
    : std::add_lvalue_reference<IC> {};

template <typename IC, typename C, int K>
struct kth_element_const_reference_type<planar_pixel_iterator<IC, C>, K>
    : std::add_lvalue_reference<typename std::add_const<IC>::type>
{};

/////////////////////////////
//  HomogeneousPixelBasedConcept
/////////////////////////////

template <typename IC, typename C>
struct color_space_type<planar_pixel_iterator<IC,C>>
{
    using type = C;
};

template <typename IC, typename C>
struct channel_mapping_type<planar_pixel_iterator<IC, C>>
    : channel_mapping_type<typename planar_pixel_iterator<IC,C>::value_type>
{};

template <typename IC, typename C>
struct is_planar<planar_pixel_iterator<IC, C>> : std::true_type {};

template <typename IC, typename C>
struct channel_type<planar_pixel_iterator<IC, C>>
{
    using type = typename std::iterator_traits<IC>::value_type;
};

/////////////////////////////
//  MemoryBasedIteratorConcept
/////////////////////////////

template <typename IC, typename C>
inline std::ptrdiff_t memunit_step(const planar_pixel_iterator<IC,C>&) { return sizeof(typename std::iterator_traits<IC>::value_type); }

template <typename IC, typename C>
inline std::ptrdiff_t memunit_distance(const planar_pixel_iterator<IC,C>& p1, const planar_pixel_iterator<IC,C>& p2) {
    return memunit_distance(gil::at_c<0>(p1),gil::at_c<0>(p2));
}

template <typename IC>
struct memunit_advance_fn {
    memunit_advance_fn(std::ptrdiff_t diff) : _diff(diff) {}
    IC operator()(const IC& p) const { return memunit_advanced(p,_diff); }

    std::ptrdiff_t _diff;
};

template <typename IC, typename C>
inline void memunit_advance(planar_pixel_iterator<IC,C>& p, std::ptrdiff_t diff) {
    static_transform(p, p, memunit_advance_fn<IC>(diff));
}

template <typename IC, typename C>
inline planar_pixel_iterator<IC,C> memunit_advanced(const planar_pixel_iterator<IC,C>& p, std::ptrdiff_t diff) {
    planar_pixel_iterator<IC,C> ret=p;
    memunit_advance(ret, diff);
    return ret;
}

template <typename ChannelPtr, typename ColorSpace>
inline planar_pixel_reference<typename std::iterator_traits<ChannelPtr>::reference,ColorSpace>
    memunit_advanced_ref(const planar_pixel_iterator<ChannelPtr,ColorSpace>& ptr, std::ptrdiff_t diff) {
    return planar_pixel_reference<typename std::iterator_traits<ChannelPtr>::reference,ColorSpace>(ptr, diff);
}

/////////////////////////////
//  HasDynamicXStepTypeConcept
/////////////////////////////

template <typename IC, typename C>
struct dynamic_x_step_type<planar_pixel_iterator<IC,C> > {
    using type = memory_based_step_iterator<planar_pixel_iterator<IC,C>>;
};
} }  // namespace boost::gil

#endif

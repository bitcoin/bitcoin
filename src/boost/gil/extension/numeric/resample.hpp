//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_NUMERIC_RESAMPLE_HPP
#define BOOST_GIL_EXTENSION_NUMERIC_RESAMPLE_HPP

#include <boost/gil/extension/numeric/affine.hpp>
#include <boost/gil/extension/dynamic_image/dynamic_image_all.hpp>

#include <algorithm>
#include <functional>

namespace boost { namespace gil {

// Support for generic image resampling
// NOTE: The code is for example use only. It is not optimized for performance

///////////////////////////////////////////////////////////////////////////
////
////   resample_pixels: set each pixel in the destination view as the result of a sampling function over the transformed coordinates of the source view
////
///////////////////////////////////////////////////////////////////////////

template <typename MapFn> struct mapping_traits {};

/// \brief Set each pixel in the destination view as the result of a sampling function over the transformed coordinates of the source view
/// \ingroup ImageAlgorithms
///
/// The provided implementation works for 2D image views only
template <typename Sampler,        // Models SamplerConcept
          typename SrcView,        // Models RandomAccess2DImageViewConcept
          typename DstView,        // Models MutableRandomAccess2DImageViewConcept
          typename MapFn>        // Models MappingFunctionConcept
void resample_pixels(const SrcView& src_view, const DstView& dst_view, const MapFn& dst_to_src, Sampler sampler=Sampler())
{
    typename DstView::point_t dst_dims=dst_view.dimensions();
    typename DstView::point_t dst_p;

    for (dst_p.y=0; dst_p.y<dst_dims.y; ++dst_p.y) {
        typename DstView::x_iterator xit = dst_view.row_begin(dst_p.y);
        for (dst_p.x=0; dst_p.x<dst_dims.x; ++dst_p.x) {
            sample(sampler, src_view, transform(dst_to_src, dst_p), xit[dst_p.x]);
        }
    }
}

///////////////////////////////////////////////////////////////////////////
////
////   resample_pixels when one or both image views are run-time instantiated.
////
///////////////////////////////////////////////////////////////////////////

namespace detail {
    template <typename Sampler, typename MapFn>
    struct resample_pixels_fn : public binary_operation_obj<resample_pixels_fn<Sampler,MapFn> > {
        MapFn  _dst_to_src;
        Sampler _sampler;
        resample_pixels_fn(const MapFn& dst_to_src, const Sampler& sampler) : _dst_to_src(dst_to_src), _sampler(sampler) {}

        template <typename SrcView, typename DstView> BOOST_FORCEINLINE void apply_compatible(const SrcView& src, const DstView& dst)  const {
            resample_pixels(src, dst, _dst_to_src, _sampler);
        }
    };
}

/// \brief resample_pixels when the source is run-time specified
///        If invoked on incompatible views, throws std::bad_cast()
/// \ingroup ImageAlgorithms
template <typename Sampler, typename ...Types1, typename V2, typename MapFn>
void resample_pixels(const any_image_view<Types1...>& src, const V2& dst, const MapFn& dst_to_src, Sampler sampler=Sampler())
{
    apply_operation(src, std::bind(
        detail::resample_pixels_fn<Sampler, MapFn>(dst_to_src, sampler),
        std::placeholders::_1,
        dst));
}

/// \brief resample_pixels when the destination is run-time specified
///        If invoked on incompatible views, throws std::bad_cast()
/// \ingroup ImageAlgorithms
template <typename Sampler, typename V1, typename ...Types2, typename MapFn>
void resample_pixels(const V1& src, const any_image_view<Types2...>& dst, const MapFn& dst_to_src, Sampler sampler=Sampler())
{
    using namespace std::placeholders;
    apply_operation(dst, std::bind(
        detail::resample_pixels_fn<Sampler, MapFn>(dst_to_src, sampler),
        src,
        std::placeholders::_1));
}

/// \brief resample_pixels when both the source and the destination are run-time specified
///        If invoked on incompatible views, throws std::bad_cast()
/// \ingroup ImageAlgorithms
template <typename Sampler, typename ...SrcTypes, typename ...DstTypes, typename MapFn>
void resample_pixels(const any_image_view<SrcTypes...>& src, const any_image_view<DstTypes...>& dst, const MapFn& dst_to_src, Sampler sampler=Sampler()) {
    apply_operation(src,dst,detail::resample_pixels_fn<Sampler,MapFn>(dst_to_src,sampler));
}

///////////////////////////////////////////////////////////////////////////
////
////   resample_subimage: copy into the destination a rotated rectangular region from the source, rescaling it to fit into the destination
////
///////////////////////////////////////////////////////////////////////////

// Extract into dst the rotated bounds [src_min..src_max] rotated at 'angle' from the source view 'src'
// The source coordinates are in the coordinate space of the source image
// Note that the views could also be variants (i.e. any_image_view)
template <typename Sampler, typename SrcMetaView, typename DstMetaView>
void resample_subimage(const SrcMetaView& src, const DstMetaView& dst,
                         double src_min_x, double src_min_y,
                         double src_max_x, double src_max_y,
                         double angle, const Sampler& sampler=Sampler()) {
    double src_width  = std::max<double>(src_max_x - src_min_x - 1,1);
    double src_height = std::max<double>(src_max_y - src_min_y - 1,1);
    double dst_width  = std::max<double>((double)(dst.width()-1),1);
    double dst_height = std::max<double>((double)(dst.height()-1),1);

    matrix3x2<double> mat =
        matrix3x2<double>::get_translate(-dst_width/2.0, -dst_height/2.0) *
        matrix3x2<double>::get_scale(src_width / dst_width, src_height / dst_height)*
        matrix3x2<double>::get_rotate(-angle)*
        matrix3x2<double>::get_translate(src_min_x + src_width/2.0, src_min_y + src_height/2.0);
    resample_pixels(src,dst,mat,sampler);
}

///////////////////////////////////////////////////////////////////////////
////
////   resize_view: Copy the source view into the destination, scaling to fit
////
///////////////////////////////////////////////////////////////////////////

template <typename Sampler, typename SrcMetaView, typename DstMetaView>
void resize_view(const SrcMetaView& src, const DstMetaView& dst, const Sampler& sampler=Sampler()) {
    resample_subimage(src,dst,0.0,0.0,(double)src.width(),(double)src.height(),0.0,sampler);
}

} }  // namespace boost::gil

#endif // BOOST_GIL_EXTENSION_NUMERIC_RESAMPLE_HPP

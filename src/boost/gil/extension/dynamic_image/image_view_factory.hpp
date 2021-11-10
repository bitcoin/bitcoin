//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_DYNAMIC_IMAGE_IMAGE_VIEW_FACTORY_HPP
#define BOOST_GIL_EXTENSION_DYNAMIC_IMAGE_IMAGE_VIEW_FACTORY_HPP

#include <boost/gil/extension/dynamic_image/any_image_view.hpp>

#include <boost/gil/dynamic_step.hpp>
#include <boost/gil/image_view_factory.hpp>
#include <boost/gil/point.hpp>
#include <boost/gil/detail/mp11.hpp>

#include <cstdint>

namespace boost { namespace gil {

// Methods for constructing any image views from other any image views
// Extends image view factory to runtime type-specified views (any_image_view)

namespace detail {

template <typename ResultView>
struct flipped_up_down_view_fn
{
    using result_type = ResultView;

    template <typename View>
    auto operator()(View const& src) const -> result_type
    {
        return result_type{flipped_up_down_view(src)};
    }
};

template <typename ResultView>
struct flipped_left_right_view_fn
{
    using result_type = ResultView;

    template <typename View>
    auto operator()(View const& src) const -> result_type
    {
        return result_type{flipped_left_right_view(src)};
    }
};

template <typename ResultView>
struct rotated90cw_view_fn
{
    using result_type = ResultView;

    template <typename View>
    auto operator()(View const& src) const -> result_type
    {
        return result_type{rotated90cw_view(src)};
    }
};

template <typename ResultView>
struct rotated90ccw_view_fn
{
    using result_type = ResultView;

    template <typename View>
    auto operator()(View const& src) const -> result_type
    {
        return result_type{rotated90ccw_view(src)};
    }
};

template <typename ResultView>
struct tranposed_view_fn
{
    using result_type = ResultView;

    template <typename View>
    auto operator()(View const& src) const -> result_type
    {
        return result_type{tranposed_view(src)};
    }
};

template <typename ResultView>
struct rotated180_view_fn
{
    using result_type = ResultView;

    template <typename View>
    auto operator()(View const& src) const -> result_type
    {
        return result_type{rotated180_view(src)};
    }
};

template <typename ResultView>
struct subimage_view_fn
{
    using result_type = ResultView;

    subimage_view_fn(point_t const& topleft, point_t const& dimensions)
        : _topleft(topleft), _size2(dimensions)
    {}

    template <typename View>
    auto operator()(View const& src) const -> result_type
    {
        return result_type{subimage_view(src,_topleft,_size2)};
    }

    point_t _topleft;
    point_t _size2;
};

template <typename ResultView>
struct subsampled_view_fn
{
    using result_type = ResultView;

    subsampled_view_fn(point_t const& step) : _step(step) {}

    template <typename View>
    auto operator()(View const& src) const -> result_type
    {
        return result_type{subsampled_view(src,_step)};
    }

    point_t _step;
};

template <typename ResultView>
struct nth_channel_view_fn
{
    using result_type = ResultView;

    nth_channel_view_fn(int n) : _n(n) {}

    template <typename View>
    auto operator()(View const& src) const -> result_type
    {
        return result_type(nth_channel_view(src,_n));
    }

    int _n;
};

template <typename DstP, typename ResultView, typename CC = default_color_converter>
struct color_converted_view_fn
{
    using result_type = ResultView;

    color_converted_view_fn(CC cc = CC()): _cc(cc) {}

    template <typename View>
    auto operator()(View const& src) const -> result_type
    {
        return result_type{color_converted_view<DstP>(src, _cc)};
    }

private:
    CC _cc;
};

} // namespace detail

/// \ingroup ImageViewTransformationsFlipUD
/// \tparam Views Models Boost.MP11-compatible list of models of ImageViewConcept
template <typename ...Views>
inline
auto flipped_up_down_view(any_image_view<Views...> const& src)
    -> typename dynamic_y_step_type<any_image_view<Views...>>::type
{
    using result_view_t = typename dynamic_y_step_type<any_image_view<Views...>>::type;
    return apply_operation(src, detail::flipped_up_down_view_fn<result_view_t>());
}

/// \ingroup ImageViewTransformationsFlipLR
/// \tparam Views Models Boost.MP11-compatible list of models of ImageViewConcept
template <typename ...Views>
inline
auto flipped_left_right_view(any_image_view<Views...> const& src)
    -> typename dynamic_x_step_type<any_image_view<Views...>>::type
{
    using result_view_t = typename dynamic_x_step_type<any_image_view<Views...>>::type;
    return apply_operation(src, detail::flipped_left_right_view_fn<result_view_t>());
}

/// \ingroup ImageViewTransformationsTransposed
/// \tparam Views Models Boost.MP11-compatible list of models of ImageViewConcept
template <typename ...Views>
inline
auto transposed_view(const any_image_view<Views...>& src)
    -> typename dynamic_xy_step_transposed_type<any_image_view<Views...>>::type
{
    using result_view_t = typename dynamic_xy_step_transposed_type<any_image_view<Views...>>::type;
    return apply_operation(src, detail::tranposed_view_fn<result_view_t>());
}

/// \ingroup ImageViewTransformations90CW
/// \tparam Views Models Boost.MP11-compatible list of models of ImageViewConcept
template <typename ...Views>
inline
auto rotated90cw_view(const any_image_view<Views...>& src)
    -> typename dynamic_xy_step_transposed_type<any_image_view<Views...>>::type
{
    using result_view_t = typename dynamic_xy_step_transposed_type<any_image_view<Views...>>::type;
    return apply_operation(src,detail::rotated90cw_view_fn<result_view_t>());
}

/// \ingroup ImageViewTransformations90CCW
/// \tparam Views Models Boost.MP11-compatible list of models of ImageViewConcept
template <typename ...Views>
inline
auto rotated90ccw_view(const any_image_view<Views...>& src)
    -> typename dynamic_xy_step_transposed_type<any_image_view<Views...>>::type
{
    return apply_operation(src,detail::rotated90ccw_view_fn<typename dynamic_xy_step_transposed_type<any_image_view<Views...>>::type>());
}

/// \ingroup ImageViewTransformations180
/// \tparam Views Models Boost.MP11-compatible list of models of ImageViewConcept
template <typename ...Views>
inline
auto rotated180_view(any_image_view<Views...> const& src)
    -> typename dynamic_xy_step_type<any_image_view<Views...>>::type
{
    using step_type = typename dynamic_xy_step_type<any_image_view<Views...>>::type;
    return apply_operation(src, detail::rotated180_view_fn<step_type>());
}

/// \ingroup ImageViewTransformationsSubimage
/// \tparam Views Models Boost.MP11-compatible list of models of ImageViewConcept
template <typename ...Views>
inline
auto subimage_view(
    any_image_view<Views...> const& src,
    point_t const& topleft,
    point_t const& dimensions)
    -> any_image_view<Views...>
{
    using subimage_view_fn = detail::subimage_view_fn<any_image_view<Views...>>;
    return apply_operation(src, subimage_view_fn(topleft, dimensions));
}

/// \ingroup ImageViewTransformationsSubimage
/// \tparam Views Models Boost.MP11-compatible list of models of ImageViewConcept
template <typename ...Views>
inline
auto subimage_view(
    any_image_view<Views...> const& src,
    std::ptrdiff_t x_min, std::ptrdiff_t y_min, std::ptrdiff_t width, std::ptrdiff_t height)
    -> any_image_view<Views...>
{
    using subimage_view_fn = detail::subimage_view_fn<any_image_view<Views...>>;
    return apply_operation(src, subimage_view_fn(point_t(x_min, y_min),point_t(width, height)));
}

/// \ingroup ImageViewTransformationsSubsampled
/// \tparam Views Models Boost.MP11-compatible list of models of ImageViewConcept
template <typename ...Views>
inline
auto subsampled_view(any_image_view<Views...> const& src, point_t const& step)
    -> typename dynamic_xy_step_type<any_image_view<Views...>>::type
{
    using step_type = typename dynamic_xy_step_type<any_image_view<Views...>>::type;
    using subsampled_view = detail::subsampled_view_fn<step_type>;
    return apply_operation(src, subsampled_view(step));
}

/// \ingroup ImageViewTransformationsSubsampled
/// \tparam Views Models Boost.MP11-compatible list of models of ImageViewConcept
template <typename ...Views>
inline
auto subsampled_view(any_image_view<Views...> const& src, std::ptrdiff_t x_step, std::ptrdiff_t y_step)
    -> typename dynamic_xy_step_type<any_image_view<Views...>>::type
{
    using step_type = typename dynamic_xy_step_type<any_image_view<Views...>>::type;
    using subsampled_view_fn = detail::subsampled_view_fn<step_type>;
    return apply_operation(src, subsampled_view_fn(point_t(x_step, y_step)));
}

namespace detail {

template <typename View>
struct get_nthchannel_type { using type = typename nth_channel_view_type<View>::type; };

template <typename Views>
struct views_get_nthchannel_type : mp11::mp_transform<get_nthchannel_type, Views> {};

} // namespace detail

/// \ingroup ImageViewTransformationsNthChannel
/// \brief Given a runtime source image view, returns the type of a runtime image view over a single channel of the source view
template <typename ...Views>
struct nth_channel_view_type<any_image_view<Views...>>
{
    using type = typename detail::views_get_nthchannel_type<any_image_view<Views...>>;
};

/// \ingroup ImageViewTransformationsNthChannel
/// \tparam Views Models Boost.MP11-compatible list of models of ImageViewConcept
template <typename ...Views>
inline
auto nth_channel_view(const any_image_view<Views...>& src, int n)
    -> typename nth_channel_view_type<any_image_view<Views...>>::type
{
    using result_view_t = typename nth_channel_view_type<any_image_view<Views...>>::type;
    return apply_operation(src,detail::nth_channel_view_fn<result_view_t>(n));
}

namespace detail {

template <typename View, typename DstP, typename CC>
struct get_ccv_type : color_converted_view_type<View, DstP, CC> {};

template <typename Views, typename DstP, typename CC>
struct views_get_ccv_type
{
private:
    // FIXME: Remove class name injection with detail:: qualification
    // Required as workaround for MP11 issue that treats unqualified metafunction
    // in the class definition of the same name as the specialization (Peter Dimov):
    //    invalid template argument for template parameter 'F', expected a class template
    template <typename T>
    using ccvt = detail::get_ccv_type<T, DstP, CC>;

public:
    using type = mp11::mp_transform<ccvt, Views>;
};

} // namespace detail

/// \ingroup ImageViewTransformationsColorConvert
/// \brief Returns the type of a runtime-specified view, color-converted to a given pixel type with user specified color converter
template <typename ...Views, typename DstP, typename CC>
struct color_converted_view_type<any_image_view<Views...>,DstP,CC>
{
    //using type = any_image_view<typename detail::views_get_ccv_type<Views, DstP, CC>::type>;
    using type = detail::views_get_ccv_type<any_image_view<Views...>, DstP, CC>;
};

/// \ingroup ImageViewTransformationsColorConvert
/// \brief overload of generic color_converted_view with user defined color-converter
/// \tparam Views Models Boost.MP11-compatible list of models of ImageViewConcept
template <typename DstP, typename ...Views, typename CC>
inline
auto color_converted_view(const any_image_view<Views...>& src, CC)
    -> typename color_converted_view_type<any_image_view<Views...>, DstP, CC>::type
{
    using cc_view_t = typename color_converted_view_type<any_image_view<Views...>, DstP, CC>::type;
    return apply_operation(src, detail::color_converted_view_fn<DstP, cc_view_t>());
}

/// \ingroup ImageViewTransformationsColorConvert
/// \brief Returns the type of a runtime-specified view, color-converted to a given pixel type with the default coor converter
template <typename ...Views, typename DstP>
struct color_converted_view_type<any_image_view<Views...>,DstP>
{
    using type = detail::views_get_ccv_type<any_image_view<Views...>, DstP, default_color_converter>;
};

/// \ingroup ImageViewTransformationsColorConvert
/// \brief overload of generic color_converted_view with the default color-converter
/// \tparam Views Models Boost.MP11-compatible list of models of ImageViewConcept
template <typename DstP, typename ...Views>
inline
auto color_converted_view(any_image_view<Views...> const& src)
    -> typename color_converted_view_type<any_image_view<Views...>, DstP>::type
{
    using cc_view_t = typename color_converted_view_type<any_image_view<Views...>, DstP>::type;
    return apply_operation(src, detail::color_converted_view_fn<DstP, cc_view_t>());
}

/// \ingroup ImageViewTransformationsColorConvert
/// \brief overload of generic color_converted_view with user defined color-converter
///        These are workarounds for GCC 3.4, which thinks color_converted_view is ambiguous with the same method for templated views (in gil/image_view_factory.hpp)
/// \tparam Views Models Boost.MP11-compatible list of models of ImageViewConcept
template <typename DstP, typename ...Views, typename CC>
inline
auto any_color_converted_view(const any_image_view<Views...>& src, CC)
    -> typename color_converted_view_type<any_image_view<Views...>, DstP, CC>::type
{
    using cc_view_t = typename color_converted_view_type<any_image_view<Views...>, DstP, CC>::type;
    return apply_operation(src, detail::color_converted_view_fn<DstP, cc_view_t>());
}

/// \ingroup ImageViewTransformationsColorConvert
/// \brief overload of generic color_converted_view with the default color-converter
///        These are workarounds for GCC 3.4, which thinks color_converted_view is ambiguous with the same method for templated views (in gil/image_view_factory.hpp)
/// \tparam Views Models Boost.MP11-compatible list of models of ImageViewConcept
template <typename DstP, typename ...Views>
inline
auto any_color_converted_view(const any_image_view<Views...>& src)
    -> typename color_converted_view_type<any_image_view<Views...>, DstP>::type
{
    using cc_view_t = typename color_converted_view_type<any_image_view<Views...>, DstP>::type;
    return apply_operation(src, detail::color_converted_view_fn<DstP, cc_view_t>());
}

/// \}

}}  // namespace boost::gil

#endif

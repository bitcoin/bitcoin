//
// Copyright 2019 Miral Shah <miralshah2211@gmail.com>
//
// Use, modification and distribution are subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_GIL_IMAGE_PROCESSING_THRESHOLD_HPP
#define BOOST_GIL_IMAGE_PROCESSING_THRESHOLD_HPP

#include <limits>
#include <array>
#include <type_traits>
#include <cstddef>
#include <algorithm>
#include <vector>
#include <cmath>

#include <boost/assert.hpp>

#include <boost/gil/image.hpp>
#include <boost/gil/extension/numeric/kernel.hpp>
#include <boost/gil/extension/numeric/convolve.hpp>
#include <boost/gil/image_processing/numeric.hpp>

namespace boost { namespace gil {

namespace detail {

template
<
    typename SourceChannelT,
    typename ResultChannelT,
    typename SrcView,
    typename DstView,
    typename Operator
>
void threshold_impl(SrcView const& src_view, DstView const& dst_view, Operator const& threshold_op)
{
    gil_function_requires<ImageViewConcept<SrcView>>();
    gil_function_requires<MutableImageViewConcept<DstView>>();
    static_assert(color_spaces_are_compatible
    <
        typename color_space_type<SrcView>::type,
        typename color_space_type<DstView>::type
    >::value, "Source and destination views must have pixels with the same color space");

    //iterate over the image checking each pixel value for the threshold
    for (std::ptrdiff_t y = 0; y < src_view.height(); y++)
    {
        typename SrcView::x_iterator src_it = src_view.row_begin(y);
        typename DstView::x_iterator dst_it = dst_view.row_begin(y);

        for (std::ptrdiff_t x = 0; x < src_view.width(); x++)
        {
            static_transform(src_it[x], dst_it[x], threshold_op);
        }
    }
}

} //namespace boost::gil::detail

/// \addtogroup ImageProcessing
/// @{
///
/// \brief Direction of image segmentation.
/// The direction specifies which pixels are considered as corresponding to object
/// and which pixels correspond to background.
enum class threshold_direction
{
    regular, ///< Consider values greater than threshold value
    inverse  ///< Consider values less than or equal to threshold value
};

/// \ingroup ImageProcessing
/// \brief Method of optimal threshold value calculation.
enum class threshold_optimal_value
{
    otsu        ///< \todo TODO
};

/// \ingroup ImageProcessing
/// \brief TODO
enum class threshold_truncate_mode
{
    threshold,  ///< \todo TODO
    zero        ///< \todo TODO
};

enum class threshold_adaptive_method
{
    mean,
    gaussian
};

/// \ingroup ImageProcessing
/// \brief Applies fixed threshold to each pixel of image view.
/// Performs image binarization by thresholding channel value of each
/// pixel of given image view.
/// \param src_view - TODO
/// \param dst_view - TODO
/// \param threshold_value - TODO
/// \param max_value - TODO
/// \param threshold_direction - if regular, values greater than threshold_value are
/// set to max_value else set to 0; if inverse, values greater than threshold_value are
/// set to 0 else set to max_value.
template <typename SrcView, typename DstView>
void threshold_binary(
    SrcView const& src_view,
    DstView const& dst_view,
    typename channel_type<DstView>::type threshold_value,
    typename channel_type<DstView>::type max_value,
    threshold_direction direction = threshold_direction::regular
)
{
    //deciding output channel type and creating functor
    using source_channel_t = typename channel_type<SrcView>::type;
    using result_channel_t = typename channel_type<DstView>::type;

    if (direction == threshold_direction::regular)
    {
        detail::threshold_impl<source_channel_t, result_channel_t>(src_view, dst_view,
            [threshold_value, max_value](source_channel_t px) -> result_channel_t {
                return px > threshold_value ? max_value : 0;
            });
    }
    else
    {
        detail::threshold_impl<source_channel_t, result_channel_t>(src_view, dst_view,
            [threshold_value, max_value](source_channel_t px) -> result_channel_t {
                return px > threshold_value ? 0 : max_value;
            });
    }
}

/// \ingroup ImageProcessing
/// \brief Applies fixed threshold to each pixel of image view.
/// Performs image binarization by thresholding channel value of each
/// pixel of given image view.
/// This variant of threshold_binary automatically deduces maximum value for each channel
/// of pixel based on channel type.
/// If direction is regular, values greater than threshold_value will be set to maximum
/// numeric limit of channel else 0.
/// If direction is inverse, values greater than threshold_value will be set to 0 else maximum
/// numeric limit of channel.
template <typename SrcView, typename DstView>
void threshold_binary(
    SrcView const& src_view,
    DstView const& dst_view,
    typename channel_type<DstView>::type threshold_value,
    threshold_direction direction = threshold_direction::regular
)
{
    //deciding output channel type and creating functor
    using result_channel_t = typename channel_type<DstView>::type;

    result_channel_t max_value = (std::numeric_limits<result_channel_t>::max)();
    threshold_binary(src_view, dst_view, threshold_value, max_value, direction);
}

/// \ingroup ImageProcessing
/// \brief Applies truncating threshold to each pixel of image view.
/// Takes an image view and performs truncating threshold operation on each chennel.
/// If mode is threshold and direction is regular:
/// values greater than threshold_value will be set to threshold_value else no change
/// If mode is threshold and direction is inverse:
/// values less than or equal to threshold_value will be set to threshold_value else no change
/// If mode is zero and direction is regular:
/// values less than or equal to threshold_value will be set to 0 else no change
/// If mode is zero and direction is inverse:
/// values more than threshold_value will be set to 0 else no change
template <typename SrcView, typename DstView>
void threshold_truncate(
    SrcView const& src_view,
    DstView const& dst_view,
    typename channel_type<DstView>::type threshold_value,
    threshold_truncate_mode mode = threshold_truncate_mode::threshold,
    threshold_direction direction = threshold_direction::regular
)
{
    //deciding output channel type and creating functor
    using source_channel_t = typename channel_type<SrcView>::type;
    using result_channel_t = typename channel_type<DstView>::type;

    std::function<result_channel_t(source_channel_t)> threshold_logic;

    if (mode == threshold_truncate_mode::threshold)
    {
        if (direction == threshold_direction::regular)
        {
            detail::threshold_impl<source_channel_t, result_channel_t>(src_view, dst_view,
                [threshold_value](source_channel_t px) -> result_channel_t {
                    return px > threshold_value ? threshold_value : px;
                });
        }
        else
        {
            detail::threshold_impl<source_channel_t, result_channel_t>(src_view, dst_view,
                [threshold_value](source_channel_t px) -> result_channel_t {
                    return px > threshold_value ? px : threshold_value;
                });
        }
    }
    else
    {
        if (direction == threshold_direction::regular)
        {
            detail::threshold_impl<source_channel_t, result_channel_t>(src_view, dst_view,
                [threshold_value](source_channel_t px) -> result_channel_t {
                    return px > threshold_value ? px : 0;
                });
        }
        else
        {
            detail::threshold_impl<source_channel_t, result_channel_t>(src_view, dst_view,
                [threshold_value](source_channel_t px) -> result_channel_t {
                    return px > threshold_value ? 0 : px;
                });
        }
    }
}

namespace detail{

template <typename SrcView, typename DstView>
void otsu_impl(SrcView const& src_view, DstView const& dst_view, threshold_direction direction)
{
    //deciding output channel type and creating functor
    using source_channel_t = typename channel_type<SrcView>::type;

    std::array<std::size_t, 256> histogram{};
    //initial value of min is set to maximum possible value to compare histogram data
    //initial value of max is set to minimum possible value to compare histogram data
    auto min = (std::numeric_limits<source_channel_t>::max)(),
        max = (std::numeric_limits<source_channel_t>::min)();

    if (sizeof(source_channel_t) > 1 || std::is_signed<source_channel_t>::value)
    {
        //iterate over the image to find the min and max pixel values
        for (std::ptrdiff_t y = 0; y < src_view.height(); y++)
        {
            typename SrcView::x_iterator src_it = src_view.row_begin(y);
            for (std::ptrdiff_t x = 0; x < src_view.width(); x++)
            {
                if (src_it[x] < min) min = src_it[x];
                if (src_it[x] > min) min = src_it[x];
            }
        }

        //making histogram
        for (std::ptrdiff_t y = 0; y < src_view.height(); y++)
        {
            typename SrcView::x_iterator src_it = src_view.row_begin(y);

            for (std::ptrdiff_t x = 0; x < src_view.width(); x++)
            {
                histogram[((src_it[x] - min) * 255) / (max - min)]++;
            }
        }
    }
    else
    {
        //making histogram
        for (std::ptrdiff_t y = 0; y < src_view.height(); y++)
        {
            typename SrcView::x_iterator src_it = src_view.row_begin(y);

            for (std::ptrdiff_t x = 0; x < src_view.width(); x++)
            {
                histogram[src_it[x]]++;
            }
        }
    }

    //histData = histogram data
    //sum = total (background + foreground)
    //sumB = sum background
    //wB = weight background
    //wf = weight foreground
    //varMax = tracking the maximum known value of between class variance
    //mB = mu background
    //mF = mu foreground
    //varBeetween = between class variance
    //http://www.labbookpages.co.uk/software/imgProc/otsuThreshold.html
    //https://www.ipol.im/pub/art/2016/158/
    std::ptrdiff_t total_pixel = src_view.height() * src_view.width();
    std::ptrdiff_t sum_total = 0, sum_back = 0;
    std::size_t weight_back = 0, weight_fore = 0, threshold = 0;
    double var_max = 0, mean_back, mean_fore, var_intra_class;

    for (std::size_t t = 0; t < 256; t++)
    {
        sum_total += t * histogram[t];
    }

    for (int t = 0; t < 256; t++)
    {
        weight_back += histogram[t];               // Weight Background
        if (weight_back == 0) continue;

        weight_fore = total_pixel - weight_back;          // Weight Foreground
        if (weight_fore == 0) break;

        sum_back += t * histogram[t];

        mean_back = sum_back / weight_back;            // Mean Background
        mean_fore = (sum_total - sum_back) / weight_fore;    // Mean Foreground

        // Calculate Between Class Variance
        var_intra_class = weight_back * weight_fore * (mean_back - mean_fore) * (mean_back - mean_fore);

        // Check if new maximum found
        if (var_intra_class > var_max) {
            var_max = var_intra_class;
            threshold = t;
        }
    }
    if (sizeof(source_channel_t) > 1 && std::is_unsigned<source_channel_t>::value)
    {
        threshold_binary(src_view, dst_view, (threshold * (max - min) / 255) + min, direction);
    }
    else {
        threshold_binary(src_view, dst_view, threshold, direction);
    }
}
} //namespace detail

template <typename SrcView, typename DstView>
void threshold_optimal
(
    SrcView const& src_view,
    DstView const& dst_view,
    threshold_optimal_value mode = threshold_optimal_value::otsu,
    threshold_direction direction = threshold_direction::regular
)
{
    if (mode == threshold_optimal_value::otsu)
    {
        for (std::size_t i = 0; i < src_view.num_channels(); i++)
        {
            detail::otsu_impl
                (nth_channel_view(src_view, i), nth_channel_view(dst_view, i), direction);
        }
    }
}

namespace detail {

template
<
    typename SourceChannelT,
    typename ResultChannelT,
    typename SrcView,
    typename DstView,
    typename Operator
>
void adaptive_impl
(
    SrcView const& src_view,
    SrcView const& convolved_view,
    DstView const& dst_view,
    Operator const& threshold_op
)
{
    //template argument validation
    gil_function_requires<ImageViewConcept<SrcView>>();
    gil_function_requires<MutableImageViewConcept<DstView>>();

    static_assert(color_spaces_are_compatible
    <
        typename color_space_type<SrcView>::type,
        typename color_space_type<DstView>::type
    >::value, "Source and destination views must have pixels with the same color space");

    //iterate over the image checking each pixel value for the threshold
    for (std::ptrdiff_t y = 0; y < src_view.height(); y++)
    {
        typename SrcView::x_iterator src_it = src_view.row_begin(y);
        typename SrcView::x_iterator convolved_it = convolved_view.row_begin(y);
        typename DstView::x_iterator dst_it = dst_view.row_begin(y);

        for (std::ptrdiff_t x = 0; x < src_view.width(); x++)
        {
            static_transform(src_it[x], convolved_it[x], dst_it[x], threshold_op);
        }
    }
}
} //namespace boost::gil::detail

template <typename SrcView, typename DstView>
void threshold_adaptive
(
    SrcView const& src_view,
    DstView const& dst_view,
    typename channel_type<DstView>::type max_value,
    std::size_t kernel_size,
    threshold_adaptive_method method = threshold_adaptive_method::mean,
    threshold_direction direction = threshold_direction::regular,
    typename channel_type<DstView>::type constant = 0
)
{
    BOOST_ASSERT_MSG((kernel_size % 2 != 0), "Kernel size must be an odd number");

    typedef typename channel_type<SrcView>::type source_channel_t;
    typedef typename channel_type<DstView>::type result_channel_t;

    image<typename SrcView::value_type> temp_img(src_view.width(), src_view.height());
    typename image<typename SrcView::value_type>::view_t temp_view = view(temp_img);
    SrcView temp_conv(temp_view);

    if (method == threshold_adaptive_method::mean)
    {
        std::vector<float> mean_kernel_values(kernel_size, 1.0f/kernel_size);
        kernel_1d<float> kernel(mean_kernel_values.begin(), kernel_size, kernel_size/2);

        detail::convolve_1d
        <
            pixel<float, typename SrcView::value_type::layout_t>
        >(src_view, kernel, temp_view);
    }
    else if (method == threshold_adaptive_method::gaussian)
    {
        detail::kernel_2d<float> kernel = generate_gaussian_kernel(kernel_size, 1.0);
        convolve_2d(src_view, kernel, temp_view);
    }

    if (direction == threshold_direction::regular)
    {
        detail::adaptive_impl<source_channel_t, result_channel_t>(src_view, temp_conv, dst_view,
            [max_value, constant](source_channel_t px, source_channel_t threshold) -> result_channel_t
        { return px > (threshold - constant) ? max_value : 0; });
    }
    else
    {
        detail::adaptive_impl<source_channel_t, result_channel_t>(src_view, temp_conv, dst_view,
            [max_value, constant](source_channel_t px, source_channel_t threshold) -> result_channel_t
        { return px > (threshold - constant) ? 0 : max_value; });
    }
}

template <typename SrcView, typename DstView>
void threshold_adaptive
(
    SrcView const& src_view,
    DstView const& dst_view,
    std::size_t kernel_size,
    threshold_adaptive_method method = threshold_adaptive_method::mean,
    threshold_direction direction = threshold_direction::regular,
    int constant = 0
)
{
    //deciding output channel type and creating functor
    typedef typename channel_type<DstView>::type result_channel_t;

    result_channel_t max_value = (std::numeric_limits<result_channel_t>::max)();

    threshold_adaptive(src_view, dst_view, max_value, kernel_size, method, direction, constant);
}

/// @}

}} //namespace boost::gil

#endif //BOOST_GIL_IMAGE_PROCESSING_THRESHOLD_HPP

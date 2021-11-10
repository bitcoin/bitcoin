#ifndef BOOST_GIL_IMAGE_PROCESSING_HESSIAN_HPP
#define BOOST_GIL_IMAGE_PROCESSING_HESSIAN_HPP

#include <boost/gil/image_view.hpp>
#include <boost/gil/typedefs.hpp>
#include <boost/gil/extension/numeric/kernel.hpp>
#include <stdexcept>

namespace boost { namespace gil {

/// \brief Computes Hessian response
///
/// Computes Hessian response based on computed entries of Hessian matrix, e.g. second order
/// derivates in x and y, and derivatives in both x, y.
/// d stands for derivative, and x or y stand for derivative direction. For example,
/// ddxx means taking two derivatives (gradients) in horizontal direction.
/// Weights change perception of surroinding pixels.
/// Additional filtering is strongly advised.
template <typename GradientView, typename T, typename Allocator, typename OutputView>
inline void compute_hessian_responses(
    GradientView ddxx,
    GradientView dxdy,
    GradientView ddyy,
    const detail::kernel_2d<T, Allocator>& weights,
    OutputView dst)
{
    if (ddxx.dimensions() != ddyy.dimensions()
        || ddyy.dimensions() != dxdy.dimensions()
        || dxdy.dimensions() != dst.dimensions()
        || weights.center_x() != weights.center_y())
    {
        throw std::invalid_argument("dimensions of views are not the same"
            " or weights don't have equal width and height"
            " or weights' dimensions are not odd");
    }
    // Use pixel type of output, as values will be written to output
    using pixel_t = typename std::remove_reference<decltype(std::declval<OutputView>()(0, 0))>::type;

    using channel_t = typename std::remove_reference
        <
            decltype(std::declval<pixel_t>().at(std::integral_constant<int, 0>{}))
        >::type;


    auto center = weights.center_y();
    for (auto y = center; y < dst.height() - center; ++y)
    {
        for (auto x = center; x < dst.width() - center; ++x)
        {
            auto ddxx_i = channel_t();
            auto ddyy_i = channel_t();
            auto dxdy_i = channel_t();
            for (typename OutputView::coord_t w_y = 0; w_y < weights.size(); ++w_y)
            {
                for (typename OutputView::coord_t w_x = 0; w_x < weights.size(); ++w_x)
                {
                    ddxx_i += ddxx(x + w_x - center, y + w_y - center)
                        .at(std::integral_constant<int, 0>{}) * weights.at(w_x, w_y);
                    ddyy_i += ddyy(x + w_x - center, y + w_y - center)
                        .at(std::integral_constant<int, 0>{}) * weights.at(w_x, w_y);
                    dxdy_i += dxdy(x + w_x - center, y + w_y - center)
                        .at(std::integral_constant<int, 0>{}) * weights.at(w_x, w_y);
                }
            }
            auto determinant = ddxx_i * ddyy_i - dxdy_i * dxdy_i;
            dst(x, y).at(std::integral_constant<int, 0>{}) = determinant;
        }
    }
}

}} // namespace boost::gil

#endif

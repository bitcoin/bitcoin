//
// Copyright 2014 Bill Gallafent
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_PREMULTIPLY_HPP
#define BOOST_GIL_PREMULTIPLY_HPP

#include <boost/gil/rgba.hpp>
#include <boost/gil/detail/mp11.hpp>

#include <boost/core/ignore_unused.hpp>

#include <type_traits>

namespace boost { namespace gil {

template <typename SrcP, typename DstP>
struct channel_premultiply
{
    channel_premultiply(SrcP const & src, DstP & dst)
        : src_(src), dst_(dst)
    {}

    template <typename Channel>
    void operator()(Channel /* channel */) const
    {
        // TODO: Explain why 'channel' input paramater is not used, or used as tag only.

        // @todo: need to do a "channel_convert" too, in case the channel types aren't the same?
        get_color(dst_, Channel()) = channel_multiply(get_color(src_,Channel()), alpha_or_max(src_));
    }
    SrcP const & src_;
    DstP & dst_;
};

namespace detail
{
    template <typename SrcP, typename DstP>
    void assign_alpha_if(std::true_type, SrcP const &src, DstP &dst)
    {
        get_color(dst,alpha_t()) = alpha_or_max(src);
    }

    template <typename SrcP, typename DstP>
    void assign_alpha_if(std::false_type, SrcP const& src, DstP& dst)
    {
        // nothing to do
        boost::ignore_unused(src);
        boost::ignore_unused(dst);
    }
}

struct premultiply
{
    template <typename SrcP, typename DstP>
    void operator()(const SrcP& src, DstP& dst) const
    {
        using src_colour_space_t = typename color_space_type<SrcP>::type;
        using dst_colour_space_t = typename color_space_type<DstP>::type;
        using src_colour_channels = mp11::mp_remove<src_colour_space_t, alpha_t>;

        using has_alpha_t = std::integral_constant<bool, mp11::mp_contains<dst_colour_space_t, alpha_t>::value>;
        mp11::mp_for_each<src_colour_channels>(channel_premultiply<SrcP, DstP>(src, dst));
        detail::assign_alpha_if(has_alpha_t(), src, dst);
    }
};

template <typename SrcConstRefP,  // const reference to the source pixel
          typename DstP>          // Destination pixel value (models PixelValueConcept)
class premultiply_deref_fn
{
public:
    using const_t = premultiply_deref_fn<SrcConstRefP, DstP>;
    using value_type = DstP;
    using reference = value_type;      // read-only dereferencing
    using const_reference = const value_type &;
    using argument_type = SrcConstRefP;
    using result_type = reference;
    static constexpr bool is_mutable = false;

    result_type operator()(argument_type srcP) const
    {
        result_type dstP;
        premultiply()(srcP,dstP);
        return dstP;
    }
};

template <typename SrcView, typename DstP>
struct premultiplied_view_type
{
private:
    using src_pix_ref = typename SrcView::const_t::reference;  // const reference to pixel in SrcView
    using deref_t = premultiply_deref_fn<src_pix_ref, DstP>; // the dereference adaptor that performs color conversion
    using add_ref_t = typename SrcView::template add_deref<deref_t>;
public:
    using type = typename add_ref_t::type; // the color converted view type
    static type make(const SrcView& sv) { return add_ref_t::make(sv, deref_t()); }
};

template <typename DstP, typename View> inline
typename premultiplied_view_type<View,DstP>::type premultiply_view(const View& src)
{
    return premultiplied_view_type<View,DstP>::make(src);
}

}} // namespace boost::gil

#endif

//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_DYNAMIC_IMAGE_ANY_IMAGE_VIEW_HPP
#define BOOST_GIL_EXTENSION_DYNAMIC_IMAGE_ANY_IMAGE_VIEW_HPP

#include <boost/gil/dynamic_step.hpp>
#include <boost/gil/image.hpp>
#include <boost/gil/image_view.hpp>
#include <boost/gil/point.hpp>
#include <boost/gil/detail/mp11.hpp>

#include <boost/variant2/variant.hpp>

namespace boost { namespace gil {

template <typename View>
struct dynamic_xy_step_transposed_type;

namespace detail {

template <typename View>
struct get_const_t { using type = typename View::const_t; };

template <typename Views>
struct views_get_const_t : mp11::mp_transform<get_const_t, Views> {};

// works for both image_view and image
struct any_type_get_num_channels
{
    using result_type = int;
    template <typename T>
    result_type operator()(const T&) const { return num_channels<T>::value; }
};

// works for both image_view and image
struct any_type_get_dimensions
{
    using result_type = point<std::ptrdiff_t>;
    template <typename T>
    result_type operator()(const T& v) const { return v.dimensions(); }
};

// works for image_view
struct any_type_get_size
{
    using result_type = std::size_t;
    template <typename T>
    result_type operator()(const T& v) const { return v.size(); }
};

} // namespace detail

////////////////////////////////////////////////////////////////////////////////////////
/// CLASS any_image_view
///
/// \ingroup ImageViewModel
/// \brief Represents a run-time specified image view. Models HasDynamicXStepTypeConcept, HasDynamicYStepTypeConcept, Note that this class does NOT model ImageViewConcept
///
/// Represents a view whose type (color space, layout, planar/interleaved organization, etc) can be specified at run time.
/// It is the runtime equivalent of \p image_view.
/// Some of the requirements of ImageViewConcept, such as the \p value_type alias cannot be fulfilled, since the language does not allow runtime type specification.
/// Other requirements, such as access to the pixels, would be inefficient to provide. Thus \p any_image_view does not fully model ImageViewConcept.
/// However, many algorithms provide overloads taking runtime specified views and thus in many cases \p any_image_view can be used in places taking a view.
///
/// To perform an algorithm on any_image_view, put the algorithm in a function object and invoke it by calling \p apply_operation(runtime_view, algorithm_fn);
////////////////////////////////////////////////////////////////////////////////////////

template <typename ...Views>
class any_image_view : public variant2::variant<Views...>
{
    using parent_t = variant2::variant<Views...>;

public:    
    using const_t = detail::views_get_const_t<any_image_view>;
    using x_coord_t = std::ptrdiff_t;
    using y_coord_t = std::ptrdiff_t;
    using point_t = point<std::ptrdiff_t>;
    using size_type = std::size_t;

    any_image_view() = default;
    any_image_view(any_image_view const& view) : parent_t((parent_t const&)view) {}

    template <typename View>
    explicit any_image_view(View const& view) : parent_t(view) {}

    template <typename ...OtherViews>
    any_image_view(any_image_view<OtherViews...> const& view)
        : parent_t((variant2::variant<OtherViews...> const&)view)
    {}

    any_image_view& operator=(any_image_view const& view)
    {
        parent_t::operator=((parent_t const&)view);
        return *this;
    }

    template <typename View>
    any_image_view& operator=(View const& view)
    {
        parent_t::operator=(view);
        return *this;
    }

    template <typename ...OtherViews>
    any_image_view& operator=(any_image_view<OtherViews...> const& view)
    {
        parent_t::operator=((variant2::variant<OtherViews...> const&)view);
        return *this;
    }

    std::size_t num_channels()  const { return apply_operation(*this, detail::any_type_get_num_channels()); }
    point_t     dimensions()    const { return apply_operation(*this, detail::any_type_get_dimensions()); }
    size_type   size()          const { return apply_operation(*this, detail::any_type_get_size()); }
    x_coord_t   width()         const { return dimensions().x; }
    y_coord_t   height()        const { return dimensions().y; }
};

/////////////////////////////
//  HasDynamicXStepTypeConcept
/////////////////////////////

template <typename ...Views>
struct dynamic_x_step_type<any_image_view<Views...>>
{
private:
    // FIXME: Remove class name injection with gil:: qualification
    // Required as workaround for Boost.MP11 issue that treats unqualified metafunction
    // in the class definition of the same name as the specialization (Peter Dimov):
    //    invalid template argument for template parameter 'F', expected a class template
    template <typename T>
    using dynamic_step_view = typename gil::dynamic_x_step_type<T>::type;

public:
    using type = mp11::mp_transform<dynamic_step_view, any_image_view<Views...>>;
};

/////////////////////////////
//  HasDynamicYStepTypeConcept
/////////////////////////////

template <typename ...Views>
struct dynamic_y_step_type<any_image_view<Views...>>
{
private:
    // FIXME: Remove class name injection with gil:: qualification
    // Required as workaround for Boost.MP11 issue that treats unqualified metafunction
    // in the class definition of the same name as the specialization (Peter Dimov):
    //    invalid template argument for template parameter 'F', expected a class template
    template <typename T>
    using dynamic_step_view = typename gil::dynamic_y_step_type<T>::type;

public:
    using type = mp11::mp_transform<dynamic_step_view, any_image_view<Views...>>;
};

template <typename ...Views>
struct dynamic_xy_step_type<any_image_view<Views...>>
{
private:
    // FIXME: Remove class name injection with gil:: qualification
    // Required as workaround for Boost.MP11 issue that treats unqualified metafunction
    // in the class definition of the same name as the specialization (Peter Dimov):
    //    invalid template argument for template parameter 'F', expected a class template
    template <typename T>
    using dynamic_step_view = typename gil::dynamic_xy_step_type<T>::type;

public:
    using type = mp11::mp_transform<dynamic_step_view, any_image_view<Views...>>;
};

template <typename ...Views>
struct dynamic_xy_step_transposed_type<any_image_view<Views...>>
{
private:
    // FIXME: Remove class name injection with gil:: qualification
    // Required as workaround for Boost.MP11 issue that treats unqualified metafunction
    // in the class definition of the same name as the specialization (Peter Dimov):
    //    invalid template argument for template parameter 'F', expected a class template
    template <typename T>
    using dynamic_step_view = typename gil::dynamic_xy_step_type<T>::type;

public:
    using type = mp11::mp_transform<dynamic_step_view, any_image_view<Views...>>;
};

}}  // namespace boost::gil

#endif

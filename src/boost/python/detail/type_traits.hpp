// Copyright Shreyans Doshi 2017.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PYTHON_DETAIL_TYPE_TRAITS_HPP
# define BOOST_PYTHON_DETAIL_TYPE_TRAITS_HPP


#include <boost/config.hpp>
#ifdef BOOST_NO_CXX11_HDR_TYPE_TRAITS
# include <boost/type_traits/transform_traits.hpp>
# include <boost/type_traits/same_traits.hpp>
# include <boost/type_traits/cv_traits.hpp>
# include <boost/type_traits/is_polymorphic.hpp>
# include <boost/type_traits/composite_traits.hpp>
# include <boost/type_traits/conversion_traits.hpp>
# include <boost/type_traits/add_pointer.hpp>
# include <boost/type_traits/remove_pointer.hpp>
# include <boost/type_traits/is_void.hpp>
# include <boost/type_traits/object_traits.hpp>
# include <boost/type_traits/add_lvalue_reference.hpp>
# include <boost/type_traits/function_traits.hpp>
# include <boost/type_traits/is_scalar.hpp>
# include <boost/type_traits/alignment_traits.hpp>
# include <boost/mpl/bool.hpp>
#else
# include <type_traits>
#endif

# include <boost/type_traits/is_base_and_derived.hpp>
# include <boost/type_traits/alignment_traits.hpp>
# include <boost/type_traits/has_trivial_copy.hpp>


namespace boost { namespace python { namespace detail {

#ifdef BOOST_NO_CXX11_HDR_TYPE_TRAITS
    using boost::alignment_of;
    using boost::add_const;
    using boost::add_cv;
    using boost::add_lvalue_reference;
    using boost::add_pointer;

    using boost::is_array;
    using boost::is_class;
    using boost::is_const;
    using boost::is_convertible;
    using boost::is_enum;
    using boost::is_function;
    using boost::is_integral;
    using boost::is_lvalue_reference;
    using boost::is_member_function_pointer;
    using boost::is_member_pointer;
    using boost::is_pointer;
    using boost::is_polymorphic;
    using boost::is_reference;
    using boost::is_same;
    using boost::is_scalar;
    using boost::is_union;
    using boost::is_void;
    using boost::is_volatile;

    using boost::remove_reference;
    using boost::remove_pointer;
    using boost::remove_cv;
    using boost::remove_const;

    using boost::mpl::true_;
    using boost::mpl::false_;
#else
    using std::alignment_of;
    using std::add_const;
    using std::add_cv;
    using std::add_lvalue_reference;
    using std::add_pointer;

    using std::is_array;
    using std::is_class;
    using std::is_const;
    using std::is_convertible;
    using std::is_enum;
    using std::is_function;
    using std::is_integral;
    using std::is_lvalue_reference;
    using std::is_member_function_pointer;
    using std::is_member_pointer;
    using std::is_pointer;
    using std::is_polymorphic;
    using std::is_reference;
    using std::is_same;
    using std::is_scalar;
    using std::is_union;
    using std::is_void;
    using std::is_volatile;

    using std::remove_reference;
    using std::remove_pointer;
    using std::remove_cv;
    using std::remove_const;

    typedef std::integral_constant<bool, true> true_;
    typedef std::integral_constant<bool, false> false_;
#endif
    using boost::is_base_and_derived;
    using boost::type_with_alignment;
    using boost::has_trivial_copy;
}}} // namespace boost::python::detail


#endif //BOOST_DETAIL_TYPE_TRAITS_HPP

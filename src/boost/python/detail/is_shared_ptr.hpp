// Copyright David Abrahams 2003.
// Copyright Stefan Seefeld 2016.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef boost_python_detail_is_shared_ptr_hpp_
#define boost_python_detail_is_shared_ptr_hpp_

#include <boost/python/detail/is_xxx.hpp>
#include <boost/shared_ptr.hpp>

namespace boost { namespace python { namespace detail { 

BOOST_PYTHON_IS_XXX_DEF(shared_ptr, shared_ptr, 1)
#if !defined(BOOST_NO_CXX11_SMART_PTR)
template <typename T>
struct is_shared_ptr<std::shared_ptr<T> > : std::true_type {};
#endif

}}} // namespace boost::python::detail

#endif

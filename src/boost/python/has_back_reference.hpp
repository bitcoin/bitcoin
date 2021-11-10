// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef HAS_BACK_REFERENCE_DWA2002323_HPP
# define HAS_BACK_REFERENCE_DWA2002323_HPP

# include <boost/python/detail/prefix.hpp>
# include <boost/mpl/bool.hpp>

namespace boost { namespace python { 

// traits class which users can specialize to indicate that a class
// contains a back-reference to its owning PyObject*
template <class T>
struct has_back_reference
  : mpl::false_
{
};


}} // namespace boost::python

#endif // HAS_BACK_REFERENCE_DWA2002323_HPP

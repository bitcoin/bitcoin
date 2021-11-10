// Copyright David Abrahams 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef IS_AUTO_PTR_DWA2003224_HPP
# define IS_AUTO_PTR_DWA2003224_HPP

# ifndef BOOST_NO_AUTO_PTR
#  include <boost/python/detail/is_xxx.hpp>
#  include <memory>
# endif

namespace boost { namespace python { namespace detail { 

# if !defined(BOOST_NO_AUTO_PTR)

BOOST_PYTHON_IS_XXX_DEF(auto_ptr, std::auto_ptr, 1)

# else

template <class T>
struct is_auto_ptr : mpl::false_
{
};

# endif
    
}}} // namespace boost::python::detail

#endif // IS_AUTO_PTR_DWA2003224_HPP

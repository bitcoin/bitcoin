// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef ARG_TO_PYTHON_BASE_DWA200237_HPP
# define ARG_TO_PYTHON_BASE_DWA200237_HPP
# include <boost/python/handle.hpp>

namespace boost { namespace python { namespace converter {

struct registration;

namespace detail
{
  struct BOOST_PYTHON_DECL arg_to_python_base
      : handle<>
  {
      arg_to_python_base(void const volatile* source, registration const&);
  };
}

}}} // namespace boost::python::converter

#endif // ARG_TO_PYTHON_BASE_DWA200237_HPP

// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef CONVERTIBLE_FUNCTION_DWA200278_HPP
# define CONVERTIBLE_FUNCTION_DWA200278_HPP

namespace boost { namespace python { namespace converter { 

typedef void* (*convertible_function)(PyObject*);
    
}}} // namespace boost::python::converter

#endif // CONVERTIBLE_FUNCTION_DWA200278_HPP

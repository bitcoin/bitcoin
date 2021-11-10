// Copyright David Abrahams 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef CONTEXT_RESULT_CONVERTER_DWA2003917_HPP
# define CONTEXT_RESULT_CONVERTER_DWA2003917_HPP

namespace boost { namespace python { namespace converter { 

// A ResultConverter base class used to indicate that this result
// converter should be constructed with the original Python argument
// list.
struct context_result_converter {};

}}} // namespace boost::python::converter

#endif // CONTEXT_RESULT_CONVERTER_DWA2003917_HPP

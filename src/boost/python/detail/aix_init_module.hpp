// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef AIX_INIT_MODULE_DWA2002529_HPP
# define AIX_INIT_MODULE_DWA2002529_HPP
# ifdef _AIX
# include <boost/python/detail/prefix.hpp>
# include <cstdio>
# ifdef __KCC
#  include <iostream> // this works around a problem in KCC 4.0f
# endif 

namespace boost { namespace python { namespace detail { 

extern "C"
{
    typedef PyObject* (*so_load_function)(char*,char*,FILE*);
}

void aix_init_module(so_load_function, char const* name, void (*init_module)());

}}} // namespace boost::python::detail
# endif

#endif // AIX_INIT_MODULE_DWA2002529_HPP

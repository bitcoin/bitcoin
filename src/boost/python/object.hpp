// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef OBJECT_DWA2002612_HPP
# define OBJECT_DWA2002612_HPP

# include <boost/python/ssize_t.hpp>
# include <boost/python/object_core.hpp>
# include <boost/python/object_attributes.hpp>
# include <boost/python/object_items.hpp>
# include <boost/python/object_slices.hpp>
# include <boost/python/object_operators.hpp>
# include <boost/python/converter/arg_to_python.hpp>

namespace boost { namespace python {

    inline ssize_t len(object const& obj)
    {
        ssize_t result = PyObject_Length(obj.ptr());
        if (PyErr_Occurred()) throw_error_already_set();
        return result;
    }

}} // namespace boost::python

#endif // OBJECT_DWA2002612_HPP

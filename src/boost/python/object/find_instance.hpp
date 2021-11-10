// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef FIND_INSTANCE_DWA2002312_HPP
# define FIND_INSTANCE_DWA2002312_HPP

# include <boost/python/type_id.hpp>

namespace boost { namespace python { namespace objects { 

// Given a type_id, find the instance data which corresponds to it, or
// return 0 in case no such type is held.  If null_shared_ptr_only is
// true and the type being sought is a shared_ptr, only find an
// instance if it turns out to be NULL.  Needed for shared_ptr rvalue
// from_python support.
BOOST_PYTHON_DECL void* find_instance_impl(PyObject*, type_info, bool null_shared_ptr_only = false);

}}} // namespace boost::python::objects

#endif // FIND_INSTANCE_DWA2002312_HPP

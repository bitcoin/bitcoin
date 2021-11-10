// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef CLASS_DETAIL_DWA200295_HPP
# define CLASS_DETAIL_DWA200295_HPP

# include <boost/python/handle.hpp>
# include <boost/python/type_id.hpp>

namespace boost { namespace python { namespace objects { 

BOOST_PYTHON_DECL type_handle registered_class_object(type_info id);
BOOST_PYTHON_DECL type_handle class_metatype();
BOOST_PYTHON_DECL type_handle class_type();

}}} // namespace boost::python::object

#endif // CLASS_DETAIL_DWA200295_HPP

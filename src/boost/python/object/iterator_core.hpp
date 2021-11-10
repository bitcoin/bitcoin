// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef ITERATOR_CORE_DWA2002512_HPP
# define ITERATOR_CORE_DWA2002512_HPP

# include <boost/python/object_fwd.hpp>

namespace boost { namespace python { namespace objects {

BOOST_PYTHON_DECL object const& identity_function();
BOOST_PYTHON_DECL void stop_iteration_error();

}}} // namespace boost::python::object

#endif // ITERATOR_CORE_DWA2002512_HPP

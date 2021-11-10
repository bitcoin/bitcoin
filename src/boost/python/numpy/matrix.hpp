// Copyright Jim Bosch 2010-2012.
// Copyright Stefan Seefeld 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef boost_python_numpy_matrix_hpp_
#define boost_python_numpy_matrix_hpp_

/**
 *  @brief Object manager for numpy.matrix.
 */

#include <boost/python.hpp>
#include <boost/python/numpy/numpy_object_mgr_traits.hpp>
#include <boost/python/numpy/ndarray.hpp>
#include <boost/python/numpy/config.hpp>


namespace boost { namespace python { namespace numpy {

/**
 *  @brief A boost.python "object manager" (subclass of object) for numpy.matrix.
 *
 *  @internal numpy.matrix is defined in Python, so object_manager_traits<matrix>::get_pytype()
 *            is implemented by importing numpy and getting the "matrix" attribute of the module.
 *            We then just hope that doesn't get destroyed while we need it, because if we put
 *            a dynamic python object in a static-allocated boost::python::object or handle<>,
 *            bad things happen when Python shuts down.  I think this solution is safe, but I'd
 *            love to get that confirmed.
 */
class BOOST_NUMPY_DECL matrix : public ndarray 
{
  static object construct(object_cref obj, dtype const & dt, bool copy);
  static object construct(object_cref obj, bool copy);
public:

  BOOST_PYTHON_FORWARD_OBJECT_CONSTRUCTORS(matrix, ndarray);

  /// @brief Equivalent to "numpy.matrix(obj,dt,copy)" in Python.
  explicit matrix(object const & obj, dtype const & dt, bool copy=true)
    : ndarray(extract<ndarray>(construct(obj, dt, copy))) {}

  /// @brief Equivalent to "numpy.matrix(obj,copy=copy)" in Python.
  explicit matrix(object const & obj, bool copy=true)
    : ndarray(extract<ndarray>(construct(obj, copy))) {}

  /// \brief Return a view of the matrix with the given dtype.
  matrix view(dtype const & dt) const;

  /// \brief Copy the scalar (deep for all non-object fields).
  matrix copy() const;

  /// \brief Transpose the matrix.
  matrix transpose() const;

};

/**
 *  @brief CallPolicies that causes a function that returns a numpy.ndarray to
 *         return a numpy.matrix instead.
 */
template <typename Base = default_call_policies>
struct as_matrix : Base
{
  static PyObject * postcall(PyObject *, PyObject * result)
  {
    object a = object(handle<>(result));
    numpy::matrix m(a, false);
    Py_INCREF(m.ptr());
    return m.ptr();
  }
};

} // namespace boost::python::numpy

namespace converter 
{

NUMPY_OBJECT_MANAGER_TRAITS(numpy::matrix);

}}} // namespace boost::python::converter

#endif

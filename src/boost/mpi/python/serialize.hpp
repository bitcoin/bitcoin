// Copyright (C) 2006 Douglas Gregor <doug.gregor -at- gmail.com>

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor

/** @file serialize.hpp
 *
 *  This file provides Boost.Serialization support for Python objects
 *  within Boost.MPI. Python objects can be serialized in one of two
 *  ways. The default serialization method involves using the Python
 *  "pickle" module to pickle the Python objects, transmits the
 *  pickled representation, and unpickles the result when
 *  received. For C++ types that have been exposed to Python and
 *  registered with register_serialized(), objects are directly
 *  serialized for transmissing, skipping the pickling step.
 */
#ifndef BOOST_MPI_PYTHON_SERIALIZE_HPP
#define BOOST_MPI_PYTHON_SERIALIZE_HPP

#include <boost/mpi/python/config.hpp>

#include <boost/python/object.hpp>
#include <boost/python/str.hpp>
#include <boost/python/extract.hpp>

#include <map>

#include <boost/function/function3.hpp>

#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>

#include <boost/serialization/split_free.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/array_wrapper.hpp>
#include <boost/smart_ptr/scoped_array.hpp>

#include <boost/assert.hpp>

#include <boost/type_traits/is_fundamental.hpp>

#define BOOST_MPI_PYTHON_FORWARD_ONLY
#include <boost/mpi/python.hpp>

#include "bytesobject.h"

/************************************************************************
 * Boost.Python Serialization Section                                   *
 ************************************************************************/
#if !defined(BOOST_NO_SFINAE) && !defined(BOOST_NO_IS_CONVERTIBLE)
/**
 * @brief Declare IArchive and OArchive as a Boost.Serialization
 * archives that can be used for Python objects.
 *
 * This macro can only be expanded from the global namespace. It only
 * requires that Archiver be forward-declared. IArchiver and OArchiver
 * will only support Serialization of Python objects by pickling
 * them. If the Archiver type should also support "direct"
 * serialization (for C++ types), use
 * BOOST_PYTHON_DIRECT_SERIALIZATION_ARCHIVE instead.
 */
#  define BOOST_PYTHON_SERIALIZATION_ARCHIVE(IArchiver, OArchiver)        \
namespace boost { namespace python { namespace api {    \
  template<typename R, typename T>                      \
  struct enable_binary< IArchiver , R, T> {};           \
                                                        \
  template<typename R, typename T>                      \
  struct enable_binary< OArchiver , R, T> {};           \
} } } 
# else
#  define BOOST_PYTHON_SERIALIZATION_ARCHIVE(IArchiver, OArchiver)
#endif

/**
 * @brief Declare IArchiver and OArchiver as a Boost.Serialization
 * archives that can be used for Python objects and C++ objects
 * wrapped in Python.
 *
 * This macro can only be expanded from the global namespace. It only
 * requires that IArchiver and OArchiver be forward-declared. However,
 * note that you will also need to write
 * BOOST_PYTHON_DIRECT_SERIALIZATION_ARCHIVE_IMPL(IArchiver,
 * OArchiver) in one of your translation units.

DPG PICK UP HERE
 */
#define BOOST_PYTHON_DIRECT_SERIALIZATION_ARCHIVE(IArchiver, OArchiver) \
BOOST_PYTHON_SERIALIZATION_ARCHIVE(IArchiver, OArchiver)                \
namespace boost { namespace python { namespace detail {                 \
template<>                                                              \
BOOST_MPI_PYTHON_DECL direct_serialization_table< IArchiver , OArchiver >& \
 get_direct_serialization_table< IArchiver , OArchiver >();             \
}                                                                       \
                                                                        \
template<>                                                              \
struct has_direct_serialization< IArchiver , OArchiver> : mpl::true_ { }; \
                                                                        \
template<>                                                              \
struct output_archiver< IArchiver > { typedef OArchiver type; };        \
                                                                        \
template<>                                                              \
struct input_archiver< OArchiver > { typedef IArchiver type; };         \
} }

/**
 * @brief Define the implementation for Boost.Serialization archivers
 * that can be used for Python objects and C++ objects wrapped in
 * Python.
 *
 * This macro can only be expanded from the global namespace. It only
 * requires that IArchiver and OArchiver be forward-declared. Before
 * using this macro, you will need to declare IArchiver and OArchiver
 * as direct serialization archives with
 * BOOST_PYTHON_DIRECT_SERIALIZATION_ARCHIVE(IArchiver, OArchiver).
 */
#define BOOST_PYTHON_DIRECT_SERIALIZATION_ARCHIVE_IMPL(IArchiver, OArchiver) \
namespace boost { namespace python { namespace detail {                 \
template                                                                \
  class BOOST_MPI_PYTHON_DECL direct_serialization_table< IArchiver , OArchiver >; \
                                                                        \
template<>                                                              \
 BOOST_MPI_PYTHON_DECL                                                  \
 direct_serialization_table< IArchiver , OArchiver >&                   \
 get_direct_serialization_table< IArchiver , OArchiver >( )             \
{                                                                       \
  static direct_serialization_table< IArchiver, OArchiver > table;      \
  return table;                                                         \
}                                                                       \
} } }

namespace boost { namespace python {

/**
 * INTERNAL ONLY
 *
 * Provides access to the Python "pickle" module from within C++.
 */
class BOOST_MPI_PYTHON_DECL pickle {
  struct data_t;

public:
  static object dumps(object obj, int protocol = -1);
  static object loads(object s);
  
private:
  static void initialize_data();

  static data_t* data;
};

/**
 * @brief Whether the input/output archiver pair has "direct"
 * serialization for C++ objects exposed in Python.
 *
 * Users do not typically need to specialize this trait, as it will be
 * specialized as part of the macro
 * BOOST_PYTHON_DIRECT_SERIALIZATION_ARCHIVE.
 */
template<typename IArchiver, typename OArchiver>
struct has_direct_serialization : mpl::false_ { };

/**
 *  @brief A metafunction that determines the output archiver for the
 *  given input archiver.
 *
 * Users do not typically need to specialize this trait, as it will be
 * specialized as part of the macro
 * BOOST_PYTHON_DIRECT_SERIALIZATION_ARCHIVE.
 */
template<typename IArchiver> struct output_archiver { };

/**
 *  @brief A metafunction that determines the input archiver for the
 *  given output archiver.
 *
 * Users do not typically need to specialize this trait, as it will be
 * specialized as part of the macro
 * BOOST_PYTHON_DIRECT_SERIALIZATION_ARCHIVE.
 *
 */
template<typename OArchiver> struct input_archiver { };

namespace detail {

  /**
   * INTERNAL ONLY
   *
   * This class contains the direct-serialization code for the given
   * IArchiver/OArchiver pair. It is intended to be used as a
   * singleton class, and will be accessed when (de-)serializing a
   * Boost.Python object with an archiver that supports direct
   * serializations. Do not create instances of this class directly:
   * instead, use get_direct_serialization_table.
   */
  template<typename IArchiver, typename OArchiver>
  class BOOST_MPI_PYTHON_DECL direct_serialization_table
  {
  public:
    typedef boost::function3<void, OArchiver&, const object&, const unsigned int>
      saver_t;
    typedef boost::function3<void, IArchiver&, object&, const unsigned int>
      loader_t;

    typedef std::map<PyTypeObject*, std::pair<int, saver_t> > savers_t;
    typedef std::map<int, loader_t> loaders_t;

    /**
     * Retrieve the saver (serializer) associated with the Python
     * object @p obj.
     *
     *   @param obj The object we want to save. Only its (Python) type
     *   is important.
     *
     *   @param descriptor The value of the descriptor associated to
     *   the returned saver. Will be set to zero if no saver was found
     *   for @p obj.
     *
     *   @returns a function object that can be used to serialize this
     *   object (and other objects of the same type), if possible. If
     *   no saver can be found, returns an empty function object..
     */
    saver_t saver(const object& obj, int& descriptor)
    {
      typename savers_t::iterator pos = savers.find(obj.ptr()->ob_type);
      if (pos != savers.end()) {
        descriptor = pos->second.first;
        return pos->second.second;
      }
      else {
        descriptor = 0;
        return saver_t();
      }
    }

    /**
     * Retrieve the loader (deserializer) associated with the given
     * descriptor.
     *
     *  @param descriptor The descriptor number provided by saver()
     *  when determining the saver for this type.
     *
     *  @returns a function object that can be used to deserialize an
     *  object whose type is the same as that corresponding to the
     *  descriptor. If the descriptor is unknown, the return value
     *  will be an empty function object.
     */
    loader_t loader(int descriptor)
    {
      typename loaders_t::iterator pos = loaders.find(descriptor);
      if (pos != loaders.end())
        return pos->second;
      else
        return loader_t();
    }

    /**
     * Register the type T for direct serialization.
     *
     *  @param value A sample value of the type @c T. This may be used
     *  to compute the Python type associated with the C++ type @c T.
     *
     *  @param type The Python type associated with the C++ type @c
     *  T. If not provided, it will be computed from the same value @p
     *  value.
     */
    template<typename T>
    void register_type(const T& value = T(), PyTypeObject* type = 0)
    {
      // If the user did not provide us with a Python type, figure it
      // out for ourselves.
      if (!type) {
        object obj(value);
        type = obj.ptr()->ob_type;
      }

      register_type(default_saver<T>(), default_loader<T>(type), value, type);
    }

    /**
     * Register the type T for direct serialization.
     *
     *  @param saver A function object that will serialize a
     *  Boost.Python object (that represents a C++ object of type @c
     *  T) to an @c OArchive.
     *
     *  @param loader A function object that will deserialize from an
     *  @c IArchive into a Boost.Python object that represents a C++
     *  object of type @c T.
     *
     *  @param value A sample value of the type @c T. This may be used
     *  to compute the Python type associated with the C++ type @c T.
     *
     *  @param type The Python type associated with the C++ type @c
     *  T. If not provided, it will be computed from the same value @p
     *  value.
     */
    template<typename T>
    void register_type(const saver_t& saver, const loader_t& loader, 
                       const T& value = T(), PyTypeObject* type = 0)
    {
      // If the user did not provide us with a Python type, figure it
      // out for ourselves.
      if (!type) {
        object obj(value);
        type = obj.ptr()->ob_type;
      }

      int descriptor = savers.size() + 1;
      if (savers.find(type) != savers.end())
        return;

      savers[type] = std::make_pair(descriptor, saver);
      loaders[descriptor] = loader;
    }

  protected:
    template<typename T>
    struct default_saver {
      void operator()(OArchiver& ar, const object& obj, const unsigned int) {
        T value = extract<T>(obj)();
        ar << value;
      }
    };

    template<typename T>
    struct default_loader {
      default_loader(PyTypeObject* type) : type(type) { }

      void operator()(IArchiver& ar, object& obj, const unsigned int) {
        // If we can, extract the object in place.
        if (!is_fundamental<T>::value && obj && obj.ptr()->ob_type == type) {
          ar >> extract<T&>(obj)();
        } else {
          T value;
          ar >> value;
          obj = object(value);
        }
      }

    private:
      PyTypeObject* type;
    };

    savers_t savers;
    loaders_t loaders;
  };

  /**
   * @brief Retrieve the direct-serialization table for an
   * IArchiver/OArchiver pair.
   *
   * This function is responsible for returning a reference to the
   * singleton direct-serialization table. Its primary template is
   * left undefined, to force the use of an explicit specialization
   * with a definition in a single translation unit. Use the macro
   * BOOST_PYTHON_DIRECT_SERIALIZATION_ARCHIVE_IMPL to define this
   * explicit specialization.
   */
  template<typename IArchiver, typename OArchiver>
  direct_serialization_table<IArchiver, OArchiver>&
  get_direct_serialization_table();
} // end namespace detail 

/**
 * @brief Register the type T for direct serialization.
 *
 * The @c register_serialized function registers a C++ type for direct
 * serialization with the given @c IArchiver/@c OArchiver pair. Direct
 * serialization elides the use of the Python @c pickle package when
 * serializing Python objects that represent C++ values. Direct
 * serialization can be beneficial both to improve serialization
 * performance (Python pickling can be very inefficient) and to permit
 * serialization for Python-wrapped C++ objects that do not support
 * pickling.
 *
 *  @param value A sample value of the type @c T. This may be used
 *  to compute the Python type associated with the C++ type @c T.
 *
 *  @param type The Python type associated with the C++ type @c
 *  T. If not provided, it will be computed from the same value @p
 *  value.
 */
template<typename IArchiver, typename OArchiver, typename T>
void
register_serialized(const T& value = T(), PyTypeObject* type = 0)
{
  detail::direct_serialization_table<IArchiver, OArchiver>& table = 
    detail::get_direct_serialization_table<IArchiver, OArchiver>();
  table.register_type(value, type);
}

namespace detail {

/// Save a Python object by pickling it.
template<typename Archiver>
void 
save_impl(Archiver& ar, const boost::python::object& obj, 
          const unsigned int /*version*/,
          mpl::false_ /*has_direct_serialization*/)
{
  boost::python::object bytes = boost::python::pickle::dumps(obj);
  int   sz    = PyBytes_Size(bytes.ptr());
  char *data  = PyBytes_AsString(bytes.ptr());  
  ar << sz << boost::serialization::make_array(data, sz);
}

/// Try to save a Python object by directly serializing it; fall back
/// on pickling if required.
template<typename Archiver>
void 
save_impl(Archiver& ar, const boost::python::object& obj, 
          const unsigned int version,
          mpl::true_ /*has_direct_serialization*/)
{
  typedef Archiver OArchiver;
  typedef typename input_archiver<OArchiver>::type IArchiver;
  typedef typename direct_serialization_table<IArchiver, OArchiver>::saver_t
    saver_t;

  direct_serialization_table<IArchiver, OArchiver>& table = 
    get_direct_serialization_table<IArchiver, OArchiver>();

  int descriptor = 0;
  if (saver_t saver = table.saver(obj, descriptor)) {
    ar << descriptor;
    saver(ar, obj, version);
  } else {
    // Pickle it
    ar << descriptor;
    detail::save_impl(ar, obj, version, mpl::false_());
  }
}

/// Load a Python object by unpickling it
template<typename Archiver>
void 
load_impl(Archiver& ar, boost::python::object& obj, 
          const unsigned int /*version*/, 
          mpl::false_ /*has_direct_serialization*/)
{
  int len;
  ar >> len;
  boost::scoped_array<char> data(new char[len]);
  ar >> boost::serialization::make_array(data.get(), len);
  boost::python::object bytes(boost::python::handle<>(PyBytes_FromStringAndSize(data.get(), len)));
  obj = boost::python::pickle::loads(bytes);
}

/// Try to load a Python object by directly deserializing it; fall back
/// on unpickling if required.
template<typename Archiver>
void 
load_impl(Archiver& ar, boost::python::object& obj, 
          const unsigned int version,
          mpl::true_ /*has_direct_serialization*/)
{
  typedef Archiver IArchiver;
  typedef typename output_archiver<IArchiver>::type OArchiver;
  typedef typename direct_serialization_table<IArchiver, OArchiver>::loader_t
    loader_t;

  direct_serialization_table<IArchiver, OArchiver>& table = 
    get_direct_serialization_table<IArchiver, OArchiver>();

  int descriptor;
  ar >> descriptor;

  if (descriptor) {
    loader_t loader = table.loader(descriptor);
    BOOST_ASSERT(loader);

    loader(ar, obj, version);
  } else {
    // Unpickle it
    detail::load_impl(ar, obj, version, mpl::false_());
  }
}

} // end namespace detail

template<typename Archiver>
void 
save(Archiver& ar, const boost::python::object& obj, 
     const unsigned int version)
{
  typedef Archiver OArchiver;
  typedef typename input_archiver<OArchiver>::type IArchiver;
  detail::save_impl(ar, obj, version, 
                    has_direct_serialization<IArchiver, OArchiver>());
}

template<typename Archiver>
void 
load(Archiver& ar, boost::python::object& obj, 
     const unsigned int version)
{
  typedef Archiver IArchiver;
  typedef typename output_archiver<IArchiver>::type OArchiver;
  detail::load_impl(ar, obj, version, 
                    has_direct_serialization<IArchiver, OArchiver>());
}

template<typename Archive>
inline void 
serialize(Archive& ar, boost::python::object& obj, const unsigned int version)
{
  boost::serialization::split_free(ar, obj, version);
}

} } // end namespace boost::python

/************************************************************************
 * Boost.MPI-Specific Section                                           *
 ************************************************************************/
namespace boost { namespace mpi {
 class packed_iarchive;
 class packed_oarchive;
} } // end namespace boost::mpi

BOOST_PYTHON_DIRECT_SERIALIZATION_ARCHIVE(
  ::boost::mpi::packed_iarchive,
  ::boost::mpi::packed_oarchive)

namespace boost { namespace mpi { namespace python {

template<typename T>
void
register_serialized(const T& value, PyTypeObject* type)
{
  using boost::python::register_serialized;
  register_serialized<packed_iarchive, packed_oarchive>(value, type);
}

} } } // end namespace boost::mpi::python

#endif // BOOST_MPI_PYTHON_SERIALIZE_HPP

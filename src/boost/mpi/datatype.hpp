// Copyright 2004 The Trustees of Indiana University.
// Copyright 2005 Matthias Troyer.
// Copyright 2006 Douglas Gregor <doug.gregor -at- gmail.com>.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
//           Matthias Troyer

/** @file datatype.hpp
 *
 *  This header provides the mapping from C++ types to MPI data types.
 */
#ifndef BOOST_MPI_DATATYPE_HPP
#define BOOST_MPI_DATATYPE_HPP

#include <boost/mpi/config.hpp>
#include <boost/mpi/datatype_fwd.hpp>
#include <mpi.h>
#include <boost/config.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpi/detail/mpi_datatype_cache.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/archive/basic_archive.hpp>
#include <boost/serialization/item_version_type.hpp>
#include <utility> // for std::pair

#if defined(__cplusplus) && (201103L <= __cplusplus) 
#include <array>
#endif

namespace boost { namespace mpi {

/**
 *  @brief Type trait that determines if there exists a built-in
 *  integer MPI data type for a given C++ type.
 *
 *  This type trait determines when there is a direct mapping from a
 *  C++ type to an MPI data type that is classified as an integer data
 *  type. See @c is_mpi_builtin_datatype for general information about
 *  built-in MPI data types.
 */
template<typename T>
struct is_mpi_integer_datatype
  : public boost::mpl::false_ { };

/**
 *  @brief Type trait that determines if there exists a built-in
 *  floating point MPI data type for a given C++ type.
 *
 *  This type trait determines when there is a direct mapping from a
 *  C++ type to an MPI data type that is classified as a floating
 *  point data type. See @c is_mpi_builtin_datatype for general
 *  information about built-in MPI data types.
 */
template<typename T>
struct is_mpi_floating_point_datatype
  : public boost::mpl::false_ { };

/**
 *  @brief Type trait that determines if there exists a built-in
 *  logical MPI data type for a given C++ type.
 *
 *  This type trait determines when there is a direct mapping from a
 *  C++ type to an MPI data type that is classified as an logical data
 *  type. See @c is_mpi_builtin_datatype for general information about
 *  built-in MPI data types.
 */
template<typename T>
struct is_mpi_logical_datatype
  : public boost::mpl::false_ { };

/**
 *  @brief Type trait that determines if there exists a built-in
 *  complex MPI data type for a given C++ type.
 *
 *  This type trait determines when there is a direct mapping from a
 *  C++ type to an MPI data type that is classified as a complex data
 *  type. See @c is_mpi_builtin_datatype for general information about
 *  built-in MPI data types.
 */
template<typename T>
struct is_mpi_complex_datatype
  : public boost::mpl::false_ { };

/**
 *  @brief Type trait that determines if there exists a built-in
 *  byte MPI data type for a given C++ type.
 *
 *  This type trait determines when there is a direct mapping from a
 *  C++ type to an MPI data type that is classified as an byte data
 *  type. See @c is_mpi_builtin_datatype for general information about
 *  built-in MPI data types.
 */
template<typename T>
struct is_mpi_byte_datatype
  : public boost::mpl::false_ { };

/** @brief Type trait that determines if there exists a built-in MPI
 *  data type for a given C++ type.
 *
 *  This type trait determines when there is a direct mapping from a
 *  C++ type to an MPI type. For instance, the C++ @c int type maps
 *  directly to the MPI type @c MPI_INT. When there is a direct
 *  mapping from the type @c T to an MPI type, @c
 *  is_mpi_builtin_datatype will derive from @c mpl::true_ and the MPI
 *  data type will be accessible via @c get_mpi_datatype. 
 *
 *  In general, users should not need to specialize this
 *  trait. However, if you have an additional C++ type that can map
 *  directly to only of MPI's built-in types, specialize either this
 *  trait or one of the traits corresponding to categories of MPI data
 *  types (@c is_mpi_integer_datatype, @c
 *  is_mpi_floating_point_datatype, @c is_mpi_logical_datatype, @c
 *  is_mpi_complex_datatype, or @c is_mpi_builtin_datatype). @c
 *  is_mpi_builtin_datatype derives @c mpl::true_ if any of the traits
 *  corresponding to MPI data type categories derived @c mpl::true_.
 */
template<typename T>
struct is_mpi_builtin_datatype
  : boost::mpl::or_<is_mpi_integer_datatype<T>,
                    is_mpi_floating_point_datatype<T>,
                    is_mpi_logical_datatype<T>,
                    is_mpi_complex_datatype<T>,
                    is_mpi_byte_datatype<T> >
{
};

/** @brief Type trait that determines if a C++ type can be mapped to
 *  an MPI data type.
 *
 *  This type trait determines if it is possible to build an MPI data
 *  type that represents a C++ data type. When this is the case, @c
 *  is_mpi_datatype derives @c mpl::true_ and the MPI data type will
 *  be accessible via @c get_mpi_datatype.

 *  For any C++ type that maps to a built-in MPI data type (see @c
 *  is_mpi_builtin_datatype), @c is_mpi_data_type is trivially
 *  true. However, any POD ("Plain Old Data") type containing types
 *  that themselves can be represented by MPI data types can itself be
 *  represented as an MPI data type. For instance, a @c point3d class
 *  containing three @c double values can be represented as an MPI
 *  data type. To do so, first make the data type Serializable (using
 *  the Boost.Serialization library); then, specialize the @c
 *  is_mpi_datatype trait for the point type so that it will derive @c
 *  mpl::true_:
 *
 *    @code
 *    namespace boost { namespace mpi {
 *      template<> struct is_mpi_datatype<point>
 *        : public mpl::true_ { };
 *    } }
 *    @endcode
 */
template<typename T>
struct is_mpi_datatype
 : public is_mpi_builtin_datatype<T>
{
};

/** @brief Returns an MPI data type for a C++ type.
 *
 *  The function creates an MPI data type for the given object @c
 *  x. The first time it is called for a class @c T, the MPI data type
 *  is created and cached. Subsequent calls for objects of the same
 *  type @c T return the cached MPI data type.  The type @c T must
 *  allow creation of an MPI data type. That is, it must be
 *  Serializable and @c is_mpi_datatype<T> must derive @c mpl::true_.
 *
 *  For fundamental MPI types, a copy of the MPI data type of the MPI
 *  library is returned.
 *
 *  Note that since the data types are cached, the caller should never
 *  call @c MPI_Type_free() for the MPI data type returned by this
 *  call.
 *
 *  @param x for an optimized call, a constructed object of the type
 *  should be passed; otherwise, an object will be
 *  default-constructed.
 *
 *  @returns The MPI data type corresponding to type @c T.
 */
template<typename T> MPI_Datatype get_mpi_datatype(const T& x)
{
  BOOST_MPL_ASSERT((is_mpi_datatype<T>));
  return detail::mpi_datatype_cache().datatype(x);
}

// Don't parse this part when we're generating Doxygen documentation.
#ifndef BOOST_MPI_DOXYGEN

/// INTERNAL ONLY
#define BOOST_MPI_DATATYPE(CppType, MPIType, Kind)                      \
template<>                                                              \
inline MPI_Datatype                                                     \
get_mpi_datatype< CppType >(const CppType&) { return MPIType; }         \
                                                                        \
template<>                                                              \
 struct BOOST_JOIN(is_mpi_,BOOST_JOIN(Kind,_datatype))< CppType >       \
: boost::mpl::true_                                                     \
{}

/// INTERNAL ONLY
BOOST_MPI_DATATYPE(packed, MPI_PACKED, builtin);

/// INTERNAL ONLY
BOOST_MPI_DATATYPE(char, MPI_CHAR, builtin);

/// INTERNAL ONLY
BOOST_MPI_DATATYPE(short, MPI_SHORT, integer);

/// INTERNAL ONLY
BOOST_MPI_DATATYPE(int, MPI_INT, integer);

/// INTERNAL ONLY
BOOST_MPI_DATATYPE(long, MPI_LONG, integer);

/// INTERNAL ONLY
BOOST_MPI_DATATYPE(float, MPI_FLOAT, floating_point);

/// INTERNAL ONLY
BOOST_MPI_DATATYPE(double, MPI_DOUBLE, floating_point);

/// INTERNAL ONLY
BOOST_MPI_DATATYPE(long double, MPI_LONG_DOUBLE, floating_point);

/// INTERNAL ONLY
BOOST_MPI_DATATYPE(unsigned char, MPI_UNSIGNED_CHAR, builtin);

/// INTERNAL ONLY
BOOST_MPI_DATATYPE(unsigned short, MPI_UNSIGNED_SHORT, integer);

/// INTERNAL ONLY
BOOST_MPI_DATATYPE(unsigned, MPI_UNSIGNED, integer);

/// INTERNAL ONLY
BOOST_MPI_DATATYPE(unsigned long, MPI_UNSIGNED_LONG, integer);

/// INTERNAL ONLY
#define BOOST_MPI_LIST2(A, B) A, B
/// INTERNAL ONLY
BOOST_MPI_DATATYPE(std::pair<BOOST_MPI_LIST2(float, int)>, MPI_FLOAT_INT, 
                   builtin);
/// INTERNAL ONLY
BOOST_MPI_DATATYPE(std::pair<BOOST_MPI_LIST2(double, int)>, MPI_DOUBLE_INT, 
                   builtin);
/// INTERNAL ONLY
BOOST_MPI_DATATYPE(std::pair<BOOST_MPI_LIST2(long double, int)>,
                   MPI_LONG_DOUBLE_INT, builtin);
/// INTERNAL ONLY
BOOST_MPI_DATATYPE(std::pair<BOOST_MPI_LIST2(long, int>), MPI_LONG_INT, 
                   builtin);
/// INTERNAL ONLY
BOOST_MPI_DATATYPE(std::pair<BOOST_MPI_LIST2(short, int>), MPI_SHORT_INT, 
                   builtin);
/// INTERNAL ONLY
BOOST_MPI_DATATYPE(std::pair<BOOST_MPI_LIST2(int, int>), MPI_2INT, builtin);
#undef BOOST_MPI_LIST2

/// specialization of is_mpi_datatype for pairs
template <class T, class U>
struct is_mpi_datatype<std::pair<T,U> >
 : public mpl::and_<is_mpi_datatype<T>,is_mpi_datatype<U> >
{
};

/// specialization of is_mpi_datatype for arrays
#if defined(__cplusplus) && (201103L <= __cplusplus)
template<class T, std::size_t N>
struct is_mpi_datatype<std::array<T, N> >
 : public is_mpi_datatype<T>
{
};
#endif

// Define wchar_t specialization of is_mpi_datatype, if possible.
#if !defined(BOOST_NO_INTRINSIC_WCHAR_T) && \
  (defined(MPI_WCHAR) || (BOOST_MPI_VERSION >= 2))
BOOST_MPI_DATATYPE(wchar_t, MPI_WCHAR, builtin);
#endif

// Define long long or __int64 specialization of is_mpi_datatype, if possible.
#if defined(BOOST_HAS_LONG_LONG) && \
  (defined(MPI_LONG_LONG_INT) || (BOOST_MPI_VERSION >= 2))
BOOST_MPI_DATATYPE(long long, MPI_LONG_LONG_INT, builtin);
#elif defined(BOOST_HAS_MS_INT64) && \
  (defined(MPI_LONG_LONG_INT) || (BOOST_MPI_VERSION >= 2))
BOOST_MPI_DATATYPE(__int64, MPI_LONG_LONG_INT, builtin); 
#endif

// Define unsigned long long or unsigned __int64 specialization of
// is_mpi_datatype, if possible. We separate this from the check for
// the (signed) long long/__int64 because some MPI implementations
// (e.g., MPICH-MX) have MPI_LONG_LONG_INT but not
// MPI_UNSIGNED_LONG_LONG.
#if defined(BOOST_HAS_LONG_LONG) && \
  (defined(MPI_UNSIGNED_LONG_LONG) \
   || (BOOST_MPI_VERSION >= 2))
BOOST_MPI_DATATYPE(unsigned long long, MPI_UNSIGNED_LONG_LONG, builtin);
#elif defined(BOOST_HAS_MS_INT64) && \
  (defined(MPI_UNSIGNED_LONG_LONG) \
   || (BOOST_MPI_VERSION >= 2))
BOOST_MPI_DATATYPE(unsigned __int64, MPI_UNSIGNED_LONG_LONG, builtin); 
#endif

// Define signed char specialization of is_mpi_datatype, if possible.
#if defined(MPI_SIGNED_CHAR) || (BOOST_MPI_VERSION >= 2)
BOOST_MPI_DATATYPE(signed char, MPI_SIGNED_CHAR, builtin);
#endif


#endif // Doxygen

namespace detail {
  inline MPI_Datatype build_mpi_datatype_for_bool()
  {
    MPI_Datatype type;
    MPI_Type_contiguous(sizeof(bool), MPI_BYTE, &type);
    MPI_Type_commit(&type);
    return type;
  }
}

/// Support for bool. There is no corresponding MPI_BOOL.
/// INTERNAL ONLY
template<>
inline MPI_Datatype get_mpi_datatype<bool>(const bool&)
{
  static MPI_Datatype type = detail::build_mpi_datatype_for_bool();
  return type;
}

/// INTERNAL ONLY
template<>
struct is_mpi_datatype<bool>
  : boost::mpl::bool_<true>
{};


#ifndef BOOST_MPI_DOXYGEN
// direct support for special primitive data types of the serialization library
BOOST_MPI_DATATYPE(boost::archive::library_version_type, get_mpi_datatype(uint_least16_t()), integer);
BOOST_MPI_DATATYPE(boost::archive::version_type, get_mpi_datatype(uint_least8_t()), integer);
BOOST_MPI_DATATYPE(boost::archive::class_id_type, get_mpi_datatype(int_least16_t()), integer);
BOOST_MPI_DATATYPE(boost::archive::class_id_reference_type, get_mpi_datatype(int_least16_t()), integer);
BOOST_MPI_DATATYPE(boost::archive::class_id_optional_type, get_mpi_datatype(int_least16_t()), integer);
BOOST_MPI_DATATYPE(boost::archive::object_id_type, get_mpi_datatype(uint_least32_t()), integer);
BOOST_MPI_DATATYPE(boost::archive::object_reference_type, get_mpi_datatype(uint_least32_t()), integer);
BOOST_MPI_DATATYPE(boost::archive::tracking_type, get_mpi_datatype(bool()), builtin);
BOOST_MPI_DATATYPE(boost::serialization::collection_size_type, get_mpi_datatype(std::size_t()), integer);
BOOST_MPI_DATATYPE(boost::serialization::item_version_type, get_mpi_datatype(uint_least8_t()), integer);
#endif // Doxygen


} } // end namespace boost::mpi

// direct support for special primitive data types of the serialization library
// in the case of homogeneous systems
// define a macro to make explicit designation of this more transparent
#define BOOST_IS_MPI_DATATYPE(T)              \
namespace boost {                             \
namespace mpi {                               \
template<>                                    \
struct is_mpi_datatype< T > : mpl::true_ {};  \
}}                                            \
/**/


#endif // BOOST_MPI_MPI_DATATYPE_HPP

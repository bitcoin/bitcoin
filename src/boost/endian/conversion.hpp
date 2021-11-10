//  boost/endian/conversion.hpp  -------------------------------------------------------//

//  Copyright Beman Dawes 2010, 2011, 2014

//  Distributed under the Boost Software License, Version 1.0.
//  http://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_ENDIAN_CONVERSION_HPP
#define BOOST_ENDIAN_CONVERSION_HPP

#include <boost/endian/detail/endian_reverse.hpp>
#include <boost/endian/detail/endian_load.hpp>
#include <boost/endian/detail/endian_store.hpp>
#include <boost/endian/detail/order.hpp>
#include <boost/type_traits/is_class.hpp>
#include <boost/type_traits/is_array.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/static_assert.hpp>
#include <boost/cstdint.hpp>
#include <boost/config.hpp>

//------------------------------------- synopsis ---------------------------------------//

namespace boost
{
namespace endian
{

//--------------------------------------------------------------------------------------//
//                                                                                      //
//                             return-by-value interfaces                               //
//                             suggested by Phil Endecott                               //
//                                                                                      //
//                             user-defined types (UDTs)                                //
//                                                                                      //
//  All return-by-value conversion function templates are required to be implemented in //
//  terms of an unqualified call to "endian_reverse(x)", a function returning the       //
//  value of x with endianness reversed. This provides a customization point for any    //
//  UDT that provides a "endian_reverse" free-function meeting the requirements.        //
//  It must be defined in the same namespace as the UDT itself so that it will be found //
//  by argument dependent lookup (ADL).                                                 //
//                                                                                      //
//--------------------------------------------------------------------------------------//

  //  reverse byte order
  //  requires T to be a non-bool integral type
  //  in detail/endian_reverse.hpp
  //
  //  template<class T> inline BOOST_CONSTEXPR T endian_reverse( T x ) BOOST_NOEXCEPT;

  //  reverse byte order unless native endianness is big
  template <class EndianReversible >
    inline BOOST_CONSTEXPR EndianReversible big_to_native(EndianReversible x) BOOST_NOEXCEPT;
    //  Returns: x if native endian order is big, otherwise endian_reverse(x)
  template <class EndianReversible >
    inline BOOST_CONSTEXPR EndianReversible native_to_big(EndianReversible x) BOOST_NOEXCEPT;
    //  Returns: x if native endian order is big, otherwise endian_reverse(x)

  //  reverse byte order unless native endianness is little
  template <class EndianReversible >
    inline BOOST_CONSTEXPR EndianReversible little_to_native(EndianReversible x) BOOST_NOEXCEPT;
    //  Returns: x if native endian order is little, otherwise endian_reverse(x)
  template <class EndianReversible >
    inline BOOST_CONSTEXPR EndianReversible native_to_little(EndianReversible x) BOOST_NOEXCEPT;
    //  Returns: x if native endian order is little, otherwise endian_reverse(x)

  //  generic conditional reverse byte order
  template <BOOST_SCOPED_ENUM(order) From, BOOST_SCOPED_ENUM(order) To,
    class EndianReversible>
      inline BOOST_CONSTEXPR EndianReversible conditional_reverse(EndianReversible from) BOOST_NOEXCEPT;
    //  Returns: If From == To have different values, from.
    //           Otherwise endian_reverse(from).
    //  Remarks: The From == To test, and as a consequence which form the return takes, is
    //           is determined at compile time.

  //  runtime conditional reverse byte order
  template <class EndianReversible >
    inline BOOST_CONSTEXPR EndianReversible conditional_reverse(EndianReversible from,
      BOOST_SCOPED_ENUM(order) from_order, BOOST_SCOPED_ENUM(order) to_order)
        BOOST_NOEXCEPT;
      //  Returns: from_order == to_order ? from : endian_reverse(from).

  //------------------------------------------------------------------------------------//


  //  Q: What happened to bswap, htobe, and the other synonym functions based on names
  //     popularized by BSD, OS X, and Linux?
  //  A: Turned out these may be implemented as macros on some systems. Ditto POSIX names
  //     for such functionality. Since macros would cause endless problems with functions
  //     of the same names, and these functions are just synonyms anyhow, they have been
  //     removed.


  //------------------------------------------------------------------------------------//
  //                                                                                    //
  //                            reverse in place interfaces                             //
  //                                                                                    //
  //                             user-defined types (UDTs)                              //
  //                                                                                    //
  //  All reverse in place function templates are required to be implemented in terms   //
  //  of an unqualified call to "endian_reverse_inplace(x)", a function reversing       //
  //  the endianness of x, which is a non-const reference. This provides a              //
  //  customization point for any UDT that provides a "reverse_inplace" free-function   //
  //  meeting the requirements. The free-function must be declared in the same          //
  //  namespace as the UDT itself so that it will be found by argument-dependent        //
  //   lookup (ADL).                                                                    //
  //                                                                                    //
  //------------------------------------------------------------------------------------//

  //  reverse in place
  //  in detail/endian_reverse.hpp
  //
  //  template <class EndianReversible>
  //    inline void endian_reverse_inplace(EndianReversible& x) BOOST_NOEXCEPT;
  //
  //  Effects: x = endian_reverse(x)

  //  reverse in place unless native endianness is big
  template <class EndianReversibleInplace>
    inline void big_to_native_inplace(EndianReversibleInplace& x) BOOST_NOEXCEPT;
    //  Effects: none if native byte-order is big, otherwise endian_reverse_inplace(x)
  template <class EndianReversibleInplace>
    inline void native_to_big_inplace(EndianReversibleInplace& x) BOOST_NOEXCEPT;
    //  Effects: none if native byte-order is big, otherwise endian_reverse_inplace(x)

  //  reverse in place unless native endianness is little
  template <class EndianReversibleInplace>
    inline void little_to_native_inplace(EndianReversibleInplace& x) BOOST_NOEXCEPT;
    //  Effects: none if native byte-order is little, otherwise endian_reverse_inplace(x);
  template <class EndianReversibleInplace>
    inline void native_to_little_inplace(EndianReversibleInplace& x) BOOST_NOEXCEPT;
    //  Effects: none if native byte-order is little, otherwise endian_reverse_inplace(x);

  //  generic conditional reverse in place
  template <BOOST_SCOPED_ENUM(order) From, BOOST_SCOPED_ENUM(order) To,
    class EndianReversibleInplace>
  inline void conditional_reverse_inplace(EndianReversibleInplace& x) BOOST_NOEXCEPT;

  //  runtime reverse in place
  template <class EndianReversibleInplace>
  inline void conditional_reverse_inplace(EndianReversibleInplace& x,
    BOOST_SCOPED_ENUM(order) from_order,  BOOST_SCOPED_ENUM(order) to_order)
    BOOST_NOEXCEPT;

//----------------------------------- end synopsis -------------------------------------//

template <class EndianReversible>
inline BOOST_CONSTEXPR EndianReversible big_to_native( EndianReversible x ) BOOST_NOEXCEPT
{
    return boost::endian::conditional_reverse<order::big, order::native>( x );
}

template <class EndianReversible>
inline BOOST_CONSTEXPR EndianReversible native_to_big( EndianReversible x ) BOOST_NOEXCEPT
{
    return boost::endian::conditional_reverse<order::native, order::big>( x );
}

template <class EndianReversible>
inline BOOST_CONSTEXPR EndianReversible little_to_native( EndianReversible x ) BOOST_NOEXCEPT
{
    return boost::endian::conditional_reverse<order::little, order::native>( x );
}

template <class EndianReversible>
inline BOOST_CONSTEXPR EndianReversible native_to_little( EndianReversible x ) BOOST_NOEXCEPT
{
    return boost::endian::conditional_reverse<order::native, order::little>( x );
}

namespace detail
{

template<class EndianReversible>
inline BOOST_CONSTEXPR EndianReversible conditional_reverse_impl( EndianReversible x, boost::true_type ) BOOST_NOEXCEPT
{
    return x;
}

template<class EndianReversible>
inline BOOST_CONSTEXPR EndianReversible conditional_reverse_impl( EndianReversible x, boost::false_type ) BOOST_NOEXCEPT
{
    return endian_reverse( x );
}

} // namespace detail

// generic conditional reverse
template <BOOST_SCOPED_ENUM(order) From, BOOST_SCOPED_ENUM(order) To, class EndianReversible>
inline BOOST_CONSTEXPR EndianReversible conditional_reverse( EndianReversible x ) BOOST_NOEXCEPT
{
    BOOST_STATIC_ASSERT( boost::is_class<EndianReversible>::value || detail::is_endian_reversible<EndianReversible>::value );
    return detail::conditional_reverse_impl( x, boost::integral_constant<bool, From == To>() );
}

// runtime conditional reverse
template <class EndianReversible>
inline BOOST_CONSTEXPR EndianReversible conditional_reverse( EndianReversible x,
    BOOST_SCOPED_ENUM(order) from_order, BOOST_SCOPED_ENUM(order) to_order ) BOOST_NOEXCEPT
{
    BOOST_STATIC_ASSERT( boost::is_class<EndianReversible>::value || detail::is_endian_reversible<EndianReversible>::value );
    return from_order == to_order? x: endian_reverse( x );
}

//--------------------------------------------------------------------------------------//
//                           reverse-in-place implementation                            //
//--------------------------------------------------------------------------------------//

template <class EndianReversibleInplace>
inline void big_to_native_inplace( EndianReversibleInplace& x ) BOOST_NOEXCEPT
{
    boost::endian::conditional_reverse_inplace<order::big, order::native>( x );
}

template <class EndianReversibleInplace>
inline void native_to_big_inplace( EndianReversibleInplace& x ) BOOST_NOEXCEPT
{
    boost::endian::conditional_reverse_inplace<order::native, order::big>( x );
}

template <class EndianReversibleInplace>
inline void little_to_native_inplace( EndianReversibleInplace& x ) BOOST_NOEXCEPT
{
    boost::endian::conditional_reverse_inplace<order::little, order::native>( x );
}

template <class EndianReversibleInplace>
inline void native_to_little_inplace( EndianReversibleInplace& x ) BOOST_NOEXCEPT
{
    boost::endian::conditional_reverse_inplace<order::native, order::little>( x );
}

namespace detail
{

template<class EndianReversibleInplace>
inline void conditional_reverse_inplace_impl( EndianReversibleInplace&, boost::true_type ) BOOST_NOEXCEPT
{
}

template<class EndianReversibleInplace>
inline void conditional_reverse_inplace_impl( EndianReversibleInplace& x, boost::false_type ) BOOST_NOEXCEPT
{
    endian_reverse_inplace( x );
}

}  // namespace detail

// generic conditional reverse in place
template <BOOST_SCOPED_ENUM(order) From, BOOST_SCOPED_ENUM(order) To, class EndianReversibleInplace>
inline void conditional_reverse_inplace( EndianReversibleInplace& x ) BOOST_NOEXCEPT
{
    BOOST_STATIC_ASSERT(
        boost::is_class<EndianReversibleInplace>::value ||
        boost::is_array<EndianReversibleInplace>::value ||
        detail::is_endian_reversible_inplace<EndianReversibleInplace>::value );

    detail::conditional_reverse_inplace_impl( x, boost::integral_constant<bool, From == To>() );
}

// runtime reverse in place
template <class EndianReversibleInplace>
inline void conditional_reverse_inplace( EndianReversibleInplace& x,
    BOOST_SCOPED_ENUM(order) from_order, BOOST_SCOPED_ENUM(order) to_order ) BOOST_NOEXCEPT
{
    BOOST_STATIC_ASSERT(
        boost::is_class<EndianReversibleInplace>::value ||
        boost::is_array<EndianReversibleInplace>::value ||
        detail::is_endian_reversible_inplace<EndianReversibleInplace>::value );

    if( from_order != to_order )
    {
        endian_reverse_inplace( x );
    }
}

// load/store convenience functions

// load 16

inline boost::int16_t load_little_s16( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::int16_t, 2, order::little>( p );
}

inline boost::uint16_t load_little_u16( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::uint16_t, 2, order::little>( p );
}

inline boost::int16_t load_big_s16( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::int16_t, 2, order::big>( p );
}

inline boost::uint16_t load_big_u16( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::uint16_t, 2, order::big>( p );
}

// load 24

inline boost::int32_t load_little_s24( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::int32_t, 3, order::little>( p );
}

inline boost::uint32_t load_little_u24( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::uint32_t, 3, order::little>( p );
}

inline boost::int32_t load_big_s24( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::int32_t, 3, order::big>( p );
}

inline boost::uint32_t load_big_u24( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::uint32_t, 3, order::big>( p );
}

// load 32

inline boost::int32_t load_little_s32( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::int32_t, 4, order::little>( p );
}

inline boost::uint32_t load_little_u32( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::uint32_t, 4, order::little>( p );
}

inline boost::int32_t load_big_s32( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::int32_t, 4, order::big>( p );
}

inline boost::uint32_t load_big_u32( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::uint32_t, 4, order::big>( p );
}

// load 40

inline boost::int64_t load_little_s40( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::int64_t, 5, order::little>( p );
}

inline boost::uint64_t load_little_u40( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::uint64_t, 5, order::little>( p );
}

inline boost::int64_t load_big_s40( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::int64_t, 5, order::big>( p );
}

inline boost::uint64_t load_big_u40( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::uint64_t, 5, order::big>( p );
}

// load 48

inline boost::int64_t load_little_s48( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::int64_t, 6, order::little>( p );
}

inline boost::uint64_t load_little_u48( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::uint64_t, 6, order::little>( p );
}

inline boost::int64_t load_big_s48( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::int64_t, 6, order::big>( p );
}

inline boost::uint64_t load_big_u48( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::uint64_t, 6, order::big>( p );
}

// load 56

inline boost::int64_t load_little_s56( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::int64_t, 7, order::little>( p );
}

inline boost::uint64_t load_little_u56( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::uint64_t, 7, order::little>( p );
}

inline boost::int64_t load_big_s56( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::int64_t, 7, order::big>( p );
}

inline boost::uint64_t load_big_u56( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::uint64_t, 7, order::big>( p );
}

// load 64

inline boost::int64_t load_little_s64( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::int64_t, 8, order::little>( p );
}

inline boost::uint64_t load_little_u64( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::uint64_t, 8, order::little>( p );
}

inline boost::int64_t load_big_s64( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::int64_t, 8, order::big>( p );
}

inline boost::uint64_t load_big_u64( unsigned char const * p ) BOOST_NOEXCEPT
{
    return boost::endian::endian_load<boost::uint64_t, 8, order::big>( p );
}

// store 16

inline void store_little_s16( unsigned char * p, boost::int16_t v )
{
    boost::endian::endian_store<boost::int16_t, 2, order::little>( p, v );
}

inline void store_little_u16( unsigned char * p, boost::uint16_t v )
{
    boost::endian::endian_store<boost::uint16_t, 2, order::little>( p, v );
}

inline void store_big_s16( unsigned char * p, boost::int16_t v )
{
    boost::endian::endian_store<boost::int16_t, 2, order::big>( p, v );
}

inline void store_big_u16( unsigned char * p, boost::uint16_t v )
{
    boost::endian::endian_store<boost::uint16_t, 2, order::big>( p, v );
}

// store 24

inline void store_little_s24( unsigned char * p, boost::int32_t v )
{
    boost::endian::endian_store<boost::int32_t, 3, order::little>( p, v );
}

inline void store_little_u24( unsigned char * p, boost::uint32_t v )
{
    boost::endian::endian_store<boost::uint32_t, 3, order::little>( p, v );
}

inline void store_big_s24( unsigned char * p, boost::int32_t v )
{
    boost::endian::endian_store<boost::int32_t, 3, order::big>( p, v );
}

inline void store_big_u24( unsigned char * p, boost::uint32_t v )
{
    boost::endian::endian_store<boost::uint32_t, 3, order::big>( p, v );
}

// store 32

inline void store_little_s32( unsigned char * p, boost::int32_t v )
{
    boost::endian::endian_store<boost::int32_t, 4, order::little>( p, v );
}

inline void store_little_u32( unsigned char * p, boost::uint32_t v )
{
    boost::endian::endian_store<boost::uint32_t, 4, order::little>( p, v );
}

inline void store_big_s32( unsigned char * p, boost::int32_t v )
{
    boost::endian::endian_store<boost::int32_t, 4, order::big>( p, v );
}

inline void store_big_u32( unsigned char * p, boost::uint32_t v )
{
    boost::endian::endian_store<boost::uint32_t, 4, order::big>( p, v );
}

// store 40

inline void store_little_s40( unsigned char * p, boost::int64_t v )
{
    boost::endian::endian_store<boost::int64_t, 5, order::little>( p, v );
}

inline void store_little_u40( unsigned char * p, boost::uint64_t v )
{
    boost::endian::endian_store<boost::uint64_t, 5, order::little>( p, v );
}

inline void store_big_s40( unsigned char * p, boost::int64_t v )
{
    boost::endian::endian_store<boost::int64_t, 5, order::big>( p, v );
}

inline void store_big_u40( unsigned char * p, boost::uint64_t v )
{
    boost::endian::endian_store<boost::uint64_t, 5, order::big>( p, v );
}

// store 48

inline void store_little_s48( unsigned char * p, boost::int64_t v )
{
    boost::endian::endian_store<boost::int64_t, 6, order::little>( p, v );
}

inline void store_little_u48( unsigned char * p, boost::uint64_t v )
{
    boost::endian::endian_store<boost::uint64_t, 6, order::little>( p, v );
}

inline void store_big_s48( unsigned char * p, boost::int64_t v )
{
    boost::endian::endian_store<boost::int64_t, 6, order::big>( p, v );
}

inline void store_big_u48( unsigned char * p, boost::uint64_t v )
{
    boost::endian::endian_store<boost::uint64_t, 6, order::big>( p, v );
}

// store 56

inline void store_little_s56( unsigned char * p, boost::int64_t v )
{
    boost::endian::endian_store<boost::int64_t, 7, order::little>( p, v );
}

inline void store_little_u56( unsigned char * p, boost::uint64_t v )
{
    boost::endian::endian_store<boost::uint64_t, 7, order::little>( p, v );
}

inline void store_big_s56( unsigned char * p, boost::int64_t v )
{
    boost::endian::endian_store<boost::int64_t, 7, order::big>( p, v );
}

inline void store_big_u56( unsigned char * p, boost::uint64_t v )
{
    boost::endian::endian_store<boost::uint64_t, 7, order::big>( p, v );
}

// store 64

inline void store_little_s64( unsigned char * p, boost::int64_t v )
{
    boost::endian::endian_store<boost::int64_t, 8, order::little>( p, v );
}

inline void store_little_u64( unsigned char * p, boost::uint64_t v )
{
    boost::endian::endian_store<boost::uint64_t, 8, order::little>( p, v );
}

inline void store_big_s64( unsigned char * p, boost::int64_t v )
{
    boost::endian::endian_store<boost::int64_t, 8, order::big>( p, v );
}

inline void store_big_u64( unsigned char * p, boost::uint64_t v )
{
    boost::endian::endian_store<boost::uint64_t, 8, order::big>( p, v );
}

}  // namespace endian
}  // namespace boost

#endif // BOOST_ENDIAN_CONVERSION_HPP

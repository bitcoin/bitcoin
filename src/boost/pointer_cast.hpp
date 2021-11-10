//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005. 
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_POINTER_CAST_HPP
#define BOOST_POINTER_CAST_HPP

#include <boost/config.hpp>
#include <boost/smart_ptr/detail/sp_noexcept.hpp>

namespace boost { 

//static_pointer_cast overload for raw pointers
template<class T, class U>
inline T* static_pointer_cast(U *ptr) BOOST_SP_NOEXCEPT
{  
   return static_cast<T*>(ptr);
}

//dynamic_pointer_cast overload for raw pointers
template<class T, class U>
inline T* dynamic_pointer_cast(U *ptr) BOOST_SP_NOEXCEPT
{  
   return dynamic_cast<T*>(ptr);
}

//const_pointer_cast overload for raw pointers
template<class T, class U>
inline T* const_pointer_cast(U *ptr) BOOST_SP_NOEXCEPT
{  
   return const_cast<T*>(ptr);
}

//reinterpret_pointer_cast overload for raw pointers
template<class T, class U>
inline T* reinterpret_pointer_cast(U *ptr) BOOST_SP_NOEXCEPT
{  
   return reinterpret_cast<T*>(ptr);
}

} // namespace boost

#if !defined( BOOST_NO_CXX11_SMART_PTR )

#include <boost/type_traits/has_virtual_destructor.hpp>
#include <boost/static_assert.hpp>
#include <memory>

namespace boost {

//static_pointer_cast overload for std::shared_ptr
using std::static_pointer_cast;

//dynamic_pointer_cast overload for std::shared_ptr
using std::dynamic_pointer_cast;

//const_pointer_cast overload for std::shared_ptr
using std::const_pointer_cast;

//reinterpret_pointer_cast overload for std::shared_ptr
template<class T, class U> std::shared_ptr<T> reinterpret_pointer_cast(const std::shared_ptr<U> & r ) BOOST_SP_NOEXCEPT
{
    (void) reinterpret_cast< T* >( static_cast< U* >( 0 ) );

    typedef typename std::shared_ptr<T>::element_type E;

    E * p = reinterpret_cast< E* >( r.get() );
    return std::shared_ptr<T>( r, p );
}

//static_pointer_cast overload for std::unique_ptr
template<class T, class U> std::unique_ptr<T> static_pointer_cast( std::unique_ptr<U> && r ) BOOST_SP_NOEXCEPT
{
    (void) static_cast< T* >( static_cast< U* >( 0 ) );

    typedef typename std::unique_ptr<T>::element_type E;

    return std::unique_ptr<T>( static_cast<E*>( r.release() ) );
}

//dynamic_pointer_cast overload for std::unique_ptr
template<class T, class U> std::unique_ptr<T> dynamic_pointer_cast( std::unique_ptr<U> && r ) BOOST_SP_NOEXCEPT
{
    (void) dynamic_cast< T* >( static_cast< U* >( 0 ) );

    BOOST_STATIC_ASSERT_MSG( boost::has_virtual_destructor<T>::value, "The target of dynamic_pointer_cast must have a virtual destructor." );

    T * p = dynamic_cast<T*>( r.get() );
    if( p ) r.release();
    return std::unique_ptr<T>( p );
}

//const_pointer_cast overload for std::unique_ptr
template<class T, class U> std::unique_ptr<T> const_pointer_cast( std::unique_ptr<U> && r ) BOOST_SP_NOEXCEPT
{
    (void) const_cast< T* >( static_cast< U* >( 0 ) );

    typedef typename std::unique_ptr<T>::element_type E;

    return std::unique_ptr<T>( const_cast<E*>( r.release() ) );
}

//reinterpret_pointer_cast overload for std::unique_ptr
template<class T, class U> std::unique_ptr<T> reinterpret_pointer_cast( std::unique_ptr<U> && r ) BOOST_SP_NOEXCEPT
{
    (void) reinterpret_cast< T* >( static_cast< U* >( 0 ) );

    typedef typename std::unique_ptr<T>::element_type E;

    return std::unique_ptr<T>( reinterpret_cast<E*>( r.release() ) );
}

} // namespace boost

#endif // #if !defined( BOOST_NO_CXX11_SMART_PTR )

#endif   //BOOST_POINTER_CAST_HPP

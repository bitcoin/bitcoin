// (C) Copyright 2002-2008, Fernando Luis Cacciola Carballal.
// Copyright 2020 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// 21 Ago 2002 (Created) Fernando Cacciola
// 24 Dec 2007 (Refactored and worked around various compiler bugs) Fernando Cacciola, Niels Dekker
// 23 May 2008 (Fixed operator= const issue, added initialized_value) Niels Dekker, Fernando Cacciola
// 21 Ago 2008 (Added swap) Niels Dekker, Fernando Cacciola
// 20 Feb 2009 (Fixed logical const-ness issues) Niels Dekker, Fernando Cacciola
// 03 Apr 2010 (Added initialized<T>, suggested by Jeffrey Hellrung, fixing #3472) Niels Dekker
// 30 May 2010 (Made memset call conditional, fixing #3869) Niels Dekker
//
#ifndef BOOST_UTILITY_VALUE_INIT_21AGO2002_HPP
#define BOOST_UTILITY_VALUE_INIT_21AGO2002_HPP

// Note: The implementation of boost::value_initialized had to deal with the
// fact that various compilers haven't fully implemented value-initialization.
// The constructor of boost::value_initialized<T> works around these compiler
// issues, by clearing the bytes of T, before constructing the T object it
// contains. More details on these issues are at libs/utility/value_init.htm

#include <boost/config.hpp> // For BOOST_NO_COMPLETE_VALUE_INITIALIZATION.
#include <boost/swap.hpp>
#include <cstring>
#include <cstddef>

#ifdef BOOST_MSVC
#pragma warning(push)
// It is safe to ignore the following warning from MSVC 7.1 or higher:
// "warning C4351: new behavior: elements of array will be default initialized"
#pragma warning(disable: 4351)
// It is safe to ignore the following MSVC warning, which may pop up when T is 
// a const type: "warning C4512: assignment operator could not be generated".
#pragma warning(disable: 4512)
#endif

#ifdef BOOST_NO_COMPLETE_VALUE_INITIALIZATION
  // Implementation detail: The macro BOOST_DETAIL_VALUE_INIT_WORKAROUND_SUGGESTED 
  // suggests that a workaround should be applied, because of compiler issues 
  // regarding value-initialization.
  #define BOOST_DETAIL_VALUE_INIT_WORKAROUND_SUGGESTED
#endif

// Implementation detail: The macro BOOST_DETAIL_VALUE_INIT_WORKAROUND
// switches the value-initialization workaround either on or off.
#ifndef BOOST_DETAIL_VALUE_INIT_WORKAROUND
  #ifdef BOOST_DETAIL_VALUE_INIT_WORKAROUND_SUGGESTED
  #define BOOST_DETAIL_VALUE_INIT_WORKAROUND 1
  #else
  #define BOOST_DETAIL_VALUE_INIT_WORKAROUND 0
  #endif
#endif

namespace boost {

namespace detail {

  struct zero_init
  {
    zero_init()
    {
    }

    zero_init( void * p, std::size_t n )
    {
      std::memset( p, 0, n );
    }
  };

} // namespace detail

template<class T>
class initialized
#if BOOST_DETAIL_VALUE_INIT_WORKAROUND
  : detail::zero_init
#endif
{
  private:

    T data_;

  public :

    BOOST_GPU_ENABLED
    initialized():
#if BOOST_DETAIL_VALUE_INIT_WORKAROUND
      zero_init( &const_cast< char& >( reinterpret_cast<char const volatile&>( data_ ) ), sizeof( data_ ) ),
#endif
      data_()
    {
    }

    BOOST_GPU_ENABLED
    explicit initialized(T const & arg): data_( arg )
    {
    }

    BOOST_GPU_ENABLED
    T const & data() const
    {
      return data_;
    }

    BOOST_GPU_ENABLED
    T& data()
    {
      return data_;
    }

    BOOST_GPU_ENABLED
    void swap(initialized & arg)
    {
      ::boost::swap( this->data(), arg.data() );
    }

    BOOST_GPU_ENABLED
    operator T const &() const
    {
      return data_;
    }

    BOOST_GPU_ENABLED
    operator T&()
    {
      return data_;
    }

} ;

template<class T>
BOOST_GPU_ENABLED
T const& get ( initialized<T> const& x )
{
  return x.data() ;
}

template<class T>
BOOST_GPU_ENABLED
T& get ( initialized<T>& x )
{
  return x.data() ;
}

template<class T>
BOOST_GPU_ENABLED
void swap ( initialized<T> & lhs, initialized<T> & rhs )
{
  lhs.swap(rhs) ;
}

template<class T>
class value_initialized
{
  private :

    // initialized<T> does value-initialization by default.
    initialized<T> m_data;

  public :
    
    BOOST_GPU_ENABLED
    value_initialized()
    :
    m_data()
    { }
    
    BOOST_GPU_ENABLED
    T const & data() const
    {
      return m_data.data();
    }

    BOOST_GPU_ENABLED
    T& data()
    {
      return m_data.data();
    }

    BOOST_GPU_ENABLED
    void swap(value_initialized & arg)
    {
      m_data.swap(arg.m_data);
    }

    BOOST_GPU_ENABLED
    operator T const &() const
    {
      return m_data;
    }

    BOOST_GPU_ENABLED
    operator T&()
    {
      return m_data;
    }
} ;


template<class T>
BOOST_GPU_ENABLED
T const& get ( value_initialized<T> const& x )
{
  return x.data() ;
}

template<class T>
BOOST_GPU_ENABLED
T& get ( value_initialized<T>& x )
{
  return x.data() ;
}

template<class T>
BOOST_GPU_ENABLED
void swap ( value_initialized<T> & lhs, value_initialized<T> & rhs )
{
  lhs.swap(rhs) ;
}


class initialized_value_t
{
  public :
    
    template <class T> BOOST_GPU_ENABLED operator T() const
    {
      return initialized<T>().data();
    }
};

initialized_value_t const initialized_value = {} ;


} // namespace boost

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif

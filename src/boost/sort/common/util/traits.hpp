//----------------------------------------------------------------------------
/// @file traits.hpp
/// @brief this file contains the metaprogramming classes  compare_iter and
///         enable_if_not_integral
/// @author Copyright(c) 2016 Francisco Jose Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanying file LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt  )
/// @version 0.1
///
//-----------------------------------------------------------------------------
#ifndef __BOOST_SORT_COMMON_UTIL_TRAITS_HPP
#define __BOOST_SORT_COMMON_UTIL_TRAITS_HPP

#include <functional>
#include <iterator>
#include <type_traits>

namespace boost
{
namespace sort
{
namespace common
{
namespace util
{
//----------------------------------------------------------------------------
//                  USING SENTENCES
//----------------------------------------------------------------------------
using std::iterator_traits;

//
//---------------------------------------------------------------------------
/// @class value_iter
/// @brief From the iterator, obtain the type pointed by it
/// @remarks The main utility of this, is simplify the default template
///          parameter of comparison
//---------------------------------------------------------------------------
template<class iter_t>
using value_iter = typename iterator_traits< iter_t >::value_type;
//
//---------------------------------------------------------------------------
/// @class compare_iter
/// @brief From the iterator, received as template parameter, obtain the type
///        of the object pointed by the iterator, and with this define the
///        std::less with this type obtained
/// @remarks The main utility of this, is simplify the default template
///          parameter of comparison
//---------------------------------------------------------------------------
template<class iter_t>
using compare_iter =  std::less< value_iter< iter_t > >;

//
//---------------------------------------------------------------------------
/// @class enable_if_not_integral
/// @brief This is a SFINAE class for to detect if the third parameter in the
///        invocation of the parallel sorting algorithms is an integer
///        representing the number of threads to use or is a comparison object
/// @remarks
//---------------------------------------------------------------------------
template<class T>
using enable_if_not_integral =
      typename std::enable_if< !std::is_integral< T >::value >::type;
//
//---------------------------------------------------------------------------
/// @class enable_if_integral
/// @brief This is a SFINAE class for to detect if the third parameter in the
///        invocation of the parallel sorting algorithms is an integer
///        representing the number of threads to use or is a comparison object
/// @remarks
//---------------------------------------------------------------------------
template<class T>
using enable_if_integral =
      typename std::enable_if< std::is_integral< T >::value >::type;

//
//---------------------------------------------------------------------------
/// @class enable_if_string
/// @brief This is a SFINAE class for to detect if the parameter is a
///        std::string for to apply specialized parameters in the invocation
///        of the block_indirect_sort algorithm
/// @remarks
//---------------------------------------------------------------------------
template<class T>
using enable_if_string =
      typename std::enable_if< std::is_same< T, std::string >::value >::type;

//
//---------------------------------------------------------------------------
/// @class enable_if_not_string
/// @brief This is a SFINAE class for to detect if the parameter is a
///        std::string for to apply specialized parameters in the invocation
///        of the block_indirect_sort algorithm
/// @remarks
//---------------------------------------------------------------------------
template<class T>
using enable_if_not_string =
      typename std::enable_if<! std::is_same< T, std::string >::value >::type;

//
//---------------------------------------------------------------------------
/// @class constructor
/// @brief create a functor with the constructor of a class for to be invoked
///        from a bind or a lambda
/// @remarks
//---------------------------------------------------------------------------
template<class T>
struct constructor
{
    template<class ... Args>
    void operator()(Args && ... args)
    {
        T(std::forward<Args> (args) ...);
    };
};
//
//****************************************************************************
};// End namespace util
};// End namespace common
};// End namespace sort
};// End namespace boost
//****************************************************************************
#endif

//----------------------------------------------------------------------------
/// @file rearrange.hpp
/// @brief Indirect algorithm
///
/// @author Copyright (c) 2016 Francisco Jose Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanying file LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt  )
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#ifndef __BOOST_SORT_COMMON_REARRANGE_HPP
#define __BOOST_SORT_COMMON_REARRANGE_HPP

//#include <boost/sort/common/atomic.hpp>
#include <boost/sort/common/util/traits.hpp>
#include <functional>
#include <iterator>
#include <type_traits>
#include <vector>
#include <cassert>

namespace boost
{
namespace sort
{
namespace common
{

template<class Iter_data>
struct filter_iterator
{
    //-----------------------------------------------------------------------
    //                   Variables
    //-----------------------------------------------------------------------
    Iter_data origin;

    //-----------------------------------------------------------------------
    //                   Functions
    //-----------------------------------------------------------------------
    filter_iterator(Iter_data global_first): origin(global_first) { };
    size_t operator ()(Iter_data itx) const
    {
        return size_t(itx - origin);
    }
};

struct filter_pos
{
    size_t operator ()(size_t pos) const {  return pos; };
};

//
//-----------------------------------------------------------------------------
//  function : rearrange
/// @brief This function transform a logical sort of the elements in the index  
///        of iterators in a physical sort. 
//
/// @param global_first : iterator to the first element of the data
/// @param [in] index : vector of the iterators
//-----------------------------------------------------------------------------
template<class Iter_data, class Iter_index, class Filter_pos>
void rearrange(Iter_data global_first, Iter_index itx_first,
               Iter_index itx_last, Filter_pos pos)
{
    //-----------------------------------------------------------------------
    //                    Metaprogramming
    //-----------------------------------------------------------------------
    typedef util::value_iter<Iter_data>     value_data;
    typedef util::value_iter<Iter_index>    value_index;

    //-------------------------------------------------------------------------
    //                     Code
    //-------------------------------------------------------------------------	
    assert((itx_last - itx_first) >= 0);
    size_t pos_dest, pos_src, pos_ini;
    size_t nelem = size_t(itx_last - itx_first);
    Iter_data data = global_first;
    Iter_index index = itx_first;

    pos_ini = 0;
    while (pos_ini < nelem)
    {
        while (pos_ini < nelem and pos(index[pos_ini]) == pos_ini)
            ++pos_ini;
        if (pos_ini == nelem) return;
        pos_dest = pos_src = pos_ini;
        value_data aux = std::move(data[pos_ini]);
        value_index itx_src = std::move(index[pos_ini]);

        while ((pos_src = pos(itx_src)) != pos_ini)
        {
            data[pos_dest] = std::move(data[pos_src]);
            std::swap(itx_src, index[pos_src]);
            pos_dest = pos_src;
        };

        data[pos_dest] = std::move(aux);
        index[pos_ini] = std::move(itx_src);
        ++pos_ini;
    };
};

/*
 //
 //-----------------------------------------------------------------------------
 //  function : rearrange_pos
 /// @brief This function transform a logical sort of the elements in the index  
 ///        of iterators in a physical sort. 
 //
 /// @param global_first : iterator to the first element of the data
 /// @param [in] index : vector of the iterators
 //-----------------------------------------------------------------------------
 template < class Iter_t, class Number >
 void rearrange_pos (Iter_t global_first, std::vector< Number> &index)
 {	
 //-------------------------------------------------------------------------
 //          METAPROGRAMMING AND DEFINITIONS
 //-------------------------------------------------------------------------
 static_assert ( std::is_integral<Number>::value, "Incompatible Types");
 typedef iter_value< Iter_t > value_t;

 //-------------------------------------------------------------------------
 //                     CODE
 //-------------------------------------------------------------------------
 size_t pos_dest = 0;
 size_t pos_src = 0;
 size_t pos_ini = 0;
 size_t nelem = index.size ( );
 Iter_t it_dest (global_first), it_src(global_first);

 while (pos_ini < nelem)
 {
 while (pos_ini < nelem and
 index[pos_ini] == pos_ini)
 {
 ++pos_ini;
 };

 if (pos_ini == nelem) return;
 pos_dest = pos_src = pos_ini;
 it_dest = global_first + pos_dest;
 value_t Aux = std::move (*it_dest);

 while ((pos_src = index[pos_dest]) != pos_ini)
 {
 index[pos_dest] = it_dest - global_first;
 it_src = global_first + pos_src;
 *it_dest = std::move (*it_src);
 it_dest = it_src;
 pos_dest = pos_src;
 };

 *it_dest = std::move (Aux);
 index[pos_dest] = it_dest - global_first;
 ++pos_ini;
 };
 };
 */
//
//****************************************************************************
};//    End namespace common
};//    End namespace sort
};//    End namespace boost
//****************************************************************************
//
#endif

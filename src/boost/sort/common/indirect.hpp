//----------------------------------------------------------------------------
/// @file indirect.hpp
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
#ifndef __BOOST_SORT_PARALLEL_COMMON_INDIRECT_HPP
#define __BOOST_SORT_PARALLEL_COMMON_INDIRECT_HPP

//#include <boost/sort/common/atomic.hpp>
#include <boost/sort/common/util/traits.hpp>
#include <functional>
#include <iterator>
#include <type_traits>
#include <vector>

namespace boost
{
namespace sort
{
namespace common
{

//
//---------------------------------------------------------------------------
/// @struct less_ptr_no_null
///
/// @remarks this is the comparison object for pointers. Compare the objects
///          pointed by the iterators
//---------------------------------------------------------------------------
template<class Iter_t, class Compare = util::compare_iter<Iter_t> >
struct less_ptr_no_null
{
    //----------------------------- Variables -----------------------
    Compare comp; // comparison object of the elements pointed by Iter_t

    //------------------------------------------------------------------------
    //  function : less_ptr_no_null
    /// @brief constructor from a Compare object
    /// @param C1 : comparison object
    //-----------------------------------------------------------------------
    less_ptr_no_null(Compare C1 = Compare()): comp(C1) { };

    //------------------------------------------------------------------------
    //  function : operator ( )
    /// @brief Make the comparison of the objects pointed by T1 and T2, using
    //         the internal comp
    //
    /// @param  T1 : first iterator
    /// @param  T2 : second iterator
    /// @return bool result of the comparison
    //-----------------------------------------------------------------------
    bool operator( )(Iter_t T1, Iter_t T2) const
    {
        return comp(*T1, *T2);
    };
};
//
//-----------------------------------------------------------------------------
//  function : create_index
/// @brief From a vector of objects, create a vector of iterators to
///        the objects
///
/// @param first : iterator to the first element of the range
/// @param last : iterator to the element after the last of the range
/// @param index : vector where store the iterators
//-----------------------------------------------------------------------------
template<class Iter_t>
static void create_index(Iter_t first, Iter_t last, std::vector<Iter_t> &index)
{
    auto nelem = last - first;
    assert(nelem >= 0);
    index.clear();
    index.reserve(nelem);
    for (; first != last; ++first) index.push_back(first);
};
//
//-----------------------------------------------------------------------------
//  function : sort_index
/// @brief This function transform a logical sort of the elements in the index
///        in a physical sort
//
/// @param global_first : iterator to the first element of the data
/// @param [in] index : vector of the iterators
//-----------------------------------------------------------------------------
template<class Iter_t>
static void sort_index(Iter_t global_first, std::vector<Iter_t> &index)
{
    typedef util::value_iter<Iter_t> value_t;

    size_t pos_dest = 0;
    size_t pos_src = 0;
    size_t pos_in_vector = 0;
    size_t nelem = index.size();
    Iter_t it_dest, it_src;

    while (pos_in_vector < nelem)
    {
        while (pos_in_vector < nelem and
               (size_t(index[pos_in_vector] - global_first)) == pos_in_vector)
        {
            ++pos_in_vector;
        };

        if (pos_in_vector == nelem) return;
        pos_dest = pos_src = pos_in_vector;
        it_dest = global_first + pos_dest;
        value_t Aux = std::move(*it_dest);

        while ((pos_src = (size_t(index[pos_dest] - global_first)))
               != pos_in_vector)
        {
            index[pos_dest] = it_dest;
            it_src = global_first + pos_src;
            *it_dest = std::move(*it_src);
            it_dest = it_src;
            pos_dest = pos_src;
        };

        *it_dest = std::move(Aux);
        index[pos_dest] = it_dest;
        ++pos_in_vector;
    };
};

template<class func, class Iter_t, class Compare = compare_iter<Iter_t> >
static void indirect_sort(func method, Iter_t first, Iter_t last, Compare comp)
{
    auto nelem = (last - first);
    assert(nelem >= 0);
    if (nelem < 2) return;
    std::vector<Iter_t> index;
    index.reserve((size_t) nelem);
    create_index(first, last, index);
    less_ptr_no_null<Iter_t, Compare> index_comp(comp);
    method(index.begin(), index.end(), index_comp);
    sort_index(first, index);
};

//
//****************************************************************************
};//    End namespace common
};//    End namespace sort
};//    End namespace boost
//****************************************************************************
//
#endif

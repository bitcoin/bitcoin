//----------------------------------------------------------------------------
/// @file search.hpp
/// @brief
/// @author Copyright (c) 2017 Francisco José Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See copy at http://www.boost.org/LICENSE_1_0.txt  )
/// @remarks
//-----------------------------------------------------------------------------
#ifndef __BOOST_SORT_COMMON_SEARCH_HPP
#define __BOOST_SORT_COMMON_SEARCH_HPP

#include <boost/sort/common/util/traits.hpp>
#include <cassert>

namespace boost
{
namespace sort
{
namespace common
{
namespace util
{

template<class T>
struct filter_pass
{
    typedef T key;
    const key & operator()(const T & val) const
    {
        return val;
    };
};

//
//###########################################################################
//                                                                         ##
//    ################################################################     ##
//    #                                                              #     ##
//    #           I N T E R N A L      F U N C T I O N S             #     ##
//    #                                                              #     ##
//    ################################################################     ##
//                                                                         ##
//                       I M P O R T A N T                                 ##
//                                                                         ##
// These functions are not directly callable by the user, are for internal ##
// use only.                                                               ##
// These functions don't check the parameters                              ##
//                                                                         ##
//###########################################################################
//
//-----------------------------------------------------------------------------
//  function : internal_find_first
/// @brief find if a value exist in the range [first, last).
///        Always return as valid iterator in the range [first, last-1]
///        If exist return the iterator to the first occurrence. If don't exist
///        return the first greater than val.
///        If val is greater than the *(last-1), return (last-1)
///        If val is lower than  (*first), return  first
//
/// @param [in] first : iterator to the first element of the range
/// @param [in] last : iterator to the last element of the range
/// @param [in] val : value to find
/// @param [in] comp : object for to compare two value_t objects
/// @return iterator to the element found,
//-----------------------------------------------------------------------------
template <class Iter_t, class Filter = filter_pass<value_iter<Iter_t> >,
          class Compare = std::less<typename Filter::key> >
inline Iter_t internal_find_first(Iter_t first, Iter_t last,
                                  const typename Filter::key &val,
                                  const Compare & comp = Compare(), 
                                  Filter flt = Filter())
{
    Iter_t LI = first, LS = last - 1, it_out = first;
    while (LI != LS)
    {
        it_out = LI + ((LS - LI) >> 1);
        if (comp(flt(*it_out), val))
            LI = it_out + 1;
        else LS = it_out;
    };
    return LS;
};
//
//-----------------------------------------------------------------------------
//  function : internal_find_last
/// @brief find if a value exist in the range [first, last).
///        Always return as valid iterator in the range [first, last-1]
///        If exist return the iterator to the last occurrence.
///        If don't exist return the first lower than val.
///        If val is greater than *(last-1) return (last-1).
///        If is lower than the first, return first
//
/// @param [in] first : iterator to the first element of the range
/// @param [in] last : iterator to the last element of the range
/// @param [in] val : value to find
/// @param [in] comp : object for to compare two value_t objects
/// @return iterator to the element found, if not found return last

//-----------------------------------------------------------------------------
template<class Iter_t, class Filter = filter_pass<value_iter<Iter_t> >,
                class Compare = std::less<typename Filter::key> >
inline Iter_t internal_find_last(Iter_t first, Iter_t last,
                                 const typename Filter::key &val,
                                 const Compare & comp = Compare(), Filter flt =
                                                 Filter())
{
    Iter_t LI = first, LS = last - 1, it_out = first;
    while (LI != LS)
    {
        it_out = LI + ((LS - LI + 1) >> 1);
        if (comp(val, flt(*it_out))) LS = it_out - 1;
        else                         LI = it_out;
    };
    return LS;
};

//
//###########################################################################
//                                                                         ##
//    ################################################################     ##
//    #                                                              #     ##
//    #              P U B L I C       F U N C T I O N S             #     ##
//    #                                                              #     ##
//    ################################################################     ##
//                                                                         ##
//###########################################################################
//
//-----------------------------------------------------------------------------
//  function : find_first
/// @brief find if a value exist in the range [first, last). If exist return the
///        iterator to the first occurrence. If don't exist return last
//
/// @param [in] first : iterator to the first element of the range
/// @param [in] last : iterator to the last element of the range
/// @param [in] val : value to find
/// @param [in] comp : object for to compare two value_t objects
/// @return iterator to the element found, and if not last
//-----------------------------------------------------------------------------
template<class Iter_t, class Filter = filter_pass<value_iter<Iter_t> >,
                class Compare = std::less<typename Filter::key> >
inline Iter_t find_first(Iter_t first, Iter_t last,
                         const typename Filter::key &val, 
                         const Compare & comp = Compare(),
                         Filter flt = Filter())
{
    assert((last - first) >= 0);
    if (first == last) return last;
    Iter_t LS = internal_find_first(first, last, val, comp, flt);
    return (comp(flt(*LS), val) or comp(val, flt(*LS))) ? last : LS;
};
//
//-----------------------------------------------------------------------------
//  function : find_last
/// @brief find if a value exist in the range [first, last). If exist return the
///        iterator to the last occurrence. If don't exist return last
//
/// @param [in] first : iterator to the first element of the range
/// @param [in] last : iterator to the last element of the range
/// @param [in] val : value to find
/// @param [in] comp : object for to compare two value_t objects
/// @return iterator to the element found, if not found return last

//-----------------------------------------------------------------------------
template <class Iter_t, class Filter = filter_pass<value_iter<Iter_t> >,
          class Compare = std::less<typename Filter::key> >
inline Iter_t find_last(Iter_t first, Iter_t last,
                        const typename Filter::key &val, 
                        const Compare & comp = Compare(),
                        Filter flt = Filter())
{
    assert((last - first) >= 0);
    if (last == first) return last;
    Iter_t LS = internal_find_last(first, last, val, comp, flt);
    return (comp(flt(*LS), val) or comp(val, flt(*LS))) ? last : LS;
};

//----------------------------------------------------------------------------
//  function : lower_bound
/// @brief Returns an iterator pointing to the first element in the range
///        [first, last) that is not less than (i.e. greater or equal to) val.
/// @param [in] last : iterator to the last element of the range
/// @param [in] val : value to find
/// @param [in] comp : object for to compare two value_t objects
/// @return iterator to the element found
//-----------------------------------------------------------------------------
template<class Iter_t, class Filter = filter_pass<value_iter<Iter_t> >,
                class Compare = std::less<typename Filter::key> >
inline Iter_t lower_bound(Iter_t first, Iter_t last,
                          const typename Filter::key &val,
                          const Compare & comp = Compare(), 
                          Filter flt = Filter())
{
    assert((last - first) >= 0);
    if (last == first) return last;
    Iter_t itaux = internal_find_first(first, last, val, comp, flt);
    return (itaux == (last - 1) and comp(flt(*itaux), val)) ? last : itaux;
};
//----------------------------------------------------------------------------
//  function :upper_bound
/// @brief return the first element greather than val.If don't exist
///        return last
//
/// @param [in] first : iterator to the first element of the range
/// @param [in] last : iterator to the last element of the range
/// @param [in] val : value to find
/// @param [in] comp : object for to compare two value_t objects
/// @return iterator to the element found
/// @remarks
//-----------------------------------------------------------------------------
template<class Iter_t, class Filter = filter_pass<value_iter<Iter_t> >,
                class Compare = std::less<typename Filter::key> >
inline Iter_t upper_bound(Iter_t first, Iter_t last,
                          const typename Filter::key &val,
                          const Compare & comp = Compare(), 
                          Filter flt = Filter())
{
    assert((last - first) >= 0);
    if (last == first) return last;
    Iter_t itaux = internal_find_last(first, last, val, comp, flt);
    return (itaux == first and comp(val, flt(*itaux))) ? itaux : itaux + 1;
}
;
//----------------------------------------------------------------------------
//  function :equal_range
/// @brief return a pair of lower_bound and upper_bound with the value val.If
///        don't exist return last in the two elements of the pair
//
/// @param [in] first : iterator to the first element of the range
/// @param [in] last : iterator to the last element of the range
/// @param [in] val : value to find
/// @param [in] comp : object for to compare two value_t objects
/// @return pair of iterators
//-----------------------------------------------------------------------------
template<class Iter_t, class Filter = filter_pass<value_iter<Iter_t> >,
         class Compare = std::less<typename Filter::key> >
inline std::pair<Iter_t, Iter_t> equal_range(Iter_t first, Iter_t last,
                                             const typename Filter::key &val,
                                             const Compare & comp = Compare(),
                                             Filter flt = Filter())
{
    return std::make_pair(lower_bound(first, last, val, comp, flt),
                    upper_bound(first, last, val, comp, flt));
};
//
//-----------------------------------------------------------------------------
//  function : insert_first
/// @brief find if a value exist in the range [first, last). If exist return the
///        iterator to the first occurrence. If don't exist return last
//
/// @param [in] first : iterator to the first element of the range
/// @param [in] last : iterator to the last element of the range
/// @param [in] val : value to find
/// @param [in] comp : object for to compare two value_t objects
/// @return iterator to the element found, and if not last
//-----------------------------------------------------------------------------
template<class Iter_t, class Filter = filter_pass<value_iter<Iter_t> >,
                class Compare = std::less<typename Filter::key> >
inline Iter_t insert_first(Iter_t first, Iter_t last,
                           const typename Filter::key &val,
                           const Compare & comp = Compare(), Filter flt =
                                           Filter())
{
    return lower_bound(first, last, val, comp, flt);
};
//
//-----------------------------------------------------------------------------
//  function : insert_last
/// @brief find if a value exist in the range [first, last). If exist return the
///        iterator to the last occurrence. If don't exist return last
//
/// @param [in] first : iterator to the first element of the range
/// @param [in] last : iterator to the last element of the range
/// @param [in] val : value to find
/// @param [in] comp : object for to compare two value_t objects
/// @return iterator to the element found, if not found return last

//-----------------------------------------------------------------------------
template<class Iter_t, class Filter = filter_pass<value_iter<Iter_t> >,
                class Compare = std::less<typename Filter::key> >
inline Iter_t insert_last(Iter_t first, Iter_t last,
                          const typename Filter::key &val,
                          const Compare & comp = Compare(), Filter flt =
                                          Filter())
{
    return upper_bound(first, last, val, comp, flt);
};

/*

 //
 //###########################################################################
 //                                                                         ##
 //    ################################################################     ##
 //    #                                                              #     ##
 //    #           I N T E R N A L      F U N C T I O N S             #     ##
 //    #                                                              #     ##
 //    ################################################################     ##
 //                                                                         ##
 //                       I M P O R T A N T                                 ##
 //                                                                         ##
 // These functions are not directly callable by the user, are for internal ##
 // use only.                                                               ##
 // These functions don't check the parameters                              ##
 //                                                                         ##
 //###########################################################################
 //
 //-----------------------------------------------------------------------------
 //  function : internal_find_first
 /// @brief find if a value exist in the range [first, last).
 ///        Always return as valid iterator in the range [first, last-1]
 ///        If exist return the iterator to the first occurrence. If don't exist
 ///        return the first greater than val.
 ///        If val is greater than the *(last-1), return (last-1)
 ///        If val is lower than  (*first), return  first
 //
 /// @param [in] first : iterator to the first element of the range
 /// @param [in] last : iterator to the last element of the range
 /// @param [in] val : value to find
 /// @param [in] comp : object for to compare two value_t objects
 /// @return iterator to the element found,
 //-----------------------------------------------------------------------------
 template < class Iter_t, class Compare = compare_iter<Iter_t>  >
 inline Iter_t internal_find_first ( Iter_t first, Iter_t last,
 const value_iter<Iter_t> &val,
 const Compare & comp= Compare()  )
 {
 Iter_t LI = first , LS = last - 1, it_out = first;
 while ( LI != LS)
 {   it_out = LI + ( (LS - LI) >> 1);
 if ( comp ( *it_out, val)) LI = it_out + 1 ; else LS = it_out ;
 };
 return LS ;
 };
 //
 //-----------------------------------------------------------------------------
 //  function : internal_find_last
 /// @brief find if a value exist in the range [first, last).
 ///        Always return as valid iterator in the range [first, last-1]
 ///        If exist return the iterator to the last occurrence.
 ///        If don't exist return the first lower than val.
 ///        If val is greater than *(last-1) return (last-1).
 ///        If is lower than the first, return first
 //
 /// @param [in] first : iterator to the first element of the range
 /// @param [in] last : iterator to the last element of the range
 /// @param [in] val : value to find
 /// @param [in] comp : object for to compare two value_t objects
 /// @return iterator to the element found, if not found return last

 //-----------------------------------------------------------------------------
 template < class Iter_t, class Compare = compare_iter<Iter_t> >
 inline Iter_t internal_find_last ( Iter_t first, Iter_t last ,
 const value_iter<Iter_t> &val,
 const Compare &comp= Compare() )
 {
 Iter_t LI = first , LS = last - 1, it_out = first ;
 while ( LI != LS)
 {   it_out = LI + ( (LS - LI + 1) >> 1);
 if ( comp (val, *it_out)) LS = it_out - 1 ; else LI = it_out ;
 };
 return LS ;
 };

 //
 //###########################################################################
 //                                                                         ##
 //    ################################################################     ##
 //    #                                                              #     ##
 //    #              P U B L I C       F U N C T I O N S             #     ##
 //    #                                                              #     ##
 //    ################################################################     ##
 //                                                                         ##
 //###########################################################################
 //
 //-----------------------------------------------------------------------------
 //  function : find_first
 /// @brief find if a value exist in the range [first, last). If exist return the
 ///        iterator to the first occurrence. If don't exist return last
 //
 /// @param [in] first : iterator to the first element of the range
 /// @param [in] last : iterator to the last element of the range
 /// @param [in] val : value to find
 /// @param [in] comp : object for to compare two value_t objects
 /// @return iterator to the element found, and if not last
 //-----------------------------------------------------------------------------
 template < class Iter_t, class Compare = compare_iter<Iter_t> >
 inline Iter_t find_first ( Iter_t first, Iter_t last,
 const value_iter<Iter_t> &val,
 Compare comp = Compare() )
 {
 assert ( (last - first) >= 0 );
 if ( first == last) return last ;
 Iter_t LS = internal_find_first ( first, last, val, comp);
 return (comp (*LS, val) or comp (val, *LS))?last:LS;
 };
 //
 //-----------------------------------------------------------------------------
 //  function : find_last
 /// @brief find if a value exist in the range [first, last). If exist return the
 ///        iterator to the last occurrence. If don't exist return last
 //
 /// @param [in] first : iterator to the first element of the range
 /// @param [in] last : iterator to the last element of the range
 /// @param [in] val : value to find
 /// @param [in] comp : object for to compare two value_t objects
 /// @return iterator to the element found, if not found return last

 //-----------------------------------------------------------------------------
 template < class Iter_t, class Compare = compare_iter<Iter_t> >
 inline Iter_t find_last ( Iter_t first, Iter_t last ,
 const value_iter<Iter_t> &val,
 Compare comp = Compare())
 {
 assert ( (last - first ) >= 0 );
 if ( last == first ) return last ;
 Iter_t LS = internal_find_last (first, last, val, comp);
 return (comp (*LS, val) or comp (val, *LS))?last:LS ;
 };

 //----------------------------------------------------------------------------
 //  function : lower_bound
 /// @brief Returns an iterator pointing to the first element in the range
 ///        [first, last) that is not less than (i.e. greater or equal to) val.
 /// @param [in] last : iterator to the last element of the range
 /// @param [in] val : value to find
 /// @param [in] comp : object for to compare two value_t objects
 /// @return iterator to the element found
 //-----------------------------------------------------------------------------
 template < class Iter_t, class Compare = compare_iter<Iter_t> >
 inline Iter_t lower_bound ( Iter_t first, Iter_t last ,
 const value_iter<Iter_t> &val,
 Compare &comp = Compare() )
 {
 assert ( (last - first ) >= 0 );
 if ( last == first ) return last ;
 Iter_t  itaux = internal_find_first( first, last, val,comp);
 return (itaux == (last - 1) and comp (*itaux, val))?last: itaux;
 };
 //----------------------------------------------------------------------------
 //  function :upper_bound
 /// @brief return the first element greather than val.If don't exist
 ///        return last
 //
 /// @param [in] first : iterator to the first element of the range
 /// @param [in] last : iterator to the last element of the range
 /// @param [in] val : value to find
 /// @param [in] comp : object for to compare two value_t objects
 /// @return iterator to the element found
 /// @remarks
 //-----------------------------------------------------------------------------
 template < class Iter_t, class Compare = compare_iter<Iter_t> >
 inline Iter_t upper_bound ( Iter_t first, Iter_t last ,
 const value_iter<Iter_t> &val,
 Compare &comp = Compare() )
 {
 assert ( (last - first ) >= 0 );
 if ( last == first ) return last ;
 Iter_t itaux = internal_find_last( first, last, val,comp);
 return ( itaux == first and comp (val,*itaux))? itaux: itaux + 1;
 };
 //----------------------------------------------------------------------------
 //  function :equal_range
 /// @brief return a pair of lower_bound and upper_bound with the value val.If
 ///        don't exist return last in the two elements of the pair
 //
 /// @param [in] first : iterator to the first element of the range
 /// @param [in] last : iterator to the last element of the range
 /// @param [in] val : value to find
 /// @param [in] comp : object for to compare two value_t objects
 /// @return pair of iterators
 //-----------------------------------------------------------------------------
 template < class Iter_t, class Compare = compare_iter<Iter_t> >
 inline std::pair<Iter_t, Iter_t> equal_range ( Iter_t first, Iter_t last ,
 const value_iter<Iter_t> &val,
 Compare &comp = Compare() )
 {
 return std::make_pair(lower_bound(first, last, val,comp),
 upper_bound(first, last, val,comp));
 };
 //
 //-----------------------------------------------------------------------------
 //  function : insert_first
 /// @brief find if a value exist in the range [first, last). If exist return the
 ///        iterator to the first occurrence. If don't exist return last
 //
 /// @param [in] first : iterator to the first element of the range
 /// @param [in] last : iterator to the last element of the range
 /// @param [in] val : value to find
 /// @param [in] comp : object for to compare two value_t objects
 /// @return iterator to the element found, and if not last
 //-----------------------------------------------------------------------------
 template < class Iter_t, class Compare = compare_iter<Iter_t> >
 inline Iter_t insert_first ( Iter_t first, Iter_t last,
 const value_iter<Iter_t> &val,
 Compare comp = Compare() )
 {
 return lower_bound (first, last, val, comp);
 };
 //
 //-----------------------------------------------------------------------------
 //  function : insert_last
 /// @brief find if a value exist in the range [first, last). If exist return the
 ///        iterator to the last occurrence. If don't exist return last
 //
 /// @param [in] first : iterator to the first element of the range
 /// @param [in] last : iterator to the last element of the range
 /// @param [in] val : value to find
 /// @param [in] comp : object for to compare two value_t objects
 /// @return iterator to the element found, if not found return last

 //-----------------------------------------------------------------------------
 template < class Iter_t, class Compare = compare_iter<Iter_t> >
 inline Iter_t insert_last ( Iter_t first, Iter_t last ,
 const value_iter<Iter_t> &val,
 Compare comp = Compare())
 {
 return upper_bound (first, last, val, comp);
 };

 */
//
//****************************************************************************
};//    End namespace util
};//    End namespace common
};//    End namespace sort
};//    End namespace boost
//****************************************************************************
//
#endif

/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2011: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TYPE_TRAITS_SUCC_PRED_HPP_JOFA_080913
#define BOOST_ICL_TYPE_TRAITS_SUCC_PRED_HPP_JOFA_080913

#include <boost/icl/type_traits/is_increasing.hpp>

namespace boost{ namespace icl
{
template <class IncrementableT>
inline static IncrementableT succ(IncrementableT x) { return ++x; }

template <class DecrementableT>
inline static DecrementableT pred(DecrementableT x) { return --x; }

namespace detail
{
    template <class DomainT, bool increasing = true>
    struct successor;

    template <class DomainT>
    struct successor<DomainT, true>
    {
        typedef successor type;
        inline static DomainT apply(DomainT value){ return ++value; }
    };

    template <class DomainT>
    struct successor<DomainT, false>
    {
        typedef successor type;
        inline static DomainT apply(DomainT value){ return --value; }
    };

    template <class DomainT, bool increasing = true>
    struct predecessor;

    template <class DomainT>
    struct predecessor<DomainT, true>
    {
        typedef predecessor type;
        inline static DomainT apply(DomainT value){ return --value; }
    };

    template <class DomainT>
    struct predecessor<DomainT, false>
    {
        typedef predecessor type;
        inline static DomainT apply(DomainT value){ return ++value; }
    };
} // namespace detail

//------------------------------------------------------------------------------
template <class DomainT, class Compare>
struct successor
{
    inline static DomainT apply(DomainT value)
    {
        return detail::successor
            <DomainT, is_increasing<DomainT,Compare>::value>::apply(value);
    }
};

template <class DomainT, class Compare>
struct predecessor
{
    inline static DomainT apply(DomainT value)
    {
        return detail::predecessor
            <DomainT, is_increasing<DomainT,Compare>::value>::apply(value);
    }
};

}} // namespace boost icl

#endif



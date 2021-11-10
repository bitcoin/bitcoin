// Boost.Range library
//
//  Copyright Adam D. Walling 2012. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_ADAPTOR_MFC_MAP_HPP
#define BOOST_RANGE_ADAPTOR_MFC_MAP_HPP

#if !defined(BOOST_RANGE_MFC_NO_CPAIR)

#include <boost/range/mfc.hpp>
#include <boost/range/adaptor/map.hpp>

namespace boost
{
    namespace range_detail
    {
        // CMap and CMapStringToString range iterators return CPair,
        // which has a key and value member. Other MFC range iterators
        // already return adapted std::pair objects. This allows usage
        // of the map_keys and map_values range adaptors with CMap 
        // and CMapStringToString
        
        // CPair has a VALUE value member, and a KEY key member; we will
        // use VALUE& as the result_type consistent with CMap::operator[]
        
        // specialization for CMap 
        template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
        struct select_first< CMap<KEY, ARG_KEY, VALUE, ARG_VALUE> >
        {
            typedef BOOST_DEDUCED_TYPENAME CMap<KEY, ARG_KEY, VALUE, ARG_VALUE> map_type;
            typedef BOOST_DEDUCED_TYPENAME range_reference<const map_type>::type argument_type;
            typedef BOOST_DEDUCED_TYPENAME const KEY& result_type;

            result_type operator()( argument_type r ) const
            {
                return r.key;
            }
        };

        template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
        struct select_second_mutable< CMap<KEY, ARG_KEY, VALUE, ARG_VALUE> >
        {
            typedef BOOST_DEDUCED_TYPENAME CMap<KEY, ARG_KEY, VALUE, ARG_VALUE> map_type;
            typedef BOOST_DEDUCED_TYPENAME range_reference<map_type>::type argument_type;
            typedef BOOST_DEDUCED_TYPENAME VALUE& result_type;

            result_type operator()( argument_type r ) const
            {
                return r.value;
            }
        };

        template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
        struct select_second_const< CMap<KEY, ARG_KEY, VALUE, ARG_VALUE> >
        {
            typedef BOOST_DEDUCED_TYPENAME CMap<KEY, ARG_KEY, VALUE, ARG_VALUE> map_type;
            typedef BOOST_DEDUCED_TYPENAME range_reference<const map_type>::type argument_type;
            typedef BOOST_DEDUCED_TYPENAME const VALUE& result_type;

            result_type operator()( argument_type r ) const
            {
                return r.value;
            }
        };


        // specialization for CMapStringToString
        template<>
        struct select_first< CMapStringToString >
        {
            typedef range_reference<const CMapStringToString>::type argument_type;
            typedef const CString& result_type;

            result_type operator()( argument_type r ) const
            {
                return r.key;
            }
        };

        template<>
        struct select_second_mutable< CMapStringToString >
        {
            typedef range_reference<CMapStringToString>::type argument_type;
            typedef CString& result_type;

            result_type operator()( argument_type r ) const
            {
                return r.value;
            }
        };

        template<>
        struct select_second_const< CMapStringToString >
        {
            typedef range_reference<const CMapStringToString>::type argument_type;
            typedef const CString& result_type;

            result_type operator()( argument_type r ) const
            {
                return r.value;
            }
        };
    } // 'range_detail'
} // 'boost'

#endif // !defined(BOOST_RANGE_MFC_NO_CPAIR)

#endif

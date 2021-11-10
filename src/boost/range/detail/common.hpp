// Boost.Range library
//
//  Copyright Thorsten Ottosen 2003-2004. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_DETAIL_COMMON_HPP
#define BOOST_RANGE_DETAIL_COMMON_HPP

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/range/config.hpp>
#include <boost/range/detail/sfinae.hpp>
#include <boost/type_traits/is_void.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/int.hpp>
#include <cstddef>

//////////////////////////////////////////////////////////////////////////////
// missing partial specialization  workaround.
//////////////////////////////////////////////////////////////////////////////

namespace boost 
{
    namespace range_detail 
    {        
        // 1 = std containers
        // 2 = std::pair
        // 3 = const std::pair
        // 4 = array
        // 5 = const array
        // 6 = char array
        // 7 = wchar_t array
        // 8 = char*
        // 9 = const char*
        // 10 = whar_t*
        // 11 = const wchar_t*
        // 12 = string
        
        typedef mpl::int_<1>::type    std_container_;
        typedef mpl::int_<2>::type    std_pair_;
        typedef mpl::int_<3>::type    const_std_pair_;
        typedef mpl::int_<4>::type    array_;
        typedef mpl::int_<5>::type    const_array_;
        typedef mpl::int_<6>::type    char_array_;
        typedef mpl::int_<7>::type    wchar_t_array_;
        typedef mpl::int_<8>::type    char_ptr_;
        typedef mpl::int_<9>::type    const_char_ptr_;
        typedef mpl::int_<10>::type   wchar_t_ptr_;
        typedef mpl::int_<11>::type   const_wchar_t_ptr_;
        typedef mpl::int_<12>::type   string_;
        
        template< typename C >
        struct range_helper
        {
            static C* c;
            static C  ptr;

            BOOST_STATIC_CONSTANT( bool, is_pair_                = sizeof( boost::range_detail::is_pair_impl( c ) ) == sizeof( yes_type ) );
            BOOST_STATIC_CONSTANT( bool, is_char_ptr_            = sizeof( boost::range_detail::is_char_ptr_impl( ptr ) ) == sizeof( yes_type ) );
            BOOST_STATIC_CONSTANT( bool, is_const_char_ptr_      = sizeof( boost::range_detail::is_const_char_ptr_impl( ptr ) ) == sizeof( yes_type ) );
            BOOST_STATIC_CONSTANT( bool, is_wchar_t_ptr_         = sizeof( boost::range_detail::is_wchar_t_ptr_impl( ptr ) ) == sizeof( yes_type ) );
            BOOST_STATIC_CONSTANT( bool, is_const_wchar_t_ptr_   = sizeof( boost::range_detail::is_const_wchar_t_ptr_impl( ptr ) ) == sizeof( yes_type ) );
            BOOST_STATIC_CONSTANT( bool, is_char_array_          = sizeof( boost::range_detail::is_char_array_impl( ptr ) ) == sizeof( yes_type ) );
            BOOST_STATIC_CONSTANT( bool, is_wchar_t_array_       = sizeof( boost::range_detail::is_wchar_t_array_impl( ptr ) ) == sizeof( yes_type ) );
            BOOST_STATIC_CONSTANT( bool, is_string_              = (is_const_char_ptr_ || is_const_wchar_t_ptr_));
            BOOST_STATIC_CONSTANT( bool, is_array_               = boost::is_array<C>::value );
            
        };
        
        template< typename C >
        class range
        {
            typedef BOOST_RANGE_DEDUCED_TYPENAME   boost::mpl::if_c< ::boost::range_detail::range_helper<C>::is_pair_,
                                                                  boost::range_detail::std_pair_,
                                                                  void >::type pair_t;
            typedef BOOST_RANGE_DEDUCED_TYPENAME   boost::mpl::if_c< ::boost::range_detail::range_helper<C>::is_array_,
                                                                    boost::range_detail::array_,
                                                                    pair_t >::type array_t;
            typedef BOOST_RANGE_DEDUCED_TYPENAME   boost::mpl::if_c< ::boost::range_detail::range_helper<C>::is_string_,
                                                                    boost::range_detail::string_,
                                                                    array_t >::type string_t;
            typedef BOOST_RANGE_DEDUCED_TYPENAME   boost::mpl::if_c< ::boost::range_detail::range_helper<C>::is_const_char_ptr_,
                                                                    boost::range_detail::const_char_ptr_,
                                                                    string_t >::type const_char_ptr_t;
            typedef BOOST_RANGE_DEDUCED_TYPENAME   boost::mpl::if_c< ::boost::range_detail::range_helper<C>::is_char_ptr_,
                                                                    boost::range_detail::char_ptr_,
                                                                    const_char_ptr_t >::type char_ptr_t;
            typedef BOOST_RANGE_DEDUCED_TYPENAME   boost::mpl::if_c< ::boost::range_detail::range_helper<C>::is_const_wchar_t_ptr_,
                                                                    boost::range_detail::const_wchar_t_ptr_,
                                                                    char_ptr_t >::type const_wchar_ptr_t;
            typedef BOOST_RANGE_DEDUCED_TYPENAME   boost::mpl::if_c< ::boost::range_detail::range_helper<C>::is_wchar_t_ptr_,
                                                                    boost::range_detail::wchar_t_ptr_,
                                                                    const_wchar_ptr_t >::type wchar_ptr_t;
            typedef BOOST_RANGE_DEDUCED_TYPENAME   boost::mpl::if_c< ::boost::range_detail::range_helper<C>::is_wchar_t_array_,
                                                                    boost::range_detail::wchar_t_array_,
                                                                    wchar_ptr_t >::type wchar_array_t;
            typedef BOOST_RANGE_DEDUCED_TYPENAME   boost::mpl::if_c< ::boost::range_detail::range_helper<C>::is_char_array_,
                                                                    boost::range_detail::char_array_,
                                                                    wchar_array_t >::type char_array_t;
        public:
            typedef BOOST_RANGE_DEDUCED_TYPENAME   boost::mpl::if_c< ::boost::is_void<char_array_t>::value,
                                                                    boost::range_detail::std_container_,
                                                                    char_array_t >::type type;  
        }; // class 'range' 
    }
}
        
#endif


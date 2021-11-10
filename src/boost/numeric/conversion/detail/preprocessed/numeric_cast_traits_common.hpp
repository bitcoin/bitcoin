//
//! Copyright (c) 2011-2012
//! Brandon Kohn
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
	
	
	
namespace boost { namespace numeric {
    
    template <>
    struct numeric_cast_traits
        <
            char
          , char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            char
          , signed char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<signed char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            char
          , unsigned char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            char
          , short
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<short> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            char
          , unsigned short
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned short> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            char
          , int
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<int> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            char
          , unsigned int
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned int> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            char
          , long
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<long> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            char
          , unsigned long
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned long> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            char
          , float
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<float> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            char
          , double
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<double> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            char
          , long double
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<long double> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            signed char
          , char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            signed char
          , signed char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<signed char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            signed char
          , unsigned char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            signed char
          , short
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<short> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            signed char
          , unsigned short
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned short> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            signed char
          , int
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<int> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            signed char
          , unsigned int
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned int> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            signed char
          , long
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<long> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            signed char
          , unsigned long
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned long> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            signed char
          , float
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<float> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            signed char
          , double
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<double> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            signed char
          , long double
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<long double> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned char
          , char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned char
          , signed char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<signed char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned char
          , unsigned char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned char
          , short
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<short> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned char
          , unsigned short
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned short> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned char
          , int
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<int> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned char
          , unsigned int
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned int> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned char
          , long
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<long> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned char
          , unsigned long
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned long> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned char
          , float
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<float> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned char
          , double
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<double> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned char
          , long double
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<long double> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            short
          , char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            short
          , signed char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<signed char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            short
          , unsigned char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            short
          , short
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<short> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            short
          , unsigned short
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned short> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            short
          , int
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<int> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            short
          , unsigned int
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned int> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            short
          , long
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<long> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            short
          , unsigned long
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned long> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            short
          , float
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<float> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            short
          , double
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<double> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            short
          , long double
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<long double> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned short
          , char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned short
          , signed char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<signed char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned short
          , unsigned char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned short
          , short
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<short> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned short
          , unsigned short
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned short> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned short
          , int
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<int> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned short
          , unsigned int
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned int> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned short
          , long
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<long> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned short
          , unsigned long
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned long> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned short
          , float
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<float> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned short
          , double
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<double> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned short
          , long double
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<long double> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            int
          , char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            int
          , signed char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<signed char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            int
          , unsigned char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            int
          , short
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<short> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            int
          , unsigned short
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned short> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            int
          , int
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<int> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            int
          , unsigned int
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned int> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            int
          , long
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<long> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            int
          , unsigned long
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned long> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            int
          , float
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<float> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            int
          , double
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<double> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            int
          , long double
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<long double> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned int
          , char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned int
          , signed char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<signed char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned int
          , unsigned char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned int
          , short
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<short> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned int
          , unsigned short
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned short> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned int
          , int
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<int> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned int
          , unsigned int
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned int> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned int
          , long
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<long> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned int
          , unsigned long
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned long> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned int
          , float
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<float> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned int
          , double
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<double> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned int
          , long double
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<long double> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            long
          , char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            long
          , signed char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<signed char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            long
          , unsigned char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            long
          , short
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<short> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            long
          , unsigned short
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned short> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            long
          , int
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<int> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            long
          , unsigned int
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned int> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            long
          , long
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<long> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            long
          , unsigned long
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned long> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            long
          , float
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<float> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            long
          , double
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<double> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            long
          , long double
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<long double> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned long
          , char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned long
          , signed char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<signed char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned long
          , unsigned char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned long
          , short
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<short> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned long
          , unsigned short
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned short> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned long
          , int
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<int> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned long
          , unsigned int
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned int> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned long
          , long
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<long> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned long
          , unsigned long
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned long> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned long
          , float
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<float> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned long
          , double
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<double> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            unsigned long
          , long double
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<long double> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            float
          , char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            float
          , signed char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<signed char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            float
          , unsigned char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            float
          , short
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<short> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            float
          , unsigned short
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned short> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            float
          , int
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<int> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            float
          , unsigned int
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned int> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            float
          , long
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<long> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            float
          , unsigned long
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned long> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            float
          , float
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<float> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            float
          , double
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<double> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            float
          , long double
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<long double> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            double
          , char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            double
          , signed char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<signed char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            double
          , unsigned char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            double
          , short
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<short> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            double
          , unsigned short
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned short> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            double
          , int
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<int> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            double
          , unsigned int
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned int> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            double
          , long
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<long> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            double
          , unsigned long
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned long> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            double
          , float
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<float> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            double
          , double
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<double> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            double
          , long double
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<long double> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            long double
          , char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            long double
          , signed char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<signed char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            long double
          , unsigned char
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned char> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            long double
          , short
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<short> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            long double
          , unsigned short
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned short> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            long double
          , int
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<int> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            long double
          , unsigned int
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned int> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            long double
          , long
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<long> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            long double
          , unsigned long
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<unsigned long> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            long double
          , float
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<float> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            long double
          , double
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<double> rounding_policy;
    }; 
    
    template <>
    struct numeric_cast_traits
        <
            long double
          , long double
        >
    {
        typedef def_overflow_handler overflow_policy;
        typedef UseInternalRangeChecker range_checking_policy;
        typedef Trunc<long double> rounding_policy;
    }; 
}}

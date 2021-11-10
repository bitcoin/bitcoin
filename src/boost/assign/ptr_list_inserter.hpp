// Boost.Assign library
//
//  Copyright Thorsten Ottosen 2003-2005. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/assign/
//

#ifndef BOOST_ASSIGN_PTR_LIST_INSERTER_HPP
#define BOOST_ASSIGN_PTR_LIST_INSERTER_HPP

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/assign/list_inserter.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/move/utility.hpp>

#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/iteration/local.hpp>

#endif

namespace boost
{

namespace assign
{
    template< class Function, class Obj > 
    class ptr_list_inserter
    {
        typedef BOOST_DEDUCED_TYPENAME
                remove_pointer< BOOST_DEDUCED_TYPENAME 
                       remove_reference<Obj>::type >::type
           obj_type;
    public:
        
        ptr_list_inserter( Function fun ) : insert_( fun )
        {}
        
        template< class Function2, class Obj2 >
        ptr_list_inserter( const ptr_list_inserter<Function2,Obj2>& r ) 
        : insert_( r.fun_private() ) 
        {}

        ptr_list_inserter( const ptr_list_inserter& r ) : insert_( r.insert_ )
        {}


#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        ptr_list_inserter& operator()()
        {
            insert_( new obj_type() );
            return *this;
        }
        
        template< class T >
        ptr_list_inserter& operator()( const T& t )
        {
            insert_( new obj_type(t) );
            return *this;
        }

#ifndef BOOST_ASSIGN_MAX_PARAMS // use user's value
#define BOOST_ASSIGN_MAX_PARAMS 5        
#endif
#define BOOST_ASSIGN_MAX_PARAMETERS (BOOST_ASSIGN_MAX_PARAMS - 1)
#define BOOST_ASSIGN_PARAMS1(n) BOOST_PP_ENUM_PARAMS(n, class T)
#define BOOST_ASSIGN_PARAMS2(n) BOOST_PP_ENUM_BINARY_PARAMS(n, T, const& t)
#define BOOST_ASSIGN_PARAMS3(n) BOOST_PP_ENUM_PARAMS(n, t)
        
#define BOOST_PP_LOCAL_LIMITS (1, BOOST_ASSIGN_MAX_PARAMETERS)
#define BOOST_PP_LOCAL_MACRO(n) \
    template< class T, BOOST_ASSIGN_PARAMS1(n) > \
    ptr_list_inserter& operator()( const T& t, BOOST_ASSIGN_PARAMS2(n) ) \
    { \
        insert_( new obj_type(t, BOOST_ASSIGN_PARAMS3(n) )); \
        return *this; \
    } \
    /**/
        
#include BOOST_PP_LOCAL_ITERATE()

#else

        template< class... Ts >
        ptr_list_inserter& operator()(Ts&&... ts)
        {
            insert_(new obj_type(boost::forward<Ts>(ts)...));
            return *this;
        }

#endif

    private:
        
        ptr_list_inserter& operator=( const ptr_list_inserter& );
        Function insert_;
    };
    
    template< class Obj, class Function >
    inline ptr_list_inserter< Function, Obj >
    make_ptr_list_inserter( Function fun )
    {
        return ptr_list_inserter< Function, Obj >( fun );
    }
    
    template< class C >
    inline ptr_list_inserter< assign_detail::call_push_back<C>, 
                              BOOST_DEDUCED_TYPENAME C::reference >
    ptr_push_back( C& c )
    {
        return make_ptr_list_inserter<BOOST_DEDUCED_TYPENAME C::reference>
                   ( assign_detail::call_push_back<C>( c ) ); 
    }

#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING

    template< class T, class C >
    inline ptr_list_inserter< assign_detail::call_push_back<C>, T >
    ptr_push_back( C& c )
    {
        return make_ptr_list_inserter<T>( 
                    assign_detail::call_push_back<C>( c ) );
    }

#endif
    
    template< class C >
    inline ptr_list_inserter< assign_detail::call_push_front<C>,
                              BOOST_DEDUCED_TYPENAME C::reference >
    ptr_push_front( C& c )
    {
        return make_ptr_list_inserter<BOOST_DEDUCED_TYPENAME C::reference>
                 ( assign_detail::call_push_front<C>( c ) );
    }

#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING

    template< class T, class C >
    inline ptr_list_inserter< assign_detail::call_push_front<C>, T >
    ptr_push_front( C& c )
    {
        return make_ptr_list_inserter<T>( 
                    assign_detail::call_push_front<C>( c ) );
    }

#endif
    
    template< class C >
    inline ptr_list_inserter< assign_detail::call_insert<C>, 
                          BOOST_DEDUCED_TYPENAME C::reference>
    ptr_insert( C& c )
    {
        return make_ptr_list_inserter<BOOST_DEDUCED_TYPENAME C::reference>
                    ( assign_detail::call_insert<C>( c ) );
    }

#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING

    template< class T, class C >
    inline ptr_list_inserter< assign_detail::call_insert<C>, T >
    ptr_insert( C& c )
    {
        return make_ptr_list_inserter<T>( assign_detail::call_insert<C>( c ) );
    }

#endif
    
    
} // namespace 'assign'
} // namespace 'boost'

#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

#undef BOOST_ASSIGN_PARAMS1
#undef BOOST_ASSIGN_PARAMS2
#undef BOOST_ASSIGN_PARAMS3
#undef BOOST_ASSIGN_MAX_PARAMETERS

#endif

#endif

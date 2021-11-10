// Boost.Assign library
//
//  Copyright Thorsten Ottosen 2003-2004. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/assign/
//

#ifndef BOOST_ASSIGN_LIST_INSERTER_HPP
#define BOOST_ASSIGN_LIST_INSERTER_HPP

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/detail/workaround.hpp>

#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/config.hpp>
#include <boost/move/utility.hpp>
#include <cstddef>

#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/iteration/local.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>

#endif

namespace boost
{
namespace assign_detail
{
    template< class T >
    struct repeater
    {
        std::size_t  sz;
        T            val;

        repeater( std::size_t sz_, T r ) : sz( sz_ ), val( r )
        { }
    };
    
    template< class Fun >
    struct fun_repeater
    {
        std::size_t  sz;
        Fun          val;
        
        fun_repeater( std::size_t sz_, Fun r ) : sz( sz_ ), val( r )
        { }
    };


    template< class T >
    struct is_repeater : boost::false_type {};

    template< class T >
    struct is_repeater< boost::assign_detail::repeater<T> > : boost::true_type{};

    template< class Fun >
    struct is_repeater< boost::assign_detail::fun_repeater<Fun> > : boost::true_type{};


    template< class C >
    class call_push_back
    {
        C& c_;
    public:
        call_push_back( C& c ) : c_( c )
        { }
        
#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template< class T >
        void operator()( T r ) 
        {
            c_.push_back( r );
        }
#else
        template< class T >
        void operator()(T&& r)
        {
            c_.push_back(boost::forward<T>(r));
        }
#endif
    };
    
    template< class C >
    class call_push_front
    {
        C& c_;
    public:
        call_push_front( C& c ) : c_( c )
        { }

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template< class T >
        void operator()( T r ) 
        {
            c_.push_front( r );
        }
#else
        template< class T >
        void operator()(T&& r)
        {
            c_.push_front(boost::forward<T>(r));
        }
#endif
    };
    
    template< class C >
    class call_push
    {
        C& c_;
    public:
        call_push( C& c ) : c_( c )
        { }
    
#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template< class T >
        void operator()( T r ) 
        {
            c_.push( r );
        }
#else
        template< class T >
        void operator()(T&& r)
        {
            c_.push(boost::forward<T>(r));
        }
#endif
    };
    
    template< class C >
    class call_insert
    {
        C& c_;
    public:
        call_insert( C& c ) : c_( c )
        { }

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template< class T >
        void operator()( T r ) 
        {
            c_.insert( r );
        }
#else
        template< class T >
        void operator()(T&& r)
        {
            c_.insert(boost::forward<T>(r));
        }
#endif
    };

    template< class C >
    class call_add_edge
    {
        C& c_;
    public:
        call_add_edge( C& c ) : c_(c)
        { }

        template< class T >
        void operator()( T l, T r )
        {
            add_edge( l, r, c_ );
        }

        template< class T, class EP >
        void operator()( T l, T r, const EP& ep )
        {
            add_edge( l, r, ep, c_ );
        }

    };
    
    struct forward_n_arguments {};
    
} // namespace 'assign_detail'

namespace assign
{

    template< class T >
    inline assign_detail::repeater<T>
    repeat( std::size_t sz, T r )
    {
        return assign_detail::repeater<T>( sz, r );
    }
    
    template< class Function >
    inline assign_detail::fun_repeater<Function>
    repeat_fun( std::size_t sz, Function r )
    {
        return assign_detail::fun_repeater<Function>( sz, r );
    }
    

    template< class Function, class Argument = assign_detail::forward_n_arguments > 
    class list_inserter
    {
        struct single_arg_type   {};
        struct n_arg_type        {};
        struct repeater_arg_type {};

        typedef BOOST_DEDUCED_TYPENAME mpl::if_c< is_same<Argument,assign_detail::forward_n_arguments>::value,
                                                  n_arg_type,
                                                  single_arg_type >::type arg_type;  
            
    public:
        
        list_inserter( Function fun ) : insert_( fun )
        {}
        
        template< class Function2, class Arg >
        list_inserter( const list_inserter<Function2,Arg>& r ) 
        : insert_( r.fun_private() ) 
        {}

        list_inserter( const list_inserter& r ) : insert_( r.insert_ )
        {}

        list_inserter& operator()()
        {
            insert_( Argument() );
            return *this;
        }

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template< class T >
        list_inserter& operator=( const T& r )
        {
            insert_( r );
            return *this;
        }

        template< class T >
        list_inserter& operator=( assign_detail::repeater<T> r )
        {
            return operator,( r );
        }
        
        template< class Nullary_function >
        list_inserter& operator=( const assign_detail::fun_repeater<Nullary_function>& r )
        {
            return operator,( r );
        }
        
        template< class T >
        list_inserter& operator,( const T& r )
        {
            insert_( r  );
            return *this;
        }

#if BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3205))
        template< class T >
        list_inserter& operator,( const assign_detail::repeater<T> & r )
        {
            return repeat( r.sz, r.val ); 
        }
#else
        template< class T >
        list_inserter& operator,( assign_detail::repeater<T> r )
        {
            return repeat( r.sz, r.val ); 
        }
#endif
        
        template< class Nullary_function >
        list_inserter& operator,( const assign_detail::fun_repeater<Nullary_function>& r )
        {
            return repeat_fun( r.sz, r.val ); 
        }
#else
        // BOOST_NO_CXX11_RVALUE_REFERENCES
        template< class T >
        list_inserter& operator=(T&& r)
        {
            return operator,(boost::forward<T>(r));
        }
#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
        template< class T >
        list_inserter& operator,(T&& r)
        {
            typedef BOOST_DEDUCED_TYPENAME mpl::if_c< assign_detail::is_repeater< T >::value,
                repeater_arg_type,
                arg_type >::type tag;

            insert(boost::forward<T>(r), tag());
            return *this;
        }
#else
        // we add the tag as the first argument when using variadic templates
        template< class T >
        list_inserter& operator,(T&& r)
        {
            typedef BOOST_DEDUCED_TYPENAME mpl::if_c< assign_detail::is_repeater< T >::value,
                repeater_arg_type,
                arg_type >::type tag;

            insert(tag(), boost::forward<T>(r));
            return *this;
        }
#endif
#endif

        template< class T >
        list_inserter& repeat( std::size_t sz, T r )
        {
            std::size_t i = 0;
            while( i++ != sz )
                insert_( r );
            return *this;
        }
        
        template< class Nullary_function >
        list_inserter& repeat_fun( std::size_t sz, Nullary_function fun )
        {
            std::size_t i = 0;
            while( i++ != sz )
                insert_( fun() );
            return *this;
        }

        template< class SinglePassIterator >
        list_inserter& range( SinglePassIterator first, 
                              SinglePassIterator last )
        {
            for( ; first != last; ++first )
                insert_( *first );
            return *this;
        }
        
        template< class SinglePassRange >
        list_inserter& range( const SinglePassRange& r )
        {
            return range( boost::begin(r), boost::end(r) );
        }
        
#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template< class T >
        list_inserter& operator()( const T& t )
        {
            insert_( t );
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
    list_inserter& operator()(T t, BOOST_ASSIGN_PARAMS2(n) ) \
        { \
            BOOST_PP_CAT(insert, BOOST_PP_INC(n))(t, BOOST_ASSIGN_PARAMS3(n), arg_type()); \
            return *this; \
        } \
        /**/

#include BOOST_PP_LOCAL_ITERATE()
        

#define BOOST_PP_LOCAL_LIMITS (1, BOOST_ASSIGN_MAX_PARAMETERS)
#define BOOST_PP_LOCAL_MACRO(n) \
    template< class T, BOOST_ASSIGN_PARAMS1(n) > \
    void BOOST_PP_CAT(insert, BOOST_PP_INC(n))(T const& t, BOOST_ASSIGN_PARAMS2(n), single_arg_type) \
    { \
        insert_( Argument(t, BOOST_ASSIGN_PARAMS3(n) )); \
    } \
    /**/
        
#include BOOST_PP_LOCAL_ITERATE()

#define BOOST_PP_LOCAL_LIMITS (1, BOOST_ASSIGN_MAX_PARAMETERS)
#define BOOST_PP_LOCAL_MACRO(n) \
    template< class T, BOOST_ASSIGN_PARAMS1(n) > \
    void BOOST_PP_CAT(insert, BOOST_PP_INC(n))(T const& t, BOOST_ASSIGN_PARAMS2(n), n_arg_type) \
    { \
        insert_(t, BOOST_ASSIGN_PARAMS3(n) ); \
    } \
    /**/
        
#include BOOST_PP_LOCAL_ITERATE()

#else
        template< class... Ts >
        list_inserter& operator()(Ts&&... ts)
        {
            insert(arg_type(), boost::forward<Ts>(ts)...);
            return *this;
        }

        template< class T >
        void insert(single_arg_type, T&& t)
        {
            // Special implementation for single argument overload to prevent accidental casts (type-cast using functional notation)
            insert_(boost::forward<T>(t));
        }

        template< class T1, class T2, class... Ts >
        void insert(single_arg_type, T1&& t1, T2&& t2, Ts&&... ts)
        {
            insert_(Argument(boost::forward<T1>(t1), boost::forward<T2>(t2), boost::forward<Ts>(ts)...));
        }

        template< class... Ts >
        void insert(n_arg_type, Ts&&... ts)
        {
            insert_(boost::forward<Ts>(ts)...);
        }

#endif

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

        template< class T >
        void insert( T&& r, arg_type)
        {
            insert_( boost::forward<T>(r) );
        }

        template< class T >
        void insert(assign_detail::repeater<T> r, repeater_arg_type)
        {
            repeat(r.sz, r.val);
        }

        template< class Nullary_function >
        void insert(const assign_detail::fun_repeater<Nullary_function>& r, repeater_arg_type)
        {
            repeat_fun(r.sz, r.val);
        }
#else
        template< class T >
        void insert(repeater_arg_type, assign_detail::repeater<T> r)
        {
            repeat(r.sz, r.val);
        }

        template< class Nullary_function >
        void insert(repeater_arg_type, const assign_detail::fun_repeater<Nullary_function>& r)
        {
            repeat_fun(r.sz, r.val);
        }
#endif
#endif


        Function fun_private() const
        {
            return insert_;
        }

    private:
        
        list_inserter& operator=( const list_inserter& );
        Function insert_;
    };
    
    template< class Function >
    inline list_inserter< Function >
    make_list_inserter( Function fun )
    {
        return list_inserter< Function >( fun );
    }
    
    template< class Function, class Argument >
    inline list_inserter<Function,Argument>
    make_list_inserter( Function fun, Argument* )
    {
        return list_inserter<Function,Argument>( fun );
    }

    template< class C >
    inline list_inserter< assign_detail::call_push_back<C>, 
                          BOOST_DEDUCED_TYPENAME C::value_type >
    push_back( C& c )
    {
        static BOOST_DEDUCED_TYPENAME C::value_type* p = 0;
        return make_list_inserter( assign_detail::call_push_back<C>( c ), 
                                   p );
    }
    
    template< class C >
    inline list_inserter< assign_detail::call_push_front<C>,
                          BOOST_DEDUCED_TYPENAME C::value_type >
    push_front( C& c )
    {
        static BOOST_DEDUCED_TYPENAME C::value_type* p = 0;
        return make_list_inserter( assign_detail::call_push_front<C>( c ),
                                   p );
    }

    template< class C >
    inline list_inserter< assign_detail::call_insert<C>, 
                          BOOST_DEDUCED_TYPENAME C::value_type >
    insert( C& c )
    {
        static BOOST_DEDUCED_TYPENAME C::value_type* p = 0;
        return make_list_inserter( assign_detail::call_insert<C>( c ),
                                   p );
    }

    template< class C >
    inline list_inserter< assign_detail::call_push<C>, 
                          BOOST_DEDUCED_TYPENAME C::value_type >
    push( C& c )
    {
        static BOOST_DEDUCED_TYPENAME C::value_type* p = 0;
        return make_list_inserter( assign_detail::call_push<C>( c ),
                                   p );
    }

    template< class C >
    inline list_inserter< assign_detail::call_add_edge<C> >
    add_edge( C& c )   
    {
        return make_list_inserter( assign_detail::call_add_edge<C>( c ) );
    }
    
} // namespace 'assign'
} // namespace 'boost'

#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

#undef BOOST_ASSIGN_PARAMS1
#undef BOOST_ASSIGN_PARAMS2
#undef BOOST_ASSIGN_PARAMS3
#undef BOOST_ASSIGN_MAX_PARAMETERS

#endif

#endif

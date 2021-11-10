// Boost.Assign library
//
//  Copyright Thorsten Ottosen 2003-2005. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/assign/
//


#ifndef BOOST_ASSIGN_PTR_LIST_OF_HPP
#define BOOST_ASSIGN_PTR_LIST_OF_HPP

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/assign/list_of.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/detail/yes_no_type.hpp>
#include <boost/type_traits/decay.hpp>
#include <boost/type_traits/is_array.hpp>
#include <boost/mpl/if.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/move/utility.hpp>

#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/iteration/local.hpp>

#endif

namespace boost
{

namespace assign_detail
{
    /////////////////////////////////////////////////////////////////////////
    // Part 1: flexible and efficient interface
    /////////////////////////////////////////////////////////////////////////

    template< class T >
    class generic_ptr_list :
        public converter< generic_ptr_list<T>,
                          BOOST_DEDUCED_TYPENAME boost::ptr_vector<T>::iterator >
    {
    protected:
        typedef boost::ptr_vector<T>       impl_type;
#if defined(BOOST_NO_AUTO_PTR)
        typedef std::unique_ptr<impl_type> release_type;
#else
        typedef std::auto_ptr<impl_type>   release_type;
#endif	
        mutable impl_type                  values_;

    public:
        typedef BOOST_DEDUCED_TYPENAME impl_type::iterator         iterator;
        typedef iterator                                           const_iterator;
        typedef BOOST_DEDUCED_TYPENAME impl_type::value_type       value_type;
        typedef BOOST_DEDUCED_TYPENAME impl_type::size_type        size_type;
        typedef BOOST_DEDUCED_TYPENAME impl_type::difference_type  difference_type;
    public:
        generic_ptr_list() : values_( 32u )
        { }

        generic_ptr_list( release_type r ) : values_(r)
        { }

        release_type release()
        {
            return values_.release();
        }

    public:
        iterator begin() const       { return values_.begin(); }
        iterator end() const         { return values_.end(); }
        bool empty() const           { return values_.empty(); }
        size_type size() const       { return values_.size(); }

    public:

        operator impl_type() const
        {
            return values_;
        }

        template< template<class,class,class> class Seq, class U,
                  class CA, class A >
        operator Seq<U,CA,A>() const
        {
            Seq<U,CA,A> result;
            result.transfer( result.end(), values_ );
            BOOST_ASSERT( empty() );
            return result;
        }

        template< class PtrContainer >
#if defined(BOOST_NO_AUTO_PTR)
        std::unique_ptr<PtrContainer>
#else
        std::auto_ptr<PtrContainer>
#endif	
		convert( const PtrContainer* c ) const
        {
#if defined(BOOST_NO_AUTO_PTR)
            std::unique_ptr<PtrContainer> res( new PtrContainer() );
#else
            std::auto_ptr<PtrContainer> res( new PtrContainer() );
#endif	
            while( !empty() )
                res->insert( res->end(),
                             values_.pop_back().release() );
            return res;
        }

        template< class PtrContainer >
#if defined(BOOST_NO_AUTO_PTR)
        std::unique_ptr<PtrContainer>
#else
        std::auto_ptr<PtrContainer>
#endif	
        to_container( const PtrContainer& c ) const
        {
            return convert( &c );
        }

    protected:
        void push_back( T* r ) { values_.push_back( r ); }

    public:
#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

        generic_ptr_list& operator()()
        {
            this->push_back( new T() );
            return *this;
        }

        template< class U >
        generic_ptr_list& operator()( const U& u )
        {
            this->push_back( new T(u) );
            return *this;
        }


#ifndef BOOST_ASSIGN_MAX_PARAMS // use user's value
#define BOOST_ASSIGN_MAX_PARAMS 5
#endif
#define BOOST_ASSIGN_MAX_PARAMETERS (BOOST_ASSIGN_MAX_PARAMS - 1)
#define BOOST_ASSIGN_PARAMS1(n) BOOST_PP_ENUM_PARAMS(n, class U)
#define BOOST_ASSIGN_PARAMS2(n) BOOST_PP_ENUM_BINARY_PARAMS(n, U, const& u)
#define BOOST_ASSIGN_PARAMS3(n) BOOST_PP_ENUM_PARAMS(n, u)

#define BOOST_PP_LOCAL_LIMITS (1, BOOST_ASSIGN_MAX_PARAMETERS)
#define BOOST_PP_LOCAL_MACRO(n) \
    template< class U, BOOST_ASSIGN_PARAMS1(n) > \
    generic_ptr_list& operator()(U const& u, BOOST_ASSIGN_PARAMS2(n) ) \
    { \
        this->push_back( new T(u, BOOST_ASSIGN_PARAMS3(n))); \
        return *this; \
    } \
    /**/

#include BOOST_PP_LOCAL_ITERATE()

#else
        template< class... Us >
        generic_ptr_list& operator()(Us&&... us)
        {
            this->push_back(new T(boost::forward<Us>(us)...));
            return *this;
        }
#endif



    }; // class 'generic_ptr_list'

} // namespace 'assign_detail'

namespace assign
{
#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

    template< class T >
    inline assign_detail::generic_ptr_list<T>
    ptr_list_of()
    {
        assign_detail::generic_ptr_list<T> gpl;
        gpl();
        return gpl;
    }

    template< class T, class U >
    inline assign_detail::generic_ptr_list<T>
    ptr_list_of( const U& t )
    {
        assign_detail::generic_ptr_list<T> gpl;
        gpl( t );
        return gpl;
    }


#define BOOST_PP_LOCAL_LIMITS (1, BOOST_ASSIGN_MAX_PARAMETERS)
#define BOOST_PP_LOCAL_MACRO(n) \
    template< class T, class U, BOOST_ASSIGN_PARAMS1(n) > \
    inline assign_detail::generic_ptr_list<T> \
    ptr_list_of(U const& u, BOOST_ASSIGN_PARAMS2(n) ) \
    { \
        return assign_detail::generic_ptr_list<T>()(u, BOOST_ASSIGN_PARAMS3(n)); \
    } \
    /**/

#include BOOST_PP_LOCAL_ITERATE()

#else
    template< class T, class... Us >
    inline assign_detail::generic_ptr_list<T>
    ptr_list_of(Us&&... us)
    {
        assign_detail::generic_ptr_list<T> gpl;
        gpl(boost::forward<Us>(us)...);
        return gpl;
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

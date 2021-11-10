#ifndef BOOST_SIGNALS2_DECONSTRUCT_HPP
#define BOOST_SIGNALS2_DECONSTRUCT_HPP

//  deconstruct.hpp
//
// A factory function for creating a shared_ptr which creates
// an object and its owning shared_ptr with one allocation, similar
// to make_shared<T>().  It also supports postconstructors
// and predestructors through unqualified calls of adl_postconstruct() and
// adl_predestruct, relying on argument-dependent
// lookup to find the appropriate postconstructor or predestructor.
// Passing arguments to postconstructors is also supported.
//
//  based on make_shared.hpp and make_shared_access patch from Michael Marcin
//
//  Copyright (c) 2007, 2008 Peter Dimov
//  Copyright (c) 2008 Michael Marcin
//  Copyright (c) 2009 Frank Mori Hess
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
//  See http://www.boost.org
//  for more information

#include <boost/config.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/type_with_alignment.hpp>
#include <cstddef>
#include <new>

namespace boost
{
  template<typename T> class enable_shared_from_this;

namespace signals2
{
  class deconstruct_access;

namespace detail
{
  inline void adl_predestruct(...) {}
} // namespace detail

template<typename T>
    class postconstructor_invoker
{
public:
    operator const shared_ptr<T> & () const
    {
        return postconstruct();
    }
    const shared_ptr<T>& postconstruct() const
    {
        if(!_postconstructed)
        {
            adl_postconstruct(_sp, const_cast<typename boost::remove_const<T>::type *>(_sp.get()));
            _postconstructed = true;
        }
        return _sp;
    }
#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template<class... Args>
      const shared_ptr<T>& postconstruct(Args && ... args) const
    {
        if(!_postconstructed)
        {
            adl_postconstruct(_sp, const_cast<typename boost::remove_const<T>::type *>(_sp.get()),
                std::forward<Args>(args)...);
            _postconstructed = true;
        }
        return _sp;
    }
#else // !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template<typename A1>
      const shared_ptr<T>& postconstruct(const A1 &a1) const
    {
        if(!_postconstructed)
        {
            adl_postconstruct(_sp, const_cast<typename boost::remove_const<T>::type *>(_sp.get()),
                a1);
            _postconstructed = true;
        }
        return _sp;
    }
    template<typename A1, typename A2>
      const shared_ptr<T>& postconstruct(const A1 &a1, const A2 &a2) const
    {
        if(!_postconstructed)
        {
            adl_postconstruct(_sp, const_cast<typename boost::remove_const<T>::type *>(_sp.get()),
                a1, a2);
            _postconstructed = true;
        }
        return _sp;
    }
    template<typename A1, typename A2, typename A3>
      const shared_ptr<T>& postconstruct(const A1 &a1, const A2 &a2, const A3 &a3) const
    {
        if(!_postconstructed)
        {
            adl_postconstruct(_sp, const_cast<typename boost::remove_const<T>::type *>(_sp.get()),
                a1, a2, a3);
            _postconstructed = true;
        }
        return _sp;
    }
    template<typename A1, typename A2, typename A3, typename A4>
      const shared_ptr<T>& postconstruct(const A1 &a1, const A2 &a2, const A3 &a3, const A4 &a4) const
    {
        if(!_postconstructed)
        {
            adl_postconstruct(_sp, const_cast<typename boost::remove_const<T>::type *>(_sp.get()),
                a1, a2, a3, a4);
            _postconstructed = true;
        }
        return _sp;
    }
    template<typename A1, typename A2, typename A3, typename A4, typename A5>
      const shared_ptr<T>& postconstruct(const A1 &a1, const A2 &a2, const A3 &a3, const A4 &a4, const A5 &a5) const
    {
        if(!_postconstructed)
        {
            adl_postconstruct(_sp, const_cast<typename boost::remove_const<T>::type *>(_sp.get()),
                a1, a2, a3, a4, a5);
            _postconstructed = true;
        }
        return _sp;
    }
    template<typename A1, typename A2, typename A3, typename A4, typename A5,
      typename A6>
      const shared_ptr<T>& postconstruct(const A1 &a1, const A2 &a2, const A3 &a3, const A4 &a4, const A5 &a5,
      const A6 &a6) const
    {
        if(!_postconstructed)
        {
            adl_postconstruct(_sp, const_cast<typename boost::remove_const<T>::type *>(_sp.get()),
                a1, a2, a3, a4, a5, a6);
            _postconstructed = true;
        }
        return _sp;
    }
    template<typename A1, typename A2, typename A3, typename A4, typename A5,
      typename A6, typename A7>
      const shared_ptr<T>& postconstruct(const A1 &a1, const A2 &a2, const A3 &a3, const A4 &a4, const A5 &a5,
      const A6 &a6, const A7 &a7) const
    {
        if(!_postconstructed)
        {
            adl_postconstruct(_sp, const_cast<typename boost::remove_const<T>::type *>(_sp.get()),
                a1, a2, a3, a4, a5, a6, a7);
            _postconstructed = true;
        }
        return _sp;
    }
    template<typename A1, typename A2, typename A3, typename A4, typename A5,
      typename A6, typename A7, typename A8>
      const shared_ptr<T>& postconstruct(const A1 &a1, const A2 &a2, const A3 &a3, const A4 &a4, const A5 &a5,
      const A6 &a6, const A7 &a7, const A8 &a8) const
    {
        if(!_postconstructed)
        {
            adl_postconstruct(_sp, const_cast<typename boost::remove_const<T>::type *>(_sp.get()),
                a1, a2, a3, a4, a5, a6, a7, a8);
            _postconstructed = true;
        }
        return _sp;
    }
    template<typename A1, typename A2, typename A3, typename A4, typename A5,
      typename A6, typename A7, typename A8, typename A9>
      const shared_ptr<T>& postconstruct(const A1 &a1, const A2 &a2, const A3 &a3, const A4 &a4, const A5 &a5,
      const A6 &a6, const A7 &a7, const A8 &a8, const A9 &a9) const
    {
        if(!_postconstructed)
        {
            adl_postconstruct(_sp, const_cast<typename boost::remove_const<T>::type *>(_sp.get()),
                a1, a2, a3, a4, a5, a6, a7, a8, a9);
            _postconstructed = true;
        }
        return _sp;
    }
#endif // !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
private:
    friend class boost::signals2::deconstruct_access;
    postconstructor_invoker(const shared_ptr<T> & sp):
        _sp(sp), _postconstructed(false)
    {}
    shared_ptr<T> _sp;
    mutable bool _postconstructed;
};

namespace detail
{

template< std::size_t N, std::size_t A > struct sp_aligned_storage
{
    union type
    {
        char data_[ N ];
        typename boost::type_with_alignment< A >::type align_;
    };
};

template< class T > class deconstruct_deleter
{
private:

    typedef typename sp_aligned_storage< sizeof( T ), ::boost::alignment_of< T >::value >::type storage_type;

    bool initialized_;
    storage_type storage_;

private:

    void destroy()
    {
        if( initialized_ )
        {
            T* p = reinterpret_cast< T* >( storage_.data_ );
            using boost::signals2::detail::adl_predestruct;
            adl_predestruct(const_cast<typename boost::remove_const<T>::type *>(p));
            p->~T();
            initialized_ = false;
        }
    }

public:

    deconstruct_deleter(): initialized_( false )
    {
    }

    // this copy constructor is an optimization: we don't need to copy the storage_ member,
    // and shouldn't be copying anyways after initialized_ becomes true
    deconstruct_deleter(const deconstruct_deleter &): initialized_( false )
    {
    }

    ~deconstruct_deleter()
    {
        destroy();
    }

    void operator()( T * )
    {
        destroy();
    }

    void * address()
    {
        return storage_.data_;
    }

    void set_initialized()
    {
        initialized_ = true;
    }
};
}  // namespace detail

class deconstruct_access
{
public:

    template< class T >
    static postconstructor_invoker<T> deconstruct()
    {
        boost::shared_ptr< T > pt( static_cast< T* >( 0 ), detail::deconstruct_deleter< T >() );

        detail::deconstruct_deleter< T > * pd = boost::get_deleter< detail::deconstruct_deleter< T > >( pt );

        void * pv = pd->address();

        new( pv ) T();
        pd->set_initialized();

        boost::shared_ptr< T > retval( pt, static_cast< T* >( pv ) );
        boost::detail::sp_enable_shared_from_this(&retval, retval.get(), retval.get());
        return retval;

    }

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

    // Variadic templates, rvalue reference

    template< class T, class... Args >
    static postconstructor_invoker<T> deconstruct( Args && ... args )
    {
        boost::shared_ptr< T > pt( static_cast< T* >( 0 ), detail::deconstruct_deleter< T >() );

        detail::deconstruct_deleter< T > * pd = boost::get_deleter< detail::deconstruct_deleter< T > >( pt );

        void * pv = pd->address();

        new( pv ) T( std::forward<Args>( args )... );
        pd->set_initialized();

        boost::shared_ptr< T > retval( pt, static_cast< T* >( pv ) );
        boost::detail::sp_enable_shared_from_this(&retval, retval.get(), retval.get());
        return retval;
    }

#else

    template< class T, class A1 >
    static postconstructor_invoker<T> deconstruct( A1 const & a1 )
    {
        boost::shared_ptr< T > pt( static_cast< T* >( 0 ), detail::deconstruct_deleter< T >() );

        detail::deconstruct_deleter< T > * pd = boost::get_deleter< detail::deconstruct_deleter< T > >( pt );

        void * pv = pd->address();

        new( pv ) T( a1 );
        pd->set_initialized();

        boost::shared_ptr< T > retval( pt, static_cast< T* >( pv ) );
        boost::detail::sp_enable_shared_from_this(&retval, retval.get(), retval.get());
        return retval;
    }

    template< class T, class A1, class A2 >
    static postconstructor_invoker<T> deconstruct( A1 const & a1, A2 const & a2 )
    {
        boost::shared_ptr< T > pt( static_cast< T* >( 0 ), detail::deconstruct_deleter< T >() );

        detail::deconstruct_deleter< T > * pd = boost::get_deleter< detail::deconstruct_deleter< T > >( pt );

        void * pv = pd->address();

        new( pv ) T( a1, a2 );
        pd->set_initialized();

        boost::shared_ptr< T > retval( pt, static_cast< T* >( pv ) );
        boost::detail::sp_enable_shared_from_this(&retval, retval.get(), retval.get());
        return retval;
    }

    template< class T, class A1, class A2, class A3 >
    static postconstructor_invoker<T> deconstruct( A1 const & a1, A2 const & a2, A3 const & a3 )
    {
        boost::shared_ptr< T > pt( static_cast< T* >( 0 ), detail::deconstruct_deleter< T >() );

        detail::deconstruct_deleter< T > * pd = boost::get_deleter< detail::deconstruct_deleter< T > >( pt );

        void * pv = pd->address();

        new( pv ) T( a1, a2, a3 );
        pd->set_initialized();

        boost::shared_ptr< T > retval( pt, static_cast< T* >( pv ) );
        boost::detail::sp_enable_shared_from_this(&retval, retval.get(), retval.get());
        return retval;
    }

    template< class T, class A1, class A2, class A3, class A4 >
    static postconstructor_invoker<T> deconstruct( A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4 )
    {
        boost::shared_ptr< T > pt( static_cast< T* >( 0 ), detail::deconstruct_deleter< T >() );

        detail::deconstruct_deleter< T > * pd = boost::get_deleter< detail::deconstruct_deleter< T > >( pt );

        void * pv = pd->address();

        new( pv ) T( a1, a2, a3, a4 );
        pd->set_initialized();

        boost::shared_ptr< T > retval( pt, static_cast< T* >( pv ) );
        boost::detail::sp_enable_shared_from_this(&retval, retval.get(), retval.get());
        return retval;
    }

    template< class T, class A1, class A2, class A3, class A4, class A5 >
    static postconstructor_invoker<T> deconstruct( A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5 )
    {
        boost::shared_ptr< T > pt( static_cast< T* >( 0 ), detail::deconstruct_deleter< T >() );

        detail::deconstruct_deleter< T > * pd = boost::get_deleter< detail::deconstruct_deleter< T > >( pt );

        void * pv = pd->address();

        new( pv ) T( a1, a2, a3, a4, a5 );
        pd->set_initialized();

        boost::shared_ptr< T > retval( pt, static_cast< T* >( pv ) );
        boost::detail::sp_enable_shared_from_this(&retval, retval.get(), retval.get());
        return retval;
    }

    template< class T, class A1, class A2, class A3, class A4, class A5, class A6 >
    static postconstructor_invoker<T> deconstruct( A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6 )
    {
        boost::shared_ptr< T > pt( static_cast< T* >( 0 ), detail::deconstruct_deleter< T >() );

        detail::deconstruct_deleter< T > * pd = boost::get_deleter< detail::deconstruct_deleter< T > >( pt );

        void * pv = pd->address();

        new( pv ) T( a1, a2, a3, a4, a5, a6 );
        pd->set_initialized();

        boost::shared_ptr< T > retval( pt, static_cast< T* >( pv ) );
        boost::detail::sp_enable_shared_from_this(&retval, retval.get(), retval.get());
        return retval;
    }

    template< class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7 >
    static postconstructor_invoker<T> deconstruct( A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6, A7 const & a7 )
    {
        boost::shared_ptr< T > pt( static_cast< T* >( 0 ), detail::deconstruct_deleter< T >() );

        detail::deconstruct_deleter< T > * pd = boost::get_deleter< detail::deconstruct_deleter< T > >( pt );

        void * pv = pd->address();

        new( pv ) T( a1, a2, a3, a4, a5, a6, a7 );
        pd->set_initialized();

        boost::shared_ptr< T > retval( pt, static_cast< T* >( pv ) );
        boost::detail::sp_enable_shared_from_this(&retval, retval.get(), retval.get());
        return retval;
    }

    template< class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8 >
    static postconstructor_invoker<T> deconstruct( A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6, A7 const & a7, A8 const & a8 )
    {
        boost::shared_ptr< T > pt( static_cast< T* >( 0 ), detail::deconstruct_deleter< T >() );

        detail::deconstruct_deleter< T > * pd = boost::get_deleter< detail::deconstruct_deleter< T > >( pt );

        void * pv = pd->address();

        new( pv ) T( a1, a2, a3, a4, a5, a6, a7, a8 );
        pd->set_initialized();

        boost::shared_ptr< T > retval( pt, static_cast< T* >( pv ) );
        boost::detail::sp_enable_shared_from_this(&retval, retval.get(), retval.get());
        return retval;
    }

    template< class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9 >
    static postconstructor_invoker<T> deconstruct( A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6, A7 const & a7, A8 const & a8, A9 const & a9 )
    {
        boost::shared_ptr< T > pt( static_cast< T* >( 0 ), detail::deconstruct_deleter< T >() );

        detail::deconstruct_deleter< T > * pd = boost::get_deleter< detail::deconstruct_deleter< T > >( pt );

        void * pv = pd->address();

        new( pv ) T( a1, a2, a3, a4, a5, a6, a7, a8, a9 );
        pd->set_initialized();

        boost::shared_ptr< T > retval( pt, static_cast< T* >( pv ) );
        boost::detail::sp_enable_shared_from_this(&retval, retval.get(), retval.get());
        return retval;
    }

#endif
};

// Zero-argument versions
//
// Used even when variadic templates are available because of the new T() vs new T issue

template< class T > postconstructor_invoker<T> deconstruct()
{
    return deconstruct_access::deconstruct<T>();
}

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

// Variadic templates, rvalue reference

template< class T, class... Args > postconstructor_invoker< T > deconstruct( Args && ... args )
{
    return deconstruct_access::deconstruct<T>( std::forward<Args>( args )... );
}

#else

// C++03 version

template< class T, class A1 >
postconstructor_invoker<T> deconstruct( A1 const & a1 )
{
    return deconstruct_access::deconstruct<T>(a1);
}

template< class T, class A1, class A2 >
postconstructor_invoker<T> deconstruct( A1 const & a1, A2 const & a2 )
{
    return deconstruct_access::deconstruct<T>(a1,a2);
}

template< class T, class A1, class A2, class A3 >
postconstructor_invoker<T> deconstruct( A1 const & a1, A2 const & a2, A3 const & a3 )
{
    return deconstruct_access::deconstruct<T>(a1,a2,a3);
}

template< class T, class A1, class A2, class A3, class A4 >
postconstructor_invoker<T> deconstruct( A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4 )
{
    return deconstruct_access::deconstruct<T>(a1,a2,a3,a4);
}

template< class T, class A1, class A2, class A3, class A4, class A5 >
postconstructor_invoker<T> deconstruct( A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5 )
{
    return deconstruct_access::deconstruct<T>(a1,a2,a3,a4,a5);
}

template< class T, class A1, class A2, class A3, class A4, class A5, class A6 >
postconstructor_invoker<T> deconstruct( A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6 )
{
    return deconstruct_access::deconstruct<T>(a1,a2,a3,a4,a5,a6);
}

template< class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7 >
postconstructor_invoker<T> deconstruct( A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6, A7 const & a7 )
{
    return deconstruct_access::deconstruct<T>(a1,a2,a3,a4,a5,a6,a7);
}

template< class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8 >
postconstructor_invoker<T> deconstruct( A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6, A7 const & a7, A8 const & a8 )
{
    return deconstruct_access::deconstruct<T>(a1,a2,a3,a4,a5,a6,a7,a8);
}

template< class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9 >
postconstructor_invoker<T> deconstruct( A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6, A7 const & a7, A8 const & a8, A9 const & a9 )
{
    return deconstruct_access::deconstruct<T>(a1,a2,a3,a4,a5,a6,a7,a8,a9);
}

#endif

} // namespace signals2
} // namespace boost

#endif // #ifndef BOOST_SIGNALS2_DECONSTRUCT_HPP

// Copyright (C) 2003, 2008 Fernando Luis Cacciola Carballal.
// Copyright (C) 2014-2016 Andrzej Krzemienski.
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/optional for documentation.
//
// You are welcome to contact the maintainer at:
//  akrzemi1@gmail.com

#ifndef BOOST_OPTIONAL_DETAIL_OLD_OPTIONAL_IMPLEMENTATION_AJK_28JAN2015_HPP
#define BOOST_OPTIONAL_DETAIL_OLD_OPTIONAL_IMPLEMENTATION_AJK_28JAN2015_HPP

#include <boost/detail/reference_content.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/type_traits/conditional.hpp>

namespace boost {

namespace optional_detail {


template<class T>
struct types_when_isnt_ref
{
  typedef T const& reference_const_type ;
  typedef T &      reference_type ;
#ifndef  BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
  typedef T &&     rval_reference_type ;
  typedef T &&     reference_type_of_temporary_wrapper;
#ifdef BOOST_MOVE_OLD_RVALUE_REF_BINDING_RULES
  // GCC 4.4 has support for an early draft of rvalue references. The conforming version below
  // causes warnings about returning references to a temporary.
  static T&& move(T&& r) { return r; }
#else
  static rval_reference_type move(reference_type r) { return boost::move(r); }
#endif
#endif
  typedef T const* pointer_const_type ;
  typedef T *      pointer_type ;
  typedef T const& argument_type ;
} ;

template<class T>
struct types_when_is_ref
{
  typedef BOOST_DEDUCED_TYPENAME remove_reference<T>::type raw_type ;

  typedef raw_type&  reference_const_type ;
  typedef raw_type&  reference_type ;
#ifndef  BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
  typedef BOOST_DEDUCED_TYPENAME remove_const<raw_type>::type&& rval_reference_type ;
  typedef raw_type&  reference_type_of_temporary_wrapper;
  static reference_type move(reference_type r) { return r; }
#endif
  typedef raw_type*  pointer_const_type ;
  typedef raw_type*  pointer_type ;
  typedef raw_type&  argument_type ;
} ;

template <class To, class From>
void prevent_binding_rvalue_ref_to_optional_lvalue_ref()
{
#ifndef BOOST_OPTIONAL_CONFIG_ALLOW_BINDING_TO_RVALUES
  BOOST_STATIC_ASSERT_MSG(
    !boost::is_lvalue_reference<To>::value || !boost::is_rvalue_reference<From>::value, 
    "binding rvalue references to optional lvalue references is disallowed");
#endif    
}

struct optional_tag {} ;

template<class T>
class optional_base : public optional_tag
{
  private :

    typedef
#if !BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564))
    BOOST_DEDUCED_TYPENAME
#endif
    ::boost::detail::make_reference_content<T>::type internal_type ;

    typedef aligned_storage<internal_type> storage_type ;

    typedef types_when_isnt_ref<T> types_when_not_ref ;
    typedef types_when_is_ref<T>   types_when_ref   ;

    typedef optional_base<T> this_type ;

  protected :

    typedef T value_type ;

    typedef true_type  is_reference_tag ;
    typedef false_type is_not_reference_tag ;

    typedef BOOST_DEDUCED_TYPENAME is_reference<T>::type is_reference_predicate ;

  public:
    typedef BOOST_DEDUCED_TYPENAME conditional<is_reference_predicate::value,types_when_ref,types_when_not_ref>::type types ;

  protected:
    typedef BOOST_DEDUCED_TYPENAME types::reference_type       reference_type ;
    typedef BOOST_DEDUCED_TYPENAME types::reference_const_type reference_const_type ;
#ifndef  BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    typedef BOOST_DEDUCED_TYPENAME types::rval_reference_type  rval_reference_type ;
    typedef BOOST_DEDUCED_TYPENAME types::reference_type_of_temporary_wrapper reference_type_of_temporary_wrapper ;
#endif
    typedef BOOST_DEDUCED_TYPENAME types::pointer_type         pointer_type ;
    typedef BOOST_DEDUCED_TYPENAME types::pointer_const_type   pointer_const_type ;
    typedef BOOST_DEDUCED_TYPENAME types::argument_type        argument_type ;

    // Creates an optional<T> uninitialized.
    // No-throw
    optional_base()
      :
      m_initialized(false) {}

    // Creates an optional<T> uninitialized.
    // No-throw
    optional_base ( none_t )
      :
      m_initialized(false) {}

    // Creates an optional<T> initialized with 'val'.
    // Can throw if T::T(T const&) does
    optional_base ( argument_type val )
      :
      m_initialized(false)
    {
        construct(val);
    }

#ifndef  BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    // move-construct an optional<T> initialized from an rvalue-ref to 'val'.
    // Can throw if T::T(T&&) does
    optional_base ( rval_reference_type val )
      :
      m_initialized(false)
    {
      construct( boost::move(val) );
    }
#endif

    // Creates an optional<T> initialized with 'val' IFF cond is true, otherwise creates an uninitialized optional<T>.
    // Can throw if T::T(T const&) does
    optional_base ( bool cond, argument_type val )
      :
      m_initialized(false)
    {
      if ( cond )
        construct(val);
    }

    // Creates a deep copy of another optional<T>
    // Can throw if T::T(T const&) does
    optional_base ( optional_base const& rhs )
      :
      m_initialized(false)
    {
      if ( rhs.is_initialized() )
        construct(rhs.get_impl());
    }

#ifndef  BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    // Creates a deep move of another optional<T>
    // Can throw if T::T(T&&) does
    optional_base ( optional_base&& rhs )
      :
      m_initialized(false)
    {
      if ( rhs.is_initialized() )
        construct( boost::move(rhs.get_impl()) );
    }
#endif

#ifndef  BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES

    template<class Expr, class PtrExpr>
    explicit optional_base ( Expr&& expr, PtrExpr const* tag )
      :
      m_initialized(false)
    {
      construct(boost::forward<Expr>(expr),tag);
    }

#else
    // This is used for both converting and in-place constructions.
    // Derived classes use the 'tag' to select the appropriate
    // implementation (the correct 'construct()' overload)
    template<class Expr>
    explicit optional_base ( Expr const& expr, Expr const* tag )
      :
      m_initialized(false)
    {
      construct(expr,tag);
    }

#endif


    // No-throw (assuming T::~T() doesn't)
    ~optional_base() { destroy() ; }

    // Assigns from another optional<T> (deep-copies the rhs value)
    void assign ( optional_base const& rhs )
    {
      if (is_initialized())
      {
        if ( rhs.is_initialized() )
             assign_value(rhs.get_impl(), is_reference_predicate() );
        else destroy();
      }
      else
      {
        if ( rhs.is_initialized() )
          construct(rhs.get_impl());
      }
    }
    
#ifndef BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    // Assigns from another optional<T> (deep-moves the rhs value)
    void assign ( optional_base&& rhs )
    {
      if (is_initialized())
      {
        if ( rhs.is_initialized() )
             assign_value(boost::move(rhs.get_impl()), is_reference_predicate() );
        else destroy();
      }
      else
      {
        if ( rhs.is_initialized() )
          construct(boost::move(rhs.get_impl()));
      }
    }
#endif 

    // Assigns from another _convertible_ optional<U> (deep-copies the rhs value)
    template<class U>
    void assign ( optional<U> const& rhs )
    {
      if (is_initialized())
      {
        if ( rhs.is_initialized() )
#ifndef BOOST_OPTIONAL_CONFIG_RESTORE_ASSIGNMENT_OF_NONCONVERTIBLE_TYPES
          assign_value(rhs.get(), is_reference_predicate() );
#else
          assign_value(static_cast<value_type>(rhs.get()), is_reference_predicate() );
#endif
          
        else destroy();
      }
      else
      {
        if ( rhs.is_initialized() )
#ifndef BOOST_OPTIONAL_CONFIG_RESTORE_ASSIGNMENT_OF_NONCONVERTIBLE_TYPES
          construct(rhs.get());
#else
          construct(static_cast<value_type>(rhs.get()));
#endif
      }
    }

#ifndef BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    // move-assigns from another _convertible_ optional<U> (deep-moves from the rhs value)
    template<class U>
    void assign ( optional<U>&& rhs )
    {
      typedef BOOST_DEDUCED_TYPENAME optional<U>::rval_reference_type ref_type;
      if (is_initialized())
      {
        if ( rhs.is_initialized() )
             assign_value(static_cast<ref_type>(rhs.get()), is_reference_predicate() );
        else destroy();
      }
      else
      {
        if ( rhs.is_initialized() )
          construct(static_cast<ref_type>(rhs.get()));
      }
    }
#endif
    
    // Assigns from a T (deep-copies the rhs value)
    void assign ( argument_type val )
    {
      if (is_initialized())
           assign_value(val, is_reference_predicate() );
      else construct(val);
    }
    
#ifndef BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    // Assigns from a T (deep-moves the rhs value)
    void assign ( rval_reference_type val )
    {
      if (is_initialized())
           assign_value( boost::move(val), is_reference_predicate() );
      else construct( boost::move(val) );
    }
#endif

    // Assigns from "none", destroying the current value, if any, leaving this UNINITIALIZED
    // No-throw (assuming T::~T() doesn't)
    void assign ( none_t ) BOOST_NOEXCEPT { destroy(); }

#ifndef BOOST_OPTIONAL_NO_INPLACE_FACTORY_SUPPORT

#ifndef BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    template<class Expr, class ExprPtr>
    void assign_expr ( Expr&& expr, ExprPtr const* tag )
    {
      if (is_initialized())
        assign_expr_to_initialized(boost::forward<Expr>(expr),tag);
      else construct(boost::forward<Expr>(expr),tag);
    }
#else
    template<class Expr>
    void assign_expr ( Expr const& expr, Expr const* tag )
    {
      if (is_initialized())
        assign_expr_to_initialized(expr,tag);
      else construct(expr,tag);
    }
#endif

#endif

  public :

    // Destroys the current value, if any, leaving this UNINITIALIZED
    // No-throw (assuming T::~T() doesn't)
    void reset() BOOST_NOEXCEPT { destroy(); }

    // **DEPPRECATED** Replaces the current value -if any- with 'val'
    void reset ( argument_type val ) { assign(val); }

    // Returns a pointer to the value if this is initialized, otherwise,
    // returns NULL.
    // No-throw
    pointer_const_type get_ptr() const { return m_initialized ? get_ptr_impl() : 0 ; }
    pointer_type       get_ptr()       { return m_initialized ? get_ptr_impl() : 0 ; }

    bool is_initialized() const { return m_initialized ; }

  protected :

    void construct ( argument_type val )
     {
       ::new (m_storage.address()) internal_type(val) ;
       m_initialized = true ;
     }
     
#ifndef  BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    void construct ( rval_reference_type val )
     {
       ::new (m_storage.address()) internal_type( types::move(val) ) ;
       m_initialized = true ;
     }
#endif


#if (!defined BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES) && (!defined BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    // Constructs in-place
    // upon exception *this is always uninitialized
    template<class... Args>
    void emplace_assign ( Args&&... args )
     {
       destroy();
       ::new (m_storage.address()) internal_type( boost::forward<Args>(args)... );
       m_initialized = true ;
     }
#elif (!defined BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES)
    template<class Arg>
    void emplace_assign ( Arg&& arg )
     {
       destroy();
       ::new (m_storage.address()) internal_type( boost::forward<Arg>(arg) );
       m_initialized = true ;
     }
     
    void emplace_assign ()
     {
       destroy();
       ::new (m_storage.address()) internal_type();
       m_initialized = true ;
     }
#else
    template<class Arg>
    void emplace_assign ( const Arg& arg )
     {
       destroy();
       ::new (m_storage.address()) internal_type( arg );
       m_initialized = true ;
     }
     
    template<class Arg>
    void emplace_assign ( Arg& arg )
     {
       destroy();
       ::new (m_storage.address()) internal_type( arg );
       m_initialized = true ;
     }
     
    void emplace_assign ()
     {
       destroy();
       ::new (m_storage.address()) internal_type();
       m_initialized = true ;
     }
#endif

#ifndef BOOST_OPTIONAL_NO_INPLACE_FACTORY_SUPPORT

#ifndef BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    // Constructs in-place using the given factory
    template<class Expr>
    void construct ( Expr&& factory, in_place_factory_base const* )
     {
       BOOST_STATIC_ASSERT ( !is_reference_predicate::value ) ;
       boost_optional_detail::construct<value_type>(factory, m_storage.address());
       m_initialized = true ;
     }

    // Constructs in-place using the given typed factory
    template<class Expr>
    void construct ( Expr&& factory, typed_in_place_factory_base const* )
     {
       BOOST_STATIC_ASSERT ( !is_reference_predicate::value ) ;
       factory.apply(m_storage.address()) ;
       m_initialized = true ;
     }

    template<class Expr>
    void assign_expr_to_initialized ( Expr&& factory, in_place_factory_base const* tag )
     {
       destroy();
       construct(factory,tag);
     }

    // Constructs in-place using the given typed factory
    template<class Expr>
    void assign_expr_to_initialized ( Expr&& factory, typed_in_place_factory_base const* tag )
     {
       destroy();
       construct(factory,tag);
     }

#else
    // Constructs in-place using the given factory
    template<class Expr>
    void construct ( Expr const& factory, in_place_factory_base const* )
     {
       BOOST_STATIC_ASSERT ( !is_reference_predicate::value ) ;
       boost_optional_detail::construct<value_type>(factory, m_storage.address());
       m_initialized = true ;
     }

    // Constructs in-place using the given typed factory
    template<class Expr>
    void construct ( Expr const& factory, typed_in_place_factory_base const* )
     {
       BOOST_STATIC_ASSERT ( !is_reference_predicate::value ) ;
       factory.apply(m_storage.address()) ;
       m_initialized = true ;
     }

    template<class Expr>
    void assign_expr_to_initialized ( Expr const& factory, in_place_factory_base const* tag )
     {
       destroy();
       construct(factory,tag);
     }

    // Constructs in-place using the given typed factory
    template<class Expr>
    void assign_expr_to_initialized ( Expr const& factory, typed_in_place_factory_base const* tag )
     {
       destroy();
       construct(factory,tag);
     }
#endif

#endif

#ifndef BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    // Constructs using any expression implicitly convertible to the single argument
    // of a one-argument T constructor.
    // Converting constructions of optional<T> from optional<U> uses this function with
    // 'Expr' being of type 'U' and relying on a converting constructor of T from U.
    template<class Expr>
    void construct ( Expr&& expr, void const* )
    {
      new (m_storage.address()) internal_type(boost::forward<Expr>(expr)) ;
      m_initialized = true ;
    }

    // Assigns using a form any expression implicitly convertible to the single argument
    // of a T's assignment operator.
    // Converting assignments of optional<T> from optional<U> uses this function with
    // 'Expr' being of type 'U' and relying on a converting assignment of T from U.
    template<class Expr>
    void assign_expr_to_initialized ( Expr&& expr, void const* )
    {
      assign_value(boost::forward<Expr>(expr), is_reference_predicate());
    }
#else
    // Constructs using any expression implicitly convertible to the single argument
    // of a one-argument T constructor.
    // Converting constructions of optional<T> from optional<U> uses this function with
    // 'Expr' being of type 'U' and relying on a converting constructor of T from U.
    template<class Expr>
    void construct ( Expr const& expr, void const* )
     {
       new (m_storage.address()) internal_type(expr) ;
       m_initialized = true ;
     }

    // Assigns using a form any expression implicitly convertible to the single argument
    // of a T's assignment operator.
    // Converting assignments of optional<T> from optional<U> uses this function with
    // 'Expr' being of type 'U' and relying on a converting assignment of T from U.
    template<class Expr>
    void assign_expr_to_initialized ( Expr const& expr, void const* )
     {
       assign_value(expr, is_reference_predicate());
     }

#endif

#ifdef BOOST_OPTIONAL_WEAK_OVERLOAD_RESOLUTION
    // BCB5.64 (and probably lower versions) workaround.
    //   The in-place factories are supported by means of catch-all constructors
    //   and assignment operators (the functions are parameterized in terms of
    //   an arbitrary 'Expr' type)
    //   This compiler incorrectly resolves the overload set and sinks optional<T> and optional<U>
    //   to the 'Expr'-taking functions even though explicit overloads are present for them.
    //   Thus, the following overload is needed to properly handle the case when the 'lhs'
    //   is another optional.
    //
    // For VC<=70 compilers this workaround dosen't work becasue the comnpiler issues and error
    // instead of choosing the wrong overload
    //
#ifndef  BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    // Notice that 'Expr' will be optional<T> or optional<U> (but not optional_base<..>)
    template<class Expr>
    void construct ( Expr&& expr, optional_tag const* )
     {
       if ( expr.is_initialized() )
       {
         // An exception can be thrown here.
         // It it happens, THIS will be left uninitialized.
         new (m_storage.address()) internal_type(types::move(expr.get())) ;
         m_initialized = true ;
       }
     }
#else
    // Notice that 'Expr' will be optional<T> or optional<U> (but not optional_base<..>)
    template<class Expr>
    void construct ( Expr const& expr, optional_tag const* )
     {
       if ( expr.is_initialized() )
       {
         // An exception can be thrown here.
         // It it happens, THIS will be left uninitialized.
         new (m_storage.address()) internal_type(expr.get()) ;
         m_initialized = true ;
       }
     }
#endif
#endif // defined BOOST_OPTIONAL_WEAK_OVERLOAD_RESOLUTION

    void assign_value ( argument_type val, is_not_reference_tag ) { get_impl() = val; }
    void assign_value ( argument_type val, is_reference_tag     ) { construct(val); }
#ifndef  BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    void assign_value ( rval_reference_type val, is_not_reference_tag ) { get_impl() = static_cast<rval_reference_type>(val); }
    void assign_value ( rval_reference_type val, is_reference_tag     ) { construct( static_cast<rval_reference_type>(val) ); }
#endif

    void destroy()
    {
      if ( m_initialized )
        destroy_impl(is_reference_predicate()) ;
    }

    reference_const_type get_impl() const { return dereference(get_object(), is_reference_predicate() ) ; }
    reference_type       get_impl()       { return dereference(get_object(), is_reference_predicate() ) ; }

    pointer_const_type get_ptr_impl() const { return cast_ptr(get_object(), is_reference_predicate() ) ; }
    pointer_type       get_ptr_impl()       { return cast_ptr(get_object(), is_reference_predicate() ) ; }

  private :

    // internal_type can be either T or reference_content<T>
#if defined(BOOST_OPTIONAL_DETAIL_USE_ATTRIBUTE_MAY_ALIAS)
    // This workaround is supposed to silence GCC warnings about broken strict aliasing rules
    internal_type const* get_object() const
    {
        union { void const* ap_pvoid; internal_type const* as_ptype; } caster = { m_storage.address() };
        return caster.as_ptype;
    }
    internal_type *      get_object()
    {
        union { void* ap_pvoid; internal_type* as_ptype; } caster = { m_storage.address() };
        return caster.as_ptype;
    }
#else
    internal_type const* get_object() const { return static_cast<internal_type const*>(m_storage.address()); }
    internal_type *      get_object()       { return static_cast<internal_type *>     (m_storage.address()); }
#endif

    // reference_content<T> lacks an implicit conversion to T&, so the following is needed to obtain a proper reference.
    reference_const_type dereference( internal_type const* p, is_not_reference_tag ) const { return *p ; }
    reference_type       dereference( internal_type*       p, is_not_reference_tag )       { return *p ; }
    reference_const_type dereference( internal_type const* p, is_reference_tag     ) const { return p->get() ; }
    reference_type       dereference( internal_type*       p, is_reference_tag     )       { return p->get() ; }

#if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x581))
    void destroy_impl ( is_not_reference_tag ) { get_ptr_impl()->internal_type::~internal_type() ; m_initialized = false ; }
#else
    void destroy_impl ( is_not_reference_tag ) { get_ptr_impl()->~T() ; m_initialized = false ; }
#endif

    void destroy_impl ( is_reference_tag     ) { m_initialized = false ; }

    // If T is of reference type, trying to get a pointer to the held value must result in a compile-time error.
    // Decent compilers should disallow conversions from reference_content<T>* to T*, but just in case,
    // the following olverloads are used to filter out the case and guarantee an error in case of T being a reference.
    pointer_const_type cast_ptr( internal_type const* p, is_not_reference_tag ) const { return p ; }
    pointer_type       cast_ptr( internal_type *      p, is_not_reference_tag )       { return p ; }
    pointer_const_type cast_ptr( internal_type const* p, is_reference_tag     ) const { return &p->get() ; }
    pointer_type       cast_ptr( internal_type *      p, is_reference_tag     )       { return &p->get() ; }

    bool m_initialized ;
    storage_type m_storage ;
} ;

} // namespace optional_detail

template<class T>
class optional : public optional_detail::optional_base<T>
{
    typedef optional_detail::optional_base<T> base ;

  public :

    typedef optional<T> this_type ;

    typedef BOOST_DEDUCED_TYPENAME base::value_type           value_type ;
    typedef BOOST_DEDUCED_TYPENAME base::reference_type       reference_type ;
    typedef BOOST_DEDUCED_TYPENAME base::reference_const_type reference_const_type ;
#ifndef  BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    typedef BOOST_DEDUCED_TYPENAME base::rval_reference_type  rval_reference_type ;
    typedef BOOST_DEDUCED_TYPENAME base::reference_type_of_temporary_wrapper reference_type_of_temporary_wrapper ;
#endif
    typedef BOOST_DEDUCED_TYPENAME base::pointer_type         pointer_type ;
    typedef BOOST_DEDUCED_TYPENAME base::pointer_const_type   pointer_const_type ;
    typedef BOOST_DEDUCED_TYPENAME base::argument_type        argument_type ;

    // Creates an optional<T> uninitialized.
    // No-throw
    optional() BOOST_NOEXCEPT : base() {}

    // Creates an optional<T> uninitialized.
    // No-throw
    optional( none_t none_ ) BOOST_NOEXCEPT : base(none_) {}

    // Creates an optional<T> initialized with 'val'.
    // Can throw if T::T(T const&) does
    optional ( argument_type val ) : base(val) {}

#ifndef  BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    // Creates an optional<T> initialized with 'move(val)'.
    // Can throw if T::T(T &&) does
    optional ( rval_reference_type val ) : base( boost::forward<T>(val) ) 
      {optional_detail::prevent_binding_rvalue_ref_to_optional_lvalue_ref<T, rval_reference_type>();}
#endif

    // Creates an optional<T> initialized with 'val' IFF cond is true, otherwise creates an uninitialized optional.
    // Can throw if T::T(T const&) does
    optional ( bool cond, argument_type val ) : base(cond,val) {}

    // NOTE: MSVC needs templated versions first

    // Creates a deep copy of another convertible optional<U>
    // Requires a valid conversion from U to T.
    // Can throw if T::T(U const&) does
    template<class U>
    explicit optional ( optional<U> const& rhs )
      :
      base()
    {
      if ( rhs.is_initialized() )
        this->construct(rhs.get());
    }
    
#ifndef  BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    // Creates a deep move of another convertible optional<U>
    // Requires a valid conversion from U to T.
    // Can throw if T::T(U&&) does
    template<class U>
    explicit optional ( optional<U> && rhs )
      :
      base()
    {
      if ( rhs.is_initialized() )
        this->construct( boost::move(rhs.get()) );
    }
#endif

#ifndef BOOST_OPTIONAL_NO_INPLACE_FACTORY_SUPPORT
    // Creates an optional<T> with an expression which can be either
    //  (a) An instance of InPlaceFactory (i.e. in_place(a,b,...,n);
    //  (b) An instance of TypedInPlaceFactory ( i.e. in_place<T>(a,b,...,n);
    //  (c) Any expression implicitly convertible to the single type
    //      of a one-argument T's constructor.
    //  (d*) Weak compilers (BCB) might also resolved Expr as optional<T> and optional<U>
    //       even though explicit overloads are present for these.
    // Depending on the above some T ctor is called.
    // Can throw if the resolved T ctor throws.
#ifndef BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES


  template<class Expr>
  explicit optional ( Expr&& expr, 
                      BOOST_DEDUCED_TYPENAME boost::disable_if_c<
                        (boost::is_base_of<optional_detail::optional_tag, BOOST_DEDUCED_TYPENAME boost::decay<Expr>::type>::value) || 
                        boost::is_same<BOOST_DEDUCED_TYPENAME boost::decay<Expr>::type, none_t>::value, bool >::type = true 
  ) 
    : base(boost::forward<Expr>(expr),boost::addressof(expr)) 
    {optional_detail::prevent_binding_rvalue_ref_to_optional_lvalue_ref<T, Expr&&>();}

#else
    template<class Expr>
    explicit optional ( Expr const& expr ) : base(expr,boost::addressof(expr)) {}
#endif // !defined BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
#endif // !defined BOOST_OPTIONAL_NO_INPLACE_FACTORY_SUPPORT

    // Creates a deep copy of another optional<T>
    // Can throw if T::T(T const&) does
    optional ( optional const& rhs ) : base( static_cast<base const&>(rhs) ) {}

#ifndef  BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    // Creates a deep move of another optional<T>
    // Can throw if T::T(T&&) does
    optional ( optional && rhs )
      BOOST_NOEXCEPT_IF(::boost::is_nothrow_move_constructible<T>::value)
      : base( boost::move(rhs) )
    {}

#endif
   // No-throw (assuming T::~T() doesn't)
    ~optional() {}

#if !defined(BOOST_OPTIONAL_NO_INPLACE_FACTORY_SUPPORT) && !defined(BOOST_OPTIONAL_WEAK_OVERLOAD_RESOLUTION)
    // Assigns from an expression. See corresponding constructor.
    // Basic Guarantee: If the resolved T ctor throws, this is left UNINITIALIZED
#ifndef  BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES

    template<class Expr>
    BOOST_DEDUCED_TYPENAME boost::disable_if_c<
      boost::is_base_of<optional_detail::optional_tag, BOOST_DEDUCED_TYPENAME boost::decay<Expr>::type>::value || 
        boost::is_same<BOOST_DEDUCED_TYPENAME boost::decay<Expr>::type, none_t>::value,
      optional&
    >::type 
    operator= ( Expr&& expr )
      {
        optional_detail::prevent_binding_rvalue_ref_to_optional_lvalue_ref<T, Expr&&>();
        this->assign_expr(boost::forward<Expr>(expr),boost::addressof(expr));
        return *this ;
      }

#else
    template<class Expr>
    optional& operator= ( Expr const& expr )
      {
        this->assign_expr(expr,boost::addressof(expr));
        return *this ;
      }
#endif // !defined  BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
#endif // !defined(BOOST_OPTIONAL_NO_INPLACE_FACTORY_SUPPORT) && !defined(BOOST_OPTIONAL_WEAK_OVERLOAD_RESOLUTION)

    // Copy-assigns from another convertible optional<U> (converts && deep-copies the rhs value)
    // Requires a valid conversion from U to T.
    // Basic Guarantee: If T::T( U const& ) throws, this is left UNINITIALIZED
    template<class U>
    optional& operator= ( optional<U> const& rhs )
      {
        this->assign(rhs);
        return *this ;
      }
      
#ifndef  BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    // Move-assigns from another convertible optional<U> (converts && deep-moves the rhs value)
    // Requires a valid conversion from U to T.
    // Basic Guarantee: If T::T( U && ) throws, this is left UNINITIALIZED
    template<class U>
    optional& operator= ( optional<U> && rhs )
      {
        this->assign(boost::move(rhs));
        return *this ;
      }
#endif

    // Assigns from another optional<T> (deep-copies the rhs value)
    // Basic Guarantee: If T::T( T const& ) throws, this is left UNINITIALIZED
    //  (NOTE: On BCB, this operator is not actually called and left is left UNMODIFIED in case of a throw)
    optional& operator= ( optional const& rhs )
      {
        this->assign( static_cast<base const&>(rhs) ) ;
        return *this ;
      }

#ifndef  BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    // Assigns from another optional<T> (deep-moves the rhs value)
    optional& operator= ( optional && rhs ) 
      BOOST_NOEXCEPT_IF(::boost::is_nothrow_move_constructible<T>::value && ::boost::is_nothrow_move_assignable<T>::value)
      {
        this->assign( static_cast<base &&>(rhs) ) ;
        return *this ;
      }
#endif

    // Assigns from a T (deep-copies the rhs value)
    // Basic Guarantee: If T::( T const& ) throws, this is left UNINITIALIZED
    optional& operator= ( argument_type val )
      {
        this->assign( val ) ;
        return *this ;
      }

#ifndef BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    // Assigns from a T (deep-moves the rhs value)
    optional& operator= ( rval_reference_type val )
      {
        optional_detail::prevent_binding_rvalue_ref_to_optional_lvalue_ref<T, rval_reference_type>();
        this->assign( boost::move(val) ) ;
        return *this ;
      }
#endif

    // Assigns from a "none"
    // Which destroys the current value, if any, leaving this UNINITIALIZED
    // No-throw (assuming T::~T() doesn't)
    optional& operator= ( none_t none_ ) BOOST_NOEXCEPT
      {
        this->assign( none_ ) ;
        return *this ;
      }
      
#if (!defined BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES) && (!defined BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    // Constructs in-place
    // upon exception *this is always uninitialized
    template<class... Args>
    void emplace ( Args&&... args )
     {
       this->emplace_assign( boost::forward<Args>(args)... );
     }
#elif (!defined BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES)
    template<class Arg>
    void emplace ( Arg&& arg )
     {
       this->emplace_assign( boost::forward<Arg>(arg) );
     }
     
    void emplace ()
     {
       this->emplace_assign();
     }
#else
    template<class Arg>
    void emplace ( const Arg& arg )
     {
       this->emplace_assign( arg );
     }
     
    template<class Arg>
    void emplace ( Arg& arg )
     {
       this->emplace_assign( arg );
     }
     
    void emplace ()
     {
       this->emplace_assign();
     }
#endif

    void swap( optional & arg )
      BOOST_NOEXCEPT_IF(::boost::is_nothrow_move_constructible<T>::value && ::boost::is_nothrow_move_assignable<T>::value)
      {
        // allow for Koenig lookup
        boost::swap(*this, arg);
      }


    // Returns a reference to the value if this is initialized, otherwise,
    // the behaviour is UNDEFINED
    // No-throw
    reference_const_type get() const { BOOST_ASSERT(this->is_initialized()) ; return this->get_impl(); }
    reference_type       get()       { BOOST_ASSERT(this->is_initialized()) ; return this->get_impl(); }

    // Returns a copy of the value if this is initialized, 'v' otherwise
    reference_const_type get_value_or ( reference_const_type v ) const { return this->is_initialized() ? get() : v ; }
    reference_type       get_value_or ( reference_type       v )       { return this->is_initialized() ? get() : v ; }

    // Returns a pointer to the value if this is initialized, otherwise,
    // the behaviour is UNDEFINED
    // No-throw
    pointer_const_type operator->() const { BOOST_ASSERT(this->is_initialized()) ; return this->get_ptr_impl() ; }
    pointer_type       operator->()       { BOOST_ASSERT(this->is_initialized()) ; return this->get_ptr_impl() ; }

    // Returns a reference to the value if this is initialized, otherwise,
    // the behaviour is UNDEFINED
    // No-throw
#if (!defined BOOST_NO_CXX11_REF_QUALIFIERS) && (!defined BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES) 
    reference_const_type operator *() const& { return this->get() ; }
    reference_type       operator *() &      { return this->get() ; }
    reference_type_of_temporary_wrapper operator *() && { return base::types::move(this->get()) ; }
#else
    reference_const_type operator *() const { return this->get() ; }
    reference_type       operator *()       { return this->get() ; }
#endif // !defined BOOST_NO_CXX11_REF_QUALIFIERS

#if (!defined BOOST_NO_CXX11_REF_QUALIFIERS) && (!defined BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES) 
    reference_const_type value() const&
      { 
        if (this->is_initialized())
          return this->get() ;
        else
          throw_exception(bad_optional_access());
      }
      
    reference_type value() &
      { 
        if (this->is_initialized())
          return this->get() ;
        else
          throw_exception(bad_optional_access());
      }
      
    reference_type_of_temporary_wrapper value() &&
      { 
        if (this->is_initialized())
          return base::types::move(this->get()) ;
        else
          throw_exception(bad_optional_access());
      }

#else 
    reference_const_type value() const
      { 
        if (this->is_initialized())
          return this->get() ;
        else
          throw_exception(bad_optional_access());
      }
      
    reference_type value()
      { 
        if (this->is_initialized())
          return this->get() ;
        else
          throw_exception(bad_optional_access());
      }
#endif


#ifndef BOOST_NO_CXX11_REF_QUALIFIERS
    template <class U>
    value_type value_or ( U&& v ) const&
      { 
        if (this->is_initialized())
          return get();
        else
          return boost::forward<U>(v);
      }
    
    template <class U>
    value_type value_or ( U&& v ) && 
      { 
        if (this->is_initialized())
          return base::types::move(get());
        else
          return boost::forward<U>(v);
      }
#elif !defined BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    template <class U>
    value_type value_or ( U&& v ) const 
      {
        if (this->is_initialized())
          return get();
        else
          return boost::forward<U>(v);
      }
#else
    template <class U>
    value_type value_or ( U const& v ) const 
      { 
        if (this->is_initialized())
          return get();
        else
          return v;
      }
      
    template <class U>
    value_type value_or ( U& v ) const 
      { 
        if (this->is_initialized())
          return get();
        else
          return v;
      }
#endif


#ifndef BOOST_NO_CXX11_REF_QUALIFIERS
    template <typename F>
    value_type value_or_eval ( F f ) const&
      {
        if (this->is_initialized())
          return get();
        else
          return f();
      }
      
    template <typename F>
    value_type value_or_eval ( F f ) &&
      {
        if (this->is_initialized())
          return base::types::move(get());
        else
          return f();
      }
#else
    template <typename F>
    value_type value_or_eval ( F f ) const
      {
        if (this->is_initialized())
          return get();
        else
          return f();
      }
#endif
      
    bool operator!() const BOOST_NOEXCEPT { return !this->is_initialized() ; }
    
    BOOST_EXPLICIT_OPERATOR_BOOL_NOEXCEPT()
} ;

} // namespace boost


#endif // header guard

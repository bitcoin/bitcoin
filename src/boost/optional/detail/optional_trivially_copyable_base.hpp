// trivilally-copyable version of the storage

template<class T>
class tc_optional_base : public optional_tag
{
  private :

    typedef tc_optional_base<T> this_type ;

  protected :

    typedef T value_type ;

  protected:
    typedef T &       reference_type ;
    typedef T const&  reference_const_type ;
#ifndef  BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    typedef T &&  rval_reference_type ;
    typedef T &&  reference_type_of_temporary_wrapper ;
#endif
    typedef T *         pointer_type ;
    typedef T const*    pointer_const_type ;
    typedef T const&    argument_type ;

    tc_optional_base()
      :
      m_initialized(false) {}

    tc_optional_base ( none_t )
      :
      m_initialized(false) {}

    tc_optional_base ( init_value_tag, argument_type val )
      :
      m_initialized(true), m_storage(val) {}

    tc_optional_base ( bool cond, argument_type val )
      :
      m_initialized(cond), m_storage(val) {}

    // tc_optional_base ( tc_optional_base const& ) = default;


#ifndef  BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES

    template<class Expr, class PtrExpr>
    explicit tc_optional_base ( Expr&& expr, PtrExpr const* tag )
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
    explicit tc_optional_base ( Expr const& expr, Expr const* tag )
      :
      m_initialized(false)
    {
      construct(expr,tag);
    }

#endif

    // tc_optional_base& operator= ( tc_optional_base const& ) = default;
    // ~tc_optional_base() = default;

    // Assigns from another optional<T> (deep-copies the rhs value)
    void assign ( tc_optional_base const& rhs ) 
    {
      *this = rhs;
    }

    // Assigns from another _convertible_ optional<U> (deep-copies the rhs value)
    template<class U>
    void assign ( optional<U> const& rhs )
    {
      if ( rhs.is_initialized() )
#ifndef BOOST_OPTIONAL_CONFIG_RESTORE_ASSIGNMENT_OF_NONCONVERTIBLE_TYPES
        m_storage = rhs.get();
#else
        m_storage = static_cast<value_type>(rhs.get());
#endif
          
      m_initialized = rhs.is_initialized();
    }

#ifndef BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    // move-assigns from another _convertible_ optional<U> (deep-moves from the rhs value)
    template<class U>
    void assign ( optional<U>&& rhs )
    {
      typedef BOOST_DEDUCED_TYPENAME optional<U>::rval_reference_type ref_type;
      if ( rhs.is_initialized() )
        m_storage = static_cast<ref_type>(rhs.get());
      m_initialized = rhs.is_initialized();
    }
#endif
    
    void assign ( argument_type val )
    {
      construct(val);
    }

    void assign ( none_t ) { destroy(); }

#ifndef BOOST_OPTIONAL_NO_INPLACE_FACTORY_SUPPORT

#ifndef BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    template<class Expr, class ExprPtr>
    void assign_expr ( Expr&& expr, ExprPtr const* tag )
    {
       construct(boost::forward<Expr>(expr),tag);
    }
#else
    template<class Expr>
    void assign_expr ( Expr const& expr, Expr const* tag )
    {
      construct(expr,tag);
    }
#endif

#endif

  public :

    // Destroys the current value, if any, leaving this UNINITIALIZED
    // No-throw (assuming T::~T() doesn't)
    void reset() BOOST_NOEXCEPT { destroy(); }

    // **DEPPRECATED** Replaces the current value -if any- with 'val'
    void reset ( argument_type val ) BOOST_NOEXCEPT { assign(val); }

    // Returns a pointer to the value if this is initialized, otherwise,
    // returns NULL.
    // No-throw
    pointer_const_type get_ptr() const { return m_initialized ? get_ptr_impl() : 0 ; }
    pointer_type       get_ptr()       { return m_initialized ? get_ptr_impl() : 0 ; }

    bool is_initialized() const { return m_initialized ; }

  protected :

    void construct ( argument_type val )
     {
       m_storage = val ;
       m_initialized = true ;
     }


#if (!defined BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES) && (!defined BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    // Constructs in-place
    // upon exception *this is always uninitialized
    template<class... Args>
    void construct ( in_place_init_t, Args&&... args )
    {
      m_storage = value_type( boost::forward<Args>(args)... ) ;
      m_initialized = true ;
    }

    template<class... Args>
    void emplace_assign ( Args&&... args )
    {
      construct(in_place_init, boost::forward<Args>(args)...);
    }
     
    template<class... Args>
    explicit tc_optional_base ( in_place_init_t, Args&&... args )
      :
      m_initialized(false)
    {
      construct(in_place_init, boost::forward<Args>(args)...);
    }
    
    template<class... Args>
    explicit tc_optional_base ( in_place_init_if_t, bool cond, Args&&... args )
      :
      m_initialized(false)
    {
      if ( cond )
        construct(in_place_init, boost::forward<Args>(args)...);
    }
#elif (!defined BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES)
    template<class Arg>
    void construct ( in_place_init_t, Arg&& arg )
     {
       m_storage = value_type( boost::forward<Arg>(arg) );
       m_initialized = true ;
     }
     
    void construct ( in_place_init_t )
     {
       m_storage = value_type();
       m_initialized = true ;
     }
     
    template<class Arg>
    void emplace_assign ( Arg&& arg )
     {
       construct(in_place_init, boost::forward<Arg>(arg)) ;
     }
     
    void emplace_assign ()
     {
       construct(in_place_init) ;
     }
     
    template<class Arg>
    explicit tc_optional_base ( in_place_init_t, Arg&& arg )
      :
      m_initialized(false)
    {
      construct(in_place_init, boost::forward<Arg>(arg));
    }
    
    explicit tc_optional_base ( in_place_init_t )
      :
      m_initialized(false), m_storage() {}
    
    template<class Arg>
    explicit tc_optional_base ( in_place_init_if_t, bool cond, Arg&& arg )
      :
      m_initialized(false)
    {
      if ( cond )
        construct(in_place_init, boost::forward<Arg>(arg));
    }
    
    explicit tc_optional_base ( in_place_init_if_t, bool cond )
      :
      m_initialized(false)
    {
      if ( cond )
        construct(in_place_init);
    }

#else
     
    template<class Arg>
    void construct ( in_place_init_t, const Arg& arg )
     {
       m_storage = value_type( arg );
       m_initialized = true ;
     }
     
    template<class Arg>
    void construct ( in_place_init_t, Arg& arg )
     {
       m_storage = value_type( arg );
       m_initialized = true ;
     }
     
    void construct ( in_place_init_t )
     {
       m_storage = value_type();
       m_initialized = true ;
     }

    template<class Arg>
    void emplace_assign ( const Arg& arg )
    {
      construct(in_place_init, arg);
    }
     
    template<class Arg>
    void emplace_assign ( Arg& arg )
    {
      construct(in_place_init, arg);
    }
     
    void emplace_assign ()
    {
      construct(in_place_init);
    }
    
    template<class Arg>
    explicit tc_optional_base ( in_place_init_t, const Arg& arg )
      : m_initialized(false)
    {
      construct(in_place_init, arg);
    }

    template<class Arg>
    explicit tc_optional_base ( in_place_init_t, Arg& arg )
      : m_initialized(false)
    {
      construct(in_place_init, arg);
    }
    
    explicit tc_optional_base ( in_place_init_t )
      : m_initialized(false)
    {
      construct(in_place_init);
    }
    
    template<class Arg>
    explicit tc_optional_base ( in_place_init_if_t, bool cond, const Arg& arg )
      : m_initialized(false)
    {
      if ( cond )
        construct(in_place_init, arg);
    }
    
    template<class Arg>
    explicit tc_optional_base ( in_place_init_if_t, bool cond, Arg& arg )
      : m_initialized(false)
    {
      if ( cond )
        construct(in_place_init, arg);
    } 
    
    explicit tc_optional_base ( in_place_init_if_t, bool cond )
      : m_initialized(false)
    {
      if ( cond )
        construct(in_place_init);
    }
#endif

#ifndef BOOST_OPTIONAL_NO_INPLACE_FACTORY_SUPPORT

#ifndef BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    // Constructs in-place using the given factory
    template<class Expr>
    void construct ( Expr&& factory, in_place_factory_base const* )
     {
       boost_optional_detail::construct<value_type>(factory, boost::addressof(m_storage));
       m_initialized = true ;
     }

    // Constructs in-place using the given typed factory
    template<class Expr>
    void construct ( Expr&& factory, typed_in_place_factory_base const* )
     {
       factory.apply(boost::addressof(m_storage)) ;
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
       boost_optional_detail::construct<value_type>(factory, boost::addressof(m_storage));
       m_initialized = true ;
     }

    // Constructs in-place using the given typed factory
    template<class Expr>
    void construct ( Expr const& factory, typed_in_place_factory_base const* )
     {
       factory.apply(boost::addressof(m_storage)) ;
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
      m_storage = value_type(boost::forward<Expr>(expr)) ;
      m_initialized = true ;
    }

    // Assigns using a form any expression implicitly convertible to the single argument
    // of a T's assignment operator.
    // Converting assignments of optional<T> from optional<U> uses this function with
    // 'Expr' being of type 'U' and relying on a converting assignment of T from U.
    template<class Expr>
    void assign_expr_to_initialized ( Expr&& expr, void const* )
    {
      assign_value( boost::forward<Expr>(expr) );
    }
#else
    // Constructs using any expression implicitly convertible to the single argument
    // of a one-argument T constructor.
    // Converting constructions of optional<T> from optional<U> uses this function with
    // 'Expr' being of type 'U' and relying on a converting constructor of T from U.
    template<class Expr>
    void construct ( Expr const& expr, void const* )
     {
       m_storage = value_type(expr) ;
       m_initialized = true ;
     }

    // Assigns using a form any expression implicitly convertible to the single argument
    // of a T's assignment operator.
    // Converting assignments of optional<T> from optional<U> uses this function with
    // 'Expr' being of type 'U' and relying on a converting assignment of T from U.
    template<class Expr>
    void assign_expr_to_initialized ( Expr const& expr, void const* )
     {
       assign_value(expr);
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
    // Notice that 'Expr' will be optional<T> or optional<U> (but not tc_optional_base<..>)
    template<class Expr>
    void construct ( Expr&& expr, optional_tag const* )
     {
       if ( expr.is_initialized() )
       {
         // An exception can be thrown here.
         // It it happens, THIS will be left uninitialized.
         m_storage = value_type(boost::move(expr.get())) ;
         m_initialized = true ;
       }
     }
#else
    // Notice that 'Expr' will be optional<T> or optional<U> (but not tc_optional_base<..>)
    template<class Expr>
    void construct ( Expr const& expr, optional_tag const* )
     {
       if ( expr.is_initialized() )
       {
         // An exception can be thrown here.
         // It it happens, THIS will be left uninitialized.
         m_storage = value_type(expr.get()) ;
         m_initialized = true ;
       }
     }
#endif
#endif // defined BOOST_OPTIONAL_WEAK_OVERLOAD_RESOLUTION

    void assign_value ( argument_type val ) { m_storage = val; }
#ifndef  BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
    void assign_value ( rval_reference_type val ) { m_storage = static_cast<rval_reference_type>(val); }
#endif

    void destroy()
    {
      m_initialized = false;
    }

    reference_const_type get_impl() const { return m_storage ; }
    reference_type       get_impl()       { return m_storage ; }

    pointer_const_type get_ptr_impl() const { return boost::addressof(m_storage); }
    pointer_type       get_ptr_impl()       { return boost::addressof(m_storage); }

  private :

    bool m_initialized ;
    T    m_storage ;
} ;

    ///////////////////////////////////////////////////////////////////////////////
    /// \file make.hpp
    /// Contains definition of the make<> transform.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
    namespace detail
    {
        template<typename R >
        struct is_applyable<R()>
          : mpl::true_
        {};
        template<typename R >
        struct is_applyable<R(*)()>
          : mpl::true_
        {};
        template<typename T, typename A>
        struct construct_<proto::expr<T, A, 0>, true>
        {
            typedef proto::expr<T, A, 0> result_type;
            template<typename A0>
            BOOST_FORCEINLINE
            result_type operator ()(A0 &a0) const
            {
                return result_type::make(a0);
            }
        };
        template<typename T, typename A>
        struct construct_<proto::basic_expr<T, A, 0>, true>
        {
            typedef proto::basic_expr<T, A, 0> result_type;
            template<typename A0>
            BOOST_FORCEINLINE
            result_type operator ()(A0 &a0) const
            {
                return result_type::make(a0);
            }
        };
        template<typename Type >
        BOOST_FORCEINLINE
        Type construct()
        {
            return construct_<Type>()();
        }
    } 
    
    
    
    
    template<typename Object >
    struct make<Object()>
      : transform<make<Object()> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            
            typedef typename detail::make_if_<Object, Expr, State, Data>::type result_type;
            
            
            
            
            
            
            
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                proto::detail::ignore_unused(e);
                proto::detail::ignore_unused(s);
                proto::detail::ignore_unused(d);
                return detail::construct<result_type>();
            }
        };
    };
    namespace detail
    {
        template<
            template<typename> class R
            , typename A0
          , typename Expr, typename State, typename Data
        >
        struct make_<
            R<A0>
          , Expr, State, Data
            BOOST_PROTO_TEMPLATE_ARITY_PARAM(1)
        >
          : nested_type_if<
                R<typename make_if_<A0, Expr, State, Data> ::type>
              , (make_if_<A0, Expr, State, Data> ::applied || false)
            >
        {};
        template<
            template<typename> class R
            , typename A0
          , typename Expr, typename State, typename Data
        >
        struct make_<
            noinvoke<R<A0> >
          , Expr, State, Data
            BOOST_PROTO_TEMPLATE_ARITY_PARAM(1)
        >
        {
            typedef R<typename make_if_<A0, Expr, State, Data> ::type> type;
            static bool const applied = true;
        };
        template<typename R , typename A0>
        struct is_applyable<R(A0)>
          : mpl::true_
        {};
        template<typename R , typename A0>
        struct is_applyable<R(*)(A0)>
          : mpl::true_
        {};
        template<typename T, typename A>
        struct construct_<proto::expr<T, A, 1>, true>
        {
            typedef proto::expr<T, A, 1> result_type;
            template<typename A0>
            BOOST_FORCEINLINE
            result_type operator ()(A0 &a0) const
            {
                return result_type::make(a0);
            }
        };
        template<typename T, typename A>
        struct construct_<proto::basic_expr<T, A, 1>, true>
        {
            typedef proto::basic_expr<T, A, 1> result_type;
            template<typename A0>
            BOOST_FORCEINLINE
            result_type operator ()(A0 &a0) const
            {
                return result_type::make(a0);
            }
        };
        template<typename Type , typename A0>
        BOOST_FORCEINLINE
        Type construct(A0 &a0)
        {
            return construct_<Type>()(a0);
        }
    } 
    
    
    
    
    template<typename Object , typename A0>
    struct make<Object(A0)>
      : transform<make<Object(A0)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            
            typedef typename detail::make_if_<Object, Expr, State, Data>::type result_type;
            
            
            
            
            
            
            
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                proto::detail::ignore_unused(e);
                proto::detail::ignore_unused(s);
                proto::detail::ignore_unused(d);
                return detail::construct<result_type>(detail::as_lvalue( typename when<_, A0>::template impl<Expr, State, Data>()(e, s, d) ));
            }
        };
    };
    
    
    
    
    template<typename Object , typename A0>
    struct make<Object(A0...)>
      : transform<make<Object(A0...)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : make<
                typename detail::expand_pattern<
                    proto::arity_of<Expr>::value
                  , A0
                  , detail::expand_pattern_rest_0<
                        Object
                        
                    >
                >::type
            >::template impl<Expr, State, Data>
        {};
    };
    namespace detail
    {
        template<
            template<typename , typename> class R
            , typename A0 , typename A1
          , typename Expr, typename State, typename Data
        >
        struct make_<
            R<A0 , A1>
          , Expr, State, Data
            BOOST_PROTO_TEMPLATE_ARITY_PARAM(2)
        >
          : nested_type_if<
                R<typename make_if_<A0, Expr, State, Data> ::type , typename make_if_<A1, Expr, State, Data> ::type>
              , (make_if_<A0, Expr, State, Data> ::applied || make_if_<A1, Expr, State, Data> ::applied || false)
            >
        {};
        template<
            template<typename , typename> class R
            , typename A0 , typename A1
          , typename Expr, typename State, typename Data
        >
        struct make_<
            noinvoke<R<A0 , A1> >
          , Expr, State, Data
            BOOST_PROTO_TEMPLATE_ARITY_PARAM(1)
        >
        {
            typedef R<typename make_if_<A0, Expr, State, Data> ::type , typename make_if_<A1, Expr, State, Data> ::type> type;
            static bool const applied = true;
        };
        template<typename R , typename A0 , typename A1>
        struct is_applyable<R(A0 , A1)>
          : mpl::true_
        {};
        template<typename R , typename A0 , typename A1>
        struct is_applyable<R(*)(A0 , A1)>
          : mpl::true_
        {};
        template<typename T, typename A>
        struct construct_<proto::expr<T, A, 2>, true>
        {
            typedef proto::expr<T, A, 2> result_type;
            template<typename A0 , typename A1>
            BOOST_FORCEINLINE
            result_type operator ()(A0 &a0 , A1 &a1) const
            {
                return result_type::make(a0 , a1);
            }
        };
        template<typename T, typename A>
        struct construct_<proto::basic_expr<T, A, 2>, true>
        {
            typedef proto::basic_expr<T, A, 2> result_type;
            template<typename A0 , typename A1>
            BOOST_FORCEINLINE
            result_type operator ()(A0 &a0 , A1 &a1) const
            {
                return result_type::make(a0 , a1);
            }
        };
        template<typename Type , typename A0 , typename A1>
        BOOST_FORCEINLINE
        Type construct(A0 &a0 , A1 &a1)
        {
            return construct_<Type>()(a0 , a1);
        }
    } 
    
    
    
    
    template<typename Object , typename A0 , typename A1>
    struct make<Object(A0 , A1)>
      : transform<make<Object(A0 , A1)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            
            typedef typename detail::make_if_<Object, Expr, State, Data>::type result_type;
            
            
            
            
            
            
            
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                proto::detail::ignore_unused(e);
                proto::detail::ignore_unused(s);
                proto::detail::ignore_unused(d);
                return detail::construct<result_type>(detail::as_lvalue( typename when<_, A0>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A1>::template impl<Expr, State, Data>()(e, s, d) ));
            }
        };
    };
    
    
    
    
    template<typename Object , typename A0 , typename A1>
    struct make<Object(A0 , A1...)>
      : transform<make<Object(A0 , A1...)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : make<
                typename detail::expand_pattern<
                    proto::arity_of<Expr>::value
                  , A1
                  , detail::expand_pattern_rest_1<
                        Object
                        , A0
                    >
                >::type
            >::template impl<Expr, State, Data>
        {};
    };
    namespace detail
    {
        template<
            template<typename , typename , typename> class R
            , typename A0 , typename A1 , typename A2
          , typename Expr, typename State, typename Data
        >
        struct make_<
            R<A0 , A1 , A2>
          , Expr, State, Data
            BOOST_PROTO_TEMPLATE_ARITY_PARAM(3)
        >
          : nested_type_if<
                R<typename make_if_<A0, Expr, State, Data> ::type , typename make_if_<A1, Expr, State, Data> ::type , typename make_if_<A2, Expr, State, Data> ::type>
              , (make_if_<A0, Expr, State, Data> ::applied || make_if_<A1, Expr, State, Data> ::applied || make_if_<A2, Expr, State, Data> ::applied || false)
            >
        {};
        template<
            template<typename , typename , typename> class R
            , typename A0 , typename A1 , typename A2
          , typename Expr, typename State, typename Data
        >
        struct make_<
            noinvoke<R<A0 , A1 , A2> >
          , Expr, State, Data
            BOOST_PROTO_TEMPLATE_ARITY_PARAM(1)
        >
        {
            typedef R<typename make_if_<A0, Expr, State, Data> ::type , typename make_if_<A1, Expr, State, Data> ::type , typename make_if_<A2, Expr, State, Data> ::type> type;
            static bool const applied = true;
        };
        template<typename R , typename A0 , typename A1 , typename A2>
        struct is_applyable<R(A0 , A1 , A2)>
          : mpl::true_
        {};
        template<typename R , typename A0 , typename A1 , typename A2>
        struct is_applyable<R(*)(A0 , A1 , A2)>
          : mpl::true_
        {};
        template<typename T, typename A>
        struct construct_<proto::expr<T, A, 3>, true>
        {
            typedef proto::expr<T, A, 3> result_type;
            template<typename A0 , typename A1 , typename A2>
            BOOST_FORCEINLINE
            result_type operator ()(A0 &a0 , A1 &a1 , A2 &a2) const
            {
                return result_type::make(a0 , a1 , a2);
            }
        };
        template<typename T, typename A>
        struct construct_<proto::basic_expr<T, A, 3>, true>
        {
            typedef proto::basic_expr<T, A, 3> result_type;
            template<typename A0 , typename A1 , typename A2>
            BOOST_FORCEINLINE
            result_type operator ()(A0 &a0 , A1 &a1 , A2 &a2) const
            {
                return result_type::make(a0 , a1 , a2);
            }
        };
        template<typename Type , typename A0 , typename A1 , typename A2>
        BOOST_FORCEINLINE
        Type construct(A0 &a0 , A1 &a1 , A2 &a2)
        {
            return construct_<Type>()(a0 , a1 , a2);
        }
    } 
    
    
    
    
    template<typename Object , typename A0 , typename A1 , typename A2>
    struct make<Object(A0 , A1 , A2)>
      : transform<make<Object(A0 , A1 , A2)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            
            typedef typename detail::make_if_<Object, Expr, State, Data>::type result_type;
            
            
            
            
            
            
            
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                proto::detail::ignore_unused(e);
                proto::detail::ignore_unused(s);
                proto::detail::ignore_unused(d);
                return detail::construct<result_type>(detail::as_lvalue( typename when<_, A0>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A1>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A2>::template impl<Expr, State, Data>()(e, s, d) ));
            }
        };
    };
    
    
    
    
    template<typename Object , typename A0 , typename A1 , typename A2>
    struct make<Object(A0 , A1 , A2...)>
      : transform<make<Object(A0 , A1 , A2...)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : make<
                typename detail::expand_pattern<
                    proto::arity_of<Expr>::value
                  , A2
                  , detail::expand_pattern_rest_2<
                        Object
                        , A0 , A1
                    >
                >::type
            >::template impl<Expr, State, Data>
        {};
    };
    namespace detail
    {
        template<
            template<typename , typename , typename , typename> class R
            , typename A0 , typename A1 , typename A2 , typename A3
          , typename Expr, typename State, typename Data
        >
        struct make_<
            R<A0 , A1 , A2 , A3>
          , Expr, State, Data
            BOOST_PROTO_TEMPLATE_ARITY_PARAM(4)
        >
          : nested_type_if<
                R<typename make_if_<A0, Expr, State, Data> ::type , typename make_if_<A1, Expr, State, Data> ::type , typename make_if_<A2, Expr, State, Data> ::type , typename make_if_<A3, Expr, State, Data> ::type>
              , (make_if_<A0, Expr, State, Data> ::applied || make_if_<A1, Expr, State, Data> ::applied || make_if_<A2, Expr, State, Data> ::applied || make_if_<A3, Expr, State, Data> ::applied || false)
            >
        {};
        template<
            template<typename , typename , typename , typename> class R
            , typename A0 , typename A1 , typename A2 , typename A3
          , typename Expr, typename State, typename Data
        >
        struct make_<
            noinvoke<R<A0 , A1 , A2 , A3> >
          , Expr, State, Data
            BOOST_PROTO_TEMPLATE_ARITY_PARAM(1)
        >
        {
            typedef R<typename make_if_<A0, Expr, State, Data> ::type , typename make_if_<A1, Expr, State, Data> ::type , typename make_if_<A2, Expr, State, Data> ::type , typename make_if_<A3, Expr, State, Data> ::type> type;
            static bool const applied = true;
        };
        template<typename R , typename A0 , typename A1 , typename A2 , typename A3>
        struct is_applyable<R(A0 , A1 , A2 , A3)>
          : mpl::true_
        {};
        template<typename R , typename A0 , typename A1 , typename A2 , typename A3>
        struct is_applyable<R(*)(A0 , A1 , A2 , A3)>
          : mpl::true_
        {};
        template<typename T, typename A>
        struct construct_<proto::expr<T, A, 4>, true>
        {
            typedef proto::expr<T, A, 4> result_type;
            template<typename A0 , typename A1 , typename A2 , typename A3>
            BOOST_FORCEINLINE
            result_type operator ()(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3) const
            {
                return result_type::make(a0 , a1 , a2 , a3);
            }
        };
        template<typename T, typename A>
        struct construct_<proto::basic_expr<T, A, 4>, true>
        {
            typedef proto::basic_expr<T, A, 4> result_type;
            template<typename A0 , typename A1 , typename A2 , typename A3>
            BOOST_FORCEINLINE
            result_type operator ()(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3) const
            {
                return result_type::make(a0 , a1 , a2 , a3);
            }
        };
        template<typename Type , typename A0 , typename A1 , typename A2 , typename A3>
        BOOST_FORCEINLINE
        Type construct(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3)
        {
            return construct_<Type>()(a0 , a1 , a2 , a3);
        }
    } 
    
    
    
    
    template<typename Object , typename A0 , typename A1 , typename A2 , typename A3>
    struct make<Object(A0 , A1 , A2 , A3)>
      : transform<make<Object(A0 , A1 , A2 , A3)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            
            typedef typename detail::make_if_<Object, Expr, State, Data>::type result_type;
            
            
            
            
            
            
            
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                proto::detail::ignore_unused(e);
                proto::detail::ignore_unused(s);
                proto::detail::ignore_unused(d);
                return detail::construct<result_type>(detail::as_lvalue( typename when<_, A0>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A1>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A2>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A3>::template impl<Expr, State, Data>()(e, s, d) ));
            }
        };
    };
    
    
    
    
    template<typename Object , typename A0 , typename A1 , typename A2 , typename A3>
    struct make<Object(A0 , A1 , A2 , A3...)>
      : transform<make<Object(A0 , A1 , A2 , A3...)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : make<
                typename detail::expand_pattern<
                    proto::arity_of<Expr>::value
                  , A3
                  , detail::expand_pattern_rest_3<
                        Object
                        , A0 , A1 , A2
                    >
                >::type
            >::template impl<Expr, State, Data>
        {};
    };
    namespace detail
    {
        template<
            template<typename , typename , typename , typename , typename> class R
            , typename A0 , typename A1 , typename A2 , typename A3 , typename A4
          , typename Expr, typename State, typename Data
        >
        struct make_<
            R<A0 , A1 , A2 , A3 , A4>
          , Expr, State, Data
            BOOST_PROTO_TEMPLATE_ARITY_PARAM(5)
        >
          : nested_type_if<
                R<typename make_if_<A0, Expr, State, Data> ::type , typename make_if_<A1, Expr, State, Data> ::type , typename make_if_<A2, Expr, State, Data> ::type , typename make_if_<A3, Expr, State, Data> ::type , typename make_if_<A4, Expr, State, Data> ::type>
              , (make_if_<A0, Expr, State, Data> ::applied || make_if_<A1, Expr, State, Data> ::applied || make_if_<A2, Expr, State, Data> ::applied || make_if_<A3, Expr, State, Data> ::applied || make_if_<A4, Expr, State, Data> ::applied || false)
            >
        {};
        template<
            template<typename , typename , typename , typename , typename> class R
            , typename A0 , typename A1 , typename A2 , typename A3 , typename A4
          , typename Expr, typename State, typename Data
        >
        struct make_<
            noinvoke<R<A0 , A1 , A2 , A3 , A4> >
          , Expr, State, Data
            BOOST_PROTO_TEMPLATE_ARITY_PARAM(1)
        >
        {
            typedef R<typename make_if_<A0, Expr, State, Data> ::type , typename make_if_<A1, Expr, State, Data> ::type , typename make_if_<A2, Expr, State, Data> ::type , typename make_if_<A3, Expr, State, Data> ::type , typename make_if_<A4, Expr, State, Data> ::type> type;
            static bool const applied = true;
        };
        template<typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
        struct is_applyable<R(A0 , A1 , A2 , A3 , A4)>
          : mpl::true_
        {};
        template<typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
        struct is_applyable<R(*)(A0 , A1 , A2 , A3 , A4)>
          : mpl::true_
        {};
        template<typename T, typename A>
        struct construct_<proto::expr<T, A, 5>, true>
        {
            typedef proto::expr<T, A, 5> result_type;
            template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
            BOOST_FORCEINLINE
            result_type operator ()(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3 , A4 &a4) const
            {
                return result_type::make(a0 , a1 , a2 , a3 , a4);
            }
        };
        template<typename T, typename A>
        struct construct_<proto::basic_expr<T, A, 5>, true>
        {
            typedef proto::basic_expr<T, A, 5> result_type;
            template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
            BOOST_FORCEINLINE
            result_type operator ()(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3 , A4 &a4) const
            {
                return result_type::make(a0 , a1 , a2 , a3 , a4);
            }
        };
        template<typename Type , typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
        BOOST_FORCEINLINE
        Type construct(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3 , A4 &a4)
        {
            return construct_<Type>()(a0 , a1 , a2 , a3 , a4);
        }
    } 
    
    
    
    
    template<typename Object , typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
    struct make<Object(A0 , A1 , A2 , A3 , A4)>
      : transform<make<Object(A0 , A1 , A2 , A3 , A4)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            
            typedef typename detail::make_if_<Object, Expr, State, Data>::type result_type;
            
            
            
            
            
            
            
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                proto::detail::ignore_unused(e);
                proto::detail::ignore_unused(s);
                proto::detail::ignore_unused(d);
                return detail::construct<result_type>(detail::as_lvalue( typename when<_, A0>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A1>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A2>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A3>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A4>::template impl<Expr, State, Data>()(e, s, d) ));
            }
        };
    };
    
    
    
    
    template<typename Object , typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
    struct make<Object(A0 , A1 , A2 , A3 , A4...)>
      : transform<make<Object(A0 , A1 , A2 , A3 , A4...)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : make<
                typename detail::expand_pattern<
                    proto::arity_of<Expr>::value
                  , A4
                  , detail::expand_pattern_rest_4<
                        Object
                        , A0 , A1 , A2 , A3
                    >
                >::type
            >::template impl<Expr, State, Data>
        {};
    };
    namespace detail
    {
        template<
            template<typename , typename , typename , typename , typename , typename> class R
            , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5
          , typename Expr, typename State, typename Data
        >
        struct make_<
            R<A0 , A1 , A2 , A3 , A4 , A5>
          , Expr, State, Data
            BOOST_PROTO_TEMPLATE_ARITY_PARAM(6)
        >
          : nested_type_if<
                R<typename make_if_<A0, Expr, State, Data> ::type , typename make_if_<A1, Expr, State, Data> ::type , typename make_if_<A2, Expr, State, Data> ::type , typename make_if_<A3, Expr, State, Data> ::type , typename make_if_<A4, Expr, State, Data> ::type , typename make_if_<A5, Expr, State, Data> ::type>
              , (make_if_<A0, Expr, State, Data> ::applied || make_if_<A1, Expr, State, Data> ::applied || make_if_<A2, Expr, State, Data> ::applied || make_if_<A3, Expr, State, Data> ::applied || make_if_<A4, Expr, State, Data> ::applied || make_if_<A5, Expr, State, Data> ::applied || false)
            >
        {};
        template<
            template<typename , typename , typename , typename , typename , typename> class R
            , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5
          , typename Expr, typename State, typename Data
        >
        struct make_<
            noinvoke<R<A0 , A1 , A2 , A3 , A4 , A5> >
          , Expr, State, Data
            BOOST_PROTO_TEMPLATE_ARITY_PARAM(1)
        >
        {
            typedef R<typename make_if_<A0, Expr, State, Data> ::type , typename make_if_<A1, Expr, State, Data> ::type , typename make_if_<A2, Expr, State, Data> ::type , typename make_if_<A3, Expr, State, Data> ::type , typename make_if_<A4, Expr, State, Data> ::type , typename make_if_<A5, Expr, State, Data> ::type> type;
            static bool const applied = true;
        };
        template<typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
        struct is_applyable<R(A0 , A1 , A2 , A3 , A4 , A5)>
          : mpl::true_
        {};
        template<typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
        struct is_applyable<R(*)(A0 , A1 , A2 , A3 , A4 , A5)>
          : mpl::true_
        {};
        template<typename T, typename A>
        struct construct_<proto::expr<T, A, 6>, true>
        {
            typedef proto::expr<T, A, 6> result_type;
            template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
            BOOST_FORCEINLINE
            result_type operator ()(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3 , A4 &a4 , A5 &a5) const
            {
                return result_type::make(a0 , a1 , a2 , a3 , a4 , a5);
            }
        };
        template<typename T, typename A>
        struct construct_<proto::basic_expr<T, A, 6>, true>
        {
            typedef proto::basic_expr<T, A, 6> result_type;
            template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
            BOOST_FORCEINLINE
            result_type operator ()(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3 , A4 &a4 , A5 &a5) const
            {
                return result_type::make(a0 , a1 , a2 , a3 , a4 , a5);
            }
        };
        template<typename Type , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
        BOOST_FORCEINLINE
        Type construct(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3 , A4 &a4 , A5 &a5)
        {
            return construct_<Type>()(a0 , a1 , a2 , a3 , a4 , a5);
        }
    } 
    
    
    
    
    template<typename Object , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
    struct make<Object(A0 , A1 , A2 , A3 , A4 , A5)>
      : transform<make<Object(A0 , A1 , A2 , A3 , A4 , A5)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            
            typedef typename detail::make_if_<Object, Expr, State, Data>::type result_type;
            
            
            
            
            
            
            
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                proto::detail::ignore_unused(e);
                proto::detail::ignore_unused(s);
                proto::detail::ignore_unused(d);
                return detail::construct<result_type>(detail::as_lvalue( typename when<_, A0>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A1>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A2>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A3>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A4>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A5>::template impl<Expr, State, Data>()(e, s, d) ));
            }
        };
    };
    
    
    
    
    template<typename Object , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
    struct make<Object(A0 , A1 , A2 , A3 , A4 , A5...)>
      : transform<make<Object(A0 , A1 , A2 , A3 , A4 , A5...)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : make<
                typename detail::expand_pattern<
                    proto::arity_of<Expr>::value
                  , A5
                  , detail::expand_pattern_rest_5<
                        Object
                        , A0 , A1 , A2 , A3 , A4
                    >
                >::type
            >::template impl<Expr, State, Data>
        {};
    };
    namespace detail
    {
        template<
            template<typename , typename , typename , typename , typename , typename , typename> class R
            , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6
          , typename Expr, typename State, typename Data
        >
        struct make_<
            R<A0 , A1 , A2 , A3 , A4 , A5 , A6>
          , Expr, State, Data
            BOOST_PROTO_TEMPLATE_ARITY_PARAM(7)
        >
          : nested_type_if<
                R<typename make_if_<A0, Expr, State, Data> ::type , typename make_if_<A1, Expr, State, Data> ::type , typename make_if_<A2, Expr, State, Data> ::type , typename make_if_<A3, Expr, State, Data> ::type , typename make_if_<A4, Expr, State, Data> ::type , typename make_if_<A5, Expr, State, Data> ::type , typename make_if_<A6, Expr, State, Data> ::type>
              , (make_if_<A0, Expr, State, Data> ::applied || make_if_<A1, Expr, State, Data> ::applied || make_if_<A2, Expr, State, Data> ::applied || make_if_<A3, Expr, State, Data> ::applied || make_if_<A4, Expr, State, Data> ::applied || make_if_<A5, Expr, State, Data> ::applied || make_if_<A6, Expr, State, Data> ::applied || false)
            >
        {};
        template<
            template<typename , typename , typename , typename , typename , typename , typename> class R
            , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6
          , typename Expr, typename State, typename Data
        >
        struct make_<
            noinvoke<R<A0 , A1 , A2 , A3 , A4 , A5 , A6> >
          , Expr, State, Data
            BOOST_PROTO_TEMPLATE_ARITY_PARAM(1)
        >
        {
            typedef R<typename make_if_<A0, Expr, State, Data> ::type , typename make_if_<A1, Expr, State, Data> ::type , typename make_if_<A2, Expr, State, Data> ::type , typename make_if_<A3, Expr, State, Data> ::type , typename make_if_<A4, Expr, State, Data> ::type , typename make_if_<A5, Expr, State, Data> ::type , typename make_if_<A6, Expr, State, Data> ::type> type;
            static bool const applied = true;
        };
        template<typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
        struct is_applyable<R(A0 , A1 , A2 , A3 , A4 , A5 , A6)>
          : mpl::true_
        {};
        template<typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
        struct is_applyable<R(*)(A0 , A1 , A2 , A3 , A4 , A5 , A6)>
          : mpl::true_
        {};
        template<typename T, typename A>
        struct construct_<proto::expr<T, A, 7>, true>
        {
            typedef proto::expr<T, A, 7> result_type;
            template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
            BOOST_FORCEINLINE
            result_type operator ()(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3 , A4 &a4 , A5 &a5 , A6 &a6) const
            {
                return result_type::make(a0 , a1 , a2 , a3 , a4 , a5 , a6);
            }
        };
        template<typename T, typename A>
        struct construct_<proto::basic_expr<T, A, 7>, true>
        {
            typedef proto::basic_expr<T, A, 7> result_type;
            template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
            BOOST_FORCEINLINE
            result_type operator ()(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3 , A4 &a4 , A5 &a5 , A6 &a6) const
            {
                return result_type::make(a0 , a1 , a2 , a3 , a4 , a5 , a6);
            }
        };
        template<typename Type , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
        BOOST_FORCEINLINE
        Type construct(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3 , A4 &a4 , A5 &a5 , A6 &a6)
        {
            return construct_<Type>()(a0 , a1 , a2 , a3 , a4 , a5 , a6);
        }
    } 
    
    
    
    
    template<typename Object , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
    struct make<Object(A0 , A1 , A2 , A3 , A4 , A5 , A6)>
      : transform<make<Object(A0 , A1 , A2 , A3 , A4 , A5 , A6)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            
            typedef typename detail::make_if_<Object, Expr, State, Data>::type result_type;
            
            
            
            
            
            
            
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                proto::detail::ignore_unused(e);
                proto::detail::ignore_unused(s);
                proto::detail::ignore_unused(d);
                return detail::construct<result_type>(detail::as_lvalue( typename when<_, A0>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A1>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A2>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A3>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A4>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A5>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A6>::template impl<Expr, State, Data>()(e, s, d) ));
            }
        };
    };
    
    
    
    
    template<typename Object , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
    struct make<Object(A0 , A1 , A2 , A3 , A4 , A5 , A6...)>
      : transform<make<Object(A0 , A1 , A2 , A3 , A4 , A5 , A6...)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : make<
                typename detail::expand_pattern<
                    proto::arity_of<Expr>::value
                  , A6
                  , detail::expand_pattern_rest_6<
                        Object
                        , A0 , A1 , A2 , A3 , A4 , A5
                    >
                >::type
            >::template impl<Expr, State, Data>
        {};
    };
    namespace detail
    {
        template<
            template<typename , typename , typename , typename , typename , typename , typename , typename> class R
            , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7
          , typename Expr, typename State, typename Data
        >
        struct make_<
            R<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7>
          , Expr, State, Data
            BOOST_PROTO_TEMPLATE_ARITY_PARAM(8)
        >
          : nested_type_if<
                R<typename make_if_<A0, Expr, State, Data> ::type , typename make_if_<A1, Expr, State, Data> ::type , typename make_if_<A2, Expr, State, Data> ::type , typename make_if_<A3, Expr, State, Data> ::type , typename make_if_<A4, Expr, State, Data> ::type , typename make_if_<A5, Expr, State, Data> ::type , typename make_if_<A6, Expr, State, Data> ::type , typename make_if_<A7, Expr, State, Data> ::type>
              , (make_if_<A0, Expr, State, Data> ::applied || make_if_<A1, Expr, State, Data> ::applied || make_if_<A2, Expr, State, Data> ::applied || make_if_<A3, Expr, State, Data> ::applied || make_if_<A4, Expr, State, Data> ::applied || make_if_<A5, Expr, State, Data> ::applied || make_if_<A6, Expr, State, Data> ::applied || make_if_<A7, Expr, State, Data> ::applied || false)
            >
        {};
        template<
            template<typename , typename , typename , typename , typename , typename , typename , typename> class R
            , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7
          , typename Expr, typename State, typename Data
        >
        struct make_<
            noinvoke<R<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7> >
          , Expr, State, Data
            BOOST_PROTO_TEMPLATE_ARITY_PARAM(1)
        >
        {
            typedef R<typename make_if_<A0, Expr, State, Data> ::type , typename make_if_<A1, Expr, State, Data> ::type , typename make_if_<A2, Expr, State, Data> ::type , typename make_if_<A3, Expr, State, Data> ::type , typename make_if_<A4, Expr, State, Data> ::type , typename make_if_<A5, Expr, State, Data> ::type , typename make_if_<A6, Expr, State, Data> ::type , typename make_if_<A7, Expr, State, Data> ::type> type;
            static bool const applied = true;
        };
        template<typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
        struct is_applyable<R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7)>
          : mpl::true_
        {};
        template<typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
        struct is_applyable<R(*)(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7)>
          : mpl::true_
        {};
        template<typename T, typename A>
        struct construct_<proto::expr<T, A, 8>, true>
        {
            typedef proto::expr<T, A, 8> result_type;
            template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
            BOOST_FORCEINLINE
            result_type operator ()(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3 , A4 &a4 , A5 &a5 , A6 &a6 , A7 &a7) const
            {
                return result_type::make(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7);
            }
        };
        template<typename T, typename A>
        struct construct_<proto::basic_expr<T, A, 8>, true>
        {
            typedef proto::basic_expr<T, A, 8> result_type;
            template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
            BOOST_FORCEINLINE
            result_type operator ()(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3 , A4 &a4 , A5 &a5 , A6 &a6 , A7 &a7) const
            {
                return result_type::make(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7);
            }
        };
        template<typename Type , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
        BOOST_FORCEINLINE
        Type construct(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3 , A4 &a4 , A5 &a5 , A6 &a6 , A7 &a7)
        {
            return construct_<Type>()(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7);
        }
    } 
    
    
    
    
    template<typename Object , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
    struct make<Object(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7)>
      : transform<make<Object(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            
            typedef typename detail::make_if_<Object, Expr, State, Data>::type result_type;
            
            
            
            
            
            
            
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                proto::detail::ignore_unused(e);
                proto::detail::ignore_unused(s);
                proto::detail::ignore_unused(d);
                return detail::construct<result_type>(detail::as_lvalue( typename when<_, A0>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A1>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A2>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A3>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A4>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A5>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A6>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A7>::template impl<Expr, State, Data>()(e, s, d) ));
            }
        };
    };
    
    
    
    
    template<typename Object , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
    struct make<Object(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7...)>
      : transform<make<Object(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7...)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : make<
                typename detail::expand_pattern<
                    proto::arity_of<Expr>::value
                  , A7
                  , detail::expand_pattern_rest_7<
                        Object
                        , A0 , A1 , A2 , A3 , A4 , A5 , A6
                    >
                >::type
            >::template impl<Expr, State, Data>
        {};
    };
    namespace detail
    {
        template<
            template<typename , typename , typename , typename , typename , typename , typename , typename , typename> class R
            , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8
          , typename Expr, typename State, typename Data
        >
        struct make_<
            R<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8>
          , Expr, State, Data
            BOOST_PROTO_TEMPLATE_ARITY_PARAM(9)
        >
          : nested_type_if<
                R<typename make_if_<A0, Expr, State, Data> ::type , typename make_if_<A1, Expr, State, Data> ::type , typename make_if_<A2, Expr, State, Data> ::type , typename make_if_<A3, Expr, State, Data> ::type , typename make_if_<A4, Expr, State, Data> ::type , typename make_if_<A5, Expr, State, Data> ::type , typename make_if_<A6, Expr, State, Data> ::type , typename make_if_<A7, Expr, State, Data> ::type , typename make_if_<A8, Expr, State, Data> ::type>
              , (make_if_<A0, Expr, State, Data> ::applied || make_if_<A1, Expr, State, Data> ::applied || make_if_<A2, Expr, State, Data> ::applied || make_if_<A3, Expr, State, Data> ::applied || make_if_<A4, Expr, State, Data> ::applied || make_if_<A5, Expr, State, Data> ::applied || make_if_<A6, Expr, State, Data> ::applied || make_if_<A7, Expr, State, Data> ::applied || make_if_<A8, Expr, State, Data> ::applied || false)
            >
        {};
        template<
            template<typename , typename , typename , typename , typename , typename , typename , typename , typename> class R
            , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8
          , typename Expr, typename State, typename Data
        >
        struct make_<
            noinvoke<R<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8> >
          , Expr, State, Data
            BOOST_PROTO_TEMPLATE_ARITY_PARAM(1)
        >
        {
            typedef R<typename make_if_<A0, Expr, State, Data> ::type , typename make_if_<A1, Expr, State, Data> ::type , typename make_if_<A2, Expr, State, Data> ::type , typename make_if_<A3, Expr, State, Data> ::type , typename make_if_<A4, Expr, State, Data> ::type , typename make_if_<A5, Expr, State, Data> ::type , typename make_if_<A6, Expr, State, Data> ::type , typename make_if_<A7, Expr, State, Data> ::type , typename make_if_<A8, Expr, State, Data> ::type> type;
            static bool const applied = true;
        };
        template<typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
        struct is_applyable<R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8)>
          : mpl::true_
        {};
        template<typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
        struct is_applyable<R(*)(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8)>
          : mpl::true_
        {};
        template<typename T, typename A>
        struct construct_<proto::expr<T, A, 9>, true>
        {
            typedef proto::expr<T, A, 9> result_type;
            template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
            BOOST_FORCEINLINE
            result_type operator ()(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3 , A4 &a4 , A5 &a5 , A6 &a6 , A7 &a7 , A8 &a8) const
            {
                return result_type::make(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8);
            }
        };
        template<typename T, typename A>
        struct construct_<proto::basic_expr<T, A, 9>, true>
        {
            typedef proto::basic_expr<T, A, 9> result_type;
            template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
            BOOST_FORCEINLINE
            result_type operator ()(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3 , A4 &a4 , A5 &a5 , A6 &a6 , A7 &a7 , A8 &a8) const
            {
                return result_type::make(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8);
            }
        };
        template<typename Type , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
        BOOST_FORCEINLINE
        Type construct(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3 , A4 &a4 , A5 &a5 , A6 &a6 , A7 &a7 , A8 &a8)
        {
            return construct_<Type>()(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8);
        }
    } 
    
    
    
    
    template<typename Object , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
    struct make<Object(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8)>
      : transform<make<Object(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            
            typedef typename detail::make_if_<Object, Expr, State, Data>::type result_type;
            
            
            
            
            
            
            
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                proto::detail::ignore_unused(e);
                proto::detail::ignore_unused(s);
                proto::detail::ignore_unused(d);
                return detail::construct<result_type>(detail::as_lvalue( typename when<_, A0>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A1>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A2>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A3>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A4>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A5>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A6>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A7>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A8>::template impl<Expr, State, Data>()(e, s, d) ));
            }
        };
    };
    
    
    
    
    template<typename Object , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
    struct make<Object(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8...)>
      : transform<make<Object(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8...)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : make<
                typename detail::expand_pattern<
                    proto::arity_of<Expr>::value
                  , A8
                  , detail::expand_pattern_rest_8<
                        Object
                        , A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7
                    >
                >::type
            >::template impl<Expr, State, Data>
        {};
    };
    namespace detail
    {
        template<
            template<typename , typename , typename , typename , typename , typename , typename , typename , typename , typename> class R
            , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9
          , typename Expr, typename State, typename Data
        >
        struct make_<
            R<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9>
          , Expr, State, Data
            BOOST_PROTO_TEMPLATE_ARITY_PARAM(10)
        >
          : nested_type_if<
                R<typename make_if_<A0, Expr, State, Data> ::type , typename make_if_<A1, Expr, State, Data> ::type , typename make_if_<A2, Expr, State, Data> ::type , typename make_if_<A3, Expr, State, Data> ::type , typename make_if_<A4, Expr, State, Data> ::type , typename make_if_<A5, Expr, State, Data> ::type , typename make_if_<A6, Expr, State, Data> ::type , typename make_if_<A7, Expr, State, Data> ::type , typename make_if_<A8, Expr, State, Data> ::type , typename make_if_<A9, Expr, State, Data> ::type>
              , (make_if_<A0, Expr, State, Data> ::applied || make_if_<A1, Expr, State, Data> ::applied || make_if_<A2, Expr, State, Data> ::applied || make_if_<A3, Expr, State, Data> ::applied || make_if_<A4, Expr, State, Data> ::applied || make_if_<A5, Expr, State, Data> ::applied || make_if_<A6, Expr, State, Data> ::applied || make_if_<A7, Expr, State, Data> ::applied || make_if_<A8, Expr, State, Data> ::applied || make_if_<A9, Expr, State, Data> ::applied || false)
            >
        {};
        template<
            template<typename , typename , typename , typename , typename , typename , typename , typename , typename , typename> class R
            , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9
          , typename Expr, typename State, typename Data
        >
        struct make_<
            noinvoke<R<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9> >
          , Expr, State, Data
            BOOST_PROTO_TEMPLATE_ARITY_PARAM(1)
        >
        {
            typedef R<typename make_if_<A0, Expr, State, Data> ::type , typename make_if_<A1, Expr, State, Data> ::type , typename make_if_<A2, Expr, State, Data> ::type , typename make_if_<A3, Expr, State, Data> ::type , typename make_if_<A4, Expr, State, Data> ::type , typename make_if_<A5, Expr, State, Data> ::type , typename make_if_<A6, Expr, State, Data> ::type , typename make_if_<A7, Expr, State, Data> ::type , typename make_if_<A8, Expr, State, Data> ::type , typename make_if_<A9, Expr, State, Data> ::type> type;
            static bool const applied = true;
        };
        template<typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
        struct is_applyable<R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9)>
          : mpl::true_
        {};
        template<typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
        struct is_applyable<R(*)(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9)>
          : mpl::true_
        {};
        template<typename T, typename A>
        struct construct_<proto::expr<T, A, 10>, true>
        {
            typedef proto::expr<T, A, 10> result_type;
            template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
            BOOST_FORCEINLINE
            result_type operator ()(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3 , A4 &a4 , A5 &a5 , A6 &a6 , A7 &a7 , A8 &a8 , A9 &a9) const
            {
                return result_type::make(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9);
            }
        };
        template<typename T, typename A>
        struct construct_<proto::basic_expr<T, A, 10>, true>
        {
            typedef proto::basic_expr<T, A, 10> result_type;
            template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
            BOOST_FORCEINLINE
            result_type operator ()(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3 , A4 &a4 , A5 &a5 , A6 &a6 , A7 &a7 , A8 &a8 , A9 &a9) const
            {
                return result_type::make(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9);
            }
        };
        template<typename Type , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
        BOOST_FORCEINLINE
        Type construct(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3 , A4 &a4 , A5 &a5 , A6 &a6 , A7 &a7 , A8 &a8 , A9 &a9)
        {
            return construct_<Type>()(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9);
        }
    } 
    
    
    
    
    template<typename Object , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
    struct make<Object(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9)>
      : transform<make<Object(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            
            typedef typename detail::make_if_<Object, Expr, State, Data>::type result_type;
            
            
            
            
            
            
            
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                proto::detail::ignore_unused(e);
                proto::detail::ignore_unused(s);
                proto::detail::ignore_unused(d);
                return detail::construct<result_type>(detail::as_lvalue( typename when<_, A0>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A1>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A2>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A3>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A4>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A5>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A6>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A7>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A8>::template impl<Expr, State, Data>()(e, s, d) ) , detail::as_lvalue( typename when<_, A9>::template impl<Expr, State, Data>()(e, s, d) ));
            }
        };
    };
    
    
    
    
    template<typename Object , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
    struct make<Object(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9...)>
      : transform<make<Object(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9...)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : make<
                typename detail::expand_pattern<
                    proto::arity_of<Expr>::value
                  , A9
                  , detail::expand_pattern_rest_9<
                        Object
                        , A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8
                    >
                >::type
            >::template impl<Expr, State, Data>
        {};
    };

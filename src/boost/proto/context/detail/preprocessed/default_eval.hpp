    ///////////////////////////////////////////////////////////////////////////////
    /// \file default_eval.hpp
    /// Contains specializations of the default_eval\<\> class template.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
    template<typename Expr, typename Context>
    struct default_eval<Expr, Context, proto::tag::function, 3>
    {
        typedef
            typename proto::detail::result_of_fixup<
                typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 0>::type >::type , Context >::type
            >::type
        function_type;
        typedef
            typename BOOST_PROTO_RESULT_OF<
                function_type(typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 1>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 2>::type >::type , Context >::type)
            >::type
        result_type;
        result_type operator ()(Expr &expr, Context &context) const
        {
            return this->invoke(expr, context, is_member_function_pointer<function_type>());
        }
    private:
        result_type invoke(Expr &expr, Context &context, mpl::false_) const
        {
            return proto::eval(proto::child_c< 0>( expr), context)(
                proto::eval(proto::child_c< 1>( expr), context) , proto::eval(proto::child_c< 2>( expr), context)
            );
        }
        result_type invoke(Expr &expr, Context &context, mpl::true_) const
        {
            BOOST_PROTO_USE_GET_POINTER();
            typedef typename detail::class_member_traits<function_type>::class_type class_type;
            return (
                BOOST_PROTO_GET_POINTER(class_type, (proto::eval(proto::child_c< 1>( expr), context))) ->*
                proto::eval(proto::child_c< 0>( expr), context)
            )(proto::eval(proto::child_c< 2>( expr), context));
        }
    };
    template<typename Expr, typename Context>
    struct default_eval<Expr, Context, proto::tag::function, 4>
    {
        typedef
            typename proto::detail::result_of_fixup<
                typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 0>::type >::type , Context >::type
            >::type
        function_type;
        typedef
            typename BOOST_PROTO_RESULT_OF<
                function_type(typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 1>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 2>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 3>::type >::type , Context >::type)
            >::type
        result_type;
        result_type operator ()(Expr &expr, Context &context) const
        {
            return this->invoke(expr, context, is_member_function_pointer<function_type>());
        }
    private:
        result_type invoke(Expr &expr, Context &context, mpl::false_) const
        {
            return proto::eval(proto::child_c< 0>( expr), context)(
                proto::eval(proto::child_c< 1>( expr), context) , proto::eval(proto::child_c< 2>( expr), context) , proto::eval(proto::child_c< 3>( expr), context)
            );
        }
        result_type invoke(Expr &expr, Context &context, mpl::true_) const
        {
            BOOST_PROTO_USE_GET_POINTER();
            typedef typename detail::class_member_traits<function_type>::class_type class_type;
            return (
                BOOST_PROTO_GET_POINTER(class_type, (proto::eval(proto::child_c< 1>( expr), context))) ->*
                proto::eval(proto::child_c< 0>( expr), context)
            )(proto::eval(proto::child_c< 2>( expr), context) , proto::eval(proto::child_c< 3>( expr), context));
        }
    };
    template<typename Expr, typename Context>
    struct default_eval<Expr, Context, proto::tag::function, 5>
    {
        typedef
            typename proto::detail::result_of_fixup<
                typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 0>::type >::type , Context >::type
            >::type
        function_type;
        typedef
            typename BOOST_PROTO_RESULT_OF<
                function_type(typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 1>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 2>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 3>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 4>::type >::type , Context >::type)
            >::type
        result_type;
        result_type operator ()(Expr &expr, Context &context) const
        {
            return this->invoke(expr, context, is_member_function_pointer<function_type>());
        }
    private:
        result_type invoke(Expr &expr, Context &context, mpl::false_) const
        {
            return proto::eval(proto::child_c< 0>( expr), context)(
                proto::eval(proto::child_c< 1>( expr), context) , proto::eval(proto::child_c< 2>( expr), context) , proto::eval(proto::child_c< 3>( expr), context) , proto::eval(proto::child_c< 4>( expr), context)
            );
        }
        result_type invoke(Expr &expr, Context &context, mpl::true_) const
        {
            BOOST_PROTO_USE_GET_POINTER();
            typedef typename detail::class_member_traits<function_type>::class_type class_type;
            return (
                BOOST_PROTO_GET_POINTER(class_type, (proto::eval(proto::child_c< 1>( expr), context))) ->*
                proto::eval(proto::child_c< 0>( expr), context)
            )(proto::eval(proto::child_c< 2>( expr), context) , proto::eval(proto::child_c< 3>( expr), context) , proto::eval(proto::child_c< 4>( expr), context));
        }
    };
    template<typename Expr, typename Context>
    struct default_eval<Expr, Context, proto::tag::function, 6>
    {
        typedef
            typename proto::detail::result_of_fixup<
                typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 0>::type >::type , Context >::type
            >::type
        function_type;
        typedef
            typename BOOST_PROTO_RESULT_OF<
                function_type(typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 1>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 2>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 3>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 4>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 5>::type >::type , Context >::type)
            >::type
        result_type;
        result_type operator ()(Expr &expr, Context &context) const
        {
            return this->invoke(expr, context, is_member_function_pointer<function_type>());
        }
    private:
        result_type invoke(Expr &expr, Context &context, mpl::false_) const
        {
            return proto::eval(proto::child_c< 0>( expr), context)(
                proto::eval(proto::child_c< 1>( expr), context) , proto::eval(proto::child_c< 2>( expr), context) , proto::eval(proto::child_c< 3>( expr), context) , proto::eval(proto::child_c< 4>( expr), context) , proto::eval(proto::child_c< 5>( expr), context)
            );
        }
        result_type invoke(Expr &expr, Context &context, mpl::true_) const
        {
            BOOST_PROTO_USE_GET_POINTER();
            typedef typename detail::class_member_traits<function_type>::class_type class_type;
            return (
                BOOST_PROTO_GET_POINTER(class_type, (proto::eval(proto::child_c< 1>( expr), context))) ->*
                proto::eval(proto::child_c< 0>( expr), context)
            )(proto::eval(proto::child_c< 2>( expr), context) , proto::eval(proto::child_c< 3>( expr), context) , proto::eval(proto::child_c< 4>( expr), context) , proto::eval(proto::child_c< 5>( expr), context));
        }
    };
    template<typename Expr, typename Context>
    struct default_eval<Expr, Context, proto::tag::function, 7>
    {
        typedef
            typename proto::detail::result_of_fixup<
                typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 0>::type >::type , Context >::type
            >::type
        function_type;
        typedef
            typename BOOST_PROTO_RESULT_OF<
                function_type(typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 1>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 2>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 3>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 4>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 5>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 6>::type >::type , Context >::type)
            >::type
        result_type;
        result_type operator ()(Expr &expr, Context &context) const
        {
            return this->invoke(expr, context, is_member_function_pointer<function_type>());
        }
    private:
        result_type invoke(Expr &expr, Context &context, mpl::false_) const
        {
            return proto::eval(proto::child_c< 0>( expr), context)(
                proto::eval(proto::child_c< 1>( expr), context) , proto::eval(proto::child_c< 2>( expr), context) , proto::eval(proto::child_c< 3>( expr), context) , proto::eval(proto::child_c< 4>( expr), context) , proto::eval(proto::child_c< 5>( expr), context) , proto::eval(proto::child_c< 6>( expr), context)
            );
        }
        result_type invoke(Expr &expr, Context &context, mpl::true_) const
        {
            BOOST_PROTO_USE_GET_POINTER();
            typedef typename detail::class_member_traits<function_type>::class_type class_type;
            return (
                BOOST_PROTO_GET_POINTER(class_type, (proto::eval(proto::child_c< 1>( expr), context))) ->*
                proto::eval(proto::child_c< 0>( expr), context)
            )(proto::eval(proto::child_c< 2>( expr), context) , proto::eval(proto::child_c< 3>( expr), context) , proto::eval(proto::child_c< 4>( expr), context) , proto::eval(proto::child_c< 5>( expr), context) , proto::eval(proto::child_c< 6>( expr), context));
        }
    };
    template<typename Expr, typename Context>
    struct default_eval<Expr, Context, proto::tag::function, 8>
    {
        typedef
            typename proto::detail::result_of_fixup<
                typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 0>::type >::type , Context >::type
            >::type
        function_type;
        typedef
            typename BOOST_PROTO_RESULT_OF<
                function_type(typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 1>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 2>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 3>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 4>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 5>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 6>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 7>::type >::type , Context >::type)
            >::type
        result_type;
        result_type operator ()(Expr &expr, Context &context) const
        {
            return this->invoke(expr, context, is_member_function_pointer<function_type>());
        }
    private:
        result_type invoke(Expr &expr, Context &context, mpl::false_) const
        {
            return proto::eval(proto::child_c< 0>( expr), context)(
                proto::eval(proto::child_c< 1>( expr), context) , proto::eval(proto::child_c< 2>( expr), context) , proto::eval(proto::child_c< 3>( expr), context) , proto::eval(proto::child_c< 4>( expr), context) , proto::eval(proto::child_c< 5>( expr), context) , proto::eval(proto::child_c< 6>( expr), context) , proto::eval(proto::child_c< 7>( expr), context)
            );
        }
        result_type invoke(Expr &expr, Context &context, mpl::true_) const
        {
            BOOST_PROTO_USE_GET_POINTER();
            typedef typename detail::class_member_traits<function_type>::class_type class_type;
            return (
                BOOST_PROTO_GET_POINTER(class_type, (proto::eval(proto::child_c< 1>( expr), context))) ->*
                proto::eval(proto::child_c< 0>( expr), context)
            )(proto::eval(proto::child_c< 2>( expr), context) , proto::eval(proto::child_c< 3>( expr), context) , proto::eval(proto::child_c< 4>( expr), context) , proto::eval(proto::child_c< 5>( expr), context) , proto::eval(proto::child_c< 6>( expr), context) , proto::eval(proto::child_c< 7>( expr), context));
        }
    };
    template<typename Expr, typename Context>
    struct default_eval<Expr, Context, proto::tag::function, 9>
    {
        typedef
            typename proto::detail::result_of_fixup<
                typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 0>::type >::type , Context >::type
            >::type
        function_type;
        typedef
            typename BOOST_PROTO_RESULT_OF<
                function_type(typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 1>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 2>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 3>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 4>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 5>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 6>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 7>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 8>::type >::type , Context >::type)
            >::type
        result_type;
        result_type operator ()(Expr &expr, Context &context) const
        {
            return this->invoke(expr, context, is_member_function_pointer<function_type>());
        }
    private:
        result_type invoke(Expr &expr, Context &context, mpl::false_) const
        {
            return proto::eval(proto::child_c< 0>( expr), context)(
                proto::eval(proto::child_c< 1>( expr), context) , proto::eval(proto::child_c< 2>( expr), context) , proto::eval(proto::child_c< 3>( expr), context) , proto::eval(proto::child_c< 4>( expr), context) , proto::eval(proto::child_c< 5>( expr), context) , proto::eval(proto::child_c< 6>( expr), context) , proto::eval(proto::child_c< 7>( expr), context) , proto::eval(proto::child_c< 8>( expr), context)
            );
        }
        result_type invoke(Expr &expr, Context &context, mpl::true_) const
        {
            BOOST_PROTO_USE_GET_POINTER();
            typedef typename detail::class_member_traits<function_type>::class_type class_type;
            return (
                BOOST_PROTO_GET_POINTER(class_type, (proto::eval(proto::child_c< 1>( expr), context))) ->*
                proto::eval(proto::child_c< 0>( expr), context)
            )(proto::eval(proto::child_c< 2>( expr), context) , proto::eval(proto::child_c< 3>( expr), context) , proto::eval(proto::child_c< 4>( expr), context) , proto::eval(proto::child_c< 5>( expr), context) , proto::eval(proto::child_c< 6>( expr), context) , proto::eval(proto::child_c< 7>( expr), context) , proto::eval(proto::child_c< 8>( expr), context));
        }
    };
    template<typename Expr, typename Context>
    struct default_eval<Expr, Context, proto::tag::function, 10>
    {
        typedef
            typename proto::detail::result_of_fixup<
                typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 0>::type >::type , Context >::type
            >::type
        function_type;
        typedef
            typename BOOST_PROTO_RESULT_OF<
                function_type(typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 1>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 2>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 3>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 4>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 5>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 6>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 7>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 8>::type >::type , Context >::type , typename proto::result_of::eval< typename remove_reference< typename proto::result_of::child_c< Expr, 9>::type >::type , Context >::type)
            >::type
        result_type;
        result_type operator ()(Expr &expr, Context &context) const
        {
            return this->invoke(expr, context, is_member_function_pointer<function_type>());
        }
    private:
        result_type invoke(Expr &expr, Context &context, mpl::false_) const
        {
            return proto::eval(proto::child_c< 0>( expr), context)(
                proto::eval(proto::child_c< 1>( expr), context) , proto::eval(proto::child_c< 2>( expr), context) , proto::eval(proto::child_c< 3>( expr), context) , proto::eval(proto::child_c< 4>( expr), context) , proto::eval(proto::child_c< 5>( expr), context) , proto::eval(proto::child_c< 6>( expr), context) , proto::eval(proto::child_c< 7>( expr), context) , proto::eval(proto::child_c< 8>( expr), context) , proto::eval(proto::child_c< 9>( expr), context)
            );
        }
        result_type invoke(Expr &expr, Context &context, mpl::true_) const
        {
            BOOST_PROTO_USE_GET_POINTER();
            typedef typename detail::class_member_traits<function_type>::class_type class_type;
            return (
                BOOST_PROTO_GET_POINTER(class_type, (proto::eval(proto::child_c< 1>( expr), context))) ->*
                proto::eval(proto::child_c< 0>( expr), context)
            )(proto::eval(proto::child_c< 2>( expr), context) , proto::eval(proto::child_c< 3>( expr), context) , proto::eval(proto::child_c< 4>( expr), context) , proto::eval(proto::child_c< 5>( expr), context) , proto::eval(proto::child_c< 6>( expr), context) , proto::eval(proto::child_c< 7>( expr), context) , proto::eval(proto::child_c< 8>( expr), context) , proto::eval(proto::child_c< 9>( expr), context));
        }
    };

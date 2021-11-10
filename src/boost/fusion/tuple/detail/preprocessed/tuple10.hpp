/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    This is an auto-generated file. Do not edit!
==============================================================================*/
namespace boost { namespace fusion
{
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9>
    struct tuple : vector<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9>
    {
        typedef vector<
            T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9>
        base_type;
        BOOST_FUSION_GPU_ENABLED tuple()
            : base_type() {}
        BOOST_FUSION_GPU_ENABLED tuple(tuple const& rhs)
            : base_type(static_cast<base_type const&>(rhs)) {}
        template <typename U1, typename U2>
        BOOST_FUSION_GPU_ENABLED
        tuple(std::pair<U1, U2> const& rhs)
            : base_type(rhs) {}
    BOOST_FUSION_GPU_ENABLED
    explicit
    tuple(typename detail::call_param<T0 >::type arg0)
        : base_type(arg0) {}
    template <typename U0>
    BOOST_FUSION_GPU_ENABLED
    tuple(tuple<U0> const& rhs)
        : base_type(rhs) {}
    template <typename U0>
    BOOST_FUSION_GPU_ENABLED
    tuple& operator=(tuple<U0> const& rhs)
    {
        base_type::operator=(rhs);
        return *this;
    }
    BOOST_FUSION_GPU_ENABLED
    tuple(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1)
        : base_type(arg0 , arg1) {}
    template <typename U0 , typename U1>
    BOOST_FUSION_GPU_ENABLED
    tuple(tuple<U0 , U1> const& rhs)
        : base_type(rhs) {}
    template <typename U0 , typename U1>
    BOOST_FUSION_GPU_ENABLED
    tuple& operator=(tuple<U0 , U1> const& rhs)
    {
        base_type::operator=(rhs);
        return *this;
    }
    BOOST_FUSION_GPU_ENABLED
    tuple(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2)
        : base_type(arg0 , arg1 , arg2) {}
    template <typename U0 , typename U1 , typename U2>
    BOOST_FUSION_GPU_ENABLED
    tuple(tuple<U0 , U1 , U2> const& rhs)
        : base_type(rhs) {}
    template <typename U0 , typename U1 , typename U2>
    BOOST_FUSION_GPU_ENABLED
    tuple& operator=(tuple<U0 , U1 , U2> const& rhs)
    {
        base_type::operator=(rhs);
        return *this;
    }
    BOOST_FUSION_GPU_ENABLED
    tuple(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3)
        : base_type(arg0 , arg1 , arg2 , arg3) {}
    template <typename U0 , typename U1 , typename U2 , typename U3>
    BOOST_FUSION_GPU_ENABLED
    tuple(tuple<U0 , U1 , U2 , U3> const& rhs)
        : base_type(rhs) {}
    template <typename U0 , typename U1 , typename U2 , typename U3>
    BOOST_FUSION_GPU_ENABLED
    tuple& operator=(tuple<U0 , U1 , U2 , U3> const& rhs)
    {
        base_type::operator=(rhs);
        return *this;
    }
    BOOST_FUSION_GPU_ENABLED
    tuple(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4)
        : base_type(arg0 , arg1 , arg2 , arg3 , arg4) {}
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4>
    BOOST_FUSION_GPU_ENABLED
    tuple(tuple<U0 , U1 , U2 , U3 , U4> const& rhs)
        : base_type(rhs) {}
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4>
    BOOST_FUSION_GPU_ENABLED
    tuple& operator=(tuple<U0 , U1 , U2 , U3 , U4> const& rhs)
    {
        base_type::operator=(rhs);
        return *this;
    }
    BOOST_FUSION_GPU_ENABLED
    tuple(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5)
        : base_type(arg0 , arg1 , arg2 , arg3 , arg4 , arg5) {}
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5>
    BOOST_FUSION_GPU_ENABLED
    tuple(tuple<U0 , U1 , U2 , U3 , U4 , U5> const& rhs)
        : base_type(rhs) {}
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5>
    BOOST_FUSION_GPU_ENABLED
    tuple& operator=(tuple<U0 , U1 , U2 , U3 , U4 , U5> const& rhs)
    {
        base_type::operator=(rhs);
        return *this;
    }
    BOOST_FUSION_GPU_ENABLED
    tuple(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6)
        : base_type(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6) {}
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6>
    BOOST_FUSION_GPU_ENABLED
    tuple(tuple<U0 , U1 , U2 , U3 , U4 , U5 , U6> const& rhs)
        : base_type(rhs) {}
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6>
    BOOST_FUSION_GPU_ENABLED
    tuple& operator=(tuple<U0 , U1 , U2 , U3 , U4 , U5 , U6> const& rhs)
    {
        base_type::operator=(rhs);
        return *this;
    }
    BOOST_FUSION_GPU_ENABLED
    tuple(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7)
        : base_type(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7) {}
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7>
    BOOST_FUSION_GPU_ENABLED
    tuple(tuple<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7> const& rhs)
        : base_type(rhs) {}
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7>
    BOOST_FUSION_GPU_ENABLED
    tuple& operator=(tuple<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7> const& rhs)
    {
        base_type::operator=(rhs);
        return *this;
    }
    BOOST_FUSION_GPU_ENABLED
    tuple(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8)
        : base_type(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8) {}
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8>
    BOOST_FUSION_GPU_ENABLED
    tuple(tuple<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8> const& rhs)
        : base_type(rhs) {}
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8>
    BOOST_FUSION_GPU_ENABLED
    tuple& operator=(tuple<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8> const& rhs)
    {
        base_type::operator=(rhs);
        return *this;
    }
    BOOST_FUSION_GPU_ENABLED
    tuple(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9)
        : base_type(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9) {}
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9>
    BOOST_FUSION_GPU_ENABLED
    tuple(tuple<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9> const& rhs)
        : base_type(rhs) {}
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9>
    BOOST_FUSION_GPU_ENABLED
    tuple& operator=(tuple<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9> const& rhs)
    {
        base_type::operator=(rhs);
        return *this;
    }
        template <typename T>
        BOOST_FUSION_GPU_ENABLED
        tuple& operator=(T const& rhs)
        {
            base_type::operator=(rhs);
            return *this;
        }
        BOOST_FUSION_GPU_ENABLED
        tuple& operator=(tuple const& rhs)
        {
            base_type::operator=(static_cast<base_type const&>(rhs));
            return *this;
        }
        template <typename U1, typename U2>
        BOOST_FUSION_GPU_ENABLED
        tuple& operator=(std::pair<U1, U2> const& rhs)
        {
            base_type::operator=(rhs);
            return *this;
        }
    };
    template <typename Tuple>
    struct tuple_size : result_of::size<Tuple> {};
    template <int N, typename Tuple>
    struct tuple_element : result_of::value_at_c<Tuple, N> {};
    template <int N, typename Tuple>
    BOOST_FUSION_GPU_ENABLED
    inline typename
        lazy_disable_if<
            is_const<Tuple>
          , result_of::at_c<Tuple, N>
        >::type
    get(Tuple& tup)
    {
        return at_c<N>(tup);
    }
    template <int N, typename Tuple>
    BOOST_FUSION_GPU_ENABLED
    inline typename result_of::at_c<Tuple const, N>::type
    get(Tuple const& tup)
    {
        return at_c<N>(tup);
    }
}}

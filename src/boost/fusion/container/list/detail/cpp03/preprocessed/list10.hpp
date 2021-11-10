/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    This is an auto-generated file. Do not edit!
==============================================================================*/
namespace boost { namespace fusion
{
    struct nil_;
    struct void_;
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9>
    struct list
        : detail::list_to_cons<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9>::type
    {
    private:
        typedef
            detail::list_to_cons<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9>
        list_to_cons;
        typedef typename list_to_cons::type inherited_type;
    public:
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        list()
            : inherited_type() {}
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        list(list<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9> const& rhs)
            : inherited_type(rhs) {}
        template <typename Sequence>
        BOOST_FUSION_GPU_ENABLED
        list(Sequence const& rhs
            , typename enable_if<traits::is_sequence<Sequence>, detail::enabler_>::type = detail::enabler)
            : inherited_type(rhs) {}
        
        
        
        
        
        
        
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    explicit
    list(typename detail::call_param<T0 >::type arg0)
        : inherited_type(list_to_cons::call(arg0))
    {}
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    list(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1)
        : inherited_type(list_to_cons::call(arg0 , arg1))
    {}
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    list(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2)
        : inherited_type(list_to_cons::call(arg0 , arg1 , arg2))
    {}
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    list(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3)
        : inherited_type(list_to_cons::call(arg0 , arg1 , arg2 , arg3))
    {}
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    list(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4)
        : inherited_type(list_to_cons::call(arg0 , arg1 , arg2 , arg3 , arg4))
    {}
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    list(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5)
        : inherited_type(list_to_cons::call(arg0 , arg1 , arg2 , arg3 , arg4 , arg5))
    {}
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    list(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6)
        : inherited_type(list_to_cons::call(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6))
    {}
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    list(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7)
        : inherited_type(list_to_cons::call(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7))
    {}
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    list(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8)
        : inherited_type(list_to_cons::call(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8))
    {}
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    list(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9)
        : inherited_type(list_to_cons::call(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9))
    {}
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        list&
        operator=(list<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9> const& rhs)
        {
            inherited_type::operator=(rhs);
            return *this;
        }
        template <typename Sequence>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename boost::enable_if<traits::is_sequence<Sequence>, list&>::type
        operator=(Sequence const& rhs)
        {
            inherited_type::operator=(rhs);
            return *this;
        }
    };
}}

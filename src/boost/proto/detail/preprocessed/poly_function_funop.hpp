    ///////////////////////////////////////////////////////////////////////////////
    // poly_function_funop.hpp
    // Contains overloads of poly_function\<\>::operator()
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
    template<typename This , typename A0>
    struct result<This(A0)>
      : Derived::template impl<
            typename normalize_arg<A0 >::type
        >
    {
        typedef typename result::result_type type;
    };
    template<typename A0>
    typename result<
        Derived const(
            A0 const &
        )
    >::type
    operator ()(A0 const &a0) const
    {
        result<
            Derived const(
                A0 const &
            )
        > impl;
        return impl(static_cast<typename normalize_arg<A0 const &> ::reference>(a0));
    }
    template<typename This , typename A0 , typename A1>
    struct result<This(A0 , A1)>
      : Derived::template impl<
            typename normalize_arg<A0 >::type , typename normalize_arg<A1 >::type
        >
    {
        typedef typename result::result_type type;
    };
    template<typename A0 , typename A1>
    typename result<
        Derived const(
            A0 const & , A1 const &
        )
    >::type
    operator ()(A0 const &a0 , A1 const &a1) const
    {
        result<
            Derived const(
                A0 const & , A1 const &
            )
        > impl;
        return impl(static_cast<typename normalize_arg<A0 const &> ::reference>(a0) , static_cast<typename normalize_arg<A1 const &> ::reference>(a1));
    }
    template<typename This , typename A0 , typename A1 , typename A2>
    struct result<This(A0 , A1 , A2)>
      : Derived::template impl<
            typename normalize_arg<A0 >::type , typename normalize_arg<A1 >::type , typename normalize_arg<A2 >::type
        >
    {
        typedef typename result::result_type type;
    };
    template<typename A0 , typename A1 , typename A2>
    typename result<
        Derived const(
            A0 const & , A1 const & , A2 const &
        )
    >::type
    operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2) const
    {
        result<
            Derived const(
                A0 const & , A1 const & , A2 const &
            )
        > impl;
        return impl(static_cast<typename normalize_arg<A0 const &> ::reference>(a0) , static_cast<typename normalize_arg<A1 const &> ::reference>(a1) , static_cast<typename normalize_arg<A2 const &> ::reference>(a2));
    }
    template<typename This , typename A0 , typename A1 , typename A2 , typename A3>
    struct result<This(A0 , A1 , A2 , A3)>
      : Derived::template impl<
            typename normalize_arg<A0 >::type , typename normalize_arg<A1 >::type , typename normalize_arg<A2 >::type , typename normalize_arg<A3 >::type
        >
    {
        typedef typename result::result_type type;
    };
    template<typename A0 , typename A1 , typename A2 , typename A3>
    typename result<
        Derived const(
            A0 const & , A1 const & , A2 const & , A3 const &
        )
    >::type
    operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3) const
    {
        result<
            Derived const(
                A0 const & , A1 const & , A2 const & , A3 const &
            )
        > impl;
        return impl(static_cast<typename normalize_arg<A0 const &> ::reference>(a0) , static_cast<typename normalize_arg<A1 const &> ::reference>(a1) , static_cast<typename normalize_arg<A2 const &> ::reference>(a2) , static_cast<typename normalize_arg<A3 const &> ::reference>(a3));
    }
    template<typename This , typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
    struct result<This(A0 , A1 , A2 , A3 , A4)>
      : Derived::template impl<
            typename normalize_arg<A0 >::type , typename normalize_arg<A1 >::type , typename normalize_arg<A2 >::type , typename normalize_arg<A3 >::type , typename normalize_arg<A4 >::type
        >
    {
        typedef typename result::result_type type;
    };
    template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
    typename result<
        Derived const(
            A0 const & , A1 const & , A2 const & , A3 const & , A4 const &
        )
    >::type
    operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4) const
    {
        result<
            Derived const(
                A0 const & , A1 const & , A2 const & , A3 const & , A4 const &
            )
        > impl;
        return impl(static_cast<typename normalize_arg<A0 const &> ::reference>(a0) , static_cast<typename normalize_arg<A1 const &> ::reference>(a1) , static_cast<typename normalize_arg<A2 const &> ::reference>(a2) , static_cast<typename normalize_arg<A3 const &> ::reference>(a3) , static_cast<typename normalize_arg<A4 const &> ::reference>(a4));
    }
    template<typename This , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
    struct result<This(A0 , A1 , A2 , A3 , A4 , A5)>
      : Derived::template impl<
            typename normalize_arg<A0 >::type , typename normalize_arg<A1 >::type , typename normalize_arg<A2 >::type , typename normalize_arg<A3 >::type , typename normalize_arg<A4 >::type , typename normalize_arg<A5 >::type
        >
    {
        typedef typename result::result_type type;
    };
    template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
    typename result<
        Derived const(
            A0 const & , A1 const & , A2 const & , A3 const & , A4 const & , A5 const &
        )
    >::type
    operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5) const
    {
        result<
            Derived const(
                A0 const & , A1 const & , A2 const & , A3 const & , A4 const & , A5 const &
            )
        > impl;
        return impl(static_cast<typename normalize_arg<A0 const &> ::reference>(a0) , static_cast<typename normalize_arg<A1 const &> ::reference>(a1) , static_cast<typename normalize_arg<A2 const &> ::reference>(a2) , static_cast<typename normalize_arg<A3 const &> ::reference>(a3) , static_cast<typename normalize_arg<A4 const &> ::reference>(a4) , static_cast<typename normalize_arg<A5 const &> ::reference>(a5));
    }
    template<typename This , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
    struct result<This(A0 , A1 , A2 , A3 , A4 , A5 , A6)>
      : Derived::template impl<
            typename normalize_arg<A0 >::type , typename normalize_arg<A1 >::type , typename normalize_arg<A2 >::type , typename normalize_arg<A3 >::type , typename normalize_arg<A4 >::type , typename normalize_arg<A5 >::type , typename normalize_arg<A6 >::type
        >
    {
        typedef typename result::result_type type;
    };
    template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
    typename result<
        Derived const(
            A0 const & , A1 const & , A2 const & , A3 const & , A4 const & , A5 const & , A6 const &
        )
    >::type
    operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6) const
    {
        result<
            Derived const(
                A0 const & , A1 const & , A2 const & , A3 const & , A4 const & , A5 const & , A6 const &
            )
        > impl;
        return impl(static_cast<typename normalize_arg<A0 const &> ::reference>(a0) , static_cast<typename normalize_arg<A1 const &> ::reference>(a1) , static_cast<typename normalize_arg<A2 const &> ::reference>(a2) , static_cast<typename normalize_arg<A3 const &> ::reference>(a3) , static_cast<typename normalize_arg<A4 const &> ::reference>(a4) , static_cast<typename normalize_arg<A5 const &> ::reference>(a5) , static_cast<typename normalize_arg<A6 const &> ::reference>(a6));
    }
    template<typename This , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
    struct result<This(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7)>
      : Derived::template impl<
            typename normalize_arg<A0 >::type , typename normalize_arg<A1 >::type , typename normalize_arg<A2 >::type , typename normalize_arg<A3 >::type , typename normalize_arg<A4 >::type , typename normalize_arg<A5 >::type , typename normalize_arg<A6 >::type , typename normalize_arg<A7 >::type
        >
    {
        typedef typename result::result_type type;
    };
    template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
    typename result<
        Derived const(
            A0 const & , A1 const & , A2 const & , A3 const & , A4 const & , A5 const & , A6 const & , A7 const &
        )
    >::type
    operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7) const
    {
        result<
            Derived const(
                A0 const & , A1 const & , A2 const & , A3 const & , A4 const & , A5 const & , A6 const & , A7 const &
            )
        > impl;
        return impl(static_cast<typename normalize_arg<A0 const &> ::reference>(a0) , static_cast<typename normalize_arg<A1 const &> ::reference>(a1) , static_cast<typename normalize_arg<A2 const &> ::reference>(a2) , static_cast<typename normalize_arg<A3 const &> ::reference>(a3) , static_cast<typename normalize_arg<A4 const &> ::reference>(a4) , static_cast<typename normalize_arg<A5 const &> ::reference>(a5) , static_cast<typename normalize_arg<A6 const &> ::reference>(a6) , static_cast<typename normalize_arg<A7 const &> ::reference>(a7));
    }
    template<typename This , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
    struct result<This(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8)>
      : Derived::template impl<
            typename normalize_arg<A0 >::type , typename normalize_arg<A1 >::type , typename normalize_arg<A2 >::type , typename normalize_arg<A3 >::type , typename normalize_arg<A4 >::type , typename normalize_arg<A5 >::type , typename normalize_arg<A6 >::type , typename normalize_arg<A7 >::type , typename normalize_arg<A8 >::type
        >
    {
        typedef typename result::result_type type;
    };
    template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
    typename result<
        Derived const(
            A0 const & , A1 const & , A2 const & , A3 const & , A4 const & , A5 const & , A6 const & , A7 const & , A8 const &
        )
    >::type
    operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7 , A8 const &a8) const
    {
        result<
            Derived const(
                A0 const & , A1 const & , A2 const & , A3 const & , A4 const & , A5 const & , A6 const & , A7 const & , A8 const &
            )
        > impl;
        return impl(static_cast<typename normalize_arg<A0 const &> ::reference>(a0) , static_cast<typename normalize_arg<A1 const &> ::reference>(a1) , static_cast<typename normalize_arg<A2 const &> ::reference>(a2) , static_cast<typename normalize_arg<A3 const &> ::reference>(a3) , static_cast<typename normalize_arg<A4 const &> ::reference>(a4) , static_cast<typename normalize_arg<A5 const &> ::reference>(a5) , static_cast<typename normalize_arg<A6 const &> ::reference>(a6) , static_cast<typename normalize_arg<A7 const &> ::reference>(a7) , static_cast<typename normalize_arg<A8 const &> ::reference>(a8));
    }
    template<typename This , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
    struct result<This(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9)>
      : Derived::template impl<
            typename normalize_arg<A0 >::type , typename normalize_arg<A1 >::type , typename normalize_arg<A2 >::type , typename normalize_arg<A3 >::type , typename normalize_arg<A4 >::type , typename normalize_arg<A5 >::type , typename normalize_arg<A6 >::type , typename normalize_arg<A7 >::type , typename normalize_arg<A8 >::type , typename normalize_arg<A9 >::type
        >
    {
        typedef typename result::result_type type;
    };
    template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
    typename result<
        Derived const(
            A0 const & , A1 const & , A2 const & , A3 const & , A4 const & , A5 const & , A6 const & , A7 const & , A8 const & , A9 const &
        )
    >::type
    operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7 , A8 const &a8 , A9 const &a9) const
    {
        result<
            Derived const(
                A0 const & , A1 const & , A2 const & , A3 const & , A4 const & , A5 const & , A6 const & , A7 const & , A8 const & , A9 const &
            )
        > impl;
        return impl(static_cast<typename normalize_arg<A0 const &> ::reference>(a0) , static_cast<typename normalize_arg<A1 const &> ::reference>(a1) , static_cast<typename normalize_arg<A2 const &> ::reference>(a2) , static_cast<typename normalize_arg<A3 const &> ::reference>(a3) , static_cast<typename normalize_arg<A4 const &> ::reference>(a4) , static_cast<typename normalize_arg<A5 const &> ::reference>(a5) , static_cast<typename normalize_arg<A6 const &> ::reference>(a6) , static_cast<typename normalize_arg<A7 const &> ::reference>(a7) , static_cast<typename normalize_arg<A8 const &> ::reference>(a8) , static_cast<typename normalize_arg<A9 const &> ::reference>(a9));
    }

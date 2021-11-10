    ///////////////////////////////////////////////////////////////////////////////
    /// \file construct_funop.hpp
    /// Overloads of construct_\<\>::operator().
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
    template<typename A0>
    BOOST_FORCEINLINE
    Type operator ()(A0 &a0) const
    {
        return Type(a0);
    }
    template<typename A0 , typename A1>
    BOOST_FORCEINLINE
    Type operator ()(A0 &a0 , A1 &a1) const
    {
        return Type(a0 , a1);
    }
    template<typename A0 , typename A1 , typename A2>
    BOOST_FORCEINLINE
    Type operator ()(A0 &a0 , A1 &a1 , A2 &a2) const
    {
        return Type(a0 , a1 , a2);
    }
    template<typename A0 , typename A1 , typename A2 , typename A3>
    BOOST_FORCEINLINE
    Type operator ()(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3) const
    {
        return Type(a0 , a1 , a2 , a3);
    }
    template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
    BOOST_FORCEINLINE
    Type operator ()(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3 , A4 &a4) const
    {
        return Type(a0 , a1 , a2 , a3 , a4);
    }
    template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
    BOOST_FORCEINLINE
    Type operator ()(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3 , A4 &a4 , A5 &a5) const
    {
        return Type(a0 , a1 , a2 , a3 , a4 , a5);
    }
    template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
    BOOST_FORCEINLINE
    Type operator ()(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3 , A4 &a4 , A5 &a5 , A6 &a6) const
    {
        return Type(a0 , a1 , a2 , a3 , a4 , a5 , a6);
    }
    template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
    BOOST_FORCEINLINE
    Type operator ()(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3 , A4 &a4 , A5 &a5 , A6 &a6 , A7 &a7) const
    {
        return Type(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7);
    }
    template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
    BOOST_FORCEINLINE
    Type operator ()(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3 , A4 &a4 , A5 &a5 , A6 &a6 , A7 &a7 , A8 &a8) const
    {
        return Type(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8);
    }
    template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
    BOOST_FORCEINLINE
    Type operator ()(A0 &a0 , A1 &a1 , A2 &a2 , A3 &a3 , A4 &a4 , A5 &a5 , A6 &a6 , A7 &a7 , A8 &a8 , A9 &a9) const
    {
        return Type(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9);
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \file when.hpp
    /// Definition of when transform.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R >
    struct when<Grammar, R()>
      : detail::when_impl<Grammar, R, R()>
    {};
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0>
    struct when<Grammar, R(A0)>
      : detail::when_impl<Grammar, R, R(A0)>
    {};
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0>
    struct when<Grammar, R(A0...)>
      : detail::when_impl<Grammar, R, R(A0...)>
    {};
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1>
    struct when<Grammar, R(A0 , A1)>
      : detail::when_impl<Grammar, R, R(A0 , A1)>
    {};
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1>
    struct when<Grammar, R(A0 , A1...)>
      : detail::when_impl<Grammar, R, R(A0 , A1...)>
    {};
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1 , typename A2>
    struct when<Grammar, R(A0 , A1 , A2)>
      : detail::when_impl<Grammar, R, R(A0 , A1 , A2)>
    {};
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1 , typename A2>
    struct when<Grammar, R(A0 , A1 , A2...)>
      : detail::when_impl<Grammar, R, R(A0 , A1 , A2...)>
    {};
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1 , typename A2 , typename A3>
    struct when<Grammar, R(A0 , A1 , A2 , A3)>
      : detail::when_impl<Grammar, R, R(A0 , A1 , A2 , A3)>
    {};
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1 , typename A2 , typename A3>
    struct when<Grammar, R(A0 , A1 , A2 , A3...)>
      : detail::when_impl<Grammar, R, R(A0 , A1 , A2 , A3...)>
    {};
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
    struct when<Grammar, R(A0 , A1 , A2 , A3 , A4)>
      : detail::when_impl<Grammar, R, R(A0 , A1 , A2 , A3 , A4)>
    {};
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
    struct when<Grammar, R(A0 , A1 , A2 , A3 , A4...)>
      : detail::when_impl<Grammar, R, R(A0 , A1 , A2 , A3 , A4...)>
    {};
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
    struct when<Grammar, R(A0 , A1 , A2 , A3 , A4 , A5)>
      : detail::when_impl<Grammar, R, R(A0 , A1 , A2 , A3 , A4 , A5)>
    {};
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
    struct when<Grammar, R(A0 , A1 , A2 , A3 , A4 , A5...)>
      : detail::when_impl<Grammar, R, R(A0 , A1 , A2 , A3 , A4 , A5...)>
    {};
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
    struct when<Grammar, R(A0 , A1 , A2 , A3 , A4 , A5 , A6)>
      : detail::when_impl<Grammar, R, R(A0 , A1 , A2 , A3 , A4 , A5 , A6)>
    {};
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
    struct when<Grammar, R(A0 , A1 , A2 , A3 , A4 , A5 , A6...)>
      : detail::when_impl<Grammar, R, R(A0 , A1 , A2 , A3 , A4 , A5 , A6...)>
    {};
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
    struct when<Grammar, R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7)>
      : detail::when_impl<Grammar, R, R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7)>
    {};
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
    struct when<Grammar, R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7...)>
      : detail::when_impl<Grammar, R, R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7...)>
    {};
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
    struct when<Grammar, R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8)>
      : detail::when_impl<Grammar, R, R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8)>
    {};
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
    struct when<Grammar, R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8...)>
      : detail::when_impl<Grammar, R, R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8...)>
    {};
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
    struct when<Grammar, R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9)>
      : detail::when_impl<Grammar, R, R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9)>
    {};
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
    struct when<Grammar, R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9...)>
      : detail::when_impl<Grammar, R, R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9...)>
    {};

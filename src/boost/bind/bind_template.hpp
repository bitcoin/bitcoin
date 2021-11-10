//
//  bind/bind_template.hpp
//
//  Do not include this header directly.
//
//  Copyright (c) 2001-2004 Peter Dimov and Multi Media Ltd.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/bind/bind.html for documentation.
//

    typedef typename result_traits<R, F>::type result_type;

    result_type operator()()
    {
        list0 a;
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

    result_type operator()() const
    {
        list0 a;
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

    template<class A1> result_type operator()(A1 & a1)
    {
        list1<A1 &> a(a1);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

    template<class A1> result_type operator()(A1 & a1) const
    {
        list1<A1 &> a(a1);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING) \
 && !BOOST_WORKAROUND(__EDG_VERSION__, <= 238)

    template<class A1> result_type operator()(A1 const & a1)
    {
        list1<A1 const &> a(a1);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

    template<class A1> result_type operator()(A1 const & a1) const
    {
        list1<A1 const &> a(a1);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

#endif

    template<class A1, class A2> result_type operator()(A1 & a1, A2 & a2)
    {
        list2<A1 &, A2 &> a(a1, a2);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

    template<class A1, class A2> result_type operator()(A1 & a1, A2 & a2) const
    {
        list2<A1 &, A2 &> a(a1, a2);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING) \
 && !BOOST_WORKAROUND(__EDG_VERSION__, <= 238)

    template<class A1, class A2> result_type operator()(A1 const & a1, A2 & a2)
    {
        list2<A1 const &, A2 &> a(a1, a2);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

    template<class A1, class A2> result_type operator()(A1 const & a1, A2 & a2) const
    {
        list2<A1 const &, A2 &> a(a1, a2);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }


    template<class A1, class A2> result_type operator()(A1 & a1, A2 const & a2)
    {
        list2<A1 &, A2 const &> a(a1, a2);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

    template<class A1, class A2> result_type operator()(A1 & a1, A2 const & a2) const
    {
        list2<A1 &, A2 const &> a(a1, a2);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }


    template<class A1, class A2> result_type operator()(A1 const & a1, A2 const & a2)
    {
        list2<A1 const &, A2 const &> a(a1, a2);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

    template<class A1, class A2> result_type operator()(A1 const & a1, A2 const & a2) const
    {
        list2<A1 const &, A2 const &> a(a1, a2);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

#endif

    template<class A1, class A2, class A3> result_type operator()(A1 & a1, A2 & a2, A3 & a3)
    {
        list3<A1 &, A2 &, A3 &> a(a1, a2, a3);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

    template<class A1, class A2, class A3> result_type operator()(A1 & a1, A2 & a2, A3 & a3) const
    {
        list3<A1 &, A2 &, A3 &> a(a1, a2, a3);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING) \
 && !BOOST_WORKAROUND(__EDG_VERSION__, <= 238)

    template<class A1, class A2, class A3> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3)
    {
        list3<A1 const &, A2 const &, A3 const &> a(a1, a2, a3);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

    template<class A1, class A2, class A3> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3) const
    {
        list3<A1 const &, A2 const &, A3 const &> a(a1, a2, a3);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

#endif

    template<class A1, class A2, class A3, class A4> result_type operator()(A1 & a1, A2 & a2, A3 & a3, A4 & a4)
    {
        list4<A1 &, A2 &, A3 &, A4 &> a(a1, a2, a3, a4);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

    template<class A1, class A2, class A3, class A4> result_type operator()(A1 & a1, A2 & a2, A3 & a3, A4 & a4) const
    {
        list4<A1 &, A2 &, A3 &, A4 &> a(a1, a2, a3, a4);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING) \
 && !BOOST_WORKAROUND(__EDG_VERSION__, <= 238)

    template<class A1, class A2, class A3, class A4> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4)
    {
        list4<A1 const &, A2 const &, A3 const &, A4 const &> a(a1, a2, a3, a4);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

    template<class A1, class A2, class A3, class A4> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4) const
    {
        list4<A1 const &, A2 const &, A3 const &, A4 const &> a(a1, a2, a3, a4);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

#endif

    template<class A1, class A2, class A3, class A4, class A5> result_type operator()(A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5)
    {
        list5<A1 &, A2 &, A3 &, A4 &, A5 &> a(a1, a2, a3, a4, a5);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

    template<class A1, class A2, class A3, class A4, class A5> result_type operator()(A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5) const
    {
        list5<A1 &, A2 &, A3 &, A4 &, A5 &> a(a1, a2, a3, a4, a5);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING) \
 && !BOOST_WORKAROUND(__EDG_VERSION__, <= 238)

    template<class A1, class A2, class A3, class A4, class A5> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5)
    {
        list5<A1 const &, A2 const &, A3 const &, A4 const &, A5 const &> a(a1, a2, a3, a4, a5);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

    template<class A1, class A2, class A3, class A4, class A5> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5) const
    {
        list5<A1 const &, A2 const &, A3 const &, A4 const &, A5 const &> a(a1, a2, a3, a4, a5);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

#endif

    template<class A1, class A2, class A3, class A4, class A5, class A6> result_type operator()(A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5, A6 & a6)
    {
        list6<A1 &, A2 &, A3 &, A4 &, A5 &, A6 &> a(a1, a2, a3, a4, a5, a6);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

    template<class A1, class A2, class A3, class A4, class A5, class A6> result_type operator()(A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5, A6 & a6) const
    {
        list6<A1 &, A2 &, A3 &, A4 &, A5 &, A6 &> a(a1, a2, a3, a4, a5, a6);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING) \
 && !BOOST_WORKAROUND(__EDG_VERSION__, <= 238)

    template<class A1, class A2, class A3, class A4, class A5, class A6> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6)
    {
        list6<A1 const &, A2 const &, A3 const &, A4 const &, A5 const &, A6 const &> a(a1, a2, a3, a4, a5, a6);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

    template<class A1, class A2, class A3, class A4, class A5, class A6> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6) const
    {
        list6<A1 const &, A2 const &, A3 const &, A4 const &, A5 const &, A6 const &> a(a1, a2, a3, a4, a5, a6);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

#endif

    template<class A1, class A2, class A3, class A4, class A5, class A6, class A7> result_type operator()(A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5, A6 & a6, A7 & a7)
    {
        list7<A1 &, A2 &, A3 &, A4 &, A5 &, A6 &, A7 &> a(a1, a2, a3, a4, a5, a6, a7);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

    template<class A1, class A2, class A3, class A4, class A5, class A6, class A7> result_type operator()(A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5, A6 & a6, A7 & a7) const
    {
        list7<A1 &, A2 &, A3 &, A4 &, A5 &, A6 &, A7 &> a(a1, a2, a3, a4, a5, a6, a7);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING) \
 && !BOOST_WORKAROUND(__EDG_VERSION__, <= 238)

    template<class A1, class A2, class A3, class A4, class A5, class A6, class A7> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6, A7 const & a7)
    {
        list7<A1 const &, A2 const &, A3 const &, A4 const &, A5 const &, A6 const &, A7 const &> a(a1, a2, a3, a4, a5, a6, a7);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

    template<class A1, class A2, class A3, class A4, class A5, class A6, class A7> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6, A7 const & a7) const
    {
        list7<A1 const &, A2 const &, A3 const &, A4 const &, A5 const &, A6 const &, A7 const &> a(a1, a2, a3, a4, a5, a6, a7);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

#endif

    template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> result_type operator()(A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5, A6 & a6, A7 & a7, A8 & a8)
    {
        list8<A1 &, A2 &, A3 &, A4 &, A5 &, A6 &, A7 &, A8 &> a(a1, a2, a3, a4, a5, a6, a7, a8);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

    template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> result_type operator()(A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5, A6 & a6, A7 & a7, A8 & a8) const
    {
        list8<A1 &, A2 &, A3 &, A4 &, A5 &, A6 &, A7 &, A8 &> a(a1, a2, a3, a4, a5, a6, a7, a8);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING) \
 && !BOOST_WORKAROUND(__EDG_VERSION__, <= 238)

    template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6, A7 const & a7, A8 const & a8)
    {
        list8<A1 const &, A2 const &, A3 const &, A4 const &, A5 const &, A6 const &, A7 const &, A8 const &> a(a1, a2, a3, a4, a5, a6, a7, a8);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

    template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6, A7 const & a7, A8 const & a8) const
    {
        list8<A1 const &, A2 const &, A3 const &, A4 const &, A5 const &, A6 const &, A7 const &, A8 const &> a(a1, a2, a3, a4, a5, a6, a7, a8);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

#endif

    template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> result_type operator()(A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5, A6 & a6, A7 & a7, A8 & a8, A9 & a9)
    {
        list9<A1 &, A2 &, A3 &, A4 &, A5 &, A6 &, A7 &, A8 &, A9 &> a(a1, a2, a3, a4, a5, a6, a7, a8, a9);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

    template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> result_type operator()(A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5, A6 & a6, A7 & a7, A8 & a8, A9 & a9) const
    {
        list9<A1 &, A2 &, A3 &, A4 &, A5 &, A6 &, A7 &, A8 &, A9 &> a(a1, a2, a3, a4, a5, a6, a7, a8, a9);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING) \
 && !BOOST_WORKAROUND(__EDG_VERSION__, <= 238)

    template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6, A7 const & a7, A8 const & a8, A9 const & a9)
    {
        list9<A1 const &, A2 const &, A3 const &, A4 const &, A5 const &, A6 const &, A7 const &, A8 const &, A9 const &> a(a1, a2, a3, a4, a5, a6, a7, a8, a9);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

    template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6, A7 const & a7, A8 const & a8, A9 const & a9) const
    {
        list9<A1 const &, A2 const &, A3 const &, A4 const &, A5 const &, A6 const &, A7 const &, A8 const &, A9 const &> a(a1, a2, a3, a4, a5, a6, a7, a8, a9);
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

#endif

    template<class A> result_type eval(A & a)
    {
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

    template<class A> result_type eval(A & a) const
    {
        BOOST_BIND_RETURN l_(type<result_type>(), f_, a, 0);
    }

    template<class V> void accept(V & v) const
    {
#if !defined( BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP ) && !defined( BOOST_BORLANDC )

        using boost::visit_each;

#endif
        BOOST_BIND_VISIT_EACH(v, f_, 0);
        l_.accept(v);
    }

    bool compare(this_type const & rhs) const
    {
        return ref_compare(f_, rhs.f_, 0) && l_ == rhs.l_;
    }

private:

    F f_;
    L l_;

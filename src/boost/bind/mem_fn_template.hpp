//
//  bind/mem_fn_template.hpp
//
//  Do not include this header directly
//
//  Copyright (c) 2001 Peter Dimov and Multi Media Ltd.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/bind/mem_fn.html for documentation.
//

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)
# define BOOST_MEM_FN_ENABLE_CONST_OVERLOADS
#endif

// mf0

template<class R, class T BOOST_MEM_FN_CLASS_F> class BOOST_MEM_FN_NAME(mf0)
{
public:

    typedef R result_type;
    typedef T * argument_type;

private:
    
    BOOST_MEM_FN_TYPEDEF(R (BOOST_MEM_FN_CC T::*F) ())
    F f_;

    template<class U> R call(U & u, T const *) const
    {
        BOOST_MEM_FN_RETURN (u.*f_)();
    }

    template<class U> R call(U & u, void const *) const
    {
        BOOST_MEM_FN_RETURN (get_pointer(u)->*f_)();
    }

public:
    
    explicit BOOST_MEM_FN_NAME(mf0)(F f): f_(f) {}

    R operator()(T * p) const
    {
        BOOST_MEM_FN_RETURN (p->*f_)();
    }

    template<class U> R operator()(U & u) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p);
    }

#ifdef BOOST_MEM_FN_ENABLE_CONST_OVERLOADS

    template<class U> R operator()(U const & u) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p);
    }

#endif

    R operator()(T & t) const
    {
        BOOST_MEM_FN_RETURN (t.*f_)();
    }

    bool operator==(BOOST_MEM_FN_NAME(mf0) const & rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(BOOST_MEM_FN_NAME(mf0) const & rhs) const
    {
        return f_ != rhs.f_;
    }
};

// cmf0

template<class R, class T BOOST_MEM_FN_CLASS_F> class BOOST_MEM_FN_NAME(cmf0)
{
public:

    typedef R result_type;
    typedef T const * argument_type;

private:
    
    BOOST_MEM_FN_TYPEDEF(R (BOOST_MEM_FN_CC T::*F) () const)
    F f_;

    template<class U> R call(U & u, T const *) const
    {
        BOOST_MEM_FN_RETURN (u.*f_)();
    }

    template<class U> R call(U & u, void const *) const
    {
        BOOST_MEM_FN_RETURN (get_pointer(u)->*f_)();
    }

public:
    
    explicit BOOST_MEM_FN_NAME(cmf0)(F f): f_(f) {}

    template<class U> R operator()(U const & u) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p);
    }

    R operator()(T const & t) const
    {
        BOOST_MEM_FN_RETURN (t.*f_)();
    }

    bool operator==(BOOST_MEM_FN_NAME(cmf0) const & rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(BOOST_MEM_FN_NAME(cmf0) const & rhs) const
    {
        return f_ != rhs.f_;
    }
};

// mf1

template<class R, class T, class A1 BOOST_MEM_FN_CLASS_F> class BOOST_MEM_FN_NAME(mf1)
{
public:

    typedef R result_type;
    typedef T * first_argument_type;
    typedef A1 second_argument_type;

private:
    
    BOOST_MEM_FN_TYPEDEF(R (BOOST_MEM_FN_CC T::*F) (A1))
    F f_;

    template<class U, class B1> R call(U & u, T const *, B1 & b1) const
    {
        BOOST_MEM_FN_RETURN (u.*f_)(b1);
    }

    template<class U, class B1> R call(U & u, void const *, B1 & b1) const
    {
        BOOST_MEM_FN_RETURN (get_pointer(u)->*f_)(b1);
    }

public:
    
    explicit BOOST_MEM_FN_NAME(mf1)(F f): f_(f) {}

    R operator()(T * p, A1 a1) const
    {
        BOOST_MEM_FN_RETURN (p->*f_)(a1);
    }

    template<class U> R operator()(U & u, A1 a1) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p, a1);
    }

#ifdef BOOST_MEM_FN_ENABLE_CONST_OVERLOADS

    template<class U> R operator()(U const & u, A1 a1) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p, a1);
    }

#endif

    R operator()(T & t, A1 a1) const
    {
        BOOST_MEM_FN_RETURN (t.*f_)(a1);
    }

    bool operator==(BOOST_MEM_FN_NAME(mf1) const & rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(BOOST_MEM_FN_NAME(mf1) const & rhs) const
    {
        return f_ != rhs.f_;
    }
};

// cmf1

template<class R, class T, class A1 BOOST_MEM_FN_CLASS_F> class BOOST_MEM_FN_NAME(cmf1)
{
public:

    typedef R result_type;
    typedef T const * first_argument_type;
    typedef A1 second_argument_type;

private:
    
    BOOST_MEM_FN_TYPEDEF(R (BOOST_MEM_FN_CC T::*F) (A1) const)
    F f_;

    template<class U, class B1> R call(U & u, T const *, B1 & b1) const
    {
        BOOST_MEM_FN_RETURN (u.*f_)(b1);
    }

    template<class U, class B1> R call(U & u, void const *, B1 & b1) const
    {
        BOOST_MEM_FN_RETURN (get_pointer(u)->*f_)(b1);
    }

public:
    
    explicit BOOST_MEM_FN_NAME(cmf1)(F f): f_(f) {}

    template<class U> R operator()(U const & u, A1 a1) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p, a1);
    }

    R operator()(T const & t, A1 a1) const
    {
        BOOST_MEM_FN_RETURN (t.*f_)(a1);
    }

    bool operator==(BOOST_MEM_FN_NAME(cmf1) const & rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(BOOST_MEM_FN_NAME(cmf1) const & rhs) const
    {
        return f_ != rhs.f_;
    }
};

// mf2

template<class R, class T, class A1, class A2 BOOST_MEM_FN_CLASS_F> class BOOST_MEM_FN_NAME(mf2)
{
public:

    typedef R result_type;

private:
    
    BOOST_MEM_FN_TYPEDEF(R (BOOST_MEM_FN_CC T::*F) (A1, A2))
    F f_;

    template<class U, class B1, class B2> R call(U & u, T const *, B1 & b1, B2 & b2) const
    {
        BOOST_MEM_FN_RETURN (u.*f_)(b1, b2);
    }

    template<class U, class B1, class B2> R call(U & u, void const *, B1 & b1, B2 & b2) const
    {
        BOOST_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2);
    }

public:
    
    explicit BOOST_MEM_FN_NAME(mf2)(F f): f_(f) {}

    R operator()(T * p, A1 a1, A2 a2) const
    {
        BOOST_MEM_FN_RETURN (p->*f_)(a1, a2);
    }

    template<class U> R operator()(U & u, A1 a1, A2 a2) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p, a1, a2);
    }

#ifdef BOOST_MEM_FN_ENABLE_CONST_OVERLOADS

    template<class U> R operator()(U const & u, A1 a1, A2 a2) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p, a1, a2);
    }

#endif

    R operator()(T & t, A1 a1, A2 a2) const
    {
        BOOST_MEM_FN_RETURN (t.*f_)(a1, a2);
    }

    bool operator==(BOOST_MEM_FN_NAME(mf2) const & rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(BOOST_MEM_FN_NAME(mf2) const & rhs) const
    {
        return f_ != rhs.f_;
    }
};

// cmf2

template<class R, class T, class A1, class A2 BOOST_MEM_FN_CLASS_F> class BOOST_MEM_FN_NAME(cmf2)
{
public:

    typedef R result_type;

private:
    
    BOOST_MEM_FN_TYPEDEF(R (BOOST_MEM_FN_CC T::*F) (A1, A2) const)
    F f_;

    template<class U, class B1, class B2> R call(U & u, T const *, B1 & b1, B2 & b2) const
    {
        BOOST_MEM_FN_RETURN (u.*f_)(b1, b2);
    }

    template<class U, class B1, class B2> R call(U & u, void const *, B1 & b1, B2 & b2) const
    {
        BOOST_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2);
    }

public:
    
    explicit BOOST_MEM_FN_NAME(cmf2)(F f): f_(f) {}

    template<class U> R operator()(U const & u, A1 a1, A2 a2) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p, a1, a2);
    }

    R operator()(T const & t, A1 a1, A2 a2) const
    {
        BOOST_MEM_FN_RETURN (t.*f_)(a1, a2);
    }

    bool operator==(BOOST_MEM_FN_NAME(cmf2) const & rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(BOOST_MEM_FN_NAME(cmf2) const & rhs) const
    {
        return f_ != rhs.f_;
    }
};

// mf3

template<class R, class T, class A1, class A2, class A3 BOOST_MEM_FN_CLASS_F> class BOOST_MEM_FN_NAME(mf3)
{
public:

    typedef R result_type;

private:
    
    BOOST_MEM_FN_TYPEDEF(R (BOOST_MEM_FN_CC T::*F) (A1, A2, A3))
    F f_;

    template<class U, class B1, class B2, class B3> R call(U & u, T const *, B1 & b1, B2 & b2, B3 & b3) const
    {
        BOOST_MEM_FN_RETURN (u.*f_)(b1, b2, b3);
    }

    template<class U, class B1, class B2, class B3> R call(U & u, void const *, B1 & b1, B2 & b2, B3 & b3) const
    {
        BOOST_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2, b3);
    }

public:
    
    explicit BOOST_MEM_FN_NAME(mf3)(F f): f_(f) {}

    R operator()(T * p, A1 a1, A2 a2, A3 a3) const
    {
        BOOST_MEM_FN_RETURN (p->*f_)(a1, a2, a3);
    }

    template<class U> R operator()(U & u, A1 a1, A2 a2, A3 a3) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p, a1, a2, a3);
    }

#ifdef BOOST_MEM_FN_ENABLE_CONST_OVERLOADS

    template<class U> R operator()(U const & u, A1 a1, A2 a2, A3 a3) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p, a1, a2, a3);
    }

#endif

    R operator()(T & t, A1 a1, A2 a2, A3 a3) const
    {
        BOOST_MEM_FN_RETURN (t.*f_)(a1, a2, a3);
    }

    bool operator==(BOOST_MEM_FN_NAME(mf3) const & rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(BOOST_MEM_FN_NAME(mf3) const & rhs) const
    {
        return f_ != rhs.f_;
    }
};

// cmf3

template<class R, class T, class A1, class A2, class A3 BOOST_MEM_FN_CLASS_F> class BOOST_MEM_FN_NAME(cmf3)
{
public:

    typedef R result_type;

private:

    BOOST_MEM_FN_TYPEDEF(R (BOOST_MEM_FN_CC T::*F) (A1, A2, A3) const)
    F f_;

    template<class U, class B1, class B2, class B3> R call(U & u, T const *, B1 & b1, B2 & b2, B3 & b3) const
    {
        BOOST_MEM_FN_RETURN (u.*f_)(b1, b2, b3);
    }

    template<class U, class B1, class B2, class B3> R call(U & u, void const *, B1 & b1, B2 & b2, B3 & b3) const
    {
        BOOST_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2, b3);
    }

public:

    explicit BOOST_MEM_FN_NAME(cmf3)(F f): f_(f) {}

    template<class U> R operator()(U const & u, A1 a1, A2 a2, A3 a3) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p, a1, a2, a3);
    }

    R operator()(T const & t, A1 a1, A2 a2, A3 a3) const
    {
        BOOST_MEM_FN_RETURN (t.*f_)(a1, a2, a3);
    }

    bool operator==(BOOST_MEM_FN_NAME(cmf3) const & rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(BOOST_MEM_FN_NAME(cmf3) const & rhs) const
    {
        return f_ != rhs.f_;
    }
};

// mf4

template<class R, class T, class A1, class A2, class A3, class A4 BOOST_MEM_FN_CLASS_F> class BOOST_MEM_FN_NAME(mf4)
{
public:

    typedef R result_type;

private:
    
    BOOST_MEM_FN_TYPEDEF(R (BOOST_MEM_FN_CC T::*F) (A1, A2, A3, A4))
    F f_;

    template<class U, class B1, class B2, class B3, class B4> R call(U & u, T const *, B1 & b1, B2 & b2, B3 & b3, B4 & b4) const
    {
        BOOST_MEM_FN_RETURN (u.*f_)(b1, b2, b3, b4);
    }

    template<class U, class B1, class B2, class B3, class B4> R call(U & u, void const *, B1 & b1, B2 & b2, B3 & b3, B4 & b4) const
    {
        BOOST_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2, b3, b4);
    }

public:
    
    explicit BOOST_MEM_FN_NAME(mf4)(F f): f_(f) {}

    R operator()(T * p, A1 a1, A2 a2, A3 a3, A4 a4) const
    {
        BOOST_MEM_FN_RETURN (p->*f_)(a1, a2, a3, a4);
    }

    template<class U> R operator()(U & u, A1 a1, A2 a2, A3 a3, A4 a4) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p, a1, a2, a3, a4);
    }

#ifdef BOOST_MEM_FN_ENABLE_CONST_OVERLOADS

    template<class U> R operator()(U const & u, A1 a1, A2 a2, A3 a3, A4 a4) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p, a1, a2, a3, a4);
    }

#endif

    R operator()(T & t, A1 a1, A2 a2, A3 a3, A4 a4) const
    {
        BOOST_MEM_FN_RETURN (t.*f_)(a1, a2, a3, a4);
    }

    bool operator==(BOOST_MEM_FN_NAME(mf4) const & rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(BOOST_MEM_FN_NAME(mf4) const & rhs) const
    {
        return f_ != rhs.f_;
    }
};

// cmf4

template<class R, class T, class A1, class A2, class A3, class A4 BOOST_MEM_FN_CLASS_F> class BOOST_MEM_FN_NAME(cmf4)
{
public:

    typedef R result_type;

private:
    
    BOOST_MEM_FN_TYPEDEF(R (BOOST_MEM_FN_CC T::*F) (A1, A2, A3, A4) const)
    F f_;

    template<class U, class B1, class B2, class B3, class B4> R call(U & u, T const *, B1 & b1, B2 & b2, B3 & b3, B4 & b4) const
    {
        BOOST_MEM_FN_RETURN (u.*f_)(b1, b2, b3, b4);
    }

    template<class U, class B1, class B2, class B3, class B4> R call(U & u, void const *, B1 & b1, B2 & b2, B3 & b3, B4 & b4) const
    {
        BOOST_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2, b3, b4);
    }

public:
    
    explicit BOOST_MEM_FN_NAME(cmf4)(F f): f_(f) {}

    template<class U> R operator()(U const & u, A1 a1, A2 a2, A3 a3, A4 a4) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p, a1, a2, a3, a4);
    }

    R operator()(T const & t, A1 a1, A2 a2, A3 a3, A4 a4) const
    {
        BOOST_MEM_FN_RETURN (t.*f_)(a1, a2, a3, a4);
    }

    bool operator==(BOOST_MEM_FN_NAME(cmf4) const & rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(BOOST_MEM_FN_NAME(cmf4) const & rhs) const
    {
        return f_ != rhs.f_;
    }
};

// mf5

template<class R, class T, class A1, class A2, class A3, class A4, class A5 BOOST_MEM_FN_CLASS_F> class BOOST_MEM_FN_NAME(mf5)
{
public:

    typedef R result_type;

private:
    
    BOOST_MEM_FN_TYPEDEF(R (BOOST_MEM_FN_CC T::*F) (A1, A2, A3, A4, A5))
    F f_;

    template<class U, class B1, class B2, class B3, class B4, class B5> R call(U & u, T const *, B1 & b1, B2 & b2, B3 & b3, B4 & b4, B5 & b5) const
    {
        BOOST_MEM_FN_RETURN (u.*f_)(b1, b2, b3, b4, b5);
    }

    template<class U, class B1, class B2, class B3, class B4, class B5> R call(U & u, void const *, B1 & b1, B2 & b2, B3 & b3, B4 & b4, B5 & b5) const
    {
        BOOST_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2, b3, b4, b5);
    }

public:
    
    explicit BOOST_MEM_FN_NAME(mf5)(F f): f_(f) {}

    R operator()(T * p, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) const
    {
        BOOST_MEM_FN_RETURN (p->*f_)(a1, a2, a3, a4, a5);
    }

    template<class U> R operator()(U & u, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p, a1, a2, a3, a4, a5);
    }

#ifdef BOOST_MEM_FN_ENABLE_CONST_OVERLOADS

    template<class U> R operator()(U const & u, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p, a1, a2, a3, a4, a5);
    }

#endif

    R operator()(T & t, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) const
    {
        BOOST_MEM_FN_RETURN (t.*f_)(a1, a2, a3, a4, a5);
    }

    bool operator==(BOOST_MEM_FN_NAME(mf5) const & rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(BOOST_MEM_FN_NAME(mf5) const & rhs) const
    {
        return f_ != rhs.f_;
    }
};

// cmf5

template<class R, class T, class A1, class A2, class A3, class A4, class A5 BOOST_MEM_FN_CLASS_F> class BOOST_MEM_FN_NAME(cmf5)
{
public:

    typedef R result_type;

private:
    
    BOOST_MEM_FN_TYPEDEF(R (BOOST_MEM_FN_CC T::*F) (A1, A2, A3, A4, A5) const)
    F f_;

    template<class U, class B1, class B2, class B3, class B4, class B5> R call(U & u, T const *, B1 & b1, B2 & b2, B3 & b3, B4 & b4, B5 & b5) const
    {
        BOOST_MEM_FN_RETURN (u.*f_)(b1, b2, b3, b4, b5);
    }

    template<class U, class B1, class B2, class B3, class B4, class B5> R call(U & u, void const *, B1 & b1, B2 & b2, B3 & b3, B4 & b4, B5 & b5) const
    {
        BOOST_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2, b3, b4, b5);
    }

public:
    
    explicit BOOST_MEM_FN_NAME(cmf5)(F f): f_(f) {}

    template<class U> R operator()(U const & u, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p, a1, a2, a3, a4, a5);
    }

    R operator()(T const & t, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) const
    {
        BOOST_MEM_FN_RETURN (t.*f_)(a1, a2, a3, a4, a5);
    }

    bool operator==(BOOST_MEM_FN_NAME(cmf5) const & rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(BOOST_MEM_FN_NAME(cmf5) const & rhs) const
    {
        return f_ != rhs.f_;
    }
};

// mf6

template<class R, class T, class A1, class A2, class A3, class A4, class A5, class A6 BOOST_MEM_FN_CLASS_F> class BOOST_MEM_FN_NAME(mf6)
{
public:

    typedef R result_type;

private:

    BOOST_MEM_FN_TYPEDEF(R (BOOST_MEM_FN_CC T::*F) (A1, A2, A3, A4, A5, A6))
    F f_;

    template<class U, class B1, class B2, class B3, class B4, class B5, class B6> R call(U & u, T const *, B1 & b1, B2 & b2, B3 & b3, B4 & b4, B5 & b5, B6 & b6) const
    {
        BOOST_MEM_FN_RETURN (u.*f_)(b1, b2, b3, b4, b5, b6);
    }

    template<class U, class B1, class B2, class B3, class B4, class B5, class B6> R call(U & u, void const *, B1 & b1, B2 & b2, B3 & b3, B4 & b4, B5 & b5, B6 & b6) const
    {
        BOOST_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2, b3, b4, b5, b6);
    }

public:

    explicit BOOST_MEM_FN_NAME(mf6)(F f): f_(f) {}

    R operator()(T * p, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) const
    {
        BOOST_MEM_FN_RETURN (p->*f_)(a1, a2, a3, a4, a5, a6);
    }

    template<class U> R operator()(U & u, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p, a1, a2, a3, a4, a5, a6);
    }

#ifdef BOOST_MEM_FN_ENABLE_CONST_OVERLOADS

    template<class U> R operator()(U const & u, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p, a1, a2, a3, a4, a5, a6);
    }

#endif

    R operator()(T & t, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) const
    {
        BOOST_MEM_FN_RETURN (t.*f_)(a1, a2, a3, a4, a5, a6);
    }

    bool operator==(BOOST_MEM_FN_NAME(mf6) const & rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(BOOST_MEM_FN_NAME(mf6) const & rhs) const
    {
        return f_ != rhs.f_;
    }
};

// cmf6

template<class R, class T, class A1, class A2, class A3, class A4, class A5, class A6 BOOST_MEM_FN_CLASS_F> class BOOST_MEM_FN_NAME(cmf6)
{
public:

    typedef R result_type;

private:
    
    BOOST_MEM_FN_TYPEDEF(R (BOOST_MEM_FN_CC T::*F) (A1, A2, A3, A4, A5, A6) const)
    F f_;

    template<class U, class B1, class B2, class B3, class B4, class B5, class B6> R call(U & u, T const *, B1 & b1, B2 & b2, B3 & b3, B4 & b4, B5 & b5, B6 & b6) const
    {
        BOOST_MEM_FN_RETURN (u.*f_)(b1, b2, b3, b4, b5, b6);
    }

    template<class U, class B1, class B2, class B3, class B4, class B5, class B6> R call(U & u, void const *, B1 & b1, B2 & b2, B3 & b3, B4 & b4, B5 & b5, B6 & b6) const
    {
        BOOST_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2, b3, b4, b5, b6);
    }

public:
    
    explicit BOOST_MEM_FN_NAME(cmf6)(F f): f_(f) {}

    template<class U> R operator()(U const & u, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p, a1, a2, a3, a4, a5, a6);
    }

    R operator()(T const & t, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) const
    {
        BOOST_MEM_FN_RETURN (t.*f_)(a1, a2, a3, a4, a5, a6);
    }

    bool operator==(BOOST_MEM_FN_NAME(cmf6) const & rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(BOOST_MEM_FN_NAME(cmf6) const & rhs) const
    {
        return f_ != rhs.f_;
    }
};

// mf7

template<class R, class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7 BOOST_MEM_FN_CLASS_F> class BOOST_MEM_FN_NAME(mf7)
{
public:

    typedef R result_type;

private:
    
    BOOST_MEM_FN_TYPEDEF(R (BOOST_MEM_FN_CC T::*F) (A1, A2, A3, A4, A5, A6, A7))
    F f_;

    template<class U, class B1, class B2, class B3, class B4, class B5, class B6, class B7> R call(U & u, T const *, B1 & b1, B2 & b2, B3 & b3, B4 & b4, B5 & b5, B6 & b6, B7 & b7) const
    {
        BOOST_MEM_FN_RETURN (u.*f_)(b1, b2, b3, b4, b5, b6, b7);
    }

    template<class U, class B1, class B2, class B3, class B4, class B5, class B6, class B7> R call(U & u, void const *, B1 & b1, B2 & b2, B3 & b3, B4 & b4, B5 & b5, B6 & b6, B7 & b7) const
    {
        BOOST_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2, b3, b4, b5, b6, b7);
    }

public:
    
    explicit BOOST_MEM_FN_NAME(mf7)(F f): f_(f) {}

    R operator()(T * p, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) const
    {
        BOOST_MEM_FN_RETURN (p->*f_)(a1, a2, a3, a4, a5, a6, a7);
    }

    template<class U> R operator()(U & u, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p, a1, a2, a3, a4, a5, a6, a7);
    }

#ifdef BOOST_MEM_FN_ENABLE_CONST_OVERLOADS

    template<class U> R operator()(U const & u, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p, a1, a2, a3, a4, a5, a6, a7);
    }

#endif

    R operator()(T & t, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) const
    {
        BOOST_MEM_FN_RETURN (t.*f_)(a1, a2, a3, a4, a5, a6, a7);
    }

    bool operator==(BOOST_MEM_FN_NAME(mf7) const & rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(BOOST_MEM_FN_NAME(mf7) const & rhs) const
    {
        return f_ != rhs.f_;
    }
};

// cmf7

template<class R, class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7 BOOST_MEM_FN_CLASS_F> class BOOST_MEM_FN_NAME(cmf7)
{
public:

    typedef R result_type;

private:
    
    BOOST_MEM_FN_TYPEDEF(R (BOOST_MEM_FN_CC T::*F) (A1, A2, A3, A4, A5, A6, A7) const)
    F f_;

    template<class U, class B1, class B2, class B3, class B4, class B5, class B6, class B7> R call(U & u, T const *, B1 & b1, B2 & b2, B3 & b3, B4 & b4, B5 & b5, B6 & b6, B7 & b7) const
    {
        BOOST_MEM_FN_RETURN (u.*f_)(b1, b2, b3, b4, b5, b6, b7);
    }

    template<class U, class B1, class B2, class B3, class B4, class B5, class B6, class B7> R call(U & u, void const *, B1 & b1, B2 & b2, B3 & b3, B4 & b4, B5 & b5, B6 & b6, B7 & b7) const
    {
        BOOST_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2, b3, b4, b5, b6, b7);
    }

public:
    
    explicit BOOST_MEM_FN_NAME(cmf7)(F f): f_(f) {}

    template<class U> R operator()(U const & u, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p, a1, a2, a3, a4, a5, a6, a7);
    }

    R operator()(T const & t, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) const
    {
        BOOST_MEM_FN_RETURN (t.*f_)(a1, a2, a3, a4, a5, a6, a7);
    }

    bool operator==(BOOST_MEM_FN_NAME(cmf7) const & rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(BOOST_MEM_FN_NAME(cmf7) const & rhs) const
    {
        return f_ != rhs.f_;
    }
};

// mf8

template<class R, class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8 BOOST_MEM_FN_CLASS_F> class BOOST_MEM_FN_NAME(mf8)
{
public:

    typedef R result_type;

private:
    
    BOOST_MEM_FN_TYPEDEF(R (BOOST_MEM_FN_CC T::*F) (A1, A2, A3, A4, A5, A6, A7, A8))
    F f_;

    template<class U, class B1, class B2, class B3, class B4, class B5, class B6, class B7, class B8> R call(U & u, T const *, B1 & b1, B2 & b2, B3 & b3, B4 & b4, B5 & b5, B6 & b6, B7 & b7, B8 & b8) const
    {
        BOOST_MEM_FN_RETURN (u.*f_)(b1, b2, b3, b4, b5, b6, b7, b8);
    }

    template<class U, class B1, class B2, class B3, class B4, class B5, class B6, class B7, class B8> R call(U & u, void const *, B1 & b1, B2 & b2, B3 & b3, B4 & b4, B5 & b5, B6 & b6, B7 & b7, B8 & b8) const
    {
        BOOST_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2, b3, b4, b5, b6, b7, b8);
    }

public:
    
    explicit BOOST_MEM_FN_NAME(mf8)(F f): f_(f) {}

    R operator()(T * p, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) const
    {
        BOOST_MEM_FN_RETURN (p->*f_)(a1, a2, a3, a4, a5, a6, a7, a8);
    }

    template<class U> R operator()(U & u, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p, a1, a2, a3, a4, a5, a6, a7, a8);
    }

#ifdef BOOST_MEM_FN_ENABLE_CONST_OVERLOADS

    template<class U> R operator()(U const & u, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p, a1, a2, a3, a4, a5, a6, a7, a8);
    }

#endif

    R operator()(T & t, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) const
    {
        BOOST_MEM_FN_RETURN (t.*f_)(a1, a2, a3, a4, a5, a6, a7, a8);
    }

    bool operator==(BOOST_MEM_FN_NAME(mf8) const & rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(BOOST_MEM_FN_NAME(mf8) const & rhs) const
    {
        return f_ != rhs.f_;
    }
};

// cmf8

template<class R, class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8 BOOST_MEM_FN_CLASS_F> class BOOST_MEM_FN_NAME(cmf8)
{
public:

    typedef R result_type;

private:
    
    BOOST_MEM_FN_TYPEDEF(R (BOOST_MEM_FN_CC T::*F) (A1, A2, A3, A4, A5, A6, A7, A8) const)
    F f_;

    template<class U, class B1, class B2, class B3, class B4, class B5, class B6, class B7, class B8> R call(U & u, T const *, B1 & b1, B2 & b2, B3 & b3, B4 & b4, B5 & b5, B6 & b6, B7 & b7, B8 & b8) const
    {
        BOOST_MEM_FN_RETURN (u.*f_)(b1, b2, b3, b4, b5, b6, b7, b8);
    }

    template<class U, class B1, class B2, class B3, class B4, class B5, class B6, class B7, class B8> R call(U & u, void const *, B1 & b1, B2 & b2, B3 & b3, B4 & b4, B5 & b5, B6 & b6, B7 & b7, B8 & b8) const
    {
        BOOST_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2, b3, b4, b5, b6, b7, b8);
    }

public:
    
    explicit BOOST_MEM_FN_NAME(cmf8)(F f): f_(f) {}

    R operator()(T const * p, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) const
    {
        BOOST_MEM_FN_RETURN (p->*f_)(a1, a2, a3, a4, a5, a6, a7, a8);
    }

    template<class U> R operator()(U const & u, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) const
    {
        U const * p = 0;
        BOOST_MEM_FN_RETURN call(u, p, a1, a2, a3, a4, a5, a6, a7, a8);
    }

    R operator()(T const & t, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) const
    {
        BOOST_MEM_FN_RETURN (t.*f_)(a1, a2, a3, a4, a5, a6, a7, a8);
    }

    bool operator==(BOOST_MEM_FN_NAME(cmf8) const & rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(BOOST_MEM_FN_NAME(cmf8) const & rhs) const
    {
        return f_ != rhs.f_;
    }
};

#undef BOOST_MEM_FN_ENABLE_CONST_OVERLOADS

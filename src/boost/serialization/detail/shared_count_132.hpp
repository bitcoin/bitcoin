#ifndef BOOST_DETAIL_SHARED_COUNT_132_HPP_INCLUDED
#define BOOST_DETAIL_SHARED_COUNT_132_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER)
# pragma once
#endif

//
//  detail/shared_count.hpp
//
//  Copyright (c) 2001, 2002, 2003 Peter Dimov and Multi Media Ltd.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/config.hpp>

#if defined(BOOST_SP_USE_STD_ALLOCATOR) && defined(BOOST_SP_USE_QUICK_ALLOCATOR)
# error BOOST_SP_USE_STD_ALLOCATOR and BOOST_SP_USE_QUICK_ALLOCATOR are incompatible.
#endif

#include <boost/checked_delete.hpp>
#include <boost/serialization/throw_exception.hpp>
#include <boost/detail/lightweight_mutex.hpp>

#if defined(BOOST_SP_USE_QUICK_ALLOCATOR)
#include <boost/detail/quick_allocator.hpp>
#endif

#include <memory>           // std::auto_ptr, std::allocator
#include <functional>       // std::less
#include <exception>        // std::exception
#include <new>              // std::bad_alloc
#include <typeinfo>         // std::type_info in get_deleter
#include <cstddef>          // std::size_t

#include <boost/config.hpp> // msvc 6.0 needs this for warning suppression
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{
    using ::size_t;
} // namespace std
#endif

namespace boost_132 {

// Debug hooks

#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)

void sp_scalar_constructor_hook(void * px, std::size_t size, void * pn);
void sp_array_constructor_hook(void * px);
void sp_scalar_destructor_hook(void * px, std::size_t size, void * pn);
void sp_array_destructor_hook(void * px);

#endif


// The standard library that comes with Borland C++ 5.5.1
// defines std::exception and its members as having C calling
// convention (-pc). When the definition of bad_weak_ptr
// is compiled with -ps, the compiler issues an error.
// Hence, the temporary #pragma option -pc below. The version
// check is deliberately conservative.

class bad_weak_ptr: public std::exception
{
public:

    virtual char const * what() const throw()
    {
        return "boost::bad_weak_ptr";
    }
};

namespace detail{

class sp_counted_base
{
//private:

    typedef boost::detail::lightweight_mutex mutex_type;

public:

    sp_counted_base(): use_count_(1), weak_count_(1)
    {
    }

    virtual ~sp_counted_base() // nothrow
    {
    }

    // dispose() is called when use_count_ drops to zero, to release
    // the resources managed by *this.

    virtual void dispose() = 0; // nothrow

    // destruct() is called when weak_count_ drops to zero.

    virtual void destruct() // nothrow
    {
        delete this;
    }

    virtual void * get_deleter(std::type_info const & ti) = 0;

    void add_ref_copy()
    {
#if defined(BOOST_HAS_THREADS)
        mutex_type::scoped_lock lock(mtx_);
#endif
        ++use_count_;
    }

    void add_ref_lock()
    {
#if defined(BOOST_HAS_THREADS)
        mutex_type::scoped_lock lock(mtx_);
#endif
        if(use_count_ == 0) boost::serialization::throw_exception(bad_weak_ptr());
        ++use_count_;
    }

    void release() // nothrow
    {
        {
#if defined(BOOST_HAS_THREADS)
            mutex_type::scoped_lock lock(mtx_);
#endif
            long new_use_count = --use_count_;

            if(new_use_count != 0) return;
        }

        dispose();
        weak_release();
    }

    void weak_add_ref() // nothrow
    {
#if defined(BOOST_HAS_THREADS)
        mutex_type::scoped_lock lock(mtx_);
#endif
        ++weak_count_;
    }

    void weak_release() // nothrow
    {
        long new_weak_count;

        {
#if defined(BOOST_HAS_THREADS)
            mutex_type::scoped_lock lock(mtx_);
#endif
            new_weak_count = --weak_count_;
        }

        if(new_weak_count == 0)
        {
            destruct();
        }
    }

    long use_count() const // nothrow
    {
#if defined(BOOST_HAS_THREADS)
        mutex_type::scoped_lock lock(mtx_);
#endif
        return use_count_;
    }

//private:
public:
    sp_counted_base(sp_counted_base const &);
    sp_counted_base & operator= (sp_counted_base const &);

    long use_count_;        // #shared
    long weak_count_;       // #weak + (#shared != 0)

#if defined(BOOST_HAS_THREADS) || defined(BOOST_LWM_WIN32)
    mutable mutex_type mtx_;
#endif
};

#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)

template<class T> void cbi_call_constructor_hook(sp_counted_base * pn, T * px, boost::checked_deleter< T > const &)
{
    boost::sp_scalar_constructor_hook(px, sizeof(T), pn);
}

template<class T> void cbi_call_constructor_hook(sp_counted_base *, T * px, boost::checked_array_deleter< T > const &)
{
    boost::sp_array_constructor_hook(px);
}

template<class P, class D> void cbi_call_constructor_hook(sp_counted_base *, P const &, D const &, long)
{
}

template<class T> void cbi_call_destructor_hook(sp_counted_base * pn, T * px, boost::checked_deleter< T > const &)
{
    boost::sp_scalar_destructor_hook(px, sizeof(T), pn);
}

template<class T> void cbi_call_destructor_hook(sp_counted_base *, T * px, boost::checked_array_deleter< T > const &)
{
    boost::sp_array_destructor_hook(px);
}

template<class P, class D> void cbi_call_destructor_hook(sp_counted_base *, P const &, D const &, long)
{
}

#endif

//
// Borland's Codeguard trips up over the -Vx- option here:
//
#ifdef __CODEGUARD__
# pragma option push -Vx-
#endif

template<class P, class D> class sp_counted_base_impl: public sp_counted_base
{
//private:
public:
    P ptr; // copy constructor must not throw
    D del; // copy constructor must not throw

    sp_counted_base_impl(sp_counted_base_impl const &);
    sp_counted_base_impl & operator= (sp_counted_base_impl const &);

    typedef sp_counted_base_impl<P, D> this_type;

public:

    // pre: initial_use_count <= initial_weak_count, d(p) must not throw

    sp_counted_base_impl(P p, D d): ptr(p), del(d)
    {
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        detail::cbi_call_constructor_hook(this, p, d, 0);
#endif
    }

    virtual void dispose() // nothrow
    {
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        detail::cbi_call_destructor_hook(this, ptr, del, 0);
#endif
        del(ptr);
    }

    virtual void * get_deleter(std::type_info const & ti)
    {
        return ti == typeid(D)? &del: 0;
    }

#if defined(BOOST_SP_USE_STD_ALLOCATOR)

    void * operator new(std::size_t)
    {
        return std::allocator<this_type>().allocate(1, static_cast<this_type *>(0));
    }

    void operator delete(void * p)
    {
        std::allocator<this_type>().deallocate(static_cast<this_type *>(p), 1);
    }

#endif

#if defined(BOOST_SP_USE_QUICK_ALLOCATOR)

    void * operator new(std::size_t)
    {
        return boost::detail::quick_allocator<this_type>::alloc();
    }

    void operator delete(void * p)
    {
        boost::detail::quick_allocator<this_type>::dealloc(p);
    }

#endif
};

#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)

int const shared_count_id = 0x2C35F101;
int const   weak_count_id = 0x298C38A4;

#endif

class weak_count;

class shared_count
{
//private:
public:
    sp_counted_base * pi_;

#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
    int id_;
#endif

    friend class weak_count;

public:

    shared_count(): pi_(0) // nothrow
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
    {
    }

    template<class P, class D> shared_count(P p, D d): pi_(0)
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
    {
#ifndef BOOST_NO_EXCEPTIONS

        try
        {
            pi_ = new sp_counted_base_impl<P, D>(p, d);
        }
        catch(...)
        {
            d(p); // delete p
            throw;
        }

#else

        pi_ = new sp_counted_base_impl<P, D>(p, d);

        if(pi_ == 0)
        {
            d(p); // delete p
            boost::serialization::throw_exception(std::bad_alloc());
        }

#endif
    }

#ifndef BOOST_NO_AUTO_PTR

    // auto_ptr<Y> is special cased to provide the strong guarantee

    template<class Y>
    explicit shared_count(std::auto_ptr<Y> & r): pi_(
        new sp_counted_base_impl<
            Y *,
            boost::checked_deleter<Y>
        >(r.get(), boost::checked_deleter<Y>()))
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
    {
        r.release();
    }

#endif

    ~shared_count() // nothrow
    {
        if(pi_ != 0) pi_->release();
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        id_ = 0;
#endif
    }

    shared_count(shared_count const & r): pi_(r.pi_) // nothrow
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
    {
        if(pi_ != 0) pi_->add_ref_copy();
    }

    explicit shared_count(weak_count const & r); // throws bad_weak_ptr when r.use_count() == 0

    shared_count & operator= (shared_count const & r) // nothrow
    {
        sp_counted_base * tmp = r.pi_;

        if(tmp != pi_)
        {
            if(tmp != 0) tmp->add_ref_copy();
            if(pi_ != 0) pi_->release();
            pi_ = tmp;
        }

        return *this;
    }

    void swap(shared_count & r) // nothrow
    {
        sp_counted_base * tmp = r.pi_;
        r.pi_ = pi_;
        pi_ = tmp;
    }

    long use_count() const // nothrow
    {
        return pi_ != 0? pi_->use_count(): 0;
    }

    bool unique() const // nothrow
    {
        return use_count() == 1;
    }

    friend inline bool operator==(shared_count const & a, shared_count const & b)
    {
        return a.pi_ == b.pi_;
    }

    friend inline bool operator<(shared_count const & a, shared_count const & b)
    {
        return std::less<sp_counted_base *>()(a.pi_, b.pi_);
    }

    void * get_deleter(std::type_info const & ti) const
    {
        return pi_? pi_->get_deleter(ti): 0;
    }
};

#ifdef __CODEGUARD__
# pragma option pop
#endif


class weak_count
{
private:

    sp_counted_base * pi_;

#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
    int id_;
#endif

    friend class shared_count;

public:

    weak_count(): pi_(0) // nothrow
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(weak_count_id)
#endif
    {
    }

    weak_count(shared_count const & r): pi_(r.pi_) // nothrow
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
    {
        if(pi_ != 0) pi_->weak_add_ref();
    }

    weak_count(weak_count const & r): pi_(r.pi_) // nothrow
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
    {
        if(pi_ != 0) pi_->weak_add_ref();
    }

    ~weak_count() // nothrow
    {
        if(pi_ != 0) pi_->weak_release();
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        id_ = 0;
#endif
    }

    weak_count & operator= (shared_count const & r) // nothrow
    {
        sp_counted_base * tmp = r.pi_;
        if(tmp != 0) tmp->weak_add_ref();
        if(pi_ != 0) pi_->weak_release();
        pi_ = tmp;

        return *this;
    }

    weak_count & operator= (weak_count const & r) // nothrow
    {
        sp_counted_base * tmp = r.pi_;
        if(tmp != 0) tmp->weak_add_ref();
        if(pi_ != 0) pi_->weak_release();
        pi_ = tmp;

        return *this;
    }

    void swap(weak_count & r) // nothrow
    {
        sp_counted_base * tmp = r.pi_;
        r.pi_ = pi_;
        pi_ = tmp;
    }

    long use_count() const // nothrow
    {
        return pi_ != 0? pi_->use_count(): 0;
    }

    friend inline bool operator==(weak_count const & a, weak_count const & b)
    {
        return a.pi_ == b.pi_;
    }

    friend inline bool operator<(weak_count const & a, weak_count const & b)
    {
        return std::less<sp_counted_base *>()(a.pi_, b.pi_);
    }
};

inline shared_count::shared_count(weak_count const & r): pi_(r.pi_)
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
{
    if(pi_ != 0)
    {
        pi_->add_ref_lock();
    }
    else
    {
        boost::serialization::throw_exception(bad_weak_ptr());
    }
}

} // namespace detail

} // namespace boost

BOOST_SERIALIZATION_ASSUME_ABSTRACT(boost_132::detail::sp_counted_base)

#endif  // #ifndef BOOST_DETAIL_SHARED_COUNT_HPP_INCLUDED

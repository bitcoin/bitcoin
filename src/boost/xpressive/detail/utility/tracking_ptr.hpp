///////////////////////////////////////////////////////////////////////////////
// tracking_ptr.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_UTILITY_TRACKING_PTR_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_UTILITY_TRACKING_PTR_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#ifdef BOOST_XPRESSIVE_DEBUG_TRACKING_POINTER
# include <iostream>
#endif
#include <set>
#include <functional>
#include <boost/config.hpp>
#include <boost/assert.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/detail/atomic_count.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>

namespace boost { namespace xpressive { namespace detail
{

template<typename Type>
struct tracking_ptr;

template<typename Derived>
struct enable_reference_tracking;

///////////////////////////////////////////////////////////////////////////////
// weak_iterator
//  steps through a set of weak_ptr, converts to shared_ptrs on the fly and
//  removes from the set the weak_ptrs that have expired.
template<typename Derived>
struct weak_iterator
  : iterator_facade
    <
        weak_iterator<Derived>
      , shared_ptr<Derived> const
      , std::forward_iterator_tag
    >
{
    typedef std::set<weak_ptr<Derived> > set_type;
    typedef typename set_type::iterator base_iterator;

    weak_iterator()
      : cur_()
      , iter_()
      , set_(0)
    {
    }

    weak_iterator(base_iterator iter, set_type *set)
      : cur_()
      , iter_(iter)
      , set_(set)
    {
        this->satisfy_();
    }

private:
    friend class boost::iterator_core_access;

    shared_ptr<Derived> const &dereference() const
    {
        return this->cur_;
    }

    void increment()
    {
        ++this->iter_;
        this->satisfy_();
    }

    bool equal(weak_iterator<Derived> const &that) const
    {
        return this->iter_ == that.iter_;
    }

    void satisfy_()
    {
        while(this->iter_ != this->set_->end())
        {
            this->cur_ = this->iter_->lock();
            if(this->cur_)
                return;
            base_iterator tmp = this->iter_++;
            this->set_->erase(tmp);
        }
        this->cur_.reset();
    }

    shared_ptr<Derived> cur_;
    base_iterator iter_;
    set_type *set_;
};

///////////////////////////////////////////////////////////////////////////////
// filter_self
//  for use with a filter_iterator to filter a node out of a list of dependencies
template<typename Derived>
struct filter_self
{
    typedef shared_ptr<Derived> argument_type;
    typedef bool result_type;

    filter_self(enable_reference_tracking<Derived> *self)
      : self_(self)
    {
    }

    bool operator ()(shared_ptr<Derived> const &that) const
    {
        return this->self_ != that.get();
    }

private:
    enable_reference_tracking<Derived> *self_;
};

///////////////////////////////////////////////////////////////////////////////
// swap without bringing in std::swap -- must be found by ADL.
template<typename T>
void adl_swap(T &t1, T &t2)
{
    swap(t1, t2);
}

///////////////////////////////////////////////////////////////////////////////
// enable_reference_tracking
//   inherit from this type to enable reference tracking for a type. You can
//   then use tracking_ptr (below) as a holder for derived objects.
//
template<typename Derived>
struct enable_reference_tracking
{
    typedef std::set<shared_ptr<Derived> > references_type;
    typedef std::set<weak_ptr<Derived> > dependents_type;

    void tracking_copy(Derived const &that)
    {
        if(&this->derived_() != &that)
        {
            this->raw_copy_(that);
            this->tracking_update();
        }
    }

    void tracking_clear()
    {
        this->raw_copy_(Derived());
    }

    // called automatically as a result of a tracking_copy(). Must be called explicitly
    // if you change the references without calling tracking_copy().
    void tracking_update()
    {
        // add "this" as a dependency to all the references
        this->update_references_();
        // notify our dependencies that we have new references
        this->update_dependents_();
    }

    void track_reference(enable_reference_tracking<Derived> &that)
    {
        // avoid some unbounded memory growth in certain circumstances by
        // opportunistically removing stale dependencies from "that"
        that.purge_stale_deps_();
        // add "that" as a reference
        this->refs_.insert(that.self_);
        // also inherit that's references
        this->refs_.insert(that.refs_.begin(), that.refs_.end());
    }

    long use_count() const
    {
        return this->cnt_;
    }

    void add_ref()
    {
        ++this->cnt_;
    }

    void release()
    {
        BOOST_ASSERT(0 < this->cnt_);
        if(0 == --this->cnt_)
        {
            this->refs_.clear();
            this->self_.reset();
        }
    }

    //{{AFX_DEBUG
    #ifdef BOOST_XPRESSIVE_DEBUG_TRACKING_POINTER
    friend std::ostream &operator <<(std::ostream &sout, enable_reference_tracking<Derived> const &that)
    {
        that.dump_(sout);
        return sout;
    }
    #endif
    //}}AFX_DEBUG

protected:

    enable_reference_tracking()
      : refs_()
      , deps_()
      , self_()
      , cnt_(0)
    {
    }

    enable_reference_tracking(enable_reference_tracking<Derived> const &that)
      : refs_()
      , deps_()
      , self_()
      , cnt_(0)
    {
        this->operator =(that);
    }

    enable_reference_tracking<Derived> &operator =(enable_reference_tracking<Derived> const &that)
    {
        references_type(that.refs_).swap(this->refs_);
        return *this;
    }

    void swap(enable_reference_tracking<Derived> &that)
    {
        this->refs_.swap(that.refs_);
    }

private:
    friend struct tracking_ptr<Derived>;

    Derived &derived_()
    {
        return *static_cast<Derived *>(this);
    }

    void raw_copy_(Derived that)
    {
        detail::adl_swap(this->derived_(), that);
    }

    bool has_deps_() const
    {
        return !this->deps_.empty();
    }

    void update_references_()
    {
        typename references_type::iterator cur = this->refs_.begin();
        typename references_type::iterator end = this->refs_.end();
        for(; cur != end; ++cur)
        {
            // for each reference, add this as a dependency
            (*cur)->track_dependency_(*this);
        }
    }

    void update_dependents_()
    {
        // called whenever this regex object changes (i.e., is assigned to). it walks
        // the list of dependent regexes and updates *their* lists of references,
        // thereby spreading out the reference counting responsibility evenly.
        weak_iterator<Derived> cur(this->deps_.begin(), &this->deps_);
        weak_iterator<Derived> end(this->deps_.end(), &this->deps_);

        for(; cur != end; ++cur)
        {
            (*cur)->track_reference(*this);
        }
    }

    void track_dependency_(enable_reference_tracking<Derived> &dep)
    {
        if(this == &dep) // never add ourself as a dependency
            return;

        // add dep as a dependency
        this->deps_.insert(dep.self_);

        filter_self<Derived> not_self(this);
        weak_iterator<Derived> begin(dep.deps_.begin(), &dep.deps_);
        weak_iterator<Derived> end(dep.deps_.end(), &dep.deps_);

        // also inherit dep's dependencies
        this->deps_.insert(
            make_filter_iterator(not_self, begin, end)
          , make_filter_iterator(not_self, end, end)
        );
    }

    void purge_stale_deps_()
    {
        weak_iterator<Derived> cur(this->deps_.begin(), &this->deps_);
        weak_iterator<Derived> end(this->deps_.end(), &this->deps_);

        for(; cur != end; ++cur)
            ;
    }

    //{{AFX_DEBUG
    #ifdef BOOST_XPRESSIVE_DEBUG_TRACKING_POINTER
    void dump_(std::ostream &sout) const;
    #endif
    //}}AFX_DEBUG

    references_type refs_;
    dependents_type deps_;
    shared_ptr<Derived> self_;
    boost::detail::atomic_count cnt_;
};

template<typename Derived>
inline void intrusive_ptr_add_ref(enable_reference_tracking<Derived> *p)
{
    p->add_ref();
}

template<typename Derived>
inline void intrusive_ptr_release(enable_reference_tracking<Derived> *p)
{
    p->release();
}

//{{AFX_DEBUG
#ifdef BOOST_XPRESSIVE_DEBUG_TRACKING_POINTER
///////////////////////////////////////////////////////////////////////////////
// dump_
//
template<typename Derived>
inline void enable_reference_tracking<Derived>::dump_(std::ostream &sout) const
{
    shared_ptr<Derived> this_ = this->self_;
    sout << "0x" << (void*)this << " cnt=" << this_.use_count()-1 << " refs={";
    typename references_type::const_iterator cur1 = this->refs_.begin();
    typename references_type::const_iterator end1 = this->refs_.end();
    for(; cur1 != end1; ++cur1)
    {
        sout << "0x" << (void*)&**cur1 << ',';
    }
    sout << "} deps={";
    typename dependents_type::const_iterator cur2 = this->deps_.begin();
    typename dependents_type::const_iterator end2 = this->deps_.end();
    for(; cur2 != end2; ++cur2)
    {
        // ericne, 27/nov/05: CW9_4 doesn't like if(shared_ptr x = y)
        shared_ptr<Derived> dep = cur2->lock();
        if(dep.get())
        {
            sout << "0x" << (void*)&*dep << ',';
        }
    }
    sout << '}';
}
#endif
//}}AFX_DEBUG

///////////////////////////////////////////////////////////////////////////////
// tracking_ptr
//   holder for a reference-tracked type. Does cycle-breaking, lazy initialization
//   and copy-on-write. TODO: implement move semantics.
//
template<typename Type>
struct tracking_ptr
{
    BOOST_MPL_ASSERT((is_base_and_derived<enable_reference_tracking<Type>, Type>));
    typedef Type element_type;

    tracking_ptr()
      : impl_()
    {
    }

    tracking_ptr(tracking_ptr<element_type> const &that)
      : impl_()
    {
        this->operator =(that);
    }

    tracking_ptr<element_type> &operator =(tracking_ptr<element_type> const &that)
    {
        // Note: the copy-and-swap idiom doesn't work here if has_deps_()==true
        // because it invalidates references to the element_type object.
        if(this != &that)
        {
            if(that)
            {
                if(that.has_deps_() || this->has_deps_())
                {
                    this->fork_(); // deep copy, forks data if necessary
                    this->impl_->tracking_copy(*that);
                }
                else
                {
                    this->impl_ = that.impl_; // shallow, copy-on-write
                }
            }
            else if(*this)
            {
                this->impl_->tracking_clear();
            }
        }
        return *this;
    }

    // NOTE: this does *not* do tracking. Can't provide a non-throwing swap that tracks references
    void swap(tracking_ptr<element_type> &that) // throw()
    {
        this->impl_.swap(that.impl_);
    }

    // calling this forces this->impl_ to fork.
    shared_ptr<element_type> const &get() const
    {
        if(intrusive_ptr<element_type> impl = this->fork_())
        {
            this->impl_->tracking_copy(*impl);
        }
        return this->impl_->self_;
    }

    // smart-pointer operators
    #if defined(__SUNPRO_CC) && BOOST_WORKAROUND(__SUNPRO_CC, <= 0x530)

    operator bool() const
    {
        return this->impl_;
    }

    #else

    typedef intrusive_ptr<element_type> tracking_ptr::* unspecified_bool_type;

    operator unspecified_bool_type() const
    {
        return this->impl_ ? &tracking_ptr::impl_ : 0;
    }

    #endif

    bool operator !() const
    {
        return !this->impl_;
    }

    // Since this does not un-share the data, it returns a ptr-to-const
    element_type const *operator ->() const
    {
        return get_pointer(this->impl_);
    }

    // Since this does not un-share the data, it returns a ref-to-const
    element_type const &operator *() const
    {
        return *this->impl_;
    }

private:

    // calling this forces impl_ to fork.
    intrusive_ptr<element_type> fork_() const
    {
        intrusive_ptr<element_type> impl;
        if(!this->impl_ || 1 != this->impl_->use_count())
        {
            impl = this->impl_;
            BOOST_ASSERT(!this->has_deps_());
            shared_ptr<element_type> simpl(new element_type);
            this->impl_ = get_pointer(simpl->self_ = simpl);
        }
        return impl;
    }

    // does anybody have a dependency on us?
    bool has_deps_() const
    {
        return this->impl_ && this->impl_->has_deps_();
    }

    // mutable to allow lazy initialization
    mutable intrusive_ptr<element_type> impl_;
};

}}} // namespace boost::xpressive::detail

#endif

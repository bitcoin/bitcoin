/*=============================================================================
    Copyright (c) 2001, Daniel C. Nuffer
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_ITERATOR_MULTI_PASS_HPP
#define BOOST_SPIRIT_ITERATOR_MULTI_PASS_HPP

#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <deque>
#include <iterator>
#include <iostream>
#include <algorithm>    // for std::swap
#include <exception>    // for std::exception
#include <boost/limits.hpp>

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/assert.hpp> // for BOOST_SPIRIT_ASSERT
#include <boost/spirit/home/classic/iterator/fixed_size_queue.hpp>

#include <boost/spirit/home/classic/iterator/multi_pass_fwd.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

namespace impl {
    template <typename T>
    inline void mp_swap(T& t1, T& t2);
}

namespace multi_pass_policies
{

///////////////////////////////////////////////////////////////////////////////
// class ref_counted
// Implementation of an OwnershipPolicy used by multi_pass.
//
// Implementation modified from RefCounted class from the Loki library by
// Andrei Alexandrescu
///////////////////////////////////////////////////////////////////////////////
class ref_counted
{
    protected:
        ref_counted()
            : count(new std::size_t(1))
        {}

        ref_counted(ref_counted const& x)
            : count(x.count)
        {}

        // clone is called when a copy of the iterator is made, so increment
        // the ref-count.
        void clone()
        {
            ++*count;
        }

        // called when a copy is deleted.  Decrement the ref-count.  Return
        // value of true indicates that the last copy has been released.
        bool release()
        {
            if (!--*count)
            {
                delete count;
                count = 0;
                return true;
            }
            return false;
        }

        void swap(ref_counted& x)
        {
            impl::mp_swap(count, x.count);
        }

    public:
        // returns true if there is only one iterator in existence.
        // std_deque StoragePolicy will free it's buffered data if this
        // returns true.
        bool unique() const
        {
            return *count == 1;
        }

    private:
        std::size_t* count;
};

///////////////////////////////////////////////////////////////////////////////
// class first_owner
// Implementation of an OwnershipPolicy used by multi_pass
// This ownership policy dictates that the first iterator created will
// determine the lifespan of the shared components.  This works well for
// spirit, since no dynamic allocation of iterators is done, and all copies
// are make on the stack.
//
// There is a caveat about using this policy together with the std_deque
// StoragePolicy. Since first_owner always returns false from unique(),
// std_deque will only release the queued data if clear_queue() is called.
///////////////////////////////////////////////////////////////////////////////
class first_owner
{
    protected:
        first_owner()
            : first(true)
        {}

        first_owner(first_owner const&)
            : first(false)
        {}

        void clone()
        {
        }

        // return true to indicate deletion of resources
        bool release()
        {
            return first;
        }

        void swap(first_owner&)
        {
            // if we're the first, we still remain the first, even if assigned
            // to, so don't swap first_.  swap is only called from operator=
        }

    public:
        bool unique() const
        {
            return false; // no way to know, so always return false
        }

    private:
        bool first;
};

///////////////////////////////////////////////////////////////////////////////
// class illegal_backtracking
// thrown by buf_id_check CheckingPolicy if an instance of an iterator is
// used after another one has invalidated the queue
///////////////////////////////////////////////////////////////////////////////
class BOOST_SYMBOL_VISIBLE illegal_backtracking : public std::exception
{
public:

    illegal_backtracking() BOOST_NOEXCEPT_OR_NOTHROW {}
    ~illegal_backtracking() BOOST_NOEXCEPT_OR_NOTHROW BOOST_OVERRIDE {}

    const char*
    what() const BOOST_NOEXCEPT_OR_NOTHROW BOOST_OVERRIDE
    { return "BOOST_SPIRIT_CLASSIC_NS::illegal_backtracking"; }
};

///////////////////////////////////////////////////////////////////////////////
// class buf_id_check
// Implementation of the CheckingPolicy used by multi_pass
// This policy is most effective when used together with the std_deque
// StoragePolicy.
// If used with the fixed_size_queue StoragePolicy, it will not detect
// iterator dereferences that are out of the range of the queue.
///////////////////////////////////////////////////////////////////////////////
class buf_id_check
{
    protected:
        buf_id_check()
            : shared_buf_id(new unsigned long(0))
            , buf_id(0)
        {}

        buf_id_check(buf_id_check const& x)
            : shared_buf_id(x.shared_buf_id)
            , buf_id(x.buf_id)
        {}

        // will be called from the destructor of the last iterator.
        void destroy()
        {
            delete shared_buf_id;
            shared_buf_id = 0;
        }

        void swap(buf_id_check& x)
        {
            impl::mp_swap(shared_buf_id, x.shared_buf_id);
            impl::mp_swap(buf_id, x.buf_id);
        }

        // called to verify that everything is okay.
        void check_if_valid() const
        {
            if (buf_id != *shared_buf_id)
            {
                boost::throw_exception(illegal_backtracking());
            }
        }

        // called from multi_pass::clear_queue, so we can increment the count
        void clear_queue()
        {
            ++*shared_buf_id;
            ++buf_id;
        }

    private:
        unsigned long* shared_buf_id;
        unsigned long buf_id;
};

///////////////////////////////////////////////////////////////////////////////
// class no_check
// Implementation of the CheckingPolicy used by multi_pass
// It does not do anything :-)
///////////////////////////////////////////////////////////////////////////////
class no_check
{
    protected:
        no_check() {}
        no_check(no_check const&) {}
        void destroy() {}
        void swap(no_check&) {}
        void check_if_valid() const {}
        void clear_queue() {}
};

///////////////////////////////////////////////////////////////////////////////
// class std_deque
// Implementation of the StoragePolicy used by multi_pass
// This stores all data in a std::deque, and keeps an offset to the current
// position. It stores all the data unless there is only one
// iterator using the queue.
// Note: a position is used instead of an iterator, because a push_back on
// a deque can invalidate any iterators.
///////////////////////////////////////////////////////////////////////////////
class std_deque
{
    public:

template <typename ValueT>
class inner
{
    private:

        typedef std::deque<ValueT> queue_type;
        queue_type* queuedElements;
        mutable typename queue_type::size_type queuePosition;

    protected:
        inner()
            : queuedElements(new queue_type)
            , queuePosition(0)
        {}

        inner(inner const& x)
            : queuedElements(x.queuedElements)
            , queuePosition(x.queuePosition)
        {}

        // will be called from the destructor of the last iterator.
        void destroy()
        {
            BOOST_SPIRIT_ASSERT(NULL != queuedElements);
            delete queuedElements;
            queuedElements = 0;
        }

        void swap(inner& x)
        {
            impl::mp_swap(queuedElements, x.queuedElements);
            impl::mp_swap(queuePosition, x.queuePosition);
        }

        // This is called when the iterator is dereferenced.  It's a template
        // method so we can recover the type of the multi_pass iterator
        // and call unique and access the m_input data member.
        template <typename MultiPassT>
        static typename MultiPassT::reference dereference(MultiPassT const& mp)
        {
            if (mp.queuePosition == mp.queuedElements->size())
            {
                // check if this is the only iterator
                if (mp.unique())
                {
                    // free up the memory used by the queue.
                    if (mp.queuedElements->size() > 0)
                    {
                        mp.queuedElements->clear();
                        mp.queuePosition = 0;
                    }
                }
                return mp.get_input();
            }
            else
            {
                return (*mp.queuedElements)[mp.queuePosition];
            }
        }

        // This is called when the iterator is incremented.  It's a template
        // method so we can recover the type of the multi_pass iterator
        // and call unique and access the m_input data member.
        template <typename MultiPassT>
        static void increment(MultiPassT& mp)
        {
            if (mp.queuePosition == mp.queuedElements->size())
            {
                // check if this is the only iterator
                if (mp.unique())
                {
                    // free up the memory used by the queue.
                    if (mp.queuedElements->size() > 0)
                    {
                        mp.queuedElements->clear();
                        mp.queuePosition = 0;
                    }
                }
                else
                {
                    mp.queuedElements->push_back(mp.get_input());
                    ++mp.queuePosition;
                }
                mp.advance_input();
            }
            else
            {
                ++mp.queuePosition;
            }

        }

        // called to forcibly clear the queue
        void clear_queue()
        {
            queuedElements->clear();
            queuePosition = 0;
        }

        // called to determine whether the iterator is an eof iterator
        template <typename MultiPassT>
        static bool is_eof(MultiPassT const& mp)
        {
            return mp.queuePosition == mp.queuedElements->size() &&
                mp.input_at_eof();
        }

        // called by operator==
        bool equal_to(inner const& x) const
        {
            return queuePosition == x.queuePosition;
        }

        // called by operator<
        bool less_than(inner const& x) const
        {
            return queuePosition < x.queuePosition;
        }
}; // class inner

}; // class std_deque


///////////////////////////////////////////////////////////////////////////////
// class fixed_size_queue
// Implementation of the StoragePolicy used by multi_pass
// fixed_size_queue keeps a circular buffer (implemented by
// BOOST_SPIRIT_CLASSIC_NS::fixed_size_queue class) that is size N+1 and stores N elements.
// It is up to the user to ensure that there is enough look ahead for their
// grammar.  Currently there is no way to tell if an iterator is pointing
// to forgotten data.  The leading iterator will put an item in the queue
// and remove one when it is incremented.  No dynamic allocation is done,
// except on creation of the queue (fixed_size_queue constructor).
///////////////////////////////////////////////////////////////////////////////
template < std::size_t N>
class fixed_size_queue
{
    public:

template <typename ValueT>
class inner
{
    private:

        typedef BOOST_SPIRIT_CLASSIC_NS::fixed_size_queue<ValueT, N> queue_type;
        queue_type * queuedElements;
        mutable typename queue_type::iterator queuePosition;

    protected:
        inner()
            : queuedElements(new queue_type)
            , queuePosition(queuedElements->begin())
        {}

        inner(inner const& x)
            : queuedElements(x.queuedElements)
            , queuePosition(x.queuePosition)
        {}

        // will be called from the destructor of the last iterator.
        void destroy()
        {
            BOOST_SPIRIT_ASSERT(NULL != queuedElements);
            delete queuedElements;
            queuedElements = 0;
        }

        void swap(inner& x)
        {
            impl::mp_swap(queuedElements, x.queuedElements);
            impl::mp_swap(queuePosition, x.queuePosition);
        }

        // This is called when the iterator is dereferenced.  It's a template
        // method so we can recover the type of the multi_pass iterator
        // and access the m_input data member.
        template <typename MultiPassT>
        static typename MultiPassT::reference dereference(MultiPassT const& mp)
        {
            if (mp.queuePosition == mp.queuedElements->end())
            {
                return mp.get_input();
            }
            else
            {
                return *mp.queuePosition;
            }
        }

        // This is called when the iterator is incremented.  It's a template
        // method so we can recover the type of the multi_pass iterator
        // and access the m_input data member.
        template <typename MultiPassT>
        static void increment(MultiPassT& mp)
        {
            if (mp.queuePosition == mp.queuedElements->end())
            {
                // don't let the queue get larger than N
                if (mp.queuedElements->size() >= N)
                    mp.queuedElements->pop_front();

                mp.queuedElements->push_back(mp.get_input());
                mp.advance_input();
            }
            ++mp.queuePosition;
        }

        // no-op
        void clear_queue()
        {}

        // called to determine whether the iterator is an eof iterator
        template <typename MultiPassT>
        static bool is_eof(MultiPassT const& mp)
        {
            return mp.queuePosition == mp.queuedElements->end() &&
                mp.input_at_eof();
        }

        // called by operator==
        bool equal_to(inner const& x) const
        {
            return queuePosition == x.queuePosition;
        }

        // called by operator<
        bool less_than(inner const& x) const
        {
            return queuePosition < x.queuePosition;
        }
}; // class inner

}; // class fixed_size_queue


///////////////////////////////////////////////////////////////////////////////
// class input_iterator
// Implementation of the InputPolicy used by multi_pass
// input_iterator encapsulates an input iterator of type InputT
///////////////////////////////////////////////////////////////////////////////
class input_iterator
{
    public:

template <typename InputT>
class inner
{
    private:
        typedef
            typename std::iterator_traits<InputT>::value_type
            result_type;

    public:
        typedef result_type value_type;

    private:
        struct Data {
            Data(InputT const &input_) 
            :   input(input_), was_initialized(false)
            {}
            
            InputT input;
            value_type curtok;
            bool was_initialized;
        };

       // Needed by compilers not implementing the resolution to DR45. For
       // reference, see
       // http://www.open-std.org/JTC1/SC22/WG21/docs/cwg_defects.html#45.

       friend struct Data;

    public:
        typedef
            typename std::iterator_traits<InputT>::difference_type
            difference_type;
        typedef
            typename std::iterator_traits<InputT>::pointer
            pointer;
        typedef
            typename std::iterator_traits<InputT>::reference
            reference;

    protected:
        inner()
            : data(0)
        {}

        inner(InputT x)
            : data(new Data(x))
        {}

        inner(inner const& x)
            : data(x.data)
        {}

        void destroy()
        {
            delete data;
            data = 0;
        }

        bool same_input(inner const& x) const
        {
            return data == x.data;
        }

        typedef
            typename std::iterator_traits<InputT>::value_type
            value_t;
        void swap(inner& x)
        {
            impl::mp_swap(data, x.data);
        }

        void ensure_initialized() const
        {
            if (data && !data->was_initialized) {
                data->curtok = *data->input;      // get the first token
                data->was_initialized = true;
            }
        }

    public:
        reference get_input() const
        {
            BOOST_SPIRIT_ASSERT(NULL != data);
            ensure_initialized();
            return data->curtok;
        }

        void advance_input()
        {
            BOOST_SPIRIT_ASSERT(NULL != data);
            data->was_initialized = false;        // should get the next token
            ++data->input;
        }

        bool input_at_eof() const
        {
            return !data || data->input == InputT();
        }

    private:
        Data *data;
};

};

///////////////////////////////////////////////////////////////////////////////
// class lex_input
// Implementation of the InputPolicy used by multi_pass
// lex_input gets tokens (ints) from yylex()
///////////////////////////////////////////////////////////////////////////////
class lex_input
{
    public:

template <typename InputT>
class inner
{
    public:
        typedef int value_type;
    typedef std::ptrdiff_t difference_type;
        typedef int* pointer;
        typedef int& reference;

    protected:
        inner()
            : curtok(new int(0))
        {}

        inner(InputT x)
            : curtok(new int(x))
        {}

        inner(inner const& x)
            : curtok(x.curtok)
        {}

        void destroy()
        {
            delete curtok;
            curtok = 0;
        }

        bool same_input(inner const& x) const
        {
            return curtok == x.curtok;
        }

        void swap(inner& x)
        {
            impl::mp_swap(curtok, x.curtok);
        }

    public:
        reference get_input() const
        {
            return *curtok;
        }

        void advance_input()
        {
            extern int yylex();
            *curtok = yylex();
        }

        bool input_at_eof() const
        {
            return *curtok == 0;
        }

    private:
        int* curtok;

};

};

///////////////////////////////////////////////////////////////////////////////
// class functor_input
// Implementation of the InputPolicy used by multi_pass
// functor_input gets tokens from a functor
// Note: the functor must have a typedef for result_type
// It also must have a static variable of type result_type defined to
// represent eof that is called eof.
///////////////////////////////////////////////////////////////////////////////
class functor_input
{
    public:

template <typename FunctorT>
class inner
{
    typedef typename FunctorT::result_type result_type;
    public:
        typedef result_type value_type;
    typedef std::ptrdiff_t difference_type;
        typedef result_type* pointer;
        typedef result_type& reference;

    protected:
        inner()
            : ftor(0)
            , curtok(0)
        {}

        inner(FunctorT const& x)
            : ftor(new FunctorT(x))
            , curtok(new result_type((*ftor)()))
        {}

        inner(inner const& x)
            : ftor(x.ftor)
            , curtok(x.curtok)
        {}

        void destroy()
        {
            delete ftor;
            ftor = 0;
            delete curtok;
            curtok = 0;
        }

        bool same_input(inner const& x) const
        {
            return ftor == x.ftor;
        }

        void swap(inner& x)
        {
            impl::mp_swap(curtok, x.curtok);
            impl::mp_swap(ftor, x.ftor);
        }

    public:
        reference get_input() const
        {
            return *curtok;
        }

        void advance_input()
        {
            if (curtok) {
                *curtok = (*ftor)();
            }
        }

        bool input_at_eof() const
        {
            return !curtok || *curtok == ftor->eof;
        }

        FunctorT& get_functor() const
        {
            return *ftor;
        }


    private:
        FunctorT* ftor;
        result_type* curtok;

};

};

} // namespace multi_pass_policies

///////////////////////////////////////////////////////////////////////////////
// iterator_base_creator
///////////////////////////////////////////////////////////////////////////////

namespace iterator_ { namespace impl {

// Meta-function to generate a std::iterator<>-like base class for multi_pass.
template <typename InputPolicyT, typename InputT>
struct iterator_base_creator
{
    typedef typename InputPolicyT::BOOST_NESTED_TEMPLATE inner<InputT> input_t;

    struct type {
        typedef std::forward_iterator_tag iterator_category;
        typedef typename input_t::value_type value_type;
        typedef typename input_t::difference_type difference_type;
        typedef typename input_t::pointer pointer;
        typedef typename input_t::reference reference;
    };
};

}}

///////////////////////////////////////////////////////////////////////////////
// class template multi_pass 
///////////////////////////////////////////////////////////////////////////////

// The default multi_pass instantiation uses a ref-counted std_deque scheme.
template
<
    typename InputT,
    typename InputPolicy,
    typename OwnershipPolicy,
    typename CheckingPolicy,
    typename StoragePolicy
>
class multi_pass
    : public OwnershipPolicy
    , public CheckingPolicy
    , public StoragePolicy::template inner<
                typename InputPolicy::template inner<InputT>::value_type>
    , public InputPolicy::template inner<InputT>
    , public iterator_::impl::iterator_base_creator<InputPolicy, InputT>::type
{
        typedef OwnershipPolicy OP;
        typedef CheckingPolicy CHP;
        typedef typename StoragePolicy::template inner<
            typename InputPolicy::template inner<InputT>::value_type> SP;
        typedef typename InputPolicy::template inner<InputT> IP;
        typedef typename
            iterator_::impl::iterator_base_creator<InputPolicy, InputT>::type
            IB;

    public:
        typedef typename IB::value_type value_type;
        typedef typename IB::difference_type difference_type;
        typedef typename IB::reference reference;
        typedef typename IB::pointer pointer;
        typedef InputT iterator_type;

        multi_pass();
        explicit multi_pass(InputT input);

#if BOOST_WORKAROUND(__GLIBCPP__, == 20020514)
        multi_pass(int);
#endif // BOOST_WORKAROUND(__GLIBCPP__, == 20020514)

        ~multi_pass();

        multi_pass(multi_pass const&);
        multi_pass& operator=(multi_pass const&);

        void swap(multi_pass& x);

        reference operator*() const;
        pointer operator->() const;
        multi_pass& operator++();
        multi_pass operator++(int);

        void clear_queue();

        bool operator==(const multi_pass& y) const;
        bool operator<(const multi_pass& y) const;

    private: // helper functions
        bool is_eof() const;
};

template
<
    typename InputT,
    typename InputPolicy,
    typename OwnershipPolicy,
    typename CheckingPolicy,
    typename StoragePolicy
>
inline
multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy, StoragePolicy>::
multi_pass()
    : OP()
    , CHP()
    , SP()
    , IP()
{
}

template
<
    typename InputT,
    typename InputPolicy,
    typename OwnershipPolicy,
    typename CheckingPolicy,
    typename StoragePolicy
>
inline
multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy, StoragePolicy>::
multi_pass(InputT input)
    : OP()
    , CHP()
    , SP()
    , IP(input)
{
}

#if BOOST_WORKAROUND(__GLIBCPP__, == 20020514)
    // The standard library shipped with gcc-3.1 has a bug in
    // bits/basic_string.tcc. It tries  to use iter::iter(0) to
    // construct an iterator. Ironically, this  happens in sanity
    // checking code that isn't required by the standard.
    // The workaround is to provide an additional constructor that
    // ignores its int argument and behaves like the default constructor.
template
<
    typename InputT,
    typename InputPolicy,
    typename OwnershipPolicy,
    typename CheckingPolicy,
    typename StoragePolicy
>
inline
multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy, StoragePolicy>::
multi_pass(int)
    : OP()
    , CHP()
    , SP()
    , IP()
{
}
#endif // BOOST_WORKAROUND(__GLIBCPP__, == 20020514)

template
<
    typename InputT,
    typename InputPolicy,
    typename OwnershipPolicy,
    typename CheckingPolicy,
    typename StoragePolicy
>
inline
multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy, StoragePolicy>::
~multi_pass()
{
    if (OP::release())
    {
        CHP::destroy();
        SP::destroy();
        IP::destroy();
    }
}

template
<
    typename InputT,
    typename InputPolicy,
    typename OwnershipPolicy,
    typename CheckingPolicy,
    typename StoragePolicy
>
inline
multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy, StoragePolicy>::
multi_pass(
        multi_pass const& x)
    : OP(x)
    , CHP(x)
    , SP(x)
    , IP(x)
{
    OP::clone();
}

template
<
    typename InputT,
    typename InputPolicy,
    typename OwnershipPolicy,
    typename CheckingPolicy,
    typename StoragePolicy
>
inline
multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy, StoragePolicy>&
multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy, StoragePolicy>::
operator=(
        multi_pass const& x)
{
    multi_pass temp(x);
    temp.swap(*this);
    return *this;
}

template
<
    typename InputT,
    typename InputPolicy,
    typename OwnershipPolicy,
    typename CheckingPolicy,
    typename StoragePolicy
>
inline void
multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy, StoragePolicy>::
swap(multi_pass& x)
{
    OP::swap(x);
    CHP::swap(x);
    SP::swap(x);
    IP::swap(x);
}

template
<
    typename InputT,
    typename InputPolicy,
    typename OwnershipPolicy,
    typename CheckingPolicy,
    typename StoragePolicy
>
inline
typename multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy, StoragePolicy>::
reference
multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy, StoragePolicy>::
operator*() const
{
    CHP::check_if_valid();
    return SP::dereference(*this);
}

template
<
    typename InputT,
    typename InputPolicy,
    typename OwnershipPolicy,
    typename CheckingPolicy,
    typename StoragePolicy
>
inline
typename multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy, StoragePolicy>::
pointer
multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy, StoragePolicy>::
operator->() const
{
    return &(operator*());
}

template
<
    typename InputT,
    typename InputPolicy,
    typename OwnershipPolicy,
    typename CheckingPolicy,
    typename StoragePolicy
>
inline
multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy, StoragePolicy>&
multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy, StoragePolicy>::
operator++()
{
    CHP::check_if_valid();
    SP::increment(*this);
    return *this;
}

template
<
    typename InputT,
    typename InputPolicy,
    typename OwnershipPolicy,
    typename CheckingPolicy,
    typename StoragePolicy
>
inline
multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy, StoragePolicy>
multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy, StoragePolicy>::
operator++(int)
{
    multi_pass
    <
        InputT,
        InputPolicy,
        OwnershipPolicy,
        CheckingPolicy,
        StoragePolicy
    > tmp(*this);

    ++*this;

    return tmp;
}

template
<
    typename InputT,
    typename InputPolicy,
    typename OwnershipPolicy,
    typename CheckingPolicy,
    typename StoragePolicy
>
inline void
multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy, StoragePolicy>::
clear_queue()
{
    SP::clear_queue();
    CHP::clear_queue();
}

template
<
    typename InputT,
    typename InputPolicy,
    typename OwnershipPolicy,
    typename CheckingPolicy,
    typename StoragePolicy
>
inline bool
multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy, StoragePolicy>::
is_eof() const
{
    return SP::is_eof(*this);
}

///// Comparisons
template
<
    typename InputT,
    typename InputPolicy,
    typename OwnershipPolicy,
    typename CheckingPolicy,
    typename StoragePolicy
>
inline bool
multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy, StoragePolicy>::
operator==(const multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy,
        StoragePolicy>& y) const
{
    bool is_eof_ = SP::is_eof(*this);
    bool y_is_eof_ = SP::is_eof(y);
    
    if (is_eof_ && y_is_eof_)
    {
        return true;  // both are EOF
    }
    else if (is_eof_ ^ y_is_eof_)
    {
        return false; // one is EOF, one isn't
    }
    else if (!IP::same_input(y))
    {
        return false;
    }
    else
    {
        return SP::equal_to(y);
    }
}

template
<
    typename InputT,
    typename InputPolicy,
    typename OwnershipPolicy,
    typename CheckingPolicy,
    typename StoragePolicy
>
inline bool
multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy, StoragePolicy>::
operator<(const multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy,
        StoragePolicy>& y) const
{
    return SP::less_than(y);
}

template
<
    typename InputT,
    typename InputPolicy,
    typename OwnershipPolicy,
    typename CheckingPolicy,
    typename StoragePolicy
>
inline
bool operator!=(
        const multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy,
                        StoragePolicy>& x,
        const multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy,
                        StoragePolicy>& y)
{
    return !(x == y);
}

template
<
    typename InputT,
    typename InputPolicy,
    typename OwnershipPolicy,
    typename CheckingPolicy,
    typename StoragePolicy
>
inline
bool operator>(
        const multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy,
                        StoragePolicy>& x,
        const multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy,
                        StoragePolicy>& y)
{
    return y < x;
}

template
<
    typename InputT,
    typename InputPolicy,
    typename OwnershipPolicy,
    typename CheckingPolicy,
    typename StoragePolicy
>
inline
bool operator>=(
        const multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy,
                        StoragePolicy>& x,
        const multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy,
                        StoragePolicy>& y)
{
    return !(x < y);
}

template
<
    typename InputT,
    typename InputPolicy,
    typename OwnershipPolicy,
    typename CheckingPolicy,
    typename StoragePolicy
>
inline
bool operator<=(
        const multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy,
                        StoragePolicy>& x,
        const multi_pass<InputT, InputPolicy, OwnershipPolicy, CheckingPolicy,
                        StoragePolicy>& y)
{
    return !(y < x);
}

///// Generator function
template <typename InputT>
inline multi_pass<InputT, 
    multi_pass_policies::input_iterator, multi_pass_policies::ref_counted,
    multi_pass_policies::buf_id_check, multi_pass_policies::std_deque>
make_multi_pass(InputT i)
{
    return multi_pass<InputT, 
        multi_pass_policies::input_iterator, multi_pass_policies::ref_counted,
        multi_pass_policies::buf_id_check, multi_pass_policies::std_deque>(i);
}

// this could be a template typedef, since such a thing doesn't
// exist in C++, we'll use inheritance to accomplish the same thing.

template <typename InputT, std::size_t N>
class look_ahead :
    public multi_pass<
        InputT,
        multi_pass_policies::input_iterator,
        multi_pass_policies::first_owner,
        multi_pass_policies::no_check,
        multi_pass_policies::fixed_size_queue<N> >
{
        typedef multi_pass<
            InputT,
            multi_pass_policies::input_iterator,
            multi_pass_policies::first_owner,
            multi_pass_policies::no_check,
            multi_pass_policies::fixed_size_queue<N> > base_t;
    public:
        look_ahead()
            : base_t() {}

        explicit look_ahead(InputT x)
            : base_t(x) {}

        look_ahead(look_ahead const& x)
            : base_t(x) {}

#if BOOST_WORKAROUND(__GLIBCPP__, == 20020514)
        look_ahead(int)         // workaround for a bug in the library
            : base_t() {}       // shipped with gcc 3.1
#endif // BOOST_WORKAROUND(__GLIBCPP__, == 20020514)

    // default generated operators destructor and assignment operator are okay.
};

template
<
    typename InputT,
    typename InputPolicy,
    typename OwnershipPolicy,
    typename CheckingPolicy,
    typename StoragePolicy
>
void swap(
    multi_pass<
        InputT, InputPolicy, OwnershipPolicy, CheckingPolicy, StoragePolicy
    > &x,
    multi_pass<
        InputT, InputPolicy, OwnershipPolicy, CheckingPolicy, StoragePolicy
    > &y)
{
    x.swap(y);
}

namespace impl {

    template <typename T>
    inline void mp_swap(T& t1, T& t2)
    {
        using std::swap;
        using BOOST_SPIRIT_CLASSIC_NS::swap;
        swap(t1, t2);
    }
}

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif // BOOST_SPIRIT_ITERATOR_MULTI_PASS_HPP

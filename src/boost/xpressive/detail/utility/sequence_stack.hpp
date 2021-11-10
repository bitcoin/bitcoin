///////////////////////////////////////////////////////////////////////////////
// sequence_stack.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_SEQUENCE_STACK_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_SEQUENCE_STACK_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
# pragma warning(push)
# pragma warning(disable : 4127) // conditional expression constant
#endif

#include <cstddef>
#include <algorithm>
#include <functional>

namespace boost { namespace xpressive { namespace detail
{

struct fill_t {} const fill = {};

//////////////////////////////////////////////////////////////////////////
// sequence_stack
//
//   For storing a stack of sequences of type T, where each sequence
//   is guaranteed to be stored in contiguous memory.
template<typename T>
struct sequence_stack
{
    struct allocate_guard_t;
    friend struct allocate_guard_t;
    struct allocate_guard_t
    {
        std::size_t i;
        T *p;
        bool dismissed;
        ~allocate_guard_t()
        {
            if(!this->dismissed)
                sequence_stack::deallocate(this->p, this->i);
        }
    };
private:
    static T *allocate(std::size_t size, T const &t)
    {
        allocate_guard_t guard = {0, (T *)::operator new(size * sizeof(T)), false};

        for(; guard.i < size; ++guard.i)
            ::new((void *)(guard.p + guard.i)) T(t);
        guard.dismissed = true;

        return guard.p;
    }

    static void deallocate(T *p, std::size_t i)
    {
        while(i-- > 0)
            (p+i)->~T();
        ::operator delete(p);
    }

    struct chunk
    {
        chunk(std::size_t size, T const &t, std::size_t count, chunk *back, chunk *next)
          : begin_(allocate(size, t))
          , curr_(begin_ + count)
          , end_(begin_ + size)
          , back_(back)
          , next_(next)
        {
            if(this->back_)
                this->back_->next_ = this;
            if(this->next_)
                this->next_->back_ = this;
        }

        ~chunk()
        {
            deallocate(this->begin_, this->size());
        }

        std::size_t size() const
        {
            return static_cast<std::size_t>(this->end_ - this->begin_);
        }

        T *const begin_, *curr_, *const end_;
        chunk *back_, *next_;

    private:
        chunk &operator =(chunk const &);
    };

    chunk *current_chunk_;

    // Cache these for faster access
    T *begin_;
    T *curr_;
    T *end_;

    T *grow_(std::size_t count, T const &t)
    {
        if(this->current_chunk_)
        {
            // write the cached value of current into the expr.
            // OK to do this even if later statements throw.
            this->current_chunk_->curr_ = this->curr_;

            // Do we have a expr with enough available memory already?
            if(this->current_chunk_->next_ && count <= this->current_chunk_->next_->size())
            {
                this->current_chunk_ = this->current_chunk_->next_;
                this->curr_ = this->current_chunk_->curr_ = this->current_chunk_->begin_ + count;
                this->end_ = this->current_chunk_->end_;
                this->begin_ = this->current_chunk_->begin_;
                std::fill_n(this->begin_, count, t);
                return this->begin_;
            }

            // grow exponentially
            std::size_t new_size = (std::max)(
                count
              , static_cast<std::size_t>(static_cast<double>(this->current_chunk_->size()) * 1.5)
            );

            // Create a new expr and insert it into the list
            this->current_chunk_ = new chunk(new_size, t, count, this->current_chunk_, this->current_chunk_->next_);
        }
        else
        {
            // first chunk is 256
            std::size_t new_size = (std::max)(count, static_cast<std::size_t>(256U));

            // Create a new expr and insert it into the list
            this->current_chunk_ = new chunk(new_size, t, count, 0, 0);
        }

        this->begin_ = this->current_chunk_->begin_;
        this->curr_ = this->current_chunk_->curr_;
        this->end_ = this->current_chunk_->end_;
        return this->begin_;
    }

    void unwind_chunk_()
    {
        // write the cached value of curr_ into current_chunk_
        this->current_chunk_->curr_ = this->begin_;
        // make the previous chunk the current
        this->current_chunk_ = this->current_chunk_->back_;

        // update the cache
        this->begin_ = this->current_chunk_->begin_;
        this->curr_ = this->current_chunk_->curr_;
        this->end_ = this->current_chunk_->end_;
    }

    bool in_current_chunk(T *ptr) const
    {
        return !std::less<void*>()(ptr, this->begin_) && std::less<void*>()(ptr, this->end_);
    }

public:
    sequence_stack()
      : current_chunk_(0)
      , begin_(0)
      , curr_(0)
      , end_(0)
    {
    }

    ~sequence_stack()
    {
        this->clear();
    }

    // walk to the front of the linked list
    void unwind()
    {
        if(this->current_chunk_)
        {
            while(this->current_chunk_->back_)
            {
                this->current_chunk_->curr_ = this->current_chunk_->begin_;
                this->current_chunk_ = this->current_chunk_->back_;
            }

            this->begin_ = this->curr_ = this->current_chunk_->curr_ = this->current_chunk_->begin_;
            this->end_ = this->current_chunk_->end_;
        }
    }

    void clear()
    {
        // walk to the front of the list
        this->unwind();

        // delete the list
        for(chunk *next; this->current_chunk_; this->current_chunk_ = next)
        {
            next = this->current_chunk_->next_;
            delete this->current_chunk_;
        }

        this->begin_ = this->curr_ = this->end_ = 0;
    }

    T *push_sequence(std::size_t count, T const &t)
    {
        // Check to see if we have overflowed this buffer
        std::size_t size_left = static_cast< std::size_t >(this->end_ - this->curr_);
        if (size_left < count)
        {
            // allocate a new block and return a ptr to the new memory
            return this->grow_(count, t);
        }

        // This is the ptr to return
        T *ptr = this->curr_;

        // Advance the high-water mark
        this->curr_ += count;

        return ptr;
    }

    T *push_sequence(std::size_t count, T const &t, fill_t)
    {
        T *ptr = this->push_sequence(count, t);
        std::fill_n(ptr, count, t);
        return ptr;
    }

    void unwind_to(T *ptr)
    {
        while(!this->in_current_chunk(ptr))
        {
            // completely unwind the current chunk, move to the previous chunk
            this->unwind_chunk_();
        }
        this->current_chunk_->curr_ = this->curr_ = ptr;
    }

    // shrink-to-fit: remove any unused nodes in the chain
    void conserve()
    {
        if(this->current_chunk_)
        {
            for(chunk *next; this->current_chunk_->next_; this->current_chunk_->next_ = next)
            {
                next = this->current_chunk_->next_->next_;
                delete this->current_chunk_->next_;
            }
        }
    }
};

}}} // namespace boost::xpressive::detail

#if defined(_MSC_VER)
# pragma warning(pop)
#endif

#endif

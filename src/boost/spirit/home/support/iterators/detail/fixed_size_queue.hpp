//  Copyright (c) 2001 Daniel C. Nuffer
//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_ITERATOR_FIXED_SIZE_QUEUE_MAR_16_2007_1137AM)
#define BOOST_SPIRIT_ITERATOR_FIXED_SIZE_QUEUE_MAR_16_2007_1137AM

#include <cstdlib>
#include <iterator>
#include <cstddef>

#include <boost/config.hpp>
#include <boost/assert.hpp>   // for BOOST_ASSERT
#include <boost/iterator_adaptors.hpp>

///////////////////////////////////////////////////////////////////////////////
//  Make sure we're using a decent version of the Boost.IteratorAdaptors lib
#if !defined(BOOST_ITERATOR_ADAPTORS_VERSION) || \
    BOOST_ITERATOR_ADAPTORS_VERSION < 0x0200
#error "Please use at least Boost V1.31.0 while compiling the fixed_size_queue class!"
#endif 

///////////////////////////////////////////////////////////////////////////////
#define BOOST_SPIRIT_ASSERT_FSQ_SIZE                                          \
    BOOST_ASSERT(((m_tail + N + 1) - m_head) % (N+1) == m_size % (N+1))       \
    /**/

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace detail
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Queue, typename T, typename Pointer>
    class fsq_iterator
    :   public boost::iterator_facade<
            fsq_iterator<Queue, T, Pointer>, T,
            std::random_access_iterator_tag
        >
    {
    public:
        typedef typename Queue::position_type position_type;
        typedef boost::iterator_facade<
                fsq_iterator<Queue, T, Pointer>, T,
                std::random_access_iterator_tag
            > base_type;

        fsq_iterator() {}
        fsq_iterator(position_type const &p_) : p(p_) {}

        position_type &get_position() { return p; }
        position_type const &get_position() const { return p; }

    private:
        friend class boost::iterator_core_access;

        typename base_type::reference dereference() const
        {
            return p.self->m_queue[p.pos];
        }

        void increment()
        {
            ++p.pos;
            if (p.pos == Queue::MAX_SIZE+1)
                p.pos = 0;
        }

        void decrement()
        {
            if (p.pos == 0)
                p.pos = Queue::MAX_SIZE;
            else
                --p.pos;
        }

        bool is_eof() const
        {
            return p.self == 0 || p.pos == p.self->size();
        }

        template <typename Q, typename T_, typename P>   
        bool equal(fsq_iterator<Q, T_, P> const &x) const
        {
            if (is_eof())
                return x.is_eof();
            if (x.is_eof())
                return false;

            position_type const &rhs_pos = x.get_position();
            return (p.self == rhs_pos.self) && (p.pos == rhs_pos.pos);
        }

        template <typename Q, typename T_, typename P>   
        typename base_type::difference_type distance_to(
            fsq_iterator<Q, T_, P> const &x) const
        {
            typedef typename base_type::difference_type difference_type;

            position_type const &p2 = x.get_position();
            std::size_t pos1 = p.pos;
            std::size_t pos2 = p2.pos;

            //  Undefined behavior if the iterators come from different
            //  containers
            BOOST_ASSERT(p.self == p2.self);

            if (pos1 < p.self->m_head)
                pos1 += Queue::MAX_SIZE;
            if (pos2 < p2.self->m_head)
                pos2 += Queue::MAX_SIZE;

            if (pos2 > pos1)
                return difference_type(pos2 - pos1);
            else
                return -difference_type(pos1 - pos2);
        }

        void advance(typename base_type::difference_type n)
        {
            //  Notice that we don't care values of n that can
            //  wrap around more than one time, since it would
            //  be undefined behavior anyway (going outside
            //  the begin/end range). Negative wrapping is a bit
            //  cumbersome because we don't want to case p.pos
            //  to signed.
            if (n < 0)
            {
                n = -n;
                if (p.pos < (std::size_t)n)
                    p.pos = Queue::MAX_SIZE+1 - (n - p.pos);
                else
                    p.pos -= n;
            }
            else
            {
                p.pos += n;
                if (p.pos >= Queue::MAX_SIZE+1)
                    p.pos -= Queue::MAX_SIZE+1;
            }
        }

    private:
        position_type p;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, std::size_t N>
    class fixed_size_queue
    {
    private:
        struct position
        {
            fixed_size_queue* self;
            std::size_t pos;

            position() : self(0), pos(0) {}

            // The const_cast here is just to avoid to have two different
            //  position structures for the const and non-const case.
            // The const semantic is guaranteed by the iterator itself
            position(const fixed_size_queue* s, std::size_t p)
              : self(const_cast<fixed_size_queue*>(s)), pos(p)
            {}

            bool is_initialized() const { return self != 0; }
            void set_queue(fixed_size_queue* q) { self = q; }
        };

    public:
        // Declare the iterators
        typedef fsq_iterator<fixed_size_queue<T, N>, T, T*> iterator;
        typedef 
            fsq_iterator<fixed_size_queue<T, N>, T const, T const*> 
        const_iterator;
        typedef position position_type;

        friend class fsq_iterator<fixed_size_queue<T, N>, T, T*>;
        friend class fsq_iterator<fixed_size_queue<T, N>, T const, T const*>;

        fixed_size_queue();
        fixed_size_queue(const fixed_size_queue& x);
        fixed_size_queue& operator=(const fixed_size_queue& x);
        ~fixed_size_queue();

        void push_back(const T& e);
        void push_front(const T& e);
        void serve(T& e);
        void pop_front();

        bool empty() const
        {
            return m_size == 0;
        }

        bool full() const
        {
            return m_size == N;
        }

        iterator begin()
        {
            return iterator(position(this, m_head));
        }

        const_iterator begin() const
        {
            return const_iterator(position(this, m_head));
        }

        iterator end()
        {
            return iterator(position(0, m_tail));
        }

        const_iterator end() const
        {
            return const_iterator(position(0, m_tail));
        }

        std::size_t size() const
        {
            return m_size;
        }

        T& front()
        {
            return m_queue[m_head];
        }

        const T& front() const
        {
            return m_queue[m_head];
        }

    private:
        // Redefine the template parameters to avoid using partial template
        //  specialization on the iterator policy to extract N.
        BOOST_STATIC_CONSTANT(std::size_t, MAX_SIZE = N);

        std::size_t m_head;
        std::size_t m_tail;
        std::size_t m_size;
        T m_queue[N+1];
    };

    template <typename T, std::size_t N>
    inline
    fixed_size_queue<T, N>::fixed_size_queue()
        : m_head(0)
        , m_tail(0)
        , m_size(0)
    {
        BOOST_ASSERT(m_size <= N+1);
        BOOST_SPIRIT_ASSERT_FSQ_SIZE;
        BOOST_ASSERT(m_head <= N+1);
        BOOST_ASSERT(m_tail <= N+1);
    }

    template <typename T, std::size_t N>
    inline
    fixed_size_queue<T, N>::fixed_size_queue(const fixed_size_queue& x)
        : m_head(x.m_head)
        , m_tail(x.m_tail)
        , m_size(x.m_size)
    {
        copy(x.begin(), x.end(), begin());
        BOOST_ASSERT(m_size <= N+1);
        BOOST_SPIRIT_ASSERT_FSQ_SIZE;
        BOOST_ASSERT(m_head <= N+1);
        BOOST_ASSERT(m_tail <= N+1);
    }

    template <typename T, std::size_t N>
    inline fixed_size_queue<T, N>&
    fixed_size_queue<T, N>::operator=(const fixed_size_queue& x)
    {
        if (this != &x)
        {
            m_head = x.m_head;
            m_tail = x.m_tail;
            m_size = x.m_size;
            copy(x.begin(), x.end(), begin());
        }
        BOOST_ASSERT(m_size <= N+1);
        BOOST_SPIRIT_ASSERT_FSQ_SIZE;
        BOOST_ASSERT(m_head <= N+1);
        BOOST_ASSERT(m_tail <= N+1);

        return *this;
    }

    template <typename T, std::size_t N>
    inline
    fixed_size_queue<T, N>::~fixed_size_queue()
    {
        BOOST_ASSERT(m_size <= N+1);
        BOOST_SPIRIT_ASSERT_FSQ_SIZE;
        BOOST_ASSERT(m_head <= N+1);
        BOOST_ASSERT(m_tail <= N+1);
    }

    template <typename T, std::size_t N>
    inline void
    fixed_size_queue<T, N>::push_back(const T& e)
    {
        BOOST_ASSERT(m_size <= N+1);
        BOOST_SPIRIT_ASSERT_FSQ_SIZE;
        BOOST_ASSERT(m_head <= N+1);
        BOOST_ASSERT(m_tail <= N+1);

        BOOST_ASSERT(!full());

        m_queue[m_tail] = e;
        ++m_size;
        ++m_tail;
        if (m_tail == N+1)
            m_tail = 0;


        BOOST_ASSERT(m_size <= N+1);
        BOOST_SPIRIT_ASSERT_FSQ_SIZE;
        BOOST_ASSERT(m_head <= N+1);
        BOOST_ASSERT(m_tail <= N+1);
    }

    template <typename T, std::size_t N>
    inline void
    fixed_size_queue<T, N>::push_front(const T& e)
    {
        BOOST_ASSERT(m_size <= N+1);
        BOOST_SPIRIT_ASSERT_FSQ_SIZE;
        BOOST_ASSERT(m_head <= N+1);
        BOOST_ASSERT(m_tail <= N+1);

        BOOST_ASSERT(!full());

        if (m_head == 0)
            m_head = N;
        else
            --m_head;

        m_queue[m_head] = e;
        ++m_size;

        BOOST_ASSERT(m_size <= N+1);
        BOOST_SPIRIT_ASSERT_FSQ_SIZE;
        BOOST_ASSERT(m_head <= N+1);
        BOOST_ASSERT(m_tail <= N+1);
    }


    template <typename T, std::size_t N>
    inline void
    fixed_size_queue<T, N>::serve(T& e)
    {
        BOOST_ASSERT(m_size <= N+1);
        BOOST_SPIRIT_ASSERT_FSQ_SIZE;
        BOOST_ASSERT(m_head <= N+1);
        BOOST_ASSERT(m_tail <= N+1);

        e = m_queue[m_head];
        pop_front();
    }



    template <typename T, std::size_t N>
    inline void
    fixed_size_queue<T, N>::pop_front()
    {
        BOOST_ASSERT(m_size <= N+1);
        BOOST_SPIRIT_ASSERT_FSQ_SIZE;
        BOOST_ASSERT(m_head <= N+1);
        BOOST_ASSERT(m_tail <= N+1);

        ++m_head;
        if (m_head == N+1)
            m_head = 0;
        --m_size;

        BOOST_ASSERT(m_size <= N+1);
        BOOST_SPIRIT_ASSERT_FSQ_SIZE;
        BOOST_ASSERT(m_head <= N+1);
        BOOST_ASSERT(m_tail <= N+1);
    }

}}} 

#undef BOOST_SPIRIT_ASSERT_FSQ_SIZE

#endif

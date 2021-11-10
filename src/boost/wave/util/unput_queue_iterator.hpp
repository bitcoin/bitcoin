/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Definition of the unput queue iterator

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_UNPUT_QUEUE_ITERATOR_HPP_76DA23D0_4893_4AD5_ABCC_6CED7CFB89BC_INCLUDED)
#define BOOST_UNPUT_QUEUE_ITERATOR_HPP_76DA23D0_4893_4AD5_ABCC_6CED7CFB89BC_INCLUDED

#include <list>

#include <boost/assert.hpp>
#include <boost/iterator_adaptors.hpp>

#include <boost/wave/wave_config.hpp>
#include <boost/wave/token_ids.hpp>     // token_id

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace util {

///////////////////////////////////////////////////////////////////////////////
//
//  unput_queue_iterator
//
//      The unput_queue_iterator templates encapsulates an unput_queue together
//      with the direct input to be read after the unput queue is emptied
//
//      This version is for the new iterator_adaptors (was released with
//      Boost V1.31.0)
//
///////////////////////////////////////////////////////////////////////////////
template <typename IteratorT, typename TokenT, typename ContainerT>
class unput_queue_iterator
:   public boost::iterator_adaptor<
        unput_queue_iterator<IteratorT, TokenT, ContainerT>,
            IteratorT, TokenT const, std::forward_iterator_tag>
{
    typedef boost::iterator_adaptor<
                unput_queue_iterator<IteratorT, TokenT, ContainerT>,
                IteratorT, TokenT const, std::forward_iterator_tag>
        base_type;

public:
    typedef ContainerT  container_type;
    typedef IteratorT   iterator_type;

    unput_queue_iterator(IteratorT const &it, ContainerT &queue)
    :   base_type(it), unput_queue(queue)
    {}

    ContainerT &get_unput_queue()
        { return unput_queue; }
    ContainerT const &get_unput_queue() const
        { return unput_queue; }
    IteratorT &get_base_iterator()
        { return base_type::base_reference(); }
    IteratorT const &get_base_iterator() const
        { return base_type::base_reference(); }

    unput_queue_iterator &operator= (unput_queue_iterator const &rhs)
    {
        if (this != &rhs) {
            unput_queue = rhs.unput_queue;
            base_type::operator=(rhs);
        }
        return *this;
    }

    typename base_type::reference dereference() const
    {
        if (!unput_queue.empty())
            return unput_queue.front();
        return *base_type::base_reference();
    }

    void increment()
    {
        if (!unput_queue.empty()) {
            // there exist pending tokens in the unput queue
            unput_queue.pop_front();
        }
        else {
            // the unput_queue is empty, so advance the base iterator
            ++base_type::base_reference();
        }
    }

    template <
        typename OtherDerivedT, typename OtherIteratorT,
        typename V, typename C, typename R, typename D
    >
    bool equal(
        boost::iterator_adaptor<OtherDerivedT, OtherIteratorT, V, C, R, D>
        const &x) const
    {
    // two iterators are equal, if both begin() iterators of the queue
    // objects are equal and the base iterators are equal as well
        OtherDerivedT const &rhs = static_cast<OtherDerivedT const &>(x);
        return
            ((unput_queue.empty() && rhs.unput_queue.empty()) ||
              (&unput_queue == &rhs.unput_queue &&
               unput_queue.begin() == rhs.unput_queue.begin()
              )
            ) &&
            (get_base_iterator() == rhs.get_base_iterator());
    }

private:
    ContainerT &unput_queue;
};

namespace impl {

    ///////////////////////////////////////////////////////////////////////////
    template <typename IteratorT, typename TokenT, typename ContainerT>
    struct gen_unput_queue_iterator
    {
        typedef ContainerT  container_type;
        typedef IteratorT iterator_type;
        typedef unput_queue_iterator<IteratorT, TokenT, ContainerT>
            return_type;

        static container_type last;

        static return_type
        generate(iterator_type const &it)
        {
            return return_type(it, last);
        }

        static return_type
        generate(ContainerT &queue, iterator_type const &it)
        {
            return return_type(it, queue);
        }
    };

    template <typename IteratorT, typename TokenT, typename ContainerT>
    typename gen_unput_queue_iterator<IteratorT, TokenT, ContainerT>::
          container_type
        gen_unput_queue_iterator<IteratorT, TokenT, ContainerT>::last =
            typename gen_unput_queue_iterator<IteratorT, TokenT, ContainerT>::
                  container_type();

    ///////////////////////////////////////////////////////////////////////////
    template <typename IteratorT, typename TokenT, typename ContainerT>
    struct gen_unput_queue_iterator<
        unput_queue_iterator<IteratorT, TokenT, ContainerT>,
            TokenT, ContainerT>
    {
        typedef ContainerT  container_type;
        typedef unput_queue_iterator<IteratorT, TokenT, ContainerT>
            iterator_type;
        typedef unput_queue_iterator<IteratorT, TokenT, ContainerT>
            return_type;

        static container_type last;

        static return_type
        generate(iterator_type &it)
        {
            return return_type(it.base(), last);
        }

        static return_type
        generate(ContainerT &queue, iterator_type &it)
        {
            return return_type(it.base(), queue);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename IteratorT>
    struct assign_iterator
    {
        static void
        do_ (IteratorT &dest, IteratorT const &src)
        {
            dest = src;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    // Look for the first non-whitespace token and return this token id.
    // Note though, that the embedded unput_queues are not touched in any way!
    //
    template <typename IteratorT>
    struct next_token
    {
        static boost::wave::token_id
        peek(IteratorT it, IteratorT end, bool skip_whitespace = true)
        {
            using namespace boost::wave;
            if (skip_whitespace) {
                for (++it; it != end; ++it) {
                    if (!IS_CATEGORY(*it, WhiteSpaceTokenType) &&
                        T_NEWLINE != token_id(*it))
                    {
                        break;  // stop at the first non-whitespace token
                    }
                }
            }
            else {
                ++it;           // we have at least to look ahead
            }
            if (it != end)
                return token_id(*it);
            return T_EOI;
        }
    };

    template <typename IteratorT, typename TokenT, typename ContainerT>
    struct next_token<
        unput_queue_iterator<IteratorT, TokenT, ContainerT> > {

        typedef unput_queue_iterator<IteratorT, TokenT, ContainerT> iterator_type;

        static boost::wave::token_id
        peek(iterator_type it, iterator_type end, bool skip_whitespace = true)
        {
            using namespace boost::wave;

        typename iterator_type::container_type &queue = it.get_unput_queue();

        // first try to find it in the unput_queue
            if (0 != queue.size()) {
            typename iterator_type::container_type::iterator cit = queue.begin();
            typename iterator_type::container_type::iterator cend = queue.end();

                if (skip_whitespace) {
                    for (++cit; cit != cend; ++cit) {
                        if (!IS_CATEGORY(*cit, WhiteSpaceTokenType) &&
                            T_NEWLINE != token_id(*cit))
                        {
                            break;  // stop at the first non-whitespace token
                        }
                    }
                }
                else {
                    ++cit;          // we have at least to look ahead
                }
                if (cit != cend)
                    return token_id(*cit);
            }

        // second try to move on into the base iterator stream
        typename iterator_type::iterator_type base_it = it.get_base_iterator();
        typename iterator_type::iterator_type base_end = end.get_base_iterator();

            if (0 == queue.size())
                ++base_it;  // advance, if the unput queue is empty

            if (skip_whitespace) {
                for (/**/; base_it != base_end; ++base_it) {
                    if (!IS_CATEGORY(*base_it, WhiteSpaceTokenType) &&
                        T_NEWLINE != token_id(*base_it))
                    {
                        break;  // stop at the first non-whitespace token
                    }
                }
            }
            if (base_it == base_end)
                return T_EOI;

            return token_id(*base_it);
        }
    };

///////////////////////////////////////////////////////////////////////////////
}   // namespace impl

///////////////////////////////////////////////////////////////////////////////
}   // namespace util
}   // namespace wave
}   // namespace boost

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !defined(BOOST_UNPUT_QUEUE_ITERATOR_HPP_76DA23D0_4893_4AD5_ABCC_6CED7CFB89BC_INCLUDED)

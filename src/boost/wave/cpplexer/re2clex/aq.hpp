/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    http://www.boost.org/

    Copyright (c) 2001 Daniel C. Nuffer.
    Copyright (c) 2001-2012 Hartmut Kaiser.
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_AQ_HPP_A21D9145_B643_44C0_81E7_DB346DD67EE1_INCLUDED)
#define BOOST_AQ_HPP_A21D9145_B643_44C0_81E7_DB346DD67EE1_INCLUDED

#include <boost/wave/wave_config.hpp>
#include <cstdlib>

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace cpplexer {
namespace re2clex {

typedef std::size_t aq_stdelement;

typedef struct tag_aq_queuetype
{
    std::size_t head;
    std::size_t tail;
    std::size_t size;
    std::size_t max_size;
    aq_stdelement* queue;
} aq_queuetype;

typedef aq_queuetype* aq_queue;

BOOST_WAVE_DECL int aq_enqueue(aq_queue q, aq_stdelement e);
int aq_enqueue_front(aq_queue q, aq_stdelement e);
int aq_serve(aq_queue q, aq_stdelement *e);
BOOST_WAVE_DECL int aq_pop(aq_queue q);
#define AQ_EMPTY(q) (q->size == 0)
#define AQ_FULL(q) (q->size == q->max_size)
int aq_grow(aq_queue q);

BOOST_WAVE_DECL aq_queue aq_create(void);
BOOST_WAVE_DECL void aq_terminate(aq_queue q);

///////////////////////////////////////////////////////////////////////////////
}   // namespace re2clex
}   // namespace cpplexer
}   // namespace wave
}   // namespace boost

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !defined(BOOST_AQ_HPP_A21D9145_B643_44C0_81E7_DB346DD67EE1_INCLUDED)

/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_ITERATION_CONTEXT_HPP_9556CD16_F11E_4ADC_AC8B_FB9A174BE664_INCLUDED)
#define BOOST_ITERATION_CONTEXT_HPP_9556CD16_F11E_4ADC_AC8B_FB9A174BE664_INCLUDED

#include <cstdlib>
#include <cstdio>
#include <stack>

#include <boost/wave/wave_config.hpp>
#include <boost/wave/cpp_exceptions.hpp>

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace util {

///////////////////////////////////////////////////////////////////////////////
template <typename IterationContextT>
class iteration_context_stack
{
    typedef std::stack<IterationContextT> base_type;

public:
    typedef typename base_type::size_type size_type;

    iteration_context_stack()
    :   max_include_nesting_depth(BOOST_WAVE_MAX_INCLUDE_LEVEL_DEPTH)
    {}

    void set_max_include_nesting_depth(size_type new_depth)
        {  max_include_nesting_depth = new_depth; }
    size_type get_max_include_nesting_depth() const
        { return max_include_nesting_depth; }

    typename base_type::size_type size() const { return iter_ctx.size(); }
    typename base_type::value_type &top() { return iter_ctx.top(); }
    void pop() { iter_ctx.pop(); }

    template <typename Context, typename PositionT>
    void push(Context& ctx, PositionT const &pos,
        typename base_type::value_type const &val)
    {
        if (iter_ctx.size() == max_include_nesting_depth) {
        char buffer[22];    // 21 bytes holds all NUL-terminated unsigned 64-bit numbers

            using namespace std;    // for some systems sprintf is in namespace std
            sprintf(buffer, "%d", (int)max_include_nesting_depth);
            BOOST_WAVE_THROW_CTX(ctx, preprocess_exception,
                include_nesting_too_deep, buffer, pos);
        }
        iter_ctx.push(val);
    }

private:
    size_type max_include_nesting_depth;
    base_type iter_ctx;
};

///////////////////////////////////////////////////////////////////////////////
}   // namespace util
}   // namespace wave
}   // namespace boost

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !defined(BOOST_ITERATION_CONTEXT_HPP_9556CD16_F11E_4ADC_AC8B_FB9A174BE664_INCLUDED)

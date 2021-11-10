/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    Definition of the preprocessor context

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_CPP_ITERATION_CONTEXT_HPP_00312288_9DDB_4668_AFE5_25D3994FD095_INCLUDED)
#define BOOST_CPP_ITERATION_CONTEXT_HPP_00312288_9DDB_4668_AFE5_25D3994FD095_INCLUDED

#include <iterator>
#include <boost/filesystem/fstream.hpp>
#if defined(BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS)
#include <sstream>
#endif

#include <boost/wave/wave_config.hpp>
#include <boost/wave/cpp_exceptions.hpp>
#include <boost/wave/language_support.hpp>
#include <boost/wave/util/file_position.hpp>
// #include <boost/spirit/include/iterator/classic_multi_pass.hpp> // make_multi_pass

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace iteration_context_policies {

///////////////////////////////////////////////////////////////////////////////
//
//      The iteration_context_policies templates are policies for the
//      boost::wave::iteration_context which allows to control, how a given
//      input file is to be represented by a pair of iterators pointing to the
//      begin and the end of the resulting input sequence.
//
///////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    //
    //  load_file_to_string
    //
    //      Loads a file into a string and returns the iterators pointing to
    //      the beginning and the end of the loaded string.
    //
    ///////////////////////////////////////////////////////////////////////////
    struct load_file_to_string
    {
        template <typename IterContextT>
        class inner
        {
        public:
            template <typename PositionT>
            static void init_iterators(IterContextT &iter_ctx,
                PositionT const &act_pos, language_support language)
            {
                typedef typename IterContextT::iterator_type iterator_type;

                // read in the file
                boost::filesystem::ifstream instream(iter_ctx.filename.c_str());
                if (!instream.is_open()) {
                    BOOST_WAVE_THROW_CTX(iter_ctx.ctx, preprocess_exception,
                        bad_include_file, iter_ctx.filename.c_str(), act_pos);
                    return;
                }
                instream.unsetf(std::ios::skipws);

                iter_ctx.instring.assign(
                    std::istreambuf_iterator<char>(instream.rdbuf()),
                    std::istreambuf_iterator<char>());

                iter_ctx.first = iterator_type(
                    iter_ctx.instring.begin(), iter_ctx.instring.end(),
                    PositionT(iter_ctx.filename), language);
                iter_ctx.last = iterator_type();
            }

        private:
            std::string instring;
        };
    };

}   // namespace iteration_context_policies

///////////////////////////////////////////////////////////////////////////////
//  Base class for iteration contexts
template <typename ContextT, typename IteratorT>
struct base_iteration_context
{
    enum file_type
    {
        // this iteration context handles ...
        main_file,      // ... the main preprocessed file
        system_header,  // ... a header file included used #include  <>
        user_header     // ... a header file included using #include ""
    };

    base_iteration_context(ContextT& ctx_,
            BOOST_WAVE_STRINGTYPE const &fname, std::size_t if_block_depth = 0)
    :   real_filename(fname), real_relative_filename(fname), filename(fname),
        line(1), emitted_lines(0), if_block_depth(if_block_depth), ctx(ctx_),
        type(main_file)
    {}
    base_iteration_context(ContextT& ctx_,
            IteratorT const &first_, IteratorT const &last_,
            BOOST_WAVE_STRINGTYPE const &fname, std::size_t if_block_depth = 0,
            file_type type_ = main_file)
    :   first(first_), last(last_), real_filename(fname),
        real_relative_filename(fname), filename(fname),
        line(1), emitted_lines(0), if_block_depth(if_block_depth), ctx(ctx_),
        type(type_)
    {}

    // the actual input stream
    IteratorT first;            // actual input stream position
    IteratorT last;             // end of input stream
    BOOST_WAVE_STRINGTYPE real_filename;  // real name of the current file
    BOOST_WAVE_STRINGTYPE real_relative_filename;  // real relative name of the current file
    BOOST_WAVE_STRINGTYPE filename;       // actual processed file
    std::size_t line;                     // line counter of underlying stream
    std::size_t emitted_lines;            // count of emitted newlines
    std::size_t if_block_depth; // depth of #if block recursion
    ContextT& ctx;              // corresponding context<> object
    file_type type;             // the type of the handled file
};

///////////////////////////////////////////////////////////////////////////////
//
template <
    typename ContextT, typename IteratorT,
    typename InputPolicyT = typename ContextT::input_policy_type
>
struct iteration_context
:   public base_iteration_context<ContextT, IteratorT>,
    public InputPolicyT::template
        inner<iteration_context<ContextT, IteratorT, InputPolicyT> >
{
    typedef IteratorT iterator_type;
    typedef typename IteratorT::token_type::position_type position_type;

    typedef base_iteration_context<ContextT, IteratorT> base_type;
    typedef iteration_context<ContextT, IteratorT, InputPolicyT> self_type;

    iteration_context(ContextT& ctx, BOOST_WAVE_STRINGTYPE const &fname,
            position_type const &act_pos,
            boost::wave::language_support language_,
            typename base_type::file_type type = base_type::main_file)
    :   base_iteration_context<ContextT, IteratorT>(ctx, fname, type)
    {
        InputPolicyT::template inner<self_type>::init_iterators(
            *this, act_pos, language_);
    }
};

///////////////////////////////////////////////////////////////////////////////
}   // namespace wave
}   // namespace boost

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !defined(BOOST_CPP_ITERATION_CONTEXT_HPP_00312288_9DDB_4668_AFE5_25D3994FD095_INCLUDED)

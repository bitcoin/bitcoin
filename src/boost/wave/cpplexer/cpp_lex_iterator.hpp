/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Definition of the lexer iterator

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_CPP_LEX_ITERATOR_HPP_AF0C37E3_CBD8_4F33_A225_51CF576FA61F_INCLUDED)
#define BOOST_CPP_LEX_ITERATOR_HPP_AF0C37E3_CBD8_4F33_A225_51CF576FA61F_INCLUDED

#include <string>

#include <boost/assert.hpp>
#include <boost/intrusive_ptr.hpp>

#include <boost/wave/wave_config.hpp>
#include <boost/spirit/include/support_multi_pass.hpp>

#include <boost/wave/util/file_position.hpp>
#include <boost/wave/util/functor_input.hpp>
#include <boost/wave/cpplexer/cpp_lex_interface_generator.hpp>

#include <boost/wave/language_support.hpp>

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

#if 0 != __COMO_VERSION__ || !BOOST_WORKAROUND(BOOST_MSVC, <= 1310)
#define BOOST_WAVE_EOF_PREFIX static
#else
#define BOOST_WAVE_EOF_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace cpplexer {
namespace impl {

///////////////////////////////////////////////////////////////////////////////
//
//  lex_iterator_functor_shim
//
///////////////////////////////////////////////////////////////////////////////

template <typename TokenT>
class lex_iterator_functor_shim
{
    typedef typename TokenT::position_type  position_type;

public:
    lex_iterator_functor_shim()
#if /*0 != __DECCXX_VER || */defined(__PGI)
      : eof()
#endif
    {}

#if BOOST_WORKAROUND(BOOST_MSVC, <= 1310)
    lex_iterator_functor_shim& operator= (lex_iterator_functor_shim const& rhs)
        { return *this; }   // nothing to do here
#endif

    // interface to the iterator_policies::split_functor_input policy
    typedef TokenT result_type;
    typedef lex_iterator_functor_shim unique;
    typedef lex_input_interface<TokenT>* shared;

    BOOST_WAVE_EOF_PREFIX result_type const eof;

    template <typename MultiPass>
    static result_type& get_next(MultiPass& mp, result_type& result)
    {
        return mp.shared()->ftor->get(result);
    }

    // this will be called whenever the last reference to a multi_pass will
    // be released
    template <typename MultiPass>
    static void destroy(MultiPass& mp)
    {
        delete mp.shared()->ftor;
    }

    template <typename MultiPass>
    static void set_position(MultiPass& mp, position_type const &pos)
    {
        mp.shared()->ftor->set_position(pos);
    }
#if BOOST_WAVE_SUPPORT_PRAGMA_ONCE != 0
    template <typename MultiPass>
    static bool has_include_guards(MultiPass& mp, std::string& guard_name)
    {
        return mp.shared()->ftor->has_include_guards(guard_name);
    }
#endif
};

///////////////////////////////////////////////////////////////////////////////
//  eof token
#if 0 != __COMO_VERSION__ || !BOOST_WORKAROUND(BOOST_MSVC, <= 1310)
template <typename TokenT>
typename lex_iterator_functor_shim<TokenT>::result_type const
    lex_iterator_functor_shim<TokenT>::eof;
#endif // 0 != __COMO_VERSION__

///////////////////////////////////////////////////////////////////////////////
}   // namespace impl

///////////////////////////////////////////////////////////////////////////////
//
//  lex_iterator
//
//      A generic C++ lexer interface class, which allows to plug in different
//      lexer implementations. The interface between the lexer type used and
//      the preprocessor component depends on the token type only (template
//      parameter TokenT).
//      Additionally, the following requirements apply:
//
//          - the lexer type should have a function implemented, which returnes
//            the next lexed token from the input stream:
//                typename TokenT get();
//          - at the end of the input stream this function should return the
//            eof token equivalent
//          - the lexer should implement a constructor taking two iterators
//            pointing to the beginning and the end of the input stream,
//            a third parameter containing the name of the parsed input file
//            and a 4th parameter of the type boost::wave::language_support
//            which specifies, which language subset should be supported (C++,
//            C99, C++11 etc.).
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//  Divide the given functor type into its components (unique and shared)
//  and build a std::pair from these parts
template <typename FunctorData>
struct make_multi_pass
{
    typedef
        std::pair<typename FunctorData::unique, typename FunctorData::shared>
    functor_data_type;
    typedef typename FunctorData::result_type result_type;

    typedef boost::spirit::iterator_policies::split_functor_input input_policy;
    typedef boost::spirit::iterator_policies::ref_counted ownership_policy;
#if defined(BOOST_WAVE_DEBUG)
    typedef boost::spirit::iterator_policies::buf_id_check check_policy;
#else
    typedef boost::spirit::iterator_policies::no_check check_policy;
#endif
    typedef boost::spirit::iterator_policies::split_std_deque storage_policy;

    typedef boost::spirit::iterator_policies::default_policy<
            ownership_policy, check_policy, input_policy, storage_policy>
        policy_type;
    typedef boost::spirit::multi_pass<functor_data_type, policy_type> type;
};

///////////////////////////////////////////////////////////////////////////////
template <typename TokenT>
class lex_iterator
:   public make_multi_pass<impl::lex_iterator_functor_shim<TokenT> >::type
{
    typedef impl::lex_iterator_functor_shim<TokenT> input_policy_type;

    typedef typename make_multi_pass<input_policy_type>::type base_type;
    typedef typename make_multi_pass<input_policy_type>::functor_data_type
        functor_data_type;

    typedef typename input_policy_type::unique unique_functor_type;
    typedef typename input_policy_type::shared shared_functor_type;

public:
    typedef TokenT token_type;

    lex_iterator()
    {}

    template <typename IteratorT>
    lex_iterator(IteratorT const &first, IteratorT const &last,
            typename TokenT::position_type const &pos,
            boost::wave::language_support language)
    :   base_type(
            functor_data_type(
                unique_functor_type(),
                lex_input_interface_generator<TokenT>
                    ::new_lexer(first, last, pos, language)
            )
        )
    {}

    void set_position(typename TokenT::position_type const &pos)
    {
        typedef typename TokenT::position_type position_type;

        // set the new position in the current token
        token_type const& currtoken = this->base_type::dereference(*this);
        position_type currpos = currtoken.get_position();

        currpos.set_file(pos.get_file());
        currpos.set_line(pos.get_line());
        const_cast<token_type&>(currtoken).set_position(currpos);

        // set the new position for future tokens as well
        if (token_type::string_type::npos !=
            currtoken.get_value().find_first_of('\n'))
        {
            currpos.set_line(pos.get_line() + 1);
        }
        unique_functor_type::set_position(*this, currpos);
    }

#if BOOST_WAVE_SUPPORT_PRAGMA_ONCE != 0
    // return, whether the current file has include guards
    // this function returns meaningful results only if the file was scanned
    // completely
    bool has_include_guards(std::string& guard_name) const
    {
        return unique_functor_type::has_include_guards(*this, guard_name);
    }
#endif
};

///////////////////////////////////////////////////////////////////////////////
}   // namespace cpplexer
}   // namespace wave
}   // namespace boost

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#undef BOOST_WAVE_EOF_PREFIX

#endif // !defined(BOOST_CPP_LEX_ITERATOR_HPP_AF0C37E3_CBD8_4F33_A225_51CF576FA61F_INCLUDED)

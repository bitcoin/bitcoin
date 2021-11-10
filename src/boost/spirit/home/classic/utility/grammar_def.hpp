/*=============================================================================
    Copyright (c) 2003 Hartmut Kaiser
    Copyright (c) 2003 Joel de Guzman
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_GRAMMAR_DEF_HPP)
#define BOOST_SPIRIT_GRAMMAR_DEF_HPP

#include <boost/mpl/if.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/enum.hpp>
#include <boost/preprocessor/enum_params.hpp>
#include <boost/preprocessor/repeat.hpp>
#include <boost/preprocessor/repeat_from_to.hpp>
#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/phoenix/tuples.hpp>
#include <boost/spirit/home/classic/core/assert.hpp>
#include <boost/spirit/home/classic/utility/grammar_def_fwd.hpp>

///////////////////////////////////////////////////////////////////////////////
//
//  Spirit predefined maximum grammar start parser limit. This limit defines
//  the maximum number of possible different parsers exposed from a
//  particular grammar. This number defaults to 3.
//  The actual maximum is rounded up in multiples of 3. Thus, if this value
//  is 4, the actual limit is 6. The ultimate maximum limit in this
//  implementation is 15.
//
//  It should NOT be greater than PHOENIX_LIMIT!
//
///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT)
#define BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT PHOENIX_LIMIT
#endif

///////////////////////////////////////////////////////////////////////////////
//
// ensure BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT <= PHOENIX_LIMIT and
//        BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT <= 15 and
//        BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT > 0
//
///////////////////////////////////////////////////////////////////////////////
BOOST_STATIC_ASSERT(BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT <= PHOENIX_LIMIT);
BOOST_STATIC_ASSERT(BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT <= 15);
BOOST_STATIC_ASSERT(BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT > 0);

//////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

struct same {};

///////////////////////////////////////////////////////////////////////////////
namespace impl {

    ///////////////////////////////////////////////////////////////////////////
    //
    //  The make_const_pointer meta function allows to generate a T const*
    //  needed to store the pointer to a given start parser from a grammar.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename T0, typename T = T0>
    struct make_const_pointer {

    private:
        // T0 shouldn't be of type 'same'
        BOOST_STATIC_ASSERT((!boost::is_same<T0, same>::value));

        typedef typename boost::mpl::if_c<
                    boost::is_same<T, same>::value,
                    T0 const *,
                    T const *
                >::type
            ptr_type;

    public:
        // If the type in question is phoenix::nil_t, then the returned type
        // is still phoenix::nil_t, otherwise a constant pointer type to the
        // inspected type is returned.
        typedef typename boost::mpl::if_c<
                    boost::is_same<T, ::phoenix::nil_t>::value,
                    ::phoenix::nil_t,
                    ptr_type
                >::type
            type;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <int N, typename ElementT>
    struct assign_zero_to_tuple_member {

        template <typename TupleT>
        static void do_(TupleT &t)
        {
            ::phoenix::tuple_index<N> const idx;
            t[idx] = 0;
        }
    };

    template <int N>
    struct assign_zero_to_tuple_member<N, ::phoenix::nil_t> {

        template <typename TupleT>
        static void do_(TupleT& /*t*/) {}
    };

    struct phoenix_nil_type {

        typedef ::phoenix::nil_t type;
    };

    template <int N>
    struct init_tuple_member {

        template <typename TupleT>
        static void
        do_(TupleT &t)
        {
            typedef typename boost::mpl::eval_if_c<
                        (N < TupleT::length),
                        ::phoenix::tuple_element<N, TupleT>,
                        phoenix_nil_type
                    >::type
                element_type;

            assign_zero_to_tuple_member<N, element_type>::do_(t);
        }
    };

///////////////////////////////////////////////////////////////////////////////
}   // namespace impl

///////////////////////////////////////////////////////////////////////////////
//
//  grammar_def class
//
//      This class may be used as a base class for the embedded definition
//      class inside the grammar<> derived user grammar.
//      It exposes the two functions needed for start rule access:
//
//          rule<> const &start() const;
//
//      and
//
//          template <int N>
//          rule<> const *get_start_parser() const;
//
//      Additionally it exposes a set o 'start_parsers' functions, which are to
//      be called by the user to define the parsers to use as start parsers
//      of the given grammar.
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename T,
    BOOST_PP_ENUM_PARAMS(
        BOOST_PP_DEC(BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT_A), typename T)
>
class grammar_def {

private:
    ///////////////////////////////////////////////////////////////////////////
    //
    //  This generates the full tuple type from the given template parameters
    //  T, T0, ...
    //
    //      typedef ::phoenix::tuple<
    //              typename impl::make_const_pointer<T>::type,
    //              typename impl::make_const_pointer<T, T0>::type,
    //              ...
    //          > tuple_t;
    //
    ///////////////////////////////////////////////////////////////////////////
    #define BOOST_SPIRIT_GRAMMARDEF_TUPLE_PARAM(z, N, _) \
        typename impl::make_const_pointer<T, BOOST_PP_CAT(T, N)>::type \
        /**/

    typedef ::phoenix::tuple<
            typename impl::make_const_pointer<T>::type,
            BOOST_PP_ENUM(
                BOOST_PP_DEC(BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT_A),
                BOOST_SPIRIT_GRAMMARDEF_TUPLE_PARAM,
                _
            )
        > tuple_t;

    #undef BOOST_SPIRIT_GRAMMARDEF_TUPLE_PARAM
    ///////////////////////////////////////////////////////////////////////////

protected:
    ///////////////////////////////////////////////////////////////////////////
    //
    //  This generates a sequence of 'start_parsers' functions with increasing
    //  number of arguments, which allow to initialize the tuple members with
    //  the pointers to the start parsers of the grammar:
    //
    //      template <typename TC0, ...>
    //      void start_parsers (TC0 const &t0, ...)
    //      {
    //          using ::phoenix::tuple_index_names::_1;
    //          t[_1] = &t0;
    //          ...
    //      }
    //
    //      where a TC0 const* must be convertible to a T0 const*
    //
    ///////////////////////////////////////////////////////////////////////////
    #define BOOST_SPIRIT_GRAMMARDEF_ENUM_PARAMS(z, N, _) \
        BOOST_PP_CAT(TC, N) const &BOOST_PP_CAT(t, N) \
        /**/
    #define BOOST_SPIRIT_GRAMMARDEF_ENUM_ASSIGN(z, N, _) \
        using ::phoenix::tuple_index_names::BOOST_PP_CAT(_, BOOST_PP_INC(N)); \
        t[BOOST_PP_CAT(_, BOOST_PP_INC(N))] = &BOOST_PP_CAT(t, N); \
        /**/
    #define BOOST_SPIRIT_GRAMMARDEF_ENUM_START(z, N, _) \
        template <BOOST_PP_ENUM_PARAMS_Z(z, BOOST_PP_INC(N), typename TC)> \
        void \
        start_parsers(BOOST_PP_ENUM_ ## z(BOOST_PP_INC(N), \
            BOOST_SPIRIT_GRAMMARDEF_ENUM_PARAMS, _) ) \
        { \
            BOOST_PP_REPEAT_ ## z(BOOST_PP_INC(N), \
                BOOST_SPIRIT_GRAMMARDEF_ENUM_ASSIGN, _) \
        } \
        /**/

    BOOST_PP_REPEAT(
        BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT_A,
        BOOST_SPIRIT_GRAMMARDEF_ENUM_START, _)

    #undef BOOST_SPIRIT_GRAMMARDEF_ENUM_START
    #undef BOOST_SPIRIT_GRAMMARDEF_ENUM_ASSIGN
    #undef BOOST_SPIRIT_GRAMMARDEF_ENUM_PARAMS
    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    //
    //  This generates some initialization code, which allows to initialize all
    //  used tuple members to 0 (zero):
    //
    //      t[_1] = 0;
    //      impl::init_tuple_member<1>::do_(t);
    //      ...
    //
    ///////////////////////////////////////////////////////////////////////////
    #define BOOST_SPIRIT_GRAMMARDEF_ENUM_INIT(z, N, _) \
        impl::init_tuple_member<N>::do_(t); \
        /**/

    grammar_def()
    {
        using ::phoenix::tuple_index_names::_1;
        t[_1] = 0;
        BOOST_PP_REPEAT_FROM_TO(
            1, BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT_A,
            BOOST_SPIRIT_GRAMMARDEF_ENUM_INIT, _)
    }

    #undef BOOST_SPIRIT_GRAMMARDEF_ENUM_INIT
    ///////////////////////////////////////////////////////////////////////////

public:
    T const &
    start() const
    {
    //  If the following assertion is fired, you have probably forgot to call
    //  the start_parser() function from inside the constructor of your
    //  embedded definition class to initialize the start parsers to be exposed
    //  from your grammar.
        using ::phoenix::tuple_index_names::_1;
        BOOST_SPIRIT_ASSERT(0 != t[_1]);
        return *t[_1];
    }

    template <int N>
    typename ::phoenix::tuple_element<N, tuple_t>::crtype
    get_start_parser() const
    {
    //  If the following expression yields a compiler error, you have probably
    //  tried to access a start rule, which isn't exposed as such from your
    //  grammar.
        BOOST_STATIC_ASSERT(N > 0 && N < tuple_t::length);

        ::phoenix::tuple_index<N> const idx;

    //  If the following assertion is fired, you have probably forgot to call
    //  the start_parser() function from inside the constructor of your
    //  embedded definition class to initialize the start parsers to be exposed
    //  from your grammar.
    //  Another reason may be, that there is a count mismatch between
    //  the number of template parameters to the grammar_def<> class and the
    //  number of parameters used while calling start_parsers().
        BOOST_SPIRIT_ASSERT(0 != t[idx]);

        return t[idx];
    }

private:
    tuple_t t;
};

#undef BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT_A

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif // BOOST_SPIRIT_GRAMMAR_DEF_HPP

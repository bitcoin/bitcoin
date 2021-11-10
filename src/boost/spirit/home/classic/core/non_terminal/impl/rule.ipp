/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_RULE_IPP)
#define BOOST_SPIRIT_RULE_IPP

#if BOOST_SPIRIT_RULE_SCANNERTYPE_LIMIT > 1
#include <boost/preprocessor/repeat.hpp>
#include <boost/preprocessor/repeat_from_to.hpp>
#include <boost/preprocessor/enum_params.hpp>
#include <boost/preprocessor/enum_params_with_defaults.hpp>
#include <boost/preprocessor/facilities/intercept.hpp>
#include <boost/preprocessor/inc.hpp>
#include <boost/preprocessor/cat.hpp>
#endif

#include <boost/spirit/home/classic/core/parser.hpp>
#include <boost/spirit/home/classic/core/scanner/scanner.hpp>
#include <boost/spirit/home/classic/core/non_terminal/parser_context.hpp>
#include <boost/spirit/home/classic/core/non_terminal/parser_id.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>
#include <boost/mpl/if.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

#if BOOST_SPIRIT_RULE_SCANNERTYPE_LIMIT > 1

        template <
            BOOST_PP_ENUM_BINARY_PARAMS(
                BOOST_SPIRIT_RULE_SCANNERTYPE_LIMIT,
                typename ScannerT, = mpl::void_ BOOST_PP_INTERCEPT
            )
        >
        struct scanner_list;

#endif // BOOST_SPIRIT_RULE_SCANNERTYPE_LIMIT > 1

    ///////////////////////////////////////////////////////////////////////////
    namespace impl
    {
        template <typename BaseT, typename DefaultT
            , typename T0, typename T1, typename T2>
        struct get_param
        {
            typedef typename mpl::if_<
                is_base_and_derived<BaseT, T0>
              , T0
              , typename mpl::if_<
                    is_base_and_derived<BaseT, T1>
                  , T1
                  , typename mpl::if_<
                        is_base_and_derived<BaseT, T2>
                      , T2
                      , DefaultT
                    >::type
                >::type
            >::type type;
        };

        template <typename T0, typename T1, typename T2>
        struct get_context
        {
            typedef typename get_param<
                parser_context_base, parser_context<>, T0, T1, T2>::type
            type;
        };

        template <typename T0, typename T1, typename T2>
        struct get_tag
        {
            typedef typename get_param<
                parser_tag_base, parser_address_tag, T0, T1, T2>::type
            type;
        };

        template <typename T0, typename T1, typename T2>
        struct get_scanner
        {
            typedef typename get_param<
                scanner_base, scanner<>, T0, T1, T2>::type
            type;
        };

        ///////////////////////////////////////////////////////////////////////
        //
        //  rule_base class
        //
        //      The rule_base class implements the basic plumbing for rules
        //      minus the storage mechanism. It is up to the derived class
        //      to actually store the definition somewhere. The rule_base
        //      class assumes that the derived class provides a get() function
        //      that will return a pointer to a parser. The get() function
        //      may return NULL. See rule below for details.
        //
        //      <<< For framework use only. Not for public consumption. >>>
        //
        ///////////////////////////////////////////////////////////////////////
        template <
            typename DerivedT       // derived class
          , typename EmbedT         // how derived class is embedded
          , typename T0 = nil_t     // see rule class
          , typename T1 = nil_t     // see rule class
          , typename T2 = nil_t     // see rule class
        >
        class rule_base; // forward declaration

        class rule_base_access
        {
#if defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS)
        public: // YUCK!
#else
            template <
                typename DerivedT
              , typename EmbedT
              , typename T0
              , typename T1
              , typename T2
            >
           friend class rule_base;
#endif
            template <typename RuleT>
            static typename RuleT::abstract_parser_t*
            get(RuleT const& r)
            {
                return r.get();
            }
        };

        template <
            typename DerivedT       // derived class
          , typename EmbedT         // how derived class is embedded
          , typename T0             // see rule class
          , typename T1             // see rule class
          , typename T2             // see rule class
        >
        class rule_base
            : public parser<DerivedT>
            , public impl::get_context<T0, T1, T2>::type::base_t
            , public context_aux<
                typename impl::get_context<T0, T1, T2>::type, DerivedT>
            , public impl::get_tag<T0, T1, T2>::type
        {
        public:

            typedef typename impl::get_scanner<T0, T1, T2>::type scanner_t;
            typedef typename impl::get_context<T0, T1, T2>::type context_t;
            typedef typename impl::get_tag<T0, T1, T2>::type tag_t;

            typedef EmbedT embed_t;
            typedef typename context_t::context_linker_t linked_context_t;
            typedef typename linked_context_t::attr_t attr_t;

            template <typename ScannerT>
            struct result
            {
                typedef typename match_result<ScannerT, attr_t>::type type;
            };

            template <typename ScannerT>
            typename parser_result<DerivedT, ScannerT>::type
            parse(ScannerT const& scan) const
            {
                typedef parser_scanner_linker<ScannerT> linked_scanner_t;
                typedef typename parser_result<DerivedT, ScannerT>::type result_t;
                BOOST_SPIRIT_CONTEXT_PARSE(
                    scan, *this, linked_scanner_t, linked_context_t, result_t);
            }

            template <typename ScannerT>
            typename parser_result<DerivedT, ScannerT>::type
            parse_main(ScannerT const& scan) const
            {
                typename parser_result<DerivedT, ScannerT>::type hit;

                //  MWCW 8.3 needs this cast to be done through a pointer,
                //  not a reference. Otherwise, it will silently construct
                //  a temporary, causing an infinite runtime recursion.
                DerivedT const* derived_this = static_cast<DerivedT const*>(this);

                if (rule_base_access::get(*derived_this))
                {
                    typename ScannerT::iterator_t s(scan.first);
                    hit = rule_base_access::get(*derived_this)
                            ->do_parse_virtual(scan);
                    scan.group_match(hit, this->id(), s, scan.first);
                }
                else
                {
                    hit = scan.no_match();
                }
                return hit;
            }
        };

        ///////////////////////////////////////////////////////////////////////
        //
        //  abstract_parser class
        //
        ///////////////////////////////////////////////////////////////////////
        template <typename ScannerT, typename AttrT>
        struct abstract_parser
        {
            abstract_parser() {}
            virtual ~abstract_parser() {}

            virtual typename match_result<ScannerT, AttrT>::type
            do_parse_virtual(ScannerT const& scan) const = 0;

            virtual abstract_parser*
            clone() const = 0;
        };

        ///////////////////////////////////////////////////////////////////////
        //
        //  concrete_parser class
        //
        ///////////////////////////////////////////////////////////////////////
#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#endif
        
        template <typename ParserT, typename ScannerT, typename AttrT>
        struct concrete_parser : abstract_parser<ScannerT, AttrT>
        {
            concrete_parser(ParserT const& p_) : p(p_) {}
            ~concrete_parser() BOOST_OVERRIDE {}

            typename match_result<ScannerT, AttrT>::type
            do_parse_virtual(ScannerT const& scan) const BOOST_OVERRIDE
            {
                return p.parse(scan);
            }

            abstract_parser<ScannerT, AttrT>*
            clone() const BOOST_OVERRIDE
            {
                return new concrete_parser(p);
            }

            typename ParserT::embed_t p;
        };
        
#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(pop)
#endif

#if BOOST_SPIRIT_RULE_SCANNERTYPE_LIMIT > 1

        ///////////////////////////////////////////////////////////////////////
        //
        //  This generates partial specializations for the class
        //
        //          abstract_parser
        //
        //  with an increasing number of different ScannerT template parameters
        //  and corresponding do_parse_virtual function declarations for each
        //  of the different required scanner types:
        //
        //      template <typename ScannerT0, ..., typename AttrT>
        //      struct abstract_parser<scanner_list<ScannerT0, ...>, AttrT>
        //      {
        //          abstract_parser() {}
        //          virtual ~abstract_parser() {}
        //
        //          virtual typename match_result<ScannerT0, AttrT>::type
        //          do_parse_virtual(ScannerT0 const &scan) const = 0;
        //
        //          virtual abstract_parser*
        //          clone() const = 0;
        //
        //          ...
        //      };
        //
        ///////////////////////////////////////////////////////////////////////
        #define BOOST_SPIRIT_RULE_ENUM_DOPARSE_A(z, N, _)                       \
                virtual typename match_result<                                  \
                    BOOST_PP_CAT(ScannerT, N), AttrT                            \
                >::type                                                         \
                do_parse_virtual(                                               \
                    BOOST_PP_CAT(ScannerT, N) const& scan) const = 0;           \

        #define BOOST_SPIRIT_ENUM_ABSTRACT_PARSERS(z, N, _)                     \
            template <                                                          \
                BOOST_PP_ENUM_PARAMS_Z(z, BOOST_PP_INC(N), typename ScannerT),  \
                typename AttrT                                                  \
            >                                                                   \
            struct abstract_parser<                                             \
                scanner_list<                                                   \
                    BOOST_PP_ENUM_PARAMS_Z(z, BOOST_PP_INC(N), ScannerT)        \
                >,                                                              \
                AttrT                                                           \
            >                                                                   \
            {                                                                   \
                abstract_parser() {}                                            \
                virtual ~abstract_parser() {}                                   \
                                                                                \
                BOOST_PP_REPEAT_ ## z(                                          \
                    BOOST_PP_INC(N), BOOST_SPIRIT_RULE_ENUM_DOPARSE_A, _)       \
                                                                                \
                virtual abstract_parser*                                        \
                clone() const = 0;                                              \
            };                                                                  \

        BOOST_PP_REPEAT_FROM_TO(1, BOOST_SPIRIT_RULE_SCANNERTYPE_LIMIT,
            BOOST_SPIRIT_ENUM_ABSTRACT_PARSERS, _)

        #undef BOOST_SPIRIT_RULE_ENUM_DOPARSE_A
        #undef BOOST_SPIRIT_ENUM_ABSTRACT_PARSERS
        ///////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////
        //
        //  This generates partial specializations for the class
        //
        //          concrete_parser
        //
        //  with an increasing number of different ScannerT template parameters
        //  and corresponding do_parse_virtual function declarations for each
        //  of the different required scanner types:
        //
        //      template <
        //          typename ParserT, typename ScannerT0, ..., typename AttrT
        //      >
        //      struct concrete_parser<
        //          ParserT, scanner_list<ScannerT0, ...>, AttrT
        //      >
        //      :   public abstract_parser<scanner_list<ScannerT0, ...>, AttrT>
        //      {
        //          concrete_parser(ParserT const& p_) : p(p_) {}
        //          virtual ~concrete_parser() {}
        //
        //          virtual typename match_result<ScannerT0, AttrT>::type
        //          do_parse_virtual(ScannerT0 const &scan) const
        //          { return p.parse(scan); }
        //
        //          virtual abstract_parser<scanner_list<ScannerT0, ...>, AttrT>*
        //          clone() const
        //          {
        //              return new concrete_parser(p);
        //          }
        //
        //          ...
        //
        //          typename ParserT::embed_t p;
        //      };
        //
        ///////////////////////////////////////////////////////////////////////
        #define BOOST_SPIRIT_RULE_ENUM_DOPARSE_C(z, N, _)                       \
                virtual typename match_result<                                  \
                    BOOST_PP_CAT(ScannerT, N), AttrT                            \
                >::type                                                         \
                do_parse_virtual(                                               \
                    BOOST_PP_CAT(ScannerT, N) const& scan) const                \
                { return p.parse(scan); }                                       \

        #define BOOST_SPIRIT_ENUM_CONCRETE_PARSERS(z, N, _)                     \
            template <                                                          \
                typename ParserT,                                               \
                BOOST_PP_ENUM_PARAMS_Z(z, BOOST_PP_INC(N), typename ScannerT),  \
                typename AttrT                                                  \
            >                                                                   \
            struct concrete_parser<                                             \
                ParserT,                                                        \
                scanner_list<                                                   \
                    BOOST_PP_ENUM_PARAMS_Z(z, BOOST_PP_INC(N), ScannerT)        \
                >,                                                              \
                AttrT                                                           \
            >                                                                   \
            :   abstract_parser<                                                \
                    scanner_list<                                               \
                        BOOST_PP_ENUM_PARAMS_Z(z, BOOST_PP_INC(N), ScannerT)    \
                    >,                                                          \
                    AttrT                                                       \
                >                                                               \
            {                                                                   \
                concrete_parser(ParserT const& p_) : p(p_) {}                   \
                virtual ~concrete_parser() {}                                   \
                                                                                \
                BOOST_PP_REPEAT_ ## z(                                          \
                    BOOST_PP_INC(N), BOOST_SPIRIT_RULE_ENUM_DOPARSE_C, _)       \
                                                                                \
                virtual abstract_parser<                                        \
                    scanner_list<                                               \
                        BOOST_PP_ENUM_PARAMS_Z(z, BOOST_PP_INC(N), ScannerT)    \
                    >,                                                          \
                    AttrT                                                       \
                >*                                                              \
                clone() const                                                   \
                {                                                               \
                    return new concrete_parser(p);                              \
                }                                                               \
                                                                                \
                typename ParserT::embed_t p;                                    \
            };                                                                  \

        BOOST_PP_REPEAT_FROM_TO(1, BOOST_SPIRIT_RULE_SCANNERTYPE_LIMIT,
            BOOST_SPIRIT_ENUM_CONCRETE_PARSERS, _)

        #undef BOOST_SPIRIT_ENUM_CONCRETE_PARSERS
        #undef BOOST_SPIRIT_RULE_ENUM_DOPARSE_C
        ///////////////////////////////////////////////////////////////////////

#endif // BOOST_SPIRIT_RULE_SCANNERTYPE_LIMIT > 1

    } // namespace impl

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace boost::spirit

#endif

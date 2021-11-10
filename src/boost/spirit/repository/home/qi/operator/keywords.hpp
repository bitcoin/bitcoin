/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2011-2012 Thomas Bernard

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_REPOSITORY_QI_OPERATOR_KEYWORDS_HPP
#define BOOST_SPIRIT_REPOSITORY_QI_OPERATOR_KEYWORDS_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/domain.hpp>
#include <boost/spirit/home/qi/detail/permute_function.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/support/detail/what_function.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/fusion/include/iter_fold.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/value_at.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/optional.hpp>
#include <boost/array.hpp>
#include <boost/spirit/home/qi/string/symbols.hpp>
#include <boost/spirit/home/qi/string/lit.hpp>
#include <boost/spirit/home/qi/action/action.hpp>
#include <boost/spirit/home/qi/directive/hold.hpp>
#include <boost/mpl/count_if.hpp>
#include <boost/mpl/greater.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/copy.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/back_inserter.hpp>
#include <boost/mpl/filter_view.hpp>
#include <boost/fusion/include/zip_view.hpp>
#include <boost/fusion/include/as_vector.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/proto/operators.hpp>
#include <boost/proto/tags.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/spirit/repository/home/qi/operator/detail/keywords.hpp>
#include <boost/fusion/include/any.hpp>


namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_operator<qi::domain, proto::tag::divides > // enables /
      : mpl::true_ {};

    template <>
    struct flatten_tree<qi::domain, proto::tag::divides> // flattens /
      : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace repository { namespace qi
{

    // kwd directive parser type identification
    namespace detail
    {
        BOOST_MPL_HAS_XXX_TRAIT_DEF(kwd_parser_id)
        BOOST_MPL_HAS_XXX_TRAIT_DEF(complex_kwd_parser_id)


    }

    // kwd directive type query
    template <typename T>
    struct is_kwd_parser : detail::has_kwd_parser_id<T> {};

    template <typename Subject, typename Action>
    struct is_kwd_parser<spirit::qi::action<Subject,Action> > : detail::has_kwd_parser_id<Subject> {};

    template <typename Subject>
    struct is_kwd_parser<spirit::qi::hold_directive<Subject> > : detail::has_kwd_parser_id<Subject> {};

    template <typename T>
    struct is_complex_kwd_parser : detail::has_complex_kwd_parser_id<T> {};

    template <typename Subject, typename Action>
    struct is_complex_kwd_parser<spirit::qi::action<Subject,Action> > : detail::has_complex_kwd_parser_id<Subject> {};

    template <typename Subject>
    struct is_complex_kwd_parser<spirit::qi::hold_directive<Subject> > : detail::has_complex_kwd_parser_id<Subject> {};


    // Keywords operator
    template <typename Elements, typename Modifiers>
    struct keywords : spirit::qi::nary_parser<keywords<Elements,Modifiers> >
    {
        template <typename Context, typename Iterator>
        struct attribute
        {
            // Put all the element attributes in a tuple
            typedef typename traits::build_attribute_sequence<
                Elements, Context, traits::sequence_attribute_transform, Iterator, spirit::qi::domain >::type
            all_attributes;

            // Now, build a fusion vector over the attributes. Note
            // that build_fusion_vector 1) removes all unused attributes
            // and 2) may return unused_type if all elements have
            // unused_type(s).
            typedef typename
                traits::build_fusion_vector<all_attributes>::type
            type;
        };

        /// Make sure that all subjects are of the kwd type
        typedef typename mpl::count_if<
                Elements,
                        mpl::not_<
                           mpl::or_<
                              is_kwd_parser<
                                mpl::_1
                              > ,
                              is_complex_kwd_parser<
                                mpl::_1
                              >
                          >
                        >
                > non_kwd_subject_count;

        /// If the assertion fails here then you probably forgot to wrap a
        /// subject of the / operator in a kwd directive
        BOOST_MPL_ASSERT_RELATION( non_kwd_subject_count::value, ==, 0 );

        ///////////////////////////////////////////////////////////////////////////
        // build_parser_tags
        //
        // Builds a boost::variant from an mpl::range_c in order to "mark" every
        // parser of the fusion sequence. The integer constant is used in the parser
        // dispatcher functor in order to use the parser corresponding to the recognised
        // keyword.
        ///////////////////////////////////////////////////////////////////////////

        template <typename Sequence>
        struct build_parser_tags
        {
            // Get the sequence size
            typedef typename mpl::size< Sequence >::type sequence_size;

            // Create an integer_c constant for every parser in the sequence
            typedef typename mpl::range_c<int, 0, sequence_size::value>::type int_range;

            // Transform the range_c to an mpl vector in order to be able to transform it into a variant
            typedef typename mpl::copy<int_range, mpl::back_inserter<mpl::vector<> > >::type type;

        };
        // Build an index mpl vector
        typedef typename build_parser_tags< Elements >::type parser_index_vector;

        template <typename idx>
        struct is_complex_kwd_parser_filter : is_complex_kwd_parser< typename mpl::at<Elements, idx>::type >
        {};

        template <typename idx>
        struct is_kwd_parser_filter : is_kwd_parser< typename mpl::at<Elements, idx>::type >
        {};

        // filter out the string kwd directives
        typedef typename mpl::filter_view< Elements, is_kwd_parser<mpl::_> >::type string_keywords;

        typedef typename mpl::filter_view< parser_index_vector ,
                                         is_kwd_parser_filter< mpl::_ >
                                             >::type string_keyword_indexes;
        // filter out the complex keywords
        typedef typename mpl::filter_view< parser_index_vector ,
                                         is_complex_kwd_parser_filter< mpl::_ >
                                            >::type complex_keywords_indexes;

        //typedef typename fusion::filter_view< Elements, is_complex_kwd_parser< mpl::_ > > complex_keywords_view;

        typedef typename mpl::if_<
                typename mpl::empty<complex_keywords_indexes>::type,
                detail::empty_keywords_list,
                detail::complex_keywords< complex_keywords_indexes >
                >::type complex_keywords_type;

        // build a bool array and an integer array which will be used to
        // check that the repetition constraints of the kwd parsers are
        // met and bail out a soon as possible
        typedef boost::array<bool, fusion::result_of::size<Elements>::value> flags_type;
        typedef boost::array<int, fusion::result_of::size<Elements>::value> counters_type;

        typedef typename mpl::if_<
                        typename mpl::empty<string_keyword_indexes>::type,
                        detail::empty_keywords_list,
                        detail::string_keywords<
                                Elements,
                                string_keywords,
                                string_keyword_indexes,
                                flags_type,
                                Modifiers>
                >::type string_keywords_type;

        keywords(Elements const& elements_) :
              elements(elements_)
            , string_keywords_inst(elements,flags_init)
      , complex_keywords_inst(elements,flags_init)
        {
        }

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr_) const
        {
            // Select which parse function to call
            // We need to handle the case where kwd / ikwd directives have been mixed
            // This is where we decide which function should be called.
            return parse_impl(first, last, context, skipper, attr_,
                                typename string_keywords_type::requires_one_pass()
                             );
        }

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse_impl(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr_,mpl::true_ /* one pass */) const
          {

            // wrap the attribute in a tuple if it is not a tuple
            typename traits::wrap_if_not_tuple<Attribute>::type attr(attr_);

            flags_type flags(flags_init);
            //flags.assign(false);

            counters_type counters;
            counters.assign(0);

            typedef repository::qi::detail::parse_dispatcher<Elements,Iterator, Context, Skipper
                                    , flags_type, counters_type
                                    , typename traits::wrap_if_not_tuple<Attribute>::type
                                    , mpl::false_ > parser_visitor_type;

            parser_visitor_type parse_visitor(elements, first, last
                                             , context, skipper, flags
                                             , counters, attr);

            typedef repository::qi::detail::complex_kwd_function< parser_visitor_type > complex_kwd_function_type;

            complex_kwd_function_type
                     complex_function(first,last,context,skipper,parse_visitor);

            // We have a bool array 'flags' with one flag for each parser as well as a 'counter'
            // array.
            // The kwd directive sets and increments the counter when a successeful parse occurred
            // as well as the slot of the corresponding parser to true in the flags array as soon
            // the minimum repetition requirement is met and keeps that value to true as long as
            // the maximum repetition requirement is met.
            // The parsing takes place here in two steps:
            // 1) parse a keyword and fetch the parser index associated with that keyword
            // 2) call the associated parser and store the parsed value in the matching attribute.

            for(;;)
            {

                spirit::qi::skip_over(first, last, skipper);
                Iterator save = first;
                if (string_keywords_inst.parse(first, last,parse_visitor,skipper))
                {
                    save = first;
                }
                else {
                  // restore the position to the last successful keyword parse
                  first = save;
                  if(!complex_keywords_inst.parse(complex_function))
                  {
                    first = save;
                    // Check that we are leaving the keywords parser in a successful state
                    for(typename flags_type::size_type i = 0, size = flags.size(); i < size; ++i)
                    {
                      if(!flags[i])
                      {
                        return false;
                      }
                    }
                    return true;
                  }
                  else
                    save = first;
                }
            }
            BOOST_UNREACHABLE_RETURN(false)
          }

        // Handle the mixed kwd and ikwd case
        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse_impl(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr_,mpl::false_ /* two passes */) const
          {

            // wrap the attribute in a tuple if it is not a tuple
            typename traits::wrap_if_not_tuple<Attribute>::type attr(attr_);

            flags_type flags(flags_init);
            //flags.assign(false);

            counters_type counters;
            counters.assign(0);

            typedef detail::parse_dispatcher<Elements, Iterator, Context, Skipper
                                    , flags_type, counters_type
                                    , typename traits::wrap_if_not_tuple<Attribute>::type
                                    , mpl::false_> parser_visitor_type;

           typedef detail::parse_dispatcher<Elements, Iterator, Context, Skipper
                                    , flags_type, counters_type
                                    , typename traits::wrap_if_not_tuple<Attribute>::type
                                    , mpl::true_> no_case_parser_visitor_type;


            parser_visitor_type parse_visitor(elements,first,last
                                             ,context,skipper,flags,counters,attr);
            no_case_parser_visitor_type no_case_parse_visitor(elements,first,last
                                             ,context,skipper,flags,counters,attr);

            typedef repository::qi::detail::complex_kwd_function< parser_visitor_type > complex_kwd_function_type;

            complex_kwd_function_type
                     complex_function(first,last,context,skipper,parse_visitor);


            // We have a bool array 'flags' with one flag for each parser as well as a 'counter'
            // array.
            // The kwd directive sets and increments the counter when a successeful parse occurred
            // as well as the slot of the corresponding parser to true in the flags array as soon
            // the minimum repetition requirement is met and keeps that value to true as long as
            // the maximum repetition requirement is met.
            // The parsing takes place here in two steps:
            // 1) parse a keyword and fetch the parser index associated with that keyword
            // 2) call the associated parser and store the parsed value in the matching attribute.

            for(;;)
            {
                spirit::qi::skip_over(first, last, skipper);
                Iterator save = first;
                // String keywords pass
                if (string_keywords_inst.parse(first,last,parse_visitor,no_case_parse_visitor,skipper))
                {
                    save = first;
                }
                else {
                  first = save;

                  if(!complex_keywords_inst.parse(complex_function))
                  {
                    first = save;
                    // Check that we are leaving the keywords parser in a successful state
                    for(typename flags_type::size_type i = 0, size = flags.size(); i < size; ++i)
                    {
                      if(!flags[i])
                      {
                        return false;
                      }
                    }
                    return true;
                  }
                  else
                  {
                    save = first;
                  }
                }
            }
            BOOST_UNREACHABLE_RETURN(false)
          }

        template <typename Context>
        info what(Context& context) const
        {
            info result("keywords");
            fusion::for_each(elements,
                spirit::detail::what_function<Context>(result, context));
            return result;
        }
        flags_type flags_init;
        Elements elements;
        string_keywords_type string_keywords_inst;
        complex_keywords_type complex_keywords_inst;

    };
}}}}

namespace boost { namespace spirit { namespace qi {
    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Elements, typename Modifiers >
    struct make_composite<proto::tag::divides, Elements, Modifiers >
    {
        typedef repository::qi::keywords<Elements,Modifiers> result_type;
        result_type operator()(Elements ref, unused_type) const
        {
            return result_type(ref);
        }
    };


}}}

namespace boost { namespace spirit { namespace traits
{
    // We specialize this for keywords (see support/attributes.hpp).
    // For keywords, we only wrap the attribute in a tuple IFF
    // it is not already a fusion tuple.
    template <typename Elements, typename Modifiers,typename Attribute>
    struct pass_attribute<repository::qi::keywords<Elements,Modifiers>, Attribute>
      : wrap_if_not_tuple<Attribute> {};

    template <typename Elements, typename Modifiers>
    struct has_semantic_action<repository::qi::keywords<Elements, Modifiers> >
      : nary_has_semantic_action<Elements> {};

    template <typename Elements, typename Attribute, typename Context
        , typename Iterator, typename Modifiers>
    struct handles_container<repository::qi::keywords<Elements,Modifiers>, Attribute
        , Context, Iterator>
      : nary_handles_container<Elements, Attribute, Context, Iterator> {};


}}}

#endif


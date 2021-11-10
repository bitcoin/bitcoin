/*=============================================================================
  Copyright (c) 2011-2012 Thomas Bernard

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
  =============================================================================*/
#ifndef BOOST_SPIRIT_REPOSITORY_QI_OPERATOR_DETAIL_KEYWORDS_HPP
#define BOOST_SPIRIT_REPOSITORY_QI_OPERATOR_DETAIL_KEYWORDS_HPP

#if defined(_MSC_VER)
#pragma once
#endif
#include <boost/fusion/include/nview.hpp>
#include <boost/spirit/home/qi/string/lit.hpp>
#include <boost/fusion/include/at.hpp>
namespace boost { namespace spirit { namespace repository { namespace qi { namespace detail {
    // Variant visitor class which handles dispatching the parsing to the selected parser
    // This also handles passing the correct attributes and flags/counters to the subject parsers
    template<typename T>
    struct is_distinct : T::distinct { };
    
    template<typename T, typename Action>
    struct is_distinct< spirit::qi::action<T,Action> > : T::distinct { };

    template<typename T>
    struct is_distinct< spirit::qi::hold_directive<T> > : T::distinct { };



    template < typename Elements, typename Iterator ,typename Context ,typename Skipper
        ,typename Flags ,typename Counters ,typename Attribute, typename NoCasePass>
        struct parse_dispatcher
        : public boost::static_visitor<bool>
        {

            typedef Iterator iterator_type;
            typedef Context context_type;
            typedef Skipper skipper_type;
            typedef Elements elements_type;

            typedef typename add_reference<Attribute>::type attr_reference;
            public:
            parse_dispatcher(const Elements &elements,Iterator& first, Iterator const& last
                    , Context& context, Skipper const& skipper
                    , Flags &flags, Counters &counters, attr_reference attr) :
                elements(elements), first(first), last(last)
                , context(context), skipper(skipper)
                , flags(flags),counters(counters), attr(attr)
            {}

            template<typename T> bool operator()(T& idx) const
            {
                return call(idx,typename traits::not_is_unused<Attribute>::type());
            }

            template <typename Subject,typename Index>
                bool call_subject_unused(
                        Subject const &subject, Iterator &first, Iterator const &last
                        , Context& context, Skipper const& skipper
                        , Index& /*idx*/ ) const
                {
                    Iterator save = first;
                    skipper_keyword_marker<Skipper,NoCasePass>
                        marked_skipper(skipper,flags[Index::value],counters[Index::value]);

                    if(subject.parse(first,last,context,marked_skipper,unused))
                    {
                        return true;
                    }
                    first = save;
                    return false;
                }


            template <typename Subject,typename Index>
                bool call_subject(
                        Subject const &subject, Iterator &first, Iterator const &last
                        , Context& context, Skipper const& skipper
                        , Index& /*idx*/ ) const
                {

                    Iterator save = first;
                    skipper_keyword_marker<Skipper,NoCasePass> 
                        marked_skipper(skipper,flags[Index::value],counters[Index::value]);
                    typename fusion::result_of::at_c<typename remove_reference<Attribute>::type, Index::value>::type
                        attr_ = fusion::at_c<Index::value>(attr);
                    if(subject.parse(first,last,context,marked_skipper,attr_))
                    {
                        return true;
                    }
                    first = save;
                    return false;
                }

#if defined(_MSC_VER)
# pragma warning(push)
# pragma warning(disable: 4127) // conditional expression is constant
#endif
            // Handle unused attributes
            template <typename T> bool call(T &idx, mpl::false_) const{
 
                typedef typename mpl::at<Elements,T>::type ElementType;
                if(
                       (!is_distinct<ElementType>::value)
                    || skipper.parse(first,last,unused,unused,unused)
                  ){
                      spirit::qi::skip_over(first, last, skipper);
                      return call_subject_unused(fusion::at_c<T::value>(elements), first, last, context, skipper, idx );
                    }
                return false;
            }
            // Handle normal attributes
            template <typename T> bool call(T &idx, mpl::true_) const{
                 typedef typename mpl::at<Elements,T>::type ElementType;
                 if(
                       (!is_distinct<ElementType>::value)
                    || skipper.parse(first,last,unused,unused,unused)
                  ){
                      return call_subject(fusion::at_c<T::value>(elements), first, last, context, skipper, idx);
                  }
                return false;
            }
#if defined(_MSC_VER)
# pragma warning(pop)
#endif

            const Elements &elements;
            Iterator &first;
            const Iterator &last;
            Context & context;
            const Skipper &skipper;
            Flags &flags;
            Counters &counters;
            attr_reference attr;
        };
    // string keyword loop handler
    template <typename Elements, typename StringKeywords, typename IndexList, typename FlagsType, typename Modifiers>
        struct string_keywords
        {
            // Create a variant type to be able to store parser indexes in the embedded symbols parser
            typedef typename
                spirit::detail::as_variant<
                IndexList >::type        parser_index_type;

            ///////////////////////////////////////////////////////////////////////////
            // build_char_type_sequence
            //
            // Build a fusion sequence from the kwd directive specified character type.
            ///////////////////////////////////////////////////////////////////////////
            template <typename Sequence >
                struct build_char_type_sequence
                {
                    struct element_char_type
                    {
                        template <typename T>
                            struct result;

                        template <typename F, typename Element>
                            struct result<F(Element)>
                            {
                                typedef typename Element::char_type type;

                            };
                        template <typename F, typename Element,typename Action>
                            struct result<F(spirit::qi::action<Element,Action>) >
                            {
                                typedef typename Element::char_type type;
                            };
                        template <typename F, typename Element>
                            struct result<F(spirit::qi::hold_directive<Element>)>
                            {
                                typedef typename Element::char_type type;
                            };

                        // never called, but needed for decltype-based result_of (C++0x)
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
                        template <typename Element>
                            typename result<element_char_type(Element)>::type
                            operator()(Element&&) const;
#endif
                    };

                    // Compute the list of character types of the child kwd directives
                    typedef typename
                        fusion::result_of::transform<Sequence, element_char_type>::type
                        type;
                };


            ///////////////////////////////////////////////////////////////////////////
            // get_keyword_char_type
            //
            // Collapses the character type coming from the subject kwd parsers and
            // and checks that they are all identical (necessary in order to be able
            // to build a tst parser to parse the keywords.
            ///////////////////////////////////////////////////////////////////////////
            template <typename Sequence>
                struct get_keyword_char_type
                {
                    // Make sure each of the types occur only once in the type list
                    typedef typename
                        mpl::fold<
                        Sequence, mpl::vector<>,
                        mpl::if_<
                            mpl::contains<mpl::_1, mpl::_2>,
                        mpl::_1, mpl::push_back<mpl::_1, mpl::_2>
                            >
                            >::type
                            no_duplicate_char_types;

                    // If the compiler traps here this means you mixed
                    // character type for the keywords specified in the
                    // kwd directive sequence.
                    BOOST_MPL_ASSERT_RELATION( mpl::size<no_duplicate_char_types>::value, ==, 1 );

                    typedef typename mpl::front<no_duplicate_char_types>::type type;

                };

            // Get the character type for the tst parser
            typedef typename build_char_type_sequence< StringKeywords >::type char_types;
            typedef typename get_keyword_char_type<
                typename mpl::if_<
                  mpl::equal_to<
                    typename mpl::size < char_types >::type
                    , mpl::int_<0>
                    >
                  , mpl::vector< boost::spirit::standard::char_type >
                  , char_types >::type
                >::type  char_type;

            // Our symbols container
            typedef spirit::qi::tst< char_type, parser_index_type> keywords_type;

            // Filter functor used for case insensitive parsing
            template <typename CharEncoding>
                struct no_case_filter
                {
                    char_type operator()(char_type ch) const
                    {
                        return static_cast<char_type>(CharEncoding::tolower(ch));
                    }
                };

            ///////////////////////////////////////////////////////////////////////////
            // build_case_type_sequence
            //
            // Build a fusion sequence from the kwd/ikwd directives
            // in order to determine if case sensitive and case insensitive
            // keywords have been mixed.
            ///////////////////////////////////////////////////////////////////////////
            template <typename Sequence >
                struct build_case_type_sequence
                {
                    struct element_case_type
                    {
                        template <typename T>
                            struct result;

                        template <typename F, typename Element>
                            struct result<F(Element)>
                            {
                                typedef typename Element::no_case_keyword type;

                            };
                        template <typename F, typename Element,typename Action>
                            struct result<F(spirit::qi::action<Element,Action>) >
                            {
                                typedef typename Element::no_case_keyword type;
                            };
                        template <typename F, typename Element>
                            struct result<F(spirit::qi::hold_directive<Element>)>
                            {
                                typedef typename Element::no_case_keyword type;
                            };

                        // never called, but needed for decltype-based result_of (C++0x)
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
                        template <typename Element>
                            typename result<element_case_type(Element)>::type
                            operator()(Element&&) const;
#endif
                    };

                    // Compute the list of character types of the child kwd directives
                    typedef typename
                        fusion::result_of::transform<Sequence, element_case_type>::type
                        type;
                };

            ///////////////////////////////////////////////////////////////////////////
            // get_nb_case_types
            //
            // Counts the number of entries in the case type sequence matching the
            // CaseType parameter (mpl::true_  -> case insensitve
            //                   , mpl::false_ -> case sensitive
            ///////////////////////////////////////////////////////////////////////////
            template <typename Sequence,typename CaseType>
                struct get_nb_case_types
                {
                    // Make sure each of the types occur only once in the type list
                    typedef typename
                        mpl::count_if<
                        Sequence, mpl::equal_to<mpl::_,CaseType>
                        >::type type;


                };
            // Build the case type sequence
            typedef typename build_case_type_sequence< StringKeywords >::type case_type_sequence;
            // Count the number of case sensitive entries and case insensitve entries
            typedef typename get_nb_case_types<case_type_sequence,mpl::true_>::type ikwd_count;
            typedef typename get_nb_case_types<case_type_sequence,mpl::false_>::type kwd_count;
            // Get the size of the original sequence
            typedef typename mpl::size<IndexList>::type nb_elements;
            // Determine if all the kwd directive are case sensitive/insensitive
            typedef typename mpl::and_<
                typename mpl::greater< nb_elements, mpl::int_<0> >::type
                , typename mpl::equal_to< ikwd_count, nb_elements>::type
                >::type all_ikwd;

            typedef typename mpl::and_<
                typename mpl::greater< nb_elements, mpl::int_<0> >::type
                , typename mpl::equal_to< kwd_count, nb_elements>::type
                >::type all_kwd;

            typedef typename mpl::or_< all_kwd, all_ikwd >::type all_directives_of_same_type;

            // Do we have a no case modifier
            typedef has_modifier<Modifiers, spirit::tag::char_code_base<spirit::tag::no_case> > no_case_modifier;

            // Should the no_case filter always be used ?
            typedef typename mpl::or_<
                no_case_modifier,
                mpl::and_<
                    all_directives_of_same_type
                    ,all_ikwd
                    >
                    >::type
                    no_case;

            typedef no_case_filter<
                typename spirit::detail::get_encoding_with_case<
                Modifiers
                , char_encoding::standard
                , no_case::value>::type>
                nc_filter;
            // Determine the standard case filter type
            typedef typename mpl::if_<
                no_case
                , nc_filter
                , spirit::qi::tst_pass_through >::type
                first_pass_filter_type;

            typedef typename mpl::or_<
                    all_directives_of_same_type
                  , no_case_modifier
                >::type requires_one_pass;


            // Functor which adds all the keywords/subject parser indexes
            // collected from the subject kwd directives to the keyword tst parser
            struct keyword_entry_adder
            {
                typedef int result_type;

                keyword_entry_adder(shared_ptr<keywords_type> lookup,FlagsType &flags, Elements &elements) :
                    lookup(lookup)
                    ,flags(flags)
                    ,elements(elements)
                {}

                template <typename T>
                    int operator()(const T &index) const
                    {
                        return call(fusion::at_c<T::value>(elements),index);
                    }

                template <typename T, typename Position, typename Action>
                    int call(const spirit::qi::action<T,Action> &parser, const Position position ) const
                    {

                        // Make the keyword/parse index entry in the tst parser
                        lookup->add(
                                traits::get_begin<char_type>(get_string(parser.subject.keyword)),
                                traits::get_end<char_type>(get_string(parser.subject.keyword)),
                                position
                                );
                        // Get the initial state of the flags array and store it in the flags initializer
                        flags[Position::value]=parser.subject.iter.flag_init();
                        return 0;
                    }

                template <typename T, typename Position>
                    int call( const T & parser, const Position position) const
                    {
                        // Make the keyword/parse index entry in the tst parser
                        lookup->add(
                                traits::get_begin<char_type>(get_string(parser.keyword)),
                                traits::get_end<char_type>(get_string(parser.keyword)),
                                position
                                );
                        // Get the initial state of the flags array and store it in the flags initializer
                        flags[Position::value]=parser.iter.flag_init();
                        return 0;
                    }

                template <typename T, typename Position>
                    int call( const spirit::qi::hold_directive<T> & parser, const Position position) const
                    {
                        // Make the keyword/parse index entry in the tst parser
                        lookup->add(
                                traits::get_begin<char_type>(get_string(parser.subject.keyword)),
                                traits::get_end<char_type>(get_string(parser.subject.keyword)),
                                position
                                );
                        // Get the initial state of the flags array and store it in the flags initializer
                        flags[Position::value]=parser.subject.iter.flag_init();
                        return 0;
                    }


                template <typename String, bool no_attribute>
                const String get_string(const boost::spirit::qi::literal_string<String,no_attribute> &parser) const
                {
                        return parser.str;
                }

        template <typename String, bool no_attribute>
                const typename boost::spirit::qi::no_case_literal_string<String,no_attribute>::string_type &
                        get_string(const boost::spirit::qi::no_case_literal_string<String,no_attribute> &parser) const
                {
                        return parser.str_lo;
                }
   


                shared_ptr<keywords_type> lookup;
                FlagsType & flags;
                Elements &elements;
            };

            string_keywords(Elements &elements,FlagsType &flags_init) : lookup(new keywords_type())
            {
                // Loop through all the subject parsers to build the keyword parser symbol parser
                IndexList indexes;
                keyword_entry_adder f1(lookup,flags_init,elements);
                fusion::for_each(indexes,f1);

            }
            template <typename Iterator,typename ParseVisitor, typename Skipper>
                bool parse(
                        Iterator &first,
                        const Iterator &last,
                        const ParseVisitor &parse_visitor,
                        const Skipper &/*skipper*/) const
                {
                    if(parser_index_type* val_ptr =
                            lookup->find(first,last,first_pass_filter_type()))
                    {                        
                        if(!apply_visitor(parse_visitor,*val_ptr)){
                            return false;
                        }
            return true;
                    }
                    return false;
                }

            template <typename Iterator,typename ParseVisitor, typename NoCaseParseVisitor,typename Skipper>
                bool parse(
                        Iterator &first,
                        const Iterator &last,
                        const ParseVisitor &parse_visitor,
                        const NoCaseParseVisitor &no_case_parse_visitor,
                        const Skipper &/*skipper*/) const
                {
                    Iterator saved_first = first;
                    if(parser_index_type* val_ptr =
                            lookup->find(first,last,first_pass_filter_type()))
                    {
                        if(!apply_visitor(parse_visitor,*val_ptr)){
                            return false;
                        }
            return true;
                    }
                    // Second pass case insensitive
                    if(parser_index_type* val_ptr
                            = lookup->find(saved_first,last,nc_filter()))
                    {
                        first = saved_first;
                        if(!apply_visitor(no_case_parse_visitor,*val_ptr)){
                            return false;
                        }
            return true;
                    }
                    return false;
                }
            shared_ptr<keywords_type> lookup;


        };

    struct empty_keywords_list
    {
        typedef mpl::true_ requires_one_pass;

        empty_keywords_list()
        {}
        template<typename Elements>
        empty_keywords_list(const Elements &)
        {}

       template<typename Elements, typename FlagsInit>
        empty_keywords_list(const Elements &, const FlagsInit &)
        {}

        template <typename Iterator,typename ParseVisitor, typename NoCaseParseVisitor,typename Skipper>
        bool parse(
                        Iterator &/*first*/,
                        const Iterator &/*last*/,
                        const ParseVisitor &/*parse_visitor*/,
                        const NoCaseParseVisitor &/*no_case_parse_visitor*/,
                        const Skipper &/*skipper*/) const
                {
                        return false;
                }

        template <typename Iterator,typename ParseVisitor, typename Skipper>
                bool parse(
                        Iterator &/*first*/,
                        const Iterator &/*last*/,
                        const ParseVisitor &/*parse_visitor*/,
                        const Skipper &/*skipper*/) const
                {
                    return false;
                }

        template <typename ParseFunction>
        bool parse( ParseFunction &/*function*/ ) const
                {
                   return false;
                }
    };

    template<typename ComplexKeywords>
    struct complex_keywords
    {
      // Functor which performs the flag initialization for the complex keyword parsers
      template <typename FlagsType, typename Elements>
            struct flag_init_value_setter
            {
                typedef int result_type;

                flag_init_value_setter(Elements &elements,FlagsType &flags)
          :flags(flags)
                    ,elements(elements)
                {}

                template <typename T>
                    int operator()(const T &index) const
                    {
                        return call(fusion::at_c<T::value>(elements),index);
                    }

                template <typename T, typename Position, typename Action>
                    int call(const spirit::qi::action<T,Action> &parser, const Position /*position*/ ) const
                    {
                        // Get the initial state of the flags array and store it in the flags initializer
                        flags[Position::value]=parser.subject.iter.flag_init();
                        return 0;
                    }

                template <typename T, typename Position>
                    int call( const T & parser, const Position /*position*/) const
                    {
                        // Get the initial state of the flags array and store it in the flags initializer
                        flags[Position::value]=parser.iter.flag_init();
                        return 0;
                    }

                template <typename T, typename Position>
                    int call( const spirit::qi::hold_directive<T> & parser, const Position /*position*/) const
                    {
                        // Get the initial state of the flags array and store it in the flags initializer
                        flags[Position::value]=parser.subject.iter.flag_init();
                        return 0;
                    }

                FlagsType & flags;
                Elements &elements;
            };

    template <typename Elements, typename Flags>
        complex_keywords(Elements &elements, Flags &flags)
        {
      flag_init_value_setter<Flags,Elements> flag_initializer(elements,flags);
      fusion::for_each(complex_keywords_inst,flag_initializer);
    }

        template <typename ParseFunction>
        bool parse( ParseFunction &function ) const
                {
                   return fusion::any(complex_keywords_inst,function);
                }

        ComplexKeywords complex_keywords_inst;
    };
    // This helper class enables jumping over intermediate directives
    // down the kwd parser iteration count checking policy
    struct register_successful_parse
    {
        template <typename Subject>
            static bool call(Subject const &subject,bool &flag, int &counter)
            {
                return subject.iter.register_successful_parse(flag,counter);
            }
        template <typename Subject, typename Action>
            static bool call(spirit::qi::action<Subject, Action> const &subject,bool &flag, int &counter)
            {
                return subject.subject.iter.register_successful_parse(flag,counter);
            }
        template <typename Subject>
            static bool call(spirit::qi::hold_directive<Subject> const &subject,bool &flag, int &counter)
            {
                return subject.subject.iter.register_successful_parse(flag,counter);
            }
    };

    // This helper class enables jumping over intermediate directives
    // down the kwd parser
    struct extract_keyword
    {
        template <typename Subject>
            static Subject const& call(Subject const &subject)
            {
                return subject;
            }
        template <typename Subject, typename Action>
            static Subject const& call(spirit::qi::action<Subject, Action> const &subject)
            {
                return subject.subject;
            }
        template <typename Subject>
            static Subject const& call(spirit::qi::hold_directive<Subject> const &subject)
            {
                return subject.subject;
            }
    };

    template <typename ParseDispatcher>
        struct complex_kwd_function
        {
            typedef typename ParseDispatcher::iterator_type Iterator;
            typedef typename ParseDispatcher::context_type Context;
            typedef typename ParseDispatcher::skipper_type Skipper;
            complex_kwd_function(
                    Iterator& first, Iterator const& last
                    , Context& context, Skipper const& skipper, ParseDispatcher &dispatcher)
                : first(first)
                  , last(last)
                  , context(context)
                  , skipper(skipper)
                  , dispatcher(dispatcher)
            {
            }

            template <typename Component>
                bool operator()(Component const& component)
                {
                    Iterator save = first;
                    if(
                        extract_keyword::call(
                                fusion::at_c<
                                        Component::value
                                        ,typename ParseDispatcher::elements_type
                                >(dispatcher.elements)
                                )
                                .keyword.parse(
                                        first
                                        ,last
                                        ,context
                                        ,skipper
                                        ,unused)
                    )
                    {
                        if(!dispatcher(component)){
                            first = save;
                            return false;
                        }
                        return true;
                    }
                    return false;
                }

            Iterator& first;
            Iterator const& last;
            Context& context;
            Skipper const& skipper;
            ParseDispatcher const& dispatcher;

            // silence MSVC warning C4512: assignment operator could not be generated
            BOOST_DELETED_FUNCTION(complex_kwd_function& operator= (complex_kwd_function const&))
        };


}}}}}

#endif

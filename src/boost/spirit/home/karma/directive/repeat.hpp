//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2001-2011 Joel de Guzman
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SPIRIT_KARMA_DIRECTIVE_REPEAT_HPP
#define BOOST_SPIRIT_KARMA_DIRECTIVE_REPEAT_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/detail/output_iterator.hpp>
#include <boost/spirit/home/karma/detail/get_stricttag.hpp>
#include <boost/spirit/home/karma/generator.hpp>
#include <boost/spirit/home/karma/auxiliary/lazy.hpp>
#include <boost/spirit/home/karma/operator/kleene.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/spirit/home/karma/detail/attributes.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/fusion/include/at.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_directive<karma::domain, tag::repeat>   // enables repeat[p]
      : mpl::true_ {};

    template <typename T>
    struct use_directive<karma::domain
      , terminal_ex<tag::repeat                     // enables repeat(exact)[p]
        , fusion::vector1<T> >
    > : mpl::true_ {};

    template <typename T>
    struct use_directive<karma::domain
      , terminal_ex<tag::repeat                     // enables repeat(min, max)[p]
        , fusion::vector2<T, T> >
    > : mpl::true_ {};

    template <typename T>
    struct use_directive<karma::domain
      , terminal_ex<tag::repeat                     // enables repeat(min, inf)[p]
        , fusion::vector2<T, inf_type> >
    > : mpl::true_ {};

    template <>                                     // enables *lazy* repeat(exact)[p]
    struct use_lazy_directive<
        karma::domain
      , tag::repeat
      , 1 // arity
    > : mpl::true_ {};

    template <>                                     // enables *lazy* repeat(min, max)[p]
    struct use_lazy_directive<                      // and repeat(min, inf)[p]
        karma::domain
      , tag::repeat
      , 2 // arity
    > : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace karma
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::repeat;
    using spirit::inf;
#endif
    using spirit::repeat_type;
    using spirit::inf_type;

    ///////////////////////////////////////////////////////////////////////////
    // handles repeat(exact)[p]
    template <typename T>
    struct exact_iterator
    {
        exact_iterator(T const exact)
          : exact(exact) {}

        typedef T type;
        T start() const { return 0; }
        bool got_max(T i) const { return i >= exact; }
        bool got_min(T i) const { return i >= exact; }

        T const exact;

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(exact_iterator& operator= (exact_iterator const&))
    };

    // handles repeat(min, max)[p]
    template <typename T>
    struct finite_iterator
    {
        finite_iterator(T const min, T const max)
          : min BOOST_PREVENT_MACRO_SUBSTITUTION (min)
          , max BOOST_PREVENT_MACRO_SUBSTITUTION (max) {}

        typedef T type;
        T start() const { return 0; }
        bool got_max(T i) const { return i >= max; }
        bool got_min(T i) const { return i >= min; }

        T const min;
        T const max;

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(finite_iterator& operator= (finite_iterator const&))
    };

    // handles repeat(min, inf)[p]
    template <typename T>
    struct infinite_iterator
    {
        infinite_iterator(T const min)
          : min BOOST_PREVENT_MACRO_SUBSTITUTION (min) {}

        typedef T type;
        T start() const { return 0; }
        bool got_max(T /*i*/) const { return false; }
        bool got_min(T i) const { return i >= min; }

        T const min;

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(infinite_iterator& operator= (infinite_iterator const&))
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename LoopIter, typename Strict
      , typename Derived>
    struct base_repeat_generator : unary_generator<Derived>
    {
    private:
        // iterate over the given container until its exhausted or the embedded
        // generator succeeds
        template <typename F, typename Attribute>
        bool generate_subject(F f, Attribute const&, mpl::false_) const
        {
            // Failing subject generators are just skipped. This allows to 
            // selectively generate items in the provided attribute.
            while (!f.is_at_end())
            {
                bool r = !f(subject);
                if (r) 
                    return true;
                if (!f.is_at_end())
                    f.next();
            }
            return false;
        }

        template <typename F, typename Attribute>
        bool generate_subject(F f, Attribute const&, mpl::true_) const
        {
            return !f(subject);
        }

        // There is no way to distinguish a failed generator from a 
        // generator to be skipped. We assume the user takes responsibility
        // for ending the loop if no attribute is specified.
        template <typename F>
        bool generate_subject(F f, unused_type, mpl::false_) const
        {
            return !f(subject);
        }

    public:
        typedef Subject subject_type;

        typedef mpl::int_<subject_type::properties::value> properties;

        // Build a std::vector from the subject's attribute. Note
        // that build_std_vector may return unused_type if the
        // subject's attribute is an unused_type.
        template <typename Context, typename Iterator>
        struct attribute
          : traits::build_std_vector<
                typename traits::attribute_of<Subject, Context, Iterator>::type
            >
        {};

        base_repeat_generator(Subject const& subject, LoopIter const& iter)
          : subject(subject), iter(iter) {}

        template <typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& ctx, Delimiter const& d
          , Attribute const& attr) const
        {
            typedef detail::fail_function<
                OutputIterator, Context, Delimiter
            > fail_function;

            typedef typename traits::container_iterator<
                typename add_const<Attribute>::type
            >::type iterator_type;

            typedef 
                typename traits::make_indirect_iterator<iterator_type>::type 
            indirect_iterator_type;

            typedef detail::pass_container<
                fail_function, Attribute, indirect_iterator_type, mpl::false_>
            pass_container;

            iterator_type it = traits::begin(attr);
            iterator_type end = traits::end(attr);

            pass_container pass(fail_function(sink, ctx, d), 
                indirect_iterator_type(it), indirect_iterator_type(end));

            // generate the minimal required amount of output
            typename LoopIter::type i = iter.start();
            for (/**/; !pass.is_at_end() && !iter.got_min(i); ++i)
            {
                if (!generate_subject(pass, attr, Strict()))
                {
                    // if we fail before reaching the minimum iteration
                    // required, do not output anything and return false
                    return false;
                }
            }

            if (pass.is_at_end() && !iter.got_min(i))
                return false;   // insufficient attribute elements

            // generate some more up to the maximum specified
            for (/**/; !pass.is_at_end() && !iter.got_max(i); ++i)
            {
                if (!generate_subject(pass, attr, Strict()))
                    break;
            }
            return detail::sink_is_good(sink);
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("repeat", subject.what(context));
        }

        Subject subject;
        LoopIter iter;
    };

    template <typename Subject, typename LoopIter>
    struct repeat_generator 
      : base_repeat_generator<
            Subject, LoopIter, mpl::false_
          , repeat_generator<Subject, LoopIter> >
    {
        typedef base_repeat_generator<
            Subject, LoopIter, mpl::false_, repeat_generator
        > base_repeat_generator_;

        repeat_generator(Subject const& subject, LoopIter const& iter)
          : base_repeat_generator_(subject, iter) {}
    };

    template <typename Subject, typename LoopIter>
    struct strict_repeat_generator 
      : base_repeat_generator<
            Subject, LoopIter, mpl::true_
          , strict_repeat_generator<Subject, LoopIter> >
    {
        typedef base_repeat_generator<
            Subject, LoopIter, mpl::true_, strict_repeat_generator
        > base_repeat_generator_;

        strict_repeat_generator(Subject const& subject, LoopIter const& iter)
          : base_repeat_generator_(subject, iter) {}
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Modifiers>
    struct make_directive<tag::repeat, Subject, Modifiers>
    {
        typedef typename mpl::if_<
            detail::get_stricttag<Modifiers>
          , strict_kleene<Subject>, kleene<Subject>
        >::type result_type;

        result_type operator()(unused_type, Subject const& subject
          , unused_type) const
        {
            return result_type(subject);
        }
    };

    template <typename T, typename Subject, typename Modifiers>
    struct make_directive<
        terminal_ex<tag::repeat, fusion::vector1<T> >, Subject, Modifiers>
    {
        typedef exact_iterator<T> iterator_type;

        typedef typename mpl::if_<
            detail::get_stricttag<Modifiers>
          , strict_repeat_generator<Subject, iterator_type>
          , repeat_generator<Subject, iterator_type>
        >::type result_type;

        template <typename Terminal>
        result_type operator()(
            Terminal const& term, Subject const& subject, unused_type) const
        {
            return result_type(subject, fusion::at_c<0>(term.args));
        }
    };

    template <typename T, typename Subject, typename Modifiers>
    struct make_directive<
        terminal_ex<tag::repeat, fusion::vector2<T, T> >, Subject, Modifiers>
    {
        typedef finite_iterator<T> iterator_type;

        typedef typename mpl::if_<
            detail::get_stricttag<Modifiers>
          , strict_repeat_generator<Subject, iterator_type>
          , repeat_generator<Subject, iterator_type>
        >::type result_type;

        template <typename Terminal>
        result_type operator()(
            Terminal const& term, Subject const& subject, unused_type) const
        {
            return result_type(subject,
                iterator_type(
                    fusion::at_c<0>(term.args)
                  , fusion::at_c<1>(term.args)
                )
            );
        }
    };

    template <typename T, typename Subject, typename Modifiers>
    struct make_directive<
        terminal_ex<tag::repeat
        , fusion::vector2<T, inf_type> >, Subject, Modifiers>
    {
        typedef infinite_iterator<T> iterator_type;

        typedef typename mpl::if_<
            detail::get_stricttag<Modifiers>
          , strict_repeat_generator<Subject, iterator_type>
          , repeat_generator<Subject, iterator_type>
        >::type result_type;

        template <typename Terminal>
        result_type operator()(
            Terminal const& term, Subject const& subject, unused_type) const
        {
            return result_type(subject, fusion::at_c<0>(term.args));
        }
    };
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename LoopIter>
    struct has_semantic_action<karma::repeat_generator<Subject, LoopIter> >
      : unary_has_semantic_action<Subject> {};

    template <typename Subject, typename LoopIter>
    struct has_semantic_action<karma::strict_repeat_generator<Subject, LoopIter> >
      : unary_has_semantic_action<Subject> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename LoopIter, typename Attribute
      , typename Context, typename Iterator>
    struct handles_container<
            karma::repeat_generator<Subject, LoopIter>, Attribute
          , Context, Iterator> 
      : mpl::true_ {};

    template <typename Subject, typename LoopIter, typename Attribute
      , typename Context, typename Iterator>
    struct handles_container<
            karma::strict_repeat_generator<Subject, LoopIter>, Attribute
          , Context, Iterator> 
      : mpl::true_ {};
}}}

#endif

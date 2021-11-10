//  Copyright (c) 2009 Francois Barel
//  Copyright (c) 2001-2011 Joel de Guzman
//  Copyright (c) 2001-2012 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_REPOSITORY_KARMA_SUBRULE_AUGUST_12_2009_0813PM)
#define BOOST_SPIRIT_REPOSITORY_KARMA_SUBRULE_AUGUST_12_2009_0813PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/generator.hpp>
#include <boost/spirit/home/karma/reference.hpp>
#include <boost/spirit/home/karma/nonterminal/detail/generator_binder.hpp>
#include <boost/spirit/home/karma/nonterminal/detail/parameterized.hpp>
#include <boost/spirit/home/support/argument.hpp>
#include <boost/spirit/home/support/assert_msg.hpp>
#include <boost/spirit/home/karma/detail/attributes.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/nonterminal/extract_param.hpp>
#include <boost/spirit/home/support/nonterminal/locals.hpp>
#include <boost/spirit/repository/home/support/subrule_context.hpp>

#include <boost/static_assert.hpp>
#include <boost/fusion/include/as_map.hpp>
#include <boost/fusion/include/at_key.hpp>
#include <boost/fusion/include/cons.hpp>
#include <boost/fusion/include/front.hpp>
#include <boost/fusion/include/has_key.hpp>
#include <boost/fusion/include/join.hpp>
#include <boost/fusion/include/make_map.hpp>
#include <boost/fusion/include/make_vector.hpp>
#include <boost/fusion/include/size.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/proto/extends.hpp>
#include <boost/proto/traits.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_reference.hpp>

#if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable: 4355) // 'this' : used in base member initializer list warning
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace repository { namespace karma
{
    ///////////////////////////////////////////////////////////////////////////
    // subrule_group_generator:
    // - generator representing a group of subrule definitions (one or more),
    //   invokes first subrule on entry,
    ///////////////////////////////////////////////////////////////////////////
    template <typename Defs>
    struct subrule_group_generator
      : spirit::karma::generator<subrule_group_generator<Defs> >
    {
        // Fusion associative sequence, associating each subrule ID in this
        // group (as an MPL integral constant) with its definition
        typedef Defs defs_type;

        typedef subrule_group_generator<Defs> this_type;

        explicit subrule_group_generator(Defs const& defs)
          : defs(defs)
        {
        }
        // from a subrule ID, get the type of a reference to its definition
        template <int ID>
        struct def_type
        {
            typedef mpl::int_<ID> id_type;

            // If you are seeing a compilation error here, you are trying
            // to use a subrule which was not defined in this group.
            BOOST_SPIRIT_ASSERT_MSG(
                (fusion::result_of::has_key<
                    defs_type const, id_type>::type::value)
              , subrule_used_without_being_defined, (mpl::int_<ID>));

            typedef typename
                fusion::result_of::at_key<defs_type const, id_type>::type
            type;
        };

        // from a subrule ID, get a reference to its definition
        template <int ID>
        typename def_type<ID>::type def() const
        {
            return fusion::at_key<mpl::int_<ID> >(defs);
        }

        template <typename Context, typename Iterator>
        struct attribute
            // Forward to first subrule.
          : mpl::identity<
                typename remove_reference<
                    typename fusion::result_of::front<Defs>::type
                >::type::second_type::attr_type> {};

        template <typename OutputIterator, typename Context
          , typename Delimiter, typename Attribute>
        bool generate(OutputIterator& sink, Context& context
          , Delimiter const& delimiter, Attribute const& attr) const
        {
            // Forward to first subrule.
            return generate_subrule(fusion::front(defs).second
              , sink, context, delimiter, attr);
        }

        template <typename OutputIterator, typename Context
          , typename Delimiter, typename Attribute
          , typename Params>
        bool generate(OutputIterator& sink, Context& context
          , Delimiter const& delimiter, Attribute const& attr
          , Params const& params) const
        {
            // Forward to first subrule.
            return generate_subrule(fusion::front(defs).second
              , sink, context, delimiter, attr, params);
        }

        template <int ID, typename OutputIterator, typename Context
          , typename Delimiter, typename Attribute>
        bool generate_subrule_id(OutputIterator& sink
          , Context& context, Delimiter const& delimiter
          , Attribute const& attr) const
        {
            return generate_subrule(def<ID>()
              , sink, context, delimiter, attr);
        }

        template <int ID, typename OutputIterator, typename Context
          , typename Delimiter, typename Attribute, typename Params>
        bool generate_subrule_id(OutputIterator& sink
          , Context& context, Delimiter const& delimiter
          , Attribute const& attr, Params const& params) const
        {
            return generate_subrule(def<ID>()
              , sink, context, delimiter, attr, params);
        }

        template <typename Def, typename OutputIterator, typename Context
          , typename Delimiter, typename Attribute>
        bool generate_subrule(Def const& def, OutputIterator& sink
          , Context& /*caller_context*/, Delimiter const& delimiter
          , Attribute const& attr) const
        {
            // compute context type for this subrule
            typedef typename Def::locals_type subrule_locals_type;
            typedef typename Def::attr_type subrule_attr_type;
            typedef typename Def::attr_reference_type subrule_attr_reference_type;
            typedef typename Def::parameter_types subrule_parameter_types;

            typedef
                subrule_context<
                    this_type
                  , fusion::cons<
                        subrule_attr_reference_type, subrule_parameter_types>
                  , subrule_locals_type
                >
            context_type;

            typedef traits::transform_attribute<Attribute const
              , subrule_attr_type, spirit::karma::domain> transform;

            // If you are seeing a compilation error here, you are probably
            // trying to use a subrule which has inherited attributes,
            // without passing values for them.
            context_type context(*this, transform::pre(attr));

            return def.binder(sink, context, delimiter);
        }

        template <typename Def, typename OutputIterator, typename Context
          , typename Delimiter, typename Attribute, typename Params>
        bool generate_subrule(Def const& def, OutputIterator& sink
          , Context& caller_context, Delimiter const& delimiter
          , Attribute const& attr, Params const& params) const
        {
            // compute context type for this subrule
            typedef typename Def::locals_type subrule_locals_type;
            typedef typename Def::attr_type subrule_attr_type;
            typedef typename Def::attr_reference_type subrule_attr_reference_type;
            typedef typename Def::parameter_types subrule_parameter_types;

            typedef
                subrule_context<
                    this_type
                  , fusion::cons<
                        subrule_attr_reference_type, subrule_parameter_types>
                  , subrule_locals_type
                >
            context_type;

            typedef traits::transform_attribute<Attribute const
              , subrule_attr_type, spirit::karma::domain> transform;

            // If you are seeing a compilation error here, you are probably
            // trying to use a subrule which has inherited attributes,
            // passing values of incompatible types for them.
            context_type context(*this
              , transform::pre(attr), params, caller_context);

            return def.binder(sink, context, delimiter);
        }

        template <typename Context>
        info what(Context& context) const
        {
            // Forward to first subrule.
            return fusion::front(defs).second.binder.g.what(context);
        }

        Defs defs;
    };

    ///////////////////////////////////////////////////////////////////////////
    // subrule_group:
    // - a Proto terminal, so that a group behaves like any Spirit
    //   expression.
    ///////////////////////////////////////////////////////////////////////////
    template <typename Defs>
    struct subrule_group
      : proto::extends<
            typename proto::terminal<
                subrule_group_generator<Defs>
            >::type
          , subrule_group<Defs>
        >
    {
        typedef subrule_group_generator<Defs> generator_type;
        typedef typename proto::terminal<generator_type>::type terminal;

        struct properties
            // Forward to first subrule.
          : remove_reference<
                typename fusion::result_of::front<Defs>::type
            >::type::second_type::subject_type::properties {};

        static size_t const params_size =
            // Forward to first subrule.
            remove_reference<
                typename fusion::result_of::front<Defs>::type
            >::type::second_type::params_size;

        explicit subrule_group(Defs const& defs)
          : subrule_group::proto_extends(terminal::make(generator_type(defs)))
        {
        }

        generator_type const& generator() const { return proto::value(*this); }

        Defs const& defs() const { return generator().defs; }

        template <typename Defs2>
        subrule_group<
            typename fusion::result_of::as_map<
                typename fusion::result_of::join<
                    Defs const, Defs2 const>::type>::type>
        operator,(subrule_group<Defs2> const& other) const
        {
            typedef subrule_group<
                typename fusion::result_of::as_map<
                    typename fusion::result_of::join<
                        Defs const, Defs2 const>::type>::type> result_type;
            return result_type(fusion::as_map(fusion::join(defs(), other.defs())));
        }

        // non-const versions needed to suppress proto's comma op kicking in
        template <typename Defs2>
        friend subrule_group<
            typename fusion::result_of::as_map<
                typename fusion::result_of::join<
                    Defs const, Defs2 const>::type>::type>
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
        operator,(subrule_group&& left, subrule_group<Defs2>&& other)
#else
        operator,(subrule_group& left, subrule_group<Defs2>& other)
#endif
        {
            return static_cast<subrule_group const&>(left)
                .operator,(static_cast<subrule_group<Defs2> const&>(other));
        }

        // bring in the operator() overloads
        generator_type const& get_parameterized_subject() const { return generator(); }
        typedef generator_type parameterized_subject_type;
        #include <boost/spirit/home/karma/nonterminal/detail/fcall.hpp>
    };

    ///////////////////////////////////////////////////////////////////////////
    // subrule_definition: holds one particular definition of a subrule
    ///////////////////////////////////////////////////////////////////////////
    template <
        int ID_
      , typename Locals
      , typename Attr
      , typename AttrRef
      , typename Parameters
      , size_t ParamsSize
      , typename Subject
      , bool Auto_
    >
    struct subrule_definition
    {
        typedef mpl::int_<ID_> id_type;
        BOOST_STATIC_CONSTANT(int, ID = ID_);

        typedef Locals locals_type;
        typedef Attr attr_type;
        typedef AttrRef attr_reference_type;
        typedef Parameters parameter_types;
        static size_t const params_size = ParamsSize;

        typedef Subject subject_type;
        typedef mpl::bool_<Auto_> auto_type;
        BOOST_STATIC_CONSTANT(bool, Auto = Auto_);

        typedef spirit::karma::detail::generator_binder<
            Subject, auto_type> binder_type;

        subrule_definition(Subject const& subject, std::string const& name)
          : binder(subject), name(name)
        {
        }

        binder_type const binder;
        std::string const name;
    };

    ///////////////////////////////////////////////////////////////////////////
    // subrule placeholder:
    // - on subrule definition: helper for creation of subrule_group,
    // - on subrule invocation: Proto terminal and generator.
    ///////////////////////////////////////////////////////////////////////////
    template <
        int ID_
      , typename T1 = unused_type
      , typename T2 = unused_type
    >
    struct subrule
      : proto::extends<
            typename proto::terminal<
                spirit::karma::reference<subrule<ID_, T1, T2> const>
            >::type
          , subrule<ID_, T1, T2>
        >
      , spirit::karma::generator<subrule<ID_, T1, T2> >
    {
        //FIXME should go fetch the real properties of this subrule's definition in the current context, but we don't
        // have the context here (properties would need to be 'template<typename Context> struct properties' instead)
        typedef mpl::int_<
            spirit::karma::generator_properties::all_properties> properties;

        typedef mpl::int_<ID_> id_type;
        BOOST_STATIC_CONSTANT(int, ID = ID_);

        typedef subrule<ID_, T1, T2> this_type;
        typedef spirit::karma::reference<this_type const> reference_;
        typedef typename proto::terminal<reference_>::type terminal;
        typedef proto::extends<terminal, this_type> base_type;

        typedef mpl::vector<T1, T2> template_params;

        // The subrule's locals_type: a sequence of types to be used as local variables
        typedef typename
            spirit::detail::extract_locals<template_params>::type
        locals_type;

        // The subrule's encoding type
        typedef typename
            spirit::detail::extract_encoding<template_params>::type
        encoding_type;

        // The subrule's signature
        typedef typename
            spirit::detail::extract_sig<template_params, encoding_type
              , spirit::karma::domain>::type
        sig_type;

        // This is the subrule's attribute type
        typedef typename
            spirit::detail::attr_from_sig<sig_type>::type
        attr_type;
        BOOST_STATIC_ASSERT_MSG(
            !is_reference<attr_type>::value && !is_const<attr_type>::value,
            "Const/reference qualifiers on Karma subrule attribute are meaningless");
        typedef attr_type const& attr_reference_type;

        // parameter_types is a sequence of types passed as parameters to the subrule
        typedef typename
            spirit::detail::params_from_sig<sig_type>::type
        parameter_types;

        static size_t const params_size =
            fusion::result_of::size<parameter_types>::type::value;

        explicit subrule(std::string const& name_ = "unnamed-subrule")
          : base_type(terminal::make(reference_(*this)))
          , name_(name_)
        {
        }

        // compute type of this subrule's definition for expr type Expr
        template <typename Expr, bool Auto>
        struct def_type_helper
        {
            // Report invalid expression error as early as possible.
            // If you got an error_invalid_expression error message here,
            // then the expression (Expr) is not a valid spirit karma expression.
            BOOST_SPIRIT_ASSERT_MATCH(spirit::karma::domain, Expr);

            typedef typename result_of::compile<
                spirit::karma::domain, Expr>::type subject_type;

            typedef subrule_definition<
                ID_
              , locals_type
              , attr_type
              , attr_reference_type
              , parameter_types
              , params_size
              , subject_type
              , Auto
            > const type;
        };

        // compute type of subrule group containing only this
        // subrule's definition for expr type Expr
        template <typename Expr, bool Auto>
        struct group_type_helper
        {
            typedef typename def_type_helper<Expr, Auto>::type def_type;

            // create Defs map with only one entry: (ID -> def)
            typedef typename
#ifndef BOOST_FUSION_HAS_VARIADIC_MAP
                fusion::result_of::make_map<id_type, def_type>::type
#else
                fusion::result_of::make_map<id_type>::template apply<def_type>::type
#endif
            defs_type;

            typedef subrule_group<defs_type> type;
        };

        template <typename Expr>
        typename group_type_helper<Expr, false>::type
        operator=(Expr const& expr) const
        {
            typedef group_type_helper<Expr, false> helper;
            typedef typename helper::def_type def_type;
            typedef typename helper::type result_type;
            return result_type(fusion::make_map<id_type>(
                def_type(compile<spirit::karma::domain>(expr), name_)));
        }

#define BOOST_SPIRIT_SUBRULE_MODULUS_ASSIGN_OPERATOR(lhs_ref, rhs_ref)        \
        template <typename Expr>                                              \
        friend typename group_type_helper<Expr, true>::type                   \
        operator%=(subrule lhs_ref sr, Expr rhs_ref expr)                     \
        {                                                                     \
            typedef group_type_helper<Expr, true> helper;                     \
            typedef typename helper::def_type def_type;                       \
            typedef typename helper::type result_type;                        \
            return result_type(fusion::make_map<id_type>(                     \
                def_type(compile<spirit::karma::domain>(expr), sr.name_)));   \
        }                                                                     \
        /**/

        // non-const versions needed to suppress proto's %= kicking in
        BOOST_SPIRIT_SUBRULE_MODULUS_ASSIGN_OPERATOR(const&, const&)
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
        BOOST_SPIRIT_SUBRULE_MODULUS_ASSIGN_OPERATOR(const&, &&)
#else
        BOOST_SPIRIT_SUBRULE_MODULUS_ASSIGN_OPERATOR(const&, &)
#endif
        BOOST_SPIRIT_SUBRULE_MODULUS_ASSIGN_OPERATOR(&, const&)
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
        BOOST_SPIRIT_SUBRULE_MODULUS_ASSIGN_OPERATOR(&, &&)
#else
        BOOST_SPIRIT_SUBRULE_MODULUS_ASSIGN_OPERATOR(&, &)
#endif

#undef BOOST_SPIRIT_SUBRULE_MODULUS_ASSIGN_OPERATOR

        std::string const& name() const
        {
            return name_;
        }

        void name(std::string const& str)
        {
            name_ = str;
        }

        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef attr_type type;
        };

        template <typename OutputIterator, typename Group
          , typename Attributes, typename Locals
          , typename Delimiter, typename Attribute>
        bool generate(OutputIterator& sink
          , subrule_context<Group, Attributes, Locals>& context
          , Delimiter const& delimiter, Attribute const& attr) const
        {
            return context.group.template generate_subrule_id<ID_>(
                sink, context, delimiter, attr);
        }

        template <typename OutputIterator, typename Context
          , typename Delimiter, typename Attribute>
        bool generate(OutputIterator& /*sink*/
          , Context& /*context*/
          , Delimiter const& /*delimiter*/, Attribute const& /*attr*/) const
        {
            // If you are seeing a compilation error here, you are trying
            // to use a subrule as a generator outside of a subrule group.
            BOOST_SPIRIT_ASSERT_FAIL(OutputIterator
              , subrule_used_outside_subrule_group, (id_type));

            return false;
        }

        template <typename OutputIterator, typename Group
          , typename Attributes, typename Locals
          , typename Delimiter, typename Attribute
          , typename Params>
        bool generate(OutputIterator& sink
          , subrule_context<Group, Attributes, Locals>& context
          , Delimiter const& delimiter, Attribute const& attr
          , Params const& params) const
        {
            return context.group.template generate_subrule_id<ID_>(
                sink, context, delimiter, attr, params);
        }

        template <typename OutputIterator, typename Context
          , typename Delimiter, typename Attribute
          , typename Params>
        bool generate(OutputIterator& /*sink*/
          , Context& /*context*/
          , Delimiter const& /*delimiter*/, Attribute const& /*attr*/
          , Params const& /*params*/) const
        {
            // If you are seeing a compilation error here, you are trying
            // to use a subrule as a generator outside of a subrule group.
            BOOST_SPIRIT_ASSERT_FAIL(OutputIterator
              , subrule_used_outside_subrule_group, (id_type));

            return false;
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info(name_);
        }

        // bring in the operator() overloads
        this_type const& get_parameterized_subject() const { return *this; }
        typedef this_type parameterized_subject_type;
        #include <boost/spirit/home/karma/nonterminal/detail/fcall.hpp>

        std::string name_;
    };
}}}}

#if defined(BOOST_MSVC)
# pragma warning(pop)
#endif

#endif

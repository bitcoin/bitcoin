// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_FRONT_EUML_GUARD_GRAMMAR_H
#define BOOST_MSM_FRONT_EUML_GUARD_GRAMMAR_H

#include <boost/msm/front/euml/common.hpp>
#include <boost/msm/front/euml/operator.hpp>
#include <boost/msm/front/euml/state_grammar.hpp>

namespace boost { namespace msm { namespace front { namespace euml
{
struct BuildGuards;
struct BuildActions;

struct BuildGuardsCases
{
    // The primary template matches nothing:
    template<typename Tag>
    struct case_
        : proto::not_<proto::_>
    {};
};
template<>
struct BuildGuardsCases::case_<proto::tag::logical_or>
    : proto::when<
                    proto::logical_or<BuildGuards,BuildGuards >,
                    Or_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::logical_and>
    : proto::when<
                    proto::logical_and<BuildGuards,BuildGuards >,
                    And_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::logical_not>
    : proto::when<
                    proto::logical_not<BuildGuards >,
                    Not_<BuildGuards(proto::_child)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::less>
    : proto::when<
                    proto::less<BuildGuards, BuildGuards >,
                    Less_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::less_equal>
    : proto::when<
                    proto::less_equal<BuildGuards, BuildGuards >,
                    LessEqual_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::greater>
    : proto::when<
                    proto::greater<BuildGuards, BuildGuards >,
                    Greater_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::greater_equal>
    : proto::when<
                    proto::greater_equal<BuildGuards, BuildGuards >,
                    GreaterEqual_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::equal_to>
    : proto::when<
                        proto::equal_to<BuildGuards, BuildGuards >,
                        EqualTo_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::not_equal_to>
    : proto::when<
                        proto::not_equal_to<BuildGuards, BuildGuards >,
                        NotEqualTo_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::pre_inc>
    : proto::when<
                    proto::pre_inc<BuildGuards >,
                    Pre_inc_<BuildGuards(proto::_child)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::dereference>
    : proto::when<
                    proto::dereference<BuildGuards >,
                    Deref_<BuildGuards(proto::_child)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::pre_dec>
    : proto::when<
                    proto::pre_dec<BuildGuards >,
                    Pre_dec_<BuildGuards(proto::_child)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::post_inc>
    : proto::when<
                    proto::post_inc<BuildGuards >,
                    Post_inc_<BuildGuards(proto::_child)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::post_dec>
    : proto::when<
                    proto::post_dec<BuildGuards >,
                    Post_dec_<BuildGuards(proto::_child)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::plus>
    : proto::when<
                    proto::plus<BuildGuards,BuildGuards >,
                    Plus_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::minus>
    : proto::when<
                    proto::minus<BuildGuards,BuildGuards >,
                    Minus_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::multiplies>
    : proto::when<
                    proto::multiplies<BuildGuards,BuildGuards >,
                    Multiplies_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::divides>
    : proto::when<
                    proto::divides<BuildGuards,BuildGuards >,
                    Divides_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::modulus>
    : proto::when<
                    proto::modulus<BuildGuards,BuildGuards >,
                    Modulus_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::bitwise_and>
    : proto::when<
                    proto::bitwise_and<BuildGuards,BuildGuards >,
                    Bitwise_And_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::bitwise_or>
    : proto::when<
                    proto::bitwise_or<BuildGuards,BuildGuards >,
                    Bitwise_Or_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::subscript>
    : proto::when<
                    proto::subscript<BuildGuards,BuildGuards >,
                    Subscript_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::plus_assign>
    : proto::when<
                    proto::plus_assign<BuildGuards,BuildGuards >,
                    Plus_Assign_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::minus_assign>
    : proto::when<
                    proto::minus_assign<BuildGuards,BuildGuards >,
                    Minus_Assign_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::multiplies_assign>
    : proto::when<
                    proto::multiplies_assign<BuildGuards,BuildGuards >,
                    Multiplies_Assign_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::divides_assign>
    : proto::when<
                    proto::divides_assign<BuildGuards,BuildGuards >,
                    Divides_Assign_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::modulus_assign>
    : proto::when<
                    proto::modulus_assign<BuildGuards,BuildGuards >,
                    Modulus_Assign_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::shift_left_assign>
    : proto::when<
                    proto::shift_left_assign<BuildGuards,BuildGuards >,
                    ShiftLeft_Assign_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::shift_right_assign>
    : proto::when<
                    proto::shift_right_assign<BuildGuards,BuildGuards >,
                    ShiftRight_Assign_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::shift_left>
    : proto::when<
                    proto::shift_left<BuildGuards,BuildGuards >,
                    ShiftLeft_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::shift_right>
    : proto::when<
                    proto::shift_right<BuildGuards,BuildGuards >,
                    ShiftRight_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::assign>
    : proto::when<
                    proto::assign<BuildGuards,BuildGuards >,
                    Assign_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::bitwise_xor>
    : proto::when<
                    proto::bitwise_xor<BuildGuards,BuildGuards >,
                    Bitwise_Xor_<BuildGuards(proto::_left),BuildGuards(proto::_right)>()
                >
{};
template<>
struct BuildGuardsCases::case_<proto::tag::negate>
    : proto::when<
                    proto::negate<BuildGuards >,
                    Unary_Minus_<BuildGuards(proto::_child)>()
                >
{};

template<>
struct BuildGuardsCases::case_<proto::tag::function>
    : proto::or_<
            proto::when<
                    proto::function<proto::terminal<if_tag>,BuildGuards,BuildGuards,BuildGuards >,
                    If_Else_<BuildGuards(proto::_child_c<1>),
                             BuildGuards(proto::_child_c<2>),
                             BuildGuards(proto::_child_c<3>) >()
                    >,
            proto::when<
                    proto::function<proto::terminal<proto::_> >,
                    get_fct<proto::_child_c<0> >()
                    >,
            proto::when<
                    proto::function<proto::terminal<proto::_>,BuildActions >,
                    get_fct<proto::_child_c<0>,BuildActions(proto::_child_c<1>) >()
                    >,
            proto::when<
                    proto::function<proto::terminal<proto::_>,BuildActions,BuildActions >,
                    get_fct<proto::_child_c<0>,BuildActions(proto::_child_c<1>),BuildActions(proto::_child_c<2>) >()
                    >,
            proto::when<
                    proto::function<proto::terminal<proto::_>,BuildActions,BuildActions,BuildActions >,
                    get_fct<proto::_child_c<0>,BuildActions(proto::_child_c<1>)
                                              ,BuildActions(proto::_child_c<2>),BuildActions(proto::_child_c<3>) >()
                    >,
            proto::when<
                    proto::function<proto::terminal<proto::_>,BuildActions,BuildActions,BuildActions,BuildActions >,
                    get_fct<proto::_child_c<0>
                            ,BuildActions(proto::_child_c<1>),BuildActions(proto::_child_c<2>)
                            ,BuildActions(proto::_child_c<3>),BuildActions(proto::_child_c<4>) >()
                    >,
            proto::when<
                    proto::function<proto::terminal<proto::_>,BuildActions,BuildActions,BuildActions,BuildActions,BuildActions  >,
                    get_fct<proto::_child_c<0>
                            ,BuildActions(proto::_child_c<1>),BuildActions(proto::_child_c<2>)
                            ,BuildActions(proto::_child_c<3>),BuildActions(proto::_child_c<4>),BuildActions(proto::_child_c<5>) >()
                    >
#ifdef BOOST_MSVC
            ,proto::when<
                    proto::function<proto::terminal<proto::_>,BuildActions,BuildActions,BuildActions,BuildActions,BuildActions,BuildActions >,
                    get_fct<proto::_child_c<0>
                            ,BuildActions(proto::_child_c<1>),BuildActions(proto::_child_c<2>)
                            ,BuildActions(proto::_child_c<3>),BuildActions(proto::_child_c<4>)
                            ,BuildActions(proto::_child_c<5>),BuildActions(proto::_child_c<6>) >()
                    >
#endif
    >
{};

template<>
struct BuildGuardsCases::case_<proto::tag::terminal>
    : proto::or_<
        proto::when <
            proto::terminal<action_tag>,
            get_action_name<proto::_ >()
            >,
        proto::when<
            proto::terminal<state_tag>,
            get_state_name<proto::_>()
            >,
        proto::when<
            proto::terminal<flag_tag>,
            proto::_
            >,
        proto::when<
            proto::terminal<event_tag>,
            proto::_
            >,
        proto::when<
            proto::terminal<fsm_artefact_tag>,
            get_fct<proto::_ >()
            >,
        proto::when<
            proto::terminal<proto::_>,
            proto::_value
            >
    >
{};

struct BuildGuards
    : proto::switch_<BuildGuardsCases>
{};

}}}}

#endif //BOOST_MSM_FRONT_EUML_GUARD_GRAMMAR_H

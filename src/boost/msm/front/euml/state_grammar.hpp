// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_FRONT_EUML_STATE_GRAMMAR_H
#define BOOST_MSM_FRONT_EUML_STATE_GRAMMAR_H

#ifdef BOOST_MSM_EUML_PHOENIX_SUPPORT
#include <boost/phoenix/core/meta_grammar.hpp>
#endif

#include <boost/msm/front/euml/common.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/include/pair.hpp>
#include <boost/fusion/include/as_map.hpp>

#include <boost/mpl/remove_if.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/assert.hpp>

#include <boost/msm/row_tags.hpp>
#include <boost/msm/front/common_states.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/euml/operator.hpp>
#include <boost/msm/front/euml/guard_grammar.hpp>

BOOST_MPL_HAS_XXX_TRAIT_DEF(attribute_tag)
BOOST_MPL_HAS_XXX_TRAIT_DEF(flag_create_tag)
BOOST_MPL_HAS_XXX_TRAIT_DEF(defer_create_tag)
BOOST_MPL_HAS_XXX_TRAIT_DEF(control_configure_tag)

namespace proto = boost::proto;

namespace boost { namespace msm { namespace front { namespace euml
{

// provides the typedefs and interface. Concrete states derive from it.
template<class StateNameTag,
         class EntryFunctor=NoAction, 
         class ExitFunctor=NoAction,
         class Attributes= ::boost::fusion::vector<>,
         class Flags = ::boost::mpl::vector0<>,
         class Defer = ::boost::mpl::vector0<>,
         class BASE = ::boost::msm::front::default_base_state>
struct func_state :  public ::boost::msm::front::detail::state_base<BASE,Attributes>, 
                     euml_state_intern<func_state<StateNameTag,EntryFunctor,ExitFunctor,Attributes,Flags,Defer,BASE> >
{
    func_state(){}
    // grammar testing
    BOOST_MPL_ASSERT_NOT(( boost::is_same<EntryFunctor,invalid_type> ));
    BOOST_MPL_ASSERT_NOT(( boost::is_same<ExitFunctor,invalid_type> ));

    typedef StateNameTag state_name_tag;
    // flags
    typedef Flags       flag_list;
    typedef ::boost::mpl::vector0<> internal_flag_list;
    // deferred events
    typedef Defer       deferred_events;

    template <class Event,class FSM>
    void on_entry(Event const& evt,FSM& fsm)
    {
        EntryFunctor()(evt,fsm,*this);
    }
    template <class Event,class FSM>
    void on_exit(Event const& evt,FSM& fsm)
    {
        ExitFunctor()(evt,fsm,*this);
    }
};

// provides the typedefs and interface. Concrete states derive from it.
template<class StateNameTag,
         int ZoneIndex=-1,
         class EntryFunctor=NoAction, 
         class ExitFunctor=NoAction,
         class Attributes= ::boost::fusion::vector<>,
         class Flags = ::boost::mpl::vector0<>,
         class Defer = ::boost::mpl::vector0<>,
         class BASE = default_base_state>
struct entry_func_state :  public ::boost::msm::front::detail::state_base<BASE,Attributes>, 
                           euml_state_intern<entry_func_state<StateNameTag,ZoneIndex,EntryFunctor,ExitFunctor,Attributes,Flags,Defer,BASE> >
{
    entry_func_state(){}
    // grammar testing
    BOOST_MPL_ASSERT_NOT(( boost::is_same<EntryFunctor,invalid_type> ));
    BOOST_MPL_ASSERT_NOT(( boost::is_same<ExitFunctor,invalid_type> ));

    typedef StateNameTag state_name_tag;
    // tags
    typedef int                          pseudo_entry;
    enum {zone_index=ZoneIndex};
    typedef int explicit_entry_state;

    // flags
    typedef Flags       flag_list;
    typedef ::boost::mpl::vector0<> internal_flag_list;
    // deferred events
    typedef Defer       deferred_events;

    template <class Event,class FSM>
    void on_entry(Event const& evt,FSM& fsm)
    {
        EntryFunctor()(evt,fsm,*this);
    }
    template <class Event,class FSM>
    void on_exit(Event const& evt,FSM& fsm)
    {
        ExitFunctor()(evt,fsm,*this);
    }
};
// provides the typedefs and interface. Concrete states derive from it.
template<class StateNameTag,
         int ZoneIndex=-1,
         class EntryFunctor=NoAction, 
         class ExitFunctor=NoAction,
         class Attributes= ::boost::fusion::vector<>,
         class Flags = ::boost::mpl::vector0<>,
         class Defer = ::boost::mpl::vector0<>,
         class BASE = default_base_state>
struct explicit_entry_func_state :  public ::boost::msm::front::detail::state_base<BASE,Attributes>, 
                                    public ::boost::msm::front::explicit_entry<ZoneIndex>,
                                    euml_state_intern<explicit_entry_func_state<StateNameTag,
                                        ZoneIndex,EntryFunctor,ExitFunctor,Attributes,Flags,Defer,BASE> >
{
    explicit_entry_func_state(){}
    // grammar testing
    BOOST_MPL_ASSERT_NOT(( boost::is_same<EntryFunctor,invalid_type> ));
    BOOST_MPL_ASSERT_NOT(( boost::is_same<ExitFunctor,invalid_type> ));

    typedef StateNameTag state_name_tag;
    // flags
    typedef Flags       flag_list;
    typedef ::boost::mpl::vector0<> internal_flag_list;
    // deferred events
    typedef Defer       deferred_events;

    template <class Event,class FSM>
    void on_entry(Event const& evt,FSM& fsm)
    {
        EntryFunctor()(evt,fsm,*this);
    }
    template <class Event,class FSM>
    void on_exit(Event const& evt,FSM& fsm)
    {
        ExitFunctor()(evt,fsm,*this);
    }
};

// provides the typedefs and interface. Concrete states derive from it.
template<class StateNameTag,
         class Event,
         class EntryFunctor=NoAction, 
         class ExitFunctor=NoAction,
         class Attributes= ::boost::fusion::vector<>,
         class Flags = ::boost::mpl::vector0<>,
         class Defer = ::boost::mpl::vector0<>,
         class BASE = default_base_state>
struct exit_func_state :   public ::boost::msm::front::detail::state_base<BASE,Attributes>, 
                           euml_state_intern<exit_func_state<StateNameTag,Event,EntryFunctor,ExitFunctor,Attributes,Flags,Defer,BASE> >
{
    exit_func_state(){}
    // grammar testing
    BOOST_MPL_ASSERT_NOT(( boost::is_same<EntryFunctor,invalid_type> ));
    BOOST_MPL_ASSERT_NOT(( boost::is_same<ExitFunctor,invalid_type> ));

    typedef StateNameTag state_name_tag;
    // tags
    typedef Event       event;
    typedef BASE        Base;
    typedef int         pseudo_exit;

    // flags
    typedef Flags       flag_list;
    typedef ::boost::mpl::vector0<> internal_flag_list;
    // deferred events
    typedef Defer       deferred_events;

    template <class Evt,class FSM>
    void on_entry(Evt const& evt,FSM& fsm)
    {
        EntryFunctor()(evt,fsm,*this);
    }
    template <class Evt,class FSM>
    void on_exit(Evt const& evt,FSM& fsm)
    {
        ExitFunctor()(evt,fsm,*this);
    }
};

struct BuildActions;
struct BuildGuards;

struct BuildActionSequence
   : proto::or_<
        proto::when <
                    BuildActions,
                    ActionSequence_<make_vector_one_row<BuildActions(proto::_)>()>()
        >,
        proto::when <
                    proto::comma<BuildActions,BuildActions >,
                    ActionSequence_<boost::mpl::push_back<
                        make_vector_one_row<BuildActions(proto::_left)>(),
                        BuildActions(proto::_right)>()>()                
        >,
        proto::when <
                    proto::comma<BuildActionSequence,BuildActions >,
                    ActionSequence_<boost::mpl::push_back<
                        get_sequence<BuildActionSequence(proto::_left) >(),
                        BuildActions(proto::_right) >() >()                
        >
   >
{};

#ifdef BOOST_MSM_EUML_PHOENIX_SUPPORT
struct CustomPhoenixGrammar
    : proto::switch_<CustomPhoenixGrammar>
{
    template <typename Tag, typename Dummy = void>
    struct case_ 
        : proto::and_<
            proto::not_<BuildGuards> ,
            proto::not_<BuildActionSequence>,
            boost::phoenix::meta_grammar::case_<Tag>
        >
    {};
};
#endif

struct GuardGrammar
        : proto::or_<
#ifdef BOOST_MSM_EUML_PHOENIX_SUPPORT
            proto::when<
                CustomPhoenixGrammar ,
                proto::_
            >,
#endif
            proto::when<
                BuildGuards ,
                BuildGuards
            >
    >
 {};

struct ActionGrammar
        : proto::or_<
#ifdef BOOST_MSM_EUML_PHOENIX_SUPPORT
            proto::when<
                CustomPhoenixGrammar ,
                proto::_
            >,
#endif
            proto::when<
                BuildActionSequence ,
                BuildActionSequence
            >
    >
 {};

struct BuildActionsCases
{
    // The primary template matches nothing:
    template<typename Tag>
    struct case_
        : proto::not_<proto::_>
    {};
};

template<>
struct BuildActionsCases::case_<proto::tag::pre_inc>
    : proto::when<
            proto::pre_inc<BuildActions >,
            Pre_inc_< BuildActions(proto::_child)>()               
                >
{};
template<>
struct BuildActionsCases::case_<proto::tag::pre_dec>
    : proto::when<
            proto::pre_dec<BuildActions >,
            Pre_dec_< BuildActions(proto::_child)>()               
                >
{};
template<>
struct BuildActionsCases::case_<proto::tag::post_inc>
    : proto::when<
            proto::post_inc<BuildActions >,
            Post_inc_< BuildActions(proto::_child)>()               
                >
{};
template<>
struct BuildActionsCases::case_<proto::tag::post_dec>
    : proto::when<
            proto::post_dec<BuildActions >,
            Post_dec_< BuildActions(proto::_child)>()               
                >
{};
template<>
struct BuildActionsCases::case_<proto::tag::dereference>
    : proto::when<
            proto::dereference<BuildActions >,
            Deref_< BuildActions(proto::_child)>()               
                >
{};
template<>
struct BuildActionsCases::case_<proto::tag::plus>
    : proto::when<
            proto::plus<BuildActions,BuildActions >,
            Plus_<BuildActions(proto::_left),BuildActions(proto::_right)>()               
                >
{};
template<>
struct BuildActionsCases::case_<proto::tag::minus>
    : proto::when<
            proto::minus<BuildActions,BuildActions >,
            Minus_<BuildActions(proto::_left),BuildActions(proto::_right)>()               
                >
{};
template<>
struct BuildActionsCases::case_<proto::tag::multiplies>
    : proto::when<
            proto::multiplies<BuildActions,BuildActions >,
            Multiplies_<BuildActions(proto::_left),BuildActions(proto::_right)>()               
                >
{};
template<>
struct BuildActionsCases::case_<proto::tag::divides>
    : proto::when<
            proto::divides<BuildActions,BuildActions >,
            Divides_<BuildActions(proto::_left),BuildActions(proto::_right)>()               
                >
{};
template<>
struct BuildActionsCases::case_<proto::tag::modulus>
    : proto::when<
            proto::modulus<BuildActions,BuildActions >,
            Modulus_<BuildActions(proto::_left),BuildActions(proto::_right)>()               
                >
{};
template<>
struct BuildActionsCases::case_<proto::tag::bitwise_and>
    : proto::when<
            proto::bitwise_and<BuildActions,BuildActions >,
            Bitwise_And_<BuildActions(proto::_left),BuildActions(proto::_right)>()               
                >
{};
template<>
struct BuildActionsCases::case_<proto::tag::bitwise_or>
    : proto::when<
            proto::bitwise_or<BuildActions,BuildActions >,
            Bitwise_Or_<BuildActions(proto::_left),BuildActions(proto::_right)>()               
                >
{};
template<>
struct BuildActionsCases::case_<proto::tag::bitwise_xor>
    : proto::when<
            proto::bitwise_xor<BuildActions,BuildActions >,
            Bitwise_Xor_<BuildActions(proto::_left),BuildActions(proto::_right)>()               
                >
{};

template<>
struct BuildActionsCases::case_<proto::tag::plus_assign>
    : proto::when<
            proto::plus_assign<BuildActions,BuildActions >,
            Plus_Assign_< BuildActions(proto::_left),BuildActions(proto::_right)>()
                >
{};
template<>
struct BuildActionsCases::case_<proto::tag::minus_assign>
    : proto::when<
            proto::minus_assign<BuildActions,BuildActions >,
            Minus_Assign_<BuildActions(proto::_left),BuildActions(proto::_right)>()
                >
{};
template<>
struct BuildActionsCases::case_<proto::tag::multiplies_assign>
    : proto::when<
            proto::multiplies_assign<BuildActions,BuildActions >,
            Multiplies_Assign_< BuildActions(proto::_left),BuildActions(proto::_right)>()
                >
{};
template<>
struct BuildActionsCases::case_<proto::tag::divides_assign>
    : proto::when<
            proto::divides_assign<BuildActions,BuildActions >,
            Divides_Assign_< BuildActions(proto::_left),BuildActions(proto::_right)>()
                >
{};
template<>
struct BuildActionsCases::case_<proto::tag::modulus_assign>
    : proto::when<
            proto::modulus_assign<BuildActions,BuildActions >,
            Modulus_Assign_< BuildActions(proto::_left),BuildActions(proto::_right)>()
                >
{};
template<>
struct BuildActionsCases::case_<proto::tag::shift_left_assign>
    : proto::when<
            proto::shift_left_assign<BuildActions,BuildActions >,
            ShiftLeft_Assign_< BuildActions(proto::_left),BuildActions(proto::_right)>()
                >
{};
template<>
struct BuildActionsCases::case_<proto::tag::shift_right_assign>
    : proto::when<
            proto::shift_right_assign<BuildActions,BuildActions >,
            ShiftRight_Assign_< BuildActions(proto::_left),BuildActions(proto::_right)>()
                >
{};
template<>
struct BuildActionsCases::case_<proto::tag::shift_left>
    : proto::when<
            proto::shift_left<BuildActions,BuildActions >,
            ShiftLeft_< BuildActions(proto::_left),BuildActions(proto::_right)>()
                >
{};
template<>
struct BuildActionsCases::case_<proto::tag::shift_right>
    : proto::when<
            proto::shift_right<BuildActions,BuildActions >,
            ShiftRight_< BuildActions(proto::_left),BuildActions(proto::_right)>()
                >
{};
template<>
struct BuildActionsCases::case_<proto::tag::assign>
    : proto::when<
            proto::assign<BuildActions,BuildActions >,
            Assign_< BuildActions(proto::_left),BuildActions(proto::_right)>()
                >
{};
template<>
struct BuildActionsCases::case_<proto::tag::subscript>
    : proto::when<
            proto::subscript<BuildActions,BuildActions >,
            Subscript_< BuildActions(proto::_left),BuildActions(proto::_right)>()
                >
{};
template<>
struct BuildActionsCases::case_<proto::tag::unary_plus>
    : proto::when<
            proto::unary_plus<BuildActions >,
            Unary_Plus_< BuildActions(proto::_child)>()
                >
{};
template<>
struct BuildActionsCases::case_<proto::tag::negate>
    : proto::when<
            proto::negate<BuildActions >,
            Unary_Minus_< BuildActions(proto::_child)>()
                >
{};

template<>
struct BuildActionsCases::case_<proto::tag::function>
    : proto::or_<
            proto::when<
                proto::function<proto::terminal<if_tag>,BuildGuards,BuildActionSequence,BuildActionSequence >,
                If_Else_<BuildGuards(proto::_child_c<1>),
                         BuildActionSequence(proto::_child_c<2>),
                         BuildActionSequence(proto::_child_c<3>) >()
                    >,
            proto::when<
                proto::function<proto::terminal<if_then_tag>,BuildGuards,BuildActionSequence >,
                If_Then_<BuildGuards(proto::_child_c<1>),
                         BuildActionSequence(proto::_child_c<2>)>()
                    >,
            proto::when<
                    proto::function<proto::terminal<while_do_tag>,BuildGuards,BuildActionSequence >,
                    While_Do_<BuildGuards(proto::_child_c<1>),
                             BuildActionSequence(proto::_child_c<2>) >()
                    >,
            proto::when<
                    proto::function<proto::terminal<do_while_tag>,BuildGuards,BuildActionSequence >,
                    Do_While_<BuildGuards(proto::_child_c<1>),
                             BuildActionSequence(proto::_child_c<2>) >()
                    >,
            proto::when<
                proto::function<proto::terminal<for_loop_tag>,
                                BuildActionSequence,BuildGuards,BuildActionSequence,BuildActionSequence>,
                For_Loop_<BuildActionSequence(proto::_child_c<1>),
                         BuildGuards(proto::_child_c<2>),
                         BuildActionSequence(proto::_child_c<3>),
                         BuildActionSequence(proto::_child_c<4>) >()
                    >,
         proto::or_<
            proto::when<
                    proto::function<proto::terminal<proto::_>,BuildActions >,
                    get_fct<proto::_child_c<0>,BuildActions(proto::_child_c<1>) >()
                    >,
            proto::when<
                    proto::function<proto::terminal<proto::_> >,
                    get_fct<proto::_child_c<0> >()
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
                    proto::function<proto::terminal<proto::_>,BuildActions,BuildActions,BuildActions,BuildActions,BuildActions >,
                    get_fct<proto::_child_c<0>
                            ,BuildActions(proto::_child_c<1>),BuildActions(proto::_child_c<2>)
                            ,BuildActions(proto::_child_c<3>),BuildActions(proto::_child_c<4>)
                            ,BuildActions(proto::_child_c<5>) >()
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
    >
{};

template<>
struct BuildActionsCases::case_<proto::tag::terminal>
    : proto::or_<
        proto::when<
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
struct BuildActions
    : proto::switch_<BuildActionsCases>
{};

// attributes building
#define BOOST_MSM_EUML_DECLARE_ATTRIBUTE(attr_type,attr_name)                                                           \
struct attr_name ## _                                                                                                   \
    : proto::extends< proto::terminal< ::boost::msm::front::action_tag>::type, attr_name ## _, boost::msm::sm_domain>   \
    {typedef  attr_name ## _ action_name;                                                                               \
        typedef ::boost::fusion::pair<attr_name ## _,attr_type> attribute_type;                                         \
        attr_name ## _ (){}                                                                                             \
    };                                                                                                                  \
attr_name ## _ const attr_name = attr_name ## _();

struct make_attributes_tag
{
    typedef int attribute_tag;
};

template <class T>
struct get_attribute_type
{
    typedef typename T::attribute_type type;
};
template <class Seq>
struct transform_to_fusion_pair 
{
    typedef typename ::boost::mpl::fold<
        Seq,::boost::mpl::vector<>,
        ::boost::mpl::push_back< ::boost::mpl::placeholders::_1, 
                                 get_attribute_type< ::boost::mpl::placeholders::_2> >
        >::type type;
};

template<class X = proto::is_proto_expr>
struct attribute
{
   BOOST_PROTO_BASIC_EXTENDS(
       proto::terminal<make_attributes_tag>::type
     , attribute
     , boost::msm::sm_domain
   )
   typedef ::boost::fusion::pair<int,int> attribute_type;
};

attribute<> const attributes_ = {{{}}};
 attribute<> const no_attributes_ = {{{}}};

 struct BuildAttributesHelper
   : proto::make<
       ::boost::mpl::pop_front<
          proto::fold_tree<
              proto::_
            , ::boost::fusion::vector<>()
            , ::boost::mpl::push_back<proto::_state, 
                                      ::boost::mpl::if_< has_attribute_tag< proto::_value>,
                                                         proto::_value,
                                                         get_attribute_type<proto::_> > 
                >()
           >
       >
    >
 {};

struct BuildAttributes 
 : proto::make<
      ::boost::mpl::if_<
          has_attribute_tag< ::boost::mpl::deref< ::boost::mpl::prior< ::boost::mpl::end< BuildAttributesHelper > > > >,
          ::boost::fusion::result_of::as_map< ::boost::mpl::pop_back< BuildAttributesHelper > >,
          ::boost::fusion::result_of::as_map< BuildAttributesHelper > > 
   > 
{};

// helper to build a mpl::vector from a << list
 struct BuildMplVectorHelper
   : proto::make<
       ::boost::mpl::pop_front<
          proto::fold_tree<
              proto::_
            , ::boost::mpl::vector0<>()
            , ::boost::mpl::push_back<proto::_state, proto::_>()
           >
       >
    >
 {};

// flags building
struct BuildFlags 
 : proto::make<
    ::boost::mpl::remove_if<
        BuildMplVectorHelper,
        ::boost::mpl::not_< ::boost::is_same<get_euml_tag_type< ::boost::mpl::placeholders::_ >, flag_tag > > 
    >
 >
{};

struct control_configure_tag {};

// configuration building
struct make_configure_tag
{
    typedef int control_configure_tag;
};

template<class X = proto::is_proto_expr>
struct configure
{
   typedef not_euml_tag euml_tag_type;
   BOOST_PROTO_BASIC_EXTENDS(
       proto::terminal<make_configure_tag>::type
     , configure
     , boost::msm::sm_domain
   )
};

 configure<> const configure_ = {{{}}};
 configure<> const no_configure_ = {{{}}};

struct BuildConfigure 
 : proto::make<
    ::boost::mpl::remove_if<
        BuildMplVectorHelper,
        ::boost::mpl::not_< ::boost::is_same<get_euml_tag_type< ::boost::mpl::placeholders::_ >, config_tag > > 
    >
 >
{};

struct BuildDeferred 
 : proto::make<
    ::boost::mpl::remove_if<
        BuildMplVectorHelper,
        ::boost::mpl::not_< ::boost::is_same<get_euml_tag_type< ::boost::mpl::placeholders::_ >, event_tag > > 
    >
 >
{};

template<class X = proto::is_proto_expr>
struct define_init
{
   typedef int defer_create_tag;
   BOOST_PROTO_BASIC_EXTENDS(
       proto::terminal<state_tag>::type
     , define_init
     , boost::msm::sm_domain
   )
};

define_init<> const init_ = {{{}}};
struct BuildInit
   : proto::make<
       ::boost::mpl::pop_front<
          proto::fold_tree<
              proto::_
            , ::boost::mpl::vector0<>()
            , ::boost::mpl::push_back<proto::_state, proto::_>()
           >
       >
    >
 {};

template <class StateNameTag,class Expr1,class Expr2,class Attr,class Configure,class BASE>
inline
func_state<
StateNameTag,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
typename boost::result_of<BuildAttributes(Attr)>::type,
typename boost::result_of<BuildFlags(Configure)>::type,
typename boost::result_of<BuildDeferred(Configure)>::type,
BASE
>
build_state(Expr1 const& ,Expr2 const& , Attr const&, Configure const&, BASE )
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    typedef typename boost::result_of<BuildAttributes(Attr)>::type attributes_type;
    typedef typename boost::result_of<BuildFlags(Configure)>::type flags_type;
    typedef typename boost::result_of<BuildDeferred(Configure)>::type deferred_type;
    return func_state<StateNameTag,entry_action,exit_action,attributes_type,flags_type,deferred_type,BASE>();
}

template <class StateNameTag,class Expr1,class Expr2,class Attr,class Configure>
inline
func_state<
StateNameTag,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
typename boost::result_of<BuildAttributes(Attr)>::type,
typename boost::result_of<BuildFlags(Configure)>::type,
typename boost::result_of<BuildDeferred(Configure)>::type
>
build_state(Expr1 const& ,Expr2 const& ,Attr const&, Configure const&)
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    typedef typename boost::result_of<BuildAttributes(Attr)>::type attributes_type;
    typedef typename boost::result_of<BuildFlags(Configure)>::type flags_type;
    typedef typename boost::result_of<BuildDeferred(Configure)>::type deferred_type;
    return func_state<StateNameTag,entry_action,exit_action,attributes_type,flags_type,deferred_type>();
}

template <class StateNameTag,class Expr1,class Expr2,class Attr>
inline
func_state<
StateNameTag,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
typename boost::result_of<BuildAttributes(Attr)>::type
>
build_state(Expr1 const& ,Expr2 const& ,Attr const&)
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    typedef typename boost::result_of<BuildAttributes(Attr)>::type attributes_type;
    return func_state<StateNameTag,entry_action,exit_action,attributes_type>();
}

template <class StateNameTag,class Expr1,class Expr2>
inline
func_state<
StateNameTag,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type
>
build_state(Expr1 const& ,Expr2 const& )
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    return func_state<StateNameTag,entry_action,exit_action>();
}

template <class StateNameTag,class Expr1>
inline
func_state<
StateNameTag,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
NoAction
>
build_state(Expr1 const& )
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    return func_state<StateNameTag,entry_action,NoAction>();
}
template<class StateNameTag>
inline
func_state<
StateNameTag,
NoAction,
NoAction
>
build_state()
{
    return func_state<StateNameTag,NoAction,NoAction>();
}

// provides the typedefs and interface. Concrete states derive from it.
template<class StateNameTag,
         class STT, 
         class Init,
         class EntryFunctor=NoAction, 
         class ExitFunctor=NoAction,
         class Attributes= ::boost::fusion::vector<>,
         class Flags = ::boost::mpl::vector0<>,
         class Defer = ::boost::mpl::vector0<>,
         class Configuration = ::boost::mpl::vector0<>,
         class NoTransitionFunctor = NoAction,
         class OnExceptionFunctor = NoAction,
         class BASE = ::boost::msm::front::default_base_state>
struct func_state_machine :  public ::boost::msm::front::detail::state_base<BASE,Attributes>, 
                             euml_state_intern<func_state_machine<StateNameTag,STT,Init,EntryFunctor,ExitFunctor,Attributes,Flags,
                                                        Defer,NoTransitionFunctor,OnExceptionFunctor,BASE> >
{
    func_state_machine(){}
    // grammar testing
    BOOST_MPL_ASSERT_NOT(( boost::is_same<EntryFunctor,invalid_type> ));
    BOOST_MPL_ASSERT_NOT(( boost::is_same<ExitFunctor,invalid_type> ));
    BOOST_MPL_ASSERT_NOT(( boost::is_same<NoTransitionFunctor,invalid_type> ));
    BOOST_MPL_ASSERT_NOT(( boost::is_same<OnExceptionFunctor,invalid_type> ));
    BOOST_MPL_ASSERT_NOT(( boost::is_same<STT,invalid_type> ));

    // flags
    typedef StateNameTag state_name_tag;
    typedef Flags                        flag_list;
    typedef ::boost::mpl::vector0<>      internal_flag_list;
    // deferred events
    typedef Defer                        deferred_events;
    // customization (message queue, exceptions)
    typedef Configuration                configuration;


    typedef BASE                         BaseAllStates;
    typedef STT                          transition_table;
    // the initial state of the player SM. Must be defined
    typedef Init                         initial_state;

    template <class Event,class FSM>
    void on_entry(Event const& evt,FSM& fsm)
    {
        EntryFunctor()(evt,fsm,*this);
    }
    template <class Event,class FSM>
    void on_exit(Event const& evt,FSM& fsm)
    {
        ExitFunctor()(evt,fsm,*this);
    }
protected:
    // Default no-transition handler. Can be replaced in the Derived SM class.
    template <class FSM,class Event>
    void no_transition(Event const& evt,FSM& fsm,int state)
    {
        NoTransitionFunctor()(evt,fsm,state);
    }
    // default exception handler. Can be replaced in the Derived SM class.
    template <class FSM,class Event>
    void exception_caught (Event const& evt,FSM& fsm,std::exception& e)
    {
        OnExceptionFunctor()(evt,fsm,e);
    }
};

template <class StateNameTag,class STT,class Init>
inline
func_state_machine<
StateNameTag,
STT,
typename boost::result_of<BuildInit(Init)>::type
>
build_sm(STT ,Init)
{
    typedef typename boost::result_of<BuildInit(Init)>::type init_type;
    return func_state_machine<StateNameTag,STT,init_type>();
}

template <class StateNameTag,class STT,class Init,class Expr1>
inline
func_state_machine<
StateNameTag,
STT,
typename boost::result_of<BuildInit(Init)>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type
>
build_sm(STT ,Init , Expr1 const&)
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<BuildInit(Init)>::type init_type;
    return func_state_machine<StateNameTag,STT,init_type,entry_action>();
}

template <class StateNameTag,class STT,class Init,class Expr1,class Expr2>
inline
func_state_machine<
StateNameTag,
STT,
typename boost::result_of<BuildInit(Init)>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type
>
build_sm(STT ,Init , Expr1 const& ,Expr2 const& )
{
    typedef typename boost::result_of<BuildInit(Init)>::type init_type;
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    return func_state_machine<StateNameTag,STT,init_type,entry_action,exit_action>();
}

template <class StateNameTag,class STT,class Init,class Expr1,class Expr2,class Attr>
inline
func_state_machine<
StateNameTag,
STT,
typename boost::result_of<BuildInit(Init)>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
typename boost::result_of<BuildAttributes(Attr)>::type
>
build_sm(STT ,Init , Expr1 const& ,Expr2 const& ,Attr const&)
{
    typedef typename boost::result_of<BuildInit(Init)>::type init_type;
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    typedef typename boost::result_of<BuildAttributes(Attr)>::type attributes_type;
    return func_state_machine<StateNameTag,STT,init_type,entry_action,exit_action,attributes_type>();
}

template <class StateNameTag,class STT,class Init,class Expr1,class Expr2,class Attr,class Configure>
inline
func_state_machine<
StateNameTag,
STT,
typename boost::result_of<BuildInit(Init)>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
typename boost::result_of<BuildAttributes(Attr)>::type,
typename boost::result_of<BuildFlags(Configure)>::type,
typename boost::result_of<BuildDeferred(Configure)>::type,
typename boost::result_of<BuildConfigure(Configure)>::type
>
build_sm(STT ,Init , Expr1 const& ,Expr2 const& , Attr const&, Configure const& )
{
    typedef typename boost::result_of<BuildInit(Init)>::type init_type;
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    typedef typename boost::result_of<BuildAttributes(Attr)>::type attributes_type;
    typedef typename boost::result_of<BuildFlags(Configure)>::type flags_type;
    typedef typename boost::result_of<BuildDeferred(Configure)>::type deferred_type;
    typedef typename boost::result_of<BuildConfigure(Configure)>::type config_type;
    return func_state_machine<StateNameTag,STT,init_type,entry_action,exit_action,attributes_type,flags_type,
                              deferred_type,config_type>();

}

template <class StateNameTag,class STT,class Init,class Expr1,class Expr2,class Attr,class Configure,class Expr3>
inline
func_state_machine<
StateNameTag,
STT,
typename boost::result_of<BuildInit(Init)>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
typename boost::result_of<BuildAttributes(Attr)>::type,
typename boost::result_of<BuildFlags(Configure)>::type,
typename boost::result_of<BuildDeferred(Configure)>::type,
typename boost::result_of<BuildConfigure(Configure)>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr3,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr3)>,
    make_invalid_type>::type
>
build_sm(STT ,Init , Expr1 const& ,Expr2 const& ,Attr const&, Configure const&, Expr3 const& )
{
    typedef typename boost::result_of<BuildInit(Init)>::type init_type;
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    typedef typename boost::result_of<ActionGrammar(Expr3)>::type no_transition_action;
    typedef typename boost::result_of<BuildAttributes(Attr)>::type attributes_type;
    typedef typename boost::result_of<BuildFlags(Configure)>::type flags_type;
    typedef typename boost::result_of<BuildDeferred(Configure)>::type deferred_type;
    typedef typename boost::result_of<BuildConfigure(Configure)>::type config_type;
    return func_state_machine<StateNameTag,STT,init_type,entry_action,exit_action,attributes_type,flags_type,deferred_type,
                              config_type,no_transition_action>();
}

template <class StateNameTag,class STT,class Init,class Expr1,class Expr2,class Attr,class Configure,class Expr3,class Expr4>
inline
func_state_machine<
StateNameTag,
STT,
typename boost::result_of<BuildInit(Init)>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
typename boost::result_of<BuildAttributes(Attr)>::type,
typename boost::result_of<BuildFlags(Configure)>::type,
typename boost::result_of<BuildDeferred(Configure)>::type,
typename boost::result_of<BuildConfigure(Configure)>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr3,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr3)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr4,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr4)>,
    make_invalid_type>::type
>
build_sm(STT ,Init , Expr1 const& ,Expr2 const& , Attr const&, Configure const&, Expr3 const&, Expr4 const& )
{
    typedef typename boost::result_of<BuildInit(Init)>::type init_type;
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    typedef typename boost::result_of<BuildFlags(Configure)>::type flags_type;
    typedef typename boost::result_of<BuildDeferred(Configure)>::type deferred_type;
    typedef typename boost::result_of<BuildConfigure(Configure)>::type config_type;
    typedef typename boost::result_of<ActionGrammar(Expr3)>::type no_transition_action;
    typedef typename boost::result_of<ActionGrammar(Expr4)>::type on_exception_action;
    typedef typename boost::result_of<BuildAttributes(Attr)>::type attributes_type;
    return func_state_machine<StateNameTag,STT,init_type,entry_action,exit_action,attributes_type,flags_type,deferred_type,
                              config_type,no_transition_action,on_exception_action>();
}

template <class StateNameTag,class STT,class Init,class Expr1,class Expr2,class Attr,class Configure,class Expr3,class Expr4,class BASE>
inline
func_state_machine<
StateNameTag,
STT,
typename boost::result_of<BuildInit(Init)>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
typename boost::result_of<BuildAttributes(Attr)>::type,
typename boost::result_of<BuildFlags(Configure)>::type,
typename boost::result_of<BuildDeferred(Configure)>::type,
typename boost::result_of<BuildConfigure(Configure)>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr3,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr3)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr4,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr4)>,
    make_invalid_type>::type,
BASE
>
build_sm(STT ,Init , Expr1 const& ,Expr2 const& ,Attr const& , Configure const&, Expr3 const&, Expr4 const& , BASE )
{
    typedef typename boost::result_of<BuildInit(Init)>::type init_type;
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    typedef typename boost::result_of<BuildFlags(Configure)>::type flags_type;
    typedef typename boost::result_of<BuildDeferred(Configure)>::type deferred_type;
    typedef typename boost::result_of<BuildConfigure(Configure)>::type config_type;
    typedef typename boost::result_of<ActionGrammar(Expr3)>::type no_transition_action;
    typedef typename boost::result_of<ActionGrammar(Expr4)>::type on_exception_action;
    typedef typename boost::result_of<BuildAttributes(Attr)>::type attributes_type;
    return func_state_machine<StateNameTag,STT,init_type,entry_action,exit_action,attributes_type,flags_type,deferred_type,
                              config_type,no_transition_action,on_exception_action,BASE>();
}

template <class Expr>
inline
::boost::msm::front::detail::inherit_attributes<typename boost::result_of<BuildAttributes(Expr)>::type>
build_attributes (Expr const&)
{
    return ::boost::msm::front::detail::inherit_attributes<typename boost::result_of<BuildAttributes(Expr)>::type> ();
}

template <class StateNameTag,class Expr1,class Expr2,class Attr,class Configure,class BASE>
inline
func_state<
StateNameTag,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
typename boost::result_of<BuildAttributes(Attr)>::type,
typename ::boost::mpl::push_back< typename boost::result_of<BuildFlags(Configure)>::type,
                                  ::boost::msm::TerminateFlag>::type,
typename boost::result_of<BuildDeferred(Configure)>::type,
BASE
>
build_terminate_state(Expr1 const& ,Expr2 const& , Attr const&, Configure const&, BASE )
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    typedef typename ::boost::mpl::push_back<
        typename boost::result_of<BuildFlags(Configure)>::type,
        ::boost::msm::TerminateFlag >::type flags_type;
    typedef typename boost::result_of<BuildDeferred(Configure)>::type deferred_type;
    typedef typename boost::result_of<BuildAttributes(Attr)>::type attributes_type;
    return func_state<StateNameTag,entry_action,exit_action,attributes_type,flags_type,deferred_type,BASE>();
}

template <class StateNameTag,class Expr1,class Expr2,class Attr,class Configure>
inline
func_state<
StateNameTag,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
typename boost::result_of<BuildAttributes(Attr)>::type,
typename ::boost::mpl::push_back< typename boost::result_of<BuildFlags(Configure)>::type,
                                  ::boost::msm::TerminateFlag>::type,
typename boost::result_of<BuildDeferred(Configure)>::type
>
build_terminate_state(Expr1 const& ,Expr2 const& ,Attr const&, Configure const&)
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    typedef typename ::boost::mpl::push_back<
        typename boost::result_of<BuildFlags(Configure)>::type,
        ::boost::msm::TerminateFlag >::type flags_type;
    typedef typename boost::result_of<BuildDeferred(Configure)>::type deferred_type;
    typedef typename boost::result_of<BuildAttributes(Attr)>::type attributes_type;

    return func_state<StateNameTag,entry_action,exit_action,attributes_type,flags_type,deferred_type>();
}

template <class StateNameTag,class Expr1,class Expr2,class Attr>
inline
func_state<
StateNameTag,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
typename boost::result_of<BuildAttributes(Attr)>::type,
::boost::mpl::vector<boost::msm::TerminateFlag> 
>
build_terminate_state(Expr1 const& ,Expr2 const& ,Attr const&)
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    typedef typename boost::result_of<BuildAttributes(Attr)>::type attributes_type;
    return func_state<StateNameTag,entry_action,exit_action,attributes_type, ::boost::mpl::vector< ::boost::msm::TerminateFlag> >();
}

template <class StateNameTag,class Expr1,class Expr2>
inline
func_state<
StateNameTag,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
::boost::fusion::vector<>,
::boost::mpl::vector<boost::msm::TerminateFlag> 
>
build_terminate_state(Expr1 const& ,Expr2 const& )
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    return func_state<StateNameTag,entry_action,exit_action,
                      ::boost::fusion::vector<>, ::boost::mpl::vector< ::boost::msm::TerminateFlag> >();
}

template <class StateNameTag,class Expr1>
inline
func_state<
StateNameTag,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
NoAction,
::boost::fusion::vector<>,
::boost::mpl::vector<boost::msm::TerminateFlag> 
>
build_terminate_state(Expr1 const& )
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    return func_state<StateNameTag,entry_action,NoAction,::boost::fusion::vector<>,::boost::mpl::vector<boost::msm::TerminateFlag> >();
}
template<class StateNameTag>
inline
func_state<
StateNameTag,
NoAction,
NoAction,
::boost::fusion::vector<>,
::boost::mpl::vector<boost::msm::TerminateFlag> 
>
build_terminate_state()
{
    return func_state<StateNameTag,NoAction,NoAction,::boost::fusion::vector<>,::boost::mpl::vector<boost::msm::TerminateFlag> >();
}

template <class StateNameTag,class Expr1,class Expr2,class Attr,class Configure,class BASE,class EndInterruptEvent>
inline
func_state<
StateNameTag,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
typename boost::result_of<BuildAttributes(Attr)>::type,
typename ::boost::mpl::push_back<
    typename ::boost::mpl::push_back< typename boost::result_of<BuildFlags(Configure)>::type,
                                      ::boost::msm::InterruptedFlag>::type,
    boost::msm::EndInterruptFlag<EndInterruptEvent>
    >::type,
typename boost::result_of<BuildDeferred(Configure)>::type,
BASE
>
build_interrupt_state(EndInterruptEvent const&,Expr1 const& ,Expr2 const& , Attr const&, Configure const&, BASE )
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    typedef typename boost::result_of<BuildAttributes(Attr)>::type attributes_type;
    typedef typename ::boost::mpl::push_back<
                typename ::boost::mpl::push_back< 
                    typename boost::result_of<BuildFlags(Configure)>::type,
                    ::boost::msm::InterruptedFlag>::type,
                boost::msm::EndInterruptFlag<EndInterruptEvent>
    >::type flags_type;
    typedef typename boost::result_of<BuildDeferred(Configure)>::type deferred_type;
    return func_state<StateNameTag,entry_action,exit_action,attributes_type,flags_type,deferred_type,BASE>();
}

template <class StateNameTag,class Expr1,class Expr2,class Attr,class Configure,class EndInterruptEvent>
inline
func_state<
StateNameTag,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
typename boost::result_of<BuildAttributes(Attr)>::type,
typename ::boost::mpl::push_back<
    typename ::boost::mpl::push_back< typename boost::result_of<BuildFlags(Configure)>::type,
                                      ::boost::msm::InterruptedFlag>::type,
    boost::msm::EndInterruptFlag<EndInterruptEvent>
    >::type,
typename boost::result_of<BuildDeferred(Configure)>::type
>
build_interrupt_state(EndInterruptEvent const&,Expr1 const& ,Expr2 const& ,Attr const&, Configure const&)
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    typedef typename boost::result_of<BuildAttributes(Attr)>::type attributes_type;

    typedef typename ::boost::mpl::push_back<
                typename ::boost::mpl::push_back< 
                    typename boost::result_of<BuildFlags(Configure)>::type,
                    ::boost::msm::InterruptedFlag>::type,
                boost::msm::EndInterruptFlag<EndInterruptEvent>
    >::type flags_type;
    typedef typename boost::result_of<BuildDeferred(Configure)>::type deferred_type;

    return func_state<StateNameTag,entry_action,exit_action,attributes_type,flags_type,deferred_type>();
}

template <class StateNameTag,class Expr1,class Expr2,class Attr,class EndInterruptEvent>
inline
func_state<
StateNameTag,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
typename boost::result_of<BuildAttributes(Attr)>::type,
::boost::mpl::vector<boost::msm::InterruptedFlag, boost::msm::EndInterruptFlag<EndInterruptEvent> > 
>
build_interrupt_state(EndInterruptEvent const&,Expr1 const& ,Expr2 const& ,Attr const&)
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    typedef typename boost::result_of<BuildAttributes(Attr)>::type attributes_type;
    return func_state<StateNameTag,entry_action,exit_action,attributes_type, 
                     ::boost::mpl::vector< boost::msm::InterruptedFlag, boost::msm::EndInterruptFlag<EndInterruptEvent> > >();
}

template <class StateNameTag,class Expr1,class Expr2,class EndInterruptEvent>
inline
func_state<
StateNameTag,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
::boost::fusion::vector<>,
::boost::mpl::vector<boost::msm::InterruptedFlag, boost::msm::EndInterruptFlag<EndInterruptEvent> > 
>
build_interrupt_state(EndInterruptEvent const&,Expr1 const& ,Expr2 const& )
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    return func_state<StateNameTag,entry_action,exit_action,
                      ::boost::fusion::vector<>, 
                      ::boost::mpl::vector< boost::msm::InterruptedFlag, boost::msm::EndInterruptFlag<EndInterruptEvent> > >();
}

template <class StateNameTag,class Expr1,class EndInterruptEvent>
inline
func_state<
StateNameTag,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
NoAction,
::boost::fusion::vector<>,
::boost::mpl::vector<boost::msm::InterruptedFlag, boost::msm::EndInterruptFlag<EndInterruptEvent> > 
>
build_interrupt_state(EndInterruptEvent const&, Expr1 const&)
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    return func_state<StateNameTag,entry_action,NoAction, ::boost::fusion::vector<>,
                     ::boost::mpl::vector<boost::msm::InterruptedFlag, boost::msm::EndInterruptFlag<EndInterruptEvent> > >();
}

template <class StateNameTag,class EndInterruptEvent>
inline
func_state<
StateNameTag,
NoAction,
NoAction,
::boost::fusion::vector<>,
::boost::mpl::vector<boost::msm::InterruptedFlag, boost::msm::EndInterruptFlag<EndInterruptEvent> > 
>
build_interrupt_state(EndInterruptEvent const&)
{
    return func_state<StateNameTag,NoAction,NoAction, ::boost::fusion::vector<>,
                      ::boost::mpl::vector<boost::msm::InterruptedFlag, boost::msm::EndInterruptFlag<EndInterruptEvent> > >();
}

template <class StateNameTag,int ZoneIndex,class Expr1,class Expr2,class Attr,class Configure,class BASE>
inline
entry_func_state<
StateNameTag,
ZoneIndex,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
typename boost::result_of<BuildAttributes(Attr)>::type,
typename boost::result_of<BuildFlags(Configure)>::type,
typename boost::result_of<BuildDeferred(Configure)>::type,
BASE
>
build_entry_state(Expr1 const& ,Expr2 const& , Attr const&, Configure const&, BASE )
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    typedef typename boost::result_of<BuildFlags(Configure)>::type flags_type;
    typedef typename boost::result_of<BuildDeferred(Configure)>::type deferred_type;
    typedef typename boost::result_of<BuildAttributes(Attr)>::type attributes_type;
    return entry_func_state<StateNameTag,ZoneIndex,entry_action,exit_action,attributes_type,flags_type,deferred_type,BASE>();
}

template <class StateNameTag,int ZoneIndex,class Expr1,class Expr2,class Attr,class Configure>
inline
entry_func_state<
StateNameTag,
ZoneIndex,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
typename boost::result_of<BuildAttributes(Attr)>::type,
typename boost::result_of<BuildFlags(Configure)>::type,
typename boost::result_of<BuildDeferred(Configure)>::type
>
build_entry_state(Expr1 const& ,Expr2 const& ,Attr const&, Configure const&)
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    typedef typename boost::result_of<BuildFlags(Configure)>::type flags_type;
    typedef typename boost::result_of<BuildDeferred(Configure)>::type deferred_type;
    typedef typename boost::result_of<BuildAttributes(Attr)>::type attributes_type;
    return entry_func_state<StateNameTag,ZoneIndex,entry_action,exit_action,attributes_type,flags_type,deferred_type>();
}

template <class StateNameTag,int ZoneIndex,class Expr1,class Expr2,class Attr>
inline
entry_func_state<
StateNameTag,
ZoneIndex,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
typename boost::result_of<BuildAttributes(Attr)>::type
>
build_entry_state(Expr1 const& ,Expr2 const& ,Attr const&)
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    typedef typename boost::result_of<BuildAttributes(Attr)>::type attributes_type;
    return entry_func_state<StateNameTag,ZoneIndex,entry_action,exit_action,attributes_type>();
}

template <class StateNameTag,int ZoneIndex,class Expr1,class Expr2>
inline
entry_func_state<
StateNameTag,
ZoneIndex,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type
>
build_entry_state(Expr1 const& ,Expr2 const& )
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    return entry_func_state<StateNameTag,ZoneIndex,entry_action,exit_action>();
}

template <class StateNameTag,int ZoneIndex,class Expr1>
inline
entry_func_state<
StateNameTag,
ZoneIndex,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
NoAction
>
build_entry_state(Expr1 const& )
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    return entry_func_state<StateNameTag,ZoneIndex,entry_action,NoAction>();
}

template <class StateNameTag,int ZoneIndex>
inline
entry_func_state<
StateNameTag,
ZoneIndex,
NoAction,
NoAction
>
build_entry_state()
{
    return entry_func_state<StateNameTag,ZoneIndex,NoAction,NoAction>();
}

template <class StateNameTag,class Event,class Expr1,class Expr2,class Attr,class Configure,class BASE>
inline
exit_func_state<
StateNameTag,
Event,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
typename boost::result_of<BuildAttributes(Attr)>::type,
typename boost::result_of<BuildFlags(Configure)>::type,
typename boost::result_of<BuildDeferred(Configure)>::type,
BASE
>
build_exit_state(Event const&,Expr1 const& ,Expr2 const& , Attr const&, Configure const&, BASE )
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    typedef typename boost::result_of<BuildFlags(Configure)>::type flags_type;
    typedef typename boost::result_of<BuildDeferred(Configure)>::type deferred_type;
    typedef typename boost::result_of<BuildAttributes(Attr)>::type attributes_type;
    return exit_func_state<StateNameTag,Event,entry_action,exit_action,attributes_type,flags_type,deferred_type,BASE>();
}

template <class StateNameTag,class Event,class Expr1,class Expr2,class Attr,class Configure>
inline
exit_func_state<
StateNameTag,
Event,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
typename boost::result_of<BuildAttributes(Attr)>::type,
typename boost::result_of<BuildFlags(Configure)>::type,
typename boost::result_of<BuildDeferred(Configure)>::type
>
build_exit_state(Event const&,Expr1 const& ,Expr2 const& ,Attr const&, Configure const&)
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    typedef typename boost::result_of<BuildFlags(Configure)>::type flags_type;
    typedef typename boost::result_of<BuildDeferred(Configure)>::type deferred_type;
    typedef typename boost::result_of<BuildAttributes(Attr)>::type attributes_type;
    return exit_func_state<StateNameTag,Event,entry_action,exit_action,attributes_type,flags_type,deferred_type>();
}

template <class StateNameTag,class Event,class Expr1,class Expr2,class Attr>
inline
exit_func_state<
StateNameTag,
Event,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
typename boost::result_of<BuildAttributes(Attr)>::type
>
build_exit_state(Event const&,Expr1 const& ,Expr2 const& ,Attr const&)
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    typedef typename boost::result_of<BuildAttributes(Attr)>::type attributes_type;
    return exit_func_state<StateNameTag,Event,entry_action,exit_action,attributes_type>();
}

template <class StateNameTag,class Event,class Expr1,class Expr2>
inline
exit_func_state<
StateNameTag,
Event,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type
>
build_exit_state(Event const&,Expr1 const& ,Expr2 const& )
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    return exit_func_state<StateNameTag,Event,entry_action,exit_action>();
}

template <class StateNameTag,class Event,class Expr1>
inline
exit_func_state<
StateNameTag,
Event,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
NoAction
>
build_exit_state(Event const&, Expr1 const& )
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    return exit_func_state<StateNameTag,Event,entry_action,NoAction>();
}

template <class StateNameTag,class Event>
inline
exit_func_state<
StateNameTag,
Event,
NoAction,
NoAction
>
build_exit_state(Event const&)
{
    return exit_func_state<StateNameTag,Event,NoAction,NoAction>();
}

template <class StateNameTag,int ZoneIndex,class Expr1,class Expr2,class Attr,class Configure,class BASE>
inline
explicit_entry_func_state<
StateNameTag,
ZoneIndex,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
typename boost::result_of<BuildAttributes(Attr)>::type,
typename boost::result_of<BuildFlags(Configure)>::type,
typename boost::result_of<BuildDeferred(Configure)>::type,
BASE
>
build_explicit_entry_state(Expr1 const& ,Expr2 const& , Attr const&, Configure const&, BASE )
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    typedef typename boost::result_of<BuildFlags(Configure)>::type flags_type;
    typedef typename boost::result_of<BuildDeferred(Configure)>::type deferred_type;
    typedef typename boost::result_of<BuildAttributes(Attr)>::type attributes_type;
    return explicit_entry_func_state<StateNameTag,ZoneIndex,entry_action,exit_action,attributes_type,flags_type,deferred_type,BASE>();
}

template <class StateNameTag,int ZoneIndex,class Expr1,class Expr2,class Attr,class Configure>
inline
explicit_entry_func_state<
StateNameTag,
ZoneIndex,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
typename boost::result_of<BuildAttributes(Attr)>::type,
typename boost::result_of<BuildFlags(Configure)>::type,
typename boost::result_of<BuildDeferred(Configure)>::type
>
build_explicit_entry_state(Expr1 const& ,Expr2 const& ,Attr const&, Configure const&)
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    typedef typename boost::result_of<BuildFlags(Configure)>::type flags_type;
    typedef typename boost::result_of<BuildDeferred(Configure)>::type deferred_type;
    typedef typename boost::result_of<BuildAttributes(Attr)>::type attributes_type;
    return explicit_entry_func_state<StateNameTag,ZoneIndex,entry_action,exit_action,attributes_type,flags_type,deferred_type>();
}

template <class StateNameTag,int ZoneIndex,class Expr1,class Expr2,class Attr>
inline
explicit_entry_func_state<
StateNameTag,
ZoneIndex,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type,
typename boost::result_of<BuildAttributes(Attr)>::type
>
build_explicit_entry_state(Expr1 const& ,Expr2 const& ,Attr const&)
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    typedef typename boost::result_of<BuildAttributes(Attr)>::type attributes_type;
    return explicit_entry_func_state<StateNameTag,ZoneIndex,entry_action,exit_action,attributes_type>();
}

template <class StateNameTag,int ZoneIndex,class Expr1,class Expr2>
inline
explicit_entry_func_state<
StateNameTag,
ZoneIndex,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr2,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr2)>,
    make_invalid_type>::type
>
build_explicit_entry_state(Expr1 const& ,Expr2 const& )
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    typedef typename boost::result_of<ActionGrammar(Expr2)>::type exit_action;
    return explicit_entry_func_state<StateNameTag,ZoneIndex,entry_action,exit_action>();
}

template <class StateNameTag,int ZoneIndex,class Expr1>
inline
explicit_entry_func_state<
StateNameTag,
ZoneIndex,
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr1,ActionGrammar>::type,
    boost::result_of<ActionGrammar(Expr1)>,
    make_invalid_type>::type,
NoAction
>
build_explicit_entry_state(Expr1 const& )
{
    typedef typename boost::result_of<ActionGrammar(Expr1)>::type entry_action;
    return explicit_entry_func_state<StateNameTag,ZoneIndex,entry_action,NoAction>();
}

template <class StateNameTag,int ZoneIndex>
inline
explicit_entry_func_state<
StateNameTag,
ZoneIndex,
NoAction,
NoAction
>
build_explicit_entry_state()
{
    return explicit_entry_func_state<StateNameTag,ZoneIndex,NoAction,NoAction>();
}



}}}}

#endif //BOOST_MSM_FRONT_EUML_STATE_GRAMMAR_H


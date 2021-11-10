//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision$
//
//  Description : named function parameters library
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_NAMED_PARAM
#define BOOST_TEST_UTILS_NAMED_PARAM

// Boost
#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>

// Boost.Test
#include <boost/test/utils/rtti.hpp>
#include <boost/test/utils/assign_op.hpp>

#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/remove_cv.hpp>

#include <boost/test/detail/throw_exception.hpp>

// Boost
#include <boost/mpl/if.hpp>
#include <boost/mpl/or.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/bool.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace nfp { // named function parameters

// ************************************************************************** //
// **************             forward declarations             ************** //
// ************************************************************************** //

template<typename unique_id, bool required>                     struct keyword;
template<typename T, typename unique_id, bool required = false> struct typed_keyword;

template<typename T, typename unique_id, typename RefType=T&>   struct named_parameter;
template<typename NP1,typename NP2>                             struct named_parameter_combine;

// ************************************************************************** //
// **************              is_named_param_pack             ************** //
// ************************************************************************** //

/// is_named_param_pack<T>::value is true if T is parameters pack

template<typename T>
struct is_named_param_pack : public mpl::false_ {};

template<typename T, typename unique_id, typename RefType>
struct is_named_param_pack<named_parameter<T,unique_id,RefType> > : public mpl::true_ {};

template<typename NP, typename Rest>
struct is_named_param_pack<named_parameter_combine<NP,Rest> > : public mpl::true_ {};

// ************************************************************************** //
// **************                  param_type                  ************** //
// ************************************************************************** //

/// param_type<Params,Keyword,Default>::type is the type of the parameter
/// corresponding to the Keyword (if parameter is present) or Default

template<typename NP, typename Keyword, typename DefaultType=void>
struct param_type
: mpl::if_<typename is_same<typename NP::id,typename Keyword::id>::type,
           typename remove_cv<typename NP::data_type>::type,
           DefaultType> {};

template<typename NP, typename Rest, typename Keyword, typename DefaultType>
struct param_type<named_parameter_combine<NP,Rest>,Keyword,DefaultType>
: mpl::if_<typename is_same<typename NP::id,typename Keyword::id>::type,
           typename remove_cv<typename NP::data_type>::type,
           typename param_type<Rest,Keyword,DefaultType>::type> {};

// ************************************************************************** //
// **************                  has_param                   ************** //
// ************************************************************************** //

/// has_param<Params,Keyword>::value is true if Params has parameter corresponding
/// to the Keyword

template<typename NP, typename Keyword>
struct has_param : is_same<typename NP::id,typename Keyword::id> {};

template<typename NP, typename Rest, typename Keyword>
struct has_param<named_parameter_combine<NP,Rest>,Keyword>
: mpl::or_<typename is_same<typename NP::id,typename Keyword::id>::type,
           typename has_param<Rest,Keyword>::type> {};

// ************************************************************************** //
// **************          access_to_invalid_parameter         ************** //
// ************************************************************************** //

namespace nfp_detail {

struct access_to_invalid_parameter {};

//____________________________________________________________________________//

inline void
report_access_to_invalid_parameter( bool v )
{
    BOOST_TEST_I_ASSRT( !v, access_to_invalid_parameter() );
}

} // namespace nfp_detail

// ************************************************************************** //
// **************                      nil                     ************** //
// ************************************************************************** //

struct nil {
    template<typename T>
#if defined(__GNUC__) || defined(__HP_aCC) || defined(__EDG__) || defined(__SUNPRO_CC) || defined(BOOST_EMBTC)
    operator T() const
#else
    operator T const&() const
#endif
    { nfp_detail::report_access_to_invalid_parameter(true); static T* v = 0; return *v; }

    template<typename T>
    T any_cast() const
    { nfp_detail::report_access_to_invalid_parameter(true); static typename remove_reference<T>::type* v = 0; return *v; }

    template<typename Arg1>
    nil operator()( Arg1 const& )
    { nfp_detail::report_access_to_invalid_parameter(true); return nil(); }

    template<typename Arg1,typename Arg2>
    nil operator()( Arg1 const&, Arg2 const& )
    { nfp_detail::report_access_to_invalid_parameter(true); return nil(); }

    template<typename Arg1,typename Arg2,typename Arg3>
    nil operator()( Arg1 const&, Arg2 const&, Arg3 const& )
    { nfp_detail::report_access_to_invalid_parameter(true); return nil(); }

    // Visitation support
    template<typename Visitor>
    void            apply_to( Visitor& /*v*/ ) const {}

    static nil&     inst() { static nil s_inst; return s_inst; }
private:
    nil() {}
};

// ************************************************************************** //
// **************             named_parameter_base             ************** //
// ************************************************************************** //

namespace nfp_detail {

template<typename Derived>
struct named_parameter_base {
    template<typename NP>
    named_parameter_combine<NP,Derived>
    operator,( NP const& np ) const { return named_parameter_combine<NP,Derived>( np, *static_cast<Derived const*>(this) ); }
};

} // namespace nfp_detail

// ************************************************************************** //
// **************            named_parameter_combine           ************** //
// ************************************************************************** //

template<typename NP, typename Rest = nil>
struct named_parameter_combine
: Rest
, nfp_detail::named_parameter_base<named_parameter_combine<NP,Rest> > {
    typedef typename NP::ref_type  res_type;
    typedef named_parameter_combine<NP,Rest> self_type;

    // Constructor
    named_parameter_combine( NP const& np, Rest const& r )
    : Rest( r )
    , m_param( np )
    {
    }

    // Access methods
    res_type    operator[]( keyword<typename NP::id,true> kw ) const    { return m_param[kw]; }
    res_type    operator[]( keyword<typename NP::id,false> kw ) const   { return m_param[kw]; }
    using       Rest::operator[];

    bool        has( keyword<typename NP::id,false> kw ) const          { return m_param.has( kw ); }
    using       Rest::has;

    void        erase( keyword<typename NP::id,false> kw ) const        { m_param.erase( kw ); }
    using       Rest::erase;

    using       nfp_detail::named_parameter_base<named_parameter_combine<NP,Rest> >::operator,;

    // Visitation support
    template<typename Visitor>
    void            apply_to( Visitor& V ) const
    {
        m_param.apply_to( V );

        Rest::apply_to( V );
    }
private:
    // Data members
    NP          m_param;
};

// ************************************************************************** //
// **************                named_parameter               ************** //
// ************************************************************************** //

template<typename T, typename unique_id, typename RefType>
struct named_parameter
: nfp_detail::named_parameter_base<named_parameter<T,unique_id,RefType> >
{
    typedef T               data_type;
    typedef RefType         ref_type;
    typedef unique_id       id;

    // Constructor
    explicit        named_parameter( ref_type v )
    : m_value( v )
    , m_erased( false )
    {}
    named_parameter( named_parameter const& np )
    : m_value( np.m_value )
    , m_erased( np.m_erased )
    {}

    // Access methods
    ref_type        operator[]( keyword<unique_id,true> ) const     { return m_erased ? nil::inst().template any_cast<ref_type>() :  m_value; }
    ref_type        operator[]( keyword<unique_id,false> ) const    { return m_erased ? nil::inst().template any_cast<ref_type>() :  m_value; }
    template<typename UnknownId>
    nil             operator[]( keyword<UnknownId,false> ) const    { return nil::inst(); }

    bool            has( keyword<unique_id,false> ) const           { return !m_erased; }
    template<typename UnknownId>
    bool            has( keyword<UnknownId,false> ) const           { return false; }

    void            erase( keyword<unique_id,false> ) const         { m_erased = true; }
    template<typename UnknownId>
    void            erase( keyword<UnknownId,false> ) const         {}

    // Visitation support
    template<typename Visitor>
    void            apply_to( Visitor& V ) const
    {
        V.set_parameter( rtti::type_id<unique_id>(), m_value );
    }

private:
    // Data members
    ref_type        m_value;
    mutable bool    m_erased;
};

// ************************************************************************** //
// **************                   no_params                  ************** //
// ************************************************************************** //

typedef named_parameter<char, struct no_params_type_t,char> no_params_type;

namespace {
no_params_type no_params( '\0' );
} // local namespace

// ************************************************************************** //
// **************                    keyword                   ************** //
// ************************************************************************** //

template<typename unique_id, bool required = false>
struct keyword {
    typedef unique_id id;

    template<typename T>
    named_parameter<T const,unique_id>
    operator=( T const& t ) const       { return named_parameter<T const,unique_id>( t ); }

    template<typename T>
    named_parameter<T,unique_id>
    operator=( T& t ) const             { return named_parameter<T,unique_id>( t ); }

    named_parameter<char const*,unique_id,char const*>
    operator=( char const* t ) const    { return named_parameter<char const*,unique_id,char const*>( t ); }
};

//____________________________________________________________________________//

// ************************************************************************** //
// **************                 typed_keyword                ************** //
// ************************************************************************** //

template<typename T, typename unique_id, bool required>
struct typed_keyword : keyword<unique_id,required> {
    named_parameter<T const,unique_id>
    operator=( T const& t ) const       { return named_parameter<T const,unique_id>( t ); }

    named_parameter<T,unique_id>
    operator=( T& t ) const             { return named_parameter<T,unique_id>( t ); }
};

//____________________________________________________________________________//

template<typename unique_id, bool required>
struct typed_keyword<bool,unique_id,required>
: keyword<unique_id,required>
, named_parameter<bool,unique_id,bool> {
    typedef unique_id id;

    typed_keyword() : named_parameter<bool,unique_id,bool>( true ) {}

    named_parameter<bool,unique_id,bool>
    operator!() const           { return named_parameter<bool,unique_id,bool>( false ); }
};

// ************************************************************************** //
// **************                  opt_assign                  ************** //
// ************************************************************************** //

template<typename T, typename Params, typename Keyword>
inline typename enable_if_c<!has_param<Params,Keyword>::value,void>::type
opt_assign( T& /*target*/, Params const& /*p*/, Keyword /*k*/ )
{
}

//____________________________________________________________________________//

template<typename T, typename Params, typename Keyword>
inline typename enable_if_c<has_param<Params,Keyword>::value,void>::type
opt_assign( T& target, Params const& p, Keyword k )
{
    using namespace unit_test;

    assign_op( target, p[k], static_cast<int>(0) );
}

// ************************************************************************** //
// **************                    opt_get                   ************** //
// ************************************************************************** //

template<typename T, typename Params, typename Keyword>
inline T
opt_get( Params const& p, Keyword k, T default_val )
{
    opt_assign( default_val, p, k );

    return default_val;
}

// ************************************************************************** //
// **************                    opt_get                   ************** //
// ************************************************************************** //

template<typename Params, typename NP>
inline typename enable_if_c<!has_param<Params,keyword<typename NP::id> >::value,
named_parameter_combine<NP,Params> >::type
opt_append( Params const& params, NP const& np )
{
    return (params,np);
}

//____________________________________________________________________________//

template<typename Params, typename NP>
inline typename enable_if_c<has_param<Params,keyword<typename NP::id> >::value,Params>::type
opt_append( Params const& params, NP const& )
{
    return params;
}

} // namespace nfp
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UTILS_NAMED_PARAM

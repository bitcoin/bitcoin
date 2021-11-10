/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_MACRO_DEFINITION_HPP_D68A639E_2DA5_4E9C_8ACD_CFE6B903831E_INCLUDED)
#define BOOST_MACRO_DEFINITION_HPP_D68A639E_2DA5_4E9C_8ACD_CFE6B903831E_INCLUDED

#include <vector>
#include <list>

#include <boost/detail/atomic_count.hpp>
#include <boost/intrusive_ptr.hpp>

#include <boost/wave/wave_config.hpp>
#if BOOST_WAVE_SERIALIZATION != 0
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#endif

#include <boost/wave/token_ids.hpp>

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace util {

///////////////////////////////////////////////////////////////////////////////
//
//  macro_definition
//
//      This class containes all infos for a defined macro.
//
///////////////////////////////////////////////////////////////////////////////
template <typename TokenT, typename ContainerT>
struct macro_definition {

    typedef std::vector<TokenT> parameter_container_type;
    typedef ContainerT          definition_container_type;

    typedef typename parameter_container_type::const_iterator
        const_parameter_iterator_t;
    typedef typename definition_container_type::const_iterator
        const_definition_iterator_t;

    macro_definition(TokenT const &token_, bool has_parameters,
            bool is_predefined_, long uid_)
    :   macroname(token_), uid(uid_), is_functionlike(has_parameters),
        replaced_parameters(false), is_available_for_replacement(true),
        is_predefined(is_predefined_)
#if BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0
        , has_ellipsis(false)
#endif
        , use_count(0)
    {
    }
    // generated copy constructor
    // generated destructor
    // generated assignment operator

    // Replace all occurrences of the parameters throughout the macrodefinition
    // with special parameter tokens to simplify later macro replacement.
    // Additionally mark all occurrences of the macro name itself throughout
    // the macro definition
    template<typename ContextT>
    void replace_parameters(ContextT const & ctx)
    {
        using namespace boost::wave;

        if (!replaced_parameters) {
            typename definition_container_type::iterator end = macrodefinition.end();
            typename definition_container_type::iterator it = macrodefinition.begin();

            for (/**/; it != end; ++it) {
                token_id id = *it;

                if (T_IDENTIFIER == id ||
                    IS_CATEGORY(id, KeywordTokenType) ||
                    IS_EXTCATEGORY(id, OperatorTokenType|AltExtTokenType) ||
                    IS_CATEGORY(id, OperatorTokenType))
                {
                // may be a parameter to replace
                    const_parameter_iterator_t cend = macroparameters.end();
                    const_parameter_iterator_t cit = macroparameters.begin();
                    for (typename parameter_container_type::size_type i = 0;
                        cit != cend; ++cit, ++i)
                    {
                        if ((*it).get_value() == (*cit).get_value()) {
                            (*it).set_token_id(token_id(T_PARAMETERBASE+i));
                            break;
                        }
#if BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0
                        else if (need_variadics(ctx.get_language()) &&
                            T_ELLIPSIS == token_id(*cit) &&
                            "__VA_ARGS__" == (*it).get_value())
                        {
                        // __VA_ARGS__ requires special handling
                            (*it).set_token_id(token_id(T_EXTPARAMETERBASE+i));
                            break;
                        }
#if BOOST_WAVE_SUPPORT_VA_OPT != 0
                        else if (need_va_opt(ctx.get_language()) &&
                            T_ELLIPSIS == token_id(*cit) &&
                            "__VA_OPT__" == (*it).get_value())
                        {
                        // __VA_OPT__ also requires related special handling
                            (*it).set_token_id(token_id(T_OPTPARAMETERBASE+i));
                            break;
                        }
#endif
#endif
                    }
                }
            }

#if BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0
            // we need to know, if the last of the formal arguments is an ellipsis
            if (macroparameters.size() > 0 &&
                T_ELLIPSIS == token_id(macroparameters.back()))
            {
                has_ellipsis = true;
            }
#endif
            replaced_parameters = true;     // do it only once
        }
    }

    TokenT macroname;                       // macro name
    parameter_container_type macroparameters;  // formal parameters
    definition_container_type macrodefinition; // macro definition token sequence
    long uid;                               // unique id of this macro
    bool is_functionlike;
    bool replaced_parameters;
    bool is_available_for_replacement;
    bool is_predefined;
#if BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0
    bool has_ellipsis;
#endif
    boost::detail::atomic_count use_count;

#if BOOST_WAVE_SERIALIZATION != 0
    // default constructor is needed for serialization only
    macro_definition()
    :   uid(0), is_functionlike(false), replaced_parameters(false),
        is_available_for_replacement(false), is_predefined(false)
#if BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0
      , has_ellipsis(false)
#endif
      , use_count(0)
    {}

private:
    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        using namespace boost::serialization;
        ar & make_nvp("name", macroname);
        ar & make_nvp("parameters", macroparameters);
        ar & make_nvp("definition", macrodefinition);
        ar & make_nvp("uid", uid);
        ar & make_nvp("is_functionlike", is_functionlike);
        ar & make_nvp("has_replaced_parameters", replaced_parameters);
        ar & make_nvp("is_available_for_replacement", is_available_for_replacement);
        ar & make_nvp("is_predefined", is_predefined);
#if BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0
        ar & make_nvp("has_ellipsis", has_ellipsis);
#endif
    }
#endif
};

#if BOOST_WAVE_SERIALIZATION == 0
///////////////////////////////////////////////////////////////////////////////
template <typename TokenT, typename ContainerT>
inline void
intrusive_ptr_add_ref(macro_definition<TokenT, ContainerT>* p)
{
    ++p->use_count;
}

template <typename TokenT, typename ContainerT>
inline void
intrusive_ptr_release(macro_definition<TokenT, ContainerT>* p)
{
    if (--p->use_count == 0)
        delete p;
}
#endif

///////////////////////////////////////////////////////////////////////////////
}   // namespace util
}   // namespace wave
}   // namespace boost

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !defined(BOOST_MACRO_DEFINITION_HPP_D68A639E_2DA5_4E9C_8ACD_CFE6B903831E_INCLUDED)

/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Macro expansion engine

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_CPP_MACROMAP_HPP_CB8F51B0_A3F0_411C_AEF4_6FF631B8B414_INCLUDED)
#define BOOST_CPP_MACROMAP_HPP_CB8F51B0_A3F0_411C_AEF4_6FF631B8B414_INCLUDED

#include <cstdlib>
#include <cstdio>
#include <ctime>

#include <list>
#include <map>
#include <set>
#include <vector>
#include <iterator>
#include <algorithm>

#include <boost/assert.hpp>
#include <boost/wave/wave_config.hpp>
#if BOOST_WAVE_SERIALIZATION != 0
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/shared_ptr.hpp>
#endif

#include <boost/filesystem/path.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>

#include <boost/wave/util/time_conversion_helper.hpp>
#include <boost/wave/util/unput_queue_iterator.hpp>
#include <boost/wave/util/macro_helpers.hpp>
#include <boost/wave/util/macro_definition.hpp>
#include <boost/wave/util/symbol_table.hpp>
#include <boost/wave/util/cpp_macromap_utils.hpp>
#include <boost/wave/util/cpp_macromap_predef.hpp>
#include <boost/wave/util/filesystem_compatibility.hpp>
#include <boost/wave/grammars/cpp_defined_grammar_gen.hpp>
#if BOOST_WAVE_SUPPORT_HAS_INCLUDE != 0
#include <boost/wave/grammars/cpp_has_include_grammar_gen.hpp>
#endif

#include <boost/wave/wave_version.hpp>
#include <boost/wave/cpp_exceptions.hpp>
#include <boost/wave/language_support.hpp>

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace wave { namespace util {

///////////////////////////////////////////////////////////////////////////////
//
//  macromap
//
//      This class holds all currently defined macros and on demand expands
//      those macro definitions
//
///////////////////////////////////////////////////////////////////////////////
template <typename ContextT>
class macromap {

    typedef macromap<ContextT>                      self_type;
    typedef typename ContextT::token_type           token_type;
    typedef typename token_type::string_type        string_type;
    typedef typename token_type::position_type      position_type;

    typedef typename ContextT::token_sequence_type  definition_container_type;
    typedef std::vector<token_type>                 parameter_container_type;

    typedef macro_definition<token_type, definition_container_type>
        macro_definition_type;
    typedef symbol_table<string_type, macro_definition_type>
        defined_macros_type;
    typedef typename defined_macros_type::value_type::second_type
        macro_ref_type;

public:
    macromap(ContextT &ctx_)
    :   current_macros(0), defined_macros(new defined_macros_type(1)),
        main_pos("", 0), ctx(ctx_), macro_uid(1)
    {
        current_macros = defined_macros.get();
    }
    ~macromap() {}

    //  Add a new macro to the given macro scope
    bool add_macro(token_type const &name, bool has_parameters,
        parameter_container_type &parameters,
        definition_container_type &definition, bool is_predefined = false,
        defined_macros_type *scope = 0);

    //  Tests, whether the given macro name is defined in the given macro scope
    bool is_defined(string_type const &name,
        typename defined_macros_type::iterator &it,
        defined_macros_type *scope = 0) const;

    // expects a token sequence as its parameters
    template <typename IteratorT>
    bool is_defined(IteratorT const &begin, IteratorT const &end) const;

    // expects an arbitrary string as its parameter
    bool is_defined(string_type const &str) const;

#if BOOST_WAVE_SUPPORT_HAS_INCLUDE != 0
    // expects a token sequence as its parameters
    template <typename IteratorT>
    bool has_include(IteratorT const &begin, IteratorT const &end,
                     bool is_quoted_filename, bool is_system) const;
#endif

    //  Get the macro definition for the given macro scope
    bool get_macro(string_type const &name, bool &has_parameters,
        bool &is_predefined, position_type &pos,
        parameter_container_type &parameters,
        definition_container_type &definition,
        defined_macros_type *scope = 0) const;

    //  Remove a macro name from the given macro scope
    bool remove_macro(string_type const &name, position_type const& pos,
        bool even_predefined = false);

    template <typename IteratorT, typename ContainerT>
    token_type const &expand_tokensequence(IteratorT &first,
        IteratorT const &last, ContainerT &pending, ContainerT &expanded,
        bool& seen_newline, bool expand_operator_defined,
        bool expand_operator_has_include);

    //  Expand all macros inside the given token sequence
    template <typename IteratorT, typename ContainerT>
    void expand_whole_tokensequence(ContainerT &expanded,
        IteratorT &first, IteratorT const &last,
        bool expand_operator_defined,
        bool expand_operator_has_include);

    //  Init the predefined macros (add them to the given scope)
    void init_predefined_macros(char const *fname = "<Unknown>",
        defined_macros_type *scope = 0, bool at_global_scope = true);
    void predefine_macro(defined_macros_type *scope, string_type const &name,
        token_type const &t);

    //  Init the internal macro symbol namespace
    void reset_macromap();

    position_type &get_main_pos() { return main_pos; }
    position_type const& get_main_pos() const { return main_pos; }

    //  interface for macro name introspection
    typedef typename defined_macros_type::name_iterator name_iterator;
    typedef typename defined_macros_type::const_name_iterator const_name_iterator;

    name_iterator begin()
        { return defined_macros_type::make_iterator(current_macros->begin()); }
    name_iterator end()
        { return defined_macros_type::make_iterator(current_macros->end()); }
    const_name_iterator begin() const
        { return defined_macros_type::make_iterator(current_macros->begin()); }
    const_name_iterator end() const
        { return defined_macros_type::make_iterator(current_macros->end()); }

protected:
    //  Helper functions for expanding all macros in token sequences
    template <typename IteratorT, typename ContainerT>
    token_type const &expand_tokensequence_worker(ContainerT &pending,
        unput_queue_iterator<IteratorT, token_type, ContainerT> &first,
        unput_queue_iterator<IteratorT, token_type, ContainerT> const &last,
        bool& seen_newline, bool expand_operator_defined,
        bool expand_operator_has_include,
        boost::optional<position_type> expanding_pos);

//  Collect all arguments supplied to a macro invocation
    template <typename IteratorT, typename ContainerT, typename SizeT>
    typename std::vector<ContainerT>::size_type collect_arguments (
        token_type const curr_token, std::vector<ContainerT> &arguments,
        IteratorT &next, IteratorT &endparen, IteratorT const &end,
        SizeT const &parameter_count, bool& seen_newline);

    //  Expand a single macro name
    template <typename IteratorT, typename ContainerT>
    bool expand_macro(ContainerT &pending, token_type const &name,
        typename defined_macros_type::iterator it,
        IteratorT &first, IteratorT const &last,
        bool& seen_newline, bool expand_operator_defined,
        bool expand_operator_has_include,
        boost::optional<position_type> expanding_pos,
        defined_macros_type *scope = 0, ContainerT *queue_symbol = 0);

    //  Expand a predefined macro (__LINE__, __FILE__ and __INCLUDE_LEVEL__)
    template <typename ContainerT>
    bool expand_predefined_macro(token_type const &curr_token,
        ContainerT &expanded);

    //  Expand a single macro argument
    template <typename ContainerT>
    void expand_argument (typename std::vector<ContainerT>::size_type arg,
        std::vector<ContainerT> &arguments,
        std::vector<ContainerT> &expanded_args, bool expand_operator_defined,
        bool expand_operator_has_include,
        std::vector<bool> &has_expanded_args);

    //  Expand the replacement list (replaces parameters with arguments)
    template <typename ContainerT>
    void expand_replacement_list(
        typename macro_definition_type::const_definition_iterator_t cbeg,
        typename macro_definition_type::const_definition_iterator_t cend,
        std::vector<ContainerT> &arguments,
        bool expand_operator_defined,
        bool expand_operator_has_include,
        ContainerT &expanded);

    //  Rescans the replacement list for macro expansion
    template <typename IteratorT, typename ContainerT>
    void rescan_replacement_list(token_type const &curr_token,
        macro_definition_type &macrodef, ContainerT &replacement_list,
        ContainerT &expanded, bool expand_operator_defined,
        bool expand_operator_has_include,
        IteratorT &nfirst, IteratorT const &nlast);

    //  Resolves the operator defined() and replaces the token with "0" or "1"
    template <typename IteratorT, typename ContainerT>
    token_type const &resolve_defined(IteratorT &first, IteratorT const &last,
        ContainerT &expanded);

#if BOOST_WAVE_SUPPORT_HAS_INCLUDE != 0
    //  Resolves the operator __has_include() and replaces the token with "0" or "1"
    template <typename IteratorT, typename ContainerT>
    token_type const &resolve_has_include(IteratorT &first, IteratorT const &last,
        ContainerT &expanded);
#endif

    //  Resolve operator _Pragma or the #pragma directive
    template <typename IteratorT, typename ContainerT>
    bool resolve_operator_pragma(IteratorT &first,
        IteratorT const &last, ContainerT &expanded, bool& seen_newline);

    //  Handle the concatenation operator '##'
    template <typename ContainerT>
    bool concat_tokensequence(ContainerT &expanded);

    template <typename ContainerT>
    bool is_valid_concat(string_type new_value,
        position_type const &pos, ContainerT &rescanned);

    static bool is_space(char);

    // batch update tokens with a single expand position
    template <typename ContainerT>
    static void set_expand_positions(ContainerT &tokens, position_type pos);

#if BOOST_WAVE_SERIALIZATION != 0
public:
    BOOST_STATIC_CONSTANT(unsigned int, version = 0x10);
    BOOST_STATIC_CONSTANT(unsigned int, version_mask = 0x0f);

private:
    friend class boost::serialization::access;
    template<typename Archive>
    void save(Archive &ar, const unsigned int version) const
    {
        using namespace boost::serialization;
        ar & make_nvp("defined_macros", defined_macros);
    }
    template<typename Archive>
    void load(Archive &ar, const unsigned int loaded_version)
    {
        using namespace boost::serialization;
        if (version != (loaded_version & ~version_mask)) {
            BOOST_WAVE_THROW(preprocess_exception, incompatible_config,
                "cpp_context state version", get_main_pos());
        }
        ar & make_nvp("defined_macros", defined_macros);
        current_macros = defined_macros.get();
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
#endif

private:
    defined_macros_type *current_macros;                   // current symbol table
    boost::shared_ptr<defined_macros_type> defined_macros; // global symbol table

    token_type act_token;       // current token
    position_type main_pos;     // last token position in the pp_iterator
    string_type base_name;      // the name to be expanded by __BASE_FILE__
    ContextT &ctx;              // context object associated with the macromap
    long macro_uid;
    predefined_macros predef;   // predefined macro support
};
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
//  add_macro(): adds a new macro to the macromap
//
///////////////////////////////////////////////////////////////////////////////
template <typename ContextT>
inline bool
macromap<ContextT>::add_macro(token_type const &name, bool has_parameters,
    parameter_container_type &parameters, definition_container_type &definition,
    bool is_predefined, defined_macros_type *scope)
{
    if (!is_predefined && impl::is_special_macroname (ctx, name.get_value())) {
        // exclude special macro names
        BOOST_WAVE_THROW_NAME_CTX(ctx, macro_handling_exception,
            illegal_redefinition, name.get_value().c_str(), main_pos,
            name.get_value().c_str());
        return false;
    }
    if (boost::wave::need_variadics(ctx.get_language()) &&
        "__VA_ARGS__" == name.get_value())
    {
        // can't use __VA_ARGS__ as a macro name
        BOOST_WAVE_THROW_NAME_CTX(ctx, macro_handling_exception,
            bad_define_statement_va_args, name.get_value().c_str(), main_pos,
            name.get_value().c_str());
        return false;
    }
    if (boost::wave::need_variadics(ctx.get_language()) &&
        "__VA_OPT__" == name.get_value())
    {
        // can't use __VA_OPT__ as a macro name
        BOOST_WAVE_THROW_NAME_CTX(ctx, macro_handling_exception,
            bad_define_statement_va_opt, name.get_value().c_str(), main_pos,
            name.get_value().c_str());
        return false;
    }
#if BOOST_WAVE_SUPPORT_HAS_INCLUDE != 0
    if (boost::wave::need_has_include(ctx.get_language()) &&
        "__has_include" == name.get_value())
    {
        // can't use __has_include as a macro name
        BOOST_WAVE_THROW_NAME_CTX(ctx, macro_handling_exception,
            bad_define_statement_va_opt, name.get_value().c_str(), main_pos,
            name.get_value().c_str());
        return false;
    }
#endif
    if (AltExtTokenType == (token_id(name) & ExtTokenOnlyMask)) {
        // exclude special operator names
        BOOST_WAVE_THROW_NAME_CTX(ctx, macro_handling_exception,
            illegal_operator_redefinition, name.get_value().c_str(), main_pos,
            name.get_value().c_str());
        return false;
    }

    // try to define the new macro
    defined_macros_type* current_scope = scope ? scope : current_macros;
    typename defined_macros_type::iterator it = current_scope->find(name.get_value());

    if (it != current_scope->end()) {
        // redefinition, should not be different
        macro_definition_type* macrodef = (*it).second.get();
        if (macrodef->is_functionlike != has_parameters ||
            !impl::parameters_equal(macrodef->macroparameters, parameters) ||
            !impl::definition_equals(macrodef->macrodefinition, definition))
        {
            BOOST_WAVE_THROW_NAME_CTX(ctx, macro_handling_exception,
                macro_redefinition, name.get_value().c_str(), main_pos,
                name.get_value().c_str());
        }
        return false;
    }

    // test the validity of the parameter names
    if (has_parameters) {
        std::set<typename token_type::string_type> names;

        typedef typename parameter_container_type::iterator
            parameter_iterator_type;
        typedef typename std::set<typename token_type::string_type>::iterator
            name_iterator_type;

        parameter_iterator_type end = parameters.end();
        for (parameter_iterator_type itp = parameters.begin(); itp != end; ++itp)
        {
        name_iterator_type pit = names.find((*itp).get_value());

            if (pit != names.end()) {
                // duplicate parameter name
                BOOST_WAVE_THROW_NAME_CTX(ctx, macro_handling_exception,
                    duplicate_parameter_name, (*pit).c_str(), main_pos,
                    name.get_value().c_str());
                return false;
            }
            names.insert((*itp).get_value());
        }
    }

#if BOOST_WAVE_SUPPORT_VA_OPT != 0
    // check that __VA_OPT__ is used as a function macro
    if (boost::wave::need_va_opt(ctx.get_language())) {
        // __VA_OPT__, if present, must be followed by an lparen
        typedef typename macro_definition_type::const_definition_iterator_t iter_t;
        iter_t mdit = definition.begin();
        iter_t mdend = definition.end();
        for (; mdit != mdend; ++mdit) {
            // is this va_opt?
            if ((IS_EXTCATEGORY((*mdit), OptParameterTokenType)) ||  // if params replaced
                ("__VA_OPT__" == (*mdit).get_value())) {             // if not
                iter_t va_opt_it = mdit;
                // next must be lparen
                if ((++mdit == mdend) ||                             // no further tokens
                    (T_LEFTPAREN != token_id(*mdit))) {              // not lparen
                    BOOST_WAVE_THROW_NAME_CTX(ctx, macro_handling_exception,
                        bad_define_statement_va_opt_parens,
                        name.get_value().c_str(), main_pos,
                        name.get_value().c_str());
                    return false;
                }
                // check that no __VA_OPT__ appears inside
                iter_t va_opt_end = va_opt_it;
                if (!impl::find_va_opt_args(va_opt_end, mdend)) {
                    BOOST_WAVE_THROW_CTX(ctx, preprocess_exception,
                        improperly_terminated_macro, "missing ')' in __VA_OPT__",
                        main_pos);
                    return false;
                }
                // skip initial __VA_OPT__ and lparen
                ++va_opt_it; ++va_opt_it;
                for (;va_opt_it != va_opt_end; ++va_opt_it) {
                    if ((IS_EXTCATEGORY((*va_opt_it), OptParameterTokenType)) ||
                        ("__VA_OPT__" == (*va_opt_it).get_value())) {
                        BOOST_WAVE_THROW_NAME_CTX(ctx, macro_handling_exception,
                            bad_define_statement_va_opt_recurse,
                            name.get_value().c_str(), (*va_opt_it).get_position(),
                            name.get_value().c_str());
                    }
                }
            }
        }
    }
#endif

    // insert a new macro node
    std::pair<typename defined_macros_type::iterator, bool> p =
        current_scope->insert(
            typename defined_macros_type::value_type(
                name.get_value(),
                macro_ref_type(new macro_definition_type(name,
                    has_parameters, is_predefined, ++macro_uid)
                )
            )
        );

    if (!p.second) {
        BOOST_WAVE_THROW_NAME_CTX(ctx, macro_handling_exception,
            macro_insertion_error, name.get_value().c_str(), main_pos,
            name.get_value().c_str());
        return false;
    }

    // add the parameters and the definition
    std::swap((*p.first).second->macroparameters, parameters);
    std::swap((*p.first).second->macrodefinition, definition);

// call the context supplied preprocessing hook
    ctx.get_hooks().defined_macro(ctx.derived(), name, has_parameters,
        (*p.first).second->macroparameters,
        (*p.first).second->macrodefinition, is_predefined);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
//
//  is_defined(): returns, whether a given macro is already defined
//
///////////////////////////////////////////////////////////////////////////////
template <typename ContextT>
inline bool
macromap<ContextT>::is_defined(typename token_type::string_type const &name,
    typename defined_macros_type::iterator &it,
    defined_macros_type *scope) const
{
    if (0 == scope) scope = current_macros;

    if ((it = scope->find(name)) != scope->end())
        return true;        // found in symbol table

    // quick pre-check
    if (name.size() < 8 || '_' != name[0] || '_' != name[1])
        return false;       // quick check failed

    if (name == "__LINE__" || name == "__FILE__" ||
        name == "__INCLUDE_LEVEL__")
        return true;

#if BOOST_WAVE_SUPPORT_HAS_INCLUDE != 0
    return (boost::wave::need_has_include(ctx.get_language()) &&
            (name == "__has_include"));
#else
    return false;
#endif
}

template <typename ContextT>
template <typename IteratorT>
inline bool
macromap<ContextT>::is_defined(IteratorT const &begin,
    IteratorT const &end) const
{
    // in normal mode the name under inspection should consist of an identifier
    // only
    token_id id = token_id(*begin);

    if (T_IDENTIFIER != id &&
        !IS_CATEGORY(id, KeywordTokenType) &&
        !IS_EXTCATEGORY(id, OperatorTokenType|AltExtTokenType) &&
        !IS_CATEGORY(id, BoolLiteralTokenType))
    {
        std::string msg(impl::get_full_name(begin, end));
        BOOST_WAVE_THROW_CTX(ctx, preprocess_exception, invalid_macroname,
            msg.c_str(), main_pos);
        return false;
    }

    IteratorT it = begin;
    string_type name((*it).get_value());
    typename defined_macros_type::iterator cit;

    if (++it != end) {
        // there should be only one token as the inspected name
        std::string msg(impl::get_full_name(begin, end));
        BOOST_WAVE_THROW_CTX(ctx, preprocess_exception, invalid_macroname,
            msg.c_str(), main_pos);
        return false;
    }
    return is_defined(name, cit, 0);
}

///////////////////////////////////////////////////////////////////////////////
//  same as above, only takes an arbitrary string type as its parameter
template <typename ContextT>
inline bool
macromap<ContextT>::is_defined(string_type const &str) const
{
    typename defined_macros_type::iterator cit;
    return is_defined(str, cit, 0);
}

#if BOOST_WAVE_SUPPORT_HAS_INCLUDE != 0
///////////////////////////////////////////////////////////////////////////////
//
//  has_include(): returns whether a path expression is an valid include file
//
///////////////////////////////////////////////////////////////////////////////
template <typename ContextT>
template <typename IteratorT>
inline bool
macromap<ContextT>::has_include(
    IteratorT const &begin, IteratorT const &end,
    bool is_quoted_filename, bool is_system) const
{
    typename ContextT::token_sequence_type filetoks;

    if (is_quoted_filename) {
        filetoks = typename ContextT::token_sequence_type(begin, end);
    } else {
        IteratorT first = begin;
        IteratorT last = end;
        ctx.expand_whole_tokensequence(first, last, filetoks);
    }

    // extract tokens into string and trim whitespace
    using namespace boost::wave::util::impl;
    std::string fn(trim_whitespace(as_string(filetoks)).c_str());

    // verify and remove initial and final delimiters
    if (!((fn.size() >= 3) &&
          (((fn[0] == '"') && (*fn.rbegin() == '"')) ||
           ((fn[0] == '<') && (*fn.rbegin() == '>')))))
        BOOST_WAVE_THROW_CTX(ctx, preprocess_exception, bad_has_include_expression,
                             fn.c_str(), ctx.get_main_pos());

    fn = fn.substr(1, fn.size() - 2);

    // test header existence
    std::string dir_path;
    std::string native_path;
    return ctx.get_hooks().locate_include_file(
        ctx, fn, is_system, 0, dir_path, native_path);

}
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  Get the macro definition for the given macro scope
//
///////////////////////////////////////////////////////////////////////////////
template <typename ContextT>
inline bool
macromap<ContextT>::get_macro(string_type const &name, bool &has_parameters,
    bool &is_predefined, position_type &pos,
    parameter_container_type &parameters,
    definition_container_type &definition,
    defined_macros_type *scope) const
{
    typename defined_macros_type::iterator it;
    if (!is_defined(name, it, scope))
        return false;

    macro_definition_type& macro_def = *(*it).second.get();

    has_parameters = macro_def.is_functionlike;
    is_predefined = macro_def.is_predefined;
    pos = macro_def.macroname.get_position();
    parameters = macro_def.macroparameters;
    definition = macro_def.macrodefinition;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
//
//  remove_macro(): remove a macro from the macromap
//
///////////////////////////////////////////////////////////////////////////////
template <typename ContextT>
inline bool
macromap<ContextT>::remove_macro(string_type const &name,
    position_type const& pos, bool even_predefined)
{
    typename defined_macros_type::iterator it = current_macros->find(name);

    if (it != current_macros->end()) {
        if ((*it).second->is_predefined) {
            if (!even_predefined || impl::is_special_macroname(ctx, name)) {
                BOOST_WAVE_THROW_CTX(ctx, preprocess_exception,
                    bad_undefine_statement, name.c_str(), main_pos);
                return false;
            }
        }
        current_macros->erase(it);

        // call the context supplied preprocessing hook function
        token_type tok(T_IDENTIFIER, name, pos);

        ctx.get_hooks().undefined_macro(ctx.derived(), tok);
        return true;
    }
    else if (impl::is_special_macroname(ctx, name)) {
        BOOST_WAVE_THROW_CTX(ctx, preprocess_exception, bad_undefine_statement,
            name.c_str(), pos);
    }
    return false;       // macro was not defined
}

///////////////////////////////////////////////////////////////////////////////
//
//  expand_tokensequence
//
//      This function is a helper function which wraps the given iterator
//      range into corresponding unput_iterator's and calls the main workhorse
//      of the macro expansion engine (the function expand_tokensequence_worker)
//
//      This is the top level macro expansion function called from the
//      preprocessing iterator component only.
//
///////////////////////////////////////////////////////////////////////////////
template <typename ContextT>
template <typename IteratorT, typename ContainerT>
inline typename ContextT::token_type const &
macromap<ContextT>::expand_tokensequence(IteratorT &first,
    IteratorT const &last, ContainerT &pending, ContainerT &expanded,
    bool& seen_newline, bool expand_operator_defined,
    bool expand_operator_has_include)
{
    typedef impl::gen_unput_queue_iterator<IteratorT, token_type, ContainerT>
        gen_type;
    typedef typename gen_type::return_type iterator_type;

    iterator_type first_it = gen_type::generate(expanded, first);
    iterator_type last_it = gen_type::generate(last);

    on_exit::assign<IteratorT, iterator_type> on_exit(first, first_it);

    return expand_tokensequence_worker(pending, first_it, last_it,
        seen_newline, expand_operator_defined, expand_operator_has_include,
        boost::none);
}

///////////////////////////////////////////////////////////////////////////////
//
//  expand_tokensequence_worker
//
//      This function is the main workhorse of the macro expansion engine. It
//      expands as much tokens as needed to identify the next preprocessed
//      token to return to the caller.
//      It returns the next preprocessed token.
//
//      The iterator 'first' is adjusted accordingly.
//
///////////////////////////////////////////////////////////////////////////////
template <typename ContextT>
template <typename IteratorT, typename ContainerT>
inline typename ContextT::token_type const &
macromap<ContextT>::expand_tokensequence_worker(
    ContainerT &pending,
    unput_queue_iterator<IteratorT, token_type, ContainerT> &first,
    unput_queue_iterator<IteratorT, token_type, ContainerT> const &last,
    bool& seen_newline, bool expand_operator_defined,
    bool expand_operator_has_include,
    boost::optional<position_type> expanding_pos)
{
    // if there exist pending tokens (tokens, which are already preprocessed), then
    // return the next one from there
    if (!pending.empty()) {
        on_exit::pop_front<definition_container_type> pop_front_token(pending);

        return act_token = pending.front();
    }

    //  analyze the next element of the given sequence, if it is an
    //  T_IDENTIFIER token, try to replace this as a macro etc.
    using namespace boost::wave;

    if (first != last) {
        token_id id = token_id(*first);

        // ignore placeholder tokens
        if (T_PLACEHOLDER == id) {
            token_type placeholder = *first;

            ++first;
            if (first == last)
                return act_token = placeholder;
            id = token_id(*first);
        }

        if (T_IDENTIFIER == id || IS_CATEGORY(id, KeywordTokenType) ||
            IS_EXTCATEGORY(id, OperatorTokenType|AltExtTokenType) ||
            IS_CATEGORY(id, BoolLiteralTokenType))
        {
        // try to replace this identifier as a macro
            if (expand_operator_defined && (*first).get_value() == "defined") {
            // resolve operator defined()
                return resolve_defined(first, last, pending);
            }
#if BOOST_WAVE_SUPPORT_HAS_INCLUDE != 0
            else if (boost::wave::need_has_include(ctx.get_language()) &&
                     expand_operator_has_include &&
                     (*first).get_value() == "__has_include") {
                // resolve operator __has_include()
                return resolve_has_include(first, last, pending);
            }
#endif
            else if (boost::wave::need_variadics(ctx.get_language()) &&
                (*first).get_value() == "_Pragma")
            {
                // in C99 mode only: resolve the operator _Pragma
                token_type curr_token = *first;

                if (!resolve_operator_pragma(first, last, pending, seen_newline) ||
                    pending.size() > 0)
                {
                    // unknown to us pragma or supplied replacement, return the
                    // next token
                    on_exit::pop_front<definition_container_type> pop_token(pending);

                    return act_token = pending.front();
                }

                // the operator _Pragma() was eaten completely, continue
                return act_token = token_type(T_PLACEHOLDER, "_",
                    curr_token.get_position());
            }

            token_type name_token(*first);
            typename defined_macros_type::iterator it;

            if (is_defined(name_token.get_value(), it)) {
                // the current token contains an identifier, which is currently
                // defined as a macro
                if (expand_macro(pending, name_token, it, first, last,
                                 seen_newline, expand_operator_defined,
                                 expand_operator_has_include,
                                 expanding_pos))
                {
                    // the tokens returned by expand_macro should be rescanned
                    // beginning at the last token of the returned replacement list
                    if (first != last) {
                        // splice the last token back into the input queue
                        typename ContainerT::reverse_iterator rit = pending.rbegin();

                        first.get_unput_queue().splice(
                            first.get_unput_queue().begin(), pending,
                            (++rit).base(), pending.end());
                    }

                    // fall through ...
                }
                else if (!pending.empty()) {
                    // return the first token from the pending queue
                    on_exit::pop_front<definition_container_type> pop_queue(pending);

                    return act_token = pending.front();
                }
                else {
                    // macro expansion reached the eoi
                    return act_token = token_type();
                }

                // return the next preprocessed token
                if (!expanding_pos)
                    expanding_pos = name_token.get_expand_position();

                typename ContextT::token_type const & result =
                    expand_tokensequence_worker(
                        pending, first, last,
                        seen_newline, expand_operator_defined,
                        expand_operator_has_include,
                        expanding_pos);

                return result;
            }
            else {
                act_token = name_token;
                ++first;
                return act_token;
            }
        }
        else if (expand_operator_defined && IS_CATEGORY(*first, BoolLiteralTokenType)) {
            // expanding a constant expression inside #if/#elif, special handling
            // of 'true' and 'false'

            // all remaining identifiers and keywords, except for true and false,
            // are replaced with the pp-number 0 (C++ standard 16.1.4, [cpp.cond])
            return act_token = token_type(T_INTLIT, T_TRUE != id ? "0" : "1",
                (*first++).get_position());
        }
        else {
            act_token = *first;
            ++first;
            return act_token;
        }
    }
    return act_token = token_type();     // eoi
}

///////////////////////////////////////////////////////////////////////////////
//
//  collect_arguments(): collect the actual arguments of a macro invocation
//
//      return the number of successfully detected non-empty arguments
//
///////////////////////////////////////////////////////////////////////////////
template <typename ContextT>
template <typename IteratorT, typename ContainerT, typename SizeT>
inline typename std::vector<ContainerT>::size_type
macromap<ContextT>::collect_arguments (token_type const curr_token,
    std::vector<ContainerT> &arguments, IteratorT &next, IteratorT &endparen,
    IteratorT const &end, SizeT const &parameter_count, bool& seen_newline)
{
    using namespace boost::wave;

    arguments.push_back(ContainerT());

    // collect the actual arguments
    typename std::vector<ContainerT>::size_type count_arguments = 0;
    int nested_parenthesis_level = 1;
    ContainerT* argument = &arguments[0];
    bool was_whitespace = false;
    token_type startof_argument_list = *next;

    while (++next != end && nested_parenthesis_level) {
        token_id id = token_id(*next);

        if (0 == parameter_count &&
            !IS_CATEGORY((*next), WhiteSpaceTokenType) && id != T_NEWLINE &&
            id != T_RIGHTPAREN && id != T_LEFTPAREN)
        {
        // there shouldn't be any arguments
            BOOST_WAVE_THROW_CTX(ctx, preprocess_exception,
                too_many_macroarguments, curr_token.get_value().c_str(),
                main_pos);
            return 0;
        }

        switch (id) {
        case T_LEFTPAREN:
            ++nested_parenthesis_level;
            argument->push_back(*next);
            was_whitespace = false;
            break;

        case T_RIGHTPAREN:
            {
                if (--nested_parenthesis_level >= 1)
                    argument->push_back(*next);
                else {
                // found closing parenthesis
//                    trim_sequence(argument);
                    endparen = next;
                    if (parameter_count > 0) {
                        if (argument->empty() ||
                            impl::is_whitespace_only(*argument))
                        {
#if BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0
                            if (boost::wave::need_variadics(ctx.get_language())) {
                            // store a placemarker as the argument
                                argument->push_back(token_type(T_PLACEMARKER, "\xA7",
                                    (*next).get_position()));
                                ++count_arguments;
                            }
#endif
                        }
                        else {
                            ++count_arguments;
                        }
                    }
                }
                was_whitespace = false;
            }
            break;

        case T_COMMA:
            if (1 == nested_parenthesis_level) {
                // next parameter
//                trim_sequence(argument);
                if (argument->empty() ||
                    impl::is_whitespace_only(*argument))
                {
#if BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0
                    if (boost::wave::need_variadics(ctx.get_language())) {
                        // store a placemarker as the argument
                        argument->push_back(token_type(T_PLACEMARKER, "\xA7",
                            (*next).get_position()));
                        ++count_arguments;
                    }
#endif
                }
                else {
                    ++count_arguments;
                }
                arguments.push_back(ContainerT()); // add new arg
                argument = &arguments[arguments.size()-1];
            }
            else {
                // surrounded by parenthesises, so store to current argument
                argument->push_back(*next);
            }
            was_whitespace = false;
            break;

        case T_NEWLINE:
            seen_newline = true;
            /* fall through */
        case T_SPACE:
        case T_SPACE2:
        case T_CCOMMENT:
            if (!was_whitespace)
                argument->push_back(token_type(T_SPACE, " ", (*next).get_position()));
            was_whitespace = true;
            break;      // skip whitespace

        case T_PLACEHOLDER:
            break;      // ignore placeholder

        default:
            argument->push_back(*next);
            was_whitespace = false;
            break;
        }
    }

    if (nested_parenthesis_level >= 1) {
        // missing ')': improperly terminated macro invocation
        BOOST_WAVE_THROW_CTX(ctx, preprocess_exception,
            improperly_terminated_macro, "missing ')'", main_pos);
        return 0;
    }

    // if no argument was expected and we didn't find any, than remove the empty
    // element
    if (0 == parameter_count && 0 == count_arguments) {
        BOOST_ASSERT(1 == arguments.size());
        arguments.clear();
    }
    return count_arguments;
}

///////////////////////////////////////////////////////////////////////////////
//
//  expand_whole_tokensequence
//
//      fully expands a given token sequence
//
///////////////////////////////////////////////////////////////////////////////
template <typename ContextT>
template <typename IteratorT, typename ContainerT>
inline void
macromap<ContextT>::expand_whole_tokensequence(ContainerT &expanded,
                                               IteratorT &first, IteratorT const &last,
                                               bool expand_operator_defined,
                                               bool expand_operator_has_include)
{
    typedef impl::gen_unput_queue_iterator<IteratorT, token_type, ContainerT>
        gen_type;
    typedef typename gen_type::return_type iterator_type;

    ContainerT empty;
    iterator_type first_it = gen_type::generate(empty, first);
    iterator_type last_it = gen_type::generate(last);

    on_exit::assign<IteratorT, iterator_type> on_exit(first, first_it);
    ContainerT pending_queue;
    bool seen_newline;

    while (!pending_queue.empty() || first_it != last_it) {
        expanded.push_back(
            expand_tokensequence_worker(
                pending_queue, first_it,
                last_it, seen_newline, expand_operator_defined,
                expand_operator_has_include,
                boost::none)
        );
    }

    // should have returned all expanded tokens
    BOOST_ASSERT(pending_queue.empty()/* && unput_queue.empty()*/);
}

///////////////////////////////////////////////////////////////////////////////
//
//  expand_argument
//
//      fully expands the given argument of a macro call
//
///////////////////////////////////////////////////////////////////////////////
template <typename ContextT>
template <typename ContainerT>
inline void
macromap<ContextT>::expand_argument (
    typename std::vector<ContainerT>::size_type arg,
    std::vector<ContainerT> &arguments, std::vector<ContainerT> &expanded_args,
    bool expand_operator_defined, bool expand_operator_has_include,
    std::vector<bool> &has_expanded_args)
{
    if (!has_expanded_args[arg]) {
        // expand the argument only once
        typedef typename std::vector<ContainerT>::value_type::iterator
            argument_iterator_type;

        argument_iterator_type begin_it = arguments[arg].begin();
        argument_iterator_type end_it = arguments[arg].end();

        expand_whole_tokensequence(
            expanded_args[arg], begin_it, end_it,
            expand_operator_defined, expand_operator_has_include);
        impl::remove_placeholders(expanded_args[arg]);
        has_expanded_args[arg] = true;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  expand_replacement_list
//
//      fully expands the replacement list of a given macro with the
//      actual arguments/expanded arguments
//      handles the '#' [cpp.stringize] and the '##' [cpp.concat] operator
//
///////////////////////////////////////////////////////////////////////////////
template <typename ContextT>
template <typename ContainerT>
inline void
macromap<ContextT>::expand_replacement_list(
    typename macro_definition_type::const_definition_iterator_t cit,
    typename macro_definition_type::const_definition_iterator_t cend,
    std::vector<ContainerT> &arguments, bool expand_operator_defined,
    bool expand_operator_has_include,
    ContainerT &expanded)
{
    using namespace boost::wave;
    typedef typename macro_definition_type::const_definition_iterator_t
        macro_definition_iter_t;

    std::vector<ContainerT> expanded_args(arguments.size());
    std::vector<bool> has_expanded_args(arguments.size());
    bool seen_concat = false;
    bool adjacent_concat = false;
    bool adjacent_stringize = false;

    for (;cit != cend; ++cit)
    {
        bool use_replaced_arg = true;
        token_id base_id = BASE_TOKEN(token_id(*cit));

        if (T_POUND_POUND == base_id) {
            // concatenation operator
            adjacent_concat = true;
            seen_concat = true;
        }
        else if (T_POUND == base_id) {
            // stringize operator
            adjacent_stringize = true;
        }
        else {
            if (adjacent_stringize || adjacent_concat ||
                T_POUND_POUND == impl::next_token<macro_definition_iter_t>
                    ::peek(cit, cend))
            {
                use_replaced_arg = false;
            }
            if (adjacent_concat)    // spaces after '##' ?
                adjacent_concat = IS_CATEGORY(*cit, WhiteSpaceTokenType);
        }

        if (IS_CATEGORY((*cit), ParameterTokenType)) {
            // copy argument 'i' instead of the parameter token i
            typename ContainerT::size_type i;
#if BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0
            bool is_ellipsis = false;
#if BOOST_WAVE_SUPPORT_VA_OPT != 0
            bool is_va_opt = false;
#endif

            if (IS_EXTCATEGORY((*cit), ExtParameterTokenType)) {
                BOOST_ASSERT(boost::wave::need_variadics(ctx.get_language()));
                i = token_id(*cit) - T_EXTPARAMETERBASE;
                is_ellipsis = true;
            }
            else
#if BOOST_WAVE_SUPPORT_VA_OPT != 0

            if (IS_EXTCATEGORY((*cit), OptParameterTokenType)) {
                BOOST_ASSERT(boost::wave::need_va_opt(ctx.get_language()));
                i = token_id(*cit) - T_OPTPARAMETERBASE;
                is_va_opt = true;
            }
            else
#endif
#endif
            {
                i = token_id(*cit) - T_PARAMETERBASE;
            }

            BOOST_ASSERT(i <= arguments.size());
            if (use_replaced_arg) {

#if BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0
                if (is_ellipsis) {
                    position_type const& pos = (*cit).get_position();

                    BOOST_ASSERT(boost::wave::need_variadics(ctx.get_language()));

                    // ensure all variadic arguments to be expanded
                    for (typename vector<ContainerT>::size_type arg = i;
                         arg < expanded_args.size(); ++arg)
                    {
                        expand_argument(
                            arg, arguments, expanded_args,
                            expand_operator_defined, expand_operator_has_include,
                            has_expanded_args);
                    }
                    impl::replace_ellipsis(expanded_args, i, expanded, pos);
                }
                else

#if BOOST_WAVE_SUPPORT_VA_OPT != 0
                if (is_va_opt) {
                    position_type const &pos = (*cit).get_position();

                    BOOST_ASSERT(boost::wave::need_va_opt(ctx.get_language()));

                    // ensure all variadic arguments to be expanded
                    for (typename vector<ContainerT>::size_type arg = i;
                         arg < expanded_args.size(); ++arg)
                    {
                        expand_argument(
                            arg, arguments, expanded_args,
                            expand_operator_defined, expand_operator_has_include,
                            has_expanded_args);
                    }

                    // locate the end of the __VA_OPT__ call
                    typename macro_definition_type::const_definition_iterator_t cstart = cit;
                    if (!impl::find_va_opt_args(cit, cend)) {
                        BOOST_WAVE_THROW_CTX(ctx, preprocess_exception,
                            improperly_terminated_macro, "missing '(' or ')' in __VA_OPT__",
                            pos);
                    }
                    // cstart still points to __VA_OPT__; cit now points to the last rparen

                    // locate the __VA_OPT__ arguments
                    typename macro_definition_type::const_definition_iterator_t arg_start = cstart;
                    ++arg_start;  // skip __VA_OPT__
                    ++arg_start;  // skip lparen

                    // create a synthetic macro definition for use with hooks
                    token_type macroname(T_IDENTIFIER, "__VA_OPT__", position_type("<built-in>"));
                    parameter_container_type macroparameters;
                    macroparameters.push_back(token_type(T_ELLIPSIS, "...", position_type("<built-in>")));
                    definition_container_type macrodefinition;

                    bool suppress_expand = false;
                    // __VA_OPT__ treats its arguments as an undifferentiated stream of tokens
                    // for our purposes we can consider it as a single argument
                    typename std::vector<ContainerT> va_opt_args(1, ContainerT(arg_start, cit));
                    suppress_expand = ctx.get_hooks().expanding_function_like_macro(
                        ctx.derived(),
                        macroname, macroparameters, macrodefinition,
                        *cstart, va_opt_args,
                        cstart, cit);

                    if (suppress_expand) {
                        // leave the whole expression in place
                        std::copy(cstart, cit, std::back_inserter(expanded));
                        expanded.push_back(*cit);  // include the rparen
                    } else {
                        ContainerT va_expanded;
                        if ((i == arguments.size()) ||                 // no variadic argument
                            impl::is_whitespace_only(arguments[i])) {  // no visible tokens
                            // no args; insert placemarker
                            va_expanded.push_back(
                                typename ContainerT::value_type(T_PLACEMARKER, "\xA7", pos));
                        } else if (!impl::is_blank_only(arguments[i])) {
                            // [cstart, cit) is now the args to va_opt
                            // recursively process them
                            expand_replacement_list(arg_start, cit, arguments,
                                                    expand_operator_defined,
                                                    expand_operator_has_include,
                                                    va_expanded);
                        }
                        // run final hooks
                        ctx.get_hooks().expanded_macro(ctx.derived(), va_expanded);

                        // updated overall expansion with va_opt results
                        expanded.splice(expanded.end(), va_expanded);
                    }
                    // continue from rparen
                }
                else

#endif
#endif
                {
                    BOOST_ASSERT(i < arguments.size());
                    // ensure argument i to be expanded
                    expand_argument(
                        i, arguments, expanded_args,
                        expand_operator_defined, expand_operator_has_include,
                        has_expanded_args);

                    // replace argument
                    BOOST_ASSERT(i < expanded_args.size());
                    ContainerT const& arg = expanded_args[i];

                    std::copy(arg.begin(), arg.end(),
                        std::inserter(expanded, expanded.end()));
                }
            }
            else if (adjacent_stringize &&
                    !IS_CATEGORY(*cit, WhiteSpaceTokenType))
            {
                // stringize the current argument
                BOOST_ASSERT(!arguments[i].empty());

                // safe a copy of the first tokens position (not a reference!)
                position_type pos((*arguments[i].begin()).get_position());

#if BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0
                if (is_ellipsis && boost::wave::need_variadics(ctx.get_language())) {
                    impl::trim_sequence_left(arguments[i]);
                    impl::trim_sequence_right(arguments.back());
                    expanded.push_back(token_type(T_STRINGLIT,
                        impl::as_stringlit(arguments, i, pos), pos));
                }
                else
#endif
                {
                    impl::trim_sequence(arguments[i]);
                    expanded.push_back(token_type(T_STRINGLIT,
                        impl::as_stringlit(arguments[i], pos), pos));
                }
                adjacent_stringize = false;
            }
            else {
                // simply copy the original argument (adjacent '##' or '#')
#if BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0
                if (is_ellipsis) {
                    position_type const& pos = (*cit).get_position();
#if BOOST_WAVE_SUPPORT_CPP2A != 0
                    if (i < arguments.size())
#endif
                    {

                        impl::trim_sequence_left(arguments[i]);
                        impl::trim_sequence_right(arguments.back());
                        BOOST_ASSERT(boost::wave::need_variadics(ctx.get_language()));
                        impl::replace_ellipsis(arguments, i, expanded, pos);
                    }
#if BOOST_WAVE_SUPPORT_CPP2A != 0
                    else if (boost::wave::need_cpp2a(ctx.get_language())) {
                        BOOST_ASSERT(i == arguments.size());
                        // no argument supplied; insert placemarker
                        expanded.push_back(
                            typename ContainerT::value_type(T_PLACEMARKER, "\xA7", pos));
                    }
#endif
                }
                else
#endif
                {
                    ContainerT& arg = arguments[i];

                    impl::trim_sequence(arg);
                    std::copy(arg.begin(), arg.end(),
                        std::inserter(expanded, expanded.end()));
                }
            }
        }
        else if (!adjacent_stringize || T_POUND != base_id) {
            // insert the actual replacement token (if it is not the '#' operator)
            expanded.push_back(*cit);
        }
    }

    if (adjacent_stringize) {
        // error, '#' should not be the last token
        BOOST_WAVE_THROW_CTX(ctx, preprocess_exception, ill_formed_operator,
            "stringize ('#')", main_pos);
        return;
    }

    // handle the cpp.concat operator
    if (seen_concat)
        concat_tokensequence(expanded);
}

///////////////////////////////////////////////////////////////////////////////
//
//  rescan_replacement_list
//
//    As the name implies, this function is used to rescan the replacement list
//    after the first macro substitution phase.
//
///////////////////////////////////////////////////////////////////////////////
template <typename ContextT>
template <typename IteratorT, typename ContainerT>
inline void
macromap<ContextT>::rescan_replacement_list(token_type const &curr_token,
    macro_definition_type &macro_def, ContainerT &replacement_list,
    ContainerT &expanded,
    bool expand_operator_defined,
    bool expand_operator_has_include,
    IteratorT &nfirst, IteratorT const &nlast)
{
    if (!replacement_list.empty()) {
#if BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0
        // remove the placemarkers
        if (boost::wave::need_variadics(ctx.get_language())) {
            typename ContainerT::iterator end = replacement_list.end();
            typename ContainerT::iterator it = replacement_list.begin();

            while (it != end) {
                using namespace boost::wave;
                if (T_PLACEMARKER == token_id(*it)) {
                    typename ContainerT::iterator placemarker = it;

                    ++it;
                    replacement_list.erase(placemarker);
                }
                else {
                    ++it;
                }
            }
        }
#endif

        // rescan the replacement list, during this rescan the current macro under
        // expansion isn't available as an expandable macro
        on_exit::reset<bool> on_exit(macro_def.is_available_for_replacement, false);
        typename ContainerT::iterator begin_it = replacement_list.begin();
        typename ContainerT::iterator end_it = replacement_list.end();

        expand_whole_tokensequence(
            expanded, begin_it, end_it,
            expand_operator_defined, expand_operator_has_include);

        // trim replacement list, leave placeholder tokens untouched
        impl::trim_replacement_list(expanded);
    }

    if (expanded.empty()) {
        // the resulting replacement list should contain at least a placeholder
        // token
        expanded.push_back(token_type(T_PLACEHOLDER, "_", curr_token.get_position()));
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  expand_macro(): expands a defined macro
//
//      This functions tries to expand the macro, to which points the 'first'
//      iterator. The functions eats up more tokens, if the macro to expand is
//      a function-like macro.
//
///////////////////////////////////////////////////////////////////////////////
template <typename ContextT>
template <typename IteratorT, typename ContainerT>
inline bool
macromap<ContextT>::expand_macro(ContainerT &expanded,
    token_type const &curr_token, typename defined_macros_type::iterator it,
    IteratorT &first, IteratorT const &last,
    bool& seen_newline, bool expand_operator_defined,
    bool expand_operator_has_include,
    boost::optional<position_type> expanding_pos,
    defined_macros_type *scope, ContainerT *queue_symbol)
{
    using namespace boost::wave;

    if (0 == scope) scope = current_macros;

    BOOST_ASSERT(T_IDENTIFIER == token_id(curr_token) ||
        IS_CATEGORY(token_id(curr_token), KeywordTokenType) ||
        IS_EXTCATEGORY(token_id(curr_token), OperatorTokenType|AltExtTokenType) ||
        IS_CATEGORY(token_id(curr_token), BoolLiteralTokenType));

    if (it == scope->end()) {
        ++first;    // advance

        // try to expand a predefined macro (__FILE__, __LINE__ or __INCLUDE_LEVEL__)
        if (expand_predefined_macro(curr_token, expanded))
            return false;

        // not defined as a macro
        if (0 != queue_symbol) {
            expanded.splice(expanded.end(), *queue_symbol);
        }
        else {
            expanded.push_back(curr_token);
        }
        return false;
    }

    // ensure the parameters to be replaced with special parameter tokens
    macro_definition_type& macro_def = *(*it).second.get();

    macro_def.replace_parameters(ctx);

    // test if this macro is currently available for replacement
    if (!macro_def.is_available_for_replacement) {
        // this macro is marked as non-replaceable
        // copy the macro name itself
        if (0 != queue_symbol) {
            queue_symbol->push_back(token_type(T_NONREPLACABLE_IDENTIFIER,
                curr_token.get_value(), curr_token.get_position()));
            expanded.splice(expanded.end(), *queue_symbol);
        }
        else {
            expanded.push_back(token_type(T_NONREPLACABLE_IDENTIFIER,
                curr_token.get_value(), curr_token.get_position()));
        }
        ++first;
        return false;
    }

    // try to replace the current identifier as a function-like macro
    ContainerT replacement_list;

    if (T_LEFTPAREN == impl::next_token<IteratorT>::peek(first, last)) {
        // called as a function-like macro
        impl::skip_to_token(ctx, first, last, T_LEFTPAREN, seen_newline);

        IteratorT seqstart = first;
        IteratorT seqend = first;

        if (macro_def.is_functionlike) {
            // defined as a function-like macro

            // collect the arguments
            std::vector<ContainerT> arguments;
            typename std::vector<ContainerT>::size_type count_args =
                collect_arguments(curr_token, arguments, first, seqend, last,
                    macro_def.macroparameters.size(), seen_newline);

            std::size_t parm_count_required = macro_def.macroparameters.size();
#if BOOST_WAVE_SUPPORT_CPP2A
            if (boost::wave::need_cpp2a(ctx.get_language())) {
                // Starting with C++20, variable arguments may be left out
                // entirely, so reduce the mandatory argument count by one
                // if the last parameter is ellipsis:
                if ((parm_count_required > 0) &&
                    (T_ELLIPSIS == token_id(macro_def.macroparameters.back()))) {
                    --parm_count_required;
                }
            }
#endif

            // verify the parameter count
            if (count_args < parm_count_required ||
                arguments.size() < parm_count_required)
            {
                if (count_args != arguments.size()) {
                    // must been at least one empty argument in C++ mode
                    BOOST_WAVE_THROW_CTX(ctx, preprocess_exception,
                        empty_macroarguments, curr_token.get_value().c_str(),
                        main_pos);
                }
                else {
                    // too few macro arguments
                    BOOST_WAVE_THROW_CTX(ctx, preprocess_exception,
                        too_few_macroarguments, curr_token.get_value().c_str(),
                        main_pos);
                }
                return false;
            }

            if (count_args > macro_def.macroparameters.size() ||
                arguments.size() > macro_def.macroparameters.size())
            {
#if BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0
                if (!macro_def.has_ellipsis)
#endif
                {
                    // too many macro arguments
                    BOOST_WAVE_THROW_CTX(ctx, preprocess_exception,
                        too_many_macroarguments,
                        curr_token.get_value().c_str(), main_pos);
                    return false;
                }
            }

            // inject tracing support
            if (ctx.get_hooks().expanding_function_like_macro(ctx.derived(),
                    macro_def.macroname, macro_def.macroparameters,
                    macro_def.macrodefinition, curr_token, arguments,
                    seqstart, seqend))
            {
//                 // do not expand this macro, just copy the whole sequence
//                 expanded.push_back(curr_token);
//                 std::copy(seqstart, first,
//                     std::inserter(expanded, expanded.end()));
                // do not expand macro, just copy macro name and parenthesis
                expanded.push_back(curr_token);
                expanded.push_back(*seqstart);
                first = ++seqstart;
                return false;           // no further preprocessing required
            }

            // expand the replacement list of this macro
            expand_replacement_list(macro_def.macrodefinition.begin(),
                macro_def.macrodefinition.end(),
                arguments, expand_operator_defined,
                expand_operator_has_include,
                replacement_list);

            if (!expanding_pos)
                expanding_pos = curr_token.get_expand_position();
            set_expand_positions(replacement_list, *expanding_pos);
        }
        else {
            // defined as an object-like macro
            if (ctx.get_hooks().expanding_object_like_macro(ctx.derived(),
                  macro_def.macroname, macro_def.macrodefinition, curr_token))
            {
                // do not expand this macro, just copy the whole sequence
                expanded.push_back(curr_token);
                return false;           // no further preprocessing required
            }

            bool found = false;
            impl::find_concat_operator concat_tag(found);

            std::remove_copy_if(macro_def.macrodefinition.begin(),
                macro_def.macrodefinition.end(),
                std::inserter(replacement_list, replacement_list.end()),
                concat_tag);

            // handle concatenation operators
            if (found && !concat_tokensequence(replacement_list))
                return false;
        }
    }
    else {
        // called as an object like macro
        if ((*it).second->is_functionlike) {
            // defined as a function-like macro
            if (0 != queue_symbol) {
                queue_symbol->push_back(curr_token);
                expanded.splice(expanded.end(), *queue_symbol);
            }
            else {
                expanded.push_back(curr_token);
            }
            ++first;                // skip macro name
            return false;           // no further preprocessing required
        }
        else {
            // defined as an object-like macro (expand it)
            if (ctx.get_hooks().expanding_object_like_macro(ctx.derived(),
                  macro_def.macroname, macro_def.macrodefinition, curr_token))
            {
                // do not expand this macro, just copy the whole sequence
                expanded.push_back(curr_token);
                ++first;                // skip macro name
                return false;           // no further preprocessing required
            }

            bool found = false;
            impl::find_concat_operator concat_tag(found);

            std::remove_copy_if(macro_def.macrodefinition.begin(),
                macro_def.macrodefinition.end(),
                std::inserter(replacement_list, replacement_list.end()),
                concat_tag);

            // handle concatenation operators
            if (found && !concat_tokensequence(replacement_list))
                return false;

            ++first;                // skip macro name
        }
    }

    // rescan the replacement list
    ContainerT expanded_list;

    ctx.get_hooks().expanded_macro(ctx.derived(), replacement_list);

    rescan_replacement_list(
        curr_token, macro_def, replacement_list,
        expanded_list, expand_operator_defined,
        expand_operator_has_include, first, last);

    ctx.get_hooks().rescanned_macro(ctx.derived(), expanded_list);

    if (!expanding_pos)
        // set the expanding position for rescan
        expanding_pos = curr_token.get_expand_position();

    // record the location where all the tokens were expanded from
    set_expand_positions(expanded_list, *expanding_pos);

    expanded.splice(expanded.end(), expanded_list);
    return true;        // rescan is required
}

///////////////////////////////////////////////////////////////////////////////
//
//  If the token under inspection points to a certain predefined macro it will
//  be expanded, otherwise false is returned.
//  (only __FILE__, __LINE__ and __INCLUDE_LEVEL__ macros are expanded here)
//
///////////////////////////////////////////////////////////////////////////////
template <typename ContextT>
template <typename ContainerT>
inline bool
macromap<ContextT>::expand_predefined_macro(token_type const &curr_token,
    ContainerT &expanded)
{
    using namespace boost::wave;

    string_type const& value = curr_token.get_value();

    if ((value != "__LINE__") && (value != "__FILE__") && (value != "__INCLUDE_LEVEL__"))
        return false;

    // construct a fake token for the macro's definition point
    token_type deftoken(T_IDENTIFIER, value, position_type("<built-in>"));

    if (ctx.get_hooks().expanding_object_like_macro(ctx.derived(),
            deftoken, ContainerT(), curr_token))
    {
        // do not expand this macro, just copy the whole sequence
        expanded.push_back(curr_token);
        return false;           // no further preprocessing required
    }

    token_type replacement;

    if (value == "__LINE__") {
        // expand the __LINE__ macro
        std::string buffer = lexical_cast<std::string>(curr_token.get_expand_position().get_line());

        replacement = token_type(T_INTLIT, buffer.c_str(), curr_token.get_position());
    }
    else if (value == "__FILE__") {
        // expand the __FILE__ macro
        namespace fs = boost::filesystem;

        std::string file("\"");
        fs::path filename(
            wave::util::create_path(curr_token.get_expand_position().get_file().c_str()));

        using boost::wave::util::impl::escape_lit;
        file += escape_lit(wave::util::native_file_string(filename)) + "\"";
        replacement = token_type(T_STRINGLIT, file.c_str(),
            curr_token.get_position());
    }
    else if (value == "__INCLUDE_LEVEL__") {
        // expand the __INCLUDE_LEVEL__ macro
        char buffer[22]; // 21 bytes holds all NUL-terminated unsigned 64-bit numbers

        using namespace std;    // for some systems sprintf is in namespace std
        sprintf(buffer, "%d", (int)ctx.get_iteration_depth());
        replacement = token_type(T_INTLIT, buffer, curr_token.get_position());
    }

    // post-expansion hooks
    ContainerT replacement_list;
    replacement_list.push_back(replacement);

    ctx.get_hooks().expanded_macro(ctx.derived(), replacement_list);

    expanded.push_back(replacement);

    ctx.get_hooks().rescanned_macro(ctx.derived(), expanded);

    return true;

}

///////////////////////////////////////////////////////////////////////////////
//
//  resolve_defined(): resolve the operator defined() and replace it with the
//                     correct T_INTLIT token
//
///////////////////////////////////////////////////////////////////////////////
template <typename ContextT>
template <typename IteratorT, typename ContainerT>
inline typename ContextT::token_type const &
macromap<ContextT>::resolve_defined(IteratorT &first,
    IteratorT const &last, ContainerT &pending)
{
    using namespace boost::wave;
    using namespace boost::wave::grammars;

ContainerT result;
IteratorT start = first;
boost::spirit::classic::parse_info<IteratorT> hit =
    defined_grammar_gen<typename ContextT::lexer_type>::
        parse_operator_defined(start, last, result);

    if (!hit.hit) {
        string_type msg ("defined(): ");
        msg = msg + util::impl::as_string<string_type>(first, last);
        BOOST_WAVE_THROW_CTX(ctx, preprocess_exception, ill_formed_expression,
            msg.c_str(), main_pos);

        // insert a dummy token
        pending.push_back(token_type(T_INTLIT, "0", main_pos));
    }
    else {
        impl::assign_iterator<IteratorT>::do_(first, hit.stop);

        // insert a token, which reflects the outcome
        pending.push_back(token_type(T_INTLIT,
            is_defined(result.begin(), result.end()) ? "1" : "0",
            main_pos));
    }

    on_exit::pop_front<definition_container_type> pop_front_token(pending);

    return act_token = pending.front();
}

#if BOOST_WAVE_SUPPORT_HAS_INCLUDE != 0
///////////////////////////////////////////////////////////////////////////////
//
//  resolve_has_include(): resolve the operator __has_include() and replace
//                      it with the correct T_INTLIT token
//
///////////////////////////////////////////////////////////////////////////////
template <typename ContextT>
template <typename IteratorT, typename ContainerT>
inline typename ContextT::token_type const &
macromap<ContextT>::resolve_has_include(IteratorT &first,
    IteratorT const &last, ContainerT &pending)
{
    using namespace boost::wave;
    using namespace boost::wave::grammars;

    ContainerT result;
    bool is_quoted_filename;
    bool is_system;

    // to simplify the parser we check for the trailing right paren first
    // scan from the beginning because unput_queue_iterator is Forward
    IteratorT end_find_it = first;
    ++end_find_it;
    IteratorT rparen_it = first;
    while (end_find_it != last) {
        ++end_find_it;
        ++rparen_it;
    }

    boost::spirit::classic::parse_info<IteratorT> hit(first);
    if ((rparen_it != first) && (T_RIGHTPAREN == *rparen_it)) {
        IteratorT start = first;
        hit = has_include_grammar_gen<typename ContextT::lexer_type>::
            parse_operator_has_include(start, rparen_it, result, is_quoted_filename, is_system);
    }

    if (!hit.hit) {
        string_type msg ("__has_include(): ");
        msg = msg + util::impl::as_string<string_type>(first, last);
        BOOST_WAVE_THROW_CTX(ctx, preprocess_exception, ill_formed_expression,
            msg.c_str(), main_pos);

        // insert a dummy token
        pending.push_back(token_type(T_INTLIT, "0", main_pos));
    }
    else {
        impl::assign_iterator<IteratorT>::do_(first, last);

        // insert a token, which reflects the outcome
        pending.push_back(
            token_type(T_INTLIT,
                       has_include(result.begin(), result.end(),
                                   is_quoted_filename, is_system) ? "1" : "0",
                       main_pos));
    }

    on_exit::pop_front<definition_container_type> pop_front_token(pending);

    return act_token = pending.front();
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  resolve_operator_pragma(): resolve the operator _Pragma() and dispatch to
//                             the associated action
//
//      This function returns true, if the pragma was correctly interpreted.
//      The iterator 'first' is positioned behind the closing ')'.
//      This function returns false, if the _Pragma was not known, the
//      preprocessed token sequence is pushed back to the 'pending' sequence.
//
///////////////////////////////////////////////////////////////////////////////
template <typename ContextT>
template <typename IteratorT, typename ContainerT>
inline bool
macromap<ContextT>::resolve_operator_pragma(IteratorT &first,
    IteratorT const &last, ContainerT &pending, bool& seen_newline)
{
    // isolate the parameter of the operator _Pragma
    token_type pragma_token = *first;

    if (!impl::skip_to_token(ctx, first, last, T_LEFTPAREN, seen_newline)) {
        // illformed operator _Pragma
        BOOST_WAVE_THROW_CTX(ctx, preprocess_exception, ill_formed_expression,
            "operator _Pragma()", pragma_token.get_position());
        return false;
    }

    std::vector<ContainerT> arguments;
    IteratorT endparen = first;
    typename std::vector<ContainerT>::size_type count_args =
        collect_arguments (pragma_token, arguments, first, endparen, last, 1,
            seen_newline);

    // verify the parameter count
    if (pragma_token.get_position().get_file().empty())
        pragma_token.set_position(act_token.get_position());

    if (count_args < 1 || arguments.size() < 1) {
        // too few macro arguments
        BOOST_WAVE_THROW_CTX(ctx, preprocess_exception, too_few_macroarguments,
            pragma_token.get_value().c_str(), pragma_token.get_position());
        return false;
    }
    if (count_args > 1 || arguments.size() > 1) {
        // too many macro arguments
        BOOST_WAVE_THROW_CTX(ctx, preprocess_exception, too_many_macroarguments,
            pragma_token.get_value().c_str(), pragma_token.get_position());
        return false;
    }

    // preprocess the pragma token body
    typedef typename std::vector<ContainerT>::value_type::iterator
        argument_iterator_type;

    ContainerT expanded;
    argument_iterator_type begin_it = arguments[0].begin();
    argument_iterator_type end_it = arguments[0].end();
    expand_whole_tokensequence(expanded, begin_it, end_it, false, false);

    // un-escape the parameter of the operator _Pragma
    typedef typename token_type::string_type string_type;

    string_type pragma_cmd;
    typename ContainerT::const_iterator end_exp = expanded.end();
    for (typename ContainerT::const_iterator it_exp = expanded.begin();
         it_exp != end_exp; ++it_exp)
    {
        if (T_EOF == token_id(*it_exp))
            break;
        if (IS_CATEGORY(*it_exp, WhiteSpaceTokenType))
            continue;

        if (T_STRINGLIT != token_id(*it_exp)) {
            // ill formed operator _Pragma
            BOOST_WAVE_THROW_CTX(ctx, preprocess_exception,
                ill_formed_pragma_option, "_Pragma",
                pragma_token.get_position());
            return false;
        }
        if (pragma_cmd.size() > 0) {
            // there should be exactly one string literal (string literals are to
            // be concatenated at translation phase 6, but _Pragma operators are
            // to be executed at translation phase 4)
            BOOST_WAVE_THROW_CTX(ctx, preprocess_exception,
                ill_formed_pragma_option, "_Pragma",
                pragma_token.get_position());
            return false;
        }

        // remove the '\"' and concat all given string literal-values
        string_type token_str = (*it_exp).get_value();
        pragma_cmd += token_str.substr(1, token_str.size() - 2);
    }
    string_type pragma_cmd_unesc = impl::unescape_lit(pragma_cmd);

    // tokenize the pragma body
    typedef typename ContextT::lexer_type lexer_type;

    ContainerT pragma;
    std::string pragma_cmd_str(pragma_cmd_unesc.c_str());
    lexer_type it = lexer_type(pragma_cmd_str.begin(), pragma_cmd_str.end(),
        pragma_token.get_position(), ctx.get_language());
    lexer_type end = lexer_type();
    for (/**/; it != end; ++it)
        pragma.push_back(*it);

    // analyze the preprocessed token sequence and eventually dispatch to the
    // associated action
    if (interpret_pragma(ctx, pragma_token, pragma.begin(), pragma.end(),
        pending))
    {
        return true;    // successfully recognized a wave specific pragma
    }

    // unknown pragma token sequence, push it back and return to the caller
    pending.push_front(token_type(T_SPACE, " ", pragma_token.get_position()));
    pending.push_front(token_type(T_RIGHTPAREN, ")", pragma_token.get_position()));
    pending.push_front(token_type(T_STRINGLIT, string_type("\"") + pragma_cmd + "\"",
        pragma_token.get_position()));
    pending.push_front(token_type(T_LEFTPAREN, "(", pragma_token.get_position()));
    pending.push_front(pragma_token);
    return false;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Test, whether the result of a concat operator is well formed or not.
//
//  This is done by re-scanning (re-tokenizing) the resulting token sequence,
//  which should give back exactly one token.
//
///////////////////////////////////////////////////////////////////////////////
template <typename ContextT>
template <typename ContainerT>
inline bool
macromap<ContextT>::is_valid_concat(string_type new_value,
    position_type const &pos, ContainerT &rescanned)
{
    // re-tokenize the newly generated string
    typedef typename ContextT::lexer_type lexer_type;

    std::string value_to_test(new_value.c_str());

    boost::wave::language_support lang =
        boost::wave::enable_prefer_pp_numbers(ctx.get_language());
    lang = boost::wave::enable_single_line(lang);

    lexer_type it = lexer_type(value_to_test.begin(), value_to_test.end(), pos,
        lang);
    lexer_type end = lexer_type();
    for (/**/; it != end && T_EOF != token_id(*it); ++it)
    {
        // as of Wave V2.0.7 pasting of tokens is valid only if the resulting
        // tokens are pp_tokens (as mandated by C++11)
        if (!is_pp_token(*it))
            return false;
        rescanned.push_back(*it);
    }

#if BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0
    if (boost::wave::need_variadics(ctx.get_language()))
        return true;       // in variadics mode token pasting is well defined
#endif

    // test if the newly generated token sequence contains more than 1 token
    return 1 == rescanned.size();
}

///////////////////////////////////////////////////////////////////////////////
//
//  Bulk update expand positions
//
///////////////////////////////////////////////////////////////////////////////
// batch update tokens with a single expand position
template <typename ContextT>
template <typename ContainerT>
void macromap<ContextT>::set_expand_positions(ContainerT &tokens, position_type pos)
{
    typename ContainerT::iterator ex_end = tokens.end();
    for (typename ContainerT::iterator it = tokens.begin();
         it != ex_end; ++it) {
        // expand positions are only used for __LINE__, __FILE__, and macro names
        if (token_id(*it) == T_IDENTIFIER)
            it->set_expand_position(pos);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  Handle all occurrences of the concatenation operator '##' inside the given
//  token sequence.
//
///////////////////////////////////////////////////////////////////////////////
template <typename Context>
inline void report_invalid_concatenation(Context& ctx,
    typename Context::token_type const& prev,
    typename Context::token_type const& next,
    typename Context::position_type const& main_pos)
{
    typename Context::string_type error_string("\"");

    error_string += prev.get_value();
    error_string += "\" and \"";
    error_string += next.get_value();
    error_string += "\"";
    BOOST_WAVE_THROW_CTX(ctx, preprocess_exception, invalid_concat,
        error_string.c_str(), main_pos);
}

template <typename ContextT>
template <typename ContainerT>
inline bool
macromap<ContextT>::concat_tokensequence(ContainerT &expanded)
{
    using namespace boost::wave;
    typedef typename ContainerT::iterator iterator_type;

    iterator_type end = expanded.end();
    iterator_type prev = end;
    for (iterator_type it = expanded.begin(); it != end; /**/)
    {
        if (T_POUND_POUND == BASE_TOKEN(token_id(*it))) {
            iterator_type next = it;

            ++next;
            if (prev == end || next == end) {
                // error, '##' should be in between two tokens
                BOOST_WAVE_THROW_CTX(ctx, preprocess_exception,
                    ill_formed_operator, "concat ('##')", main_pos);
                return false;
            }

            // replace prev##next with the concatenated value, skip whitespace
            // before and after the '##' operator
            while (IS_CATEGORY(*next, WhiteSpaceTokenType)) {
                ++next;
                if (next == end) {
                    // error, '##' should be in between two tokens
                    BOOST_WAVE_THROW_CTX(ctx, preprocess_exception,
                        ill_formed_operator, "concat ('##')", main_pos);
                    return false;
                }
            }

#if BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0
            if (boost::wave::need_variadics(ctx.get_language())) {
                if (T_PLACEMARKER == token_id(*next)) {
                    // remove the '##' and the next tokens from the sequence
                    iterator_type first_to_delete = prev;

                    expanded.erase(++first_to_delete, ++next);
                    it = next;
                    continue;
                }
                else if (T_PLACEMARKER == token_id(*prev)) {
                    // remove the '##' and the next tokens from the sequence
                    iterator_type first_to_delete = prev;

                    *prev = *next;
                    expanded.erase(++first_to_delete, ++next);
                    it = next;
                    continue;
                }
            }
#endif // BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0

            // test if the concat operator has to concatenate two unrelated
            // tokens i.e. the result yields more then one token
            string_type concat_result;
            ContainerT rescanned;

            concat_result = ((*prev).get_value() + (*next).get_value());

            // analyze the validity of the concatenation result
            if (!is_valid_concat(concat_result, (*prev).get_position(),
                    rescanned) &&
                !IS_CATEGORY(*prev, WhiteSpaceTokenType) &&
                !IS_CATEGORY(*next, WhiteSpaceTokenType))
            {
                report_invalid_concatenation(ctx, *prev, *next, main_pos);
                return false;
            }

#if BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0
            if (boost::wave::need_variadics(ctx.get_language())) {
                // remove the prev, '##' and the next tokens from the sequence
                expanded.erase(prev, ++next); // remove not needed tokens

                // some stl implementations clear() the container if we erased all
                // the elements, which orphans all iterators. we re-initialize these
                // here
                if (expanded.empty())
                    end = next = expanded.end();

                // replace the old token (pointed to by *prev) with the re-tokenized
                // sequence
                expanded.splice(next, rescanned);

                // the last token of the inserted sequence is the new previous
                prev = next;
                if (next != expanded.end())
                    --prev;
            }
            else
#endif // BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0
            {
                // we leave the token_id unchanged, but unmark the token as
                // disabled, if appropriate
                (*prev).set_value(concat_result);
                if (T_NONREPLACABLE_IDENTIFIER == token_id(*prev))
                    (*prev).set_token_id(T_IDENTIFIER);

                // remove the '##' and the next tokens from the sequence
                iterator_type first_to_delete = prev;

                expanded.erase(++first_to_delete, ++next);
            }
            it = next;
            continue;
        }

        // save last non-whitespace token position
        if (!IS_CATEGORY(*it, WhiteSpaceTokenType))
            prev = it;

        ++it;           // next token, please
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
//
//  predefine_macro(): predefine a single macro
//
///////////////////////////////////////////////////////////////////////////////
template <typename ContextT>
inline void
macromap<ContextT>::predefine_macro(defined_macros_type *scope,
    string_type const &name, token_type const &t)
{
definition_container_type macrodefinition;
std::vector<token_type> param;

    macrodefinition.push_back(t);
    add_macro(token_type(T_IDENTIFIER, name, t.get_position()),
        false, param, macrodefinition, true, scope);
}

///////////////////////////////////////////////////////////////////////////////
//
//  init_predefined_macros(): init the predefined macros
//
///////////////////////////////////////////////////////////////////////////////
template <typename ContextT>
inline void
macromap<ContextT>::init_predefined_macros(char const *fname,
    defined_macros_type *scope, bool at_global_scope)
{
    // if no scope is given, use the current one
    defined_macros_type* current_scope = scope ? scope : current_macros;

    // first, add the static macros
    position_type pos("<built-in>");

#if BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0
    if (boost::wave::need_c99(ctx.get_language())) {
        // define C99 specifics
        for (int i = 0; 0 != predef.static_data_c99(i).name; ++i) {
            predefined_macros::static_macros const& m = predef.static_data_c99(i);
            predefine_macro(current_scope, m.name,
                token_type(m.token_id, m.value, pos));
        }
    }
    else
#endif
    {
#if BOOST_WAVE_SUPPORT_CPP0X != 0
        if (boost::wave::need_cpp0x(ctx.get_language())) {
            // define C++11 specifics
            for (int i = 0; 0 != predef.static_data_cpp0x(i).name; ++i) {
                predefined_macros::static_macros const& m = predef.static_data_cpp0x(i);
                predefine_macro(current_scope, m.name,
                    token_type(m.token_id, m.value, pos));
            }
        }
        else
#endif
#if BOOST_WAVE_SUPPORT_CPP2A != 0
            if (boost::wave::need_cpp2a(ctx.get_language())) {
            // define C++20 specifics
            for (int i = 0; 0 != predef.static_data_cpp2a(i).name; ++i) {
                predefined_macros::static_macros const& m = predef.static_data_cpp2a(i);
                predefine_macro(current_scope, m.name,
                    token_type(m.token_id, m.value, pos));
            }
        }
        else
#endif
        {
            // define C++ specifics
            for (int i = 0; 0 != predef.static_data_cpp(i).name; ++i) {
                predefined_macros::static_macros const& m = predef.static_data_cpp(i);
                predefine_macro(current_scope, m.name,
                    token_type(m.token_id, m.value, pos));
            }

#if BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0
            // define __WAVE_HAS_VARIADICS__, if appropriate
            if (boost::wave::need_variadics(ctx.get_language())) {
                predefine_macro(current_scope, "__WAVE_HAS_VARIADICS__",
                    token_type(T_INTLIT, "1", pos));
            }
#endif
        }
    }

    // predefine the __BASE_FILE__ macro which contains the main file name
    namespace fs = boost::filesystem;
    if (string_type(fname) != "<Unknown>") {
        fs::path filename(create_path(fname));

        using boost::wave::util::impl::escape_lit;
        predefine_macro(current_scope, "__BASE_FILE__",
            token_type(T_STRINGLIT, string_type("\"") +
                escape_lit(native_file_string(filename)).c_str() + "\"", pos));
        base_name = fname;
    }
    else if (!base_name.empty()) {
        fs::path filename(create_path(base_name.c_str()));

        using boost::wave::util::impl::escape_lit;
        predefine_macro(current_scope, "__BASE_FILE__",
            token_type(T_STRINGLIT, string_type("\"") +
                escape_lit(native_file_string(filename)).c_str() + "\"", pos));
    }

    // now add the dynamic macros
    for (int j = 0; 0 != predef.dynamic_data(j).name; ++j) {
        predefined_macros::dynamic_macros const& m = predef.dynamic_data(j);
        predefine_macro(current_scope, m.name,
            token_type(m.token_id, (predef.* m.generator)(), pos));
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  reset_macromap(): initialize the internal macro symbol namespace
//
///////////////////////////////////////////////////////////////////////////////
template <typename ContextT>
inline void
macromap<ContextT>::reset_macromap()
{
    current_macros->clear();
    predef.reset();
    act_token = token_type();
}

///////////////////////////////////////////////////////////////////////////////
}}}   // namespace boost::wave::util

#if BOOST_WAVE_SERIALIZATION != 0
namespace boost { namespace serialization {

template<typename ContextT>
struct version<boost::wave::util::macromap<ContextT> >
{
    typedef boost::wave::util::macromap<ContextT> target_type;
    typedef mpl::int_<target_type::version> type;
    typedef mpl::integral_c_tag tag;
    BOOST_STATIC_CONSTANT(unsigned int, value = version::type::value);
};

}}    // namespace boost::serialization
#endif

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !defined(BOOST_CPP_MACROMAP_HPP_CB8F51B0_A3F0_411C_AEF4_6FF631B8B414_INCLUDED)

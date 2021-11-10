//  Copyright (c) 2008-2009 Ben Hanson
//  Copyright (c) 2008-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEX_LEXERTL_GENERATE_CPP_FEB_10_2008_0855PM)
#define BOOST_SPIRIT_LEX_LEXERTL_GENERATE_CPP_FEB_10_2008_0855PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/detail/lexer/char_traits.hpp>
#include <boost/spirit/home/support/detail/lexer/consts.hpp>
#include <boost/spirit/home/support/detail/lexer/rules.hpp>
#include <boost/spirit/home/support/detail/lexer/size_t.hpp>
#include <boost/spirit/home/support/detail/lexer/state_machine.hpp>
#include <boost/spirit/home/support/detail/lexer/debug.hpp>
#include <boost/spirit/home/lex/lexer/lexertl/static_version.hpp>
#include <boost/scoped_array.hpp>
#include <locale>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace lex { namespace lexertl
{
    namespace detail
    {

    ///////////////////////////////////////////////////////////////////////////
    template <typename CharT>
    struct string_lit;

    template <>
    struct string_lit<char>
    {
        static char get(char c) { return c; }
        static std::string get(char const* str = "") { return str; }
    };

    template <>
    struct string_lit<wchar_t>
    {
        static wchar_t get(char c)
        {
            typedef std::ctype<wchar_t> ctype_t;
            return std::use_facet<ctype_t>(std::locale()).widen(c);
        }
        static std::basic_string<wchar_t> get(char const* source = "")
        {
            using namespace std;        // some systems have size_t in ns std
            size_t len = strlen(source);
            boost::scoped_array<wchar_t> result (new wchar_t[len+1]);
            result.get()[len] = '\0';

            // working with wide character streams is supported only if the
            // platform provides the std::ctype<wchar_t> facet
            BOOST_ASSERT(std::has_facet<std::ctype<wchar_t> >(std::locale()));

            std::use_facet<std::ctype<wchar_t> >(std::locale())
                .widen(source, source + len, result.get());
            return result.get();
        }
    };

    template <typename Char>
    inline Char L(char c)
    {
        return string_lit<Char>::get(c);
    }

    template <typename Char>
    inline std::basic_string<Char> L(char const* c = "")
    {
        return string_lit<Char>::get(c);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Char>
    inline bool
    generate_delimiter(std::basic_ostream<Char> &os_)
    {
        os_ << std::basic_string<Char>(80, '/') << "\n";
        return os_.good();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Generate a table of the names of the used lexer states, which is a bit
    // tricky, because the table stored with the rules is sorted based on the
    // names, but we need it sorted using the state ids.
    template <typename Char>
    inline bool
    generate_cpp_state_info (boost::lexer::basic_rules<Char> const& rules_
      , std::basic_ostream<Char> &os_, Char const* name_suffix)
    {
        // we need to re-sort the state names in ascending order of the state
        // ids, filling possible gaps in between later
        typedef typename
            boost::lexer::basic_rules<Char>::string_size_t_map::const_iterator
        state_iterator;
        typedef std::map<std::size_t, Char const*> reverse_state_map_type;

        reverse_state_map_type reverse_state_map;
        state_iterator send = rules_.statemap().end();
        for (state_iterator sit = rules_.statemap().begin(); sit != send; ++sit)
        {
            typedef typename reverse_state_map_type::value_type value_type;
            reverse_state_map.insert(value_type((*sit).second, (*sit).first.c_str()));
        }

        generate_delimiter(os_);
        os_ << "// this table defines the names of the lexer states\n";
        os_ << boost::lexer::detail::strings<Char>::char_name()
            << " const* const lexer_state_names"
            << (name_suffix[0] ? "_" : "") << name_suffix
            << "[" << rules_.statemap().size() << "] = \n{\n";

        typedef typename reverse_state_map_type::iterator iterator;
        iterator rend = reverse_state_map.end();
        std::size_t last_id = 0;
        for (iterator rit = reverse_state_map.begin(); rit != rend; ++last_id)
        {
            for (/**/; last_id < (*rit).first; ++last_id)
            {
                os_ << "    0,  // \"<undefined state>\"\n";
            }
            os_ << "    "
                << boost::lexer::detail::strings<Char>::char_prefix()
                << "\"" << (*rit).second << "\"";
            if (++rit != rend)
                os_ << ",\n";
            else
                os_ << "\n";        // don't generate the final comma
        }
        os_ << "};\n\n";

        generate_delimiter(os_);
        os_ << "// this variable defines the number of lexer states\n";
        os_ << "std::size_t const lexer_state_count"
            << (name_suffix[0] ? "_" : "") << name_suffix
            << " = " << rules_.statemap().size() << ";\n\n";
        return os_.good();
    }

    template <typename Char>
    inline bool
    generate_cpp_state_table (std::basic_ostream<Char> &os_
      , Char const* name_suffix, bool bol, bool eol)
    {
        std::basic_string<Char> suffix(L<Char>(name_suffix[0] ? "_" : ""));
        suffix += name_suffix;

        generate_delimiter(os_);
        os_ << "// this defines a generic accessors for the information above\n";
        os_ << "struct lexer" << suffix << "\n{\n";
        os_ << "    // version number and feature-set of compatible static lexer engine\n";
        os_ << "    enum\n";
        os_ << "    {\n        static_version = " << SPIRIT_STATIC_LEXER_VERSION << ",\n";
        os_ << "        supports_bol = " << std::boolalpha << bol << ",\n";
        os_ << "        supports_eol = " << std::boolalpha << eol << "\n";
        os_ << "    };\n\n";
        os_ << "    // return the number of lexer states\n";
        os_ << "    static std::size_t state_count()\n";
        os_ << "    {\n        return lexer_state_count" << suffix << "; \n    }\n\n";
        os_ << "    // return the name of the lexer state as given by 'idx'\n";
        os_ << "    static " << boost::lexer::detail::strings<Char>::char_name()
            << " const* state_name(std::size_t idx)\n";
        os_ << "    {\n        return lexer_state_names" << suffix << "[idx]; \n    }\n\n";
        os_ << "    // return the next matched token\n";
        os_ << "    template<typename Iterator>\n";
        os_ << "    static std::size_t next(std::size_t &start_state_, bool& bol_\n";
        os_ << "      , Iterator &start_token_, Iterator const& end_, std::size_t& unique_id_)\n";
        os_ << "    {\n        return next_token" << suffix
            << "(start_state_, bol_, start_token_, end_, unique_id_);\n    }\n";
        os_ << "};\n\n";
        return os_.good();
    }

    ///////////////////////////////////////////////////////////////////////////
    // generate function body based on traversing the DFA tables
    template <typename Char>
    bool generate_function_body_dfa(std::basic_ostream<Char>& os_
      , boost::lexer::basic_state_machine<Char> const &sm_)
    {
        std::size_t const dfas_ = sm_.data()._dfa->size();
        std::size_t const lookups_ = sm_.data()._lookup->front()->size();

        os_ << "    enum {end_state_index, id_index, unique_id_index, "
               "state_index, bol_index,\n";
        os_ << "        eol_index, dead_state_index, dfa_offset};\n\n";
        os_ << "    static std::size_t const npos = "
               "static_cast<std::size_t>(~0);\n";

        if (dfas_ > 1)
        {
            for (std::size_t state_ = 0; state_ < dfas_; ++state_)
            {
                std::size_t i_ = 0;
                std::size_t j_ = 1;
                std::size_t count_ = lookups_ / 8;
                std::size_t const* lookup_ = &sm_.data()._lookup[state_]->front();
                std::size_t const* dfa_ = &sm_.data()._dfa[state_]->front();

                os_ << "    static std::size_t const lookup" << state_
                    << "_[" << lookups_ << "] = {\n        ";
                for (/**/; i_ < count_; ++i_)
                {
                    std::size_t const index_ = i_ * 8;
                    os_ << lookup_[index_];
                    for (/**/; j_ < 8; ++j_)
                    {
                        os_ << ", " << lookup_[index_ + j_];
                    }
                    if (i_ < count_ - 1)
                    {
                        os_ << ",\n        ";
                    }
                    j_ = 1;
                }
                os_ << " };\n";

                count_ = sm_.data()._dfa[state_]->size ();
                os_ << "    static const std::size_t dfa" << state_ << "_["
                    << count_ << "] = {\n        ";
                count_ /= 8;
                for (i_ = 0; i_ < count_; ++i_)
                {
                    std::size_t const index_ = i_ * 8;
                    os_ << dfa_[index_];
                    for (j_ = 1; j_ < 8; ++j_)
                    {
                        os_ << ", " << dfa_[index_ + j_];
                    }
                    if (i_ < count_ - 1)
                    {
                        os_ << ",\n        ";
                    }
                }

                std::size_t const mod_ = sm_.data()._dfa[state_]->size () % 8;
                if (mod_)
                {
                    std::size_t const index_ = count_ * 8;
                    if (count_)
                    {
                        os_ << ",\n        ";
                    }
                    os_ << dfa_[index_];
                    for (j_ = 1; j_ < mod_; ++j_)
                    {
                        os_ << ", " << dfa_[index_ + j_];
                    }
                }
                os_ << " };\n";
            }

            std::size_t count_ = sm_.data()._dfa_alphabet.size();
            std::size_t i_ = 1;

            os_ << "    static std::size_t const* lookup_arr_[" << count_
                << "] = { lookup0_";
            for (i_ = 1; i_ < count_; ++i_)
            {
                os_ << ", " << "lookup" << i_ << "_";
            }
            os_ << " };\n";

            os_ << "    static std::size_t const dfa_alphabet_arr_["
                << count_ << "] = { ";
            os_ << sm_.data()._dfa_alphabet.front ();
            for (i_ = 1; i_ < count_; ++i_)
            {
                os_ << ", " << sm_.data()._dfa_alphabet[i_];
            }
            os_ << " };\n";

            os_ << "    static std::size_t const* dfa_arr_[" << count_
                << "] = { ";
            os_ << "dfa0_";
            for (i_ = 1; i_ < count_; ++i_)
            {
                os_ << ", " << "dfa" << i_ << "_";
            }
            os_ << " };\n";
        }
        else
        {
            std::size_t const* lookup_ = &sm_.data()._lookup[0]->front();
            std::size_t const* dfa_ = &sm_.data()._dfa[0]->front();
            std::size_t i_ = 0;
            std::size_t j_ = 1;
            std::size_t count_ = lookups_ / 8;

            os_ << "    static std::size_t const lookup_[";
            os_ << sm_.data()._lookup[0]->size() << "] = {\n        ";
            for (/**/; i_ < count_; ++i_)
            {
                const std::size_t index_ = i_ * 8;
                os_ << lookup_[index_];
                for (/**/; j_ < 8; ++j_)
                {
                    os_ << ", " << lookup_[index_ + j_];
                }
                if (i_ < count_ - 1)
                {
                    os_ << ",\n        ";
                }
                j_ = 1;
            }
            os_ << " };\n";

            os_ << "    static std::size_t const dfa_alphabet_ = "
                << sm_.data()._dfa_alphabet.front () << ";\n";
            os_ << "    static std::size_t const dfa_["
                << sm_.data()._dfa[0]->size () << "] = {\n        ";
            count_ = sm_.data()._dfa[0]->size () / 8;
            for (i_ = 0; i_ < count_; ++i_)
            {
                const std::size_t index_ = i_ * 8;
                os_ << dfa_[index_];
                for (j_ = 1; j_ < 8; ++j_)
                {
                    os_ << ", " << dfa_[index_ + j_];
                }
                if (i_ < count_ - 1)
                {
                    os_ << ",\n        ";
                }
            }

            const std::size_t mod_ = sm_.data()._dfa[0]->size () % 8;
            if (mod_)
            {
                const std::size_t index_ = count_ * 8;
                if (count_)
                {
                    os_ << ",\n        ";
                }
                os_ << dfa_[index_];
                for (j_ = 1; j_ < mod_; ++j_)
                {
                    os_ << ", " << dfa_[index_ + j_];
                }
            }
            os_ << " };\n";
        }

        os_ << "\n    if (start_token_ == end_)\n";
        os_ << "    {\n";
        os_ << "        unique_id_ = npos;\n";
        os_ << "        return 0;\n";
        os_ << "    }\n\n";
        if (sm_.data()._seen_BOL_assertion)
        {
            os_ << "    bool bol = bol_;\n\n";
        }

        if (dfas_ > 1)
        {
            os_ << "again:\n";
            os_ << "    std::size_t const* lookup_ = lookup_arr_[start_state_];\n";
            os_ << "    std::size_t dfa_alphabet_ = dfa_alphabet_arr_[start_state_];\n";
            os_ << "    std::size_t const*dfa_ = dfa_arr_[start_state_];\n";
        }

        os_ << "    std::size_t const* ptr_ = dfa_ + dfa_alphabet_;\n";
        os_ << "    Iterator curr_ = start_token_;\n";
        os_ << "    bool end_state_ = *ptr_ != 0;\n";
        os_ << "    std::size_t id_ = *(ptr_ + id_index);\n";
        os_ << "    std::size_t uid_ = *(ptr_ + unique_id_index);\n";
        if (dfas_ > 1)
        {
            os_ << "    std::size_t end_start_state_ = start_state_;\n";
        }
        if (sm_.data()._seen_BOL_assertion)
        {
            os_ << "    bool end_bol_ = bol_;\n";
        }
        os_ << "    Iterator end_token_ = start_token_;\n\n";

        os_ << "    while (curr_ != end_)\n";
        os_ << "    {\n";

        if (sm_.data()._seen_BOL_assertion)
        {
            os_ << "        std::size_t const BOL_state_ = ptr_[bol_index];\n\n";
        }

        if (sm_.data()._seen_EOL_assertion)
        {
            os_ << "        std::size_t const EOL_state_ = ptr_[eol_index];\n\n";
        }

        if (sm_.data()._seen_BOL_assertion && sm_.data()._seen_EOL_assertion)
        {
            os_ << "        if (BOL_state_ && bol)\n";
            os_ << "        {\n";
            os_ << "            ptr_ = &dfa_[BOL_state_ * dfa_alphabet_];\n";
            os_ << "        }\n";
            os_ << "        else if (EOL_state_ && *curr_ == '\\n')\n";
            os_ << "        {\n";
            os_ << "            ptr_ = &dfa_[EOL_state_ * dfa_alphabet_];\n";
            os_ << "        }\n";
            os_ << "        else\n";
            os_ << "        {\n";
            if (lookups_ == 256)
            {
                os_ << "            unsigned char index = \n";
                os_ << "                static_cast<unsigned char>(*curr_++);\n";
            }
            else
            {
                os_ << "            std::size_t index = *curr_++\n";
            }
            os_ << "            bol = (index == '\\n') ? true : false;\n";
            os_ << "            std::size_t const state_ = ptr_[\n";
            os_ << "                lookup_[static_cast<std::size_t>(index)]];\n";

            os_ << '\n';
            os_ << "            if (state_ == 0) break;\n";
            os_ << '\n';
            os_ << "            ptr_ = &dfa_[state_ * dfa_alphabet_];\n";
            os_ << "        }\n\n";
        }
        else if (sm_.data()._seen_BOL_assertion)
        {
            os_ << "        if (BOL_state_ && bol)\n";
            os_ << "        {\n";
            os_ << "            ptr_ = &dfa_[BOL_state_ * dfa_alphabet_];\n";
            os_ << "        }\n";
            os_ << "        else\n";
            os_ << "        {\n";
            if (lookups_ == 256)
            {
                os_ << "            unsigned char index = \n";
                os_ << "                static_cast<unsigned char>(*curr_++);\n";
            }
            else
            {
                os_ << "            std::size_t index = *curr_++\n";
            }
            os_ << "            bol = (index == '\\n') ? true : false;\n";
            os_ << "            std::size_t const state_ = ptr_[\n";
            os_ << "                lookup_[static_cast<std::size_t>(index)]];\n";

            os_ << '\n';
            os_ << "            if (state_ == 0) break;\n";
            os_ << '\n';
            os_ << "            ptr_ = &dfa_[state_ * dfa_alphabet_];\n";
            os_ << "        }\n\n";
        }
        else if (sm_.data()._seen_EOL_assertion)
        {
            os_ << "        if (EOL_state_ && *curr_ == '\\n')\n";
            os_ << "        {\n";
            os_ << "            ptr_ = &dfa_[EOL_state_ * dfa_alphabet_];\n";
            os_ << "        }\n";
            os_ << "        else\n";
            os_ << "        {\n";
            if (lookups_ == 256)
            {
                os_ << "            unsigned char index = \n";
                os_ << "                static_cast<unsigned char>(*curr_++);\n";
            }
            else
            {
                os_ << "            std::size_t index = *curr_++\n";
            }
            os_ << "            bol = (index == '\\n') ? true : false;\n";
            os_ << "            std::size_t const state_ = ptr_[\n";
            os_ << "                lookup_[static_cast<std::size_t>(index)]];\n";

            os_ << '\n';
            os_ << "            if (state_ == 0) break;\n";
            os_ << '\n';
            os_ << "            ptr_ = &dfa_[state_ * dfa_alphabet_];\n";
            os_ << "        }\n\n";
        }
        else
        {
            os_ << "        std::size_t const state_ =\n";

            if (lookups_ == 256)
            {
                os_ << "            ptr_[lookup_["
                       "static_cast<unsigned char>(*curr_++)]];\n";
            }
            else
            {
                os_ << "            ptr_[lookup_[*curr_++]];\n";
            }

            os_ << '\n';
            os_ << "        if (state_ == 0) break;\n";
            os_ << '\n';
            os_ << "        ptr_ = &dfa_[state_ * dfa_alphabet_];\n\n";
        }

        os_ << "        if (*ptr_)\n";
        os_ << "        {\n";
        os_ << "            end_state_ = true;\n";
        os_ << "            id_ = *(ptr_ + id_index);\n";
        os_ << "            uid_ = *(ptr_ + unique_id_index);\n";
        if (dfas_ > 1)
        {
            os_ << "            end_start_state_ = *(ptr_ + state_index);\n";
        }
        if (sm_.data()._seen_BOL_assertion)
        {
            os_ << "            end_bol_ = bol;\n";
        }
        os_ << "            end_token_ = curr_;\n";
        os_ << "        }\n";
        os_ << "    }\n\n";

        if (sm_.data()._seen_EOL_assertion)
        {
            os_ << "    std::size_t const EOL_state_ = ptr_[eol_index];\n\n";

            os_ << "    if (EOL_state_ && curr_ == end_)\n";
            os_ << "    {\n";
            os_ << "        ptr_ = &dfa_[EOL_state_ * dfa_alphabet_];\n\n";

            os_ << "        if (*ptr_)\n";
            os_ << "        {\n";
            os_ << "            end_state_ = true;\n";
            os_ << "            id_ = *(ptr_ + id_index);\n";
            os_ << "            uid_ = *(ptr_ + unique_id_index);\n";
            if (dfas_ > 1)
            {
                os_ << "            end_start_state_ = *(ptr_ + state_index);\n";
            }
            if (sm_.data()._seen_BOL_assertion)
            {
                os_ << "            end_bol_ = bol;\n";
            }
            os_ << "            end_token_ = curr_;\n";
            os_ << "        }\n";
            os_ << "    }\n\n";
        }

        os_ << "    if (end_state_)\n";
        os_ << "    {\n";
        os_ << "        // return longest match\n";
        os_ << "        start_token_ = end_token_;\n";

        if (dfas_ > 1)
        {
            os_ << "        start_state_ = end_start_state_;\n";
            os_ << "        if (id_ == 0)\n";
            os_ << "        {\n";
            if (sm_.data()._seen_BOL_assertion)
            {
                os_ << "            bol = end_bol_;\n";
            }
            os_ << "            goto again;\n";
            os_ << "        }\n";
            if (sm_.data()._seen_BOL_assertion)
            {
                os_ << "        else\n";
                os_ << "        {\n";
                os_ << "            bol_ = end_bol_;\n";
                os_ << "        }\n";
            }
        }
        else if (sm_.data()._seen_BOL_assertion)
        {
            os_ << "        bol_ = end_bol_;\n";
        }

        os_ << "    }\n";
        os_ << "    else\n";
        os_ << "    {\n";

        if (sm_.data()._seen_BOL_assertion)
        {
            os_ << "        bol_ = (*start_token_ == '\\n') ? true : false;\n";
        }

        os_ << "        id_ = npos;\n";
        os_ << "        uid_ = npos;\n";
        os_ << "    }\n\n";

        os_ << "    unique_id_ = uid_;\n";
        os_ << "    return id_;\n";
        return os_.good();
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Char>
    inline std::basic_string<Char> get_charlit(Char ch)
    {
        std::basic_string<Char> result;
        boost::lexer::basic_string_token<Char>::escape_char(ch, result);
        return result;
    }

    // check whether state0_0 is referenced from any of the other states
    template <typename Char>
    bool need_label0_0(boost::lexer::basic_state_machine<Char> const &sm_)
    {
        typedef typename boost::lexer::basic_state_machine<Char>::iterator
            iterator_type;
        iterator_type iter_ = sm_.begin();
        std::size_t const states_ = iter_->states;

        for (std::size_t state_ = 0; state_ < states_; ++state_)
        {
            if (0 == iter_->bol_index || 0 == iter_->eol_index)
            {
                return true;
            }

            std::size_t const transitions_ = iter_->transitions;
            for (std::size_t t_ = 0; t_ < transitions_; ++t_)
            {
                if (0 == iter_->goto_state)
                {
                    return true;
                }
                ++iter_;
            }
            if (transitions_ == 0) ++iter_;
        }
        return false;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Char>
    bool generate_function_body_switch(std::basic_ostream<Char> & os_
      , boost::lexer::basic_state_machine<Char> const &sm_)
    {
        typedef typename boost::lexer::basic_state_machine<Char>::iterator
            iterator_type;

        std::size_t const lookups_ = sm_.data()._lookup->front ()->size ();
        iterator_type iter_ = sm_.begin();
        iterator_type labeliter_ = iter_;
        iterator_type end_ = sm_.end();
        std::size_t const dfas_ = sm_.data()._dfa->size ();

        os_ << "    static std::size_t const npos = "
               "static_cast<std::size_t>(~0);\n";

        os_ << "\n    if (start_token_ == end_)\n";
        os_ << "    {\n";
        os_ << "        unique_id_ = npos;\n";
        os_ << "        return 0;\n";
        os_ << "    }\n\n";

        if (sm_.data()._seen_BOL_assertion)
        {
            os_ << "    bool bol = bol_;\n";
        }

        if (dfas_ > 1)
        {
            os_ << "again:\n";
        }

        os_ << "    Iterator curr_ = start_token_;\n";
        os_ << "    bool end_state_ = false;\n";
        os_ << "    std::size_t id_ = npos;\n";
        os_ << "    std::size_t uid_ = npos;\n";

        if (dfas_ > 1)
        {
            os_ << "    std::size_t end_start_state_ = start_state_;\n";
        }

        if (sm_.data()._seen_BOL_assertion)
        {
            os_ << "    bool end_bol_ = bol_;\n";
        }

        os_ << "    Iterator end_token_ = start_token_;\n";
        os_ << '\n';

        os_ << "    " << ((lookups_ == 256) ? "char" : "wchar_t")
            << " ch_ = 0;\n\n";

        if (dfas_ > 1)
        {
            os_ << "    switch (start_state_)\n";
            os_ << "    {\n";

            for (std::size_t i_ = 0; i_ < dfas_; ++i_)
            {
                os_ << "    case " << i_ << ":\n";
                os_ << "        goto state" << i_ << "_0;\n";
                os_ << "        break;\n";
            }

            os_ << "    default:\n";
            os_ << "        goto end;\n";
            os_ << "        break;\n";
            os_ << "    }\n";
        }

        bool need_state0_0_label = need_label0_0(sm_);

        for (std::size_t dfa_ = 0; dfa_ < dfas_; ++dfa_)
        {
            std::size_t const states_ = iter_->states;
            for (std::size_t state_ = 0; state_ < states_; ++state_)
            {
                std::size_t const transitions_ = iter_->transitions;
                std::size_t t_ = 0;

                if (dfas_ > 1 || dfa_ != 0 || state_ != 0 || need_state0_0_label)
                {
                    os_ << "\nstate" << dfa_ << '_' << state_ << ":\n";
                }

                if (iter_->end_state)
                {
                    os_ << "    end_state_ = true;\n";
                    os_ << "    id_ = " << iter_->id << ";\n";
                    os_ << "    uid_ = " << iter_->unique_id << ";\n";
                    os_ << "    end_token_ = curr_;\n";

                    if (dfas_ > 1)
                    {
                        os_ << "    end_start_state_ = " << iter_->goto_dfa <<
                            ";\n";
                    }

                    if (sm_.data()._seen_BOL_assertion)
                    {
                        os_ << "    end_bol_ = bol;\n";
                    }

                    if (transitions_) os_ << '\n';
                }

                if (t_ < transitions_ ||
                    iter_->bol_index != boost::lexer::npos ||
                    iter_->eol_index != boost::lexer::npos)
                {
                    os_ << "    if (curr_ == end_) goto end;\n";
                    os_ << "    ch_ = *curr_;\n";
                    if (iter_->bol_index != boost::lexer::npos)
                    {
                        os_ << "\n    if (bol) goto state" << dfa_ << '_'
                            << iter_->bol_index << ";\n";
                    }
                    if (iter_->eol_index != boost::lexer::npos)
                    {
                        os_ << "\n    if (ch_ == '\\n') goto state" << dfa_
                            << '_' << iter_->eol_index << ";\n";
                    }
                    os_ << "    ++curr_;\n";
                }

                for (/**/; t_ < transitions_; ++t_)
                {
                    Char const *ptr_ = iter_->token._charset.c_str();
                    Char const *end2_ = ptr_ + iter_->token._charset.size();
                    Char start_char_ = 0;
                    Char curr_char_ = 0;
                    bool range_ = false;
                    bool first_char_ = true;

                    os_ << "\n    if (";

                    while (ptr_ != end2_)
                    {
                        curr_char_ = *ptr_++;

                        if (*ptr_ == curr_char_ + 1)
                        {
                            if (!range_)
                            {
                                start_char_ = curr_char_;
                            }
                            range_ = true;
                        }
                        else
                        {
                            if (!first_char_)
                            {
                                os_ << ((iter_->token._negated) ? " && " : " || ");
                            }
                            else
                            {
                                first_char_ = false;
                            }
                            if (range_)
                            {
                                if (iter_->token._negated)
                                {
                                    os_ << "!";
                                }
                                os_ << "(ch_ >= '" << get_charlit(start_char_)
                                    << "' && ch_ <= '"
                                    << get_charlit(curr_char_) << "')";
                                range_ = false;
                            }
                            else
                            {
                                os_ << "ch_ "
                                    << ((iter_->token._negated) ? "!=" : "==")
                                    << " '" << get_charlit(curr_char_) << "'";
                            }
                        }
                    }

                    os_ << ") goto state" << dfa_ << '_' << iter_->goto_state
                        << ";\n";
                    ++iter_;
                }

                if (!(dfa_ == dfas_ - 1 && state_ == states_ - 1))
                {
                    os_ << "    goto end;\n";
                }

                if (transitions_ == 0) ++iter_;
            }
        }

        os_ << "\nend:\n";
        os_ << "    if (end_state_)\n";
        os_ << "    {\n";
        os_ << "        // return longest match\n";
        os_ << "        start_token_ = end_token_;\n";

        if (dfas_ > 1)
        {
            os_ << "        start_state_ = end_start_state_;\n";
            os_ << "\n        if (id_ == 0)\n";
            os_ << "        {\n";

            if (sm_.data()._seen_BOL_assertion)
            {
                os_ << "            bol = end_bol_;\n";
            }

            os_ << "            goto again;\n";
            os_ << "        }\n";

            if (sm_.data()._seen_BOL_assertion)
            {
                os_ << "        else\n";
                os_ << "        {\n";
                os_ << "            bol_ = end_bol_;\n";
                os_ << "        }\n";
            }
        }
        else if (sm_.data()._seen_BOL_assertion)
        {
            os_ << "        bol_ = end_bol_;\n";
        }

        os_ << "    }\n";
        os_ << "    else\n";
        os_ << "    {\n";

        if (sm_.data()._seen_BOL_assertion)
        {
            os_ << "        bol_ = (*start_token_ == '\\n') ? true : false;\n";
        }
        os_ << "        id_ = npos;\n";
        os_ << "        uid_ = npos;\n";
        os_ << "    }\n\n";

        os_ << "    unique_id_ = uid_;\n";
        os_ << "    return id_;\n";
        return os_.good();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Generate a tokenizer for the given state machine.
    template <typename Char, typename F>
    inline bool
    generate_cpp (boost::lexer::basic_state_machine<Char> const& sm_
      , boost::lexer::basic_rules<Char> const& rules_
      , std::basic_ostream<Char> &os_, Char const* name_suffix
      , F generate_function_body)
    {
        if (sm_.data()._lookup->empty())
            return false;

        std::size_t const dfas_ = sm_.data()._dfa->size();
//         std::size_t const lookups_ = sm_.data()._lookup->front()->size();

        os_ << "// Copyright (c) 2008-2009 Ben Hanson\n";
        os_ << "// Copyright (c) 2008-2011 Hartmut Kaiser\n";
        os_ << "//\n";
        os_ << "// Distributed under the Boost Software License, "
            "Version 1.0. (See accompanying\n";
        os_ << "// file licence_1_0.txt or copy at "
            "http://www.boost.org/LICENSE_1_0.txt)\n\n";
        os_ << "// Auto-generated by boost::lexer, do not edit\n\n";

        std::basic_string<Char> guard(name_suffix);
        guard += L<Char>(name_suffix[0] ? "_" : "");
        guard += L<Char>(__DATE__ "_" __TIME__);
        typename std::basic_string<Char>::size_type p = 
            guard.find_first_of(L<Char>(": "));
        while (std::string::npos != p)
        {
            guard.replace(p, 1, L<Char>("_"));
            p = guard.find_first_of(L<Char>(": "), p);
        }
        { // to_upper(guard)
            typedef std::ctype<Char> facet_t;
            facet_t const& facet = std::use_facet<facet_t>(std::locale());
            typedef typename std::basic_string<Char>::iterator iter_t;
            for (iter_t iter = guard.begin(),
                        last = guard.end(); iter != last; ++iter)
                *iter = facet.toupper(*iter);
        }

        os_ << "#if !defined(BOOST_SPIRIT_LEXER_NEXT_TOKEN_" << guard << ")\n";
        os_ << "#define BOOST_SPIRIT_LEXER_NEXT_TOKEN_" << guard << "\n\n";

        os_ << "#include <boost/spirit/home/support/detail/lexer/char_traits.hpp>\n\n";

        generate_delimiter(os_);
        os_ << "// the generated table of state names and the tokenizer have to be\n"
               "// defined in the boost::spirit::lex::lexertl::static_ namespace\n";
        os_ << "namespace boost { namespace spirit { namespace lex { "
            "namespace lexertl { namespace static_ {\n\n";

        // generate the lexer state information variables
        if (!generate_cpp_state_info(rules_, os_, name_suffix))
            return false;

        generate_delimiter(os_);
        os_ << "// this function returns the next matched token\n";
        os_ << "template<typename Iterator>\n";
        os_ << "std::size_t next_token" << (name_suffix[0] ? "_" : "")
            << name_suffix  << " (";

        if (dfas_ > 1)
        {
            os_ << "std::size_t& start_state_, ";
        }
        else
        {
            os_ << "std::size_t& /*start_state_*/, ";
        }
        if (sm_.data()._seen_BOL_assertion)
        {
            os_ << "bool& bol_, ";
        }
        else
        {
            os_ << "bool& /*bol_*/, ";
        }
        os_ << "\n    ";

        os_ << "Iterator &start_token_, Iterator const& end_, ";
        os_ << "std::size_t& unique_id_)\n";
        os_ << "{\n";
        if (!generate_function_body(os_, sm_))
            return false;
        os_ << "}\n\n";

        if (!generate_cpp_state_table<Char>(os_, name_suffix
            , sm_.data()._seen_BOL_assertion, sm_.data()._seen_EOL_assertion))
        {
            return false;
        }

        os_ << "}}}}}  // namespace boost::spirit::lex::lexertl::static_\n\n";

        os_ << "#endif\n";

        return os_.good();
    }

    }   // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    template <typename Lexer, typename F>
    inline bool
    generate_static(Lexer const& lexer
      , std::basic_ostream<typename Lexer::char_type>& os
      , typename Lexer::char_type const* name_suffix, F f)
    {
        if (!lexer.init_dfa(true))    // always minimize DFA for static lexers
            return false;
        return detail::generate_cpp(lexer.state_machine_, lexer.rules_, os
          , name_suffix, f);
    }

    ///////////////////////////////////////////////////////////////////////////
    // deprecated function, will be removed in the future (this has been
    // replaced by the function generate_static_dfa - see below).
    template <typename Lexer>
    inline bool
    generate_static(Lexer const& lexer
      , std::basic_ostream<typename Lexer::char_type>& os
      , typename Lexer::char_type const* name_suffix =
          detail::L<typename Lexer::char_type>())
    {
        return generate_static(lexer, os, name_suffix
          , &detail::generate_function_body_dfa<typename Lexer::char_type>);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Lexer>
    inline bool
    generate_static_dfa(Lexer const& lexer
      , std::basic_ostream<typename Lexer::char_type>& os
      , typename Lexer::char_type const* name_suffix =
          detail::L<typename Lexer::char_type>())
    {
        return generate_static(lexer, os, name_suffix
          , &detail::generate_function_body_dfa<typename Lexer::char_type>);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Lexer>
    inline bool
    generate_static_switch(Lexer const& lexer
      , std::basic_ostream<typename Lexer::char_type>& os
      , typename Lexer::char_type const* name_suffix =
          detail::L<typename Lexer::char_type>())
    {
        return generate_static(lexer, os, name_suffix
          , &detail::generate_function_body_switch<typename Lexer::char_type>);
    }

///////////////////////////////////////////////////////////////////////////////
}}}}

#endif

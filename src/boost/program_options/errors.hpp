// Copyright Vladimir Prus 2002-2004.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_ERRORS_VP_2003_01_02
#define BOOST_ERRORS_VP_2003_01_02

#include <boost/program_options/config.hpp>

#include <string>
#include <stdexcept>
#include <vector>
#include <map>


#if defined(BOOST_MSVC)
#   pragma warning (push)
#   pragma warning (disable:4275) // non dll-interface class 'std::logic_error' used as base for dll-interface class 'boost::program_options::error'
#   pragma warning (disable:4251) // class 'std::vector<_Ty>' needs to have dll-interface to be used by clients of class 'boost::program_options::ambiguous_option'
#endif

namespace boost { namespace program_options {

    inline std::string strip_prefixes(const std::string& text)
    {
        // "--foo-bar" -> "foo-bar"
        std::string::size_type i = text.find_first_not_of("-/");
        if (i == std::string::npos) {
            return text;
        } else {
            return text.substr(i);
        }
    }

    /** Base class for all errors in the library. */
    class BOOST_PROGRAM_OPTIONS_DECL BOOST_SYMBOL_VISIBLE error : public std::logic_error {
    public:
        error(const std::string& xwhat) : std::logic_error(xwhat) {}
    };


    /** Class thrown when there are too many positional options. 
        This is a programming error.
    */
    class BOOST_PROGRAM_OPTIONS_DECL BOOST_SYMBOL_VISIBLE too_many_positional_options_error : public error {
    public:
        too_many_positional_options_error() 
         : error("too many positional options have been specified on the command line") 
        {}
    };

    /** Class thrown when there are programming error related to style */
    class BOOST_PROGRAM_OPTIONS_DECL BOOST_SYMBOL_VISIBLE invalid_command_line_style : public error {
    public:
        invalid_command_line_style(const std::string& msg)
        : error(msg)
        {}
    };

    /** Class thrown if config file can not be read */
    class BOOST_PROGRAM_OPTIONS_DECL BOOST_SYMBOL_VISIBLE reading_file : public error {
    public:
        reading_file(const char* filename)
         : error(std::string("can not read options configuration file '").append(filename).append("'"))
        {}
    };


    /** Base class for most exceptions in the library.
     *  
     *  Substitutes the values for the parameter name
     *      placeholders in the template to create the human
     *      readable error message
     *  
     *  Placeholders are surrounded by % signs: %example%
     *      Poor man's version of boost::format
     *  
     *  If a parameter name is absent, perform default substitutions
     *      instead so ugly placeholders are never left in-place.
     *  
     *  Options are displayed in "canonical" form
     *      This is the most unambiguous form of the
     *      *parsed* option name and would correspond to
     *      option_description::format_name()
     *      i.e. what is shown by print_usage()
     *  
     *  The "canonical" form depends on whether the option is
     *      specified in short or long form, using dashes or slashes
     *      or without a prefix (from a configuration file)
     *  
     *   */
    class BOOST_PROGRAM_OPTIONS_DECL BOOST_SYMBOL_VISIBLE error_with_option_name : public error {

    protected:
        /** can be
         *      0 = no prefix (config file options)
         *      allow_long
         *      allow_dash_for_short
         *      allow_slash_for_short
         *      allow_long_disguise */
        int m_option_style;


        /** substitutions
         *  from placeholders to values */
        std::map<std::string, std::string> m_substitutions;
        typedef std::pair<std::string, std::string> string_pair;
        std::map<std::string, string_pair > m_substitution_defaults;

    public:
        /** template with placeholders */
        std::string m_error_template;

        error_with_option_name(const std::string& template_,
                               const std::string& option_name = "",
                               const std::string& original_token = "",
                               int option_style = 0);

        /** gcc says that throw specification on dtor is loosened 
         *  without this line                                     
         *  */ 
        ~error_with_option_name() throw() {}


        //void dump() const
        //{
        //  std::cerr << "m_substitution_defaults:\n";
        //  for (std::map<std::string, string_pair>::const_iterator iter = m_substitution_defaults.begin();
        //        iter != m_substitution_defaults.end(); ++iter)
        //      std::cerr << "\t" << iter->first << ":" << iter->second.first << "=" << iter->second.second << "\n";
        //  std::cerr << "m_substitutions:\n";
        //  for (std::map<std::string, std::string>::const_iterator iter = m_substitutions.begin();
        //        iter != m_substitutions.end(); ++iter)
        //      std::cerr << "\t" << iter->first << "=" << iter->second << "\n";
        //  std::cerr << "m_error_template:\n";
        //  std::cerr << "\t" << m_error_template << "\n";
        //  std::cerr << "canonical_option_prefix:[" << get_canonical_option_prefix() << "]\n";
        //  std::cerr << "canonical_option_name:[" << get_canonical_option_name() <<"]\n";
        //  std::cerr << "what:[" << what() << "]\n";
        //}

        /** Substitute
         *      parameter_name->value to create the error message from
         *      the error template */
        void set_substitute(const std::string& parameter_name,  const std::string& value)
        {           m_substitutions[parameter_name] = value;    }

        /** If the parameter is missing, then make the
         *      from->to substitution instead */
        void set_substitute_default(const std::string& parameter_name, 
                                    const std::string& from,  
                                    const std::string& to)
        {           
            m_substitution_defaults[parameter_name] = std::make_pair(from, to); 
        }


        /** Add context to an exception */
        void add_context(const std::string& option_name,
                         const std::string& original_token,
                         int option_style)
        {
            set_option_name(option_name);
            set_original_token(original_token);
            set_prefix(option_style);
        }

        void set_prefix(int option_style)
        {           m_option_style = option_style;}

        /** Overridden in error_with_no_option_name */
        virtual void set_option_name(const std::string& option_name)
        {           set_substitute("option", option_name);}

        std::string get_option_name() const
        {           return get_canonical_option_name();         }

        void set_original_token(const std::string& original_token)
        {           set_substitute("original_token", original_token);}


        /** Creates the error_message on the fly
         *      Currently a thin wrapper for substitute_placeholders() */
        virtual const char* what() const throw();

    protected:
        /** Used to hold the error text returned by what() */
        mutable std::string m_message;  // For on-demand formatting in 'what'

        /** Makes all substitutions using the template */
        virtual void substitute_placeholders(const std::string& error_template) const;

        // helper function for substitute_placeholders
        void replace_token(const std::string& from, const std::string& to) const;

        /** Construct option name in accordance with the appropriate
         *  prefix style: i.e. long dash or short slash etc */
        std::string get_canonical_option_name() const;
        std::string get_canonical_option_prefix() const;
    };


    /** Class thrown when there are several option values, but
        user called a method which cannot return them all. */
    class BOOST_PROGRAM_OPTIONS_DECL BOOST_SYMBOL_VISIBLE multiple_values : public error_with_option_name {
    public:
        multiple_values() 
         : error_with_option_name("option '%canonical_option%' only takes a single argument"){}

        ~multiple_values() throw() {}
    };

    /** Class thrown when there are several occurrences of an
        option, but user called a method which cannot return 
        them all. */
    class BOOST_PROGRAM_OPTIONS_DECL BOOST_SYMBOL_VISIBLE multiple_occurrences : public error_with_option_name {
    public:
        multiple_occurrences() 
         : error_with_option_name("option '%canonical_option%' cannot be specified more than once"){}

        ~multiple_occurrences() throw() {}

    };

    /** Class thrown when a required/mandatory option is missing */
    class BOOST_PROGRAM_OPTIONS_DECL BOOST_SYMBOL_VISIBLE required_option : public error_with_option_name {
    public:
       // option name is constructed by the option_descriptor and never on the fly
       required_option(const std::string& option_name)
       : error_with_option_name("the option '%canonical_option%' is required but missing", "", option_name)
       {
       }

       ~required_option() throw() {}
    };

    /** Base class of unparsable options,
     *  when the desired option cannot be identified.
     *  
     *  
     *  It makes no sense to have an option name, when we can't match an option to the
     *      parameter
     *  
     *  Having this a part of the error_with_option_name hierachy makes error handling
     *      a lot easier, even if the name indicates some sort of conceptual dissonance!
     *  
     *   */
    class BOOST_PROGRAM_OPTIONS_DECL BOOST_SYMBOL_VISIBLE error_with_no_option_name : public error_with_option_name {
    public:
        error_with_no_option_name(const std::string& template_,
                              const std::string& original_token = "")
        : error_with_option_name(template_, "", original_token)
        {
        }

        /** Does NOT set option name, because no option name makes sense */
        virtual void set_option_name(const std::string&) {}

        ~error_with_no_option_name() throw() {}
    };


    /** Class thrown when option name is not recognized. */
    class BOOST_PROGRAM_OPTIONS_DECL BOOST_SYMBOL_VISIBLE unknown_option : public error_with_no_option_name {
    public:
        unknown_option(const std::string& original_token = "")
        : error_with_no_option_name("unrecognised option '%canonical_option%'", original_token)
        {
        }

        ~unknown_option() throw() {}
    };



    /** Class thrown when there's ambiguity amoung several possible options. */
    class BOOST_PROGRAM_OPTIONS_DECL BOOST_SYMBOL_VISIBLE ambiguous_option : public error_with_no_option_name {
    public:
        ambiguous_option(const std::vector<std::string>& xalternatives)
        : error_with_no_option_name("option '%canonical_option%' is ambiguous"),
            m_alternatives(xalternatives)
        {}

        ~ambiguous_option() throw() {}

        const std::vector<std::string>& alternatives() const throw() {return m_alternatives;}

    protected:
        /** Makes all substitutions using the template */
        virtual void substitute_placeholders(const std::string& error_template) const;
    private:
        // TODO: copy ctor might throw
        std::vector<std::string> m_alternatives;
    };


    /** Class thrown when there's syntax error either for command
     *  line or config file options. See derived children for
     *  concrete classes. */
    class BOOST_PROGRAM_OPTIONS_DECL BOOST_SYMBOL_VISIBLE invalid_syntax : public error_with_option_name {
    public:
        enum kind_t {
            long_not_allowed = 30,
            long_adjacent_not_allowed,
            short_adjacent_not_allowed,
            empty_adjacent_parameter,
            missing_parameter,
            extra_parameter,
            unrecognized_line
        };

        invalid_syntax(kind_t kind, 
                       const std::string& option_name = "",
                       const std::string& original_token = "",
                       int option_style              = 0):
            error_with_option_name(get_template(kind), option_name, original_token, option_style),
            m_kind(kind)
        {
        }

        ~invalid_syntax() throw() {}

        kind_t kind() const {return m_kind;}

        /** Convenience functions for backwards compatibility */
        virtual std::string tokens() const {return get_option_name();   }
    protected:
        /** Used to convert kind_t to a related error text */
        std::string get_template(kind_t kind);
        kind_t m_kind;
    };

    class BOOST_PROGRAM_OPTIONS_DECL BOOST_SYMBOL_VISIBLE invalid_config_file_syntax : public invalid_syntax {
    public:
        invalid_config_file_syntax(const std::string& invalid_line, kind_t kind):
            invalid_syntax(kind)
        {
            m_substitutions["invalid_line"] = invalid_line;
        }

        ~invalid_config_file_syntax() throw() {}

        /** Convenience functions for backwards compatibility */
        virtual std::string tokens() const {return m_substitutions.find("invalid_line")->second;    }
    };


    /** Class thrown when there are syntax errors in given command line */
    class BOOST_PROGRAM_OPTIONS_DECL BOOST_SYMBOL_VISIBLE invalid_command_line_syntax : public invalid_syntax {
    public:
        invalid_command_line_syntax(kind_t kind,
                       const std::string& option_name = "",
                       const std::string& original_token = "",
                       int option_style              = 0):
            invalid_syntax(kind, option_name, original_token, option_style) {}
        ~invalid_command_line_syntax() throw() {}
    };


    /** Class thrown when value of option is incorrect. */
    class BOOST_PROGRAM_OPTIONS_DECL BOOST_SYMBOL_VISIBLE validation_error : public error_with_option_name {
    public:
        enum kind_t {
            multiple_values_not_allowed = 30,
            at_least_one_value_required, 
            invalid_bool_value,
            invalid_option_value,
            invalid_option
        };
        
    public:
        validation_error(kind_t kind, 
                   const std::string& option_name = "",
                   const std::string& original_token = "",
                   int option_style              = 0):
        error_with_option_name(get_template(kind), option_name, original_token, option_style),
        m_kind(kind)
        {
        }

        ~validation_error() throw() {}

        kind_t kind() const { return m_kind; }

    protected:
        /** Used to convert kind_t to a related error text */
        std::string get_template(kind_t kind);
        kind_t m_kind;
    };

    /** Class thrown if there is an invalid option value given */
    class BOOST_PROGRAM_OPTIONS_DECL BOOST_SYMBOL_VISIBLE invalid_option_value
        : public validation_error
    {
    public:
        invalid_option_value(const std::string& value);
#ifndef BOOST_NO_STD_WSTRING
        invalid_option_value(const std::wstring& value);
#endif
    };

    /** Class thrown if there is an invalid bool value given */
    class BOOST_PROGRAM_OPTIONS_DECL BOOST_SYMBOL_VISIBLE invalid_bool_value
        : public validation_error
    {
    public:
        invalid_bool_value(const std::string& value);
    };





    

}}

#if defined(BOOST_MSVC)
#   pragma warning (pop)
#endif

#endif

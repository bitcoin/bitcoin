// Copyright Vladimir Prus 2002-2004.
// Copyright Bertolt Mildner 2004.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_OPTION_DESCRIPTION_VP_2003_05_19
#define BOOST_OPTION_DESCRIPTION_VP_2003_05_19

#include <boost/program_options/config.hpp>
#include <boost/program_options/errors.hpp>
#include <boost/program_options/value_semantic.hpp>

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/any.hpp>

#include <string>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>
#include <utility>

#include <iosfwd>

#if defined(BOOST_MSVC)
#   pragma warning (push)
#   pragma warning (disable:4251) // class 'boost::shared_ptr<T>' needs to have dll-interface to be used by clients of class 'boost::program_options::option_description'
#endif


/** Boost namespace */
namespace boost { 
/** Namespace for the library. */
namespace program_options {

    /** Describes one possible command line/config file option. There are two
        kinds of properties of an option. First describe it syntactically and
        are used only to validate input. Second affect interpretation of the
        option, for example default value for it or function that should be
        called  when the value is finally known. Routines which perform parsing
        never use second kind of properties \-- they are side effect free.
        @sa options_description
    */
    class BOOST_PROGRAM_OPTIONS_DECL option_description {
    public:

        option_description();

        /** Initializes the object with the passed data.

            Note: it would be nice to make the second parameter auto_ptr,
            to explicitly pass ownership. Unfortunately, it's often needed to
            create objects of types derived from 'value_semantic':
               options_description d;
               d.add_options()("a", parameter<int>("n")->default_value(1));
            Here, the static type returned by 'parameter' should be derived
            from value_semantic.

            Alas, derived->base conversion for auto_ptr does not really work,
            see
            http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2000/n1232.pdf
            http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#84

            So, we have to use plain old pointers. Besides, users are not
            expected to use the constructor directly.

            
            The 'name' parameter is interpreted by the following rules:
            - if there's no "," character in 'name', it specifies long name
            - otherwise, the part before "," specifies long name and the part
            after \-- short name.
        */
        option_description(const char* name,
                           const value_semantic* s);

        /** Initializes the class with the passed data. 
         */
        option_description(const char* name,
                           const value_semantic* s,
                           const char* description);

        virtual ~option_description();

        enum match_result { no_match, full_match, approximate_match };

        /** Given 'option', specified in the input source,
            returns 'true' if 'option' specifies *this.
        */
        match_result match(const std::string& option, bool approx,
                           bool long_ignore_case, bool short_ignore_case) const;

        /** Returns the key that should identify the option, in
            particular in the variables_map class.
            The 'option' parameter is the option spelling from the
            input source.
            If option name contains '*', returns 'option'.
            If long name was specified, it's the long name, otherwise
            it's a short name with prepended '-'.
        */
        const std::string& key(const std::string& option) const;


        /** Returns the canonical name for the option description to enable the user to
            recognised a matching option.
            1) For short options ('-', '/'), returns the short name prefixed.
            2) For long options ('--' / '-') returns the first long name prefixed
            3) All other cases, returns the first long name (if present) or the short
               name, unprefixed.
        */
        std::string canonical_display_name(int canonical_option_style = 0) const;

        const std::string& long_name() const;

        const std::pair<const std::string*, std::size_t> long_names() const;

        /// Explanation of this option
        const std::string& description() const;

        /// Semantic of option's value
        shared_ptr<const value_semantic> semantic() const;
        
        /// Returns the option name, formatted suitably for usage message. 
        std::string format_name() const;

        /** Returns the parameter name and properties, formatted suitably for
            usage message. */
        std::string format_parameter() const;

    private:
    
        option_description& set_names(const char* name);

        /**
         * a one-character "switch" name - with its prefix,
         * so that this is either empty or has length 2 (e.g. "-c"
         */
        std::string m_short_name;

        /**
         *  one or more names by which this option may be specified
         *  on a command-line or in a config file, which are not
         *  a single-letter switch. The names here are _without_
         * any prefix.
         */
        std::vector<std::string> m_long_names;

        std::string m_description;

        // shared_ptr is needed to simplify memory management in
        // copy ctor and destructor.
        shared_ptr<const value_semantic> m_value_semantic;
    };

    class options_description;

    /** Class which provides convenient creation syntax to option_description. 
     */        
    class BOOST_PROGRAM_OPTIONS_DECL options_description_easy_init {
    public:
        options_description_easy_init(options_description* owner);

        options_description_easy_init&
        operator()(const char* name,
                   const char* description);

        options_description_easy_init&
        operator()(const char* name,
                   const value_semantic* s);
        
        options_description_easy_init&
        operator()(const char* name,
                   const value_semantic* s,
                   const char* description);
       
    private:
        options_description* owner;
    };


    /** A set of option descriptions. This provides convenient interface for
        adding new option (the add_options) method, and facilities to search
        for options by name.
        
        See @ref a_adding_options "here" for option adding interface discussion.
        @sa option_description
    */
    class BOOST_PROGRAM_OPTIONS_DECL options_description {
    public:
        static const unsigned m_default_line_length;
        
        /** Creates the instance. */
        options_description(unsigned line_length = m_default_line_length,
                            unsigned min_description_length = m_default_line_length / 2);
        /** Creates the instance. The 'caption' parameter gives the name of
            this 'options_description' instance. Primarily useful for output.
            The 'description_length' specifies the number of columns that
            should be reserved for the description text; if the option text
            encroaches into this, then the description will start on the next
            line.
        */
        options_description(const std::string& caption,
                            unsigned line_length = m_default_line_length,
                            unsigned min_description_length = m_default_line_length / 2);
        /** Adds new variable description. Throws duplicate_variable_error if
            either short or long name matches that of already present one. 
        */
        void add(shared_ptr<option_description> desc);
        /** Adds a group of option description. This has the same
            effect as adding all option_descriptions in 'desc' 
            individually, except that output operator will show
            a separate group.
            Returns *this.
        */
        options_description& add(const options_description& desc);

        /** Find the maximum width of the option column, including options 
            in groups. */
        unsigned get_option_column_width() const;

    public:
        /** Returns an object of implementation-defined type suitable for adding
            options to options_description. The returned object will
            have overloaded operator() with parameter type matching 
            'option_description' constructors. Calling the operator will create
            new option_description instance and add it.
        */
        options_description_easy_init add_options();

        const option_description& find(const std::string& name, 
                                       bool approx, 
                                       bool long_ignore_case = false,
                                       bool short_ignore_case = false) const;

        const option_description* find_nothrow(const std::string& name, 
                                               bool approx,
                                               bool long_ignore_case = false,
                                               bool short_ignore_case = false) const;


        const std::vector< shared_ptr<option_description> >& options() const;

        /** Produces a human readable output of 'desc', listing options,
            their descriptions and allowed parameters. Other options_description
            instances previously passed to add will be output separately. */
        friend BOOST_PROGRAM_OPTIONS_DECL std::ostream& operator<<(std::ostream& os, 
                                             const options_description& desc);

        /** Outputs 'desc' to the specified stream, calling 'f' to output each
            option_description element. */
        void print(std::ostream& os, unsigned width = 0) const;

    private:
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1800))
        // prevent warning C4512: assignment operator could not be generated
        options_description& operator=(const options_description&);
#endif

        typedef std::map<std::string, int>::const_iterator name2index_iterator;
        typedef std::pair<name2index_iterator, name2index_iterator> 
            approximation_range;

        //approximation_range find_approximation(const std::string& prefix) const;

        std::string m_caption;
        const unsigned m_line_length;
        const unsigned m_min_description_length;
        
        // Data organization is chosen because:
        // - there could be two names for one option
        // - option_add_proxy needs to know the last added option
        std::vector< shared_ptr<option_description> > m_options;

        // Whether the option comes from one of declared groups.
#if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, BOOST_TESTED_AT(313))
        // vector<bool> is buggy there, see
        // http://support.microsoft.com/default.aspx?scid=kb;en-us;837698
        std::vector<char> belong_to_group;
#else
        std::vector<bool> belong_to_group;
#endif

        std::vector< shared_ptr<options_description> > groups;

    };

    /** Class thrown when duplicate option description is found. */
    class BOOST_PROGRAM_OPTIONS_DECL duplicate_option_error : public error {
    public:
        duplicate_option_error(const std::string& xwhat) : error(xwhat) {}
    };
}}

#if defined(BOOST_MSVC)
#   pragma warning (pop)
#endif

#endif

// Copyright Vladimir Prus 2002-2004.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_CONFIG_FILE_VP_2003_01_02
#define BOOST_CONFIG_FILE_VP_2003_01_02

#include <iosfwd>
#include <string>
#include <set>

#include <boost/noncopyable.hpp>
#include <boost/program_options/config.hpp>
#include <boost/program_options/option.hpp>
#include <boost/program_options/eof_iterator.hpp>

#include <boost/detail/workaround.hpp>
#include <boost/program_options/detail/convert.hpp>

#if BOOST_WORKAROUND(__DECCXX_VER, BOOST_TESTED_AT(60590042))
#include <istream> // std::getline
#endif

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/shared_ptr.hpp>

#ifdef BOOST_MSVC
# pragma warning(push)
# pragma warning(disable: 4251) // class XYZ needs to have dll-interface to be used by clients of class XYZ
#endif



namespace boost { namespace program_options { namespace detail {

    /** Standalone parser for config files in ini-line format.
        The parser is a model of single-pass lvalue iterator, and
        default constructor creates past-the-end-iterator. The typical usage is:
        config_file_iterator i(is, ... set of options ...), e;
        for(; i !=e; ++i) {
            *i;
        }
        
        Syntax conventions:

        - config file can not contain positional options
        - '#' is comment character: it is ignored together with
          the rest of the line.
        - variable assignments are in the form
          name '=' value.
          spaces around '=' are trimmed.
        - Section names are given in brackets. 

         The actual option name is constructed by combining current section
         name and specified option name, with dot between. If section_name 
         already contains dot at the end, new dot is not inserted. For example:
         @verbatim
         [gui.accessibility]
         visual_bell=yes
         @endverbatim
         will result in option "gui.accessibility.visual_bell" with value
         "yes" been returned.

         TODO: maybe, we should just accept a pointer to options_description
         class.
     */    
    class BOOST_PROGRAM_OPTIONS_DECL common_config_file_iterator
        : public eof_iterator<common_config_file_iterator, option>
    {
    public:
        common_config_file_iterator() { found_eof(); }
        common_config_file_iterator(
            const std::set<std::string>& allowed_options,
            bool allow_unregistered = false);

        virtual ~common_config_file_iterator() {}

    public: // Method required by eof_iterator
        
        void get();
        
#if BOOST_WORKAROUND(_MSC_VER, <= 1900)
        void decrement() {}
        void advance(difference_type) {}
#endif

    protected: // Stubs for derived classes

        // Obtains next line from the config file
        // Note: really, this design is a bit ugly
        // The most clean thing would be to pass 'line_iterator' to
        // constructor of this class, but to avoid templating this class
        // we'd need polymorphic iterator, which does not exist yet.
        virtual bool getline(std::string&) { return false; }
        
    private:
        /** Adds another allowed option. If the 'name' ends with
            '*', then all options with the same prefix are
            allowed. For example, if 'name' is 'foo*', then 'foo1' and
            'foo_bar' are allowed. */
        void add_option(const char* name);

        // Returns true if 's' is a registered option name.
        bool allowed_option(const std::string& s) const; 

        // That's probably too much data for iterator, since
        // it will be copied, but let's not bother for now.
        std::set<std::string> allowed_options;
        // Invariant: no element is prefix of other element.
        std::set<std::string> allowed_prefixes;
        std::string m_prefix;
        bool m_allow_unregistered;
    };

    template<class charT>
    class basic_config_file_iterator : public common_config_file_iterator {
    public:
        basic_config_file_iterator()
        {
            found_eof();
        }

        /** Creates a config file parser for the specified stream.            
        */
        basic_config_file_iterator(std::basic_istream<charT>& is, 
                                   const std::set<std::string>& allowed_options,
                                   bool allow_unregistered = false); 

    private: // base overrides

        bool getline(std::string&);

    private: // internal data
        shared_ptr<std::basic_istream<charT> > is;
    };
    
    typedef basic_config_file_iterator<char> config_file_iterator;
    typedef basic_config_file_iterator<wchar_t> wconfig_file_iterator;


    struct null_deleter
    {
        void operator()(void const *) const {}
    };


    template<class charT>
    basic_config_file_iterator<charT>::
    basic_config_file_iterator(std::basic_istream<charT>& is, 
                               const std::set<std::string>& allowed_options,
                               bool allow_unregistered)
    : common_config_file_iterator(allowed_options, allow_unregistered)
    {
        this->is.reset(&is, null_deleter());                 
        get();
    }

    // Specializing this function for wchar_t causes problems on
    // borland and vc7, as well as on metrowerks. On the first two
    // I don't know a workaround, so make use of 'to_internal' to
    // avoid specialization.
    template<class charT>
    bool
    basic_config_file_iterator<charT>::getline(std::string& s)
    {
        std::basic_string<charT> in;
        if (std::getline(*is, in)) {
            s = to_internal(in);
            return true;
        } else {
            return false;
        }
    }

    // Specialization is needed to workaround getline bug on Comeau.
#if BOOST_WORKAROUND(__COMO_VERSION__, BOOST_TESTED_AT(4303)) || \
        (defined(__sgi) && BOOST_WORKAROUND(_COMPILER_VERSION, BOOST_TESTED_AT(741)))
    template<>
    bool
    basic_config_file_iterator<wchar_t>::getline(std::string& s);
#endif

    

}}}

#ifdef BOOST_MSVC
# pragma warning(pop)
#endif

#endif

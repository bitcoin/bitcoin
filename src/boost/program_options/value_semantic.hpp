// Copyright Vladimir Prus 2004.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_VALUE_SEMANTIC_HPP_VP_2004_02_24
#define BOOST_VALUE_SEMANTIC_HPP_VP_2004_02_24

#include <boost/program_options/config.hpp>
#include <boost/program_options/errors.hpp>

#include <boost/any.hpp>
#include <boost/function/function1.hpp>
#include <boost/lexical_cast.hpp>

#include <string>
#include <vector>
#include <typeinfo>
#include <limits>

namespace boost { namespace program_options {

    /** Class which specifies how the option's value is to be parsed
        and converted into C++ types.
    */
    class BOOST_PROGRAM_OPTIONS_DECL value_semantic {
    public:
        /** Returns the name of the option. The name is only meaningful
            for automatic help message.
         */
        virtual std::string name() const = 0;

        /** The minimum number of tokens for this option that
            should be present on the command line. */
        virtual unsigned min_tokens() const = 0;

        /** The maximum number of tokens for this option that
            should be present on the command line. */
        virtual unsigned max_tokens() const = 0;

        /** Returns true if values from different sources should be composed.
            Otherwise, value from the first source is used and values from
            other sources are discarded.
        */
        virtual bool is_composing() const = 0;

        /** Returns true if value must be given. Non-optional value

        */
        virtual bool is_required() const = 0;
        
        /** Parses a group of tokens that specify a value of option.
            Stores the result in 'value_store', using whatever representation
            is desired. May be be called several times if value of the same
            option is specified more than once.
        */
        virtual void parse(boost::any& value_store, 
                           const std::vector<std::string>& new_tokens,
                           bool utf8) const 
            = 0;

        /** Called to assign default value to 'value_store'. Returns
            true if default value is assigned, and false if no default
            value exists. */
        virtual bool apply_default(boost::any& value_store) const = 0;
                                   
        /** Called when final value of an option is determined. 
        */
        virtual void notify(const boost::any& value_store) const = 0;
        
        virtual ~value_semantic() {}
    };

    /** Helper class which perform necessary character conversions in the 
        'parse' method and forwards the data further.
    */
    template<class charT>
    class value_semantic_codecvt_helper {
        // Nothing here. Specializations to follow.
    };

    /** Helper conversion class for values that accept ascii
        strings as input.
        Overrides the 'parse' method and defines new 'xparse'
        method taking std::string. Depending on whether input
        to parse is ascii or UTF8, will pass it to xparse unmodified,
        or with UTF8->ascii conversion.
    */
    template<>
    class BOOST_PROGRAM_OPTIONS_DECL 
    value_semantic_codecvt_helper<char> : public value_semantic {
    private: // base overrides
        void parse(boost::any& value_store, 
                   const std::vector<std::string>& new_tokens,
                   bool utf8) const;
    protected: // interface for derived classes.
        virtual void xparse(boost::any& value_store, 
                            const std::vector<std::string>& new_tokens) 
            const = 0;
    };

    /** Helper conversion class for values that accept ascii
        strings as input.
        Overrides the 'parse' method and defines new 'xparse'
        method taking std::wstring. Depending on whether input
        to parse is ascii or UTF8, will recode input to Unicode, or
        pass it unmodified.
    */
    template<>
    class BOOST_PROGRAM_OPTIONS_DECL
    value_semantic_codecvt_helper<wchar_t> : public value_semantic {
    private: // base overrides
        void parse(boost::any& value_store, 
                   const std::vector<std::string>& new_tokens,
                   bool utf8) const;
    protected: // interface for derived classes.
#if !defined(BOOST_NO_STD_WSTRING)
        virtual void xparse(boost::any& value_store, 
                            const std::vector<std::wstring>& new_tokens) 
            const = 0;
#endif
    };

    /** Class which specifies a simple handling of a value: the value will
        have string type and only one token is allowed. */    
    class BOOST_PROGRAM_OPTIONS_DECL 
    untyped_value : public value_semantic_codecvt_helper<char>  {
    public:
        untyped_value(bool zero_tokens = false)
        : m_zero_tokens(zero_tokens)
        {}

        std::string name() const;

        unsigned min_tokens() const;
        unsigned max_tokens() const;

        bool is_composing() const { return false; }

        bool is_required() const { return false; }
        
        /** If 'value_store' is already initialized, or new_tokens
            has more than one elements, throws. Otherwise, assigns
            the first string from 'new_tokens' to 'value_store', without
            any modifications.
         */
        void xparse(boost::any& value_store,
                    const std::vector<std::string>& new_tokens) const;

        /** Does nothing. */
        bool apply_default(boost::any&) const { return false; }

        /** Does nothing. */
        void notify(const boost::any&) const {}        
    private:
        bool m_zero_tokens;
    };

#ifndef BOOST_NO_RTTI
    /** Base class for all option that have a fixed type, and are
        willing to announce this type to the outside world.
        Any 'value_semantics' for which you want to find out the
        type can be dynamic_cast-ed to typed_value_base. If conversion
        succeeds, the 'type' method can be called.
    */
    class typed_value_base 
    {
    public:
        // Returns the type of the value described by this
        // object.
        virtual const std::type_info& value_type() const = 0;
        // Not really needed, since deletion from this
        // class is silly, but just in case.
        virtual ~typed_value_base() {}
    };
#endif


    /** Class which handles value of a specific type. */
    template<class T, class charT = char>
    class typed_value : public value_semantic_codecvt_helper<charT>
#ifndef BOOST_NO_RTTI
                      , public typed_value_base
#endif
    {
    public:
        /** Ctor. The 'store_to' parameter tells where to store
            the value when it's known. The parameter can be NULL. */
        typed_value(T* store_to) 
        : m_store_to(store_to), m_composing(false),
          m_implicit(false), m_multitoken(false),
          m_zero_tokens(false), m_required(false)
        {} 

        /** Specifies default value, which will be used
            if none is explicitly specified. The type 'T' should
            provide operator<< for ostream.
        */
        typed_value* default_value(const T& v)
        {
            m_default_value = boost::any(v);
            m_default_value_as_text = boost::lexical_cast<std::string>(v);
            return this;
        }

        /** Specifies default value, which will be used
            if none is explicitly specified. Unlike the above overload,
            the type 'T' need not provide operator<< for ostream,
            but textual representation of default value must be provided
            by the user.
        */
        typed_value* default_value(const T& v, const std::string& textual)
        {
            m_default_value = boost::any(v);
            m_default_value_as_text = textual;
            return this;
        }

        /** Specifies an implicit value, which will be used
            if the option is given, but without an adjacent value.
            Using this implies that an explicit value is optional,
        */
        typed_value* implicit_value(const T &v)
        {
            m_implicit_value = boost::any(v);
            m_implicit_value_as_text =
                boost::lexical_cast<std::string>(v);
            return this;
        }

        /** Specifies the name used to to the value in help message.  */
        typed_value* value_name(const std::string& name)
        {
            m_value_name = name;
            return this;
        }

        /** Specifies an implicit value, which will be used
            if the option is given, but without an adjacent value.
            Using this implies that an explicit value is optional, but if
            given, must be strictly adjacent to the option, i.e.: '-ovalue'
            or '--option=value'.  Giving '-o' or '--option' will cause the
            implicit value to be applied.
            Unlike the above overload, the type 'T' need not provide
            operator<< for ostream, but textual representation of default
            value must be provided by the user.
        */
        typed_value* implicit_value(const T &v, const std::string& textual)
        {
            m_implicit_value = boost::any(v);
            m_implicit_value_as_text = textual;
            return this;
        }

        /** Specifies a function to be called when the final value
            is determined. */
        typed_value* notifier(function1<void, const T&> f)
        {
            m_notifier = f;
            return this;
        }

        /** Specifies that the value is composing. See the 'is_composing' 
            method for explanation. 
        */
        typed_value* composing()
        {
            m_composing = true;
            return this;
        }

        /** Specifies that the value can span multiple tokens. 
        */
        typed_value* multitoken()
        {
            m_multitoken = true;
            return this;
        }

        /** Specifies that no tokens may be provided as the value of
            this option, which means that only presense of the option
            is significant. For such option to be useful, either the
            'validate' function should be specialized, or the 
            'implicit_value' method should be also used. In most
            cases, you can use the 'bool_switch' function instead of
            using this method. */
        typed_value* zero_tokens() 
        {
            m_zero_tokens = true;
            return this;
        }
            
        /** Specifies that the value must occur. */
        typed_value* required()
        {
            m_required = true;
            return this;
        }

    public: // value semantic overrides

        std::string name() const;

        bool is_composing() const { return m_composing; }

        unsigned min_tokens() const
        {
            if (m_zero_tokens || !m_implicit_value.empty()) {
                return 0;
            } else {
                return 1;
            }
        }

        unsigned max_tokens() const {
            if (m_multitoken) {
                return std::numeric_limits<unsigned>::max BOOST_PREVENT_MACRO_SUBSTITUTION();
            } else if (m_zero_tokens) {
                return 0;
            } else {
                return 1;
            }
        }

        bool is_required() const { return m_required; }

        /** Creates an instance of the 'validator' class and calls
            its operator() to perform the actual conversion. */
        void xparse(boost::any& value_store, 
                    const std::vector< std::basic_string<charT> >& new_tokens) 
            const;

        /** If default value was specified via previous call to 
            'default_value', stores that value into 'value_store'.
            Returns true if default value was stored.
        */
        virtual bool apply_default(boost::any& value_store) const
        {
            if (m_default_value.empty()) {
                return false;
            } else {
                value_store = m_default_value;
                return true;
            }
        }

        /** If an address of variable to store value was specified
            when creating *this, stores the value there. Otherwise,
            does nothing. */
        void notify(const boost::any& value_store) const;

    public: // typed_value_base overrides
        
#ifndef BOOST_NO_RTTI
        const std::type_info& value_type() const
        {
            return typeid(T);
        }
#endif
        

    private:
        T* m_store_to;
        
        // Default value is stored as boost::any and not
        // as boost::optional to avoid unnecessary instantiations.
        std::string m_value_name;
        boost::any m_default_value;
        std::string m_default_value_as_text;
        boost::any m_implicit_value;
        std::string m_implicit_value_as_text;
        bool m_composing, m_implicit, m_multitoken, m_zero_tokens, m_required;
        boost::function1<void, const T&> m_notifier;
    };


    /** Creates a typed_value<T> instance. This function is the primary
        method to create value_semantic instance for a specific type, which
        can later be passed to 'option_description' constructor.
        The second overload is used when it's additionally desired to store the 
        value of option into program variable.
    */
    template<class T>
    typed_value<T>*
    value();

    /** @overload 
    */
    template<class T>
    typed_value<T>*
    value(T* v);

    /** Creates a typed_value<T> instance. This function is the primary
        method to create value_semantic instance for a specific type, which
        can later be passed to 'option_description' constructor.
    */
    template<class T>
    typed_value<T, wchar_t>*
    wvalue();

    /** @overload   
    */
    template<class T>
    typed_value<T, wchar_t>*
    wvalue(T* v);

    /** Works the same way as the 'value<bool>' function, but the created
        value_semantic won't accept any explicit value. So, if the option 
        is present on the command line, the value will be 'true'.
    */
    BOOST_PROGRAM_OPTIONS_DECL typed_value<bool>*
    bool_switch();

    /** @overload
    */
    BOOST_PROGRAM_OPTIONS_DECL typed_value<bool>*    
    bool_switch(bool* v);

}}

#include "boost/program_options/detail/value_semantic.hpp"

#endif


//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_LOCALE_FORMAT_HPP_INCLUDED
#define BOOST_LOCALE_FORMAT_HPP_INCLUDED

#include <boost/locale/config.hpp>
#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4275 4251 4231 4660)
#endif
#include <boost/locale/message.hpp>
#include <boost/locale/formatting.hpp>
#include <boost/locale/hold_ptr.hpp>

#include <sstream>


namespace boost {
    namespace locale {
        
        ///
        /// \defgroup format Format
        ///
        /// This module provides printf like functionality integrated into iostreams and suitable for localization
        ///
        /// @{
        ///

        /// \cond INTERNAL
        namespace details {

            template<typename CharType>
            struct formattible {
                typedef std::basic_ostream<CharType> stream_type;
                typedef void (*writer_type)(stream_type &output,void const *ptr);

                formattible() :
                    pointer_(0),
                    writer_(&formattible::void_write)
                {
                }
                
                formattible(formattible const &other) :
                    pointer_(other.pointer_),
                    writer_(other.writer_)
                {
                }

                formattible const &operator=(formattible const &other)
                {
                    if(this != &other) {
                        pointer_=other.pointer_;
                        writer_=other.writer_;
                    }
                    return *this;
                }
                
                template<typename Type>
                formattible(Type const &value)
                {
                    pointer_ = static_cast<void const *>(&value);
                    writer_ = &write<Type>;
                }

                template<typename Type>
                formattible const &operator=(Type const &other)
                {
                    *this = formattible(other);
                    return *this;
                }

                friend stream_type &operator<<(stream_type &out,formattible const &fmt)
                {
                    fmt.writer_(out,fmt.pointer_);
                    return out;
                }

            private:
                static void void_write(stream_type &output,void const * /*ptr*/)
                {
                    CharType empty_string[1]={0};
                    output<<empty_string;
                }

                template<typename Type>
                static void write(stream_type &output,void const *ptr)
                {
                    output << *static_cast<Type const *>(ptr);
                }
                
                void const *pointer_;
                writer_type writer_;
            }; // formattible
    
            class BOOST_LOCALE_DECL format_parser  {
            public:
                format_parser(std::ios_base &ios,void *,void (*imbuer)(void *,std::locale const &));
                ~format_parser();
                
                unsigned get_position();
                
                void set_one_flag(std::string const &key,std::string const &value);

                template<typename CharType>
                void set_flag_with_str(std::string const &key,std::basic_string<CharType> const &value)
                {
                    if(key=="ftime" || key=="strftime") {
                        as::strftime(ios_);
                        ios_info::get(ios_).date_time_pattern(value);
                    }
                }
                void restore();
            private:
                void imbue(std::locale const &);
                format_parser(format_parser const &);
                void operator=(format_parser const &);

                std::ios_base &ios_;
                struct data;
                hold_ptr<data> d;
            };

        }

        /// \endcond

        ///
        /// \brief a printf like class that allows type-safe and locale aware message formatting
        ///
        /// This class creates a formatted message similar to printf or boost::format and receives
        /// formatted entries via operator %.
        ///
        /// For example
        /// \code
        ///  cout << format("Hello {1}, you are {2} years old") % name % age << endl;
        /// \endcode
        ///
        /// Formatting is enclosed between curly brackets \c { \c } and defined by a comma separated list of flags in the format key[=value]
        /// value may also be text included between single quotes \c ' that is used for special purposes where inclusion of non-ASCII
        /// text is allowed
        ///
        /// Including of literal \c { and \c } is possible by specifying double brackets \c {{ and \c }} accordingly.
        ///
        ///
        /// For example:
        ///
        /// \code 
        ///   cout << format("The height of water at {1,time} is {2,num=fixed,precision=3}") % time % height;
        /// \endcode
        ///
        /// The special key -- a number without a value defines the position of an input parameter.
        /// List of keys:
        /// -   \c [0-9]+ -- digits, the index of a formatted parameter -- mandatory key.
        /// -   \c num or \c number -- format a number. Optional values are:
        ///     -  \c hex -- display hexadecimal number
        ///     -  \c oct -- display in octal format
        ///     -  \c sci or \c scientific -- display in scientific format
        ///     -  \c fix or \c fixed -- display in fixed format
        ///     .      
        ///     For example \c number=sci
        /// -  \c cur or \c currency -- format currency. Optional values are:
        /// 
        ///     -  \c iso -- display using ISO currency symbol.
        ///     -  \c nat or \c national -- display using national currency symbol.
        ///     .
        /// -  \c per or \c percent -- format percent value.
        /// -  \c date, \c time , \c datetime or \c dt -- format date, time or date and time. Optional values are:
        ///     -  \c s or \c short -- display in short format
        ///     -  \c m or \c medium -- display in medium format.
        ///     -  \c l or \c long -- display in long format.
        ///     -  \c f or \c full -- display in full format.
        ///     .
        /// -  \c ftime with string (quoted) parameter -- display as with \c strftime see, \c as::ftime manipulator
        /// -  \c spell or \c spellout -- spell the number.
        /// -  \c ord or \c ordinal -- format ordinal number (1st, 2nd... etc)
        /// -  \c left or \c < -- align to left.
        /// -  \c right or \c > -- align to right.
        /// -  \c width or \c w -- set field width (requires parameter).
        /// -  \c precision or \c p -- set precision (requires parameter).
        /// -  \c locale -- with parameter -- switch locale for current operation. This command generates locale
        ///     with formatting facets giving more fine grained control of formatting. For example:
        ///    \code
        ///    cout << format("Today {1,date} ({1,date,locale=he_IL.UTF-8@calendar=hebrew,date} Hebrew Date)") % date;
        ///    \endcode
        /// -  \c timezone or \c tz -- the name of the timezone to display the time in. For example:\n
        ///    \code
        ///    cout << format("Time is: Local {1,time}, ({1,time,tz=EET} Eastern European Time)") % date;
        ///    \endcode
        /// -  \c local - display the time in local time
        /// -  \c gmt - display the time in UTC time scale
        ///    \code
        ///    cout << format("Local time is: {1,time,local}, universal time is {1,time,gmt}") % time;
        ///    \endcode
        /// 
        /// 
        /// Invalid formatting strings are slightly ignored. This would prevent from translator
        /// to crash the program in unexpected location.
        /// 
        template<typename CharType>
        class basic_format {
        public:
            typedef CharType char_type; ///< Underlying character type
            typedef basic_message<char_type> message_type; ///< The translation message type
            /// \cond INTERNAL
            typedef details::formattible<CharType> formattible_type; 
            /// \endcond 

            typedef std::basic_string<CharType> string_type; ///< string type for this type of character
            typedef std::basic_ostream<CharType> stream_type; ///< output stream type for this type of character
 

            ///
            /// Create a format class for \a format_string
            ///
            basic_format(string_type format_string) : 
                format_(format_string),
                translate_(false),
                parameters_count_(0)
            {
            }
            ///
            /// Create a format class using message \a trans. The message if translated first according
            /// to the rules of target locale and then interpreted as format string
            ///
            basic_format(message_type const &trans) : 
                message_(trans),
                translate_(true),
                parameters_count_(0)
            {
            }

            ///
            /// Add new parameter to format list. The object should be a type
            /// with defined expression out << object where \c out is \c std::basic_ostream.
            ///
            template<typename Formattible>
            basic_format &operator % (Formattible const &object)
            {
                add(formattible_type(object));
                return *this;
            }

            ///
            /// Format a string using a locale \a loc
            ///
            string_type str(std::locale const &loc = std::locale()) const
            {
                std::basic_ostringstream<CharType> buffer;
                buffer.imbue(loc);
                write(buffer);
                return buffer.str();
            }

            ///
            /// write a formatted string to output stream \a out using out's locale
            ///
            void write(stream_type &out) const
            {
                string_type format;
                if(translate_)
                    format = message_.str(out.getloc(),ios_info::get(out).domain_id());
                else
                    format = format_;
               
                format_output(out,format);

            }
                        
            
        private:

            class format_guard {
            public:
                format_guard(details::format_parser &fmt) : 
                    fmt_(&fmt),
                    restored_(false)
                {
                }
                void restore()
                {
                    if(restored_)
                        return;
                    fmt_->restore();
                    restored_ = true;
                }
                ~format_guard()
                {
                    try {
                        restore();
                    }
                    catch(...) {
                    }
                }
            private:
                details::format_parser *fmt_;
                bool restored_;
            };
            
            void format_output(stream_type &out,string_type const &sformat) const
            {
                char_type obrk='{';
                char_type cbrk='}';
                char_type eq='=';
                char_type comma=',';
                char_type quote='\'';

                size_t pos = 0;
                size_t size=sformat.size();
                CharType const *format=sformat.c_str();
                while(format[pos]!=0) {
                    if(format[pos] != obrk) {
                        if(format[pos]==cbrk && format[pos+1]==cbrk) {
                            out << cbrk;
                            pos+=2;
                        }
                        else {
                            out<<format[pos];
                            pos++;
                        }
                        continue;
                    }

                    if(pos+1 < size && format[pos+1]==obrk) {
                        out << obrk;
                        pos+=2;
                        continue;
                    }
                    pos++;
                  
                    details::format_parser fmt(out,static_cast<void *>(&out),&basic_format::imbue_locale);

                    format_guard guard(fmt);

                    while(pos < size) { 
                        std::string key;
                        std::string svalue;
                        string_type value;
                        bool use_svalue = true;
                        for(;format[pos];pos++) {
                            char_type c=format[pos];
                            if(c==comma || c==eq || c==cbrk)
                                break;
                            else {
                                key+=static_cast<char>(c);
                            }
                        }

                        if(format[pos]==eq) {
                            pos++;
                            if(format[pos]==quote) {
                                pos++;
                                use_svalue = false;
                                while(format[pos]) {
                                    if(format[pos]==quote) {
                                        if(format[pos+1]==quote) {
                                            value+=quote;
                                            pos+=2;
                                        }
                                        else {
                                            pos++;
                                            break;
                                        }
                                    }
                                    else {
                                        value+=format[pos];
                                        pos++;
                                    }
                                }
                            }
                            else {
                                char_type c;
                                while((c=format[pos])!=0 && c!=comma && c!=cbrk) {
                                    svalue+=static_cast<char>(c);
                                    pos++;
                                }
                            }
                        }

                        if(use_svalue) {
                            fmt.set_one_flag(key,svalue);
                        }
                        else 
                            fmt.set_flag_with_str(key,value);
                        
                        if(format[pos]==comma) {
                            pos++;
                            continue;
                        }
                        else if(format[pos]==cbrk)  {
                            unsigned position = fmt.get_position();
                            out << get(position);
                            guard.restore();
                            pos++;
                            break;
                        }
                        else {                        
                            guard.restore();
                            break;
                        }
                    }
                }
            }

      
            //
            // Non-copyable 
            //
            basic_format(basic_format const &other);
            void operator=(basic_format const &other);

            void add(formattible_type const &param)
            {
                if(parameters_count_ >= base_params_)
                    ext_params_.push_back(param);
                else 
                    parameters_[parameters_count_] = param;
                parameters_count_++;
            }

            formattible_type get(unsigned id) const
            {
                if(id >= parameters_count_)
                    return formattible_type();
                else if(id >= base_params_)
                    return ext_params_[id - base_params_];
                else
                    return parameters_[id];
            }

            static void imbue_locale(void *ptr,std::locale const &l)
            {
                reinterpret_cast<stream_type *>(ptr)->imbue(l);
            }



            static unsigned const base_params_ = 8;
            
            message_type message_;
            string_type format_;
            bool translate_;


            formattible_type parameters_[base_params_];
            unsigned parameters_count_;
            std::vector<formattible_type> ext_params_;
        };

        ///
        /// Write formatted message to stream.
        ///
        /// This operator actually causes actual text formatting. It uses the locale of \a out stream
        ///
        template<typename CharType>
        std::basic_ostream<CharType> &operator<<(std::basic_ostream<CharType> &out,basic_format<CharType> const &fmt)
        {
            fmt.write(out);
            return out;
        }


        ///
        /// Definition of char based format
        ///
        typedef basic_format<char> format;

        ///
        /// Definition of wchar_t based format
        ///
        typedef basic_format<wchar_t> wformat;

        #ifdef BOOST_LOCALE_ENABLE_CHAR16_T
        ///
        /// Definition of char16_t based format
        ///
        typedef basic_format<char16_t> u16format;
        #endif

        #ifdef BOOST_LOCALE_ENABLE_CHAR32_T
        ///
        /// Definition of char32_t based format
        ///
        typedef basic_format<char32_t> u32format;
        #endif

        ///
        /// @}
        ///

    }
}

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif

///
/// \example hello.cpp
///
/// Basic example of using various functions provided by this library
///
/// \example whello.cpp
///
/// Basic example of using various functions with wide strings provided by this library
///
///

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4


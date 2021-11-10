//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_LOCALE_MESSAGE_HPP_INCLUDED
#define BOOST_LOCALE_MESSAGE_HPP_INCLUDED

#include <boost/locale/config.hpp>
#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4275 4251 4231 4660)
#endif
#include <locale>
#include <string>
#include <vector>
#include <set>
#include <memory>
#include <boost/locale/formatting.hpp>

// glibc < 2.3.4 declares those as macros if compiled with optimization turned on
#ifdef gettext
#  undef gettext
#  undef ngettext
#  undef dgettext
#  undef dngettext
#endif

namespace boost {
    namespace locale {
        ///
        /// \defgroup message Message Formatting (translation) 
        ///
        ///This module provides message translation functionality, i.e. allow your application to speak native language
        ///
        /// @{
        /// 

        /// \cond INTERNAL 

        template<typename CharType>
        struct base_message_format: public std::locale::facet
        {
        };
       
        /// \endcond
       
        ///
        /// \brief This facet provides message formatting abilities
        ///
        template<typename CharType>
        class message_format : public base_message_format<CharType>
        {
        public:

            ///
            /// Character type
            ///
            typedef CharType char_type;
            ///
            /// String type
            ///
            typedef std::basic_string<CharType> string_type;

            ///
            /// Default constructor
            ///
            message_format(size_t refs = 0) : 
                base_message_format<CharType>(refs)
            {
            }

            ///
            /// This function returns a pointer to the string for a message defined by a \a context
            /// and identification string \a id. Both create a single key for message lookup in
            /// a domain defined by \a domain_id.
            ///
            /// If \a context is NULL it is not considered to be a part of the key
            ///
            /// If a translated string is found, it is returned, otherwise NULL is returned
            /// 
            ///
            virtual char_type const *get(int domain_id,char_type const *context,char_type const *id) const = 0;
            ///
            /// This function returns a pointer to the string for a plural message defined by a \a context
            /// and identification string \a single_id. 
            ///
            /// If \a context is NULL it is not considered to be a part of the key
            ///
            /// Both create a single key for message lookup in
            /// a domain defined \a domain_id. \a n is used to pick the correct translation string for a specific
            /// number.
            ///
            /// If a translated string is found, it is returned, otherwise NULL is returned
            /// 
            ///
            virtual char_type const *get(int domain_id,char_type const *context,char_type const *single_id,int n) const = 0;

            ///
            /// Convert a string that defines \a domain to the integer id used by \a get functions
            ///
            virtual int domain(std::string const &domain) const = 0;

            ///
            /// Convert the string \a msg to target locale's encoding. If \a msg is already
            /// in target encoding it would be returned otherwise the converted
            /// string is stored in temporary \a buffer and buffer.c_str() is returned.
            ///
            /// Note: for char_type that is char16_t, char32_t and wchar_t it is no-op, returns
            /// msg
            ///
            virtual char_type const *convert(char_type const *msg,string_type &buffer) const = 0;

#if defined (__SUNPRO_CC) && defined (_RWSTD_VER)
            std::locale::id& __get_id (void) const { return id; }
#endif
        protected:
            virtual ~message_format()
            {
            }

        };
        
        /// \cond INTERNAL

        namespace details {
            inline bool is_us_ascii_char(char c)
            {
                // works for null terminated strings regardless char "signness"
                return 0<c && c<0x7F; 
            }
            inline bool is_us_ascii_string(char const *msg)
            {
                while(*msg) {
                    if(!is_us_ascii_char(*msg++))
                        return false;
                }
                return true;
            }

            template<typename CharType>
            struct string_cast_traits {
                static CharType const *cast(CharType const *msg,std::basic_string<CharType> &/*unused*/)
                {
                    return msg;
                }
            };

            template<>
            struct string_cast_traits<char> {
                static char const *cast(char const *msg,std::string &buffer)
                {
                    if(is_us_ascii_string(msg))
                        return msg;
                    buffer.reserve(strlen(msg));
                    char c;
                    while((c=*msg++)!=0) {
                        if(is_us_ascii_char(c))
                            buffer+=c;
                    }
                    return buffer.c_str();
                }
            };
        } // details

        /// \endcond

        ///
        /// \brief This class represents a message that can be converted to a specific locale message
        ///
        /// It holds the original ASCII string that is queried in the dictionary when converting to the output string.
        /// The created string may be UTF-8, UTF-16, UTF-32 or other 8-bit encoded string according to the target 
        /// character type and locale encoding.
        ///
        template<typename CharType>
        class basic_message {
        public:

            typedef CharType char_type; ///< The character this message object is used with
            typedef std::basic_string<char_type> string_type;   ///< The string type this object can be used with
            typedef message_format<char_type> facet_type;   ///< The type of the facet the messages are fetched with

            ///
            /// Create default empty message
            /// 
            basic_message() :
                n_(0),
                c_id_(0),
                c_context_(0),
                c_plural_(0)
            {
            }

            ///
            /// Create a simple message from 0 terminated string. The string should exist
            /// until the message is destroyed. Generally useful with static constant strings
            /// 
            explicit basic_message(char_type const *id) :
                n_(0),
                c_id_(id),
                c_context_(0),
                c_plural_(0)
            {
            }

            ///
            /// Create a simple plural form message from 0 terminated strings. The strings should exist
            /// until the message is destroyed. Generally useful with static constant strings.
            ///
            /// \a n is the number, \a single and \a plural are singular and plural forms of the message
            /// 
            explicit basic_message(char_type const *single,char_type const *plural,int n) :
                n_(n),
                c_id_(single),
                c_context_(0),
                c_plural_(plural)
            {
            }

            ///
            /// Create a simple message from 0 terminated strings, with context
            /// information. The string should exist
            /// until the message is destroyed. Generally useful with static constant strings
            /// 
            explicit basic_message(char_type const *context,char_type const *id) :
                n_(0),
                c_id_(id),
                c_context_(context),
                c_plural_(0)
            {
            }

            ///
            /// Create a simple plural form message from 0 terminated strings, with context. The strings should exist
            /// until the message is destroyed. Generally useful with static constant strings.
            ///
            /// \a n is the number, \a single and \a plural are singular and plural forms of the message
            /// 
            explicit basic_message(char_type const *context,char_type const *single,char_type const *plural,int n) :
                n_(n),
                c_id_(single),
                c_context_(context),
                c_plural_(plural)
            {
            }


            ///
            /// Create a simple message from a string.
            ///
            explicit basic_message(string_type const &id) :
                n_(0),
                c_id_(0),
                c_context_(0),
                c_plural_(0),
                id_(id)
            {
            }

            ///
            /// Create a simple plural form message from strings.
            ///
            /// \a n is the number, \a single and \a plural are single and plural forms of the message
            /// 
            explicit basic_message(string_type const &single,string_type const &plural,int number) :
                n_(number),
                c_id_(0),
                c_context_(0),
                c_plural_(0),
                id_(single),
                plural_(plural)
            {
            }

            ///
            /// Create a simple message from a string with context.
            ///
            explicit basic_message(string_type const &context,string_type const &id) :
                n_(0),
                c_id_(0),
                c_context_(0),
                c_plural_(0),
                id_(id),
                context_(context)
            {
            }

            ///
            /// Create a simple plural form message from strings.
            ///
            /// \a n is the number, \a single and \a plural are single and plural forms of the message
            /// 
            explicit basic_message(string_type const &context,string_type const &single,string_type const &plural,int number) :
                n_(number),
                c_id_(0),
                c_context_(0),
                c_plural_(0),
                id_(single),
                context_(context),
                plural_(plural)
            {
            }

            ///
            /// Copy an object
            ///
            basic_message(basic_message const &other) :
                n_(other.n_),
                c_id_(other.c_id_),
                c_context_(other.c_context_),
                c_plural_(other.c_plural_),
                id_(other.id_),
                context_(other.context_),
                plural_(other.plural_)
            {
            }

            ///
            /// Assign other message object to this one
            ///
            basic_message const &operator=(basic_message const &other)
            {
                if(this==&other) {
                    return *this;
                }
                basic_message tmp(other);
                swap(tmp);
                return *this;
            }

            ///
            /// Swap two message objects
            ///
            void swap(basic_message &other)
            {
                std::swap(n_,other.n_);
                std::swap(c_id_,other.c_id_);
                std::swap(c_context_,other.c_context_);
                std::swap(c_plural_,other.c_plural_);

                id_.swap(other.id_);
                context_.swap(other.context_);
                plural_.swap(other.plural_);
            }

            ///
            /// Message class can be explicitly converted to string class
            ///

            operator string_type () const
            {
                return str();
            }

            ///
            /// Translate message to a string in the default global locale, using default domain
            ///
            string_type str() const
            {
                std::locale loc;
                return str(loc,0);
            }
            
            ///
            /// Translate message to a string in the locale \a locale, using default domain
            ///
            string_type str(std::locale const &locale) const
            {
                return str(locale,0);
            }
           
            ///
            /// Translate message to a string using locale \a locale and message domain  \a domain_id
            /// 
            string_type str(std::locale const &locale,std::string const &domain_id) const
            {
                int id=0;
                if(std::has_facet<facet_type>(locale))
                    id=std::use_facet<facet_type>(locale).domain(domain_id);
                return str(locale,id);
            }

            ///
            /// Translate message to a string using the default locale and message domain  \a domain_id
            /// 
            string_type str(std::string const &domain_id) const
            {
                int id=0;
                std::locale loc;
                if(std::has_facet<facet_type>(loc))
                    id=std::use_facet<facet_type>(loc).domain(domain_id);
                return str(loc,id);
            }

            
            ///
            /// Translate message to a string using locale \a loc and message domain index  \a id
            /// 
            string_type str(std::locale const &loc,int id) const
            {
                string_type buffer;                
                char_type const *ptr = write(loc,id,buffer);
                if(ptr == buffer.c_str())
                    return buffer;
                else
                    buffer = ptr;
                return buffer;
            }


            ///
            /// Translate message and write to stream \a out, using imbued locale and domain set to the 
            /// stream
            ///
            void write(std::basic_ostream<char_type> &out) const
            {
                std::locale const &loc = out.getloc();
                int id = ios_info::get(out).domain_id();
                string_type buffer;
                out << write(loc,id,buffer);
            }

        private:
            char_type const *plural() const
            {
                if(c_plural_)
                    return c_plural_;
                if(plural_.empty())
                    return 0;
                return plural_.c_str();
            }
            char_type const *context() const
            {
                if(c_context_)
                    return c_context_;
                if(context_.empty())
                    return 0;
                return context_.c_str();
            }

            char_type const *id() const
            {
                return c_id_ ? c_id_ : id_.c_str();
            }
            
            char_type const *write(std::locale const &loc,int domain_id,string_type &buffer) const
            {
                char_type const *translated = 0;
                static const char_type empty_string[1] = {0};

                char_type const *id = this->id();
                char_type const *context = this->context();
                char_type const *plural = this->plural();
                
                if(*id == 0)
                    return empty_string;
                
                facet_type const *facet = 0;
                if(std::has_facet<facet_type>(loc))
                    facet = &std::use_facet<facet_type>(loc);

                if(facet) { 
                    if(!plural) {
                        translated = facet->get(domain_id,context,id);
                    }
                    else {
                        translated = facet->get(domain_id,context,id,n_);
                    }
                }

                if(!translated) {
                    char_type const *msg = plural ? ( n_ == 1 ? id : plural) : id;

                    if(facet) {
                        translated = facet->convert(msg,buffer);
                    }
                    else {
                        translated = details::string_cast_traits<char_type>::cast(msg,buffer);
                    }
                }
                return translated;
            }

            /// members

            int n_;
            char_type const *c_id_;
            char_type const *c_context_;
            char_type const *c_plural_;
            string_type id_;
            string_type context_;
            string_type plural_;
        };


        ///
        /// Convenience typedef for char
        ///
        typedef basic_message<char> message;
        ///
        /// Convenience typedef for wchar_t
        ///
        typedef basic_message<wchar_t> wmessage;
        #ifdef BOOST_LOCALE_ENABLE_CHAR16_T
        ///
        /// Convenience typedef for char16_t
        ///
        typedef basic_message<char16_t> u16message;
        #endif
        #ifdef BOOST_LOCALE_ENABLE_CHAR32_T
        ///
        /// Convenience typedef for char32_t
        ///
        typedef basic_message<char32_t> u32message;
        #endif

        ///
        /// Translate message \a msg and write it to stream
        ///
        template<typename CharType>
        std::basic_ostream<CharType> &operator<<(std::basic_ostream<CharType> &out,basic_message<CharType> const &msg)
        {
            msg.write(out);
            return out;
        }

        ///
        /// \anchor boost_locale_translate_family \name Indirect message translation function family
        /// @{

        ///
        /// \brief Translate a message, \a msg is not copied 
        ///
        template<typename CharType>
        inline basic_message<CharType> translate(CharType const *msg)
        {
            return basic_message<CharType>(msg);
        }
        ///
        /// \brief Translate a message in context, \a msg and \a context are not copied 
        ///
        template<typename CharType>
        inline basic_message<CharType> translate(   CharType const *context,
                                                    CharType const *msg)
        {
            return basic_message<CharType>(context,msg);
        }
        ///
        /// \brief Translate a plural message form, \a single and \a plural are not copied 
        ///
        template<typename CharType>
        inline basic_message<CharType> translate(   CharType const *single,
                                                    CharType const *plural,
                                                    int n)
        {
            return basic_message<CharType>(single,plural,n);
        }
        ///
        /// \brief Translate a plural message from in constext, \a context, \a single and \a plural are not copied 
        ///
        template<typename CharType>
        inline basic_message<CharType> translate(   CharType const *context,
                                                    CharType const *single,
                                                    CharType const *plural,
                                                    int n)
        {
            return basic_message<CharType>(context,single,plural,n);
        }
        
        ///
        /// \brief Translate a message, \a msg is copied 
        ///
        template<typename CharType>
        inline basic_message<CharType> translate(std::basic_string<CharType> const &msg)
        {
            return basic_message<CharType>(msg);
        }
        
        ///
        /// \brief Translate a message in context,\a context and \a msg is copied 
        ///
        template<typename CharType>
        inline basic_message<CharType> translate(   std::basic_string<CharType> const &context,
                                                    std::basic_string<CharType> const &msg)
        {
            return basic_message<CharType>(context,msg);
        }
        ///
        /// \brief Translate a plural message form in constext, \a context, \a single and \a plural are copied 
        ///
        template<typename CharType>
        inline basic_message<CharType> translate(   std::basic_string<CharType> const &context,
                                                    std::basic_string<CharType> const &single,
                                                    std::basic_string<CharType> const &plural,
                                                    int n)
        {
            return basic_message<CharType>(context,single,plural,n);
        }

        ///
        /// \brief Translate a plural message form, \a single and \a plural are copied 
        ///

        template<typename CharType>
        inline basic_message<CharType> translate(   std::basic_string<CharType> const &single,
                                                    std::basic_string<CharType> const &plural,
                                                    int n)
        {
            return basic_message<CharType>(single,plural,n);
        }

        /// @}

        /// 
        /// \anchor boost_locale_gettext_family \name Direct message translation functions family 
        ///

        ///
        /// Translate message \a id according to locale \a loc
        ///
        template<typename CharType>
        std::basic_string<CharType> gettext(CharType const *id,
                                            std::locale const &loc=std::locale())
        {
            return basic_message<CharType>(id).str(loc);
        }
        ///
        /// Translate plural form according to locale \a loc
        ///
        template<typename CharType>
        std::basic_string<CharType> ngettext(   CharType const *s,
                                                CharType const *p,
                                                int n,
                                                std::locale const &loc=std::locale())
        {
            return basic_message<CharType>(s,p,n).str(loc);
        }
        ///
        /// Translate message \a id according to locale \a loc in domain \a domain
        ///
        template<typename CharType>
        std::basic_string<CharType>  dgettext(  char const *domain,
                                                CharType const *id,
                                                std::locale const &loc=std::locale())
        {
            return basic_message<CharType>(id).str(loc,domain);
        }

        ///
        /// Translate plural form according to locale \a loc in domain \a domain
        ///
        template<typename CharType>
        std::basic_string<CharType>  dngettext( char const *domain,
                                                CharType const *s,
                                                CharType const *p,
                                                int n,
                                                std::locale const &loc=std::locale())
        {
            return basic_message<CharType>(s,p,n).str(loc,domain);
        }
        ///
        /// Translate message \a id according to locale \a loc in context \a context
        ///
        template<typename CharType>
        std::basic_string<CharType>  pgettext(  CharType const *context,
                                                CharType const *id,
                                                std::locale const &loc=std::locale())
        {
            return basic_message<CharType>(context,id).str(loc);
        }
        ///
        /// Translate plural form according to locale \a loc in context \a context
        ///
        template<typename CharType>
        std::basic_string<CharType>  npgettext( CharType const *context,
                                                CharType const *s,
                                                CharType const *p,
                                                int n,
                                                std::locale const &loc=std::locale())
        {
            return basic_message<CharType>(context,s,p,n).str(loc);
        }
        ///
        /// Translate message \a id according to locale \a loc in domain \a domain in context \a context
        ///
        template<typename CharType>
        std::basic_string<CharType>  dpgettext( char const *domain,
                                                CharType const *context,
                                                CharType const *id,
                                                std::locale const &loc=std::locale())
        {
            return basic_message<CharType>(context,id).str(loc,domain);
        }
        ///
        /// Translate plural form according to locale \a loc in domain \a domain in context \a context
        ///
        template<typename CharType>
        std::basic_string<CharType>  dnpgettext(char const *domain,
                                                CharType const *context,
                                                CharType const *s,
                                                CharType const *p,
                                                int n,
                                                std::locale const &loc=std::locale())
        {
            return basic_message<CharType>(context,s,p,n).str(loc,domain);
        }

        ///
        /// \cond INTERNAL
        ///
        
        template<>
        struct BOOST_LOCALE_DECL base_message_format<char> : public std::locale::facet 
        {
            base_message_format(size_t refs = 0) : std::locale::facet(refs)
            {
            }
            static std::locale::id id;
        };
        
        template<>
        struct BOOST_LOCALE_DECL base_message_format<wchar_t> : public std::locale::facet 
        {
            base_message_format(size_t refs = 0) : std::locale::facet(refs)
            {
            }
            static std::locale::id id;
        };
        
        #ifdef BOOST_LOCALE_ENABLE_CHAR16_T

        template<>
        struct BOOST_LOCALE_DECL base_message_format<char16_t> : public std::locale::facet 
        {
            base_message_format(size_t refs = 0) : std::locale::facet(refs)
            {
            }
            static std::locale::id id;
        };

        #endif

        #ifdef BOOST_LOCALE_ENABLE_CHAR32_T

        template<>
        struct BOOST_LOCALE_DECL base_message_format<char32_t> : public std::locale::facet 
        {
            base_message_format(size_t refs = 0) : std::locale::facet(refs)
            {
            }
            static std::locale::id id;
        };
        
        #endif

        /// \endcond

        ///
        /// @}
        ///

        namespace as {
            /// \cond INTERNAL
            namespace details {
                struct set_domain {
                    std::string domain_id;
                };
                template<typename CharType>
                std::basic_ostream<CharType> &operator<<(std::basic_ostream<CharType> &out, set_domain const &dom)
                {
                    int id = std::use_facet<message_format<CharType> >(out.getloc()).domain(dom.domain_id);
                    ios_info::get(out).domain_id(id);
                    return out;
                }
            } // details
            /// \endcond

            ///
            /// \addtogroup manipulators
            ///
            /// @{
            
            ///
            /// Manipulator for switching message domain in ostream,
            ///
            /// \note The returned object throws std::bad_cast if the I/O stream does not have \ref message_format facet installed
            /// 
            inline 
            #ifdef BOOST_LOCALE_DOXYGEN
            unspecified_type
            #else
            details::set_domain 
            #endif
            domain(std::string const &id)
            {
                details::set_domain tmp = { id };
                return tmp;
            }
            /// @}
        } // as
    } // locale 
} // boost

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif


#endif

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4


/*=============================================================================
    Copyright (c) 1999-2003 Jeremiah Willcock
    Copyright (c) 1999-2003 Jaakko Jarvi
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_MANIP_05052005_1200)
#define FUSION_MANIP_05052005_1200

#include <boost/fusion/support/config.hpp>
#include <boost/config.hpp>
#include <string>
#include <vector>
#include <cctype>

// Tuple I/O manipulators

#define FUSION_GET_CHAR_TYPE(T) typename T::char_type
#define FUSION_GET_TRAITS_TYPE(T) typename T::traits_type

#define FUSION_STRING_OF_STREAM(Stream)                                         \
    std::basic_string<                                                          \
        FUSION_GET_CHAR_TYPE(Stream)                                            \
      , FUSION_GET_TRAITS_TYPE(Stream)                                          \
    >

//$$$ these should be part of the public API$$$
//$$$ rename tuple_open, tuple_close and tuple_delimiter to 
//    open, close and delimeter and add these synonyms to the
//    TR1 tuple module.

namespace boost { namespace fusion
{
    namespace detail
    {
        template <typename Tag>
        int get_xalloc_index(Tag* = 0)
        {
            // each Tag will have a unique index
            static int index = std::ios::xalloc();
            return index;
        }

        template <typename Stream, typename Tag, typename T>
        struct stream_data
        {
            struct arena
            {
                ~arena()
                {
                    for (
                        typename std::vector<T*>::iterator i = data.begin()
                      ; i != data.end()
                      ; ++i)
                    {
                        delete *i;
                    }
                }

                std::vector<T*> data;
            };

            static void attach(Stream& stream, T const& data)
            {
                static arena ar; // our arena
                ar.data.push_back(new T(data));
                stream.pword(get_xalloc_index<Tag>()) = ar.data.back();
            }

            static T const* get(Stream& stream)
            {
                return (T const*)stream.pword(get_xalloc_index<Tag>());
            }
        };

        template <typename Tag, typename Stream>
        class string_ios_manip
        {
        public:

            typedef FUSION_STRING_OF_STREAM(Stream) string_type;

            typedef stream_data<Stream, Tag, string_type> stream_data_t;

            string_ios_manip(Stream& str_)
                : stream(str_)
            {}

            void
            set(string_type const& s)
            {
                stream_data_t::attach(stream, s);
            }

            void
            print(char const* default_) const
            {
                // print a delimiter
                string_type const* p = stream_data_t::get(stream);
                if (p)
                    stream << *p;
                else
                    stream << default_;
            }

            void
            read(char const* default_) const
            {
                // read a delimiter
                string_type const* p = stream_data_t::get(stream);
                std::ws(stream);

                if (p)
                {
                    typedef typename string_type::const_iterator iterator;
                    for (iterator i = p->begin(); i != p->end(); ++i)
                        check_delim(*i);
                }
                else
                {
                    while (*default_)
                        check_delim(*default_++);
                }
            }

        private:

            template <typename Char>
            void
            check_delim(Char c) const
            {
                using namespace std;
                if (!isspace(c))
                {
                    if (stream.get() != c)
                    {
                        stream.unget();
                        stream.setstate(std::ios::failbit);
                    }
                }
            }

            Stream& stream;

            // silence MSVC warning C4512: assignment operator could not be generated
            BOOST_DELETED_FUNCTION(string_ios_manip& operator= (string_ios_manip const&))
        };

    } // detail


#if defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)

#define STD_TUPLE_DEFINE_MANIPULATOR_FUNCTIONS(name)                            \
    template <typename Char, typename Traits>                                   \
    inline detail::name##_type<Char, Traits>                                    \
    name(const std::basic_string<Char, Traits>& s)                              \
    {                                                                           \
        return detail::name##_type<Char, Traits>(s);                            \
    }                                                                           \
                                                                                \
    inline detail::name##_type<char>                                            \
    name(char const* s)                                                         \
    {                                                                           \
        return detail::name##_type<char>(std::basic_string<char>(s));           \
    }                                                                           \
                                                                                \
    inline detail::name##_type<wchar_t>                                         \
    name(wchar_t const* s)                                                      \
    {                                                                           \
        return detail::name##_type<wchar_t>(std::basic_string<wchar_t>(s));     \
    }                                                                           \
                                                                                \
    inline detail::name##_type<char>                                            \
    name(char c)                                                                \
    {                                                                           \
        return detail::name##_type<char>(std::basic_string<char>(1, c));        \
    }                                                                           \
                                                                                \
    inline detail::name##_type<wchar_t>                                         \
    name(wchar_t c)                                                             \
    {                                                                           \
        return detail::name##_type<wchar_t>(std::basic_string<wchar_t>(1, c));  \
    }

#else // defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)

#define STD_TUPLE_DEFINE_MANIPULATOR_FUNCTIONS(name)                            \
    template <typename Char, typename Traits>                                   \
    inline detail::name##_type<Char, Traits>                                    \
    name(const std::basic_string<Char, Traits>& s)                              \
    {                                                                           \
        return detail::name##_type<Char, Traits>(s);                            \
    }                                                                           \
                                                                                \
    template <typename Char>                                                    \
    inline detail::name##_type<Char>                                            \
    name(Char s[])                                                              \
    {                                                                           \
        return detail::name##_type<Char>(std::basic_string<Char>(s));           \
    }                                                                           \
                                                                                \
    template <typename Char>                                                    \
    inline detail::name##_type<Char>                                            \
    name(Char const s[])                                                        \
    {                                                                           \
        return detail::name##_type<Char>(std::basic_string<Char>(s));           \
    }                                                                           \
                                                                                \
    template <typename Char>                                                    \
    inline detail::name##_type<Char>                                            \
    name(Char c)                                                                \
    {                                                                           \
        return detail::name##_type<Char>(std::basic_string<Char>(1, c));        \
    }

#endif

#define STD_TUPLE_DEFINE_MANIPULATOR(name)                                      \
    namespace detail                                                            \
    {                                                                           \
        struct name##_tag;                                                      \
                                                                                \
        template <typename Char, typename Traits = std::char_traits<Char> >     \
        struct name##_type                                                      \
        {                                                                       \
            typedef std::basic_string<Char, Traits> string_type;                \
            string_type data;                                                   \
            name##_type(const string_type& d): data(d) {}                       \
        };                                                                      \
                                                                                \
        template <typename Stream, typename Char, typename Traits>              \
        Stream& operator>>(Stream& s, const name##_type<Char,Traits>& m)        \
        {                                                                       \
            string_ios_manip<name##_tag, Stream> manip(s);                      \
            manip.set(m.data);                                                  \
            return s;                                                           \
        }                                                                       \
                                                                                \
        template <typename Stream, typename Char, typename Traits>              \
        Stream& operator<<(Stream& s, const name##_type<Char,Traits>& m)        \
        {                                                                       \
            string_ios_manip<name##_tag, Stream> manip(s);                      \
            manip.set(m.data);                                                  \
            return s;                                                           \
        }                                                                       \
    }                                                                           \


    STD_TUPLE_DEFINE_MANIPULATOR(tuple_open)
    STD_TUPLE_DEFINE_MANIPULATOR(tuple_close)
    STD_TUPLE_DEFINE_MANIPULATOR(tuple_delimiter)

    STD_TUPLE_DEFINE_MANIPULATOR_FUNCTIONS(tuple_open)
    STD_TUPLE_DEFINE_MANIPULATOR_FUNCTIONS(tuple_close)
    STD_TUPLE_DEFINE_MANIPULATOR_FUNCTIONS(tuple_delimiter)

#undef STD_TUPLE_DEFINE_MANIPULATOR
#undef STD_TUPLE_DEFINE_MANIPULATOR_FUNCTIONS
#undef FUSION_STRING_OF_STREAM
#undef FUSION_GET_CHAR_TYPE
#undef FUSION_GET_TRAITS_TYPE

}}

#endif

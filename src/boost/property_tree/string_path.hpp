// ----------------------------------------------------------------------------
// Copyright (C) 2009 Sebastian Redl
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------

#ifndef BOOST_PROPERTY_TREE_STRING_PATH_HPP_INCLUDED
#define BOOST_PROPERTY_TREE_STRING_PATH_HPP_INCLUDED

#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/property_tree/id_translator.hpp>
#include <boost/property_tree/exceptions.hpp>
#include <boost/property_tree/detail/ptree_utils.hpp>

#include <boost/static_assert.hpp>
#include <boost/assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/optional.hpp>
#include <boost/throw_exception.hpp>
#include <algorithm>
#include <string>
#include <iterator>

namespace boost { namespace property_tree
{
    namespace detail
    {
        template <typename Sequence, typename Iterator>
        void append_and_preserve_iter(Sequence &s, const Sequence &r,
                                      Iterator &, std::forward_iterator_tag)
        {
            // Here we boldly assume that anything that is not random-access
            // preserves validity. This is valid for the STL sequences.
            s.insert(s.end(), r.begin(), r.end());
        }
        template <typename Sequence, typename Iterator>
        void append_and_preserve_iter(Sequence &s, const Sequence &r,
                                      Iterator &it,
                                      std::random_access_iterator_tag)
        {
            // Convert the iterator to an index, and later back.
            typename std::iterator_traits<Iterator>::difference_type idx =
                it - s.begin();
            s.insert(s.end(), r.begin(), r.end());
            it = s.begin() + idx;
        }

        template <typename Sequence>
        inline std::string dump_sequence(const Sequence &)
        {
            return "<undumpable sequence>";
        }
        inline std::string dump_sequence(const std::string &s)
        {
            return s;
        }
#ifndef BOOST_NO_STD_WSTRING
        inline std::string dump_sequence(const std::wstring &s)
        {
            return narrow<std::string>(s.c_str());
        }
#endif
    }

    /// Default path class. A path is a sequence of values. Groups of values
    /// are separated by the separator value, which defaults to '.' cast to
    /// the sequence's value type. The group of values is then passed to the
    /// translator to get a key.
    ///
    /// If instantiated with std::string and id_translator\<std::string\>,
    /// it accepts paths of the form "one.two.three.four".
    ///
    /// @tparam String Any Sequence. If the sequence does not support random-
    ///                access iteration, concatenation of paths assumes that
    ///                insertions at the end preserve iterator validity.
    /// @tparam Translator A translator with internal_type == String.
    template <typename String, typename Translator>
    class string_path
    {
        BOOST_STATIC_ASSERT((is_same<String,
                                   typename Translator::internal_type>::value));
    public:
        typedef typename Translator::external_type key_type;
        typedef typename String::value_type char_type;

        /// Create an empty path.
        explicit string_path(char_type separator = char_type('.'));
        /// Create a path by parsing the given string.
        /// @param value A sequence, possibly with separators, that describes
        ///              the path, e.g. "one.two.three".
        /// @param separator The separator used in parsing. Defaults to '.'.
        /// @param tr The translator used by this path to convert the individual
        ///           parts to keys.
        string_path(const String &value, char_type separator = char_type('.'),
                    Translator tr = Translator());
        /// Create a path by parsing the given string.
        /// @param value A zero-terminated array of values. Only use if zero-
        ///              termination makes sense for your type, and your
        ///              sequence supports construction from it. Intended for
        ///              string literals.
        /// @param separator The separator used in parsing. Defaults to '.'.
        /// @param tr The translator used by this path to convert the individual
        ///           parts to keys.
        string_path(const char_type *value,
                    char_type separator = char_type('.'),
                    Translator tr = Translator());

        // Default copying doesn't do the right thing with the iterator
        string_path(const string_path &o);
        string_path& operator =(const string_path &o);

        /// Take a single element off the path at the front and return it.
        key_type reduce();

        /// Test if the path is empty.
        bool empty() const;

        /// Test if the path contains a single element, i.e. no separators.
        bool single() const;

        /// Get the separator used by this path.
        char_type separator() const { return m_separator; }

        std::string dump() const {
            return detail::dump_sequence(m_value);
        }

        /// Append a second path to this one.
        /// @pre o's separator is the same as this one's, or o has no separators
        string_path& operator /=(const string_path &o) {
            // If it's single, there's no separator. This allows to do
            // p /= "piece";
            // even for non-default separators.
            BOOST_ASSERT((m_separator == o.m_separator
                          || o.empty()
                          || o.single())
                         && "Incompatible paths.");
            if(!o.empty()) {
                String sub;
                if(!this->empty()) {
                    sub.push_back(m_separator);
                }
                sub.insert(sub.end(), o.cstart(), o.m_value.end());
                detail::append_and_preserve_iter(m_value, sub, m_start,
                    typename std::iterator_traits<s_iter>::iterator_category());
            }
            return *this;
        }

    private:
        typedef typename String::iterator s_iter;
        typedef typename String::const_iterator s_c_iter;
        String m_value;
        char_type m_separator;
        Translator m_tr;
        s_iter m_start;
        s_c_iter cstart() const { return m_start; }
    };

    template <typename String, typename Translator> inline
    string_path<String, Translator>::string_path(char_type separator)
        : m_separator(separator), m_start(m_value.begin())
    {}

    template <typename String, typename Translator> inline
    string_path<String, Translator>::string_path(const String &value,
                                                 char_type separator,
                                                 Translator tr)
        : m_value(value), m_separator(separator),
          m_tr(tr), m_start(m_value.begin())
    {}

    template <typename String, typename Translator> inline
    string_path<String, Translator>::string_path(const char_type *value,
                                                 char_type separator,
                                                 Translator tr)
        : m_value(value), m_separator(separator),
          m_tr(tr), m_start(m_value.begin())
    {}

    template <typename String, typename Translator> inline
    string_path<String, Translator>::string_path(const string_path &o)
        : m_value(o.m_value), m_separator(o.m_separator),
          m_tr(o.m_tr), m_start(m_value.begin())
    {
        std::advance(m_start, std::distance(o.m_value.begin(), o.cstart()));
    }

    template <typename String, typename Translator> inline
    string_path<String, Translator>&
    string_path<String, Translator>::operator =(const string_path &o)
    {
        m_value = o.m_value;
        m_separator = o.m_separator;
        m_tr = o.m_tr;
        m_start = m_value.begin();
        std::advance(m_start, std::distance(o.m_value.begin(), o.cstart()));
        return *this;
    }

    template <typename String, typename Translator>
    typename Translator::external_type string_path<String, Translator>::reduce()
    {
        BOOST_ASSERT(!empty() && "Reducing empty path");

        s_iter next_sep = std::find(m_start, m_value.end(), m_separator);
        String part(m_start, next_sep);
        m_start = next_sep;
        if(!empty()) {
          // Unless we're at the end, skip the separator we found.
          ++m_start;
        }

        if(optional<key_type> key = m_tr.get_value(part)) {
            return *key;
        }
        BOOST_PROPERTY_TREE_THROW(ptree_bad_path("Path syntax error", *this));
    }

    template <typename String, typename Translator> inline
    bool string_path<String, Translator>::empty() const
    {
        return m_start == m_value.end();
    }

    template <typename String, typename Translator> inline
    bool string_path<String, Translator>::single() const
    {
        return std::find(static_cast<s_c_iter>(m_start),
                         m_value.end(), m_separator)
            == m_value.end();
    }

    // By default, this is the path for strings. You can override this by
    // specializing path_of for a more specific form of std::basic_string.
    template <typename Ch, typename Traits, typename Alloc>
    struct path_of< std::basic_string<Ch, Traits, Alloc> >
    {
        typedef std::basic_string<Ch, Traits, Alloc> _string;
        typedef string_path< _string, id_translator<_string> > type;
    };

    template <typename String, typename Translator> inline
    string_path<String, Translator> operator /(
                                  string_path<String, Translator> p1,
                                  const string_path<String, Translator> &p2)
    {
        p1 /= p2;
        return p1;
    }

    // These shouldn't be necessary, but GCC won't find the one above.
    template <typename String, typename Translator> inline
    string_path<String, Translator> operator /(
                                  string_path<String, Translator> p1,
                                  const typename String::value_type *p2)
    {
        p1 /= p2;
        return p1;
    }

    template <typename String, typename Translator> inline
    string_path<String, Translator> operator /(
                                  const typename String::value_type *p1,
                                  const string_path<String, Translator> &p2)
    {
        string_path<String, Translator> t(p1);
        t /= p2;
        return t;
    }

}}

#endif

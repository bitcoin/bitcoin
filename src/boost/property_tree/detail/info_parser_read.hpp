// ----------------------------------------------------------------------------
// Copyright (C) 2002-2006 Marcin Kalicinski
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------
#ifndef BOOST_PROPERTY_TREE_DETAIL_INFO_PARSER_READ_HPP_INCLUDED
#define BOOST_PROPERTY_TREE_DETAIL_INFO_PARSER_READ_HPP_INCLUDED

#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/detail/info_parser_error.hpp"
#include "boost/property_tree/detail/info_parser_utils.hpp"
#include <iterator>
#include <string>
#include <stack>
#include <fstream>
#include <cctype>

namespace boost { namespace property_tree { namespace info_parser
{

    // Expand known escape sequences
    template<class It>
    std::basic_string<typename std::iterator_traits<It>::value_type>
        expand_escapes(It b, It e)
    {
        typedef typename std::iterator_traits<It>::value_type Ch;
        std::basic_string<Ch> result;
        while (b != e)
        {
            if (*b == Ch('\\'))
            {
                ++b;
                if (b == e)
                {
                    BOOST_PROPERTY_TREE_THROW(info_parser_error(
                        "character expected after backslash", "", 0));
                }
                else if (*b == Ch('0')) result += Ch('\0');
                else if (*b == Ch('a')) result += Ch('\a');
                else if (*b == Ch('b')) result += Ch('\b');
                else if (*b == Ch('f')) result += Ch('\f');
                else if (*b == Ch('n')) result += Ch('\n');
                else if (*b == Ch('r')) result += Ch('\r');
                else if (*b == Ch('t')) result += Ch('\t');
                else if (*b == Ch('v')) result += Ch('\v');
                else if (*b == Ch('"')) result += Ch('"');
                else if (*b == Ch('\'')) result += Ch('\'');
                else if (*b == Ch('\\')) result += Ch('\\');
                else
                    BOOST_PROPERTY_TREE_THROW(info_parser_error(
                        "unknown escape sequence", "", 0));
            }
            else
                result += *b;
            ++b;
        }
        return result;
    }
    
    // Detect whitespace in a not very smart way.
    template <class Ch>
    bool is_ascii_space(Ch c)
    {
        // Everything outside ASCII is not space.
        unsigned n = c;
        if (n > 127)
            return false;
        return std::isspace(c) != 0;
    }

    // Advance pointer past whitespace
    template<class Ch>
    void skip_whitespace(const Ch *&text)
    {
        using namespace std;
        while (is_ascii_space(*text))
            ++text;
    }

    // Extract word (whitespace delimited) and advance pointer accordingly
    template<class Ch>
    std::basic_string<Ch> read_word(const Ch *&text)
    {
        using namespace std;
        skip_whitespace(text);
        const Ch *start = text;
        while (!is_ascii_space(*text) && *text != Ch(';') && *text != Ch('\0'))
            ++text;
        return expand_escapes(start, text);
    }

    // Extract line (eol delimited) and advance pointer accordingly
    template<class Ch>
    std::basic_string<Ch> read_line(const Ch *&text)
    {
        using namespace std;
        skip_whitespace(text);
        const Ch *start = text;
        while (*text != Ch('\0') && *text != Ch(';'))
            ++text;
        while (text > start && is_ascii_space(*(text - 1)))
            --text;
        return expand_escapes(start, text);
    }

    // Extract string (inside ""), and advance pointer accordingly
    // Set need_more_lines to true if \ continuator found
    template<class Ch>
    std::basic_string<Ch> read_string(const Ch *&text, bool *need_more_lines)
    {
        skip_whitespace(text);
        if (*text == Ch('\"'))
        {

            // Skip "
            ++text;

            // Find end of string, but skip escaped "
            bool escaped = false;
            const Ch *start = text;
            while ((escaped || *text != Ch('\"')) && *text != Ch('\0'))
            {
                escaped = (!escaped && *text == Ch('\\'));
                ++text;
            }

            // If end of string found
            if (*text == Ch('\"'))
            {
                std::basic_string<Ch> result = expand_escapes(start, text++);
                skip_whitespace(text);
                if (*text == Ch('\\'))
                {
                    if (!need_more_lines)
                        BOOST_PROPERTY_TREE_THROW(info_parser_error(
                            "unexpected \\", "", 0));
                    ++text;
                    skip_whitespace(text);
                    if (*text == Ch('\0') || *text == Ch(';'))
                        *need_more_lines = true;
                    else
                        BOOST_PROPERTY_TREE_THROW(info_parser_error(
                            "expected end of line after \\", "", 0));
                }
                else
                    if (need_more_lines)
                        *need_more_lines = false;
                return result;
            }
            else
                BOOST_PROPERTY_TREE_THROW(info_parser_error(
                    "unexpected end of line", "", 0));

        }
        else
            BOOST_PROPERTY_TREE_THROW(info_parser_error("expected \"", "", 0));
    }

    // Extract key
    template<class Ch>
    std::basic_string<Ch> read_key(const Ch *&text)
    {
        skip_whitespace(text);
        if (*text == Ch('\"'))
            return read_string(text, NULL);
        else
            return read_word(text);
    }

    // Extract data
    template<class Ch>
    std::basic_string<Ch> read_data(const Ch *&text, bool *need_more_lines)
    {
        skip_whitespace(text);
        if (*text == Ch('\"'))
            return read_string(text, need_more_lines);
        else
        {
            *need_more_lines = false;
            return read_word(text);
        }
    }

    // Build ptree from info stream
    template<class Ptree, class Ch>
    void read_info_internal(std::basic_istream<Ch> &stream,
                            Ptree &pt,
                            const std::string &filename,
                            int include_depth)
    {
        typedef std::basic_string<Ch> str_t;
        // Possible parser states
        enum state_t {
            s_key,              // Parser expects key
            s_data,             // Parser expects data
            s_data_cont         // Parser expects data continuation
        };

        unsigned long line_no = 0;
        state_t state = s_key;          // Parser state
        Ptree *last = NULL;             // Pointer to last created ptree
        // Define line here to minimize reallocations
        str_t line;

        // Initialize ptree stack (used to handle nesting)
        std::stack<Ptree *> stack;
        stack.push(&pt);                // Push root ptree on stack initially

        try {
            // While there are characters in the stream
            while (stream.good()) {
                // Read one line from stream
                ++line_no;
                std::getline(stream, line);
                if (!stream.good() && !stream.eof())
                    BOOST_PROPERTY_TREE_THROW(info_parser_error(
                        "read error", filename, line_no));
                const Ch *text = line.c_str();

                // If directive found
                skip_whitespace(text);
                if (*text == Ch('#')) {
                    // Determine directive type
                    ++text;     // skip #
                    std::basic_string<Ch> directive = read_word(text);
                    if (directive == convert_chtype<Ch, char>("include")) {
                        // #include
                        if (include_depth > 100) {
                            BOOST_PROPERTY_TREE_THROW(info_parser_error(
                                "include depth too large, "
                                "probably recursive include",
                                filename, line_no));
                        }
                        str_t s = read_string(text, NULL);
                        std::string inc_name =
                            convert_chtype<char, Ch>(s.c_str());
                        std::basic_ifstream<Ch> inc_stream(inc_name.c_str());
                        if (!inc_stream.good())
                            BOOST_PROPERTY_TREE_THROW(info_parser_error(
                                "cannot open include file " + inc_name,
                                filename, line_no));
                        read_info_internal(inc_stream, *stack.top(),
                                           inc_name, include_depth + 1);
                    } else {   // Unknown directive
                        BOOST_PROPERTY_TREE_THROW(info_parser_error(
                            "unknown directive", filename, line_no));
                    }

                    // Directive must be followed by end of line
                    skip_whitespace(text);
                    if (*text != Ch('\0')) {
                        BOOST_PROPERTY_TREE_THROW(info_parser_error(
                            "expected end of line", filename, line_no));
                    }

                    // Go to next line
                    continue;
                }

                // While there are characters left in line
                while (1) {

                    // Stop parsing on end of line or comment
                    skip_whitespace(text);
                    if (*text == Ch('\0') || *text == Ch(';')) {
                        if (state == s_data)    // If there was no data set state to s_key
                            state = s_key;
                        break;
                    }

                    // Process according to current parser state
                    switch (state)
                    {

                        // Parser expects key
                        case s_key:
                        {

                            if (*text == Ch('{'))   // Brace opening found
                            {
                                if (!last)
                                    BOOST_PROPERTY_TREE_THROW(info_parser_error("unexpected {", "", 0));
                                stack.push(last);
                                last = NULL;
                                ++text;
                            }
                            else if (*text == Ch('}'))  // Brace closing found
                            {
                                if (stack.size() <= 1)
                                    BOOST_PROPERTY_TREE_THROW(info_parser_error("unmatched }", "", 0));
                                stack.pop();
                                last = NULL;
                                ++text;
                            }
                            else    // Key text found
                            {
                                std::basic_string<Ch> key = read_key(text);
                                last = &stack.top()->push_back(
                                    std::make_pair(key, Ptree()))->second;
                                state = s_data;
                            }

                        }; break;

                        // Parser expects data
                        case s_data:
                        {
                            
                            // Last ptree must be defined because we are going to add data to it
                            BOOST_ASSERT(last);
                            
                            if (*text == Ch('{'))   // Brace opening found
                            {
                                stack.push(last);
                                last = NULL;
                                ++text;
                                state = s_key;
                            }
                            else if (*text == Ch('}'))  // Brace closing found
                            {
                                if (stack.size() <= 1)
                                    BOOST_PROPERTY_TREE_THROW(info_parser_error("unmatched }", "", 0));
                                stack.pop();
                                last = NULL;
                                ++text;
                                state = s_key;
                            }
                            else    // Data text found
                            {
                                bool need_more_lines;
                                std::basic_string<Ch> data = read_data(text, &need_more_lines);
                                last->data() = data;
                                state = need_more_lines ? s_data_cont : s_key;
                            }


                        }; break;

                        // Parser expects continuation of data after \ on previous line
                        case s_data_cont:
                        {
                            
                            // Last ptree must be defined because we are going to update its data
                            BOOST_ASSERT(last);
                            
                            if (*text == Ch('\"'))  // Continuation must start with "
                            {
                                bool need_more_lines;
                                std::basic_string<Ch> data = read_string(text, &need_more_lines);
                                last->put_value(last->template get_value<std::basic_string<Ch> >() + data);
                                state = need_more_lines ? s_data_cont : s_key;
                            }
                            else
                                BOOST_PROPERTY_TREE_THROW(info_parser_error("expected \" after \\ in previous line", "", 0));

                        }; break;

                        // Should never happen
                        default:
                            BOOST_ASSERT(0);

                    }
                }
            }

            // Check if stack has initial size, otherwise some {'s have not been closed
            if (stack.size() != 1)
                BOOST_PROPERTY_TREE_THROW(info_parser_error("unmatched {", "", 0));

        }
        catch (info_parser_error &e)
        {
            // If line undefined rethrow error with correct filename and line
            if (e.line() == 0)
            {
                BOOST_PROPERTY_TREE_THROW(info_parser_error(e.message(), filename, line_no));
            }
            else
                BOOST_PROPERTY_TREE_THROW(e);

        }

    }

} } }

#endif

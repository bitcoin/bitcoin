/*=============================================================================
    Copyright (c) 2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_ERROR_REPORTING_MAY_19_2014_00405PM)
#define BOOST_SPIRIT_X3_ERROR_REPORTING_MAY_19_2014_00405PM

#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/spirit/home/x3/support/utility/utf8.hpp>
#include <ostream>

// Clang-style error handling utilities

namespace boost { namespace spirit { namespace x3
{
    // tag used to get our error handler from the context
    struct error_handler_tag;

    template <typename Iterator>
    class error_handler
    {
    public:

        typedef Iterator iterator_type;

        error_handler(
            Iterator first, Iterator last, std::ostream& err_out
          , std::string file = "", int tabs = 4)
          : err_out(err_out)
          , file(file)
          , tabs(tabs)
          , pos_cache(first, last) {}

        typedef void result_type;

        void operator()(Iterator err_pos, std::string const& error_message) const;
        void operator()(Iterator err_first, Iterator err_last, std::string const& error_message) const;
        void operator()(position_tagged pos, std::string const& message) const
        {
            auto where = pos_cache.position_of(pos);
            (*this)(where.begin(), where.end(), message);
        }

        template <typename AST>
        void tag(AST& ast, Iterator first, Iterator last)
        {
            return pos_cache.annotate(ast, first, last);
        }

        boost::iterator_range<Iterator> position_of(position_tagged pos) const
        {
            return pos_cache.position_of(pos);
        }

        position_cache<std::vector<Iterator>> const& get_position_cache() const
        {
            return pos_cache;
        }

    private:

        void print_file_line(std::size_t line) const;
        void print_line(Iterator line_start, Iterator last) const;
        void print_indicator(Iterator& line_start, Iterator last, char ind) const;
        Iterator get_line_start(Iterator first, Iterator pos) const;
        std::size_t position(Iterator i) const;

        std::ostream& err_out;
        std::string file;
        int tabs;
        position_cache<std::vector<Iterator>> pos_cache;
    };

    template <typename Iterator>
    void error_handler<Iterator>::print_file_line(std::size_t line) const
    {
        if (file != "")
        {
            err_out << "In file " << file << ", ";
        }
        else
        {
            err_out << "In ";
        }

        err_out << "line " << line << ':' << std::endl;
    }

    template <typename Iterator>
    void error_handler<Iterator>::print_line(Iterator start, Iterator last) const
    {
        auto end = start;
        while (end != last)
        {
            auto c = *end;
            if (c == '\r' || c == '\n')
                break;
            else
                ++end;
        }
        typedef typename std::iterator_traits<Iterator>::value_type char_type;
        std::basic_string<char_type> line{start, end};
        err_out << x3::to_utf8(line) << std::endl;
    }

    template <typename Iterator>
    void error_handler<Iterator>::print_indicator(Iterator& start, Iterator last, char ind) const
    {
        for (; start != last; ++start)
        {
            auto c = *start;
            if (c == '\r' || c == '\n')
                break;
            else if (c == '\t')
                for (int i = 0; i < tabs; ++i)
                    err_out << ind;
            else
                err_out << ind;
        }
    }

    template <class Iterator>
    inline Iterator error_handler<Iterator>::get_line_start(Iterator first, Iterator pos) const
    {
        Iterator latest = first;
        for (Iterator i = first; i != pos;)
            if (*i == '\r' || *i == '\n')
                latest = ++i;
            else
                ++i;
        return latest;
    }

    template <typename Iterator>
    std::size_t error_handler<Iterator>::position(Iterator i) const
    {
        std::size_t line { 1 };
        typename std::iterator_traits<Iterator>::value_type prev { 0 };

        for (Iterator pos = pos_cache.first(); pos != i; ++pos) {
            auto c = *pos;
            switch (c) {
            case '\n':
                if (prev != '\r') ++line;
                break;
            case '\r':
                ++line;
                break;
            default:
                break;
            }
            prev = c;
        }

        return line;
    }

    template <typename Iterator>
    void error_handler<Iterator>::operator()(
        Iterator err_pos, std::string const& error_message) const
    {
        Iterator first = pos_cache.first();
        Iterator last = pos_cache.last();

        print_file_line(position(err_pos));
        err_out << error_message << std::endl;

        Iterator start = get_line_start(first, err_pos);
        print_line(start, last);
        print_indicator(start, err_pos, '_');
        err_out << "^_" << std::endl;
    }

    template <typename Iterator>
    void error_handler<Iterator>::operator()(
        Iterator err_first, Iterator err_last, std::string const& error_message) const
    {
        Iterator first = pos_cache.first();
        Iterator last = pos_cache.last();

        print_file_line(position(err_first));
        err_out << error_message << std::endl;

        Iterator start = get_line_start(first, err_first);
        print_line(start, last);
        print_indicator(start, err_first, ' ');
        print_indicator(start, err_last, '~');
        err_out << " <<-- Here" << std::endl;
    }

}}}

#endif

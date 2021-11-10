/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Definition of the position_iterator and file_position templates

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_FILE_POSITION_H_52BDEDF7_DAD3_4F24_802F_E66BB8098F68_INCLUDED)
#define BOOST_FILE_POSITION_H_52BDEDF7_DAD3_4F24_802F_E66BB8098F68_INCLUDED

#include <string>
#include <ostream>

#include <boost/assert.hpp>
#include <boost/spirit/include/classic_version.hpp>
#include <boost/spirit/include/classic_position_iterator.hpp>
#include <boost/wave/wave_config.hpp>
#if BOOST_WAVE_SERIALIZATION != 0
#include <boost/serialization/serialization.hpp>
#endif

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace util {

///////////////////////////////////////////////////////////////////////////////
//
//  file_position
//
//  A structure to hold positional information. This includes the filename,
//  line number and column number of a current token position.
//
///////////////////////////////////////////////////////////////////////////////

template <typename StringT>
struct file_position {

public:
    typedef StringT string_type;

    file_position()
    :   file(), line(1), column(1)
    {}
    explicit file_position(string_type const& file_, std::size_t line_ = 1,
            std::size_t column_ = 1)
    :   file(file_), line(line_), column(column_)
    {}

    // accessors
    string_type const &get_file() const { return file; }
    std::size_t get_line() const { return line; }
    std::size_t get_column() const { return column; }

    void set_file(string_type const &file_)
    {
        file = file_;
    }
    void set_line(std::size_t line_) { line = line_; }
    void set_column(std::size_t column_) { column = column_; }

private:
#if BOOST_WAVE_SERIALIZATION != 0
    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        using namespace boost::serialization;
        ar & make_nvp("filename", file);
        ar & make_nvp("line", line);
        ar & make_nvp("column", column);
    }
#endif

    string_type file;
    std::size_t line;
    std::size_t column;
};

template <typename StringT>
bool operator== (file_position<StringT> const &lhs,
    file_position<StringT> const &rhs)
{
    return lhs.get_column() == rhs.get_column() &&
        lhs.get_line() == rhs.get_line() && lhs.get_file() == rhs.get_file();
}

template <typename StringT>
inline std::ostream &
operator<< (std::ostream &o, file_position<StringT> const &pos)
{
    o << pos.get_file() << ":" << pos.get_line() << ":"  << pos.get_column();
    return o;
}

typedef file_position<BOOST_WAVE_STRINGTYPE> file_position_type;

///////////////////////////////////////////////////////////////////////////////
//
//  position_iterator
//
//  The position_iterator used by Wave is now based on the corresponding Spirit
//  type. This type is used with our own file_position though. The needed
//  specialization of the boost::spirit::classic::position_policy class is
//  provided below.
//
///////////////////////////////////////////////////////////////////////////////

template <typename IteratorT, typename PositionT>
struct position_iterator
:   boost::spirit::classic::position_iterator<IteratorT, PositionT>
{
    typedef boost::spirit::classic::position_iterator<IteratorT, PositionT> base_type;

    position_iterator()
    {
    }

    position_iterator(IteratorT const &begin, IteratorT const &end,
            PositionT const &pos)
    :   base_type(begin, end, pos)
    {
    }
};

///////////////////////////////////////////////////////////////////////////////
}   // namespace util
}   // namespace wave

///////////////////////////////////////////////////////////////////////////////

namespace spirit { namespace classic {

///////////////////////////////////////////////////////////////////////////////
//
//  The boost::spirit::classic::position_policy has to be specialized for our
//  file_position class
//
///////////////////////////////////////////////////////////////////////////////

    template <>
    class position_policy<boost::wave::util::file_position_type> {

    public:
        position_policy()
            : m_CharsPerTab(4)
        {}

        void next_line(boost::wave::util::file_position_type &pos)
        {
            pos.set_line(pos.get_line() + 1);
            pos.set_column(1);
        }

        void set_tab_chars(unsigned int chars)
        {
            m_CharsPerTab = chars;
        }

        void next_char(boost::wave::util::file_position_type &pos)
        {
            pos.set_column(pos.get_column() + 1);
        }

        void tabulation(boost::wave::util::file_position_type &pos)
        {
            pos.set_column(pos.get_column() + m_CharsPerTab -
                (pos.get_column() - 1) % m_CharsPerTab);
        }

    private:
        unsigned int m_CharsPerTab;
    };

///////////////////////////////////////////////////////////////////////////////
}}   // namespace spirit::classic

}   // namespace boost

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !defined(BOOST_FILE_POSITION_H_52BDEDF7_DAD3_4F24_802F_E66BB8098F68_INCLUDED)

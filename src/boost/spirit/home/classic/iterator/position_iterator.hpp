/*=============================================================================
    Copyright (c) 2002 Juan Carlos Arevalo-Baeza
    Copyright (c) 2002-2006 Hartmut Kaiser
    Copyright (c) 2003 Giovanni Bajo
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_POSITION_ITERATOR_HPP
#define BOOST_SPIRIT_POSITION_ITERATOR_HPP

#include <string>
#include <boost/config.hpp>

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/iterator/position_iterator_fwd.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
//
//  file_position_without_column
//
//  A structure to hold positional information. This includes the file,
//  and the line number
//
///////////////////////////////////////////////////////////////////////////////
template <typename String>
struct file_position_without_column_base {
    String file;
    int line;

    file_position_without_column_base(String const& file_ = String(),
                  int line_ = 1):
        file    (file_),
        line    (line_)
    {}

    bool operator==(const file_position_without_column_base& fp) const
    { return line == fp.line && file == fp.file; }
};

///////////////////////////////////////////////////////////////////////////////
//
//  file_position
//
//  This structure holds complete file position, including file name,
//  line and column number
//
///////////////////////////////////////////////////////////////////////////////
template <typename String>
struct file_position_base : public file_position_without_column_base<String> {
    int column;

    file_position_base(String const& file_ = String(),
                       int line_ = 1, int column_ = 1):
        file_position_without_column_base<String> (file_, line_),
        column                       (column_)
    {}

    bool operator==(const file_position_base& fp) const
    { return column == fp.column && this->line == fp.line && this->file == fp.file; }
};

///////////////////////////////////////////////////////////////////////////////
//
//  position_policy<>
//
//  This template is the policy to handle the file position. It is specialized
//  on the position type. Providing a custom file_position also requires
//  providing a specialization of this class.
//
//  Policy interface:
//
//    Default constructor of the custom position class must be accessible.
//    set_tab_chars(unsigned int chars) - Set the tabstop width
//    next_char(PositionT& pos)  - Notify that a new character has been
//      processed
//    tabulation(PositionT& pos) - Notify that a tab character has been
//      processed
//    next_line(PositionT& pos)  - Notify that a new line delimiter has
//      been reached.
//
///////////////////////////////////////////////////////////////////////////////
template <typename PositionT> class position_policy;

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} /* namespace BOOST_SPIRIT_CLASSIC_NS */


// This must be included here for full compatibility with old MSVC
#include <boost/spirit/home/classic/iterator/impl/position_iterator.ipp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
//
//  position_iterator
//
//  It wraps an iterator, and keeps track of the current position in the input,
//  as it gets incremented.
//
//  The wrapped iterator must be at least a Forward iterator. The position
//  iterator itself will always be a non-mutable Forward iterator.
//
//  In order to have begin/end iterators constructed, the end iterator must be
//  empty constructed. Similar to what happens with stream iterators. The begin
//  iterator must be constructed from both, the begin and end iterators of the
//  wrapped iterator type. This is necessary to implement the lookahead of
//  characters necessary to parse CRLF sequences.
//
//  In order to extract the current positional data from the iterator, you may
//  use the get_position member function.
//
//  You can also use the set_position member function to reset the current
//  position to something new.
//
//  The structure that holds the current position can be customized through a
//  template parameter, and the class position_policy must be specialized
//  on the new type to define how to handle it. Currently, it's possible
//  to choose between the file_position and file_position_without_column
//  (which saves some overhead if managing current column is not required).
//
///////////////////////////////////////////////////////////////////////////////

#if !defined(BOOST_ITERATOR_ADAPTORS_VERSION) || \
     BOOST_ITERATOR_ADAPTORS_VERSION < 0x0200
#error "Please use at least Boost V1.31.0 while compiling the position_iterator class!"
#else // BOOST_ITERATOR_ADAPTORS_VERSION < 0x0200

///////////////////////////////////////////////////////////////////////////////
//
//  Uses the newer iterator_adaptor version (should be released with
//  Boost V1.31.0)
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename ForwardIteratorT,
    typename PositionT,
    typename SelfT
>
class position_iterator
:   public iterator_::impl::position_iterator_base_generator<
        SelfT,
        ForwardIteratorT,
        PositionT
    >::type,
    public position_policy<PositionT>
{
private:

    typedef position_policy<PositionT> position_policy_t;
    typedef typename iterator_::impl::position_iterator_base_generator<
            SelfT,
            ForwardIteratorT,
            PositionT
        >::type base_t;
    typedef typename iterator_::impl::position_iterator_base_generator<
            SelfT,
            ForwardIteratorT,
            PositionT
        >::main_iter_t main_iter_t;

public:

    typedef PositionT position_t;

    position_iterator()
    :   _isend(true)
    {}

    position_iterator(
        const ForwardIteratorT& begin,
        const ForwardIteratorT& end)
    :   base_t(begin), _end(end), _pos(PositionT()), _isend(begin == end)
    {}

    template <typename FileNameT>
    position_iterator(
        const ForwardIteratorT& begin,
        const ForwardIteratorT& end,
        FileNameT fileName)
    :   base_t(begin), _end(end), _pos(PositionT(fileName)),
        _isend(begin == end)
    {}

    template <typename FileNameT, typename LineT>
    position_iterator(
        const ForwardIteratorT& begin,
        const ForwardIteratorT& end,
        FileNameT fileName, LineT line)
    :   base_t(begin), _end(end), _pos(PositionT(fileName, line)),
        _isend(begin == end)
    {}

    template <typename FileNameT, typename LineT, typename ColumnT>
    position_iterator(
        const ForwardIteratorT& begin,
        const ForwardIteratorT& end,
        FileNameT fileName, LineT line, ColumnT column)
    :   base_t(begin), _end(end), _pos(PositionT(fileName, line, column)),
        _isend(begin == end)
    {}

    position_iterator(
        const ForwardIteratorT& begin,
        const ForwardIteratorT& end,
        const PositionT& pos)
    :   base_t(begin), _end(end), _pos(pos), _isend(begin == end)
    {}

    position_iterator(const position_iterator& iter)
    :   base_t(iter.base()), position_policy_t(iter),
        _end(iter._end), _pos(iter._pos), _isend(iter._isend)
    {}

    position_iterator& operator=(const position_iterator& iter)
    {
        base_t::operator=(iter);
        position_policy_t::operator=(iter);
        _end = iter._end;
        _pos = iter._pos;
        _isend = iter._isend;
        return *this;
    }

    void set_position(PositionT const& newpos) { _pos = newpos; }
    PositionT& get_position() { return _pos; }
    PositionT const& get_position() const { return _pos; }

    void set_tabchars(unsigned int chars)
    {
        // This function (which comes from the position_policy) has a
        //  different name on purpose, to avoid messing with using
        //  declarations or qualified calls to access the base template
        //  function, which might break some compilers.
        this->position_policy_t::set_tab_chars(chars);
    }

private:
    friend class boost::iterator_core_access;

    void increment()
    {
        typename base_t::reference val = *(this->base());
        if (val == '\n') {
            ++this->base_reference();
            this->next_line(_pos);
            static_cast<main_iter_t &>(*this).newline();
        }
        else if ( val == '\r') {
            ++this->base_reference();
            if (this->base_reference() == _end || *(this->base()) != '\n')
            {
                this->next_line(_pos);
                static_cast<main_iter_t &>(*this).newline();
            }
        }
        else if (val == '\t') {
            this->tabulation(_pos);
            ++this->base_reference();
        }
        else {
            this->next_char(_pos);
            ++this->base_reference();
        }

        // The iterator is at the end only if it's the same
        //  of the
        _isend = (this->base_reference() == _end);
    }

    template <
        typename OtherDerivedT, typename OtherIteratorT,
        typename V, typename C, typename R, typename D
    >
    bool equal(iterator_adaptor<OtherDerivedT, OtherIteratorT, V, C, R, D>
        const &x) const
    {
        OtherDerivedT const &rhs = static_cast<OtherDerivedT const &>(x);
        bool x_is_end = rhs._isend;

        return (_isend == x_is_end) && (_isend || this->base() == rhs.base());
    }

protected:

    void newline()
    {}

    ForwardIteratorT _end;
    PositionT _pos;
    bool _isend;
};

#endif // BOOST_ITERATOR_ADAPTORS_VERSION < 0x0200

///////////////////////////////////////////////////////////////////////////////
//
//  position_iterator2
//
//  Equivalent to position_iterator, but it is able to extract the current
//  line into a string. This is very handy for error reports.
//
//  Notice that the footprint of this class is higher than position_iterator,
//  (how much depends on how bulky the underlying iterator is), so it should
//  be used only if necessary.
//
///////////////////////////////////////////////////////////////////////////////

template
<
    typename ForwardIteratorT,
    typename PositionT
>
class position_iterator2
    : public position_iterator
    <
        ForwardIteratorT,
        PositionT,
        position_iterator2<ForwardIteratorT, PositionT>
    >
{
    typedef position_iterator
    <
        ForwardIteratorT,
        PositionT,
        position_iterator2<ForwardIteratorT, PositionT> // JDG 4-15-03
    >  base_t;

public:
    typedef typename base_t::value_type value_type;
    typedef PositionT position_t;

    position_iterator2()
    {}

    position_iterator2(
        const ForwardIteratorT& begin,
        const ForwardIteratorT& end):
        base_t(begin, end),
        _startline(begin)
    {}

    template <typename FileNameT>
    position_iterator2(
        const ForwardIteratorT& begin,
        const ForwardIteratorT& end,
        FileNameT file):
        base_t(begin, end, file),
        _startline(begin)
    {}

    template <typename FileNameT, typename LineT>
    position_iterator2(
        const ForwardIteratorT& begin,
        const ForwardIteratorT& end,
        FileNameT file, LineT line):
        base_t(begin, end, file, line),
        _startline(begin)
    {}

    template <typename FileNameT, typename LineT, typename ColumnT>
    position_iterator2(
        const ForwardIteratorT& begin,
        const ForwardIteratorT& end,
        FileNameT file, LineT line, ColumnT column):
        base_t(begin, end, file, line, column),
        _startline(begin)
    {}

    position_iterator2(
        const ForwardIteratorT& begin,
        const ForwardIteratorT& end,
        const PositionT& pos):
        base_t(begin, end, pos),
        _startline(begin)
    {}

    position_iterator2(const position_iterator2& iter)
        : base_t(iter), _startline(iter._startline)
    {}

    position_iterator2& operator=(const position_iterator2& iter)
    {
        base_t::operator=(iter);
        _startline = iter._startline;
        return *this;
    }

    ForwardIteratorT get_currentline_begin() const
    { return _startline; }

    ForwardIteratorT get_currentline_end() const
    { return get_endline(); }

    std::basic_string<value_type> get_currentline() const
    {
        return std::basic_string<value_type>
            (get_currentline_begin(), get_currentline_end());
    }

protected:
    ForwardIteratorT _startline;

    friend class position_iterator<ForwardIteratorT, PositionT,
        position_iterator2<ForwardIteratorT, PositionT> >;

    ForwardIteratorT get_endline() const
    {
        ForwardIteratorT endline = _startline;
        while (endline != this->_end && *endline != '\r' && *endline != '\n')
        {
            ++endline;
        }
        return endline;
    }

    void newline()
    { _startline = this->base(); }
};

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif

/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    A generic C++ lexer token definition

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_CPP_TOKEN_HPP_53A13BD2_FBAA_444B_9B8B_FCB225C2BBA8_INCLUDED)
#define BOOST_CPP_TOKEN_HPP_53A13BD2_FBAA_444B_9B8B_FCB225C2BBA8_INCLUDED

#include <boost/wave/wave_config.hpp>
#if BOOST_WAVE_SERIALIZATION != 0
#include <boost/serialization/serialization.hpp>
#endif
#include <boost/wave/util/file_position.hpp>
#include <boost/wave/token_ids.hpp>
#include <boost/wave/language_support.hpp>

#include <boost/throw_exception.hpp>
#include <boost/pool/singleton_pool.hpp>
#include <boost/detail/atomic_count.hpp>
#include <boost/optional.hpp>

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace cpplexer {

namespace impl {

template <typename StringTypeT, typename PositionT>
class token_data
{
public:
    typedef StringTypeT string_type;
    typedef PositionT   position_type;

    //  default constructed tokens correspond to EOI tokens
    token_data()
    :   id(T_EOI), refcnt(1)
    {}

    //  construct an invalid token
    explicit token_data(int)
    :   id(T_UNKNOWN), refcnt(1)
    {}

    token_data(token_id id_, string_type const &value_,
               position_type const &pos_,
               optional<position_type> const & expand_pos_ = boost::none)
    :   id(id_), value(value_), pos(pos_), expand_pos(expand_pos_), refcnt(1)
    {}

    token_data(token_data const& rhs)
    :   id(rhs.id), value(rhs.value), pos(rhs.pos), expand_pos(rhs.expand_pos), refcnt(1)
    {}

    ~token_data()
    {}

    std::size_t addref() { return ++refcnt; }
    std::size_t release() { return --refcnt; }
    std::size_t get_refcnt() const { return refcnt; }

    // accessors
    operator token_id() const { return id; }
    string_type const &get_value() const { return value; }
    position_type const &get_position() const { return pos; }
    position_type const &get_expand_position() const
    {
        if (expand_pos)
            return *expand_pos;
        else
            return pos;
    }

    void set_token_id (token_id id_) { id = id_; }
    void set_value (string_type const &value_) { value = value_; }
    void set_position (position_type const &pos_) { pos = pos_; }
    void set_expand_position (position_type const & pos_) { expand_pos = pos_; }

    friend bool operator== (token_data const& lhs, token_data const& rhs)
    {
        //  two tokens are considered equal even if they refer to different
        //  positions
        return (lhs.id == rhs.id && lhs.value == rhs.value) ? true : false;
    }

    void init(token_id id_, string_type const &value_, position_type const &pos_)
    {
        BOOST_ASSERT(refcnt == 1);
        id = id_;
        value = value_;
        pos = pos_;
    }

    void init(token_data const& rhs)
    {
        BOOST_ASSERT(refcnt == 1);
        id = rhs.id;
        value = rhs.value;
        pos = rhs.pos;
    }

    static void *operator new(std::size_t size);
    static void operator delete(void *p, std::size_t size);

#if defined(BOOST_SPIRIT_DEBUG)
    // debug support
    void print (std::ostream &stream) const
    {
        stream << get_token_name(id) << "(";
        for (std::size_t i = 0; i < value.size(); ++i) {
            switch (value[i]) {
            case '\r':  stream << "\\r"; break;
            case '\n':  stream << "\\n"; break;
            default:
                stream << value[i];
                break;
            }
        }
        stream << ")";
    }
#endif // defined(BOOST_SPIRIT_DEBUG)

#if BOOST_WAVE_SERIALIZATION != 0
    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        using namespace boost::serialization;
        ar & make_nvp("id", id);
        ar & make_nvp("value", value);
        ar & make_nvp("position", pos);
    }
#endif

private:
    token_id id;                // the token id
    string_type value;          // the text, which was parsed into this token
    position_type pos;          // the original file position
    boost::optional<position_type> expand_pos;    // where this token was expanded
    boost::detail::atomic_count refcnt;
};

///////////////////////////////////////////////////////////////////////////////
struct token_data_tag {};

template <typename StringTypeT, typename PositionT>
inline void *
token_data<StringTypeT, PositionT>::operator new(std::size_t size)
{
    BOOST_ASSERT(sizeof(token_data<StringTypeT, PositionT>) == size);
    typedef boost::singleton_pool<
            token_data_tag, sizeof(token_data<StringTypeT, PositionT>)
        > pool_type;

    void *ret = pool_type::malloc();
    if (0 == ret)
        boost::throw_exception(std::bad_alloc());
    return ret;
}

template <typename StringTypeT, typename PositionT>
inline void
token_data<StringTypeT, PositionT>::operator delete(void *p, std::size_t size)
{
    BOOST_ASSERT(sizeof(token_data<StringTypeT, PositionT>) == size);
    typedef boost::singleton_pool<
            token_data_tag, sizeof(token_data<StringTypeT, PositionT>)
        > pool_type;

    if (0 != p)
        pool_type::free(p);
}

} // namespace impl

///////////////////////////////////////////////////////////////////////////////
//  forward declaration of the token type
template <typename PositionT = boost::wave::util::file_position_type>
class lex_token;

///////////////////////////////////////////////////////////////////////////////
//
//  lex_token
//
///////////////////////////////////////////////////////////////////////////////

template <typename PositionT>
class lex_token
{
public:
    typedef BOOST_WAVE_STRINGTYPE   string_type;
    typedef PositionT               position_type;

private:
    typedef impl::token_data<string_type, position_type> data_type;

public:
    //  default constructed tokens correspond to EOI tokens
    lex_token()
    :   data(0)
    {}

    //  construct an invalid token
    explicit lex_token(int)
    :   data(new data_type(0))
    {}

    lex_token(lex_token const& rhs)
    :   data(rhs.data)
    {
        if (0 != data)
            data->addref();
    }

    lex_token(token_id id_, string_type const &value_, PositionT const &pos_)
    :   data(new data_type(id_, value_, pos_))
    {}

    ~lex_token()
    {
        if (0 != data && 0 == data->release())
            delete data;
        data = 0;
    }

    lex_token& operator=(lex_token const& rhs)
    {
        if (&rhs != this) {
            if (0 != data && 0 == data->release())
                delete data;

            data = rhs.data;
            if (0 != data)
                data->addref();
        }
        return *this;
    }

    // accessors
    operator token_id() const { return 0 != data ? token_id(*data) : T_EOI; }
    string_type const &get_value() const { return data->get_value(); }
    position_type const &get_position() const { return data->get_position(); }
    position_type const &get_expand_position() const { return data->get_expand_position(); }
    bool is_eoi() const { return 0 == data || token_id(*data) == T_EOI; }
    bool is_valid() const { return 0 != data && token_id(*data) != T_UNKNOWN; }

    void set_token_id (token_id id_) { make_unique(); data->set_token_id(id_); }
    void set_value (string_type const &value_) { make_unique(); data->set_value(value_); }
    void set_position (position_type const &pos_) { make_unique(); data->set_position(pos_); }
    void set_expand_position (position_type const &pos_) { make_unique(); data->set_expand_position(pos_); }

    friend bool operator== (lex_token const& lhs, lex_token const& rhs)
    {
        if (0 == rhs.data)
            return 0 == lhs.data;
        if (0 == lhs.data)
            return false;
        return *(lhs.data) == *(rhs.data);
    }

// debug support
#if BOOST_WAVE_DUMP_PARSE_TREE != 0
    // access functions for the tree_to_xml functionality
    static int get_token_id(lex_token const &t)
        { return token_id(t); }
    static string_type get_token_value(lex_token const &t)
        { return t.get_value(); }
#endif

#if defined(BOOST_SPIRIT_DEBUG)
    // debug support
    void print (std::ostream &stream) const
    {
        data->print(stream);
    }
#endif // defined(BOOST_SPIRIT_DEBUG)

private:
#if BOOST_WAVE_SERIALIZATION != 0
    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        data->serialize(ar, version);
    }
#endif

    // make a unique copy of the current object
    void make_unique()
    {
        if (1 == data->get_refcnt())
            return;

        data_type* newdata = new data_type(*data) ;

        data->release();          // release this reference, can't get zero
        data = newdata;
    }

    data_type* data;
};

///////////////////////////////////////////////////////////////////////////////
//  This overload is needed by the multi_pass/functor_input_policy to
//  validate a token instance. It has to be defined in the same namespace
//  as the token class itself to allow ADL to find it.
///////////////////////////////////////////////////////////////////////////////
template <typename Position>
inline bool
token_is_valid(lex_token<Position> const& t)
{
    return t.is_valid();
}

///////////////////////////////////////////////////////////////////////////////
#if defined(BOOST_SPIRIT_DEBUG)
template <typename PositionT>
inline std::ostream &
operator<< (std::ostream &stream, lex_token<PositionT> const &object)
{
    object.print(stream);
    return stream;
}
#endif // defined(BOOST_SPIRIT_DEBUG)

///////////////////////////////////////////////////////////////////////////////
}   // namespace cpplexer
}   // namespace wave
}   // namespace boost

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !defined(BOOST_CPP_TOKEN_HPP_53A13BD2_FBAA_444B_9B8B_FCB225C2BBA8_INCLUDED)

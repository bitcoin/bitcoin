// Boost.Range library
//
//  Copyright Neil Groves 2014.
//  Use, modification and distribution is subject to the Boost Software
//  License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ADAPTOR_FORMATTED_HPP_INCLUDED
#define BOOST_RANGE_ADAPTOR_FORMATTED_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/range/concepts.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/iterator.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_array.hpp>
#include <boost/type_traits/remove_extent.hpp>
#include <ostream>

namespace boost
{
    namespace range_detail
    {

template<typename Sep, typename Prefix, typename Postfix>
struct formatted_holder
{
    typedef typename boost::mpl::if_<
        boost::is_array<Sep>,
        const typename boost::remove_extent<Sep>::type*,
        Sep
    >::type separator_t;

    typedef typename boost::mpl::if_<
        boost::is_array<Prefix>,
        const typename boost::remove_extent<Prefix>::type*,
        Prefix
    >::type prefix_t;

    typedef typename boost::mpl::if_<
        boost::is_array<Postfix>,
        const typename boost::remove_extent<Postfix>::type*,
        Postfix
    >::type postfix_t;

    formatted_holder(
        const separator_t& sep,
        const prefix_t& prefix,
        const postfix_t& postfix)
        : m_sep(sep)
        , m_prefix(prefix)
        , m_postfix(postfix)
    {
    }

    separator_t m_sep;
    prefix_t m_prefix;
    postfix_t m_postfix;
};

template<typename Iter, typename Sep, typename Prefix, typename Postfix>
class formatted_range
        : public boost::iterator_range<Iter>
{
    typedef formatted_holder<Sep,Prefix,Postfix> holder_t;
public:
    formatted_range(Iter first, Iter last, const holder_t& holder)
        : boost::iterator_range<Iter>(first, last)
        , m_holder(holder)
    {
    }

    template<typename OStream>
    void write(OStream& out) const
    {
        Iter it(this->begin());
        out << m_holder.m_prefix;
        if (it != this->end())
        {
            out << *it;
            for (++it; it != this->end(); ++it)
            {
                out << m_holder.m_sep << *it;
            }
        }
        out << m_holder.m_postfix;
    }

private:
    holder_t m_holder;
};

template<
    typename SinglePassRange,
    typename Sep,
    typename Prefix,
    typename Postfix
>
inline range_detail::formatted_range<
    typename range_iterator<const SinglePassRange>::type, Sep, Prefix, Postfix
>
operator|(
    const SinglePassRange& rng,
    const range_detail::formatted_holder<Sep,Prefix,Postfix>& holder
)
{
    typedef typename range_iterator<const SinglePassRange>::type iterator;
    return range_detail::formatted_range<iterator, Sep, Prefix, Postfix>(
        boost::begin(rng), boost::end(rng), holder);
}

template<typename Char, typename Traits, typename Iter, typename Sep,
    typename Prefix, typename Postfix>
std::basic_ostream<Char, Traits>&
operator<<(
        std::basic_ostream<Char, Traits>& out,
        const formatted_range<Iter, Sep, Prefix, Postfix>& writer)
{
    writer.write(out);
    return out;
}

    } // namespace range_detail

    namespace adaptors
    {

template<typename Sep, typename Prefix, typename Postfix>
range_detail::formatted_holder<Sep, Prefix, Postfix>
formatted(const Sep& sep, const Prefix& prefix, const Postfix& postfix)
{
    return range_detail::formatted_holder<Sep,Prefix,Postfix>(
                sep, prefix, postfix);
}

template<typename Sep, typename Prefix>
range_detail::formatted_holder<Sep, Prefix, char>
formatted(const Sep& sep, const Prefix& prefix)
{
    return range_detail::formatted_holder<Sep, Prefix, char>(sep, prefix, '}');
}

template<typename Sep>
range_detail::formatted_holder<Sep, char, char>
formatted(const Sep& sep)
{
    return range_detail::formatted_holder<Sep, char, char>(sep, '{', '}');
}

inline range_detail::formatted_holder<char, char, char>
formatted()
{
    return range_detail::formatted_holder<char, char, char>(',', '{', '}');
}

using range_detail::formatted_range;

template<typename SinglePassRange, typename Sep, typename Prefix,
         typename Postfix>
inline boost::range_detail::formatted_range<
    typename boost::range_iterator<const SinglePassRange>::type,
    Sep, Prefix, Postfix
>
format(
    const SinglePassRange& rng,
    const Sep& sep,
    const Prefix& prefix,
    const Postfix& postfix
)
{
    typedef typename boost::range_iterator<const SinglePassRange>::type
                iterator_t;

    typedef boost::range_detail::formatted_range<
                iterator_t, Sep, Prefix, Postfix>       result_t;

    typedef boost::range_detail::formatted_holder<Sep, Prefix, Postfix>
                holder_t;

    return result_t(boost::begin(rng), boost::end(rng),
                    holder_t(sep, prefix, postfix));
}

template<typename SinglePassRange, typename Sep, typename Prefix>
inline boost::range_detail::formatted_range<
    typename boost::range_iterator<const SinglePassRange>::type,
    Sep, Prefix, char
>
format(
    const SinglePassRange& rng,
    const Sep& sep,
    const Prefix& prefix)
{
    return adaptors::format<SinglePassRange, Sep, Prefix, char>(rng, sep, prefix, '}');
}

template<typename SinglePassRange, typename Sep>
inline boost::range_detail::formatted_range<
    typename boost::range_iterator<const SinglePassRange>::type,
    Sep, char, char
>
format(const SinglePassRange& rng, const Sep& sep)
{
    return adaptors::format<SinglePassRange, Sep, char, char>(rng, sep, '{', '}');
}

template<typename SinglePassRange>
inline boost::range_detail::formatted_range<
    typename boost::range_iterator<const SinglePassRange>::type,
    char, char, char
>
format(const SinglePassRange& rng)
{
    return adaptors::format<SinglePassRange, char, char, char>(rng, ',', '{', '}');
}

    } // namespace adaptors

    namespace range
    {
        using boost::range_detail::formatted_range;
    } // namespace range
} // namespace boost

#endif // include guard

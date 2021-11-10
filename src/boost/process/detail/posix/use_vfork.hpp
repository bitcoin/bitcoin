/*
 * use_vfork.hpp
 *
 *  Created on: 17.06.2016
 *      Author: klemens
 */

#ifndef BOOST_PROCESS_DETAIL_POSIX_USE_VFORK_HPP_
#define BOOST_PROCESS_DETAIL_POSIX_USE_VFORK_HPP_


#include <boost/process/detail/posix/handler.hpp>
#include <boost/fusion/sequence/intrinsic/has_key.hpp>
#include <boost/fusion/container/set/convert.hpp>

namespace boost { namespace process { namespace detail { namespace posix {

struct use_vfork_ : handler_base_ext
{
    constexpr use_vfork_(){};
};

template<typename Sequence>
struct shall_use_vfork
{
    typedef typename boost::fusion::result_of::as_set<Sequence>::type set_type;
    typedef typename boost::fusion::result_of::has_key<set_type, const use_vfork_&>::type type;
};


}}}}

#endif /* BOOST_PROCESS_DETAIL_POSIX_USE_VFORK_HPP_ */

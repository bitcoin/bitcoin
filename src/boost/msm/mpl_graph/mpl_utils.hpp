// Copyright 2008-2010 Gordon Woodhull
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_MPL_GRAPH_MPL_UTILS_HPP_INCLUDED
#define BOOST_MSM_MPL_GRAPH_MPL_UTILS_HPP_INCLUDED

#include <boost/mpl/fold.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/set.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/has_key.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/and.hpp>

namespace boost {
namespace msm {
namespace mpl_graph {
namespace mpl_utils {
    
// This is a grab bag of little metafunctions I expect already
// exist under some name I haven't looked for

// I figure there are probably better ways to do all of these things,
// but for now I'll just write some utilities to isolate my ignorance

template<typename Seq>
struct as_map :
    mpl::fold<Seq, 
              mpl::map<>, 
              mpl::insert<mpl::_1, mpl::_2> >
{};
template<typename Seq>
struct as_set :
    mpl::fold<Seq, 
              mpl::set<>, 
              mpl::insert<mpl::_1, mpl::_2> >
{};

template<typename AssocSeq, typename Key, typename Default>
struct at_or_default :
    mpl::if_<typename mpl::has_key<AssocSeq, Key>::type,
             typename mpl::at<AssocSeq, Key>::type,
             Default>
{};

template<typename Seq1, typename Seq2>
struct set_equal :
    mpl::fold<Seq2,
              mpl::true_,
              mpl::and_<mpl::_1, 
                        mpl::has_key<typename as_set<Seq1>::type, 
                                     mpl::_2 > > >
{};
                                       
}
}
}
}              

#endif // BOOST_MSM_MPL_GRAPH_MPL_UTILS_HPP_INCLUDED

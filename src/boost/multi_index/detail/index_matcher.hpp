/* Copyright 2003-2015 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#ifndef BOOST_MULTI_INDEX_DETAIL_INDEX_MATCHER_HPP
#define BOOST_MULTI_INDEX_DETAIL_INDEX_MATCHER_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <algorithm>
#include <boost/noncopyable.hpp>
#include <boost/multi_index/detail/auto_space.hpp>
#include <boost/multi_index/detail/raw_ptr.hpp>
#include <cstddef>
#include <functional>

namespace boost{

namespace multi_index{

namespace detail{

/* index_matcher compares a sequence of elements against a
 * base sequence, identifying those elements that belong to the
 * longest subsequence which is ordered with respect to the base.
 * For instance, if the base sequence is:
 *
 *   0 1 2 3 4 5 6 7 8 9
 *
 * and the compared sequence (not necesarilly the same length):
 *
 *   1 4 2 3 0 7 8 9
 *
 * the elements of the longest ordered subsequence are:
 *
 *   1 2 3 7 8 9
 * 
 * The algorithm for obtaining such a subsequence is called
 * Patience Sorting, described in ch. 1 of:
 *   Aldous, D., Diaconis, P.: "Longest increasing subsequences: from
 *   patience sorting to the Baik-Deift-Johansson Theorem", Bulletin
 *   of the American Mathematical Society, vol. 36, no 4, pp. 413-432,
 *   July 1999.
 *   http://www.ams.org/bull/1999-36-04/S0273-0979-99-00796-X/
 *   S0273-0979-99-00796-X.pdf
 *
 * This implementation is not fully generic since it assumes that
 * the sequences given are pointed to by index iterators (having a
 * get_node() memfun.)
 */

namespace index_matcher{

/* The algorithm stores the nodes of the base sequence and a number
 * of "piles" that are dynamically updated during the calculation
 * stage. From a logical point of view, nodes form an independent
 * sequence from piles. They are stored together so as to minimize
 * allocated memory.
 */

struct entry
{
  entry(void* node_,std::size_t pos_=0):node(node_),pos(pos_){}

  /* node stuff */

  void*       node;
  std::size_t pos;
  entry*      previous;
  bool        ordered;

  struct less_by_node
  {
    bool operator()(
      const entry& x,const entry& y)const
    {
      return std::less<void*>()(x.node,y.node);
    }
  };

  /* pile stuff */

  std::size_t pile_top;
  entry*      pile_top_entry;

  struct less_by_pile_top
  {
    bool operator()(
      const entry& x,const entry& y)const
    {
      return x.pile_top<y.pile_top;
    }
  };
};

/* common code operating on void *'s */

template<typename Allocator>
class algorithm_base:private noncopyable
{
protected:
  algorithm_base(const Allocator& al,std::size_t size):
    spc(al,size),size_(size),n_(0),sorted(false)
  {
  }

  void add(void* node)
  {
    entries()[n_]=entry(node,n_);
    ++n_;
  }

  void begin_algorithm()const
  {
    if(!sorted){
      std::sort(entries(),entries()+size_,entry::less_by_node());
      sorted=true;
    }
    num_piles=0;
  }

  void add_node_to_algorithm(void* node)const
  {
    entry* ent=
      std::lower_bound(
        entries(),entries()+size_,
        entry(node),entry::less_by_node()); /* localize entry */
    ent->ordered=false;
    std::size_t n=ent->pos;                 /* get its position */

    entry dummy(0);
    dummy.pile_top=n;

    entry* pile_ent=                        /* find the first available pile */
      std::lower_bound(                     /* to stack the entry            */
        entries(),entries()+num_piles,
        dummy,entry::less_by_pile_top());

    pile_ent->pile_top=n;                   /* stack the entry */
    pile_ent->pile_top_entry=ent;        

    /* if not the first pile, link entry to top of the preceding pile */
    if(pile_ent>&entries()[0]){ 
      ent->previous=(pile_ent-1)->pile_top_entry;
    }

    if(pile_ent==&entries()[num_piles]){    /* new pile? */
      ++num_piles;
    }
  }

  void finish_algorithm()const
  {
    if(num_piles>0){
      /* Mark those elements which are in their correct position, i.e. those
       * belonging to the longest increasing subsequence. These are those
       * elements linked from the top of the last pile.
       */

      entry* ent=entries()[num_piles-1].pile_top_entry;
      for(std::size_t n=num_piles;n--;){
        ent->ordered=true;
        ent=ent->previous;
      }
    }
  }

  bool is_ordered(void * node)const
  {
    return std::lower_bound(
      entries(),entries()+size_,
      entry(node),entry::less_by_node())->ordered;
  }

private:
  entry* entries()const{return raw_ptr<entry*>(spc.data());}

  auto_space<entry,Allocator> spc;
  std::size_t                 size_;
  std::size_t                 n_;
  mutable bool                sorted;
  mutable std::size_t         num_piles;
};

/* The algorithm has three phases:
 *   - Initialization, during which the nodes of the base sequence are added.
 *   - Execution.
 *   - Results querying, through the is_ordered memfun.
 */

template<typename Node,typename Allocator>
class algorithm:private algorithm_base<Allocator>
{
  typedef algorithm_base<Allocator> super;

public:
  algorithm(const Allocator& al,std::size_t size):super(al,size){}

  void add(Node* node)
  {
    super::add(node);
  }

  template<typename IndexIterator>
  void execute(IndexIterator first,IndexIterator last)const
  {
    super::begin_algorithm();

    for(IndexIterator it=first;it!=last;++it){
      add_node_to_algorithm(get_node(it));
    }

    super::finish_algorithm();
  }

  bool is_ordered(Node* node)const
  {
    return super::is_ordered(node);
  }

private:
  void add_node_to_algorithm(Node* node)const
  {
    super::add_node_to_algorithm(node);
  }

  template<typename IndexIterator>
  static Node* get_node(IndexIterator it)
  {
    return static_cast<Node*>(it.get_node());
  }
};

} /* namespace multi_index::detail::index_matcher */

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace boost */

#endif

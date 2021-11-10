/* Copyright 2016-2020 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/poly_collection for library home page.
 */

#ifndef BOOST_POLY_COLLECTION_DETAIL_SPLIT_SEGMENT_HPP
#define BOOST_POLY_COLLECTION_DETAIL_SPLIT_SEGMENT_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/poly_collection/detail/segment_backend.hpp>
#include <boost/poly_collection/detail/value_holder.hpp>
#include <iterator>
#include <memory>
#include <new>
#include <utility>
#include <vector>

namespace boost{

namespace poly_collection{

namespace detail{

/* segment_backend implementation that maintains two internal vectors, one for
 * value_type's (the index) and another for the concrete elements those refer
 * to (the store).
 *
 * Requires:
 *   - [const_]base_iterator is constructible from value_type*.
 *   - value_type is copy constructible.
 *   - Model::make_value_type(x) returns a value_type created from a reference
 *     to the concrete type.
 *
 * Conversion from base_iterator to local_iterator<Concrete> requires accesing
 * value_type internal info, so the end() base_iterator has to be made to point
 * to a valid element of index, which implies size(index)=size(store)+1. This
 * slightly complicates the memory management.
 */

template<typename Model,typename Concrete,typename Allocator>
class split_segment:public segment_backend<Model,Allocator>
{
  using value_type=typename Model::value_type;
  using store_value_type=value_holder<Concrete>;
  using store=std::vector<
    store_value_type,
    typename std::allocator_traits<Allocator>::
      template rebind_alloc<store_value_type>
  >;
  using store_iterator=typename store::iterator;
  using const_store_iterator=typename store::const_iterator;
  using index=std::vector<
    value_type,
    typename std::allocator_traits<Allocator>::
      template rebind_alloc<value_type>
  >;
  using const_index_iterator=typename index::const_iterator;
  using segment_backend=detail::segment_backend<Model,Allocator>;
  using typename segment_backend::segment_backend_unique_ptr;
  using typename segment_backend::value_pointer;
  using typename segment_backend::const_value_pointer;
  using typename segment_backend::base_iterator;
  using typename segment_backend::const_base_iterator;
  using const_iterator=
    typename segment_backend::template const_iterator<Concrete>;
  using typename segment_backend::base_sentinel;
  using typename segment_backend::range;
  using segment_allocator_type=typename std::allocator_traits<Allocator>::
    template rebind_alloc<split_segment>;

public:
  virtual ~split_segment()=default;

  static segment_backend_unique_ptr make(const segment_allocator_type& al)
  {
    return new_(al,al);
  }

  virtual segment_backend_unique_ptr copy()const
  {
    return new_(s.get_allocator(),store{s});
  }

  virtual segment_backend_unique_ptr copy(const Allocator& al)const
  {
    return new_(al,store{s,al});
  }

  virtual segment_backend_unique_ptr empty_copy(const Allocator& al)const
  {
    return new_(al,al);
  }

  virtual segment_backend_unique_ptr move(const Allocator& al)
  {
    return new_(al,store{std::move(s),al});
  }

  virtual bool equal(const segment_backend& x)const
  {
    return s==static_cast<const split_segment&>(x).s;
  }

  virtual Allocator     get_allocator()const noexcept
                         {return s.get_allocator();}
  virtual base_iterator begin()const noexcept{return nv_begin();}
  base_iterator         nv_begin()const noexcept
                         {return base_iterator{value_ptr(i.data())};}
  virtual base_iterator end()const noexcept{return nv_end();}
  base_iterator         nv_end()const noexcept
                         {return base_iterator{value_ptr(i.data()+s.size())};}
  virtual bool          empty()const noexcept{return nv_empty();}
  bool                  nv_empty()const noexcept{return s.empty();}
  virtual std::size_t   size()const noexcept{return nv_size();}
  std::size_t           nv_size()const noexcept{return s.size();}
  virtual std::size_t   max_size()const noexcept{return nv_max_size();}
  std::size_t           nv_max_size()const noexcept{return s.max_size()-1;}
  virtual std::size_t   capacity()const noexcept{return nv_capacity();}
  std::size_t           nv_capacity()const noexcept{return s.capacity();}

  virtual base_sentinel reserve(std::size_t n){return nv_reserve(n);}

  base_sentinel nv_reserve(std::size_t n)
  {
    bool rebuild=n>s.capacity();
    i.reserve(n+1);
    s.reserve(n);
    if(rebuild)rebuild_index();
    return sentinel();
  };

  virtual base_sentinel shrink_to_fit(){return nv_shrink_to_fit();}

  base_sentinel nv_shrink_to_fit()
  {
    try{
      auto p=s.data();
      if(!s.empty())s.shrink_to_fit();
      else{
        store ss{s.get_allocator()};
        ss.reserve(1); /* --> s.data()!=nullptr */
        s.swap(ss);
      }
      if(p!=s.data()){
        index ii{{},i.get_allocator()};
        ii.reserve(s.capacity()+1);
        i.swap(ii);
        build_index();
      }
    }
    catch(...){
      rebuild_index();
      throw;
    }
    return sentinel();
  }

  template<typename Iterator,typename... Args>
  range nv_emplace(Iterator p,Args&&... args)
  {
    auto q=prereserve(p);
    auto it=s.emplace(
      iterator_from(q),
      value_holder_emplacing_ctor,std::forward<Args>(args)...);
    push_index_entry();
    return range_from(it);
  }

  template<typename... Args>
  range nv_emplace_back(Args&&... args)
  {
    prereserve();
    s.emplace_back(value_holder_emplacing_ctor,std::forward<Args>(args)...);
    push_index_entry();
    return range_from(s.size()-1);
  }

  virtual range push_back(const_value_pointer x)
  {return nv_push_back(const_concrete_ref(x));}

  range nv_push_back(const Concrete& x)
  {
    prereserve();
    s.emplace_back(x);
    push_index_entry();
    return range_from(s.size()-1);
  }

  virtual range push_back_move(value_pointer x)
  {return nv_push_back(std::move(concrete_ref(x)));}

  range nv_push_back(Concrete&& x)
  {
    prereserve();
    s.emplace_back(std::move(x));
    push_index_entry();
    return range_from(s.size()-1);
  }

  virtual range insert(const_base_iterator p,const_value_pointer x)
  {return nv_insert(const_iterator(p),const_concrete_ref(x));}

  range nv_insert(const_iterator p,const Concrete& x)
  {
    p=prereserve(p);
    auto it=s.emplace(iterator_from(p),x);
    push_index_entry();
    return range_from(it);
  }

  virtual range insert_move(const_base_iterator p,value_pointer x)
  {return nv_insert(const_iterator(p),std::move(concrete_ref(x)));}

  range nv_insert(const_iterator p,Concrete&& x)
  {
    p=prereserve(p);
    auto it=s.emplace(iterator_from(p),std::move(x));
    push_index_entry();
    return range_from(it);
  }

  template<typename InputIterator>
  range nv_insert(InputIterator first,InputIterator last)
  {
    return nv_insert(
      const_iterator(concrete_ptr(s.data()+s.size())),first,last);
  }

  template<typename InputIterator>
  range nv_insert(const_iterator p,InputIterator first,InputIterator last)
  {
    return insert(
      p,first,last,
      typename std::iterator_traits<InputIterator>::iterator_category{});
  }

  virtual range erase(const_base_iterator p)
  {return nv_erase(const_iterator(p));}

  range nv_erase(const_iterator p)
  {
    pop_index_entry();
    return range_from(s.erase(iterator_from(p)));
  }
    
  virtual range erase(const_base_iterator first,const_base_iterator last)
  {return nv_erase(const_iterator(first),const_iterator(last));}

  range nv_erase(const_iterator first,const_iterator last)
  {
    std::size_t n=s.size();
    auto it=s.erase(iterator_from(first),iterator_from(last));
    pop_index_entry(n-s.size());
    return range_from(it);
  }

  virtual range erase_till_end(const_base_iterator first)
  {
    std::size_t n=s.size();
    auto it=s.erase(iterator_from(first),s.end());
    pop_index_entry(n-s.size());
    return range_from(it);
  }

  virtual range erase_from_begin(const_base_iterator last)
  {
    std::size_t n=s.size();
    auto it=s.erase(s.begin(),iterator_from(last));
    pop_index_entry(n-s.size());
    return range_from(it);
  }

  base_sentinel clear()noexcept{return nv_clear();}

  base_sentinel nv_clear()noexcept
  {
    s.clear();
    for(std::size_t n=i.size()-1;n--;)i.pop_back();
    return sentinel();
  }

private:
  template<typename... Args>
  static segment_backend_unique_ptr new_(
    segment_allocator_type al,Args&&... args)
  {
    auto p=std::allocator_traits<segment_allocator_type>::allocate(al,1);
    try{
      ::new ((void*)p) split_segment{std::forward<Args>(args)...};
    }
    catch(...){
      std::allocator_traits<segment_allocator_type>::deallocate(al,p,1);
      throw;
    }
    return {p,&delete_};
  }

  static void delete_(segment_backend* p)
  {
    auto q=static_cast<split_segment*>(p);
    auto al=segment_allocator_type{q->s.get_allocator()};
    q->~split_segment();
    std::allocator_traits<segment_allocator_type>::deallocate(al,q,1);
  }

  split_segment(const Allocator& al):
    s{typename store::allocator_type{al}},
    i{{},typename index::allocator_type{al}}
  {
    s.reserve(1); /* --> s.data()!=nullptr */
    build_index();
  }

  split_segment(store&& s_):
    s{std::move(s_)},i{{},typename index::allocator_type{s.get_allocator()}}
  {
    s.reserve(1); /* --> s.data()!=nullptr */
    build_index();
  }

  void prereserve()
  {
    if(s.size()==s.capacity())expand();
  }

  const_base_iterator prereserve(const_base_iterator p)
  {
    if(s.size()==s.capacity()){
      auto n=p-i.data();
      expand();
      return const_base_iterator{i.data()+n};
    }
    else return p;
  }

  const_iterator prereserve(const_iterator p)
  {
    if(s.size()==s.capacity()){
      auto n=p-const_concrete_ptr(s.data());
      expand();
      return const_concrete_ptr(s.data())+n;
    }
    else return p;
  }

  const_iterator prereserve(const_iterator p,std::size_t m)
  {
    if(s.size()+m>s.capacity()){
      auto n=p-const_concrete_ptr(s.data());
      expand(m);
      return const_concrete_ptr(s.data())+n;
    }
    else return p;
  }

  void expand()
  {
    std::size_t c=
      s.size()<=1||(s.max_size()-1-s.size())/2<s.size()?
        s.size()+1:
        s.size()+s.size()/2;
    i.reserve(c+1);
    s.reserve(c);
    rebuild_index();
  }

  void expand(std::size_t m)
  {
    i.reserve(s.size()+m+1);
    s.reserve(s.size()+m);
    rebuild_index();
  }

  void build_index(std::size_t start=0)
  {
    for(std::size_t n=start,m=s.size();n<=m;++n){
      i.push_back(Model::make_value_type(concrete_ref(s.data()[n])));
    };
  }

  void rebuild_index()
  {
    i.clear();
    build_index();
  }

  void push_index_entry()
  {
    build_index(s.size());
  }

  void pop_index_entry(std::size_t n=1)
  {
    while(n--)i.pop_back();
  }

  static Concrete& concrete_ref(value_pointer p)noexcept
  {
    return *static_cast<Concrete*>(p);
  }

  static Concrete& concrete_ref(store_value_type& r)noexcept
  {
    return *concrete_ptr(&r);
  }

  static const Concrete& const_concrete_ref(const_value_pointer p)noexcept
  {
    return *static_cast<const Concrete*>(p);
  }

  static Concrete* concrete_ptr(store_value_type* p)noexcept
  {
    return reinterpret_cast<Concrete*>(
      static_cast<value_holder_base<Concrete>*>(p));
  }

  static const Concrete* const_concrete_ptr(const store_value_type* p)noexcept
  {
    return concrete_ptr(const_cast<store_value_type*>(p));
  }

  static value_type* value_ptr(const value_type* p)noexcept
  {
    return const_cast<value_type*>(p);
  }

  /* It would have sufficed if iterator_from returned const_store_iterator
   * except for the fact that some old versions of libstdc++ claiming to be
   * C++11 compliant do not however provide std::vector modifier ops taking
   * const_iterator's.
   */

  store_iterator iterator_from(const_base_iterator p)
  {
    return s.begin()+(p-i.data());
  }

  store_iterator iterator_from(const_iterator p)
  {
    return s.begin()+(p-const_concrete_ptr(s.data()));
  }

  base_sentinel sentinel()const noexcept
  {
    return base_iterator{value_ptr(i.data()+s.size())};
  }

  range range_from(const_store_iterator it)const
  {
    return {base_iterator{value_ptr(i.data()+(it-s.begin()))},sentinel()};
  }
    
  range range_from(std::size_t n)const
  {
    return {base_iterator{value_ptr(i.data()+n)},sentinel()};
  }

  template<typename InputIterator>
  range insert(
    const_iterator p,InputIterator first,InputIterator last,
    std::input_iterator_tag)
  {
    std::size_t n=0;
    for(;first!=last;++first,++n,++p){
      p=prereserve(p);
      s.emplace(iterator_from(p),*first);
      push_index_entry();
    }
    return range_from(iterator_from(p-n));
  }

  template<typename InputIterator>
  range insert(
    const_iterator p,InputIterator first,InputIterator last,
    std::forward_iterator_tag)
  {
    auto n=s.size();
    auto m=static_cast<std::size_t>(std::distance(first,last));
    if(m){
      p=prereserve(p,m);
      try{
        s.insert(iterator_from(p),first,last);
      }
      catch(...){
        build_index(n+1);
        throw;
      }
      build_index(n+1);
    }
    return range_from(iterator_from(p));
  }

  store s;
  index i;
};

} /* namespace poly_collection::detail */

} /* namespace poly_collection */

} /* namespace boost */

#endif

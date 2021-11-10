//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_LOCALE_BOUNDARY_INDEX_HPP_INCLUDED
#define BOOST_LOCALE_BOUNDARY_INDEX_HPP_INCLUDED

#include <boost/locale/config.hpp>
#include <boost/locale/boundary/types.hpp>
#include <boost/locale/boundary/facets.hpp>
#include <boost/locale/boundary/segment.hpp>
#include <boost/locale/boundary/boundary_point.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <boost/assert.hpp>
#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4275 4251 4231 4660)
#endif
#include <string>
#include <locale>
#include <vector>
#include <iterator>
#include <algorithm>
#include <stdexcept>

#include <iostream>

namespace boost {

    namespace locale {
        
        namespace boundary {
            ///
            /// \defgroup boundary Boundary Analysis
            ///
            /// This module contains all operations required for %boundary analysis of text: character, word, like and sentence boundaries
            ///
            /// @{
            ///

            /// \cond INTERNAL

            namespace details {

                template<typename IteratorType,typename CategoryType = typename std::iterator_traits<IteratorType>::iterator_category>
                struct mapping_traits {
                    typedef typename std::iterator_traits<IteratorType>::value_type char_type;
                    static index_type map(boundary_type t,IteratorType b,IteratorType e,std::locale const &l)
                    {
                        std::basic_string<char_type> str(b,e);
                        return std::use_facet<boundary_indexing<char_type> >(l).map(t,str.c_str(),str.c_str()+str.size());
                    }
                };

                template<typename CharType,typename SomeIteratorType>
                struct linear_iterator_traits {
                    static const bool is_linear =
                        is_same<SomeIteratorType,CharType*>::value
                        || is_same<SomeIteratorType,CharType const*>::value
                        || is_same<SomeIteratorType,typename std::basic_string<CharType>::iterator>::value
                        || is_same<SomeIteratorType,typename std::basic_string<CharType>::const_iterator>::value
                        || is_same<SomeIteratorType,typename std::vector<CharType>::iterator>::value
                        || is_same<SomeIteratorType,typename std::vector<CharType>::const_iterator>::value
                        ;
                };



                template<typename IteratorType>
                struct mapping_traits<IteratorType,std::random_access_iterator_tag> {

                    typedef typename std::iterator_traits<IteratorType>::value_type char_type;



                    static index_type map(boundary_type t,IteratorType b,IteratorType e,std::locale const &l)
                    {
                        index_type result;

                        //
                        // Optimize for most common cases
                        //
                        // C++0x requires that string is continious in memory and all known
                        // string implementations
                        // do this because of c_str() support. 
                        //

                        if(linear_iterator_traits<char_type,IteratorType>::is_linear && b!=e)
                        {
                            char_type const *begin = &*b;
                            char_type const *end = begin + (e-b);
                            index_type tmp=std::use_facet<boundary_indexing<char_type> >(l).map(t,begin,end);
                            result.swap(tmp);
                        }
                        else {
                            std::basic_string<char_type> str(b,e);
                            index_type tmp = std::use_facet<boundary_indexing<char_type> >(l).map(t,str.c_str(),str.c_str()+str.size());
                            result.swap(tmp);
                        }
                        return result;
                    }
                };

                template<typename BaseIterator>
                class mapping {
                public:
                    typedef BaseIterator base_iterator;
                    typedef typename std::iterator_traits<base_iterator>::value_type char_type;


                    mapping(boundary_type type,
                            base_iterator begin,
                            base_iterator end,
                            std::locale const &loc) 
                        :   
                            index_(new index_type()),
                            begin_(begin),
                            end_(end)
                    {
                        index_type idx=details::mapping_traits<base_iterator>::map(type,begin,end,loc);
                        index_->swap(idx);
                    }

                    mapping()
                    {
                    }

                    index_type const &index() const
                    {
                        return *index_;
                    }

                    base_iterator begin() const
                    {
                        return begin_;
                    }

                    base_iterator end() const
                    {
                        return end_;
                    }

                private:
                    boost::shared_ptr<index_type> index_;
                    base_iterator begin_,end_;
                };

                template<typename BaseIterator>
                class segment_index_iterator : 
                    public boost::iterator_facade<
                        segment_index_iterator<BaseIterator>,
                        segment<BaseIterator>,
                        boost::bidirectional_traversal_tag,
                        segment<BaseIterator> const &
                    >
                {
                public:
                    typedef BaseIterator base_iterator;
                    typedef mapping<base_iterator> mapping_type;
                    typedef segment<base_iterator> segment_type;
                    
                    segment_index_iterator() : current_(0,0),map_(0)
                    {
                    }

                    segment_index_iterator(base_iterator p,mapping_type const *map,rule_type mask,bool full_select) :
                        map_(map),
                        mask_(mask),
                        full_select_(full_select)
                    {
                        set(p);
                    }
                    segment_index_iterator(bool is_begin,mapping_type const *map,rule_type mask,bool full_select) :
                        map_(map),
                        mask_(mask),
                        full_select_(full_select)
                    {
                        if(is_begin)
                            set_begin();
                        else
                            set_end();
                    }

                    segment_type const &dereference() const
                    {
                        return value_;
                    }

                    bool equal(segment_index_iterator const &other) const
                    {
                        return map_ == other.map_ && current_.second == other.current_.second;
                    }

                    void increment()
                    {
                        std::pair<size_t,size_t> next = current_;
                        if(full_select_) {
                            next.first = next.second;
                            while(next.second < size()) {
                                next.second++;
                                if(valid_offset(next.second))
                                    break;
                            }
                            if(next.second == size())
                                next.first = next.second - 1;
                        }
                        else {
                            while(next.second < size()) {
                                next.first = next.second;
                                next.second++;
                                if(valid_offset(next.second))
                                    break;
                            }
                        }
                        update_current(next);
                    }

                    void decrement()
                    {
                        std::pair<size_t,size_t> next = current_;
                        if(full_select_) {
                            while(next.second >1) {
                                next.second--;
                                if(valid_offset(next.second))
                                    break;
                            }
                            next.first = next.second;
                            while(next.first >0) {
                                next.first--;
                                if(valid_offset(next.first))
                                    break;
                            }
                        }
                        else {
                            while(next.second >1) {
                                next.second--;
                                if(valid_offset(next.second))
                                    break;
                            }
                            next.first = next.second - 1;
                        }
                        update_current(next);
                    }

                private:

                    void set_end()
                    {
                        current_.first  = size() - 1;
                        current_.second = size();
                        value_ = segment_type(map_->end(),map_->end(),0);
                    }
                    void set_begin()
                    {
                        current_.first = current_.second = 0;
                        value_ = segment_type(map_->begin(),map_->begin(),0);
                        increment();
                    }

                    void set(base_iterator p)
                    {
                        size_t dist=std::distance(map_->begin(),p);
                        index_type::const_iterator b=map_->index().begin(),e=map_->index().end();
                        index_type::const_iterator 
                            boundary_point=std::upper_bound(b,e,break_info(dist));
                        while(boundary_point != e && (boundary_point->rule & mask_)==0)
                            boundary_point++;

                        current_.first = current_.second = boundary_point - b;
                        
                        if(full_select_) {
                            while(current_.first > 0) {
                                current_.first --;
                                if(valid_offset(current_.first))
                                    break;
                            }
                        }
                        else {
                            if(current_.first > 0)
                                current_.first --;
                        }
                        value_.first = map_->begin();
                        std::advance(value_.first,get_offset(current_.first));
                        value_.second = value_.first;
                        std::advance(value_.second,get_offset(current_.second) - get_offset(current_.first));

                        update_rule();
                    }

                    void update_current(std::pair<size_t,size_t> pos)
                    {
                        std::ptrdiff_t first_diff = get_offset(pos.first) - get_offset(current_.first);
                        std::ptrdiff_t second_diff = get_offset(pos.second) - get_offset(current_.second);
                        std::advance(value_.first,first_diff);
                        std::advance(value_.second,second_diff);
                        current_ = pos;
                        update_rule();
                    }

                    void update_rule()
                    {
                        if(current_.second != size()) {
                            value_.rule(index()[current_.second].rule);
                        }
                    }
                    size_t get_offset(size_t ind) const
                    {
                        if(ind == size())
                            return index().back().offset;
                        return index()[ind].offset;
                    }

                    bool valid_offset(size_t offset) const
                    {
                        return  offset == 0 
                                || offset == size() // make sure we not acess index[size]
                                || (index()[offset].rule & mask_)!=0;
                    }
                    
                    size_t size() const
                    {
                        return index().size();
                    }
                    
                    index_type const &index() const
                    {
                        return map_->index();
                    }
                
                    
                    segment_type value_;
                    std::pair<size_t,size_t> current_;
                    mapping_type const *map_;
                    rule_type mask_;
                    bool full_select_;
                };
                            
                template<typename BaseIterator>
                class boundary_point_index_iterator : 
                    public boost::iterator_facade<
                        boundary_point_index_iterator<BaseIterator>,
                        boundary_point<BaseIterator>,
                        boost::bidirectional_traversal_tag,
                        boundary_point<BaseIterator> const &
                    >
                {
                public:
                    typedef BaseIterator base_iterator;
                    typedef mapping<base_iterator> mapping_type;
                    typedef boundary_point<base_iterator> boundary_point_type;
                    
                    boundary_point_index_iterator() : current_(0),map_(0)
                    {
                    }

                    boundary_point_index_iterator(bool is_begin,mapping_type const *map,rule_type mask) :
                        map_(map),
                        mask_(mask)
                    {
                        if(is_begin)
                            set_begin();
                        else
                            set_end();
                    }
                    boundary_point_index_iterator(base_iterator p,mapping_type const *map,rule_type mask) :
                        map_(map),
                        mask_(mask)
                    {
                        set(p);
                    }

                    boundary_point_type const &dereference() const
                    {
                        return value_;
                    }

                    bool equal(boundary_point_index_iterator const &other) const
                    {
                        return map_ == other.map_ && current_ == other.current_;
                    }

                    void increment()
                    {
                        size_t next = current_;
                        while(next < size()) {
                            next++;
                            if(valid_offset(next))
                                break;
                        }
                        update_current(next);
                    }

                    void decrement()
                    {
                        size_t next = current_;
                        while(next>0) {
                            next--;
                            if(valid_offset(next))
                                break;
                        }
                        update_current(next);
                    }

                private:
                    void set_end()
                    {
                        current_ = size();
                        value_ = boundary_point_type(map_->end(),0);
                    }
                    void set_begin()
                    {
                        current_ = 0;
                        value_ = boundary_point_type(map_->begin(),0);
                    }

                    void set(base_iterator p)
                    {
                        size_t dist =  std::distance(map_->begin(),p);

                        index_type::const_iterator b=index().begin();
                        index_type::const_iterator e=index().end();
                        index_type::const_iterator ptr = std::lower_bound(b,e,break_info(dist));

                        if(ptr==index().end())
                            current_=size()-1;
                        else
                            current_=ptr - index().begin();

                        while(!valid_offset(current_))
                            current_ ++;

                        std::ptrdiff_t diff = get_offset(current_) - dist;
                        std::advance(p,diff);
                        value_.iterator(p);
                        update_rule();
                    }

                    void update_current(size_t pos)
                    {
                        std::ptrdiff_t diff = get_offset(pos) - get_offset(current_);
                        base_iterator i=value_.iterator();
                        std::advance(i,diff);
                        current_ = pos;
                        value_.iterator(i);
                        update_rule();
                    }

                    void update_rule()
                    {
                        if(current_ != size()) {
                            value_.rule(index()[current_].rule);
                        }
                    }
                    size_t get_offset(size_t ind) const
                    {
                        if(ind == size())
                            return index().back().offset;
                        return index()[ind].offset;
                    }

                    bool valid_offset(size_t offset) const
                    {
                        return  offset == 0 
                                || offset + 1 >= size() // last and first are always valid regardless of mark
                                || (index()[offset].rule & mask_)!=0;
                    }
                    
                    size_t size() const
                    {
                        return index().size();
                    }
                    
                    index_type const &index() const
                    {
                        return map_->index();
                    }
                
                    
                    boundary_point_type value_;
                    size_t current_;
                    mapping_type const *map_;
                    rule_type mask_;
                };


            } // details

            /// \endcond

            template<typename BaseIterator>
            class segment_index;

            template<typename BaseIterator>
            class boundary_point_index;
            

            ///
            /// \brief This class holds an index of segments in the text range and allows to iterate over them 
            ///
            /// This class is provides \ref begin() and \ref end() member functions that return bidirectional iterators
            /// to the \ref segment objects.
            ///
            /// It provides two options on way of selecting segments:
            ///
            /// -   \ref rule(rule_type mask) - a mask that allows to select only specific types of segments according to
            ///     various masks %as \ref word_any.
            ///     \n
            ///     The default is to select any types of boundaries.
            ///     \n 
            ///     For example: using word %boundary analysis, when the provided mask is \ref word_kana then the iterators
            ///     would iterate only over the words containing Kana letters and \ref word_any would select all types of
            ///     words excluding ranges that consist of white space and punctuation marks. So iterating over the text
            ///     "to be or not to be?" with \ref word_any rule would return segments "to", "be", "or", "not", "to", "be", instead
            ///     of default "to", " ", "be", " ", "or", " ", "not", " ", "to", " ", "be", "?".
            /// -   \ref full_select(bool how) - a flag that defines the way a range is selected if the rule of the previous
            ///     %boundary point does not fit the selected rule.
            ///     \n
            ///     For example: We want to fetch all sentences from the following text: "Hello! How\nare you?".
            ///     \n
            ///     This text contains three %boundary points separating it to sentences by different rules:
            ///     - The exclamation mark "!" ends the sentence "Hello!"
            ///     - The line feed that splits the sentence "How\nare you?" into two parts.
            ///     - The question mark that ends the second sentence.
            ///     \n
            ///     If you would only change the \ref rule() to \ref sentence_term then the segment_index would
            ///     provide two sentences "Hello!" and "are you?" %as only them actually terminated with required
            ///     terminator "!" or "?". But changing \ref full_select() to true, the selected segment would include
            ///     all the text up to previous valid %boundary point and would return two expected sentences:
            ///     "Hello!" and "How\nare you?".
            ///     
            /// This class allows to find a segment according to the given iterator in range using \ref find() member
            /// function.
            ///
            /// \note
            ///
            /// -   Changing any of the options - \ref rule() or \ref full_select() and of course re-indexing the text
            ///     invalidates existing iterators and they can't be used any more.
            /// -   segment_index can be created from boundary_point_index or other segment_index that was created with
            ///     same \ref boundary_type.  This is very fast operation %as they shared same index
            ///     and it does not require its regeneration.
            ///
            /// \see
            ///
            /// - \ref boundary_point_index
            /// - \ref segment
            /// - \ref boundary_point
            ///

            template<typename BaseIterator>
            class segment_index {
            public:
                
                ///
                /// The type of the iterator used to iterate over the original text
                ///
                typedef BaseIterator base_iterator;
                #ifdef BOOST_LOCALE_DOXYGEN
                ///
                /// The bidirectional iterator that iterates over \ref value_type objects.
                ///
                /// -   The iterators may be invalidated by use of any non-const member function
                ///     including but not limited to \ref rule(rule_type) and \ref full_select(bool).
                /// -   The returned value_type object is valid %as long %as iterator points to it.
                ///     So this following code is wrong %as t used after p was updated:
                ///     \code
                ///     segment_index<some_iterator>::iterator p=index.begin();
                ///     segment<some_iterator> &t = *p;
                ///     ++p;
                ///     cout << t.str() << endl;
                ///     \endcode
                ///
                typedef unspecified_iterator_type iterator;
                ///
                /// \copydoc iterator
                ///
                typedef unspecified_iterator_type const_iterator;
                #else
                typedef details::segment_index_iterator<base_iterator> iterator;
                typedef details::segment_index_iterator<base_iterator> const_iterator;
                #endif
                ///
                /// The type dereferenced by the \ref iterator and \ref const_iterator. It is
                /// an object that represents selected segment.
                ///
                typedef segment<base_iterator> value_type;

                ///
                /// Default constructor. 
                ///
                /// \note
                ///
                /// When this object is constructed by default it does not include a valid index, thus
                /// calling \ref begin(), \ref end() or \ref find() member functions would lead to undefined
                /// behavior
                ///
                segment_index() : mask_(0xFFFFFFFFu),full_select_(false)
                {
                }
                ///
                /// Create a segment_index for %boundary analysis \ref boundary_type "type" of the text
                /// in range [begin,end) using a rule \a mask for locale \a loc.
                ///
                segment_index(boundary_type type,
                            base_iterator begin,
                            base_iterator end,
                            rule_type mask,
                            std::locale const &loc=std::locale()) 
                    :
                        map_(type,begin,end,loc),
                        mask_(mask),
                        full_select_(false)
                {
                }
                ///
                /// Create a segment_index for %boundary analysis \ref boundary_type "type" of the text
                /// in range [begin,end) selecting all possible segments (full mask) for locale \a loc.
                ///
                segment_index(boundary_type type,
                            base_iterator begin,
                            base_iterator end,
                            std::locale const &loc=std::locale()) 
                    :
                        map_(type,begin,end,loc),
                        mask_(0xFFFFFFFFu),
                        full_select_(false)
                {
                }

                ///
                /// Create a segment_index from a \ref boundary_point_index. It copies all indexing information
                /// and used default rule (all possible segments)
                ///
                /// This operation is very cheap, so if you use boundary_point_index and segment_index on same text
                /// range it is much better to create one from another rather then indexing the same
                /// range twice.
                ///
                /// \note \ref rule() flags are not copied
                ///
                segment_index(boundary_point_index<base_iterator> const &);
                ///
                /// Copy an index from a \ref boundary_point_index. It copies all indexing information
                /// and uses the default rule (all possible segments)
                ///
                /// This operation is very cheap, so if you use boundary_point_index and segment_index on same text
                /// range it is much better to create one from another rather then indexing the same
                /// range twice.
                ///
                /// \note \ref rule() flags are not copied
                ///
                segment_index const &operator = (boundary_point_index<base_iterator> const &);

                
                ///
                /// Create a new index for %boundary analysis \ref boundary_type "type" of the text
                /// in range [begin,end) for locale \a loc.
                ///
                /// \note \ref rule() and \ref full_select() remain unchanged.
                ///
                void map(boundary_type type,base_iterator begin,base_iterator end,std::locale const &loc=std::locale())
                {
                    map_ = mapping_type(type,begin,end,loc);
                }

                ///
                /// Get the \ref iterator on the beginning of the segments range.
                ///
                /// Preconditions: the segment_index should have a mapping
                ///
                /// \note
                ///
                /// The returned iterator is invalidated by access to any non-const member functions of this object
                ///
                iterator begin() const
                {
                    return iterator(true,&map_,mask_,full_select_);
                }

                ///
                /// Get the \ref iterator on the ending of the segments range.
                ///
                /// Preconditions: the segment_index should have a mapping
                ///
                /// The returned iterator is invalidated by access to any non-const member functions of this object
                ///
                iterator end() const
                {
                    return iterator(false,&map_,mask_,full_select_);
                }

                ///
                /// Find a first valid segment following a position \a p. 
                ///
                /// If \a p is inside a valid segment this segment is selected:
                ///
                /// For example: For \ref word %boundary analysis with \ref word_any rule():
                ///
                /// - "to| be or ", would point to "be",
                /// - "t|o be or ", would point to "to",
                /// - "to be or| ", would point to end.
                ///
                ///                 
                /// Preconditions: the segment_index should have a mapping and \a p should be valid iterator
                /// to the text in the mapped range.
                ///
                /// The returned iterator is invalidated by access to any non-const member functions of this object
                ///
                iterator find(base_iterator p) const
                {
                    return iterator(p,&map_,mask_,full_select_);
                }
               
                ///
                /// Get the mask of rules that are used
                /// 
                rule_type rule() const
                {
                    return mask_;
                }
                ///
                /// Set the mask of rules that are used
                /// 
                void rule(rule_type v)
                {
                    mask_ = v;
                }

                ///
                /// Get the full_select property value -  should segment include in the range
                /// values that not belong to specific \ref rule() or not.
                ///
                /// The default value is false.
                ///
                /// For example for \ref sentence %boundary with rule \ref sentence_term the segments
                /// of text "Hello! How\nare you?" are "Hello!\", "are you?" when full_select() is false
                /// because "How\n" is selected %as sentence by a rule spits the text by line feed. If full_select()
                /// is true the returned segments are "Hello! ", "How\nare you?" where "How\n" is joined with the
                /// following part "are you?"
                ///

                bool full_select()  const 
                {
                    return full_select_;
                }

                ///
                /// Set the full_select property value -  should segment include in the range
                /// values that not belong to specific \ref rule() or not.
                ///
                /// The default value is false.
                ///
                /// For example for \ref sentence %boundary with rule \ref sentence_term the segments
                /// of text "Hello! How\nare you?" are "Hello!\", "are you?" when full_select() is false
                /// because "How\n" is selected %as sentence by a rule spits the text by line feed. If full_select()
                /// is true the returned segments are "Hello! ", "How\nare you?" where "How\n" is joined with the
                /// following part "are you?"
                ///

                void full_select(bool v) 
                {
                    full_select_ = v;
                }
                
            private:
                friend class boundary_point_index<base_iterator>;
                typedef details::mapping<base_iterator> mapping_type;
                mapping_type  map_;
                rule_type mask_;
                bool full_select_;
            };

            ///
            /// \brief This class holds an index of \ref boundary_point "boundary points" and allows iterating
            /// over them.
            ///
            /// This class is provides \ref begin() and \ref end() member functions that return bidirectional iterators
            /// to the \ref boundary_point objects.
            ///
            /// It provides an option that affects selecting %boundary points according to different rules:
            /// using \ref rule(rule_type mask) member function. It allows to set a mask that select only specific
            /// types of %boundary points like \ref sentence_term.
            ///
            /// For example for a sentence %boundary analysis of a text "Hello! How\nare you?" when the default
            /// rule is used the %boundary points would be:
            ///
            /// - "|Hello! How\nare you?"
            /// - "Hello! |How\nare you?"
            /// - "Hello! How\n|are you?"
            /// - "Hello! How\nare you?|"
            ///
            /// However if \ref rule() is set to \ref sentence_term then the selected %boundary points would be: 
            ///
            /// - "|Hello! How\nare you?"
            /// - "Hello! |How\nare you?"
            /// - "Hello! How\nare you?|"
            /// 
            /// Such that a %boundary point defined by a line feed character would be ignored.
            ///     
            /// This class allows to find a boundary_point according to the given iterator in range using \ref find() member
            /// function.
            ///
            /// \note
            /// -   Even an empty text range [x,x) considered to have a one %boundary point x.
            /// -   \a a and \a b points of the range [a,b) are always considered %boundary points
            ///     regardless the rules used.
            /// -   Changing any of the option \ref rule() or course re-indexing the text
            ///     invalidates existing iterators and they can't be used any more.
            /// -   boundary_point_index can be created from segment_index or other boundary_point_index that was created with
            ///     same \ref boundary_type.  This is very fast operation %as they shared same index
            ///     and it does not require its regeneration.
            ///
            /// \see
            ///
            /// - \ref segment_index
            /// - \ref boundary_point
            /// - \ref segment
            ///


            template<typename BaseIterator>
            class boundary_point_index {
            public:
                ///
                /// The type of the iterator used to iterate over the original text
                ///
                typedef BaseIterator base_iterator;
                #ifdef BOOST_LOCALE_DOXYGEN
                ///
                /// The bidirectional iterator that iterates over \ref value_type objects.
                ///
                /// -   The iterators may be invalidated by use of any non-const member function
                ///     including but not limited to \ref rule(rule_type) member function.
                /// -   The returned value_type object is valid %as long %as iterator points to it.
                ///     So this following code is wrong %as t used after p was updated:
                ///     \code
                ///     boundary_point_index<some_iterator>::iterator p=index.begin();
                ///     boundary_point<some_iterator> &t = *p;
                ///     ++p;
                ///     rule_type r = t->rule();
                ///     \endcode
                ///
                typedef unspecified_iterator_type iterator;
                ///
                /// \copydoc iterator
                ///
                typedef unspecified_iterator_type const_iterator;
                #else
                typedef details::boundary_point_index_iterator<base_iterator> iterator;
                typedef details::boundary_point_index_iterator<base_iterator> const_iterator;
                #endif
                ///
                /// The type dereferenced by the \ref iterator and \ref const_iterator. It is
                /// an object that represents the selected \ref boundary_point "boundary point".
                ///
                typedef boundary_point<base_iterator> value_type;
                
                ///
                /// Default constructor. 
                ///
                /// \note
                ///
                /// When this object is constructed by default it does not include a valid index, thus
                /// calling \ref begin(), \ref end() or \ref find() member functions would lead to undefined
                /// behavior
                ///
                boundary_point_index() : mask_(0xFFFFFFFFu)
                {
                }
                
                ///
                /// Create a segment_index for %boundary analysis \ref boundary_type "type" of the text
                /// in range [begin,end) using a rule \a mask for locale \a loc.
                ///
                boundary_point_index(boundary_type type,
                            base_iterator begin,
                            base_iterator end,
                            rule_type mask,
                            std::locale const &loc=std::locale()) 
                    :
                        map_(type,begin,end,loc),
                        mask_(mask)
                {
                }
                ///
                /// Create a segment_index for %boundary analysis \ref boundary_type "type" of the text
                /// in range [begin,end) selecting all possible %boundary points (full mask) for locale \a loc.
                ///
                boundary_point_index(boundary_type type,
                            base_iterator begin,
                            base_iterator end,
                            std::locale const &loc=std::locale()) 
                    :
                        map_(type,begin,end,loc),
                        mask_(0xFFFFFFFFu)
                {
                }

                ///
                /// Create a boundary_point_index from a \ref segment_index. It copies all indexing information
                /// and uses the default rule (all possible %boundary points)
                ///
                /// This operation is very cheap, so if you use boundary_point_index and segment_index on same text
                /// range it is much better to create one from another rather then indexing the same
                /// range twice.
                ///
                /// \note \ref rule() flags are not copied
                ///
                boundary_point_index(segment_index<base_iterator> const &other);
                ///
                /// Copy a boundary_point_index from a \ref segment_index. It copies all indexing information
                /// and keeps the current \ref rule() unchanged
                ///
                /// This operation is very cheap, so if you use boundary_point_index and segment_index on same text
                /// range it is much better to create one from another rather then indexing the same
                /// range twice.
                ///
                /// \note \ref rule() flags are not copied
                ///
                boundary_point_index const &operator=(segment_index<base_iterator> const &other);

                ///
                /// Create a new index for %boundary analysis \ref boundary_type "type" of the text
                /// in range [begin,end) for locale \a loc.
                ///
                /// \note \ref rule() remains unchanged.
                ///
                void map(boundary_type type,base_iterator begin,base_iterator end,std::locale const &loc=std::locale())
                {
                    map_ = mapping_type(type,begin,end,loc);
                }

                ///
                /// Get the \ref iterator on the beginning of the %boundary points range.
                ///
                /// Preconditions: this boundary_point_index should have a mapping
                ///
                /// \note
                ///
                /// The returned iterator is invalidated by access to any non-const member functions of this object
                ///
                iterator begin() const
                {
                    return iterator(true,&map_,mask_);
                }

                ///
                /// Get the \ref iterator on the ending of the %boundary points range.
                ///
                /// Preconditions: this boundary_point_index should have a mapping
                ///
                /// \note
                ///
                /// The returned iterator is invalidated by access to any non-const member functions of this object
                ///
                iterator end() const
                {
                    return iterator(false,&map_,mask_);
                }

                ///
                /// Find a first valid %boundary point on a position \a p or following it.
                ///
                /// For example: For \ref word %boundary analysis of the text "to be or"
                ///
                /// - "|to be", would return %boundary point at "|to be",
                /// - "t|o be", would point to "to| be"
                ///                 
                /// Preconditions: the boundary_point_index should have a mapping and \a p should be valid iterator
                /// to the text in the mapped range.
                ///
                /// The returned iterator is invalidated by access to any non-const member functions of this object
                ///
                iterator find(base_iterator p) const
                {
                    return iterator(p,&map_,mask_);
                }
                
                ///
                /// Get the mask of rules that are used
                /// 
                rule_type rule() const
                {
                    return mask_;
                }
                ///
                /// Set the mask of rules that are used
                /// 
                void rule(rule_type v)
                {
                    mask_ = v;
                }

            private:

                friend class segment_index<base_iterator>;
                typedef details::mapping<base_iterator> mapping_type;
                mapping_type  map_;
                rule_type mask_;
            };
           
            /// \cond INTERNAL  
            template<typename BaseIterator>
            segment_index<BaseIterator>::segment_index(boundary_point_index<BaseIterator> const &other) :
                map_(other.map_),
                mask_(0xFFFFFFFFu),
                full_select_(false)
            {
            }
            
            template<typename BaseIterator>
            boundary_point_index<BaseIterator>::boundary_point_index(segment_index<BaseIterator> const &other) :
                map_(other.map_),
                mask_(0xFFFFFFFFu)
            {
            }

            template<typename BaseIterator>
            segment_index<BaseIterator> const &segment_index<BaseIterator>::operator=(boundary_point_index<BaseIterator> const &other)
            {
                map_ = other.map_;
                return *this;
            }
            
            template<typename BaseIterator>
            boundary_point_index<BaseIterator> const &boundary_point_index<BaseIterator>::operator=(segment_index<BaseIterator> const &other)
            {
                map_ = other.map_;
                return *this;
            }
            /// \endcond
          
            typedef segment_index<std::string::const_iterator> ssegment_index;      ///< convenience typedef
            typedef segment_index<std::wstring::const_iterator> wssegment_index;    ///< convenience typedef
            #ifdef BOOST_LOCALE_ENABLE_CHAR16_T
            typedef segment_index<std::u16string::const_iterator> u16ssegment_index;///< convenience typedef
            #endif
            #ifdef BOOST_LOCALE_ENABLE_CHAR32_T
            typedef segment_index<std::u32string::const_iterator> u32ssegment_index;///< convenience typedef
            #endif
           
            typedef segment_index<char const *> csegment_index;                     ///< convenience typedef
            typedef segment_index<wchar_t const *> wcsegment_index;                 ///< convenience typedef
            #ifdef BOOST_LOCALE_ENABLE_CHAR16_T
            typedef segment_index<char16_t const *> u16csegment_index;              ///< convenience typedef
            #endif
            #ifdef BOOST_LOCALE_ENABLE_CHAR32_T
            typedef segment_index<char32_t const *> u32csegment_index;              ///< convenience typedef
            #endif

            typedef boundary_point_index<std::string::const_iterator> sboundary_point_index;///< convenience typedef
            typedef boundary_point_index<std::wstring::const_iterator> wsboundary_point_index;///< convenience typedef
            #ifdef BOOST_LOCALE_ENABLE_CHAR16_T
            typedef boundary_point_index<std::u16string::const_iterator> u16sboundary_point_index;///< convenience typedef
            #endif
            #ifdef BOOST_LOCALE_ENABLE_CHAR32_T
            typedef boundary_point_index<std::u32string::const_iterator> u32sboundary_point_index;///< convenience typedef
            #endif
           
            typedef boundary_point_index<char const *> cboundary_point_index;       ///< convenience typedef
            typedef boundary_point_index<wchar_t const *> wcboundary_point_index;   ///< convenience typedef
            #ifdef BOOST_LOCALE_ENABLE_CHAR16_T
            typedef boundary_point_index<char16_t const *> u16cboundary_point_index;///< convenience typedef
            #endif
            #ifdef BOOST_LOCALE_ENABLE_CHAR32_T
            typedef boundary_point_index<char32_t const *> u32cboundary_point_index;///< convenience typedef
            #endif



        } // boundary

    } // locale
} // boost

///
/// \example boundary.cpp
/// Example of using segment_index
/// \example wboundary.cpp
/// Example of using segment_index over wide strings
///

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4

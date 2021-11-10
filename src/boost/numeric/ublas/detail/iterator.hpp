//
//  Copyright (c) 2000-2002
//  Joerg Walter, Mathias Koch
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  The authors gratefully acknowledge the support of
//  GeNeSys mbH & Co. KG in producing this work.
//

#ifndef _BOOST_UBLAS_ITERATOR_
#define _BOOST_UBLAS_ITERATOR_

#include <boost/numeric/ublas/exception.hpp>
#include <iterator>


namespace boost { namespace numeric { namespace ublas {

  /** \brief Base class of all proxy classes that contain
   *       a (redirectable) reference to an immutable object.
   *
   *       \param C the type of the container referred to
   */
    template<class C>
    class container_const_reference:
        private nonassignable {
    public:
        typedef C container_type;

        BOOST_UBLAS_INLINE
        container_const_reference ():
            c_ (0) {}
        BOOST_UBLAS_INLINE
        container_const_reference (const container_type &c):
            c_ (&c) {}

        BOOST_UBLAS_INLINE
        const container_type &operator () () const {
            return *c_;
        }

        BOOST_UBLAS_INLINE
        container_const_reference &assign (const container_type *c) {
            c_ = c;
            return *this;
        }
        
        // Closure comparison
        BOOST_UBLAS_INLINE
        bool same_closure (const container_const_reference &cr) const {
            return c_ == cr.c_;
        }

    private:
        const container_type *c_;
    };

  /** \brief Base class of all proxy classes that contain
   *         a (redirectable) reference to a mutable object.
   *
   * \param C the type of the container referred to
   */
    template<class C>
    class container_reference:
        private nonassignable {
    public:
        typedef C container_type;

        BOOST_UBLAS_INLINE
        container_reference ():
            c_ (0) {}
        BOOST_UBLAS_INLINE
        container_reference (container_type &c):
            c_ (&c) {}

        BOOST_UBLAS_INLINE
        container_type &operator () () const {
           return *c_;
        }

        BOOST_UBLAS_INLINE
        container_reference &assign (container_type *c) {
            c_ = c;
            return *this;
        }

        // Closure comparison
        BOOST_UBLAS_INLINE
        bool same_closure (const container_reference &cr) const {
            return c_ == cr.c_;
        }

    private:
        container_type *c_;
    };

  /** \brief Base class of all forward iterators.
   * 
   *  \param IC the iterator category
   *  \param I the derived iterator type
   *  \param T the value type
   * 
   * The forward iterator can only proceed in one direction
   * via the post increment operator.
   */
    template<class IC, class I, class T>
    struct forward_iterator_base:
        public std::iterator<IC, T> {
        typedef I derived_iterator_type;
        typedef T derived_value_type;

        // Arithmetic
        BOOST_UBLAS_INLINE
        derived_iterator_type operator ++ (int) {
            derived_iterator_type &d (*static_cast<const derived_iterator_type *> (this));
            derived_iterator_type tmp (d);
            ++ d;
            return tmp;
        }
        BOOST_UBLAS_INLINE
        friend derived_iterator_type operator ++ (derived_iterator_type &d, int) {
            derived_iterator_type tmp (d);
            ++ d;
            return tmp;
        }

        // Comparison
        BOOST_UBLAS_INLINE
        bool operator != (const derived_iterator_type &it) const {
            const derived_iterator_type *d = static_cast<const derived_iterator_type *> (this);
            return ! (*d == it);
        }
    };

  /** \brief Base class of all bidirectional iterators.
   *
   * \param IC the iterator category
   * \param I the derived iterator type
   * \param T the value type
   *
   * The bidirectional iterator can proceed in both directions
   * via the post increment and post decrement operator.
   */
    template<class IC, class I, class T>
    struct bidirectional_iterator_base:
        public std::iterator<IC, T> {
        typedef I derived_iterator_type;
        typedef T derived_value_type;

        // Arithmetic
        BOOST_UBLAS_INLINE
        derived_iterator_type operator ++ (int) {
            derived_iterator_type &d (*static_cast<const derived_iterator_type *> (this));
            derived_iterator_type tmp (d);
            ++ d;
            return tmp;
        }
        BOOST_UBLAS_INLINE
        friend derived_iterator_type operator ++ (derived_iterator_type &d, int) {
            derived_iterator_type tmp (d);
            ++ d;
            return tmp;
        }
        BOOST_UBLAS_INLINE
        derived_iterator_type operator -- (int) {
            derived_iterator_type &d (*static_cast<const derived_iterator_type *> (this));
            derived_iterator_type tmp (d);
            -- d;
            return tmp;
        }
        BOOST_UBLAS_INLINE
        friend derived_iterator_type operator -- (derived_iterator_type &d, int) {
            derived_iterator_type tmp (d);
            -- d;
            return tmp;
        }

        // Comparison
        BOOST_UBLAS_INLINE
        bool operator != (const derived_iterator_type &it) const {
            const derived_iterator_type *d = static_cast<const derived_iterator_type *> (this);
            return ! (*d == it);
        }
    };

  /** \brief Base class of all random access iterators.
   *
   * \param IC the iterator category
   * \param I the derived iterator type
   * \param T the value type
   * \param D the difference type, default: std::ptrdiff_t
   *
   * The random access iterator can proceed in both directions
   * via the post increment/decrement operator or in larger steps
   * via the +, - and +=, -= operators. The random access iterator
   * is LessThan Comparable.
   */
    template<class IC, class I, class T, class D = std::ptrdiff_t>
    // ISSUE the default for D seems rather dangerous as it can easily be (silently) incorrect
    struct random_access_iterator_base:
        public std::iterator<IC, T> {
        typedef I derived_iterator_type;
        typedef T derived_value_type;
        typedef D derived_difference_type;

        /* FIXME Need to explicitly pass derived_reference_type as otherwise I undefined type or forward declared
        typedef typename derived_iterator_type::reference derived_reference_type;
        // Indexed element
        BOOST_UBLAS_INLINE
        derived_reference_type operator [] (derived_difference_type n) {
            return *(*this + n);
        }
        */

        // Arithmetic
        BOOST_UBLAS_INLINE
        derived_iterator_type operator ++ (int) {
            derived_iterator_type &d (*static_cast<derived_iterator_type *> (this));
            derived_iterator_type tmp (d);
            ++ d;
            return tmp;
        }
        BOOST_UBLAS_INLINE
        friend derived_iterator_type operator ++ (derived_iterator_type &d, int) {
            derived_iterator_type tmp (d);
            ++ d;
            return tmp;
        }
        BOOST_UBLAS_INLINE
        derived_iterator_type operator -- (int) {
            derived_iterator_type &d (*static_cast<derived_iterator_type *> (this));
            derived_iterator_type tmp (d);
            -- d;
            return tmp;
        }
        BOOST_UBLAS_INLINE
        friend derived_iterator_type operator -- (derived_iterator_type &d, int) {
            derived_iterator_type tmp (d);
            -- d;
            return tmp;
        }
        BOOST_UBLAS_INLINE
        derived_iterator_type operator + (derived_difference_type n) const {
            derived_iterator_type tmp (*static_cast<const derived_iterator_type *> (this));
            return tmp += n;
        }
        BOOST_UBLAS_INLINE
        friend derived_iterator_type operator + (const derived_iterator_type &d, derived_difference_type n) {
            derived_iterator_type tmp (d);
            return tmp += n;
        }
        BOOST_UBLAS_INLINE
        friend derived_iterator_type operator + (derived_difference_type n, const derived_iterator_type &d) {
            derived_iterator_type tmp (d);
            return tmp += n;
        }
        BOOST_UBLAS_INLINE
        derived_iterator_type operator - (derived_difference_type n) const {
            derived_iterator_type tmp (*static_cast<const derived_iterator_type *> (this));
            return tmp -= n;
        }
        BOOST_UBLAS_INLINE
        friend derived_iterator_type operator - (const derived_iterator_type &d, derived_difference_type n) {
            derived_iterator_type tmp (d);
            return tmp -= n;
        }

        // Comparison
        BOOST_UBLAS_INLINE
        bool operator != (const derived_iterator_type &it) const {
            const derived_iterator_type *d = static_cast<const derived_iterator_type *> (this);
            return ! (*d == it);
        }
        BOOST_UBLAS_INLINE
        bool operator <= (const derived_iterator_type &it) const {
            const derived_iterator_type *d = static_cast<const derived_iterator_type *> (this);
            return ! (it < *d);
        }
        BOOST_UBLAS_INLINE
        bool operator >= (const derived_iterator_type &it) const {
            const derived_iterator_type *d = static_cast<const derived_iterator_type *> (this);
            return ! (*d < it);
        }
        BOOST_UBLAS_INLINE
        bool operator > (const derived_iterator_type &it) const {
            const derived_iterator_type *d = static_cast<const derived_iterator_type *> (this);
            return it < *d;
        }
    };

  /** \brief Base class of all reverse iterators. (non-MSVC version)
   *
   * \param I the derived iterator type
   * \param T the value type
   * \param R the reference type
   *
   * The reverse iterator implements a bidirectional iterator
   * reversing the elements of the underlying iterator. It
   * implements most operators of a random access iterator.
   *
   * uBLAS extension: it.index()
   */

    // Renamed this class from reverse_iterator to get
    // typedef reverse_iterator<...> reverse_iterator
    // working. Thanks to Gabriel Dos Reis for explaining this.
    template <class I>
    class reverse_iterator_base:
        public std::reverse_iterator<I> {
    public:
        typedef typename I::container_type container_type;
        typedef typename container_type::size_type size_type;
        typedef typename I::difference_type difference_type;
        typedef I iterator_type;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        reverse_iterator_base ():
            std::reverse_iterator<iterator_type> () {}
        BOOST_UBLAS_INLINE
        reverse_iterator_base (const iterator_type &it):
            std::reverse_iterator<iterator_type> (it) {}

        // Arithmetic
        BOOST_UBLAS_INLINE
        reverse_iterator_base &operator ++ () {
            return *this = -- this->base ();
        }
        BOOST_UBLAS_INLINE
        reverse_iterator_base operator ++ (int) {
            reverse_iterator_base tmp (*this);
            *this = -- this->base ();
            return tmp;
        }
        BOOST_UBLAS_INLINE
        reverse_iterator_base &operator -- () {
            return *this = ++ this->base ();
        }
        BOOST_UBLAS_INLINE
        reverse_iterator_base operator -- (int) {
            reverse_iterator_base tmp (*this);
            *this = ++ this->base ();
            return tmp;
        }
        BOOST_UBLAS_INLINE
        reverse_iterator_base &operator += (difference_type n) {
            return *this = this->base () - n;
        }
        BOOST_UBLAS_INLINE
        reverse_iterator_base &operator -= (difference_type n) {
            return *this = this->base () + n;
        }

        BOOST_UBLAS_INLINE
        friend reverse_iterator_base operator + (const reverse_iterator_base &it, difference_type n) {
            reverse_iterator_base tmp (it);
            return tmp += n;
        }
        BOOST_UBLAS_INLINE
        friend reverse_iterator_base operator + (difference_type n, const reverse_iterator_base &it) {
            reverse_iterator_base tmp (it);
            return tmp += n;
        }
        BOOST_UBLAS_INLINE
        friend reverse_iterator_base operator - (const reverse_iterator_base &it, difference_type n) {
            reverse_iterator_base tmp (it);
            return tmp -= n;
        }
        BOOST_UBLAS_INLINE
        friend difference_type operator - (const reverse_iterator_base &it1, const reverse_iterator_base &it2) {
            return it2.base () - it1.base ();
        }

        BOOST_UBLAS_INLINE
        const container_type &operator () () const {
            return this->base () ();
        }

        BOOST_UBLAS_INLINE
        size_type index () const {
            iterator_type tmp (this->base ());
            return (-- tmp).index ();
        }
    };

  /** \brief 1st base class of all matrix reverse iterators. (non-MSVC version)
   *
   * \param I the derived iterator type
   *
   * The reverse iterator implements a bidirectional iterator
   * reversing the elements of the underlying iterator. It
   * implements most operators of a random access iterator.
   *
   * uBLAS extension: it.index1(), it.index2() and access to
   * the dual iterator via begin(), end(), rbegin(), rend()
   */

    // Renamed this class from reverse_iterator1 to get
    // typedef reverse_iterator1<...> reverse_iterator1
    // working. Thanks to Gabriel Dos Reis for explaining this.
    template <class I>
    class reverse_iterator_base1:
        public std::reverse_iterator<I> {
    public:
        typedef typename I::container_type container_type;
        typedef typename container_type::size_type size_type;
        typedef typename I::difference_type difference_type;
        typedef I iterator_type;
        typedef typename I::dual_iterator_type dual_iterator_type;
        typedef typename I::dual_reverse_iterator_type dual_reverse_iterator_type;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        reverse_iterator_base1 ():
            std::reverse_iterator<iterator_type> () {}
        BOOST_UBLAS_INLINE
        reverse_iterator_base1 (const iterator_type &it):
            std::reverse_iterator<iterator_type> (it) {}

        // Arithmetic
        BOOST_UBLAS_INLINE
        reverse_iterator_base1 &operator ++ () {
            return *this = -- this->base ();
        }
        BOOST_UBLAS_INLINE
        reverse_iterator_base1 operator ++ (int) {
            reverse_iterator_base1 tmp (*this);
            *this = -- this->base ();
            return tmp;
        }
        BOOST_UBLAS_INLINE
        reverse_iterator_base1 &operator -- () {
            return *this = ++ this->base ();
        }
        BOOST_UBLAS_INLINE
        reverse_iterator_base1 operator -- (int) {
            reverse_iterator_base1 tmp (*this);
            *this = ++ this->base ();
            return tmp;
        }
        BOOST_UBLAS_INLINE
        reverse_iterator_base1 &operator += (difference_type n) {
            return *this = this->base () - n;
        }
        BOOST_UBLAS_INLINE
        reverse_iterator_base1 &operator -= (difference_type n) {
            return *this = this->base () + n;
        }

        BOOST_UBLAS_INLINE
        friend reverse_iterator_base1 operator + (const reverse_iterator_base1 &it, difference_type n) {
            reverse_iterator_base1 tmp (it);
            return tmp += n;
        }
        BOOST_UBLAS_INLINE
        friend reverse_iterator_base1 operator + (difference_type n, const reverse_iterator_base1 &it) {
            reverse_iterator_base1 tmp (it);
            return tmp += n;
        }
        BOOST_UBLAS_INLINE
        friend reverse_iterator_base1 operator - (const reverse_iterator_base1 &it, difference_type n) {
            reverse_iterator_base1 tmp (it);
            return tmp -= n;
        }
        BOOST_UBLAS_INLINE
        friend difference_type operator - (const reverse_iterator_base1 &it1, const reverse_iterator_base1 &it2) {
            return it2.base () - it1.base ();
        }

        BOOST_UBLAS_INLINE
        const container_type &operator () () const {
            return this->base () ();
        }

        BOOST_UBLAS_INLINE
        size_type index1 () const {
            iterator_type tmp (this->base ());
            return (-- tmp).index1 ();
        }
        BOOST_UBLAS_INLINE
        size_type index2 () const {
            iterator_type tmp (this->base ());
            return (-- tmp).index2 ();
        }

        BOOST_UBLAS_INLINE
        dual_iterator_type begin () const {
            iterator_type tmp (this->base ());
            return (-- tmp).begin ();
        }
        BOOST_UBLAS_INLINE
        dual_iterator_type end () const {
            iterator_type tmp (this->base ());
            return (-- tmp).end ();
        }
        BOOST_UBLAS_INLINE
        dual_reverse_iterator_type rbegin () const {
            return dual_reverse_iterator_type (end ());
        }
        BOOST_UBLAS_INLINE
        dual_reverse_iterator_type rend () const {
            return dual_reverse_iterator_type (begin ());
        }
    };

  /** \brief 2nd base class of all matrix reverse iterators. (non-MSVC version)
   *
   * \param I the derived iterator type
   *
   * The reverse iterator implements a bidirectional iterator
   * reversing the elements of the underlying iterator. It
   * implements most operators of a random access iterator.
   *
   * uBLAS extension: it.index1(), it.index2() and access to
   * the dual iterator via begin(), end(), rbegin(), rend()
   *
   * Note: this type is _identical_ to reverse_iterator_base1
   */

    // Renamed this class from reverse_iterator2 to get
    // typedef reverse_iterator2<...> reverse_iterator2
    // working. Thanks to Gabriel Dos Reis for explaining this.
    template <class I>
    class reverse_iterator_base2:
        public std::reverse_iterator<I> {
    public:
        typedef typename I::container_type container_type;
        typedef typename container_type::size_type size_type;
        typedef typename I::difference_type difference_type;
        typedef I iterator_type;
        typedef typename I::dual_iterator_type dual_iterator_type;
        typedef typename I::dual_reverse_iterator_type dual_reverse_iterator_type;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        reverse_iterator_base2 ():
            std::reverse_iterator<iterator_type> () {}
        BOOST_UBLAS_INLINE
        reverse_iterator_base2 (const iterator_type &it):
            std::reverse_iterator<iterator_type> (it) {}

        // Arithmetic
        BOOST_UBLAS_INLINE
        reverse_iterator_base2 &operator ++ () {
            return *this = -- this->base ();
        }
        BOOST_UBLAS_INLINE
        reverse_iterator_base2 operator ++ (int) {
            reverse_iterator_base2 tmp (*this);
            *this = -- this->base ();
            return tmp;
        }
        BOOST_UBLAS_INLINE
        reverse_iterator_base2 &operator -- () {
            return *this = ++ this->base ();
        }
        BOOST_UBLAS_INLINE
        reverse_iterator_base2 operator -- (int) {
            reverse_iterator_base2 tmp (*this);
            *this = ++ this->base ();
            return tmp;
        }
        BOOST_UBLAS_INLINE
        reverse_iterator_base2 &operator += (difference_type n) {
            return *this = this->base () - n;
        }
        BOOST_UBLAS_INLINE
        reverse_iterator_base2 &operator -= (difference_type n) {
            return *this = this->base () + n;
        }

        BOOST_UBLAS_INLINE
        friend reverse_iterator_base2 operator + (const reverse_iterator_base2 &it, difference_type n) {
            reverse_iterator_base2 tmp (it);
            return tmp += n;
        }
        BOOST_UBLAS_INLINE
        friend reverse_iterator_base2 operator + (difference_type n, const reverse_iterator_base2 &it) {
            reverse_iterator_base2 tmp (it);
            return tmp += n;
        }
        BOOST_UBLAS_INLINE
        friend reverse_iterator_base2 operator - (const reverse_iterator_base2 &it, difference_type n) {
            reverse_iterator_base2 tmp (it);
            return tmp -= n;
        }
        BOOST_UBLAS_INLINE
        friend difference_type operator - (const reverse_iterator_base2 &it1, const reverse_iterator_base2 &it2) {
            return it2.base () - it1.base ();
        }

        BOOST_UBLAS_INLINE
        const container_type &operator () () const {
            return this->base () ();
        }

        BOOST_UBLAS_INLINE
        size_type index1 () const {
            iterator_type tmp (this->base ());
            return (-- tmp).index1 ();
        }
        BOOST_UBLAS_INLINE
        size_type index2 () const {
            iterator_type tmp (this->base ());
            return (-- tmp).index2 ();
        }

        BOOST_UBLAS_INLINE
        dual_iterator_type begin () const {
            iterator_type tmp (this->base ());
            return (-- tmp).begin ();
        }
        BOOST_UBLAS_INLINE
        dual_iterator_type end () const {
            iterator_type tmp (this->base ());
            return (-- tmp).end ();
        }
        BOOST_UBLAS_INLINE
        dual_reverse_iterator_type rbegin () const {
            return dual_reverse_iterator_type (end ());
        }
        BOOST_UBLAS_INLINE
        dual_reverse_iterator_type rend () const {
            return dual_reverse_iterator_type (begin ());
        }
    };

  /** \brief A class implementing an indexed random access iterator.
   *
   * \param C the (mutable) container type
   * \param IC the iterator category
   *
   * This class implements a random access iterator. The current 
   * position is stored as the unsigned integer it_ and the
   * values are accessed via operator()(it_) of the container.
   *
   * uBLAS extension: index()
   */

    template<class C, class IC>
    class indexed_iterator:
        public container_reference<C>,
        public random_access_iterator_base<IC,
                                           indexed_iterator<C, IC>,
                                           typename C::value_type,
                                           typename C::difference_type> {
    public:
        typedef C container_type;
        typedef IC iterator_category;
        typedef typename container_type::size_type size_type;
        typedef typename container_type::difference_type difference_type;
        typedef typename container_type::value_type value_type;
        typedef typename container_type::reference reference;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        indexed_iterator ():
            container_reference<container_type> (), it_ () {}
        BOOST_UBLAS_INLINE
        indexed_iterator (container_type &c, size_type it):
            container_reference<container_type> (c), it_ (it) {}

        // Arithmetic
        BOOST_UBLAS_INLINE
        indexed_iterator &operator ++ () {
            ++ it_;
            return *this;
        }
        BOOST_UBLAS_INLINE
        indexed_iterator &operator -- () {
            -- it_;
            return *this;
        }
        BOOST_UBLAS_INLINE
        indexed_iterator &operator += (difference_type n) {
            it_ += n;
            return *this;
        }
        BOOST_UBLAS_INLINE
        indexed_iterator &operator -= (difference_type n) {
            it_ -= n;
            return *this;
        }
        BOOST_UBLAS_INLINE
        difference_type operator - (const indexed_iterator &it) const {
            BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
            return it_ - it.it_;
        }

        // Dereference
        BOOST_UBLAS_INLINE
        reference operator * () const {
            BOOST_UBLAS_CHECK (index () < (*this) ().size (), bad_index ());
            return (*this) () (it_);
        }
        BOOST_UBLAS_INLINE
        reference operator [] (difference_type n) const {
            return *((*this) + n);
        }

        // Index
        BOOST_UBLAS_INLINE
        size_type index () const {
            return it_;
        }

        // Assignment
        BOOST_UBLAS_INLINE
        indexed_iterator &operator = (const indexed_iterator &it) {
            // FIX: ICC needs full qualification?!
            // assign (&it ());
            container_reference<C>::assign (&it ());
            it_ = it.it_;
            return *this;
        }

        // Comparison
        BOOST_UBLAS_INLINE
        bool operator == (const indexed_iterator &it) const {
            BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
            return it_ == it.it_;
        }
        BOOST_UBLAS_INLINE
        bool operator < (const indexed_iterator &it) const {
            BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
            return it_ < it.it_;
        }

    private:
        size_type it_;
    };

  /** \brief A class implementing an indexed random access iterator.
   *
   * \param C the (immutable) container type
   * \param IC the iterator category
   *
   * This class implements a random access iterator. The current 
   * position is stored as the unsigned integer \c it_ and the
   * values are accessed via \c operator()(it_) of the container.
   *
   * uBLAS extension: \c index()
   *
   * Note: there is an automatic conversion from 
   * \c indexed_iterator to \c indexed_const_iterator
   */

    template<class C, class IC>
    class indexed_const_iterator:
        public container_const_reference<C>,
        public random_access_iterator_base<IC,
                                           indexed_const_iterator<C, IC>,
                                           typename C::value_type,
                                           typename C::difference_type> {
    public:
        typedef C container_type;
        typedef IC iterator_category;
        typedef typename container_type::size_type size_type;
        typedef typename container_type::difference_type difference_type;
        typedef typename container_type::value_type value_type;
        typedef typename container_type::const_reference reference;
        typedef indexed_iterator<container_type, iterator_category> iterator_type;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        indexed_const_iterator ():
            container_const_reference<container_type> (), it_ () {}
        BOOST_UBLAS_INLINE
        indexed_const_iterator (const container_type &c, size_type it):
            container_const_reference<container_type> (c), it_ (it) {}
        BOOST_UBLAS_INLINE 
        indexed_const_iterator (const iterator_type &it):
            container_const_reference<container_type> (it ()), it_ (it.index ()) {}

        // Arithmetic
        BOOST_UBLAS_INLINE
        indexed_const_iterator &operator ++ () {
            ++ it_;
            return *this;
        }
        BOOST_UBLAS_INLINE
        indexed_const_iterator &operator -- () {
            -- it_;
            return *this;
        }
        BOOST_UBLAS_INLINE
        indexed_const_iterator &operator += (difference_type n) {
            it_ += n;
            return *this;
        }
        BOOST_UBLAS_INLINE
        indexed_const_iterator &operator -= (difference_type n) {
            it_ -= n;
            return *this;
        }
        BOOST_UBLAS_INLINE
        difference_type operator - (const indexed_const_iterator &it) const {
            BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
            return it_ - it.it_;
        }

        // Dereference
        BOOST_UBLAS_INLINE
        reference operator * () const {
            BOOST_UBLAS_CHECK (index () < (*this) ().size (), bad_index ());
            return (*this) () (it_);
        }
        BOOST_UBLAS_INLINE
        reference operator [] (difference_type n) const {
            return *((*this) + n);
        }

        // Index
        BOOST_UBLAS_INLINE
        size_type index () const {
            return it_;
        }

        // Assignment
        BOOST_UBLAS_INLINE
        indexed_const_iterator &operator = (const indexed_const_iterator &it) {
            // FIX: ICC needs full qualification?!
            // assign (&it ());
            container_const_reference<C>::assign (&it ());
            it_ = it.it_;
            return *this;
        }

        // Comparison
        BOOST_UBLAS_INLINE
        bool operator == (const indexed_const_iterator &it) const {
            BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
            return it_ == it.it_;
        }
        BOOST_UBLAS_INLINE
        bool operator < (const indexed_const_iterator &it) const {
            BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
            return it_ < it.it_;
        }

    private:
        size_type it_;

        friend class indexed_iterator<container_type, iterator_category>;
    };

    template<class C, class IC>
    class indexed_iterator2;

  /** \brief A class implementing an indexed random access iterator 
   * of a matrix.
   *
   * \param C the (mutable) container type
   * \param IC the iterator category
   *
   * This class implements a random access iterator. The current
   * position is stored as two unsigned integers \c it1_ and \c it2_
   * and the values are accessed via \c operator()(it1_, it2_) of the
   * container. The iterator changes the first index.
   *
   * uBLAS extension: \c index1(), \c index2() and access to the
   * dual iterator via \c begin(), \c end(), \c rbegin() and \c rend()
   *
   * Note: The container has to support the \code find2(rank, i, j) \endcode 
   * method
   */

    template<class C, class IC>
    class indexed_iterator1:
        public container_reference<C>, 
        public random_access_iterator_base<IC,
                                           indexed_iterator1<C, IC>, 
                                           typename C::value_type,
                                           typename C::difference_type> {
    public:
        typedef C container_type;
        typedef IC iterator_category;
        typedef typename container_type::size_type size_type;
        typedef typename container_type::difference_type difference_type;
        typedef typename container_type::value_type value_type;
        typedef typename container_type::reference reference;

        typedef indexed_iterator2<container_type, iterator_category> dual_iterator_type;
        typedef reverse_iterator_base2<dual_iterator_type> dual_reverse_iterator_type;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        indexed_iterator1 ():
            container_reference<container_type> (), it1_ (), it2_ () {}
        BOOST_UBLAS_INLINE 
        indexed_iterator1 (container_type &c, size_type it1, size_type it2):
            container_reference<container_type> (c), it1_ (it1), it2_ (it2) {}

        // Arithmetic
        BOOST_UBLAS_INLINE
        indexed_iterator1 &operator ++ () {
            ++ it1_;
            return *this;
        }
        BOOST_UBLAS_INLINE
        indexed_iterator1 &operator -- () {
            -- it1_;
            return *this;
        }
        BOOST_UBLAS_INLINE
        indexed_iterator1 &operator += (difference_type n) {
            it1_ += n;
            return *this;
        }
        BOOST_UBLAS_INLINE
        indexed_iterator1 &operator -= (difference_type n) {
            it1_ -= n;
            return *this;
        }
        BOOST_UBLAS_INLINE
        difference_type operator - (const indexed_iterator1 &it) const {
            BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
            BOOST_UBLAS_CHECK (it2_ == it.it2_, external_logic ());
            return it1_ - it.it1_;
        }

        // Dereference
        BOOST_UBLAS_INLINE
        reference operator * () const {
            BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
            BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
            return (*this) () (it1_, it2_);
        }
        BOOST_UBLAS_INLINE
        reference operator [] (difference_type n) const {
            return *((*this) + n);
        }

        // Index
        BOOST_UBLAS_INLINE
        size_type index1 () const {
            return it1_;
        }
        BOOST_UBLAS_INLINE
        size_type index2 () const {
            return it2_;
        }

        BOOST_UBLAS_INLINE
        dual_iterator_type begin () const {
            return (*this) ().find2 (1, index1 (), 0); 
        }
        BOOST_UBLAS_INLINE
        dual_iterator_type end () const {
            return (*this) ().find2 (1, index1 (), (*this) ().size2 ());
        }
        BOOST_UBLAS_INLINE
        dual_reverse_iterator_type rbegin () const {
            return dual_reverse_iterator_type (end ());
        }
        BOOST_UBLAS_INLINE
        dual_reverse_iterator_type rend () const {
            return dual_reverse_iterator_type (begin ());
        }

        // Assignment
        BOOST_UBLAS_INLINE
        indexed_iterator1 &operator = (const indexed_iterator1 &it) {
            // FIX: ICC needs full qualification?!
            // assign (&it ());
            container_reference<C>::assign (&it ());
            it1_ = it.it1_;
            it2_ = it.it2_;
            return *this;
        }

        // Comparison
        BOOST_UBLAS_INLINE
        bool operator == (const indexed_iterator1 &it) const {
            BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
            BOOST_UBLAS_CHECK (it2_ == it.it2_, external_logic ());
            return it1_ == it.it1_;
        }
        BOOST_UBLAS_INLINE
        bool operator < (const indexed_iterator1 &it) const {
            BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
            BOOST_UBLAS_CHECK (it2_ == it.it2_, external_logic ());
            return it1_ < it.it1_;
        }

    private:
        size_type it1_;
        size_type it2_;
    };

    template<class C, class IC>
    class indexed_const_iterator2;

  /** \brief A class implementing an indexed random access iterator 
   * of a matrix.
   *
   * \param C the (immutable) container type
   * \param IC the iterator category
   *
   * This class implements a random access iterator. The current
   * position is stored as two unsigned integers \c it1_ and \c it2_
   * and the values are accessed via \c operator()(it1_, it2_) of the
   * container. The iterator changes the first index.
   *
   * uBLAS extension: \c index1(), \c index2() and access to the
   * dual iterator via \c begin(), \c end(), \c rbegin() and \c rend()
   *
   * Note 1: The container has to support the find2(rank, i, j) method
   *
   * Note 2: there is an automatic conversion from 
   * \c indexed_iterator1 to \c indexed_const_iterator1
   */

    template<class C, class IC>
    class indexed_const_iterator1:
        public container_const_reference<C>, 
        public random_access_iterator_base<IC,
                                           indexed_const_iterator1<C, IC>, 
                                           typename C::value_type,
                                           typename C::difference_type> {
    public:
        typedef C container_type;
        typedef IC iterator_category;
        typedef typename container_type::size_type size_type;
        typedef typename container_type::difference_type difference_type;
        typedef typename container_type::value_type value_type;
        typedef typename container_type::const_reference reference;

        typedef indexed_iterator1<container_type, iterator_category> iterator_type;
        typedef indexed_const_iterator2<container_type, iterator_category> dual_iterator_type;
        typedef reverse_iterator_base2<dual_iterator_type> dual_reverse_iterator_type;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        indexed_const_iterator1 ():
            container_const_reference<container_type> (), it1_ (), it2_ () {}
        BOOST_UBLAS_INLINE
        indexed_const_iterator1 (const container_type &c, size_type it1, size_type it2):
            container_const_reference<container_type> (c), it1_ (it1), it2_ (it2) {}
        BOOST_UBLAS_INLINE 
        indexed_const_iterator1 (const iterator_type &it):
            container_const_reference<container_type> (it ()), it1_ (it.index1 ()), it2_ (it.index2 ()) {}

        // Arithmetic
        BOOST_UBLAS_INLINE
        indexed_const_iterator1 &operator ++ () {
            ++ it1_;
            return *this;
        }
        BOOST_UBLAS_INLINE
        indexed_const_iterator1 &operator -- () {
            -- it1_;
            return *this;
        }
        BOOST_UBLAS_INLINE
        indexed_const_iterator1 &operator += (difference_type n) {
            it1_ += n;
            return *this;
        }
        BOOST_UBLAS_INLINE
        indexed_const_iterator1 &operator -= (difference_type n) {
            it1_ -= n;
            return *this;
        }
        BOOST_UBLAS_INLINE
        difference_type operator - (const indexed_const_iterator1 &it) const {
            BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
            BOOST_UBLAS_CHECK (it2_ == it.it2_, external_logic ());
            return it1_ - it.it1_;
        }

        // Dereference
        BOOST_UBLAS_INLINE
        reference operator * () const {
            BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
            BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
            return (*this) () (it1_, it2_);
        }
        BOOST_UBLAS_INLINE
        reference operator [] (difference_type n) const {
            return *((*this) + n);
        }

        // Index
        BOOST_UBLAS_INLINE
        size_type index1 () const {
            return it1_;
        }
        BOOST_UBLAS_INLINE
        size_type index2 () const {
            return it2_;
        }

        BOOST_UBLAS_INLINE
        dual_iterator_type begin () const {
            return (*this) ().find2 (1, index1 (), 0); 
        }
        BOOST_UBLAS_INLINE
        dual_iterator_type end () const {
            return (*this) ().find2 (1, index1 (), (*this) ().size2 ()); 
        }
        BOOST_UBLAS_INLINE
        dual_reverse_iterator_type rbegin () const {
            return dual_reverse_iterator_type (end ()); 
        }
        BOOST_UBLAS_INLINE
        dual_reverse_iterator_type rend () const {
            return dual_reverse_iterator_type (begin ()); 
        }

        // Assignment
        BOOST_UBLAS_INLINE
        indexed_const_iterator1 &operator = (const indexed_const_iterator1 &it) {
            // FIX: ICC needs full qualification?!
            // assign (&it ());
            container_const_reference<C>::assign (&it ());
            it1_ = it.it1_;
            it2_ = it.it2_;
            return *this;
        }

        // Comparison
        BOOST_UBLAS_INLINE
        bool operator == (const indexed_const_iterator1 &it) const {
            BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
            BOOST_UBLAS_CHECK (it2_ == it.it2_, external_logic ());
            return it1_ == it.it1_;
        }
        BOOST_UBLAS_INLINE
        bool operator < (const indexed_const_iterator1 &it) const {
            BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
            BOOST_UBLAS_CHECK (it2_ == it.it2_, external_logic ());
            return it1_ < it.it1_;
        }

    private:
        size_type it1_;
        size_type it2_;

        friend class indexed_iterator1<container_type, iterator_category>;
    };

  /** \brief A class implementing an indexed random access iterator 
   * of a matrix.
   *
   * \param C the (mutable) container type
   * \param IC the iterator category
   *
   * This class implements a random access iterator. The current
   * position is stored as two unsigned integers \c it1_ and \c it2_
   * and the values are accessed via \c operator()(it1_, it2_) of the
   * container. The iterator changes the second index.
   *
   * uBLAS extension: \c index1(), \c index2() and access to the
   * dual iterator via \c begin(), \c end(), \c rbegin() and \c rend()
   *
   * Note: The container has to support the find1(rank, i, j) method
   */
    template<class C, class IC>
    class indexed_iterator2:
        public container_reference<C>, 
        public random_access_iterator_base<IC,
                                           indexed_iterator2<C, IC>, 
                                           typename C::value_type,
                                           typename C::difference_type> {
    public:
        typedef C container_type;
        typedef IC iterator_category;
        typedef typename container_type::size_type size_type;
        typedef typename container_type::difference_type difference_type;
        typedef typename container_type::value_type value_type;
        typedef typename container_type::reference reference;

        typedef indexed_iterator1<container_type, iterator_category> dual_iterator_type;
        typedef reverse_iterator_base1<dual_iterator_type> dual_reverse_iterator_type;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        indexed_iterator2 ():
            container_reference<container_type> (), it1_ (), it2_ () {}
        BOOST_UBLAS_INLINE
        indexed_iterator2 (container_type &c, size_type it1, size_type it2):
            container_reference<container_type> (c), it1_ (it1), it2_ (it2) {}

        // Arithmetic
        BOOST_UBLAS_INLINE
        indexed_iterator2 &operator ++ () {
            ++ it2_;
            return *this;
        }
        BOOST_UBLAS_INLINE
        indexed_iterator2 &operator -- () {
            -- it2_;
            return *this;
        }
        BOOST_UBLAS_INLINE
        indexed_iterator2 &operator += (difference_type n) {
            it2_ += n;
            return *this;
        }
        BOOST_UBLAS_INLINE
        indexed_iterator2 &operator -= (difference_type n) {
            it2_ -= n;
            return *this;
        }
        BOOST_UBLAS_INLINE
        difference_type operator - (const indexed_iterator2 &it) const {
            BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
            BOOST_UBLAS_CHECK (it1_ == it.it1_, external_logic ());
            return it2_ - it.it2_;
        }

        // Dereference
        BOOST_UBLAS_INLINE
        reference operator * () const {
            BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
            BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
            return (*this) () (it1_, it2_);
        }
        BOOST_UBLAS_INLINE
        reference operator [] (difference_type n) const {
            return *((*this) + n);
        }

        // Index
        BOOST_UBLAS_INLINE
        size_type index1 () const {
            return it1_;
        }
        BOOST_UBLAS_INLINE
        size_type index2 () const {
            return it2_;
        }

        BOOST_UBLAS_INLINE
        dual_iterator_type begin () const {
            return (*this) ().find1 (1, 0, index2 ());
        }
        BOOST_UBLAS_INLINE
        dual_iterator_type end () const {
            return (*this) ().find1 (1, (*this) ().size1 (), index2 ());
        }
        BOOST_UBLAS_INLINE
        dual_reverse_iterator_type rbegin () const {
            return dual_reverse_iterator_type (end ());
        }
        BOOST_UBLAS_INLINE
        dual_reverse_iterator_type rend () const {
            return dual_reverse_iterator_type (begin ());
        }

        // Assignment
        BOOST_UBLAS_INLINE
        indexed_iterator2 &operator = (const indexed_iterator2 &it) {
            // FIX: ICC needs full qualification?!
            // assign (&it ());
            container_reference<C>::assign (&it ());
            it1_ = it.it1_;
            it2_ = it.it2_;
            return *this;
        }

        // Comparison
        BOOST_UBLAS_INLINE
        bool operator == (const indexed_iterator2 &it) const {
            BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
            BOOST_UBLAS_CHECK (it1_ == it.it1_, external_logic ());
            return it2_ == it.it2_;
        }
        BOOST_UBLAS_INLINE
        bool operator < (const indexed_iterator2 &it) const {
            BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
            BOOST_UBLAS_CHECK (it1_ == it.it1_, external_logic ());
            return it2_ < it.it2_;
        }

    private:
        size_type it1_;
        size_type it2_;
    };

  /** \brief A class implementing an indexed random access iterator 
   * of a matrix.
   *
   * \param C the (immutable) container type
   * \param IC the iterator category
   *
   * This class implements a random access iterator. The current
   * position is stored as two unsigned integers \c it1_ and \c it2_
   * and the values are accessed via \c operator()(it1_, it2_) of the
   * container. The iterator changes the second index.
   *
   * uBLAS extension: \c index1(), \c index2() and access to the
   * dual iterator via \c begin(), \c end(), \c rbegin() and \c rend()
   *
   * Note 1: The container has to support the \c find2(rank, i, j) method
   *
   * Note 2: there is an automatic conversion from 
   * \c indexed_iterator2 to \c indexed_const_iterator2
   */

    template<class C, class IC>
    class indexed_const_iterator2:
        public container_const_reference<C>,
        public random_access_iterator_base<IC,
                                           indexed_const_iterator2<C, IC>,
                                           typename C::value_type,
                                           typename C::difference_type> {
    public:
        typedef C container_type;
        typedef IC iterator_category;
        typedef typename container_type::size_type size_type;
        typedef typename container_type::difference_type difference_type;
        typedef typename container_type::value_type value_type;
        typedef typename container_type::const_reference reference;

        typedef indexed_iterator2<container_type, iterator_category> iterator_type;
        typedef indexed_const_iterator1<container_type, iterator_category> dual_iterator_type;
        typedef reverse_iterator_base1<dual_iterator_type> dual_reverse_iterator_type;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        indexed_const_iterator2 ():
            container_const_reference<container_type> (), it1_ (), it2_ () {}
        BOOST_UBLAS_INLINE
        indexed_const_iterator2 (const container_type &c, size_type it1, size_type it2):
            container_const_reference<container_type> (c), it1_ (it1), it2_ (it2) {}
        BOOST_UBLAS_INLINE
        indexed_const_iterator2 (const iterator_type &it):
            container_const_reference<container_type> (it ()), it1_ (it.index1 ()), it2_ (it.index2 ()) {}

        // Arithmetic
        BOOST_UBLAS_INLINE
        indexed_const_iterator2 &operator ++ () {
            ++ it2_;
            return *this;
        }
        BOOST_UBLAS_INLINE
        indexed_const_iterator2 &operator -- () {
            -- it2_;
            return *this;
        }
        BOOST_UBLAS_INLINE
        indexed_const_iterator2 &operator += (difference_type n) {
            it2_ += n;
            return *this;
        }
        BOOST_UBLAS_INLINE
        indexed_const_iterator2 &operator -= (difference_type n) {
            it2_ -= n;
            return *this;
        }
        BOOST_UBLAS_INLINE
        difference_type operator - (const indexed_const_iterator2 &it) const {
            BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
            BOOST_UBLAS_CHECK (it1_ == it.it1_, external_logic ());
            return it2_ - it.it2_;
        }

        // Dereference
        BOOST_UBLAS_INLINE
        reference operator * () const {
            BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
            BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
            return (*this) () (it1_, it2_);
        }
        BOOST_UBLAS_INLINE
        reference operator [] (difference_type n) const {
            return *((*this) + n);
        }

        // Index
        BOOST_UBLAS_INLINE
        size_type index1 () const {
            return it1_;
        }
        BOOST_UBLAS_INLINE
        size_type index2 () const {
            return it2_;
        }

        BOOST_UBLAS_INLINE
        dual_iterator_type begin () const {
            return (*this) ().find1 (1, 0, index2 ());
        }
        BOOST_UBLAS_INLINE
        dual_iterator_type end () const {
            return (*this) ().find1 (1, (*this) ().size1 (), index2 ());
        }
        BOOST_UBLAS_INLINE
        dual_reverse_iterator_type rbegin () const {
            return dual_reverse_iterator_type (end ());
        }
        BOOST_UBLAS_INLINE
        dual_reverse_iterator_type rend () const {
            return dual_reverse_iterator_type (begin ());
        }

        // Assignment
        BOOST_UBLAS_INLINE
        indexed_const_iterator2 &operator = (const indexed_const_iterator2 &it) {
            // FIX: ICC needs full qualification?!
            // assign (&it ());
            container_const_reference<C>::assign (&it ());
            it1_ = it.it1_;
            it2_ = it.it2_;
            return *this;
        }

        // Comparison
        BOOST_UBLAS_INLINE
        bool operator == (const indexed_const_iterator2 &it) const {
            BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
            BOOST_UBLAS_CHECK (it1_ == it.it1_, external_logic ());
            return it2_ == it.it2_;
        }
        BOOST_UBLAS_INLINE
        bool operator < (const indexed_const_iterator2 &it) const {
            BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
            BOOST_UBLAS_CHECK (it1_ == it.it1_, external_logic ());
            return it2_ < it.it2_;
        }

    private:
        size_type it1_;
        size_type it2_;

        friend class indexed_iterator2<container_type, iterator_category>;
    };

}}}

#endif

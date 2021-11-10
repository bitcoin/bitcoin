/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c)      2011 Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_UTREE_DETAIL2)
#define BOOST_SPIRIT_UTREE_DETAIL2

#include <boost/type_traits/remove_pointer.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/throw_exception.hpp>
#include <boost/iterator/iterator_traits.hpp>
#include <cstring> // for std::memcpy

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4800) // forcing value to bool 'true' or 'false'
# if _MSC_VER < 1800
#  pragma warning(disable: 4702) // unreachable code
# endif
#endif

namespace boost { namespace spirit { namespace detail
{
    inline char& fast_string::info()
    {
        return buff[small_string_size];
    }

    inline char fast_string::info() const
    {
        return buff[small_string_size];
    }

    inline int fast_string::get_type() const
    {
        return info() >> 1;
    }

    inline void fast_string::set_type(int t)
    {
        info() = (t << 1) | (info() & 1);
    }

    inline short fast_string::tag() const
    {
        boost::int16_t tmp;
        std::memcpy(&tmp, &buff[small_string_size-2], sizeof(tmp));
        return tmp;
    }

    inline void fast_string::tag(short tag)
    {
        boost::int16_t tmp = tag;
        std::memcpy(&buff[small_string_size-2], &tmp, sizeof(tmp));
    }

    inline bool fast_string::is_heap_allocated() const
    {
        return info() & 1;
    }

    inline std::size_t fast_string::size() const
    {
        if (is_heap_allocated())
            return heap.size;
        else
            return max_string_len - buff[max_string_len];
    }

    inline char const* fast_string::str() const
    {
        if (is_heap_allocated())
            return heap.str;
        else
            return buff;
    }

    template <typename Iterator>
    inline void fast_string::construct(Iterator f, Iterator l)
    {
        std::size_t const size = static_cast<std::size_t>(l-f);
        char* str;
        if (size < max_string_len)
        {
            // if it fits, store it in-situ; small_string_size minus the length
            // of the string is placed in buff[small_string_size - 1]
            str = buff;
            buff[max_string_len] = static_cast<char>(max_string_len - size);
            info() &= ~0x1;
        }
        else
        {
            // else, store it in the heap
            str = new char[size + 1]; // add one for the null char
            heap.str = str;
            heap.size = size;
            info() |= 0x1;
        }
        for (std::size_t i = 0; i != size; ++i)
        {
            *str++ = *f++;
        }
        *str = '\0'; // add the null char
    }

    inline void fast_string::swap(fast_string& other)
    {
        std::swap(*this, other);
    }

    inline void fast_string::free()
    {
        if (is_heap_allocated())
        {
            delete [] heap.str;
        }
    }

    inline void fast_string::copy(fast_string const& other)
    {
        construct(other.str(), other.str() + other.size());
    }

    inline void fast_string::initialize()
    {
        for (std::size_t i = 0; i != buff_size / (sizeof(long)/sizeof(char)); ++i)
            lbuff[i] = 0;
    }

    struct list::node : boost::noncopyable
    {
        template <typename T>
        node(T const& val, node* next, node* prev)
          : val(val), next(next), prev(prev) {}

        void unlink()
        {
            prev->next = next;
            next->prev = prev;
        }

        utree val;
        node* next;
        node* prev;
    };

    template <typename Value>
    class list::node_iterator
      : public boost::iterator_facade<
            node_iterator<Value>
          , Value
          , boost::bidirectional_traversal_tag>
    {
    public:

        node_iterator()
          : node(0), prev(0) {}

        node_iterator(list::node* node, list::node* prev)
          : node(node), prev(prev) {}

    private:

        friend class boost::iterator_core_access;
        friend class boost::spirit::utree;
        friend struct boost::spirit::detail::list;

        void increment()
        {
            if (node != 0) // not at end
            {
                prev = node;
                node = node->next;
            }
        }

        void decrement()
        {
            if (prev != 0) // not at begin
            {
                node = prev;
                prev = prev->prev;
            }
        }

        bool equal(node_iterator const& other) const
        {
            return node == other.node;
        }

        typename node_iterator::reference dereference() const
        {
            return node->val;
        }

        list::node* node;
        list::node* prev;
    };

    template <typename Value>
    class list::node_iterator<boost::reference_wrapper<Value> >
      : public boost::iterator_facade<
            node_iterator<boost::reference_wrapper<Value> >
          , boost::reference_wrapper<Value>
          , boost::bidirectional_traversal_tag>
    {
    public:

        node_iterator()
          : node(0), prev(0), curr(nil_node) {}

        node_iterator(list::node* node, list::node* prev)
          : node(node), prev(prev), curr(node ? node->val : nil_node) {}

    private:

        friend class boost::iterator_core_access;
        friend class boost::spirit::utree;
        friend struct boost::spirit::detail::list;

        void increment()
        {
            if (node != 0) // not at end
            {
                prev = node;
                node = node->next;
                curr = boost::ref(node ? node->val : nil_node);
            }
        }

        void decrement()
        {
            if (prev != 0) // not at begin
            {
                node = prev;
                prev = prev->prev;
                curr = boost::ref(node ? node->val : nil_node);
            }
        }

        bool equal(node_iterator const& other) const
        {
            return node == other.node;
        }

        typename node_iterator::reference dereference() const
        {
            return curr;
        }

        list::node* node;
        list::node* prev;

        static Value nil_node;
        mutable boost::reference_wrapper<Value> curr;
    };

    template <typename Value>
    Value list::node_iterator<boost::reference_wrapper<Value> >::nil_node = Value();

    inline void list::free()
    {
        node* p = first;
        while (p != 0)
        {
            node* next = p->next;
            delete p;
            p = next;
        }
    }

    inline void list::copy(list const& other)
    {
        node* p = other.first;
        while (p != 0)
        {
            push_back(p->val);
            p = p->next;
        }
    }

    inline void list::default_construct()
    {
        first = last = 0;
        size = 0;
    }

    template <typename T, typename Iterator>
    inline void list::insert(T const& val, Iterator pos)
    {
        if (!pos.node)
        {
            push_back(val);
            return;
        }

        detail::list::node* new_node =
            new detail::list::node(val, pos.node, pos.node->prev);

        if (pos.node->prev)
            pos.node->prev->next = new_node;
        else
            first = new_node;

        pos.node->prev = new_node;
        ++size;
    }

    template <typename T>
    inline void list::push_front(T const& val)
    {
        detail::list::node* new_node;
        if (first == 0)
        {
            new_node = new detail::list::node(val, 0, 0);
            first = last = new_node;
            ++size;
        }
        else
        {
            new_node = new detail::list::node(val, first, first->prev);
            first->prev = new_node;
            first = new_node;
            ++size;
        }
    }

    template <typename T>
    inline void list::push_back(T const& val)
    {
        if (last == 0)
            push_front(val);
        else {
            detail::list::node* new_node =
                new detail::list::node(val, last->next, last);
            last->next = new_node;
            last = new_node;
            ++size;
        }
    }

    inline void list::pop_front()
    {
        BOOST_ASSERT(size != 0);
        if (first == last) // there's only one item
        {
            delete first;
            size = 0;
            first = last = 0;
        }
        else
        {
            node* np = first;
            first = first->next;
            first->prev = 0;
            delete np;
            --size;
        }
    }

    inline void list::pop_back()
    {
        BOOST_ASSERT(size != 0);
        if (first == last) // there's only one item
        {
            delete first;
            size = 0;
            first = last = 0;
        }
        else
        {
            node* np = last;
            last = last->prev;
            last->next = 0;
            delete np;
            --size;
        }
    }

    inline list::node* list::erase(node* pos)
    {
        BOOST_ASSERT(pos != 0);
        if (pos == first)
        {
            pop_front();
            return first;
        }
        else if (pos == last)
        {
            pop_back();
            return 0;
        }
        else
        {
            node* next(pos->next);
            pos->unlink();
            delete pos;
            --size;
            return next;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // simple binder for binary visitation (we don't want to bring in the big guns)
    template <typename F, typename X>
    struct bind_impl
    {
        typedef typename F::result_type result_type;
        X& x; // always by reference
        F f;
        bind_impl(F f, X& x) : x(x), f(f) {}

        template <typename Y>
        typename F::result_type operator()(Y& y) const
        {
            return f(x, y);
        }

        template <typename Y>
        typename F::result_type operator()(Y const& y) const
        {
            return f(x, y);
        }
    };

    template <typename F, typename X>
    bind_impl<F, X const> bind(F f, X const& x)
    {
        return bind_impl<F, X const>(f, x);
    }

    template <typename F, typename X>
    bind_impl<F, X> bind(F f, X& x)
    {
        return bind_impl<F, X>(f, x);
    }

    template <typename UTreeX, typename UTreeY = UTreeX>
    struct visit_impl
    {
        template <typename F>
        typename F::result_type
        static apply(UTreeX& x, F f) // single dispatch
        {
            typedef typename
                boost::mpl::if_<boost::is_const<UTreeX>,
                typename UTreeX::const_iterator,
                typename UTreeX::iterator>::type
            iterator;

            typedef boost::iterator_range<iterator> list_range;
            typedef utree_type type;

            switch (x.get_type())
            {
                default:
                    BOOST_THROW_EXCEPTION(
                        bad_type_exception("corrupt utree type", x.get_type()));
                    break;

                case type::invalid_type:
                    return f(invalid);

                case type::nil_type:
                    return f(nil);

                case type::bool_type:
                    return f(x.b);

                case type::int_type:
                    return f(x.i);

                case type::double_type:
                    return f(x.d);

                case type::list_type:
                    return f(list_range(iterator(x.l.first, 0), iterator(0, x.l.last)));

                case type::range_type:
                    return f(list_range(iterator(x.r.first, 0), iterator(0, x.r.last)));

                case type::string_type:
                    return f(utf8_string_range_type(x.s.str(), x.s.size()));

                case type::string_range_type:
                    return f(utf8_string_range_type(x.sr.first, x.sr.last));

                case type::symbol_type:
                    return f(utf8_symbol_range_type(x.s.str(), x.s.size()));

                case type::binary_type:
                    return f(binary_range_type(x.s.str(), x.s.size()));

                case type::reference_type:
                    return apply(*x.p, f);

                case type::any_type:
                    return f(any_ptr(x.v.p, x.v.i));

                case type::function_type:
                    return f(*x.pf);
            }
        }

        template <typename F>
        typename F::result_type
        static apply(UTreeX& x, UTreeY& y, F f) // double dispatch
        {
            typedef typename
                boost::mpl::if_<boost::is_const<UTreeX>,
                typename UTreeX::const_iterator,
                typename UTreeX::iterator>::type
            iterator;

            typedef boost::iterator_range<iterator> list_range;
            typedef utree_type type;

            switch (x.get_type())
            {
                default:
                    BOOST_THROW_EXCEPTION(
                        bad_type_exception("corrupt utree type", x.get_type()));
                    break;

                case type::invalid_type:
                    return visit_impl::apply(y, detail::bind(f, invalid));

                case type::nil_type:
                    return visit_impl::apply(y, detail::bind(f, nil));

                case type::bool_type:
                    return visit_impl::apply(y, detail::bind(f, x.b));

                case type::int_type:
                    return visit_impl::apply(y, detail::bind(f, x.i));

                case type::double_type:
                    return visit_impl::apply(y, detail::bind(f, x.d));

                case type::list_type:
                    return visit_impl::apply(
                        y, detail::bind<F, list_range>(f,
                        list_range(iterator(x.l.first, 0), iterator(0, x.l.last))));

                case type::range_type:
                    return visit_impl::apply(
                        y, detail::bind<F, list_range>(f,
                        list_range(iterator(x.r.first, 0), iterator(0, x.r.last))));

                case type::string_type:
                    return visit_impl::apply(y, detail::bind(
                        f, utf8_string_range_type(x.s.str(), x.s.size())));

                case type::string_range_type:
                    return visit_impl::apply(y, detail::bind(
                        f, utf8_string_range_type(x.sr.first, x.sr.last)));

                case type::symbol_type:
                    return visit_impl::apply(y, detail::bind(
                        f, utf8_symbol_range_type(x.s.str(), x.s.size())));

                case type::binary_type:
                    return visit_impl::apply(y, detail::bind(
                        f, binary_range_type(x.s.str(), x.s.size())));

                case type::reference_type:
                    return apply(*x.p, y, f);

                case type::any_type:
                    return visit_impl::apply(
                        y, detail::bind(f, any_ptr(x.v.p, x.v.i)));

                case type::function_type:
                    return visit_impl::apply(y, detail::bind(f, *x.pf));
            }
        }
    };

    struct index_impl
    {
        static utree& apply(utree& ut, std::size_t i)
        {
            switch (ut.get_type())
            {
                case utree_type::reference_type:
                    return apply(ut.deref(), i);
                case utree_type::range_type:
                    return apply(ut.r.first, i);
                case utree_type::list_type:
                    return apply(ut.l.first, i);
                default:
                    BOOST_THROW_EXCEPTION(
                        bad_type_exception
                            ("index operation performed on non-list utree type",
                             ut.get_type()));
            }
        }

        static utree const& apply(utree const& ut, std::size_t i)
        {
            switch (ut.get_type())
            {
                case utree_type::reference_type:
                    return apply(ut.deref(), i);
                case utree_type::range_type:
                    return apply(ut.r.first, i);
                case utree_type::list_type:
                    return apply(ut.l.first, i);
                default:
                    BOOST_THROW_EXCEPTION(
                        bad_type_exception
                            ("index operation performed on non-list utree type",
                             ut.get_type()));
            }
        }

        static utree& apply(list::node* node, std::size_t i)
        {
            for (; i > 0; --i)
                node = node->next;
            return node->val;
        }

        static utree const& apply(list::node const* node, std::size_t i)
        {
            for (; i > 0; --i)
                node = node->next;
            return node->val;
        }
    };
}}}

namespace boost { namespace spirit
{
    template <typename F>
    stored_function<F>::stored_function(F f)
      : f(f)
    {
    }

    template <typename F>
    stored_function<F>::~stored_function()
    {
    }

    template <typename F>
    utree stored_function<F>::operator()(utree const& env) const
    {
        return f(env);
    }

    template <typename F>
    utree stored_function<F>::operator()(utree& env) const
    {
        return f(env);
    }

    template <typename F>
    function_base*
    stored_function<F>::clone() const
    {
        return new stored_function<F>(f);
    }

    template <typename F>
    referenced_function<F>::referenced_function(F& f)
      : f(f)
    {
    }

    template <typename F>
    referenced_function<F>::~referenced_function()
    {
    }

    template <typename F>
    utree referenced_function<F>::operator()(utree const& env) const
    {
        return f(env);
    }

    template <typename F>
    utree referenced_function<F>::operator()(utree& env) const
    {
        return f(env);
    }

    template <typename F>
    function_base*
    referenced_function<F>::clone() const
    {
        return new referenced_function<F>(f);
    }

    inline utree::utree(utree::invalid_type)
    {
        s.initialize();
        set_type(type::invalid_type);
    }

    inline utree::utree(utree::nil_type)
    {
        s.initialize();
        set_type(type::nil_type);
    }

    inline utree::utree(bool b_)
    {
        s.initialize();
        b = b_;
        set_type(type::bool_type);
    }

    inline utree::utree(char c)
    {
        s.initialize();
        // char constructs a single element string
        s.construct(&c, &c+1);
        set_type(type::string_type);
    }

    inline utree::utree(unsigned int i_)
    {
        s.initialize();
        i = i_;
        set_type(type::int_type);
    }

    inline utree::utree(int i_)
    {
        s.initialize();
        i = i_;
        set_type(type::int_type);
    }

    inline utree::utree(double d_)
    {
        s.initialize();
        d = d_;
        set_type(type::double_type);
    }

    inline utree::utree(char const* str)
    {
        s.initialize();
        s.construct(str, str + strlen(str));
        set_type(type::string_type);
    }

    inline utree::utree(char const* str, std::size_t len)
    {
        s.initialize();
        s.construct(str, str + len);
        set_type(type::string_type);
    }

    inline utree::utree(std::string const& str)
    {
        s.initialize();
        s.construct(str.begin(), str.end());
        set_type(type::string_type);
    }

    template <typename Base, utree_type::info type_>
    inline utree::utree(basic_string<Base, type_> const& bin)
    {
        s.initialize();
        s.construct(bin.begin(), bin.end());
        set_type(type_);
    }

    inline utree::utree(boost::reference_wrapper<utree> ref)
    {
        s.initialize();
        p = ref.get_pointer();
        set_type(type::reference_type);
    }

    inline utree::utree(any_ptr const& p)
    {
        s.initialize();
        v.p = p.p;
        v.i = p.i;
        set_type(type::any_type);
    }

    inline utree::utree(function_base const& pf_)
    {
        s.initialize();
        pf = pf_.clone();
        set_type(type::function_type);
    }

    inline utree::utree(function_base* pf_)
    {
        s.initialize();
        pf = pf_;
        set_type(type::function_type);
    }

    template <typename Iter>
    inline utree::utree(boost::iterator_range<Iter> r)
    {
        s.initialize();

        assign(r.begin(), r.end());
    }

    inline utree::utree(range r, shallow_tag)
    {
        s.initialize();
        this->r.first = r.begin().node;
        this->r.last = r.end().prev;
        set_type(type::range_type);
    }

    inline utree::utree(const_range r, shallow_tag)
    {
        s.initialize();
        this->r.first = r.begin().node;
        this->r.last = r.end().prev;
        set_type(type::range_type);
    }

    inline utree::utree(utf8_string_range_type const& str, shallow_tag)
    {
        s.initialize();
        this->sr.first = str.begin();
        this->sr.last = str.end();
        set_type(type::string_range_type);
    }

    inline utree::utree(utree const& other)
    {
        s.initialize();
        copy(other);
    }

    inline utree::~utree()
    {
        free();
    }

    inline utree& utree::operator=(utree const& other)
    {
        if (this != &other)
        {
            free();
            copy(other);
        }
        return *this;
    }

    inline utree& utree::operator=(nil_type)
    {
        free();
        set_type(type::nil_type);
        return *this;
    }

    inline utree& utree::operator=(bool b_)
    {
        free();
        b = b_;
        set_type(type::bool_type);
        return *this;
    }

    inline utree& utree::operator=(char c)
    {
        // char constructs a single element string
        free();
        s.construct(&c, &c+1);
        set_type(type::string_type);
        return *this;
    }

    inline utree& utree::operator=(unsigned int i_)
    {
        free();
        i = i_;
        set_type(type::int_type);
        return *this;
    }

    inline utree& utree::operator=(int i_)
    {
        free();
        i = i_;
        set_type(type::int_type);
        return *this;
    }

    inline utree& utree::operator=(double d_)
    {
        free();
        d = d_;
        set_type(type::double_type);
        return *this;
    }

    inline utree& utree::operator=(char const* s_)
    {
        free();
        s.construct(s_, s_ + strlen(s_));
        set_type(type::string_type);
        return *this;
    }

    inline utree& utree::operator=(std::string const& s_)
    {
        free();
        s.construct(s_.begin(), s_.end());
        set_type(type::string_type);
        return *this;
    }

    template <typename Base, utree_type::info type_>
    inline utree& utree::operator=(basic_string<Base, type_> const& bin)
    {
        free();
        s.construct(bin.begin(), bin.end());
        set_type(type_);
        return *this;
    }

    inline utree& utree::operator=(boost::reference_wrapper<utree> ref)
    {
        free();
        p = ref.get_pointer();
        set_type(type::reference_type);
        return *this;
    }

    inline utree& utree::operator=(any_ptr const& p_)
    {
        free();
        v.p = p_.p;
        v.i = p_.i;
        set_type(type::any_type);
        return *this;
    }

    inline utree& utree::operator=(function_base const& pf_)
    {
        free();
        pf = pf_.clone();
        set_type(type::function_type);
        return *this;
    }

    inline utree& utree::operator=(function_base* pf_)
    {
        free();
        pf = pf_;
        set_type(type::function_type);
        return *this;
    }

    template <typename Iter>
    inline utree& utree::operator=(boost::iterator_range<Iter> r)
    {
        free();
        assign(r.begin(), r.end());
        return *this;
    }

    template <typename F>
    typename boost::result_of<F(utree const&)>::type
    inline utree::visit(utree const& x, F f)
    {
        return detail::visit_impl<utree const>::apply(x, f);
    }

    template <typename F>
    typename boost::result_of<F(utree&)>::type
    inline utree::visit(utree& x, F f)
    {
        return detail::visit_impl<utree>::apply(x, f);
    }

    template <typename F>
    typename boost::result_of<F(utree const&, utree const&)>::type
    inline utree::visit(utree const& x, utree const& y, F f)
    {
        return detail::visit_impl<utree const, utree const>::apply(x, y, f);
    }

    template <typename F>
    typename boost::result_of<F(utree const&, utree&)>::type
    inline utree::visit(utree const& x, utree& y, F f)
    {
        return detail::visit_impl<utree const, utree>::apply(x, y, f);
    }

    template <typename F>
    typename boost::result_of<F(utree&, utree const&)>::type
    inline utree::visit(utree& x, utree const& y, F f)
    {
        return detail::visit_impl<utree, utree const>::apply(x, y, f);
    }

    template <typename F>
    typename boost::result_of<F(utree&, utree&)>::type
    inline utree::visit(utree& x, utree& y, F f)
    {
        return detail::visit_impl<utree, utree>::apply(x, y, f);
    }

    inline utree::reference get(utree::reference ut, utree::size_type i)
    { return detail::index_impl::apply(ut, i); }

    inline utree::const_reference
    get(utree::const_reference ut, utree::size_type i)
    { return detail::index_impl::apply(ut, i); }

    template <typename T>
    inline void utree::push_front(T const& val)
    {
        if (get_type() == type::reference_type)
            return p->push_front(val);

        ensure_list_type("push_front()");
        l.push_front(val);
    }

    template <typename T>
    inline void utree::push_back(T const& val)
    {
        if (get_type() == type::reference_type)
            return p->push_back(val);

        ensure_list_type("push_back()");
        l.push_back(val);
    }

    template <typename T>
    inline utree::iterator utree::insert(iterator pos, T const& val)
    {
        if (get_type() == type::reference_type)
            return p->insert(pos, val);

        ensure_list_type("insert()");
        if (!pos.node)
        {
            l.push_back(val);
            return utree::iterator(l.last, l.last->prev);
        }
        l.insert(val, pos);
        return utree::iterator(pos.node->prev, pos.node->prev->prev);
    }

    template <typename T>
    inline void utree::insert(iterator pos, std::size_t n, T const& val)
    {
        if (get_type() == type::reference_type)
            return p->insert(pos, n, val);

        ensure_list_type("insert()");
        for (std::size_t i = 0; i != n; ++i)
            insert(pos, val);
    }

    template <typename Iterator>
    inline void utree::insert(iterator pos, Iterator first, Iterator last)
    {
        if (get_type() == type::reference_type)
            return p->insert(pos, first, last);

        ensure_list_type("insert()");
        while (first != last)
            insert(pos, *first++);
    }

    template <typename Iterator>
    inline void utree::assign(Iterator first, Iterator last)
    {
        if (get_type() == type::reference_type)
            return p->assign(first, last);

        clear();
        set_type(type::list_type);

        while (first != last)
        {
            push_back(*first);
            ++first;
        }
    }

    inline void utree::clear()
    {
        if (get_type() == type::reference_type)
            return p->clear();

        // clear will always make this an invalid type
        free();
        set_type(type::invalid_type);
    }

    inline void utree::pop_front()
    {
        if (get_type() == type::reference_type)
            return p->pop_front();
        if (get_type() != type::list_type)
            BOOST_THROW_EXCEPTION(
                bad_type_exception
                    ("pop_front() called on non-list utree type",
                     get_type()));

        l.pop_front();
    }

    inline void utree::pop_back()
    {
        if (get_type() == type::reference_type)
            return p->pop_back();
        if (get_type() != type::list_type)
            BOOST_THROW_EXCEPTION(
                bad_type_exception
                    ("pop_back() called on non-list utree type",
                     get_type()));

        l.pop_back();
    }

    inline utree::iterator utree::erase(iterator pos)
    {
        if (get_type() == type::reference_type)
            return p->erase(pos);
        if (get_type() != type::list_type)
            BOOST_THROW_EXCEPTION(
                bad_type_exception
                    ("erase() called on non-list utree type",
                     get_type()));

        detail::list::node* np = l.erase(pos.node);
        return iterator(np, np?np->prev:l.last);
    }

    inline utree::iterator utree::erase(iterator first, iterator last)
    {
        if (get_type() == type::reference_type)
            return p->erase(first, last);

        if (get_type() != type::list_type)
            BOOST_THROW_EXCEPTION(
                bad_type_exception
                    ("erase() called on non-list utree type",
                     get_type()));
        while (first != last)
            erase(first++);
        return last;
    }

    inline utree::iterator utree::begin()
    {
        if (get_type() == type::reference_type)
            return p->begin();
        else if (get_type() == type::range_type)
            return iterator(r.first, 0);

        // otherwise...
        ensure_list_type("begin()");
        return iterator(l.first, 0);
    }

    inline utree::iterator utree::end()
    {
        if (get_type() == type::reference_type)
            return p->end();
        else if (get_type() == type::range_type)
            return iterator(0, r.first);

        // otherwise...
        ensure_list_type("end()");
        return iterator(0, l.last);
    }

    inline utree::ref_iterator utree::ref_begin()
    {
        if (get_type() == type::reference_type)
            return p->ref_begin();
        else if (get_type() == type::range_type)
            return ref_iterator(r.first, 0);

        // otherwise...
        ensure_list_type("ref_begin()");
        return ref_iterator(l.first, 0);
    }

    inline utree::ref_iterator utree::ref_end()
    {
        if (get_type() == type::reference_type)
            return p->ref_end();
        else if (get_type() == type::range_type)
            return ref_iterator(0, r.first);

        // otherwise...
        ensure_list_type("ref_end()");
        return ref_iterator(0, l.last);
    }

    inline utree::const_iterator utree::begin() const
    {
        if (get_type() == type::reference_type)
            return ((utree const*)p)->begin();
        if (get_type() == type::range_type)
            return const_iterator(r.first, 0);

        // otherwise...
        if (get_type() != type::list_type)
            BOOST_THROW_EXCEPTION(
                bad_type_exception
                    ("begin() called on non-list utree type",
                     get_type()));

        return const_iterator(l.first, 0);
    }

    inline utree::const_iterator utree::end() const
    {
        if (get_type() == type::reference_type)
            return ((utree const*)p)->end();
        if (get_type() == type::range_type)
            return const_iterator(0, r.first);

        // otherwise...
        if (get_type() != type::list_type)
            BOOST_THROW_EXCEPTION(
                bad_type_exception
                    ("end() called on non-list utree type",
                     get_type()));

        return const_iterator(0, l.last);
    }

    inline bool utree::empty() const
    {
        type::info t = get_type();
        if (t == type::reference_type)
            return ((utree const*)p)->empty();

        if (t == type::range_type)
            return r.first == 0;
        if (t == type::list_type)
            return l.size == 0;

        return t == type::nil_type || t == type::invalid_type;
    }

    inline std::size_t utree::size() const
    {
        type::info t = get_type();
        if (t == type::reference_type)
            return ((utree const*)p)->size();

        if (t == type::range_type)
        {
            // FIXME: O(n), and we have the room to store the size of a range
            // in the union if we compute it when assigned/constructed.
            std::size_t size = 0;
            detail::list::node* n = r.first;
            while (n)
            {
                n = n->next;
                ++size;
            }
            return size;
        }
        if (t == type::list_type)
            return l.size;

        if (t == type::string_type)
            return s.size();

        if (t == type::symbol_type)
            return s.size();

        if (t == type::binary_type)
            return s.size();

        if (t == type::string_range_type)
            return sr.last - sr.first;

        if (t != type::nil_type)
            BOOST_THROW_EXCEPTION(
                bad_type_exception
                    ("size() called on non-list and non-string utree type",
                     get_type()));

        return 0;
    }

    inline utree_type::info utree::which() const
    {
        return get_type();
    }

    inline utree& utree::front()
    {
        if (get_type() == type::reference_type)
            return p->front();
        if (get_type() == type::range_type)
        {
            if (!r.first)
                BOOST_THROW_EXCEPTION(
                    empty_exception("front() called on empty utree range"));
            return r.first->val;
        }

        // otherwise...
        if (get_type() != type::list_type)
            BOOST_THROW_EXCEPTION(
                bad_type_exception
                    ("front() called on non-list utree type", get_type()));
        else if (!l.first)
            BOOST_THROW_EXCEPTION(
                empty_exception("front() called on empty utree list"));

        return l.first->val;
    }

    inline utree& utree::back()
    {
        if (get_type() == type::reference_type)
            return p->back();
        if (get_type() == type::range_type)
        {
            if (!r.last)
                BOOST_THROW_EXCEPTION(
                    empty_exception("back() called on empty utree range"));
            return r.last->val;
        }

        // otherwise...
        if (get_type() != type::list_type)
            BOOST_THROW_EXCEPTION(
                bad_type_exception
                    ("back() called on non-list utree type", get_type()));
        else if (!l.last)
            BOOST_THROW_EXCEPTION(
                empty_exception("back() called on empty utree list"));

        return l.last->val;
    }

    inline utree const& utree::front() const
    {
        if (get_type() == type::reference_type)
            return ((utree const*)p)->front();
        if (get_type() == type::range_type)
        {
            if (!r.first)
                BOOST_THROW_EXCEPTION(
                    empty_exception("front() called on empty utree range"));
            return r.first->val;
        }

        // otherwise...
        if (get_type() != type::list_type)
            BOOST_THROW_EXCEPTION(
                bad_type_exception
                    ("front() called on non-list utree type", get_type()));
        else if (!l.first)
            BOOST_THROW_EXCEPTION(
                empty_exception("front() called on empty utree list"));

        return l.first->val;
    }

    inline utree const& utree::back() const
    {
        if (get_type() == type::reference_type)
            return ((utree const*)p)->back();
        if (get_type() == type::range_type)
        {
            if (!r.last)
                BOOST_THROW_EXCEPTION(
                    empty_exception("back() called on empty utree range"));
            return r.last->val;
        }

        // otherwise...
        if (get_type() != type::list_type)
            BOOST_THROW_EXCEPTION(
                bad_type_exception
                    ("back() called on non-list utree type", get_type()));
        else if (!l.last)
            BOOST_THROW_EXCEPTION(
                empty_exception("back() called on empty utree list"));

        return l.last->val;
    }

    inline void utree::swap(utree& other)
    {
        s.swap(other.s);
    }

    inline utree::type::info utree::get_type() const
    {
        // the fast string holds the type info
        return static_cast<utree::type::info>(s.get_type());
    }

    inline void utree::set_type(type::info t)
    {
        // the fast string holds the type info
        s.set_type(t);
    }

    inline void utree::ensure_list_type(char const* failed_in)
    {
        type::info t = get_type();
        if (t == type::invalid_type)
        {
            set_type(type::list_type);
            l.default_construct();
        }
        else if (get_type() != type::list_type)
        {
            std::string msg = failed_in;
            msg += "called on non-list and non-invalid utree type";
            BOOST_THROW_EXCEPTION(bad_type_exception(msg.c_str(), get_type()));
        }
    }

    inline void utree::free()
    {
        switch (get_type())
        {
            case type::binary_type:
            case type::symbol_type:
            case type::string_type:
                s.free();
                break;
            case type::list_type:
                l.free();
                break;
            case type::function_type:
                delete pf;
                break;
            default:
                break;
        };
        s.initialize();
    }

    inline void utree::copy(utree const& other)
    {
        set_type(other.get_type());
        switch (other.get_type())
        {
            default:
                BOOST_THROW_EXCEPTION(
                    bad_type_exception("corrupt utree type", other.get_type()));
                break;
            case type::invalid_type:
            case type::nil_type:
                s.tag(other.s.tag());
                break;
            case type::bool_type:
                b = other.b;
                s.tag(other.s.tag());
                break;
            case type::int_type:
                i = other.i;
                s.tag(other.s.tag());
                break;
            case type::double_type:
                d = other.d;
                s.tag(other.s.tag());
                break;
            case type::reference_type:
                p = other.p;
                s.tag(other.s.tag());
                break;
            case type::any_type:
                v = other.v;
                s.tag(other.s.tag());
                break;
            case type::range_type:
                r = other.r;
                s.tag(other.s.tag());
                break;
            case type::string_range_type:
                sr = other.sr;
                s.tag(other.s.tag());
                break;
            case type::function_type:
                pf = other.pf->clone();
                s.tag(other.s.tag());
                break;
            case type::string_type:
            case type::symbol_type:
            case type::binary_type:
                s.copy(other.s);
                s.tag(other.s.tag());
                break;
            case type::list_type:
                l.copy(other.l);
                s.tag(other.s.tag());
                break;
        }
    }

    template <typename T>
    struct is_iterator_range
      : boost::mpl::false_
    {};

    template <typename Iterator>
    struct is_iterator_range<boost::iterator_range<Iterator> >
      : boost::mpl::true_
    {};

    template <typename To>
    struct utree_cast
    {
        typedef To result_type;

        template <typename From>
        To dispatch(From const& val, boost::mpl::true_) const
        {
            return To(val); // From is convertible to To
        }

        template <typename From>
        BOOST_NORETURN To dispatch(From const&, boost::mpl::false_) const
        {
            // From is NOT convertible to To !!!
            throw std::bad_cast();
            BOOST_UNREACHABLE_RETURN(To())
        }

        template <typename From>
        To operator()(From const& val) const
        {
            // boost::iterator_range has a templated constructor, accepting
            // any argument and hence any type is 'convertible' to it.
            typedef typename boost::mpl::eval_if<
                is_iterator_range<To>
              , boost::is_same<From, To>, boost::is_convertible<From, To>
            >::type is_convertible;
            return dispatch(val, is_convertible());
        }
    };

    template <typename T>
    struct utree_cast<T*>
    {
        typedef T* result_type;

        template <typename From>
        BOOST_NORETURN T* operator()(From const&) const
        {
            // From is NOT convertible to T !!!
            throw std::bad_cast();
            BOOST_UNREACHABLE_RETURN(NULL)
        }

        T* operator()(any_ptr const& p) const
        {
            return p.get<T*>();
        }
    };

    template <typename T>
    inline T utree::get() const
    {
        return utree::visit(*this, utree_cast<T>());
    }

    inline utree& utree::deref()
    {
        return (get_type() == type::reference_type) ? *p : *this;
    }

    inline utree const& utree::deref() const
    {
        return (get_type() == type::reference_type) ? *p : *this;
    }

    inline short utree::tag() const
    {
        return s.tag();
    }

    inline void utree::tag(short tag)
    {
        s.tag(tag);
    }

    inline utree utree::eval(utree const& env) const
    {
        if (get_type() == type::reference_type)
            return deref().eval(env);

        if (get_type() != type::function_type)
            BOOST_THROW_EXCEPTION(
                bad_type_exception(
                    "eval() called on non-function utree type", get_type()));
        return (*pf)(env);
    }

    inline utree utree::eval(utree& env) const
    {
        if (get_type() == type::reference_type)
            return deref().eval(env);

        if (get_type() != type::function_type)
            BOOST_THROW_EXCEPTION(
                bad_type_exception(
                    "eval() called on non-function utree type", get_type()));
        return (*pf)(env);
    }

    inline utree utree::operator() (utree const& env) const
    {
        return eval(env);
    }

    inline utree utree::operator() (utree& env) const
    {
        return eval(env);
    }
}}

#ifdef _MSC_VER
# pragma warning(pop)
#endif
#endif

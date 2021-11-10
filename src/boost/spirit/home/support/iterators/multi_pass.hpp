//  Copyright (c) 2001, Daniel C. Nuffer
//  Copyright (c) 2001-2011 Hartmut Kaiser
//  http://spirit.sourceforge.net/
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_ITERATOR_MULTI_PASS_MAR_16_2007_1124AM)
#define BOOST_SPIRIT_ITERATOR_MULTI_PASS_MAR_16_2007_1124AM

#include <boost/config.hpp>
#include <boost/spirit/home/support/iterators/multi_pass_fwd.hpp>
#include <boost/spirit/home/support/iterators/detail/multi_pass.hpp>
#include <boost/spirit/home/support/iterators/detail/combine_policies.hpp>
#include <boost/limits.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/utility/base_from_member.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // The default multi_pass instantiation uses a ref-counted std_deque scheme.
    ///////////////////////////////////////////////////////////////////////////
    template<typename T, typename Policies>
    class multi_pass
      : private boost::base_from_member<
            typename Policies::BOOST_NESTED_TEMPLATE shared<T>*>
      , public Policies::BOOST_NESTED_TEMPLATE unique<T>
    {
    private:
        // unique and shared data types
        typedef typename Policies::BOOST_NESTED_TEMPLATE unique<T>
            policies_base_type;
        typedef typename Policies::BOOST_NESTED_TEMPLATE shared<T>
            shared_data_type;

        typedef boost::base_from_member<shared_data_type*> member_base;

        // define the types the standard embedded iterator typedefs are taken
        // from
        typedef typename policies_base_type::input_policy iterator_type;

    public:
        // standard iterator typedefs
        typedef std::forward_iterator_tag iterator_category;
        typedef typename iterator_type::value_type value_type;
        typedef typename iterator_type::difference_type difference_type;
        typedef typename iterator_type::distance_type distance_type;
        typedef typename iterator_type::reference reference;
        typedef typename iterator_type::pointer pointer;

        multi_pass() : member_base(static_cast<shared_data_type*>(0)) {}

        explicit multi_pass(T& input)
          : member_base(new shared_data_type(input)), policies_base_type(input) {}

        explicit multi_pass(T const& input)
          : member_base(new shared_data_type(input)), policies_base_type(input) {}

        multi_pass(multi_pass const& x)
          : member_base(x.member), policies_base_type(x)
        {
            policies_base_type::clone(*this);
        }

#if BOOST_WORKAROUND(__GLIBCPP__, == 20020514)
        // The standard library shipped with gcc-3.1 has a bug in
        // bits/basic_string.tcc. It tries to use iter::iter(0) to
        // construct an iterator. Ironically, this  happens in sanity
        // checking code that isn't required by the standard.
        // The workaround is to provide an additional constructor that
        // ignores its int argument and behaves like the default constructor.
        multi_pass(int) : member_base(static_cast<shared_data_type*>(0)) {}
#endif // BOOST_WORKAROUND(__GLIBCPP__, == 20020514)

        ~multi_pass()
        {
            if (policies_base_type::release(*this)) {
                policies_base_type::destroy(*this);
                delete this->member;
            }
        }

        multi_pass& operator=(multi_pass const& x)
        {
            if (this != &x) {
                multi_pass temp(x);
                temp.swap(*this);
            }
            return *this;
        }

        void swap(multi_pass& x)
        {
            boost::swap(this->member, x.member);
            this->policies_base_type::swap(x);
        }

        reference operator*() const
        {
            policies_base_type::docheck(*this);
            return policies_base_type::dereference(*this);
        }
        pointer operator->() const
        {
            return &(operator*());
        }

        multi_pass& operator++()
        {
            policies_base_type::docheck(*this);
            policies_base_type::increment(*this);
            return *this;
        }
        multi_pass operator++(int)
        {
            multi_pass tmp(*this);
            ++*this;
            return tmp;
        }

        void clear_queue(BOOST_SCOPED_ENUM(traits::clear_mode) mode =
            traits::clear_mode::clear_if_enabled)
        {
            if (mode == traits::clear_mode::clear_always || !inhibit_clear_queue())
                policies_base_type::clear_queue(*this);
        }
        bool inhibit_clear_queue() const
        {
            return this->member->inhibit_clear_queue_;
        }
        void inhibit_clear_queue(bool flag)
        {
            this->member->inhibit_clear_queue_ = flag;
        }

        bool operator==(multi_pass const& y) const
        {
            if (is_eof())
                return y.is_eof();
            if (y.is_eof())
                return false;

            return policies_base_type::equal_to(*this, y);
        }
        bool operator<(multi_pass const& y) const
        {
            return policies_base_type::less_than(*this, y);
        }

        bool operator!=(multi_pass const& y) const
        {
            return !(*this == y);
        }
        bool operator>(multi_pass const& y) const
        {
            return y < *this;
        }
        bool operator>=(multi_pass const& y) const
        {
            return !(*this < y);
        }
        bool operator<=(multi_pass const& y) const
        {
            return !(y < *this);
        }

        // allow access to base member
        shared_data_type* shared() const { return this->member; }

    private: // helper functions
        bool is_eof() const
        {
            return (0 == this->member) || policies_base_type::is_eof(*this);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //  Generator function
    ///////////////////////////////////////////////////////////////////////////
    template <typename Policies, typename T>
    inline multi_pass<T, Policies>
    make_multi_pass(T& i)
    {
        return multi_pass<T, Policies>(i);
    }
    template <typename Policies, typename T>
    inline multi_pass<T, Policies>
    make_multi_pass(T const& i)
    {
        return multi_pass<T, Policies>(i);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    inline multi_pass<T>
    make_default_multi_pass(T& i)
    {
        return multi_pass<T>(i);
    }
    template <typename T>
    inline multi_pass<T>
    make_default_multi_pass(T const& i)
    {
        return multi_pass<T>(i);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Policies>
    inline void
    swap(multi_pass<T, Policies> &x, multi_pass<T, Policies> &y)
    {
        x.swap(y);
    }

    ///////////////////////////////////////////////////////////////////////////
    // define special functions allowing to integrate any multi_pass iterator
    // with expectation points
    namespace traits
    {
        template <typename T, typename Policies>
        void clear_queue(multi_pass<T, Policies>& mp
          , BOOST_SCOPED_ENUM(traits::clear_mode) mode)
        {
            mp.clear_queue(mode);
        }

        template <typename T, typename Policies>
        void inhibit_clear_queue(multi_pass<T, Policies>& mp, bool flag)
        {
            mp.inhibit_clear_queue(flag);
        }

        template <typename T, typename Policies>
        bool inhibit_clear_queue(multi_pass<T, Policies>& mp)
        {
            return mp.inhibit_clear_queue();
        }
    }

}} // namespace boost::spirit

#endif



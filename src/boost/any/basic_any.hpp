// Copyright Ruslan Arutyunyan, 2019-2021.
// Copyright Antony Polukhin, 2021.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Contributed by Ruslan Arutyunyan

#ifndef BOOST_ANYS_BASIC_ANY_HPP_INCLUDED
#define BOOST_ANYS_BASIC_ANY_HPP_INCLUDED

#include <boost/config.hpp>
#ifdef BOOST_HAS_PRAGMA_ONCE
# pragma once
#endif

#include <boost/any/bad_any_cast.hpp>
#include <boost/any/fwd.hpp>
#include <boost/assert.hpp>
#include <boost/aligned_storage.hpp>
#include <boost/type_index.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/decay.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_nothrow_move_constructible.hpp>
#include <boost/throw_exception.hpp>
#include <boost/static_assert.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/core/addressof.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/conditional.hpp>

namespace boost {

namespace anys {

    template <std::size_t OptimizeForSize, std::size_t OptimizeForAlignment>
    class basic_any
    {
        BOOST_STATIC_ASSERT_MSG(OptimizeForSize > 0 && OptimizeForAlignment > 0, "Size and Align shall be positive values");
        BOOST_STATIC_ASSERT_MSG(OptimizeForSize >= OptimizeForAlignment, "Size shall non less than Align");
        BOOST_STATIC_ASSERT_MSG((OptimizeForAlignment & (OptimizeForAlignment - 1)) == 0, "Align shall be a power of 2");
        BOOST_STATIC_ASSERT_MSG(OptimizeForSize % OptimizeForAlignment == 0, "Size shall be multiple of alignment");
    private:
        enum operation
        {
            Destroy,
            Move,
            Copy,
            AnyCast,
            UnsafeCast,
            Typeinfo
        };

        template <typename ValueType>
        static void* small_manager(operation op, basic_any& left, const basic_any* right, const boost::typeindex::type_info* info)
        {
            switch (op)
            {
                case Destroy:
                    BOOST_ASSERT(!left.empty());
                    reinterpret_cast<ValueType*>(&left.content.small_value)->~ValueType();
                    break;
                case Move: {
                    BOOST_ASSERT(left.empty());
                    BOOST_ASSERT(right);
                    BOOST_ASSERT(!right->empty());
                    BOOST_ASSERT(right->type() == boost::typeindex::type_id<ValueType>());
                    ValueType* value = reinterpret_cast<ValueType*>(&const_cast<basic_any*>(right)->content.small_value);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
                    new (&left.content.small_value) ValueType(std::move(*value));
#else
                    new (&left.content.small_value) ValueType(*value);
#endif
                    left.man = right->man;
                    reinterpret_cast<ValueType const*>(&right->content.small_value)->~ValueType();
                    const_cast<basic_any*>(right)->man = 0;

                    };
                    break;

                case Copy:
                    BOOST_ASSERT(left.empty());
                    BOOST_ASSERT(right);
                    BOOST_ASSERT(!right->empty());
                    BOOST_ASSERT(right->type() == boost::typeindex::type_id<ValueType>());
                    new (&left.content.small_value) ValueType(*reinterpret_cast<const ValueType*>(&right->content.small_value));
                    left.man = right->man;
                    break;
                case AnyCast:
                    BOOST_ASSERT(info);
                    BOOST_ASSERT(!left.empty());
                    return boost::typeindex::type_id<ValueType>() == *info ?
                            reinterpret_cast<typename remove_cv<ValueType>::type *>(&left.content.small_value) : 0;
                case UnsafeCast:
                    BOOST_ASSERT(!left.empty());
                    return reinterpret_cast<typename remove_cv<ValueType>::type *>(&left.content.small_value);
                case Typeinfo:
                    return const_cast<void*>(static_cast<const void*>(&boost::typeindex::type_id<ValueType>().type_info()));
            }

            return 0;
        }

        template <typename ValueType>
        static void* large_manager(operation op, basic_any& left, const basic_any* right, const boost::typeindex::type_info* info)
        {
            switch (op)
            {
                case Destroy:
                    BOOST_ASSERT(!left.empty());
                    delete static_cast<ValueType*>(left.content.large_value);
                    break;
                case Move:
                    BOOST_ASSERT(left.empty());
                    BOOST_ASSERT(right);
                    BOOST_ASSERT(!right->empty());
                    BOOST_ASSERT(right->type() == boost::typeindex::type_id<ValueType>());
                    left.content.large_value = right->content.large_value;
                    left.man = right->man;
                    const_cast<basic_any*>(right)->content.large_value = 0;
                    const_cast<basic_any*>(right)->man = 0;
                    break;
                case Copy:
                    BOOST_ASSERT(left.empty());
                    BOOST_ASSERT(right);
                    BOOST_ASSERT(!right->empty());
                    BOOST_ASSERT(right->type() == boost::typeindex::type_id<ValueType>());
                    left.content.large_value = new ValueType(*static_cast<const ValueType*>(right->content.large_value));
                    left.man = right->man;
                    break;
                case AnyCast:
                    BOOST_ASSERT(info);
                    BOOST_ASSERT(!left.empty());
                    return boost::typeindex::type_id<ValueType>() == *info ?
                            static_cast<typename remove_cv<ValueType>::type *>(left.content.large_value) : 0;
                case UnsafeCast:
                    BOOST_ASSERT(!left.empty());
                    return reinterpret_cast<typename remove_cv<ValueType>::type *>(left.content.large_value);
                case Typeinfo:
                    return const_cast<void*>(static_cast<const void*>(&boost::typeindex::type_id<ValueType>().type_info()));
            }

            return 0;
        }

        template <typename ValueType>
        struct is_small_object : boost::integral_constant<bool, sizeof(ValueType) <= OptimizeForSize &&
            boost::alignment_of<ValueType>::value <= OptimizeForAlignment &&
            boost::is_nothrow_move_constructible<ValueType>::value>
        {};

        template <typename ValueType>
        static void create(basic_any& any, const ValueType& value, boost::true_type)
        {
            typedef typename boost::decay<const ValueType>::type DecayedType;

            any.man = &small_manager<DecayedType>;
            new (&any.content.small_value) ValueType(value);
        }

        template <typename ValueType>
        static void create(basic_any& any, const ValueType& value, boost::false_type)
        {
            typedef typename boost::decay<const ValueType>::type DecayedType;

            any.man = &large_manager<DecayedType>;
            any.content.large_value = new DecayedType(value);
        }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
        template <typename ValueType>
        static void create(basic_any& any, ValueType&& value, boost::true_type)
        {
            typedef typename boost::decay<const ValueType>::type DecayedType;
            any.man = &small_manager<DecayedType>;
            new (&any.content.small_value) DecayedType(static_cast<ValueType&&>(value));
        }

        template <typename ValueType>
        static void create(basic_any& any, ValueType&& value, boost::false_type)
        {
            typedef typename boost::decay<const ValueType>::type DecayedType;
            any.man = &large_manager<DecayedType>;
            any.content.large_value = new DecayedType(static_cast<ValueType&&>(value));
        }
#endif
    public: // non-type template parameters accessors
            static BOOST_CONSTEXPR_OR_CONST std::size_t buffer_size = OptimizeForSize;
            static BOOST_CONSTEXPR_OR_CONST std::size_t buffer_align = OptimizeForAlignment;

    public: // structors

        BOOST_CONSTEXPR basic_any() BOOST_NOEXCEPT
            : man(0), content()
        {
        }

        template<typename ValueType>
        basic_any(const ValueType & value)
            : man(0), content()
        {
            BOOST_STATIC_ASSERT_MSG(
                !(boost::is_same<ValueType, boost::any>::value),
                "boost::anys::basic_any shall not be constructed from boost::any"
            );
            BOOST_STATIC_ASSERT_MSG(
                !anys::detail::is_basic_any<ValueType>::value,
                "boost::anys::basic_any<A, B> shall not be constructed from boost::anys::basic_any<C, D>"
            );
            create(*this, value, is_small_object<ValueType>());
        }

        basic_any(const basic_any & other)
          : man(0), content()
        {
            if (other.man)
            {
                other.man(Copy, *this, &other, 0);
            }
        }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
        // Move constructor
        basic_any(basic_any&& other) BOOST_NOEXCEPT
          : man(0), content()
        {
            if (other.man)
            {
                other.man(Move, *this, &other, 0);
            }
        }

        // Perfect forwarding of ValueType
        template<typename ValueType>
        basic_any(ValueType&& value
            , typename boost::disable_if<boost::is_same<basic_any&, ValueType> >::type* = 0 // disable if value has type `basic_any&`
            , typename boost::disable_if<boost::is_const<ValueType> >::type* = 0) // disable if value has type `const ValueType&&`
          : man(0), content()
        {
            typedef typename boost::decay<ValueType>::type DecayedType;
            BOOST_STATIC_ASSERT_MSG(
                !(boost::is_same<DecayedType, boost::any>::value),
                "boost::anys::basic_any shall not be constructed from boost::any"
            );
            BOOST_STATIC_ASSERT_MSG(
                !anys::detail::is_basic_any<DecayedType>::value,
                "boost::anys::basic_any<A, B> shall not be constructed from boost::anys::basic_any<C, D>"
            );
            create(*this, static_cast<ValueType&&>(value), is_small_object<DecayedType>());
        }
#endif

        ~basic_any() BOOST_NOEXCEPT
        {
            if (man)
            {
                man(Destroy, *this, 0, 0);
            }
        }

    public: // modifiers

        basic_any & swap(basic_any & rhs) BOOST_NOEXCEPT
        {
            if (this == &rhs)
            {
                return *this;
            }

            if (man && rhs.man)
            {
                basic_any tmp;
                rhs.man(Move, tmp, &rhs, 0);
                man(Move, rhs, this, 0);
                tmp.man(Move, *this, &tmp, 0);
            }
            else if (man)
            {
                man(Move, rhs, this, 0);
            }
            else if (rhs.man)
            {
                rhs.man(Move, *this, &rhs, 0);
            }
            return *this;
        }


#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
        template<typename ValueType>
        basic_any & operator=(const ValueType & rhs)
        {
            BOOST_STATIC_ASSERT_MSG(
                !(boost::is_same<ValueType, boost::any>::value),
                "boost::any shall not be assigned into boost::anys::basic_any"
            );
            BOOST_STATIC_ASSERT_MSG(
                !anys::detail::is_basic_any<ValueType>::value,
                "boost::anys::basic_any<A, B> shall not be assigned into boost::anys::basic_any<C, D>"
            );
            basic_any(rhs).swap(*this);
            return *this;
        }

        basic_any & operator=(basic_any rhs)
        {
            rhs.swap(*this);
            return *this;
        }

#else
        basic_any & operator=(const basic_any& rhs)
        {
            basic_any(rhs).swap(*this);
            return *this;
        }

        // move assignment
        basic_any & operator=(basic_any&& rhs) BOOST_NOEXCEPT
        {
            rhs.swap(*this);
            basic_any().swap(rhs);
            return *this;
        }

        // Perfect forwarding of ValueType
        template <class ValueType>
        basic_any & operator=(ValueType&& rhs)
        {
            typedef typename boost::decay<ValueType>::type DecayedType;
            BOOST_STATIC_ASSERT_MSG(
                !(boost::is_same<DecayedType, boost::any>::value),
                "boost::any shall not be assigned into boost::anys::basic_any"
            );
            BOOST_STATIC_ASSERT_MSG(
                (!anys::detail::is_basic_any<DecayedType>::value || boost::is_same<DecayedType, basic_any>::value),
                "boost::anys::basic_any<A, B> shall not be assigned into boost::anys::basic_any<C, D>"
            );
            basic_any(static_cast<ValueType&&>(rhs)).swap(*this);
            return *this;
        }
#endif

    public: // queries

        bool empty() const BOOST_NOEXCEPT
        {
            return !man;
        }

        void clear() BOOST_NOEXCEPT
        {
            basic_any().swap(*this);
        }

        const boost::typeindex::type_info& type() const BOOST_NOEXCEPT
        {
            return man
                    ? *static_cast<const boost::typeindex::type_info*>(man(Typeinfo, const_cast<basic_any&>(*this), 0, 0))
                    : boost::typeindex::type_id<void>().type_info();
        }

    private: // representation

        template<typename ValueType, std::size_t Size, std::size_t Alignment>
        friend ValueType * any_cast(basic_any<Size, Alignment> *) BOOST_NOEXCEPT;

        template<typename ValueType, std::size_t Size, std::size_t Alignment>
        friend ValueType * unsafe_any_cast(basic_any<Size, Alignment> *) BOOST_NOEXCEPT;

        typedef void*(*manager)(operation op, basic_any& left, const basic_any* right, const boost::typeindex::type_info* info);

        manager man;

        union content {
            void * large_value;
            typename boost::aligned_storage<OptimizeForSize, OptimizeForAlignment>::type small_value;
        } content;
    };

    template<std::size_t OptimizeForSize, std::size_t OptimizeForAlignment>
    void swap(basic_any<OptimizeForSize, OptimizeForAlignment>& lhs, basic_any<OptimizeForSize, OptimizeForAlignment>& rhs) BOOST_NOEXCEPT
    {
        lhs.swap(rhs);
    }

    template<typename ValueType, std::size_t Size, std::size_t Alignment>
    ValueType * any_cast(basic_any<Size, Alignment> * operand) BOOST_NOEXCEPT
    {
        return operand->man ?
                static_cast<typename remove_cv<ValueType>::type *>(operand->man(basic_any<Size, Alignment>::AnyCast, *operand, 0, &boost::typeindex::type_id<ValueType>().type_info()))
                : 0;
    }

    template<typename ValueType, std::size_t OptimizeForSize, std::size_t OptimizeForAlignment>
    inline const ValueType * any_cast(const basic_any<OptimizeForSize, OptimizeForAlignment> * operand) BOOST_NOEXCEPT
    {
        return any_cast<ValueType>(const_cast<basic_any<OptimizeForSize, OptimizeForAlignment> *>(operand));
    }

    template<typename ValueType, std::size_t OptimizeForSize, std::size_t OptimizeForAlignment>
    ValueType any_cast(basic_any<OptimizeForSize, OptimizeForAlignment> & operand)
    {
        typedef typename remove_reference<ValueType>::type nonref;

        nonref * result = any_cast<nonref>(boost::addressof(operand));
        if(!result)
            boost::throw_exception(bad_any_cast());

        // Attempt to avoid construction of a temporary object in cases when
        // `ValueType` is not a reference. Example:
        // `static_cast<std::string>(*result);`
        // which is equal to `std::string(*result);`
        typedef typename boost::conditional<
            boost::is_reference<ValueType>::value,
            ValueType,
            typename boost::add_reference<ValueType>::type
        >::type ref_type;

#ifdef BOOST_MSVC
#   pragma warning(push)
#   pragma warning(disable: 4172) // "returning address of local variable or temporary" but *result is not local!
#endif
        return static_cast<ref_type>(*result);
#ifdef BOOST_MSVC
#   pragma warning(pop)
#endif
    }

    template<typename ValueType, std::size_t OptimizeForSize, std::size_t OptimizeForAlignment>
    inline ValueType any_cast(const basic_any<OptimizeForSize, OptimizeForAlignment> & operand)
    {
        typedef typename remove_reference<ValueType>::type nonref;
        return any_cast<const nonref &>(const_cast<basic_any<OptimizeForSize, OptimizeForAlignment> &>(operand));
    }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    template<typename ValueType, std::size_t OptimizeForSize, std::size_t OptimizeForAlignment>
    inline ValueType any_cast(basic_any<OptimizeForSize, OptimizeForAlignment>&& operand)
    {
        BOOST_STATIC_ASSERT_MSG(
            boost::is_rvalue_reference<ValueType&&>::value /*true if ValueType is rvalue or just a value*/
            || boost::is_const< typename boost::remove_reference<ValueType>::type >::value,
            "boost::any_cast shall not be used for getting nonconst references to temporary objects"
        );
        return any_cast<ValueType>(operand);
    }
#endif


    // Note: The "unsafe" versions of any_cast are not part of the
    // public interface and may be removed at any time. They are
    // required where we know what type is stored in the any and can't
    // use typeid() comparison, e.g., when our types may travel across
    // different shared libraries.
    template<typename ValueType, std::size_t OptimizedForSize, std::size_t OptimizeForAlignment>
    inline ValueType * unsafe_any_cast(basic_any<OptimizedForSize, OptimizeForAlignment> * operand) BOOST_NOEXCEPT
    {
        return static_cast<ValueType*>(operand->man(basic_any<OptimizedForSize, OptimizeForAlignment>::UnsafeCast, *operand, 0, 0));
    }

    template<typename ValueType, std::size_t OptimizeForSize, std::size_t OptimizeForAlignment>
    inline const ValueType * unsafe_any_cast(const basic_any<OptimizeForSize, OptimizeForAlignment> * operand) BOOST_NOEXCEPT
    {
        return unsafe_any_cast<ValueType>(const_cast<basic_any<OptimizeForSize, OptimizeForAlignment> *>(operand));
    }

} // namespace anys

using boost::anys::any_cast;
using boost::anys::unsafe_any_cast;

} // namespace boost

#endif // #ifndef BOOST_ANYS_BASIC_ANY_HPP_INCLUDED

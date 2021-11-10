// Debug support for the circular buffer library.

// Copyright (c) 2003-2008 Jan Gaspar

// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_CIRCULAR_BUFFER_DEBUG_HPP)
#define BOOST_CIRCULAR_BUFFER_DEBUG_HPP

#if defined(_MSC_VER)
    #pragma once
#endif

#if BOOST_CB_ENABLE_DEBUG
#include <cstring>

#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std {
    using ::memset;
}
#endif

#endif // BOOST_CB_ENABLE_DEBUG
namespace boost {

namespace cb_details {

#if BOOST_CB_ENABLE_DEBUG

// The value the uninitialized memory is filled with.
const int UNINITIALIZED = 0xcc;

template <class T>
inline void do_fill_uninitialized_memory(T* data, std::size_t size_in_bytes) BOOST_NOEXCEPT {
    std::memset(static_cast<void*>(data), UNINITIALIZED, size_in_bytes);
}

template <class T>
inline void do_fill_uninitialized_memory(T& /*data*/, std::size_t /*size_in_bytes*/) BOOST_NOEXCEPT {
    // Do nothing
}


class debug_iterator_registry;

/*!
    \class debug_iterator_base
    \brief Registers/unregisters iterators into the registry of valid iterators.

    This class is intended to be a base class of an iterator.
*/
class debug_iterator_base {

private:
// Members

    //! Iterator registry.
    mutable const debug_iterator_registry* m_registry;

    //! Next iterator in the iterator chain.
    mutable const debug_iterator_base* m_next;

public:
// Construction/destruction

    //! Default constructor.
    debug_iterator_base();

    //! Constructor taking the iterator registry as a parameter.
    debug_iterator_base(const debug_iterator_registry* registry);

    //! Copy constructor.
    debug_iterator_base(const debug_iterator_base& rhs);

    //! Destructor.
    ~debug_iterator_base();

// Methods

    //! Assign operator.
    debug_iterator_base& operator = (const debug_iterator_base& rhs);

    //! Is the iterator valid?
    bool is_valid(const debug_iterator_registry* registry) const;

    //! Invalidate the iterator.
    /*!
        \note The method is const in order to invalidate const iterators, too.
    */
    void invalidate() const;

    //! Return the next iterator in the iterator chain.
    const debug_iterator_base* next() const;

    //! Set the next iterator in the iterator chain.
    /*!
        \note The method is const in order to set a next iterator to a const iterator, too.
    */
    void set_next(const debug_iterator_base* it) const;

private:
// Helpers

    //! Register self as a valid iterator.
    void register_self();

    //! Unregister self from valid iterators.
    void unregister_self();
};

/*!
    \class debug_iterator_registry
    \brief Registry of valid iterators.

    This class is intended to be a base class of a container.
*/
class debug_iterator_registry {

    //! Pointer to the chain of valid iterators.
    mutable const debug_iterator_base* m_iterators;

public:
// Methods

    //! Default constructor.
    debug_iterator_registry() : m_iterators(0) {}

    //! Register an iterator into the list of valid iterators.
    /*!
        \note The method is const in order to register iterators into const containers, too.
    */
    void register_iterator(const debug_iterator_base* it) const {
        it->set_next(m_iterators);
        m_iterators = it;
    }

    //! Unregister an iterator from the list of valid iterators.
    /*!
        \note The method is const in order to unregister iterators from const containers, too.
    */
    void unregister_iterator(const debug_iterator_base* it) const {
        const debug_iterator_base* previous = 0;
        for (const debug_iterator_base* p = m_iterators; p != it; previous = p, p = p->next()) {}
        remove(it, previous);
    }

    //! Invalidate every iterator pointing to the same element as the iterator passed as a parameter.
    template <class Iterator>
    void invalidate_iterators(const Iterator& it) {
        const debug_iterator_base* previous = 0;
        for (const debug_iterator_base* p = m_iterators; p != 0; p = p->next()) {
            if (((Iterator*)p)->m_it == it.m_it) {
                p->invalidate();
                remove(p, previous);
                continue;
            }
            previous = p;
        }
    }

    //! Invalidate all iterators except an iterator poining to the same element as the iterator passed as a parameter.
    template <class Iterator>
    void invalidate_iterators_except(const Iterator& it) {
        const debug_iterator_base* previous = 0;
        for (const debug_iterator_base* p = m_iterators; p != 0; p = p->next()) {
            if (((Iterator*)p)->m_it != it.m_it) {
                p->invalidate();
                remove(p, previous);
                continue;
            }
            previous = p;
        }
    }

    //! Invalidate all iterators.
    void invalidate_all_iterators() {
        for (const debug_iterator_base* p = m_iterators; p != 0; p = p->next())
            p->invalidate();
        m_iterators = 0;
    }

private:
// Helpers

    //! Remove the current iterator from the iterator chain.
    void remove(const debug_iterator_base* current,
                const debug_iterator_base* previous) const {
        if (previous == 0)
            m_iterators = m_iterators->next();
        else
            previous->set_next(current->next());
    }
};

// Implementation of the debug_iterator_base methods.

inline debug_iterator_base::debug_iterator_base() : m_registry(0), m_next(0) {}

inline debug_iterator_base::debug_iterator_base(const debug_iterator_registry* registry)
: m_registry(registry), m_next(0) {
    register_self();
}

inline debug_iterator_base::debug_iterator_base(const debug_iterator_base& rhs)
: m_registry(rhs.m_registry), m_next(0) {
    register_self();
}

inline debug_iterator_base::~debug_iterator_base() { unregister_self(); }

inline debug_iterator_base& debug_iterator_base::operator = (const debug_iterator_base& rhs) {
    if (m_registry == rhs.m_registry)
        return *this;
    unregister_self();
    m_registry = rhs.m_registry;
    register_self();
    return *this;
}

inline bool debug_iterator_base::is_valid(const debug_iterator_registry* registry) const {
    return m_registry == registry;
}

inline void debug_iterator_base::invalidate() const { m_registry = 0; }

inline const debug_iterator_base* debug_iterator_base::next() const { return m_next; }

inline void debug_iterator_base::set_next(const debug_iterator_base* it) const { m_next = it; }

inline void debug_iterator_base::register_self() {
    if (m_registry != 0)
        m_registry->register_iterator(this);
}

inline void debug_iterator_base::unregister_self() {
    if (m_registry != 0)
        m_registry->unregister_iterator(this);
}

#endif // #if BOOST_CB_ENABLE_DEBUG

} // namespace cb_details

} // namespace boost

#endif // #if !defined(BOOST_CIRCULAR_BUFFER_DEBUG_HPP)

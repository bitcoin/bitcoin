#ifndef BOOST_FIBERS_WAKER_H
#define BOOST_FIBERS_WAKER_H

#include <cstddef>

#include <boost/config.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/intrusive/slist.hpp>

namespace boost {
namespace fibers {

class context;

namespace detail {

typedef intrusive::slist_member_hook<> waker_queue_hook;

} // detail


class BOOST_FIBERS_DECL waker {
private:
    context *ctx_{};
    size_t epoch_{};

public:
    friend class context;

    waker() = default;

    waker(context * ctx, const size_t epoch)
        : ctx_{ ctx }
        , epoch_{ epoch }
    {}

    bool wake() const noexcept;
};


class BOOST_FIBERS_DECL waker_with_hook : public waker {
public:
    explicit waker_with_hook(waker && w)
        : waker{ std::move(w) }
    {}

    bool is_linked() const noexcept {
        return waker_queue_hook_.is_linked();
    }

    friend bool
    operator==( waker const& lhs, waker const& rhs) noexcept {
        return & lhs == & rhs;
    }

public:
    detail::waker_queue_hook waker_queue_hook_{};
};

namespace detail {
    typedef intrusive::slist<
            waker_with_hook,
            intrusive::member_hook<
                waker_with_hook, detail::waker_queue_hook, & waker_with_hook::waker_queue_hook_ >,
            intrusive::constant_time_size< false >,
            intrusive::cache_last< true >
        >                                               waker_slist_t;
}

class BOOST_FIBERS_DECL wait_queue {
private:
    detail::waker_slist_t   slist_{};

public:
    void suspend_and_wait( detail::spinlock_lock &, context *);
    bool suspend_and_wait_until( detail::spinlock_lock &,
                                 context *,
                                 std::chrono::steady_clock::time_point const&);
    void notify_one();
    void notify_all();

    bool empty() const;
};

}}

#endif // BOOST_FIBERS_WAKER_H

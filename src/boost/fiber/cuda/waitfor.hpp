
//          Copyright Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_CUDA_WAITFOR_H
#define BOOST_FIBERS_CUDA_WAITFOR_H

#include <initializer_list>
#include <mutex>
#include <iostream>
#include <set>
#include <tuple>
#include <vector>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <cuda.h>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/is_all_same.hpp>
#include <boost/fiber/condition_variable.hpp>
#include <boost/fiber/mutex.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace cuda {
namespace detail {

template< typename Rendezvous >
static void trampoline( cudaStream_t st, cudaError_t status, void * vp) {
    Rendezvous * data = static_cast< Rendezvous * >( vp);
    data->notify( st, status);
}

class single_stream_rendezvous {
public:
    single_stream_rendezvous( cudaStream_t st) {
        unsigned int flags = 0;
        cudaError_t status = ::cudaStreamAddCallback( st, trampoline< single_stream_rendezvous >, this, flags);
        if ( cudaSuccess != status) {
            st_ = st;
            status_ = status;
            done_ = true;
        }
    }

    void notify( cudaStream_t st, cudaError_t status) noexcept {
        std::unique_lock< mutex > lk{ mtx_ };
        st_ = st;
        status_ = status;
        done_ = true;
        lk.unlock();
        cv_.notify_one();
    }

    std::tuple< cudaStream_t, cudaError_t > wait() {
        std::unique_lock< mutex > lk{ mtx_ };
        cv_.wait( lk, [this]{ return done_; });
        return std::make_tuple( st_, status_);
    }

private:
    mutex               mtx_{};
    condition_variable  cv_{};
    cudaStream_t        st_{};
    cudaError_t         status_{ cudaErrorUnknown };
    bool                done_{ false };
};

class many_streams_rendezvous {
public:
    many_streams_rendezvous( std::initializer_list< cudaStream_t > l) :
            stx_{ l } {
        results_.reserve( stx_.size() );
        for ( cudaStream_t st : stx_) {
            unsigned int flags = 0;
            cudaError_t status = ::cudaStreamAddCallback( st, trampoline< many_streams_rendezvous >, this, flags);
            if ( cudaSuccess != status) {
                std::unique_lock< mutex > lk{ mtx_ };
                stx_.erase( st);
                results_.push_back( std::make_tuple( st, status) );
            }
        }
    }

    void notify( cudaStream_t st, cudaError_t status) noexcept {
        std::unique_lock< mutex > lk{ mtx_ };
        stx_.erase( st);
        results_.push_back( std::make_tuple( st, status) );
        if ( stx_.empty() ) {
            lk.unlock();
            cv_.notify_one();
        }
    }

    std::vector< std::tuple< cudaStream_t, cudaError_t > > wait() {
        std::unique_lock< mutex > lk{ mtx_ };
        cv_.wait( lk, [this]{ return stx_.empty(); });
        return results_;
    }

private:
    mutex                                                   mtx_{};
    condition_variable                                      cv_{};
    std::set< cudaStream_t >                                stx_;
    std::vector< std::tuple< cudaStream_t, cudaError_t > >  results_;
};

}

void waitfor_all();

inline
std::tuple< cudaStream_t, cudaError_t > waitfor_all( cudaStream_t st) {
    detail::single_stream_rendezvous rendezvous( st);
    return rendezvous.wait();
}

template< typename ... STP >
std::vector< std::tuple< cudaStream_t, cudaError_t > > waitfor_all( cudaStream_t st0, STP ... stx) {
    static_assert( boost::fibers::detail::is_all_same< cudaStream_t, STP ...>::value, "all arguments must be of type `CUstream*`.");
    detail::many_streams_rendezvous rendezvous{ st0, stx ... };
    return rendezvous.wait();
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_CUDA_WAITFOR_H

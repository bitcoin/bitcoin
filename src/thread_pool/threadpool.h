/////////////////////////////////////////////////////////////////////
//          Copyright Yibo Zhu 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
/////////////////////////////////////////////////////////////////////
#pragma once
#include "queue.h"
#include <atomic>
#include <functional>
#include <future>
#include <iterator>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
namespace async {
// thread pool to execute functions, functors, lamdas asynchronously,
// default poolsize = machine's logical CPU cores/threads
class threadpool final {
public:
  static int defaultpoolsize() { return std::max(2u, std::thread::hardware_concurrency()-1); }

  threadpool(int poolsize = static_cast<int>(defaultpoolsize()))
      : idlecount(0), conflag(false) {
    configurepool(poolsize);
  }

  threadpool(const threadpool &) = delete;
  threadpool(threadpool &&) = delete;
  threadpool &operator=(const threadpool &) = delete;
  threadpool &operator=(threadpool &&) = delete;

  ~threadpool() { cleanup(); }

  inline size_t size() {
    std::lock_guard<std::mutex> lg(poolmux);
    return threads.size();
  }

  inline int idlesize() { return idlecount; }

  // can be called to resize the pool at any time after construction and before
  // destruction, recommand to be called from main thread or manager thread even
  // though it is thread-safe
  void configurepool(size_t poolsize) {
    std::unique_lock<std::mutex> veclk(poolmux);
    auto currentsize = threads.size();
    if (currentsize < poolsize) { // expand the pool
      for (auto const &v : std::vector<bool>(poolsize - currentsize)) {
        tpstops.emplace_back(addthread());
      }
    } else if (currentsize > poolsize) { // shrink the pool
      std::vector<std::unique_ptr<std::thread>> dumpthreads;
      std::vector<std::atomic<bool> *> dumpthreadstops;
      std::move(threads.begin() + poolsize, threads.end(),
                std::back_inserter(dumpthreads));
      std::move(tpstops.begin() + poolsize, tpstops.end(),
                std::back_inserter(dumpthreadstops));
      tpstops.resize(poolsize);
      threads.resize(poolsize);
      veclk.unlock();
      for (auto &a : dumpthreadstops) {
        *a = true;
      }
      for (auto &t : dumpthreads) {
        t->detach();
      }
      {
        std::unique_lock<std::mutex> lk(qcvmux); // suspended threads to quit
        qcv.notify_all();
      }
    }
  }

  template <typename Func, typename... Args>
  inline auto post(Func &&func, Args &&... args)
#if ((defined(__clang__) || defined(__GNUC__)) && __cplusplus <= 201103L) ||   \
    (defined(_MSC_VER) && _MSC_VER <= 1800)
      -> std::future<typename std::result_of<Func(Args...)>::type>
#endif
  { // TODO: replace result_of with invoke_result_t when migrate to c++17
    auto taskptr = std::make_shared<
        std::packaged_task<typename std::result_of<Func(Args...)>::type()>>(
        std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
    taskqueue.enqueue([taskptr]() { (*taskptr)(); });
    {
      std::lock_guard<std::mutex> lg(qcvmux);
      conflag = true;
    }
    qcv.notify_one();
    return taskptr->get_future();
  }

  template <typename Func>
  inline auto post(Func &&func)
#if ((defined(__clang__) || defined(__GNUC__)) && __cplusplus <= 201103L) ||   \
    (defined(_MSC_VER) && _MSC_VER <= 1800)
      -> std::future<typename std::result_of<Func()>::type>
#endif
  { // a special case for func() type without any parameters, might be
    // removed later
    auto taskptr = std::make_shared<
        std::packaged_task<typename std::result_of<Func()>::type()>>(
        std::forward<Func>(func));
    taskqueue.enqueue([taskptr]() { (*taskptr)(); });
    {
      std::lock_guard<std::mutex> lg(qcvmux);
      conflag = true;
    }
    qcv.notify_one();
    return taskptr->get_future();
  }

private:
  struct executor {
    executor(std::unique_ptr<std::atomic<bool>> &&ptr, threadpool &pool)
        : stop(std::move(ptr)), thpool(pool) {}
    void operator()() {
      while (!*stop) {
        if (!thpool.executetask_in_loop(*stop)) {
          return; // signaled to quit
        }
        thpool.wait_for_task(*stop); // wait for new task
      }
    }

  private:
    std::unique_ptr<std::atomic<bool>> stop;
    threadpool &thpool;
  };

  std::atomic<bool> *addthread() {
    auto stopuniptr = std::make_unique<std::atomic<bool>>(false);
    auto stoprawptr = stopuniptr.get();
    threads.emplace_back(
        std::make_unique<std::thread>(executor(std::move(stopuniptr), *this)));
    return stoprawptr;
  }

  void cleanup() { // make sure no more tasks being pushed to the taskqueue
    {
      std::lock_guard<std::mutex> lk(qcvmux);
      qcv.notify_all(); // let running thread drain the task queue? no need,
                        // should be removed
    }
    for (auto &stop : tpstops) {
      *stop = true; // stop signaled
    }
    {
      std::lock_guard<std::mutex> lk(qcvmux);
      qcv.notify_all(); // notify again
    }
    for (auto &thread : threads) {
      if (thread->joinable())
        thread->join();
    }
    threads.clear();
    tpstops.clear();
  }

  inline void wait_for_task(std::atomic<bool> const &stop) {
    idlecount.fetch_add(1, std::memory_order_relaxed);
    {
      std::unique_lock<std::mutex> lk(qcvmux);
      qcv.wait(lk, [&]() {
        return conflag || stop.load(std::memory_order_acquire);
      }); //memory_oder can be removed
      conflag = false;
    }
    idlecount.fetch_sub(1, std::memory_order_relaxed);
  }

  inline bool executetask_in_loop(std::atomic<bool> const &stop) {
    std::function<void()> func;
    for (; taskqueue.dequeue(func);) {
      func();
      if (stop) // stop is signaled
        return false;
    }
    return true;
  }

  std::vector<std::unique_ptr<std::thread>> threads;
  std::vector<std::atomic<bool> *> tpstops; // threads terminate flags
  async::queue<std::function<void()>> taskqueue;
  std::atomic<int> idlecount; // idle thread count
  std::mutex qcvmux, poolmux;
  std::condition_variable qcv;
  bool conflag; // continue flag for cv
};
} // namespace async
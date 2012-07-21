// LevelDB Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// See port_example.h for documentation for the following types/functions.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of the University of California, Berkeley nor the
//    names of its contributors may be used to endorse or promote products
//    derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include "port/port_win.h"

#include <windows.h>
#include <cassert>

namespace leveldb {
namespace port {

Mutex::Mutex() :
    mutex_(::CreateMutex(NULL, FALSE, NULL)) {
  assert(mutex_);
}

Mutex::~Mutex() {
  assert(mutex_);
  ::CloseHandle(mutex_);
}

void Mutex::Lock() {
  assert(mutex_);
  ::WaitForSingleObject(mutex_, INFINITE);
}

void Mutex::Unlock() {
  assert(mutex_);
  ::ReleaseMutex(mutex_);
}

void Mutex::AssertHeld() {
  assert(mutex_);
  assert(1);
}

CondVar::CondVar(Mutex* mu) :
    waiting_(0), 
    mu_(mu), 
    sema_(::CreateSemaphore(NULL, 0, 0x7fffffff, NULL)), 
    event_(::CreateEvent(NULL, FALSE, FALSE, NULL)),
    broadcasted_(false){
  assert(mu_);
}

CondVar::~CondVar() {
  ::CloseHandle(sema_);
  ::CloseHandle(event_);
}

void CondVar::Wait() {
  wait_mtx_.Lock();
  ++waiting_;
  assert(waiting_ > 0);
  wait_mtx_.Unlock();

  ::SignalObjectAndWait(mu_->mutex_, sema_, INFINITE, FALSE);

  wait_mtx_.Lock();
  bool last = broadcasted_ && (--waiting_ == 0);
  assert(waiting_ >= 0);
  wait_mtx_.Unlock();

  // we leave this function with the mutex held
  if (last)
  {
    ::SignalObjectAndWait(event_, mu_->mutex_, INFINITE, FALSE);
  }
  else
  {
    ::WaitForSingleObject(mu_->mutex_, INFINITE);
  }
}

void CondVar::Signal() {
  wait_mtx_.Lock();
  bool waiters = waiting_ > 0;
  wait_mtx_.Unlock();

  if (waiters)
  {
    ::ReleaseSemaphore(sema_, 1, 0);
  }
}

void CondVar::SignalAll() {
  wait_mtx_.Lock();

  broadcasted_ = (waiting_ > 0);

  if (broadcasted_)
  {
      // release all
    ::ReleaseSemaphore(sema_, waiting_, 0);
    wait_mtx_.Unlock();
    ::WaitForSingleObject(event_, INFINITE);
    broadcasted_ = false;
  }
  else
  {
    wait_mtx_.Unlock();
  }
}

AtomicPointer::AtomicPointer(void* v) {
  Release_Store(v);
}

void* AtomicPointer::Acquire_Load() const {
  void * p = nullptr;
  InterlockedExchangePointer(&p, rep_);
  return p;
}

void AtomicPointer::Release_Store(void* v) {
  InterlockedExchangePointer(&rep_, v);
}

void* AtomicPointer::NoBarrier_Load() const {
  return rep_;
}

void AtomicPointer::NoBarrier_Store(void* v) {
  rep_ = v;
}

enum InitializationState
{
    Uninitialized = 0,
    Running = 1,
    Initialized = 2
};

void InitOnce(OnceType* once, void (*initializer)()) {

  static_assert(Uninitialized == LEVELDB_ONCE_INIT, "Invalid uninitialized state value");

  InitializationState state = static_cast<InitializationState>(InterlockedCompareExchange(once, Running, Uninitialized));

  if (state == Uninitialized) {
      initializer();
      *once = Initialized;
  }

  if (state == Running) {
      while(*once != Initialized) {
          Sleep(0); // yield
      }
  }

  assert(*once == Initialized);
}

}
}

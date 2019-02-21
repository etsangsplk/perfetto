/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "src/profiling/memory/scoped_spinlock.h"

#include <unistd.h>

#include <atomic>
#include <new>
#include <utility>

#include "perfetto/base/utils.h"

namespace perfetto {
namespace profiling {

void ScopedSpinlock::LockSlow(Mode mode) {
  // Slowpath.
  for (size_t attempt = 0; mode == Mode::Blocking || attempt < 1024 * 10;
       attempt++) {
    if (!lock_->load(std::memory_order_relaxed) &&
        PERFETTO_LIKELY(!lock_->exchange(true, std::memory_order_acquire))) {
      locked_ = true;
      return;
    }
    if (attempt && attempt % 1024 == 0)
      usleep(1000);
  }
}

ScopedSpinlock::ScopedSpinlock(ScopedSpinlock&& other) noexcept
    : lock_(other.lock_), locked_(other.locked_) {
  other.locked_ = false;
}

ScopedSpinlock& ScopedSpinlock::operator=(ScopedSpinlock&& other) {
  if (this != &other) {
    this->~ScopedSpinlock();
    new (this) ScopedSpinlock(std::move(other));
  }
  return *this;
}

ScopedSpinlock::~ScopedSpinlock() {
  Unlock();
}

}  // namespace profiling
}  // namespace perfetto

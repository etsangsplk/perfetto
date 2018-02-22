/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef INCLUDE_PERFETTO_BASE_WATCHDOG_H_
#define INCLUDE_PERFETTO_BASE_WATCHDOG_H_

#include "perfetto/base/thread_checker.h"

#include <mutex>
#include <thread>

namespace perfetto {
namespace base {

// Ensures that the calling program does not exceed certain hard limits on
// resource usage e.g. time, memory and CPU. If exceeded, the program is
// crashed.
class Watchdog {
 public:
  // Possible reasons for setting a timer limit.
  enum TimerReason {
    kTaskDeadline = 0,
    kTraceDeadline = 1,
    kMax = kTraceDeadline + 1,
  };

  // Handle to the timer set to crash the program. If the handle is dropped,
  // the timer is removed so the program does not crash.
  class TimerHandle {
   public:
    TimerHandle(TimerReason reason);
    ~TimerHandle();
    TimerHandle(TimerHandle&& other) noexcept;

   private:
    TimerHandle(const TimerHandle&) = delete;
    TimerHandle& operator=(const TimerHandle&) = delete;

    TimerReason reason_;
  };

  static Watchdog* GetInstance();

  // Sets a timer which will crash the program in |ms| milliseconds if the
  // returned handle is not destroyed.
  // Note: only one timer with each reason can exist at any one time.
  // Note: |ms| has to be a multiple of |polling_interval_ms_|.
  TimerHandle CreateFatalTimer(uint32_t ms, TimerReason reason);

  // Sets a limit on the memory (defined as the RSS) used by the program
  // averaged over the last |window_ms| milliseconds. If |kb| is 0, any
  // existing limit is removed.
  // Note: |window_ms| has to be a multiple of |polling_interval_ms_|.
  void SetMemoryLimit(uint32_t kb, uint32_t window_ms);

  // Sets a limit on the CPU usage used by the program averaged over the last
  // |window_ms| milliseconds. If |percentage| is 0, any existing limit is
  // removed.
  // Note: |window_ms| has to be a multiple of |polling_interval_ms_|.
  void SetCpuLimit(uint32_t percentage, uint32_t window_ms);

 private:
  // Represents a ring buffer in which integer values can be stored.
  class WindowedInterval {
   public:
    // Pushes a new value into a ring buffer wrapping if necessary and returns
    // whether the ring buffer is full.
    bool Push(uint64_t sample);

    // Returns the mean of the values in the buffer.
    uint64_t Mean() const;

    // Clears the ring buffer while keeping the existing size.
    void Clear();

    // Resets the size of the buffer as well as clearing it.
    void Reset(size_t new_size);

    // Gets the oldest value inserted in the buffer. The buffer must be full
    // (i.e. Push returned true) before this method can be called.
    uint64_t OldestWhenFull() const {
      PERFETTO_CHECK(filled_);
      return buffer_[position_];
    }

    // Gets the newest value inserted in the buffer. The buffer must be full
    // (i.e. Push returned true) before this method can be called.
    uint64_t NewestWhenFull() const {
      PERFETTO_CHECK(filled_);
      return buffer_[(position_ + size_ - 1) % size_];
    }

    // Returns the size of the ring buffer.
    size_t size() const { return size_; }

   private:
    bool filled_ = false;
    size_t position_ = 0;
    size_t size_ = 0;
    std::unique_ptr<uint64_t[]> buffer_;
  };

  Watchdog(uint32_t polling_interval_ms);
  ~Watchdog() = default;

  // Main method for the watchdog thread.
  [[noreturn]] void ThreadMain();

  // Check each type of resource every |polling_interval_ms_| miillis.
  void CheckMemory(uint64_t rss_kb);
  void CheckCpu(uint64_t cpu_time);
  void CheckTimers();

  // Clears the timer with the given reason.
  void ClearTimer(TimerReason reason);

  // Computes the time interval spanned by a given ring buffer with respect
  // to |polling_interval_ms_|.
  uint32_t WindowTimeForRingBuffer(const WindowedInterval& window);

  std::thread thread_;

  // --- Begin lock-protected members ---

  std::mutex mutex_;

  uint32_t memory_limit_kb_ = 0;
  WindowedInterval memory_window_kb_;

  uint32_t cpu_limit_percentage_ = 0;
  WindowedInterval cpu_window_time_;

  const uint32_t polling_interval_ms_;
  int32_t timer_window_countdown_[TimerReason::kMax];

  // --- End lock-protected members ---
};

}  // namespace base
}  // namespace perfetto
#endif  // INCLUDE_PERFETTO_BASE_WATCHDOG_H_

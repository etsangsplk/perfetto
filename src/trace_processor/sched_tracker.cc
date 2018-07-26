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

#include "src/trace_processor/sched_tracker.h"

namespace perfetto {
namespace trace_processor {

SchedTracker::SchedTracker(TraceProcessorContext* context)
    : context_(context){};

void SchedTracker::PushSchedSwitch(uint32_t cpu,
                                   uint64_t timestamp,
                                   uint32_t prev_pid,
                                   uint32_t prev_state,
                                   const char* prev_comm,
                                   size_t prev_comm_len,
                                   uint32_t next_pid) {
  SchedSwitchEvent* prev = &last_sched_per_cpu_[cpu];
  // If we had a valid previous event, then inform the storage about the
  // slice.
  if (prev->valid() && prev->next_pid != 0 /* Idle process (swapper/N) */) {
    uint64_t duration = timestamp - prev->timestamp;

    TraceStorage::UniqueTid utid = context_->process_tracker->UpdateThread(
        prev->timestamp, prev->prev_pid, prev->prev_thread_name_id);
    context_->storage->AddSliceToCpu(prev->cpu, prev->timestamp, duration,
                                     utid);
  }

  // If the this events previous pid does not match the previous event's next
  // pid, make a note of this.
  if (prev_pid != prev->next_pid) {
    context_->storage->AddMismatchedSchedSwitch(1);
  }

  // Update the map with the current event.
  prev->cpu = cpu;
  prev->timestamp = timestamp;
  prev->prev_pid = prev_pid;
  prev->prev_state = prev_state;
  prev->prev_thread_name_id =
      context_->storage->InternString(prev_comm, prev_comm_len);
  prev->next_pid = next_pid;
};

}  // namespace trace_processor
}  // namespace perfetto

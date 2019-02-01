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

#include "src/trace_processor/event_tracker.h"

#include <math.h>

#include "perfetto/base/utils.h"
#include "src/trace_processor/args_tracker.h"
#include "src/trace_processor/ftrace_descriptors.h"
#include "src/trace_processor/ftrace_utils.h"
#include "src/trace_processor/process_tracker.h"
#include "src/trace_processor/stats.h"
#include "src/trace_processor/trace_processor_context.h"

#include "perfetto/trace/ftrace/ftrace_event.pb.h"

namespace perfetto {
namespace trace_processor {

EventTracker::EventTracker(TraceProcessorContext* context) : context_(context) {
  auto* descriptor =
      GetMessageDescriptorForId(protos::FtraceEvent::kSchedSwitchFieldNumber);
  PERFETTO_CHECK(descriptor->max_field_id == kSchedSwitchMaxFieldId);

  for (size_t i = 1; i <= kSchedSwitchMaxFieldId; i++) {
    sched_switch_field_ids_[i] =
        context->storage->InternString(descriptor->fields[i].name);
  }
  sched_switch_id_ = context->storage->InternString(descriptor->name);
}

EventTracker::~EventTracker() = default;

void EventTracker::PushSchedSwitch(uint32_t cpu,
                                   int64_t ts,
                                   uint32_t prev_pid,
                                   base::StringView prev_comm,
                                   int32_t prev_prio,
                                   int64_t prev_state,
                                   uint32_t next_pid,
                                   base::StringView next_comm,
                                   int32_t next_prio) {
  // At this stage all events should be globally timestamp ordered.
  if (ts < prev_timestamp_) {
    PERFETTO_ELOG("sched_switch event out of order by %.4f ms, skipping",
                  (prev_timestamp_ - ts) / 1e6);
    context_->storage->IncrementStats(stats::sched_switch_out_of_order);
    return;
  }
  prev_timestamp_ = ts;
  PERFETTO_DCHECK(cpu < base::kMaxCpus);

  auto* slices = context_->storage->mutable_slices();
  auto* prev_slice = &pending_sched_per_cpu_[cpu];

  StringId next_comm_id = context_->storage->InternString(next_comm);
  auto next_utid =
      context_->process_tracker->UpdateThread(ts, next_pid, next_comm_id);

  // First add the slice for the "next" data.
  auto next_idx = slices->AddSlice(cpu, ts, 0 /* duration */, next_utid,
                                   ftrace_utils::TaskState(), next_prio);

  // Now use this event to update any data on the existing slice.
  bool reuse_slice_data = false;
  size_t slice_idx = prev_slice->storage_index;
  if (slice_idx < std::numeric_limits<size_t>::max()) {
    int64_t duration = ts - slices->start_ns()[slice_idx];
    slices->set_duration(slice_idx, duration);

    if (prev_pid == prev_slice->next_pid) {
      // We store the state as a uint16 as we only consider values up to 2048
      // when unpacking the information inside; this allows savings of 48 bits
      // per slice.
      slices->set_end_state(slice_idx, ftrace_utils::TaskState(
                                           static_cast<uint16_t>(prev_state)));

      // We can reuse the slice's data because the pids match.
      reuse_slice_data = true;
    } else {
      // If the this events previous pid does not match the previous event's
      // next pid, make a note of this.
      context_->storage->IncrementStats(stats::mismatched_sched_switch_tids);
    }
  }

  // Compute the prev_utid and prev_comm_id in an optimized manner if possible.
  UniqueTid prev_utid;
  StringId prev_comm_id;
  if (reuse_slice_data) {
    PERFETTO_DCHECK(slice_idx < std::numeric_limits<size_t>::max());
    prev_comm_id = prev_slice->next_comm_id;
    prev_utid = slices->utids()[slice_idx];
  } else {
    prev_comm_id = context_->storage->InternString(prev_comm);
    prev_utid =
        context_->process_tracker->UpdateThread(ts, prev_pid, prev_comm_id);
  }

  // Push the raw event - this is done as the raw ftrace event codepath does
  // not insert sched_switch.
  auto rid = context_->storage->mutable_raw_events()->AddRawEvent(
      ts, sched_switch_id_, cpu, prev_utid);

  // Note: this ordering is important. The events should be pushed in the same
  // order as the order of fields in the proto.
  using Variadic = TraceStorage::Args::Variadic;
  using SS = protos::SchedSwitchFtraceEvent;
  AddSchedRawArg(rid, SS::kPrevCommFieldNumber, Variadic::String(prev_comm_id));
  AddSchedRawArg(rid, SS::kPrevPidFieldNumber, Variadic::Integer(prev_pid));
  AddSchedRawArg(rid, SS::kPrevPrioFieldNumber, Variadic::Integer(prev_prio));
  AddSchedRawArg(rid, SS::kPrevStateFieldNumber, Variadic::Integer(prev_state));
  AddSchedRawArg(rid, SS::kNextCommFieldNumber, Variadic::String(next_comm_id));
  AddSchedRawArg(rid, SS::kNextPidFieldNumber, Variadic::Integer(next_pid));
  AddSchedRawArg(rid, SS::kNextPrioFieldNumber, Variadic::Integer(next_prio));

  // Finally, update the info for the next sched switch on this CPU.
  prev_slice->storage_index = next_idx;
  prev_slice->next_pid = next_pid;
  prev_slice->next_comm_id = next_comm_id;
}

void EventTracker::AddSchedRawArg(RowId row_id,
                                  int field_num,
                                  TraceStorage::Args::Variadic var) {
  StringId key = sched_switch_field_ids_[static_cast<size_t>(field_num)];
  context_->args_tracker->AddArg(row_id, key, key, var);
}

RowId EventTracker::PushCounter(int64_t timestamp,
                                double value,
                                StringId name_id,
                                int64_t ref,
                                RefType ref_type) {
  if (timestamp < prev_timestamp_) {
    PERFETTO_ELOG("counter event out of order by %.4f ms, skipping",
                  (prev_timestamp_ - timestamp) / 1e6);
    context_->storage->IncrementStats(stats::counter_events_out_of_order);
    return kInvalidRowId;
  }
  prev_timestamp_ = timestamp;

  auto* counters = context_->storage->mutable_counters();
  const auto& key = CounterKey{ref, name_id};
  auto counter_it = pending_counters_per_key_.find(key);
  if (counter_it != pending_counters_per_key_.end()) {
    size_t idx = counter_it->second;
    int64_t duration = timestamp - counters->timestamps()[idx];
    // Update duration of previously stored event.
    counters->set_duration(idx, duration);
  }

  // At this point we don't know the duration so just store 0.
  size_t idx = counters->AddCounter(timestamp, 0 /* duration */, name_id, value,
                                    ref, ref_type);
  pending_counters_per_key_[key] = idx;
  return TraceStorage::CreateRowId(TableId::kCounters,
                                   static_cast<uint32_t>(idx));
}

}  // namespace trace_processor
}  // namespace perfetto

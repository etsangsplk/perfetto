/*
 * Copyright (C) 2017 The Android Open Source Project
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

/*******************************************************************************
 * AUTOGENERATED - DO NOT EDIT
 *******************************************************************************
 * This file has been generated from the protobuf message
 * perfetto/config/process_stats/process_stats_config.proto
 * by
 * ../../tools/proto_to_cpp/proto_to_cpp.cc.
 * If you need to make changes here, change the .proto file and then run
 * ./tools/gen_tracing_cpp_headers_from_protos
 */

#include "perfetto/tracing/core/process_stats_config.h"

#include "perfetto/config/process_stats/process_stats_config.pb.h"

namespace perfetto {

ProcessStatsConfig::ProcessStatsConfig() = default;
ProcessStatsConfig::~ProcessStatsConfig() = default;
ProcessStatsConfig::ProcessStatsConfig(const ProcessStatsConfig&) = default;
ProcessStatsConfig& ProcessStatsConfig::operator=(const ProcessStatsConfig&) =
    default;
ProcessStatsConfig::ProcessStatsConfig(ProcessStatsConfig&&) noexcept = default;
ProcessStatsConfig& ProcessStatsConfig::operator=(ProcessStatsConfig&&) =
    default;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
bool ProcessStatsConfig::operator==(const ProcessStatsConfig& other) const {
  return (quirks_ == other.quirks_) &&
         (scan_all_processes_on_start_ == other.scan_all_processes_on_start_) &&
         (record_thread_names_ == other.record_thread_names_) &&
         (proc_stats_poll_ms_ == other.proc_stats_poll_ms_);
}
#pragma GCC diagnostic pop

void ProcessStatsConfig::FromProto(
    const perfetto::protos::ProcessStatsConfig& proto) {
  quirks_.clear();
  for (const auto& field : proto.quirks()) {
    quirks_.emplace_back();
    static_assert(sizeof(quirks_.back()) == sizeof(proto.quirks(0)),
                  "size mismatch");
    quirks_.back() = static_cast<decltype(quirks_)::value_type>(field);
  }

  static_assert(sizeof(scan_all_processes_on_start_) ==
                    sizeof(proto.scan_all_processes_on_start()),
                "size mismatch");
  scan_all_processes_on_start_ =
      static_cast<decltype(scan_all_processes_on_start_)>(
          proto.scan_all_processes_on_start());

  static_assert(
      sizeof(record_thread_names_) == sizeof(proto.record_thread_names()),
      "size mismatch");
  record_thread_names_ =
      static_cast<decltype(record_thread_names_)>(proto.record_thread_names());

  static_assert(
      sizeof(proc_stats_poll_ms_) == sizeof(proto.proc_stats_poll_ms()),
      "size mismatch");
  proc_stats_poll_ms_ =
      static_cast<decltype(proc_stats_poll_ms_)>(proto.proc_stats_poll_ms());
  unknown_fields_ = proto.unknown_fields();
}

void ProcessStatsConfig::ToProto(
    perfetto::protos::ProcessStatsConfig* proto) const {
  proto->Clear();

  for (const auto& it : quirks_) {
    proto->add_quirks(static_cast<decltype(proto->quirks(0))>(it));
    static_assert(sizeof(it) == sizeof(proto->quirks(0)), "size mismatch");
  }

  static_assert(sizeof(scan_all_processes_on_start_) ==
                    sizeof(proto->scan_all_processes_on_start()),
                "size mismatch");
  proto->set_scan_all_processes_on_start(
      static_cast<decltype(proto->scan_all_processes_on_start())>(
          scan_all_processes_on_start_));

  static_assert(
      sizeof(record_thread_names_) == sizeof(proto->record_thread_names()),
      "size mismatch");
  proto->set_record_thread_names(
      static_cast<decltype(proto->record_thread_names())>(
          record_thread_names_));

  static_assert(
      sizeof(proc_stats_poll_ms_) == sizeof(proto->proc_stats_poll_ms()),
      "size mismatch");
  proto->set_proc_stats_poll_ms(
      static_cast<decltype(proto->proc_stats_poll_ms())>(proc_stats_poll_ms_));
  *(proto->mutable_unknown_fields()) = unknown_fields_;
}

}  // namespace perfetto

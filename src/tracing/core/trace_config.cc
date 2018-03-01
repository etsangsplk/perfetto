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
 * perfetto/config/trace_config.proto
 * by
 * ../../tools/proto_to_cpp/proto_to_cpp.cc.
 * If you need to make changes here, change the .proto file and then run
 * ./tools/gen_tracing_cpp_headers_from_protos.py
 */

#include "perfetto/tracing/core/trace_config.h"

#include "perfetto/config/data_source_config.pb.h"
#include "perfetto/config/trace_config.pb.h"

namespace perfetto {

TraceConfig::TraceConfig() = default;
TraceConfig::~TraceConfig() = default;
TraceConfig::TraceConfig(const TraceConfig&) = default;
TraceConfig& TraceConfig::operator=(const TraceConfig&) = default;
TraceConfig::TraceConfig(TraceConfig&&) noexcept = default;
TraceConfig& TraceConfig::operator=(TraceConfig&&) = default;

void TraceConfig::FromProto(const perfetto::protos::TraceConfig& proto) {
  buffers_.clear();
  for (const auto& field : proto.buffers()) {
    buffers_.emplace_back();
    buffers_.back().FromProto(field);
  }

  data_sources_.clear();
  for (const auto& field : proto.data_sources()) {
    data_sources_.emplace_back();
    data_sources_.back().FromProto(field);
  }

  static_assert(sizeof(duration_ms_) == sizeof(proto.duration_ms()),
                "size mismatch");
  duration_ms_ = static_cast<decltype(duration_ms_)>(proto.duration_ms());

  static_assert(sizeof(enable_extra_guardrails_) ==
                    sizeof(proto.enable_extra_guardrails()),
                "size mismatch");
  enable_extra_guardrails_ = static_cast<decltype(enable_extra_guardrails_)>(
      proto.enable_extra_guardrails());

  static_assert(sizeof(max_shm_size_) == sizeof(proto.max_shm_size()),
                "size mismatch");
  max_shm_size_ = static_cast<decltype(max_shm_size_)>(proto.max_shm_size());

  static_assert(
      sizeof(buffer_drain_interval_) == sizeof(proto.buffer_drain_interval()),
      "size mismatch");
  buffer_drain_interval_ = static_cast<decltype(buffer_drain_interval_)>(
      proto.buffer_drain_interval());

  static_assert(sizeof(page_size_) == sizeof(proto.page_size()),
                "size mismatch");
  page_size_ = static_cast<decltype(page_size_)>(proto.page_size());
  unknown_fields_ = proto.unknown_fields();
}

void TraceConfig::ToProto(perfetto::protos::TraceConfig* proto) const {
  proto->Clear();

  for (const auto& it : buffers_) {
    auto* entry = proto->add_buffers();
    it.ToProto(entry);
  }

  for (const auto& it : data_sources_) {
    auto* entry = proto->add_data_sources();
    it.ToProto(entry);
  }

  static_assert(sizeof(duration_ms_) == sizeof(proto->duration_ms()),
                "size mismatch");
  proto->set_duration_ms(
      static_cast<decltype(proto->duration_ms())>(duration_ms_));

  static_assert(sizeof(enable_extra_guardrails_) ==
                    sizeof(proto->enable_extra_guardrails()),
                "size mismatch");
  proto->set_enable_extra_guardrails(
      static_cast<decltype(proto->enable_extra_guardrails())>(
          enable_extra_guardrails_));

  static_assert(sizeof(max_shm_size_) == sizeof(proto->max_shm_size()),
                "size mismatch");
  proto->set_max_shm_size(
      static_cast<decltype(proto->max_shm_size())>(max_shm_size_));

  static_assert(
      sizeof(buffer_drain_interval_) == sizeof(proto->buffer_drain_interval()),
      "size mismatch");
  proto->set_buffer_drain_interval(
      static_cast<decltype(proto->buffer_drain_interval())>(
          buffer_drain_interval_));

  static_assert(sizeof(page_size_) == sizeof(proto->page_size()),
                "size mismatch");
  proto->set_page_size(static_cast<decltype(proto->page_size())>(page_size_));
  *(proto->mutable_unknown_fields()) = unknown_fields_;
}

TraceConfig::BufferConfig::BufferConfig() = default;
TraceConfig::BufferConfig::~BufferConfig() = default;
TraceConfig::BufferConfig::BufferConfig(const TraceConfig::BufferConfig&) =
    default;
TraceConfig::BufferConfig& TraceConfig::BufferConfig::operator=(
    const TraceConfig::BufferConfig&) = default;
TraceConfig::BufferConfig::BufferConfig(TraceConfig::BufferConfig&&) noexcept =
    default;
TraceConfig::BufferConfig& TraceConfig::BufferConfig::operator=(
    TraceConfig::BufferConfig&&) = default;

void TraceConfig::BufferConfig::FromProto(
    const perfetto::protos::TraceConfig_BufferConfig& proto) {
  static_assert(sizeof(size_kb_) == sizeof(proto.size_kb()), "size mismatch");
  size_kb_ = static_cast<decltype(size_kb_)>(proto.size_kb());

  static_assert(sizeof(optimize_for_) == sizeof(proto.optimize_for()),
                "size mismatch");
  optimize_for_ = static_cast<decltype(optimize_for_)>(proto.optimize_for());

  static_assert(sizeof(fill_policy_) == sizeof(proto.fill_policy()),
                "size mismatch");
  fill_policy_ = static_cast<decltype(fill_policy_)>(proto.fill_policy());
  unknown_fields_ = proto.unknown_fields();
}

void TraceConfig::BufferConfig::ToProto(
    perfetto::protos::TraceConfig_BufferConfig* proto) const {
  proto->Clear();

  static_assert(sizeof(size_kb_) == sizeof(proto->size_kb()), "size mismatch");
  proto->set_size_kb(static_cast<decltype(proto->size_kb())>(size_kb_));

  static_assert(sizeof(optimize_for_) == sizeof(proto->optimize_for()),
                "size mismatch");
  proto->set_optimize_for(
      static_cast<decltype(proto->optimize_for())>(optimize_for_));

  static_assert(sizeof(fill_policy_) == sizeof(proto->fill_policy()),
                "size mismatch");
  proto->set_fill_policy(
      static_cast<decltype(proto->fill_policy())>(fill_policy_));
  *(proto->mutable_unknown_fields()) = unknown_fields_;
}

TraceConfig::DataSource::DataSource() = default;
TraceConfig::DataSource::~DataSource() = default;
TraceConfig::DataSource::DataSource(const TraceConfig::DataSource&) = default;
TraceConfig::DataSource& TraceConfig::DataSource::operator=(
    const TraceConfig::DataSource&) = default;
TraceConfig::DataSource::DataSource(TraceConfig::DataSource&&) noexcept =
    default;
TraceConfig::DataSource& TraceConfig::DataSource::operator=(
    TraceConfig::DataSource&&) = default;

void TraceConfig::DataSource::FromProto(
    const perfetto::protos::TraceConfig_DataSource& proto) {
  config_.FromProto(proto.config());

  producer_name_filter_.clear();
  for (const auto& field : proto.producer_name_filter()) {
    producer_name_filter_.emplace_back();
    static_assert(sizeof(producer_name_filter_.back()) ==
                      sizeof(proto.producer_name_filter(0)),
                  "size mismatch");
    producer_name_filter_.back() =
        static_cast<decltype(producer_name_filter_)::value_type>(field);
  }
  unknown_fields_ = proto.unknown_fields();
}

void TraceConfig::DataSource::ToProto(
    perfetto::protos::TraceConfig_DataSource* proto) const {
  proto->Clear();

  config_.ToProto(proto->mutable_config());

  for (const auto& it : producer_name_filter_) {
    auto* entry = proto->add_producer_name_filter();
    static_assert(sizeof(it) == sizeof(proto->producer_name_filter(0)),
                  "size mismatch");
    *entry = static_cast<decltype(proto->producer_name_filter(0))>(it);
  }
  *(proto->mutable_unknown_fields()) = unknown_fields_;
}

}  // namespace perfetto

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

#include "fake_producer.h"

#include "perfetto/base/logging.h"
#include "perfetto/traced/traced.h"
#include "perfetto/tracing/core/trace_config.h"
#include "perfetto/tracing/core/trace_packet.h"
#include "perfetto/tracing/core/trace_writer.h"

#include "protos/test_event.pbzero.h"
#include "protos/trace_packet.pbzero.h"

namespace perfetto {

FakeProducer::FakeProducer(const std::string& name,
                           base::UnixTaskRunner* task_runner)
    : name_(name),
      endpoint_(ProducerIPCClient::Connect(PERFETTO_PRODUCER_SOCK_NAME,
                                           this,
                                           task_runner)),
      task_runner_(task_runner) {}
FakeProducer::~FakeProducer() = default;

void FakeProducer::OnConnect() {
  PERFETTO_ILOG("connected");
  DataSourceDescriptor descriptor;
  descriptor.set_name(name_);
  endpoint_->RegisterDataSource(descriptor,
                                [this](DataSourceID id) { id_ = id; });
}

void FakeProducer::OnDisconnect() {
  PERFETTO_ILOG("Disconnect");
  Shutdown();
}

void FakeProducer::CreateDataSourceInstance(
    DataSourceInstanceID,
    const DataSourceConfig& source_config) {
  PERFETTO_ILOG("Create");
  const std::string& categories = source_config.trace_category_filters();
  if (categories != "foo,bar") {
    Shutdown();
    return;
  }

  PERFETTO_ILOG("Writing");
  auto trace_writer = endpoint_->CreateTraceWriter(
      static_cast<BufferID>(source_config.target_buffer()));
  for (int i = 0; i < 10; i++) {
    auto handle = trace_writer->NewTracePacket();
    handle->set_test("test");
    handle->Finalize();
  }

  // Temporarily create a new packet to flush the final packet to the
  // consumer.
  // TODO(primiano): remove this hack once flushing the final packet is fixed.
  trace_writer->NewTracePacket();

  PERFETTO_ILOG("Finalized");
  endpoint_->UnregisterDataSource(id_);
}

void FakeProducer::TearDownDataSourceInstance(DataSourceInstanceID) {
  PERFETTO_ILOG("Teardown");
  Shutdown();
}

void FakeProducer::Shutdown() {
  endpoint_.reset();
  task_runner_->Quit();
}

}  // namespace perfetto

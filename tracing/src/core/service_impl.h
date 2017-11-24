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

#ifndef TRACING_SRC_CORE_SERVICE_IMPL_H_
#define TRACING_SRC_CORE_SERVICE_IMPL_H_

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>

#include "tracing/core/basic_types.h"
#include "tracing/core/data_source_descriptor.h"
#include "tracing/core/service.h"
#include "tracing/core/shared_memory_abi.h"

namespace perfetto {

namespace base {
class TaskRunner;
}  // namespace base

class DataSourceConfig;
class Producer;
class SharedMemory;

// The tracing service business logic.
class ServiceImpl : public Service {
 public:
  // The implementation behind the service endpoint exposed to each producer.
  class ProducerEndpointImpl : public Service::ProducerEndpoint {
   public:
    ProducerEndpointImpl(ProducerID,
                         ServiceImpl*,
                         base::TaskRunner*,
                         Producer*,
                         std::unique_ptr<SharedMemory>,
                         size_t shared_buffer_page_size_bytes);
    ~ProducerEndpointImpl() override;

    Producer* producer() const { return producer_; }

    // Service::ProducerEndpoint implementation.
    void RegisterDataSource(const DataSourceDescriptor&,
                            RegisterDataSourceCallback) override;
    void UnregisterDataSource(DataSourceID) override;

    void NotifySharedMemoryUpdate(
        const std::vector<uint32_t>& changed_pages) override;

    std::unique_ptr<TraceWriter> CreateTraceWriter(
        uint32_t target_buffer) override;

    SharedMemory* shared_memory() const override;

   private:
    ProducerEndpointImpl(const ProducerEndpointImpl&) = delete;
    ProducerEndpointImpl& operator=(const ProducerEndpointImpl&) = delete;

    ProducerID const id_;
    ServiceImpl* const service_;
    base::TaskRunner* const task_runner_;
    Producer* producer_;
    std::unique_ptr<SharedMemory> shared_memory_;
    SharedMemoryABI shmem_abi_;
    DataSourceID last_data_source_id_ = 0;
  };

  // The implementation behind the service endpoint exposed to each consumer.
  class ConsumerEndpointImpl : public Service::ConsumerEndpoint {
   public:
    ConsumerEndpointImpl(ServiceImpl*, base::TaskRunner*, Consumer*);
    ~ConsumerEndpointImpl() override;

    Consumer* consumer() const { return consumer_; }

    // Service::ConsumerEndpoint implementation.
    void StartTracing(const TraceConfig&) override;
    void StopTracing() override;

   private:
    ConsumerEndpointImpl(const ConsumerEndpointImpl&) = delete;
    ConsumerEndpointImpl& operator=(const ConsumerEndpointImpl&) = delete;

    ServiceImpl* const service_;
    base::TaskRunner* const task_runner_;
    Consumer* consumer_;
  };

  explicit ServiceImpl(std::unique_ptr<SharedMemory::Factory>,
                       base::TaskRunner*);
  ~ServiceImpl() override;

  // Called by the ProducerEndpointImpl dtor.
  void DisconnectProducer(ProducerID);

  // Called by the ConsumerEndpointImpl dtor.
  void DisconnectConsumer(ConsumerEndpointImpl*);

  // Called by the ConsumerEndpointImpl.
  void StartTracing(ConsumerEndpointImpl*,
                    const ConsumerEndpoint::TraceConfig& cfg);
  void StopTracing(ConsumerEndpointImpl*);

  // Service implementation.
  std::unique_ptr<Service::ProducerEndpoint> ConnectProducer(
      Producer*,
      size_t shared_buffer_page_size_bytes,
      size_t shared_buffer_size_hint_bytes = 0) override;

  std::unique_ptr<Service::ConsumerEndpoint> ConnectConsumer(
      Consumer*) override;

  void set_observer_for_testing(ObserverForTesting*) override;

  // Exposed mainly for testing.
  size_t num_producers() const { return producers_.size(); }
  ProducerEndpointImpl* GetProducer(ProducerID) const;

 private:
  struct RegisteredDataSource {
    DataSourceDescriptor descriptor;
    ProducerID producer_id;
  };

  class LogBuffer {
  public:
    LogBuffer();
    ~LogBuffer();
    void Reset(size_t size = 0);  // |size| == 0 destroyes the buffer.
    explicit operator bool() const { return !!data_; }

   private:
    std::unique_ptr<char[]> data_;
    size_t size_ = 0;
  };

  struct TracingSession {
    // List of data source instances that have been enabled on the various
    // producers for this tracing session.
    std::multimap<ProducerID, DataSourceInstanceID> data_source_instances;

    // Point to objects owned by the parent's |trace_buffers_|.
    std::vector<size_t> trace_buffers;
  };

  ServiceImpl(const ServiceImpl&) = delete;
  ServiceImpl& operator=(const ServiceImpl&) = delete;

  std::unique_ptr<SharedMemory::Factory> shm_factory_;
  base::TaskRunner* const task_runner_;
  ObserverForTesting* observer_ = nullptr;
  ProducerID last_producer_id_ = 0;
  DataSourceInstanceID last_data_source_instance_id_ = 0;
  std::map<ProducerID, ProducerEndpointImpl*> producers_;
  std::set<ConsumerEndpointImpl*> consumers_;
  std::map<ConsumerEndpointImpl*, TracingSession> tracing_sessions_;
  std::multimap<std::string, RegisteredDataSource> data_sources_;

  // This vector maintain a stable index of log buffers for the various
  // |tracing_sessions_|. This is to keep a ID -> buffer table that we can query
  // while draining the Producer's shared memory buffers, avoiding expensive
  // lookups. The index of the buffers in this array matches the |target_buffer|
  // field in the SharedMemoryABI::ChunkHeader.
  LogBuffer trace_buffers_[kMaxTraceBuffers] = {};
};

}  // namespace perfetto

#endif  // TRACING_SRC_CORE_SERVICE_IMPL_H_

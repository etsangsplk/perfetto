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

#ifndef INCLUDE_PERFETTO_TRACING_CORE_STARTUP_TRACE_WRITER_REGISTRY_H_
#define INCLUDE_PERFETTO_TRACING_CORE_STARTUP_TRACE_WRITER_REGISTRY_H_

#include <memory>
#include <mutex>
#include <set>

#include "perfetto/base/export.h"
#include "perfetto/tracing/core/basic_types.h"

namespace perfetto {

class SharedMemoryArbiterImpl;
class StartupTraceWriter;

namespace base {
class TaskRunner;
}  // namespace base

class PERFETTO_EXPORT StartupTraceWriterRegistry {
 public:
  explicit StartupTraceWriterRegistry(base::TaskRunner*);
  ~StartupTraceWriterRegistry();

  // Returns a new StartupTraceWriter. The new writer will already be bound if
  // BindToArbiter() was called previously. Otherwise, it will be unbound.
  // Should only be called on the writer thread.
  std::unique_ptr<StartupTraceWriter> CreateTraceWriter();

  // Binds all StartupTraceWriters created by this registry to the given arbiter
  // and target buffer. Should only be called once. Normally this happens when
  // the perfetto service has been initialized and we want to rebind all the
  // writers created in the early startup phase.
  //
  // Note that the writers may not be bound synchronously if they are
  // concurrently being written to. The registry will retry on its TaskRunner
  // until all writers were bound successfully.
  void BindToArbiter(SharedMemoryArbiterImpl*, BufferID target_buffer);

 private:
  friend class StartupTraceWriter;
  friend class StartupTraceWriterTest;

  // Called by StartupTraceWriter.
  void OnStartupTraceWriterDestroyed(StartupTraceWriter*);

  // Try to bind the remaining unbound writers and post a continuation to
  // |task_runner_| if any writers could not be bound.
  void TryBindWriters();

  base::TaskRunner* task_runner_;

  // All variables below this point are protected by |lock_|.
  std::mutex lock_;
  std::set<StartupTraceWriter*> unbound_writers_;
  SharedMemoryArbiterImpl* arbiter_ = nullptr;  // |nullptr| while unbound.
  BufferID target_buffer_ = 0;
};

}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_TRACING_CORE_STARTUP_TRACE_WRITER_REGISTRY_H_

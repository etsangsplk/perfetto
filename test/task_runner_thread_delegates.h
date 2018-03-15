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

#ifndef TEST_TASK_RUNNER_THREAD_DELEGATES_H_
#define TEST_TASK_RUNNER_THREAD_DELEGATES_H_

#include "perfetto/tracing/ipc/service_ipc_host.h"
#include "src/traced/probes/probes_producer.h"
#include "test/fake_producer.h"
#include "test/task_runner_thread.h"

namespace perfetto {
// This is used only in daemon starting integrations tests.
class ServiceDelegate : public ThreadDelegate {
 public:
  ServiceDelegate(const char* producer_socket, const char* consumer_socket)
      : producer_socket_(producer_socket), consumer_socket_(consumer_socket) {}
  ~ServiceDelegate() override = default;

  void Initialize(base::TaskRunner* task_runner) override {
    svc_ = ServiceIPCHost::CreateInstance(task_runner);
    svc_->Start(producer_socket_, consumer_socket_);
  }

 private:
  const char* producer_socket_;
  const char* consumer_socket_;
  std::unique_ptr<ServiceIPCHost> svc_;
};

// This is used only in daemon starting integrations tests.
class ProbesProducerDelegate : public ThreadDelegate {
 public:
  ProbesProducerDelegate(const char* producer_socket)
      : producer_socket_(producer_socket) {}
  ~ProbesProducerDelegate() override = default;

  void Initialize(base::TaskRunner* task_runner) override {
    producer_.reset(new ProbesProducer);
    producer_->ConnectWithRetries(producer_socket_, task_runner);
  }

 private:
  const char* producer_socket_;
  std::unique_ptr<ProbesProducer> producer_;
};

class FakeProducerDelegate : public ThreadDelegate {
 public:
  FakeProducerDelegate(const char* producer_socket,
                       std::function<void()> connect_callback)
      : producer_socket_(producer_socket),
        connect_callback_(std::move(connect_callback)) {}
  ~FakeProducerDelegate() override = default;

  void Initialize(base::TaskRunner* task_runner) override {
    producer_.reset(new FakeProducer("android.perfetto.FakeProducer"));
    producer_->Connect(producer_socket_, task_runner,
                       std::move(connect_callback_));
  }

  FakeProducer* producer() { return producer_.get(); }

 private:
  const char* producer_socket_;
  std::unique_ptr<FakeProducer> producer_;
  std::function<void()> connect_callback_;
};
}  // namespace perfetto

#endif  // TEST_TASK_RUNNER_THREAD_DELEGATES_H_

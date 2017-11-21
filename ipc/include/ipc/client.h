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

#ifndef IPC_INCLUDE_IPC_CLIENT_H_
#define IPC_INCLUDE_IPC_CLIENT_H_

#include <functional>
#include <memory>

#include "base/scoped_file.h"
#include "base/weak_ptr.h"
#include "ipc/basic_types.h"

namespace perfetto {

namespace base {
class TaskRunner;
}  // namespace base

namespace ipc {
class ServiceProxy;

// The client-side class that talks to the host over the socket and multiplexes
// requests coming from the various autogenerated ServiceProxy stubs.
// This is meant to be used by the user code as follows:
// auto client = Client::CreateInstance("socket_name", task_runner);
// std::unique_ptr<GreeterService> svc(new GreeterService());
// client.BindService(svc);
// svc.OnConnect([] () {
//    svc.SayHello(..., ...);
// });
class Client {
 public:
  static std::unique_ptr<Client> CreateInstance(const char* socket_name,
                                                base::TaskRunner*);
  virtual ~Client() = default;

  virtual void BindService(base::WeakPtr<ServiceProxy>) = 0;

  // There is no need to call this method explicitly. Destroying the
  // ServiceProxy instance is sufficient and will automatically unbind it. This
  // method is exposed only for the ServiceProxy destructor.
  virtual void UnbindService(ServiceID) = 0;

  virtual size_t num_received_file_descriptors() const = 0;
  virtual base::ScopedFile PopReceivedFileDescriptor() = 0;
};

}  // namespace ipc
}  // namespace perfetto

#endif  // IPC_INCLUDE_IPC_CLIENT_H_

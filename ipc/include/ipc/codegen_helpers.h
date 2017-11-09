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

// This file is only meant to be included in autogenerated .cc files.

#ifndef IPC_INCLUDE_IPC_CODEGEN_HELPERS_H_
#define IPC_INCLUDE_IPC_CODEGEN_HELPERS_H_

// A templated protobuf message decoder. Returns nullptr in case of failure.
template <typename T>
::std::unique_ptr<::perfetto::ipc::ProtoMessage> _IPC_Decoder(
    const std::string& proto_data) {
  ::std::unique_ptr<::perfetto::ipc::ProtoMessage> msg(new T());
  if (msg->ParseFromString(proto_data))
    return msg;
  return nullptr;
}

// Templated method dispatcher.
template <typename TSvc,    // Type of the actual Service subclass.
          typename TReq,    // Type of the request argument.
          typename TReply,  // Type of the reply argument.
          void (TSvc::*Method)(const TReq&, ::perfetto::ipc::Deferred<TReply>)>
void _IPC_Invoker(::perfetto::ipc::Service* s,
                  const ::perfetto::ipc::ProtoMessage& req,
                  ::perfetto::ipc::DeferredBase reply) {
  (*static_cast<TSvc*>(s).*Method)(
      static_cast<const TReq&>(req),
      ::perfetto::ipc::Deferred<TReply>(::std::move(reply)));
}

#endif  // IPC_INCLUDE_IPC_CODEGEN_HELPERS_H_

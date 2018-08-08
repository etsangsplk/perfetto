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

#include "src/profiling/memory/socket_listener.h"
#include "perfetto/base/utils.h"

namespace perfetto {

void SocketListener::OnDisconnect(ipc::UnixSocket* self) {
  sockets_.erase(self);
}

void SocketListener::OnNewIncomingConnection(
    ipc::UnixSocket* self,
    std::unique_ptr<ipc::UnixSocket> new_connection) {
  sockets_.emplace(
      std::piecewise_construct, std::forward_as_tuple(self),
      std::forward_as_tuple(
          std::move(new_connection),
          // This does not need a WeakPtr because it gets called inline of the
          // Read call, which is called in OnDataAvailable below.
          [this, self](size_t size, std::unique_ptr<uint8_t[]> buf) {
            RecordReceived(self, size, std::move(buf));
          }));
}

void SocketListener::OnDataAvailable(ipc::UnixSocket* self) {
  auto it = sockets_.find(self);
  PERFETTO_DCHECK(it != sockets_.end());
  if (it == sockets_.end())
    return;

  Entry& entry = it->second;
  if (PERFETTO_LIKELY(entry.recv_fds)) {
    entry.record_reader.Read(self);
  } else {
    // The first record we receive should contain file descriptors for the
    // process' /proc/[pid]/maps and /proc/[pid]/mem. Receive those and store
    // them into metadata for process.
    //
    // If metadata for the process already exists, they will just go out of
    // scope in InitProcess.
    base::ScopedFile fds[2];
    entry.record_reader.Read(self, fds, base::ArraySize(fds));
    if (fds[0] && fds[1]) {
      InitProcess(&entry, self->peer_pid(), std::move(fds[0]),
                  std::move(fds[1]));
      entry.recv_fds = true;
    } else if (fds[0] || fds[1]) {
      PERFETTO_ELOG("Received partial FDs.");
    } else {
      PERFETTO_ELOG("Received no FDs.");
    }
  }
}

void SocketListener::InitProcess(Entry* entry,
                                 pid_t peer_pid,
                                 base::ScopedFile maps_fd,
                                 base::ScopedFile mem_fd) {
  auto it = process_metadata_.find(peer_pid);
  if (it == process_metadata_.end() || it->second.expired()) {
    // We have not seen the PID yet or the PID is being recycled.
    entry->process_metadata = std::make_shared<ProcessMetadata>(
        peer_pid, std::move(maps_fd), std::move(mem_fd));
    process_metadata_[peer_pid] = entry->process_metadata;
  } else {
    // If the process already has metadata, this is an additional socket for
    // an existing process. Reuse existing metadata and close the received
    // file descriptors.
    entry->process_metadata = std::shared_ptr<ProcessMetadata>(it->second);
  }
}

void SocketListener::RecordReceived(ipc::UnixSocket* self,
                                    size_t,
                                    std::unique_ptr<uint8_t[]>) {
  auto it = sockets_.find(self);
  PERFETTO_DCHECK(it != sockets_.end());
  if (it == sockets_.end())
    return;
  Entry& entry = it->second;
  // This needs to be a weak_ptr for two reasons:
  // 1) most importantly, the weak_ptr in process_metadata_ should expire as
  // soon as the last socket for a process goes away. Otherwise, a recycled
  // PID might reuse incorrect metadata.
  // 2) it is a waste to unwind for a process that had already gone away.
  std::weak_ptr<ProcessMetadata> weak_metadata(entry.process_metadata);
}

}  // namespace perfetto

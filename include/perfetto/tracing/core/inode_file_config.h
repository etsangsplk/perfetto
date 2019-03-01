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
 * perfetto/config/inode_file/inode_file_config.proto
 * by
 * ../../tools/proto_to_cpp/proto_to_cpp.cc.
 * If you need to make changes here, change the .proto file and then run
 * ./tools/gen_tracing_cpp_headers_from_protos
 */

#ifndef INCLUDE_PERFETTO_TRACING_CORE_INODE_FILE_CONFIG_H_
#define INCLUDE_PERFETTO_TRACING_CORE_INODE_FILE_CONFIG_H_

#include <stdint.h>
#include <string>
#include <type_traits>
#include <vector>

#include "perfetto/base/export.h"

// Forward declarations for protobuf types.
namespace perfetto {
namespace protos {
class InodeFileConfig;
class InodeFileConfig_MountPointMappingEntry;
}  // namespace protos
}  // namespace perfetto

namespace perfetto {

class PERFETTO_EXPORT InodeFileConfig {
 public:
  class PERFETTO_EXPORT MountPointMappingEntry {
   public:
    MountPointMappingEntry();
    ~MountPointMappingEntry();
    MountPointMappingEntry(MountPointMappingEntry&&) noexcept;
    MountPointMappingEntry& operator=(MountPointMappingEntry&&);
    MountPointMappingEntry(const MountPointMappingEntry&);
    MountPointMappingEntry& operator=(const MountPointMappingEntry&);
    bool operator==(const MountPointMappingEntry&) const;
    bool operator!=(const MountPointMappingEntry& other) const {
      return !(*this == other);
    }

    // Conversion methods from/to the corresponding protobuf types.
    void FromProto(
        const perfetto::protos::InodeFileConfig_MountPointMappingEntry&);
    void ToProto(
        perfetto::protos::InodeFileConfig_MountPointMappingEntry*) const;

    const std::string& mountpoint() const { return mountpoint_; }
    void set_mountpoint(const std::string& value) { mountpoint_ = value; }

    int scan_roots_size() const { return static_cast<int>(scan_roots_.size()); }
    const std::vector<std::string>& scan_roots() const { return scan_roots_; }
    void clear_scan_roots() { scan_roots_.clear(); }
    std::string* add_scan_roots() {
      scan_roots_.emplace_back();
      return &scan_roots_.back();
    }

   private:
    std::string mountpoint_ = {};
    std::vector<std::string> scan_roots_;

    // Allows to preserve unknown protobuf fields for compatibility
    // with future versions of .proto files.
    std::string unknown_fields_;
  };

  InodeFileConfig();
  ~InodeFileConfig();
  InodeFileConfig(InodeFileConfig&&) noexcept;
  InodeFileConfig& operator=(InodeFileConfig&&);
  InodeFileConfig(const InodeFileConfig&);
  InodeFileConfig& operator=(const InodeFileConfig&);
  bool operator==(const InodeFileConfig&) const;
  bool operator!=(const InodeFileConfig& other) const {
    return !(*this == other);
  }

  // Conversion methods from/to the corresponding protobuf types.
  void FromProto(const perfetto::protos::InodeFileConfig&);
  void ToProto(perfetto::protos::InodeFileConfig*) const;

  uint32_t scan_interval_ms() const { return scan_interval_ms_; }
  void set_scan_interval_ms(uint32_t value) { scan_interval_ms_ = value; }

  uint32_t scan_delay_ms() const { return scan_delay_ms_; }
  void set_scan_delay_ms(uint32_t value) { scan_delay_ms_ = value; }

  uint32_t scan_batch_size() const { return scan_batch_size_; }
  void set_scan_batch_size(uint32_t value) { scan_batch_size_ = value; }

  bool do_not_scan() const { return do_not_scan_; }
  void set_do_not_scan(bool value) { do_not_scan_ = value; }

  int scan_mount_points_size() const {
    return static_cast<int>(scan_mount_points_.size());
  }
  const std::vector<std::string>& scan_mount_points() const {
    return scan_mount_points_;
  }
  void clear_scan_mount_points() { scan_mount_points_.clear(); }
  std::string* add_scan_mount_points() {
    scan_mount_points_.emplace_back();
    return &scan_mount_points_.back();
  }

  int mount_point_mapping_size() const {
    return static_cast<int>(mount_point_mapping_.size());
  }
  const std::vector<MountPointMappingEntry>& mount_point_mapping() const {
    return mount_point_mapping_;
  }
  void clear_mount_point_mapping() { mount_point_mapping_.clear(); }
  MountPointMappingEntry* add_mount_point_mapping() {
    mount_point_mapping_.emplace_back();
    return &mount_point_mapping_.back();
  }

 private:
  uint32_t scan_interval_ms_ = {};
  uint32_t scan_delay_ms_ = {};
  uint32_t scan_batch_size_ = {};
  bool do_not_scan_ = {};
  std::vector<std::string> scan_mount_points_;
  std::vector<MountPointMappingEntry> mount_point_mapping_;

  // Allows to preserve unknown protobuf fields for compatibility
  // with future versions of .proto files.
  std::string unknown_fields_;
};

}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_TRACING_CORE_INODE_FILE_CONFIG_H_

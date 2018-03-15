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

#ifndef INCLUDE_PERFETTO_TRACED_DATA_SOURCE_TYPES_H_
#define INCLUDE_PERFETTO_TRACED_DATA_SOURCE_TYPES_H_

#include "perfetto/base/build_config.h"

namespace perfetto {

using Inode = decltype(stat::st_ino);
using BlockDeviceID = decltype(stat::st_dev);

}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_TRACED_DATA_SOURCE_TYPES_H_

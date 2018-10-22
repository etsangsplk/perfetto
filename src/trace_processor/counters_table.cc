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

#include "src/trace_processor/counters_table.h"

#include "src/trace_processor/storage_cursor.h"
#include "src/trace_processor/table_utils.h"

namespace perfetto {
namespace trace_processor {

CountersTable::CountersTable(sqlite3*, const TraceStorage* storage)
    : storage_(storage) {
  ref_types_.resize(RefType::kMax);
  ref_types_[RefType::kCPU_ID] = "cpu";
  ref_types_[RefType::kUTID] = "utid";
  ref_types_[RefType::kNoRef] = "";
  ref_types_[RefType::kIrq] = "irq";
  ref_types_[RefType::kSoftIrq] = "softirq";
}

void CountersTable::RegisterTable(sqlite3* db, const TraceStorage* storage) {
  Table::Register<CountersTable>(db, storage, "counters");
}

Table::Schema CountersTable::CreateSchema(int, const char* const*) {
  const auto& counters = storage_->counters();
  std::unique_ptr<StorageSchema::Column> cols[] = {
      StorageSchema::NumericColumnPtr("ts", &counters.timestamps(),
                                      false /* hidden */, true /* ordered */),
      StorageSchema::StringColumnPtr("name", &counters.name_ids(),
                                     &storage_->string_pool()),
      StorageSchema::NumericColumnPtr("value", &counters.values()),
      StorageSchema::NumericColumnPtr("dur", &counters.durations()),
      StorageSchema::NumericColumnPtr("value_delta", &counters.value_deltas()),
      StorageSchema::NumericColumnPtr("ref", &counters.refs()),
      StorageSchema::StringColumnPtr("ref_type", &counters.types(),
                                     &ref_types_)};
  schema_ = StorageSchema({
      std::make_move_iterator(std::begin(cols)),
      std::make_move_iterator(std::end(cols)),
  });
  return schema_.ToTableSchema({"name", "ts", "ref"});
}

std::unique_ptr<Table::Cursor> CountersTable::CreateCursor(
    const QueryConstraints& qc,
    sqlite3_value** argv) {
  uint32_t count = static_cast<uint32_t>(storage_->counters().counter_count());
  return std::unique_ptr<Table::Cursor>(new StorageCursor(
      table_utils::CreateOptimalRowIterator(schema_, count, qc, argv),
      schema_.ToColumnReporters()));
}

int CountersTable::BestIndex(const QueryConstraints&, BestIndexInfo* info) {
  info->estimated_cost =
      static_cast<uint32_t>(storage_->counters().counter_count());

  // We should be able to handle any constraint and any order by clause given
  // to us.
  info->order_by_consumed = true;
  std::fill(info->omit.begin(), info->omit.end(), true);

  return SQLITE_OK;
}

}  // namespace trace_processor
}  // namespace perfetto

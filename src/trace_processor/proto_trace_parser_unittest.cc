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

#include "src/trace_processor/proto_trace_tokenizer.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "perfetto/base/string_view.h"
#include "src/trace_processor/event_tracker.h"
#include "src/trace_processor/process_tracker.h"
#include "src/trace_processor/proto_trace_parser.h"
#include "src/trace_processor/trace_sorter.h"

#include "perfetto/trace/trace.pb.h"
#include "perfetto/trace/trace_packet.pb.h"

namespace perfetto {
namespace trace_processor {
namespace {

using ::testing::_;
using ::testing::Args;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Pointwise;
using ::testing::NiceMock;

class MockEventTracker : public EventTracker {
 public:
  MockEventTracker(TraceProcessorContext* context) : EventTracker(context) {}
  virtual ~MockEventTracker() = default;

  MOCK_METHOD6(PushSchedSwitch,
               void(uint32_t cpu,
                    int64_t timestamp,
                    uint32_t prev_pid,
                    uint32_t prev_state,
                    uint32_t next_pid,
                    base::StringView next_comm));

  MOCK_METHOD5(PushCounter,
               RowId(int64_t timestamp,
                     double value,
                     StringId name_id,
                     int64_t ref,
                     RefType ref_type));
};

class MockProcessTracker : public ProcessTracker {
 public:
  MockProcessTracker(TraceProcessorContext* context)
      : ProcessTracker(context) {}

  MOCK_METHOD2(UpdateProcess,
               UniquePid(uint32_t pid, base::StringView process_name));

  MOCK_METHOD2(UpdateThread, UniqueTid(uint32_t tid, uint32_t tgid));
};

class MockTraceStorage : public TraceStorage {
 public:
  MockTraceStorage() : TraceStorage() {}

  MOCK_METHOD1(InternString, StringId(base::StringView));
};

class ProtoTraceParserTest : public ::testing::Test {
 public:
  ProtoTraceParserTest() {
    nice_storage_ = new NiceMock<MockTraceStorage>();
    context_.storage.reset(nice_storage_);
    event_ = new MockEventTracker(&context_);
    context_.event_tracker.reset(event_);
    process_ = new MockProcessTracker(&context_);
    context_.process_tracker.reset(process_);
    const auto optim = OptimizationMode::kMinLatency;
    context_.sorter.reset(new TraceSorter(&context_, optim, 0 /*window size*/));
    context_.proto_parser.reset(new ProtoTraceParser(&context_));
  }

  void InitStorage() {
    storage_ = new MockTraceStorage();
    context_.storage.reset(storage_);
  }

  void Tokenize(const protos::Trace& trace) {
    std::unique_ptr<uint8_t[]> raw_trace(new uint8_t[trace.ByteSize()]);
    trace.SerializeToArray(raw_trace.get(), trace.ByteSize());
    ProtoTraceTokenizer tokenizer(&context_);
    tokenizer.Parse(std::move(raw_trace),
                    static_cast<size_t>(trace.ByteSize()));
  }

 protected:
  TraceProcessorContext context_;
  MockEventTracker* event_;
  MockProcessTracker* process_;
  NiceMock<MockTraceStorage>* nice_storage_;
  MockTraceStorage* storage_;
};

TEST_F(ProtoTraceParserTest, LoadSingleEvent) {
  protos::Trace trace;

  auto* bundle = trace.add_packet()->mutable_ftrace_events();
  bundle->set_cpu(10);

  auto* event = bundle->add_event();
  event->set_timestamp(1000);
  event->set_pid(12);

  static const char kProcName[] = "proc1";
  auto* sched_switch = event->mutable_sched_switch();
  sched_switch->set_prev_pid(10);
  sched_switch->set_prev_state(32);
  sched_switch->set_next_comm(kProcName);
  sched_switch->set_next_pid(100);

  EXPECT_CALL(*event_, PushSchedSwitch(10, 1000, 10, 32, 100,
                                       base::StringView(kProcName)));
  Tokenize(trace);
}

TEST_F(ProtoTraceParserTest, LoadEventsIntoRaw) {
  InitStorage();
  protos::Trace trace;

  auto* bundle = trace.add_packet()->mutable_ftrace_events();
  bundle->set_cpu(10);

  // This event is unknown and will only appear in
  // raw events table.
  auto* event = bundle->add_event();
  event->set_timestamp(1000);
  event->set_pid(12);
  auto* task = event->mutable_task_newtask();
  task->set_pid(123);
  static const char task_newtask[] = "task_newtask";
  task->set_comm(task_newtask);
  task->set_clone_flags(12);
  task->set_oom_score_adj(15);

  // This event has specific parsing logic, but will
  // also appear in raw events table.
  event = bundle->add_event();
  event->set_timestamp(1001);
  event->set_pid(12);
  auto* print = event->mutable_print();
  print->set_ip(20);
  static const char buf_value[] = "This is a print event";
  print->set_buf(buf_value);

  static const char pid[] = "pid";
  static const char comm[] = "comm";
  static const char clone[] = "clone_flags";
  static const char oom[] = "oom_score_adj";
  static const char print_event[] = "print";
  static const char ip[] = "ip";
  static const char buf[] = "buf";

  EXPECT_CALL(*storage_, InternString(base::StringView(task_newtask))).Times(2);
  EXPECT_CALL(*storage_, InternString(base::StringView(pid)));
  EXPECT_CALL(*storage_, InternString(base::StringView(comm)));
  EXPECT_CALL(*storage_, InternString(base::StringView(clone)));
  EXPECT_CALL(*storage_, InternString(base::StringView(oom)));
  EXPECT_CALL(*storage_, InternString(base::StringView(print_event)));
  EXPECT_CALL(*storage_, InternString(base::StringView(ip)));
  EXPECT_CALL(*storage_, InternString(base::StringView(buf)));
  EXPECT_CALL(*storage_, InternString(base::StringView(buf_value)));

  Tokenize(trace);
  const auto& raw = context_.storage->raw_events();
  ASSERT_EQ(raw.raw_event_count(), 2);
  const auto& args = context_.storage->args();
  ASSERT_EQ(args.args_count(), 6);
  ASSERT_EQ(args.arg_values()[0].int_value, 123);
  ASSERT_EQ(args.arg_values()[1].string_value, 0);
  ASSERT_EQ(args.arg_values()[2].int_value, 12);
  ASSERT_EQ(args.arg_values()[3].int_value, 15);
  ASSERT_EQ(args.arg_values()[4].int_value, 20);
  ASSERT_EQ(args.arg_values()[5].string_value, 0);

  // TODO(taylori): Add test ftrace event with all field types
  // and test here.
}

TEST_F(ProtoTraceParserTest, LoadMultipleEvents) {
  protos::Trace trace;

  auto* bundle = trace.add_packet()->mutable_ftrace_events();
  bundle->set_cpu(10);

  auto* event = bundle->add_event();
  event->set_timestamp(1000);
  event->set_pid(12);

  static const char kProcName1[] = "proc1";
  auto* sched_switch = event->mutable_sched_switch();
  sched_switch->set_prev_pid(10);
  sched_switch->set_prev_state(32);
  sched_switch->set_next_comm(kProcName1);
  sched_switch->set_next_pid(100);

  event = bundle->add_event();
  event->set_timestamp(1001);
  event->set_pid(12);

  static const char kProcName2[] = "proc2";
  sched_switch = event->mutable_sched_switch();
  sched_switch->set_prev_pid(100);
  sched_switch->set_prev_state(32);
  sched_switch->set_next_comm(kProcName2);
  sched_switch->set_next_pid(10);

  EXPECT_CALL(*event_, PushSchedSwitch(10, 1000, 10, 32, 100,
                                       base::StringView(kProcName1)));

  EXPECT_CALL(*event_, PushSchedSwitch(10, 1001, 100, 32, 10,
                                       base::StringView(kProcName2)));

  Tokenize(trace);
}

TEST_F(ProtoTraceParserTest, LoadMultiplePackets) {
  protos::Trace trace;

  auto* bundle = trace.add_packet()->mutable_ftrace_events();
  bundle->set_cpu(10);

  auto* event = bundle->add_event();
  event->set_timestamp(1000);
  event->set_pid(12);

  static const char kProcName1[] = "proc1";
  auto* sched_switch = event->mutable_sched_switch();
  sched_switch->set_prev_pid(10);
  sched_switch->set_prev_state(32);
  sched_switch->set_next_comm(kProcName1);
  sched_switch->set_next_pid(100);

  bundle = trace.add_packet()->mutable_ftrace_events();
  bundle->set_cpu(10);

  event = bundle->add_event();
  event->set_timestamp(1001);
  event->set_pid(12);

  static const char kProcName2[] = "proc2";
  sched_switch = event->mutable_sched_switch();
  sched_switch->set_prev_pid(100);
  sched_switch->set_prev_state(32);
  sched_switch->set_next_comm(kProcName2);
  sched_switch->set_next_pid(10);

  EXPECT_CALL(*event_, PushSchedSwitch(10, 1000, 10, 32, 100,
                                       base::StringView(kProcName1)));

  EXPECT_CALL(*event_, PushSchedSwitch(10, 1001, 100, 32, 10,
                                       base::StringView(kProcName2)));
  Tokenize(trace);
}

TEST_F(ProtoTraceParserTest, RepeatedLoadSinglePacket) {
  protos::Trace trace_1;
  auto* bundle = trace_1.add_packet()->mutable_ftrace_events();
  bundle->set_cpu(10);
  auto* event = bundle->add_event();
  event->set_timestamp(1000);
  event->set_pid(12);
  static const char kProcName1[] = "proc1";
  auto* sched_switch = event->mutable_sched_switch();
  sched_switch->set_prev_pid(10);
  sched_switch->set_prev_state(32);
  sched_switch->set_next_comm(kProcName1);
  sched_switch->set_next_pid(100);

  protos::Trace trace_2;
  bundle = trace_2.add_packet()->mutable_ftrace_events();
  bundle->set_cpu(10);
  event = bundle->add_event();
  event->set_timestamp(1001);
  event->set_pid(12);
  static const char kProcName2[] = "proc2";
  sched_switch = event->mutable_sched_switch();
  sched_switch->set_prev_pid(100);
  sched_switch->set_prev_state(32);
  sched_switch->set_next_comm(kProcName2);
  sched_switch->set_next_pid(10);

  EXPECT_CALL(*event_, PushSchedSwitch(10, 1000, 10, 32, 100,
                                       base::StringView(kProcName1)));
  Tokenize(trace_1);

  EXPECT_CALL(*event_, PushSchedSwitch(10, 1001, 100, 32, 10,
                                       base::StringView(kProcName2)));
  Tokenize(trace_2);
}

TEST_F(ProtoTraceParserTest, LoadMemInfo) {
  protos::Trace trace_1;
  auto* packet = trace_1.add_packet();
  uint64_t ts = 1000;
  packet->set_timestamp(ts);
  auto* bundle = packet->mutable_sys_stats();
  auto* meminfo = bundle->add_meminfo();
  meminfo->set_key(perfetto::protos::MEMINFO_MEM_TOTAL);
  uint32_t value = 10;
  meminfo->set_value(value);

  EXPECT_CALL(*event_, PushCounter(static_cast<int64_t>(ts), value, 0, 0,
                                   RefType::kRefNoRef));
  Tokenize(trace_1);
}

TEST_F(ProtoTraceParserTest, LoadVmStats) {
  protos::Trace trace_1;
  auto* packet = trace_1.add_packet();
  uint64_t ts = 1000;
  packet->set_timestamp(ts);
  auto* bundle = packet->mutable_sys_stats();
  auto* meminfo = bundle->add_vmstat();
  meminfo->set_key(perfetto::protos::VMSTAT_COMPACT_SUCCESS);
  uint32_t value = 10;
  meminfo->set_value(value);

  EXPECT_CALL(*event_, PushCounter(static_cast<int64_t>(ts), value, 0, 0,
                                   RefType::kRefNoRef));
  Tokenize(trace_1);
}

TEST_F(ProtoTraceParserTest, LoadCpuFreq) {
  protos::Trace trace_1;
  auto* bundle = trace_1.add_packet()->mutable_ftrace_events();
  bundle->set_cpu(12);
  auto* event = bundle->add_event();
  event->set_timestamp(1000);
  event->set_pid(12);
  auto* cpu_freq = event->mutable_cpu_frequency();
  cpu_freq->set_cpu_id(10);
  cpu_freq->set_state(2000);

  EXPECT_CALL(*event_, PushCounter(1000, 2000, 0, 10, RefType::kRefCpuId));
  Tokenize(trace_1);
}

TEST_F(ProtoTraceParserTest, LoadProcessPacket) {
  protos::Trace trace;

  auto* tree = trace.add_packet()->mutable_process_tree();
  auto* process = tree->add_processes();
  static const char kProcName1[] = "proc1";

  process->add_cmdline(kProcName1);
  process->set_pid(1);
  process->set_ppid(2);

  EXPECT_CALL(*process_, UpdateProcess(1, base::StringView(kProcName1)));
  Tokenize(trace);
}

TEST_F(ProtoTraceParserTest, LoadProcessPacket_FirstCmdline) {
  protos::Trace trace;

  auto* tree = trace.add_packet()->mutable_process_tree();
  auto* process = tree->add_processes();
  static const char kProcName1[] = "proc1";
  static const char kProcName2[] = "proc2";

  process->add_cmdline(kProcName1);
  process->add_cmdline(kProcName2);
  process->set_pid(1);
  process->set_ppid(2);

  EXPECT_CALL(*process_, UpdateProcess(1, base::StringView(kProcName1)));
  Tokenize(trace);
}

TEST_F(ProtoTraceParserTest, LoadThreadPacket) {
  protos::Trace trace;

  auto* tree = trace.add_packet()->mutable_process_tree();
  auto* thread = tree->add_threads();
  thread->set_tid(1);
  thread->set_tgid(2);

  EXPECT_CALL(*process_, UpdateThread(1, 2));
  Tokenize(trace);
}

TEST(SystraceParserTest, SystraceEvent) {
  SystraceTracePoint result{};
  ASSERT_TRUE(ParseSystraceTracePoint(base::StringView("B|1|foo"), &result));
  EXPECT_EQ(result, (SystraceTracePoint{'B', 1, base::StringView("foo"), 0}));

  ASSERT_TRUE(ParseSystraceTracePoint(base::StringView("B|42|Bar"), &result));
  EXPECT_EQ(result, (SystraceTracePoint{'B', 42, base::StringView("Bar"), 0}));

  ASSERT_TRUE(
      ParseSystraceTracePoint(base::StringView("C|543|foo|8"), &result));
  EXPECT_EQ(result, (SystraceTracePoint{'C', 543, base::StringView("foo"), 8}));
}

}  // namespace
}  // namespace trace_processor
}  // namespace perfetto

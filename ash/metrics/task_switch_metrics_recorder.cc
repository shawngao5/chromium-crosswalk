// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/metrics/task_switch_metrics_recorder.h"

#include "ash/metrics/task_switch_time_tracker.h"

namespace ash {

namespace {

const char kShelfHistogramName[] =
    "Ash.Shelf.TimeBetweenNavigateToTaskSwitches";

const char kTabStripHistogramName[] =
    "Ash.Tab.TimeBetweenSwitchToExistingTabUserActions";

const char kAcceleratorWindowCycleHistogramName[] =
    "Ash.WindowCycleController.TimeBetweenTaskSwitches";

// Returns the histogram name for the given |task_switch_source|.
const char* GetHistogramName(
    TaskSwitchMetricsRecorder::TaskSwitchSource task_switch_source) {
  switch (task_switch_source) {
    case TaskSwitchMetricsRecorder::kShelf:
      return kShelfHistogramName;
    case TaskSwitchMetricsRecorder::kTabStrip:
      return kTabStripHistogramName;
    case TaskSwitchMetricsRecorder::kWindowCycleController:
      return kAcceleratorWindowCycleHistogramName;
  }
  NOTREACHED();
  return nullptr;
}

}  // namespace

TaskSwitchMetricsRecorder::TaskSwitchMetricsRecorder() {
}

TaskSwitchMetricsRecorder::~TaskSwitchMetricsRecorder() {
}

void TaskSwitchMetricsRecorder::OnTaskSwitch(
    TaskSwitchSource task_switch_source) {
  TaskSwitchTimeTracker* task_switch_time_tracker =
      FindTaskSwitchTimeTracker(task_switch_source);
  if (!task_switch_time_tracker)
    AddTaskSwitchTimeTracker(task_switch_source);

  task_switch_time_tracker = FindTaskSwitchTimeTracker(task_switch_source);
  CHECK(task_switch_time_tracker);

  task_switch_time_tracker->OnTaskSwitch();
}

TaskSwitchTimeTracker* TaskSwitchMetricsRecorder::FindTaskSwitchTimeTracker(
    TaskSwitchSource task_switch_source) {
  return histogram_map_.get(task_switch_source);
}

void TaskSwitchMetricsRecorder::AddTaskSwitchTimeTracker(
    TaskSwitchSource task_switch_source) {
  CHECK(histogram_map_.find(task_switch_source) == histogram_map_.end());

  const char* histogram_name = GetHistogramName(task_switch_source);
  DCHECK(histogram_name);

  histogram_map_.add(
      task_switch_source,
      make_scoped_ptr(new TaskSwitchTimeTracker(histogram_name)));
}

}  // namespace ash

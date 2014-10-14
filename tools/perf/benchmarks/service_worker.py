# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import page_sets
import re

from measurements import timeline_controller
from telemetry import benchmark
from telemetry.page import page_test
from telemetry.timeline import async_slice as async_slice_module
from telemetry.timeline import slice as slice_module
from telemetry.value import scalar

class _ServiceWorkerTimelineMetric(object):
  def AddResultsOfCounters(self, process, counter_regex_string, results):
    counter_filter = re.compile(counter_regex_string)
    for counter_name, counter in process.counters.iteritems():
      if not counter_filter.search(counter_name):
        continue

      total = sum(counter.totals)

      # Results objects cannot contain the '.' character, so remove that here.
      sanitized_counter_name = counter_name.replace('.', '_')

      results.AddValue(scalar.ScalarValue(
          results.current_page, sanitized_counter_name, 'count', total))
      results.AddValue(scalar.ScalarValue(
          results.current_page, sanitized_counter_name + '_avg', 'count',
          total / float(len(counter.totals))))

  def AddResultsOfEvents(
      self, process, thread_regex_string, event_regex_string, results):
    thread_filter = re.compile(thread_regex_string)
    event_filter = re.compile(event_regex_string)

    for thread in process.threads.itervalues():
      thread_name = thread.name.replace('/', '_')
      if not thread_filter.search(thread_name):
        continue

      filtered_events = []
      for event in thread.IterAllEvents():
        event_name = event.name.replace('.', '_')
        if event_filter.search(event_name):
          filtered_events.append(event)

      async_events_by_name = collections.defaultdict(list)
      sync_events_by_name = collections.defaultdict(list)
      for event in filtered_events:
        if isinstance(event, async_slice_module.AsyncSlice):
          async_events_by_name[event.name].append(event)
        elif isinstance(event, slice_module.Slice):
          sync_events_by_name[event.name].append(event)

      for event_name, event_group in async_events_by_name.iteritems():
        times = [e.duration for e in event_group]
        self._AddResultOfEvent(thread_name, event_name, times, results)

      for event_name, event_group in sync_events_by_name.iteritems():
        times = [e.self_time for e in event_group]
        self._AddResultOfEvent(thread_name, event_name, times, results)

  def _AddResultOfEvent(self, thread_name, event_name, times, results):
    total = sum(times)
    biggest_jank = max(times)

    # Results objects cannot contain the '.' character, so remove that here.
    sanitized_event_name = event_name.replace('.', '_')

    full_name = thread_name + '|' + sanitized_event_name
    results.AddValue(scalar.ScalarValue(
        results.current_page, full_name, 'ms', total))
    results.AddValue(scalar.ScalarValue(
        results.current_page, full_name + '_max', 'ms', biggest_jank))
    results.AddValue(scalar.ScalarValue(
        results.current_page, full_name + '_avg', 'ms', total / len(times)))

class _ServiceWorkerMeasurement(page_test.PageTest):
  def __init__(self, *args, **kwargs):
    super(_ServiceWorkerMeasurement, self).__init__(*args, **kwargs)
    self._timeline_controller = timeline_controller.TimelineController()

  def CustomizeBrowserOptions(self, options):
    options.AppendExtraBrowserArgs([
        '--enable-experimental-web-platform-features'
      ])

  def WillNavigateToPage(self, page, tab):
    self._timeline_controller.SetUp(page, tab)
    self._timeline_controller.Start(tab)

  def ValidateAndMeasurePage(self, _, tab, results):
    tab.WaitForJavaScriptExpression('window.done', 40)
    self._timeline_controller.Stop(tab)

    # Measure JavaScript-land
    json = tab.EvaluateJavaScript('window.results')
    for key, value in json.iteritems():
      results.AddValue(scalar.ScalarValue(
          results.current_page, key, value['units'], value['value']))

    # Retrieve TRACE_EVENTs
    timeline_metric = _ServiceWorkerTimelineMetric()
    browser_process = self._timeline_controller.model.browser_process
    filter_text = '(RegisterServiceWorker|'\
                  'UnregisterServiceWorker|'\
                  'ProcessAllocate|'\
                  'FindRegistrationForDocument|'\
                  'PrepareForMainResource|'\
                  'DispatchFetchEvent)'
    timeline_metric.AddResultsOfEvents(
        browser_process, 'IOThread', filter_text , results)

@benchmark.Disabled
class ServiceWorkerPerfTest(benchmark.Benchmark):
  test = _ServiceWorkerMeasurement
  page_set = page_sets.ServiceWorkerPageSet

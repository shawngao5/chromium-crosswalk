# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from live_tests import chrome_proxy_measurements as measurements
from live_tests import pagesets
from telemetry import benchmark
from telemetry.core.backends.chrome import android_browser_finder


class ChromeProxyLatency(benchmark.Benchmark):
  tag = 'latency'
  test = measurements.ChromeProxyLatency
  page_set = pagesets.Top20PageSet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.latency.top_20'


class ChromeProxyLatencyDirect(benchmark.Benchmark):
  tag = 'latency_direct'
  test = measurements.ChromeProxyLatency
  page_set = pagesets.Top20PageSet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.latency_direct.top_20'


class ChromeProxyDataSaving(benchmark.Benchmark):
  tag = 'data_saving'
  test = measurements.ChromeProxyDataSaving
  page_set = pagesets.Top20PageSet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.data_saving.top_20'


class ChromeProxyDataSavingDirect(benchmark.Benchmark):
  tag = 'data_saving_direct'
  test = measurements.ChromeProxyDataSaving
  page_set = pagesets.Top20PageSet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.data_saving_direct.top_20'


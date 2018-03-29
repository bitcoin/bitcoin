# Copyright 2013 Google Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
try:
    import _pickle as cPickle
except ImportError:
    import cPickle
import errno
from functools import total_ordering
import gzip
import json
import multiprocessing
import optparse
import os
from pickle import HIGHEST_PROTOCOL as PICKLE_HIGHEST_PROTOCOL
import re
import shutil
import signal
import subprocess
import sys
import tempfile
try:
    import _thread as thread
except ImportError:
    import thread
import threading
import time

if sys.version_info.major >= 3:
    long = int

if sys.platform == 'win32':
  import msvcrt
else:
  import fcntl


# An object that catches SIGINT sent to the Python process and notices
# if processes passed to wait() die by SIGINT (we need to look for
# both of those cases, because pressing Ctrl+C can result in either
# the main process or one of the subprocesses getting the signal).
#
# Before a SIGINT is seen, wait(p) will simply call p.wait() and
# return the result. Once a SIGINT has been seen (in the main process
# or a subprocess, including the one the current call is waiting for),
# wait(p) will call p.terminate() and raise ProcessWasInterrupted.
class SigintHandler(object):
  class ProcessWasInterrupted(Exception): pass
  sigint_returncodes = {-signal.SIGINT,  # Unix
                        -1073741510,     # Windows
                        }
  def __init__(self):
    self.__lock = threading.Lock()
    self.__processes = set()
    self.__got_sigint = False
    signal.signal(signal.SIGINT, lambda signal_num, frame: self.interrupt())
  def __on_sigint(self):
    self.__got_sigint = True
    while self.__processes:
      try:
        self.__processes.pop().terminate()
      except OSError:
        pass
  def interrupt(self):
    with self.__lock:
      self.__on_sigint()
  def got_sigint(self):
    with self.__lock:
      return self.__got_sigint
  def wait(self, p):
    with self.__lock:
      if self.__got_sigint:
        p.terminate()
      self.__processes.add(p)
    code = p.wait()
    with self.__lock:
      self.__processes.discard(p)
      if code in self.sigint_returncodes:
        self.__on_sigint()
      if self.__got_sigint:
        raise self.ProcessWasInterrupted
    return code
sigint_handler = SigintHandler()


# Return the width of the terminal, or None if it couldn't be
# determined (e.g. because we're not being run interactively).
def term_width(out):
  if not out.isatty():
    return None
  try:
    p = subprocess.Popen(["stty", "size"],
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (out, err) = p.communicate()
    if p.returncode != 0 or err:
      return None
    return int(out.split()[1])
  except (IndexError, OSError, ValueError):
    return None


# Output transient and permanent lines of text. If several transient
# lines are written in sequence, the new will overwrite the old. We
# use this to ensure that lots of unimportant info (tests passing)
# won't drown out important info (tests failing).
class Outputter(object):
  def __init__(self, out_file):
    self.__out_file = out_file
    self.__previous_line_was_transient = False
    self.__width = term_width(out_file)  # Line width, or None if not a tty.
  def transient_line(self, msg):
    if self.__width is None:
      self.__out_file.write(msg + "\n")
    else:
      self.__out_file.write("\r" + msg[:self.__width].ljust(self.__width))
      self.__previous_line_was_transient = True
  def flush_transient_output(self):
    if self.__previous_line_was_transient:
      self.__out_file.write("\n")
      self.__previous_line_was_transient = False
  def permanent_line(self, msg):
    self.flush_transient_output()
    self.__out_file.write(msg + "\n")


def get_save_file_path():
  """Return path to file for saving transient data."""
  if sys.platform == 'win32':
    default_cache_path = os.path.join(os.path.expanduser('~'),
                                      'AppData', 'Local')
    cache_path = os.environ.get('LOCALAPPDATA', default_cache_path)
  else:
    # We don't use xdg module since it's not a standard.
    default_cache_path = os.path.join(os.path.expanduser('~'), '.cache')
    cache_path = os.environ.get('XDG_CACHE_HOME', default_cache_path)

  if os.path.isdir(cache_path):
    return os.path.join(cache_path, 'gtest-parallel')
  else:
    sys.stderr.write('Directory {} does not exist'.format(cache_path))
    return os.path.join(os.path.expanduser('~'), '.gtest-parallel-times')


@total_ordering
class Task(object):
  """Stores information about a task (single execution of a test).

  This class stores information about the test to be executed (gtest binary and
  test name), and its result (log file, exit code and runtime).
  Each task is uniquely identified by the gtest binary, the test name and an
  execution number that increases each time the test is executed.
  Additionaly we store the last execution time, so that next time the test is
  executed, the slowest tests are run first.
  """
  def __init__(self, test_binary, test_name, test_command, execution_number,
               last_execution_time, output_dir):
    self.test_name = test_name
    self.output_dir = output_dir
    self.test_binary = test_binary
    self.test_command = test_command
    self.execution_number = execution_number
    self.last_execution_time = last_execution_time

    self.exit_code = None
    self.runtime_ms = None

    self.test_id = (test_binary, test_name)
    self.task_id = (test_binary, test_name, self.execution_number)

    self.log_file = Task._logname(self.output_dir, self.test_binary,
                                  test_name, self.execution_number)

  def __sorting_key(self):
    # Unseen or failing tests (both missing execution time) take precedence over
    # execution time. Tests are greater (seen as slower) when missing times so
    # that they are executed first.
    return (1 if self.last_execution_time is None else 0,
            self.last_execution_time)

  def __eq__(self, other):
      return self.__sorting_key() == other.__sorting_key()

  def __ne__(self, other):
      return not (self == other)

  def __lt__(self, other):
      return self.__sorting_key() < other.__sorting_key()

  @staticmethod
  def _normalize(string):
    return re.sub('[^A-Za-z0-9]', '_', string)

  @staticmethod
  def _logname(output_dir, test_binary, test_name, execution_number):
    # Store logs to temporary files if there is no output_dir.
    if output_dir is None:
      (log_handle, log_name) = tempfile.mkstemp(prefix='gtest_parallel_',
                                                suffix=".log")
      os.close(log_handle)
      return log_name

    log_name = '%s-%s-%d.log' % (Task._normalize(os.path.basename(test_binary)),
                                 Task._normalize(test_name), execution_number)

    return os.path.join(output_dir, log_name)

  def run(self):
    begin = time.time()
    with open(self.log_file, 'w') as log:
      task = subprocess.Popen(self.test_command, stdout=log, stderr=log)
      try:
        self.exit_code = sigint_handler.wait(task)
      except sigint_handler.ProcessWasInterrupted:
        thread.exit()
    self.runtime_ms = int(1000 * (time.time() - begin))
    self.last_execution_time = None if self.exit_code else self.runtime_ms


class TaskManager(object):
  """Executes the tasks and stores the passed, failed and interrupted tasks.

  When a task is run, this class keeps track if it passed, failed or was
  interrupted. After a task finishes it calls the relevant functions of the
  Logger, TestResults and TestTimes classes, and in case of failure, retries the
  test as specified by the --retry_failed flag.
  """
  def __init__(self, times, logger, test_results, task_factory, times_to_retry,
               initial_execution_number):
    self.times = times
    self.logger = logger
    self.test_results = test_results
    self.task_factory = task_factory
    self.times_to_retry = times_to_retry
    self.initial_execution_number = initial_execution_number

    self.global_exit_code = 0

    self.passed = []
    self.failed = []
    self.started = {}
    self.execution_number = {}

    self.lock = threading.Lock()

  def __get_next_execution_number(self, test_id):
    with self.lock:
      next_execution_number = self.execution_number.setdefault(
          test_id, self.initial_execution_number)
      self.execution_number[test_id] += 1
    return next_execution_number

  def __register_start(self, task):
    with self.lock:
      self.started[task.task_id] = task

  def __register_exit(self, task):
    self.logger.log_exit(task)
    self.times.record_test_time(task.test_binary, task.test_name,
                                task.last_execution_time)
    if self.test_results:
      self.test_results.log(task.test_name, task.runtime_ms,
                            "PASS" if task.exit_code == 0 else "FAIL")

    with self.lock:
      self.started.pop(task.task_id)
      if task.exit_code == 0:
        self.passed.append(task)
      else:
        self.failed.append(task)

  def run_task(self, task):
    for try_number in range(self.times_to_retry + 1):
      self.__register_start(task)
      task.run()
      self.__register_exit(task)

      if task.exit_code == 0:
        break

      if try_number < self.times_to_retry:
        execution_number = self.__get_next_execution_number(task.test_id)
        # We need create a new Task instance. Each task represents a single test
        # execution, with its own runtime, exit code and log file.
        task = self.task_factory(task.test_binary, task.test_name,
                                 task.test_command, execution_number,
                                 task.last_execution_time, task.output_dir)

    with self.lock:
      if task.exit_code != 0:
        self.global_exit_code = task.exit_code


class FilterFormat(object):
  def __init__(self, output_dir):
    if sys.stdout.isatty():
      # stdout needs to be unbuffered since the output is interactive.
      sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', 0)

    self.output_dir = output_dir

    self.total_tasks = 0
    self.finished_tasks = 0
    self.out = Outputter(sys.stdout)
    self.stdout_lock = threading.Lock()

  def move_to(self, destination_dir, tasks):
    if self.output_dir is None:
      return

    destination_dir = os.path.join(self.output_dir, destination_dir)
    os.makedirs(destination_dir)
    for task in tasks:
      shutil.move(task.log_file, destination_dir)

  def print_tests(self, message, tasks, print_try_number):
    self.out.permanent_line("%s (%s/%s):" %
                            (message, len(tasks), self.total_tasks))
    for task in sorted(tasks):
      runtime_ms = 'Interrupted'
      if task.runtime_ms is not None:
        runtime_ms = '%d ms' % task.runtime_ms
      self.out.permanent_line("%11s: %s %s%s" % (
          runtime_ms, task.test_binary, task.test_name,
          (" (try #%d)" % task.execution_number) if print_try_number else ""))

  def log_exit(self, task):
    with self.stdout_lock:
      self.finished_tasks += 1
      self.out.transient_line("[%d/%d] %s (%d ms)"
                              % (self.finished_tasks, self.total_tasks,
                                 task.test_name, task.runtime_ms))
      if task.exit_code != 0:
        with open(task.log_file) as f:
          for line in f.readlines():
            self.out.permanent_line(line.rstrip())
        self.out.permanent_line(
          "[%d/%d] %s returned/aborted with exit code %d (%d ms)"
          % (self.finished_tasks, self.total_tasks, task.test_name,
             task.exit_code, task.runtime_ms))

    if self.output_dir is None:
      # Try to remove the file 100 times (sleeping for 0.1 second in between).
      # This is a workaround for a process handle seemingly holding on to the
      # file for too long inside os.subprocess. This workaround is in place
      # until we figure out a minimal repro to report upstream (or a better
      # suspect) to prevent os.remove exceptions.
      num_tries = 100
      for i in range(num_tries):
        try:
          os.remove(task.log_file)
        except OSError as e:
          if e.errno is not errno.ENOENT:
            if i is num_tries - 1:
              self.out.permanent_line('Could not remove temporary log file: ' + str(e))
            else:
              time.sleep(0.1)
            continue
        break

  def log_tasks(self, total_tasks):
    self.total_tasks += total_tasks
    self.out.transient_line("[0/%d] Running tests..." % self.total_tasks)

  def summarize(self, passed_tasks, failed_tasks, interrupted_tasks):
    stats = {}
    def add_stats(stats, task, idx):
      task_key = (task.test_binary, task.test_name)
      if not task_key in stats:
        # (passed, failed, interrupted) task_key is added as tie breaker to get
        # alphabetic sorting on equally-stable tests
        stats[task_key] = [0, 0, 0, task_key]
      stats[task_key][idx] += 1

    for task in passed_tasks:
      add_stats(stats, task, 0)
    for task in failed_tasks:
      add_stats(stats, task, 1)
    for task in interrupted_tasks:
      add_stats(stats, task, 2)

    self.out.permanent_line("SUMMARY:")
    for task_key in sorted(stats, key=stats.__getitem__):
      (num_passed, num_failed, num_interrupted, _) = stats[task_key]
      (test_binary, task_name) = task_key
      self.out.permanent_line(
          "  %s %s passed %d / %d times%s." %
              (test_binary, task_name, num_passed,
               num_passed + num_failed + num_interrupted,
               "" if num_interrupted == 0 else (" (%d interrupted)" % num_interrupted)))

  def flush(self):
    self.out.flush_transient_output()


class CollectTestResults(object):
  def __init__(self, json_dump_filepath):
    self.test_results_lock = threading.Lock()
    self.json_dump_file = open(json_dump_filepath, 'w')
    self.test_results = {
        "interrupted": False,
        "path_delimiter": ".",
        # Third version of the file format. See the link in the flag description
        # for details.
        "version": 3,
        "seconds_since_epoch": int(time.time()),
        "num_failures_by_type": {
            "PASS": 0,
            "FAIL": 0,
        },
        "tests": {},
    }

  def log(self, test, runtime_ms, actual_result):
    with self.test_results_lock:
      self.test_results['num_failures_by_type'][actual_result] += 1
      results = self.test_results['tests']
      for name in test.split('.'):
        results = results.setdefault(name, {})

      if results:
        results['actual'] += ' ' + actual_result
        results['times'].append(runtime_ms)
      else:  # This is the first invocation of the test
        results['actual'] = actual_result
        results['times'] = [runtime_ms]
        results['time'] = runtime_ms
        results['expected'] = 'PASS'

  def dump_to_file_and_close(self):
    json.dump(self.test_results, self.json_dump_file)
    self.json_dump_file.close()


# Record of test runtimes. Has built-in locking.
class TestTimes(object):
  class LockedFile(object):
    def __init__(self, filename, mode):
      self._filename = filename
      self._mode = mode
      self._fo = None

    def __enter__(self):
      self._fo = open(self._filename, self._mode)

      # Regardless of opening mode we always seek to the beginning of file.
      # This simplifies code working with LockedFile and also ensures that
      # we lock (and unlock below) always the same region in file on win32.
      self._fo.seek(0)

      try:
        if sys.platform == 'win32':
          # We are locking here fixed location in file to use it as
          # an exclusive lock on entire file.
          msvcrt.locking(self._fo.fileno(), msvcrt.LK_LOCK, 1)
        else:
          fcntl.flock(self._fo.fileno(), fcntl.LOCK_EX)
      except IOError:
        self._fo.close()
        raise

      return self._fo

    def __exit__(self, exc_type, exc_value, traceback):
      # Flush any buffered data to disk. This is needed to prevent race
      # condition which happens from the moment of releasing file lock
      # till closing the file.
      self._fo.flush()

      try:
        if sys.platform == 'win32':
          self._fo.seek(0)
          msvcrt.locking(self._fo.fileno(), msvcrt.LK_UNLCK, 1)
        else:
          fcntl.flock(self._fo.fileno(), fcntl.LOCK_UN)
      finally:
        self._fo.close()

      return exc_value is None

  def __init__(self, save_file):
    "Create new object seeded with saved test times from the given file."
    self.__times = {}  # (test binary, test name) -> runtime in ms

    # Protects calls to record_test_time(); other calls are not
    # expected to be made concurrently.
    self.__lock = threading.Lock()

    try:
      with TestTimes.LockedFile(save_file, 'rb') as fd:
        times = TestTimes.__read_test_times_file(fd)
    except IOError:
      # We couldn't obtain the lock.
      return

    # Discard saved times if the format isn't right.
    if type(times) is not dict:
      return
    for ((test_binary, test_name), runtime) in list(times.items()):
      if (type(test_binary) is not str or type(test_name) is not str
          or type(runtime) not in {int, long, type(None)}):
        return

    self.__times = times

  def get_test_time(self, binary, testname):
    """Return the last duration for the given test as an integer number of
    milliseconds, or None if the test failed or if there's no record for it."""
    return self.__times.get((binary, testname), None)

  def record_test_time(self, binary, testname, runtime_ms):
    """Record that the given test ran in the specified number of
    milliseconds. If the test failed, runtime_ms should be None."""
    with self.__lock:
      self.__times[(binary, testname)] = runtime_ms

  def write_to_file(self, save_file):
    "Write all the times to file."
    try:
      with TestTimes.LockedFile(save_file, 'a+b') as fd:
        times = TestTimes.__read_test_times_file(fd)

        if times is None:
          times = self.__times
        else:
          times.update(self.__times)

        # We erase data from file while still holding a lock to it. This
        # way reading old test times and appending new ones are atomic
        # for external viewer.
        fd.seek(0)
        fd.truncate()
        with gzip.GzipFile(fileobj=fd, mode='wb') as gzf:
          cPickle.dump(times, gzf, PICKLE_HIGHEST_PROTOCOL)
    except IOError:
      pass  # ignore errors---saving the times isn't that important

  @staticmethod
  def __read_test_times_file(fd):
    try:
      with gzip.GzipFile(fileobj=fd, mode='rb') as gzf:
        times = cPickle.load(gzf)
    except Exception:
      # File doesn't exist, isn't readable, is malformed---whatever.
      # Just ignore it.
      return None
    else:
      return times


def find_tests(binaries, additional_args, options, times):
  test_count = 0
  tasks = []
  for test_binary in binaries:
    command = [test_binary]
    if options.gtest_also_run_disabled_tests:
      command += ['--gtest_also_run_disabled_tests']

    list_command = command + ['--gtest_list_tests']
    if options.gtest_filter != '':
      list_command += ['--gtest_filter=' + options.gtest_filter]

    try:
      test_list = subprocess.check_output(list_command,
                                          stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
      sys.exit("%s: %s" % (test_binary, str(e)))

    command += additional_args + ['--gtest_color=' + options.gtest_color]

    test_group = ''
    for line in test_list.split('\n'):
      if not line.strip():
        continue
      if line[0] != " ":
        # Remove comments for typed tests and strip whitespace.
        test_group = line.split('#')[0].strip()
        continue
      # Remove comments for parameterized tests and strip whitespace.
      line = line.split('#')[0].strip()
      if not line:
        continue

      test_name = test_group + line
      if not options.gtest_also_run_disabled_tests and 'DISABLED_' in test_name:
        continue

      last_execution_time = times.get_test_time(test_binary, test_name)
      if options.failed and last_execution_time is not None:
        continue

      test_command = command + ['--gtest_filter=' + test_name]
      if (test_count - options.shard_index) % options.shard_count == 0:
        for execution_number in range(options.repeat):
          tasks.append(Task(test_binary, test_name, test_command,
                            execution_number + 1, last_execution_time,
                            options.output_dir))

      test_count += 1

  # Sort the tasks to run the slowest tests first, so that faster ones can be
  # finished in parallel.
  return sorted(tasks, reverse=True)


def execute_tasks(tasks, pool_size, task_manager,
                  timeout, serialize_test_cases):
  class WorkerFn(object):
    def __init__(self, tasks, running_groups):
      self.tasks = tasks
      self.running_groups = running_groups
      self.task_lock = threading.Lock()

    def __call__(self):
      while True:
        with self.task_lock:
          for task_id in range(len(self.tasks)):
            task = self.tasks[task_id]

            if self.running_groups is not None:
              test_group = task.test_name.split('.')[0]
              if test_group in self.running_groups:
                # Try to find other non-running test group.
                continue
              else:
                self.running_groups.add(test_group)

            del self.tasks[task_id]
            break
          else:
            # Either there is no tasks left or number or remaining test
            # cases (groups) is less than number or running threads.
            return

        task_manager.run_task(task)

        if self.running_groups is not None:
          with self.task_lock:
            self.running_groups.remove(test_group)

  def start_daemon(func):
    t = threading.Thread(target=func)
    t.daemon = True
    t.start()
    return t

  try:
    if timeout:
      timeout.start()
    running_groups = set() if serialize_test_cases else None
    worker_fn = WorkerFn(tasks, running_groups)
    workers = [start_daemon(worker_fn) for _ in range(pool_size)]
    for worker in workers:
      worker.join()
  finally:
    if timeout:
      timeout.cancel()


def default_options_parser():
  parser = optparse.OptionParser(
      usage = 'usage: %prog [options] binary [binary ...] -- [additional args]')

  parser.add_option('-d', '--output_dir', type='string', default=None,
                    help='Output directory for test logs. Logs will be '
                         'available under gtest-parallel-logs/, so '
                         '--output_dir=/tmp will results in all logs being '
                         'available under /tmp/gtest-parallel-logs/.')
  parser.add_option('-r', '--repeat', type='int', default=1,
                    help='Number of times to execute all the tests.')
  parser.add_option('--retry_failed', type='int', default=0,
                    help='Number of times to repeat failed tests.')
  parser.add_option('--failed', action='store_true', default=False,
                    help='run only failed and new tests')
  parser.add_option('-w', '--workers', type='int',
                    default=multiprocessing.cpu_count(),
                    help='number of workers to spawn')
  parser.add_option('--gtest_color', type='string', default='yes',
                    help='color output')
  parser.add_option('--gtest_filter', type='string', default='',
                    help='test filter')
  parser.add_option('--gtest_also_run_disabled_tests', action='store_true',
                    default=False, help='run disabled tests too')
  parser.add_option('--print_test_times', action='store_true', default=False,
                    help='list the run time of each test at the end of execution')
  parser.add_option('--shard_count', type='int', default=1,
                    help='total number of shards (for sharding test execution '
                         'between multiple machines)')
  parser.add_option('--shard_index', type='int', default=0,
                    help='zero-indexed number identifying this shard (for '
                         'sharding test execution between multiple machines)')
  parser.add_option('--dump_json_test_results', type='string', default=None,
                    help='Saves the results of the tests as a JSON machine-'
                         'readable file. The format of the file is specified at '
                         'https://www.chromium.org/developers/the-json-test-results-format')
  parser.add_option('--timeout', type='int', default=None,
                    help='Interrupt all remaining processes after the given '
                         'time (in seconds).')
  parser.add_option('--serialize_test_cases', action='store_true',
                    default=False, help='Do not run tests from the same test '
                                        'case in parallel.')
  return parser


def main():
  # Remove additional arguments (anything after --).
  additional_args = []

  for i in range(len(sys.argv)):
    if sys.argv[i] == '--':
      additional_args = sys.argv[i+1:]
      sys.argv = sys.argv[:i]
      break

  parser = default_options_parser()
  (options, binaries) = parser.parse_args()

  if (options.output_dir is not None and
      not os.path.isdir(options.output_dir)):
    parser.error('--output_dir value must be an existing directory, '
                 'current value is "%s"' % options.output_dir)

  # Append gtest-parallel-logs to log output, this is to avoid deleting user
  # data if an user passes a directory where files are already present. If a
  # user specifies --output_dir=Docs/, we'll create Docs/gtest-parallel-logs
  # and clean that directory out on startup, instead of nuking Docs/.
  if options.output_dir:
    options.output_dir = os.path.join(options.output_dir,
                                      'gtest-parallel-logs')

  if binaries == []:
    parser.print_usage()
    sys.exit(1)

  if options.shard_count < 1:
    parser.error("Invalid number of shards: %d. Must be at least 1." %
                 options.shard_count)
  if not (0 <= options.shard_index < options.shard_count):
    parser.error("Invalid shard index: %d. Must be between 0 and %d "
                 "(less than the number of shards)." %
                 (options.shard_index, options.shard_count - 1))

  # Check that all test binaries have an unique basename. That way we can ensure
  # the logs are saved to unique files even when two different binaries have
  # common tests.
  unique_binaries = set(os.path.basename(binary) for binary in binaries)
  assert len(unique_binaries) == len(binaries), (
      "All test binaries must have an unique basename.")

  if options.output_dir:
    # Remove files from old test runs.
    if os.path.isdir(options.output_dir):
      shutil.rmtree(options.output_dir)
    # Create directory for test log output.
    try:
      os.makedirs(options.output_dir)
    except OSError as e:
      # Ignore errors if this directory already exists.
      if e.errno != errno.EEXIST or not os.path.isdir(options.output_dir):
        raise e

  timeout = None
  if options.timeout is not None:
    timeout = threading.Timer(options.timeout, sigint_handler.interrupt)

  test_results = None
  if options.dump_json_test_results is not None:
    test_results = CollectTestResults(options.dump_json_test_results)

  save_file = get_save_file_path()

  times = TestTimes(save_file)
  logger = FilterFormat(options.output_dir)

  task_manager = TaskManager(times, logger, test_results, Task,
                             options.retry_failed, options.repeat + 1)

  tasks = find_tests(binaries, additional_args, options, times)
  logger.log_tasks(len(tasks))
  execute_tasks(tasks, options.workers, task_manager,
                timeout, options.serialize_test_cases)

  print_try_number = options.retry_failed > 0 or options.repeat > 1
  if task_manager.passed:
    logger.move_to('passed', task_manager.passed)
    if options.print_test_times:
      logger.print_tests('PASSED TESTS', task_manager.passed, print_try_number)

  if task_manager.failed:
    logger.print_tests('FAILED TESTS', task_manager.failed, print_try_number)
    logger.move_to('failed', task_manager.failed)

  if task_manager.started:
    logger.print_tests(
        'INTERRUPTED TESTS', list(task_manager.started.values()), print_try_number)
    logger.move_to('interrupted', list(task_manager.started.values()))

  if options.repeat > 1 and (task_manager.failed or task_manager.started):
    logger.summarize(task_manager.passed, task_manager.failed,
                     list(task_manager.started.values()))

  logger.flush()
  times.write_to_file(save_file)
  if test_results:
    test_results.dump_to_file_and_close()

  if sigint_handler.got_sigint():
    return -signal.SIGINT

  return task_manager.global_exit_code

if __name__ == "__main__":
  sys.exit(main())

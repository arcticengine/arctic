// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2018 - 2020 Huldra
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#include "engine/log.h"

#include <condition_variable>  // NOLINT
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>  // NOLINT
#include <sstream>
#include <string>
#include <thread>  // NOLINT

#include "engine/mtq_mpsc_vinfarr.h"
#include "engine/arctic_platform.h"
#include "engine/arctic_platform_def.h"
#include "engine/easy_advanced.h"
#include "engine/engine.h"

#ifdef ARCTIC_PLATFORM_MACOSX
#include <os/log.h>
#endif

namespace arctic {

template <class TQueue, class TItem>
class SyncQueue{
  std::atomic<bool> is_going_to_sleep = ATOMIC_VAR_INIT(false);
  std::atomic<Ui64> wakeup_call_idx = ATOMIC_VAR_INIT(0);
  std::mutex sleep_mutex;
  std::condition_variable sleep_condvar;
  TQueue queue;

 public:
  void Enqueue(TItem *item) {
    queue.enqueue(item);
    if (is_going_to_sleep.load()) {
      {
        std::unique_lock<std::mutex> lock(sleep_mutex);
        wakeup_call_idx.fetch_add(1);
      }
      sleep_condvar.notify_one();
    }
  }

  TItem* TryDequeue() {
    return queue.dequeue();
  }

  TItem* SyncDequeue() {
    TItem *item = queue.dequeue();
    if (item) {
      return item;
    }
    is_going_to_sleep.store(true);
    {
      while (true) {
        Ui64 last_wakeup_call = wakeup_call_idx;
        item = queue.dequeue();
        if (item) {
          is_going_to_sleep.store(false);
          return item;
        }
        {
          std::unique_lock<std::mutex> lock(sleep_mutex);
          if (last_wakeup_call == wakeup_call_idx) {
            sleep_condvar.wait(lock);
          }
        }
      }
    }
  }
};

static std::atomic<bool> g_is_log_enabled = ATOMIC_VAR_INIT(false);
static SyncQueue<
  MpscVirtInfArray<std::string*, TuneDeletePayloadFlag<true>, TuneChunkSize<4000>>,
  std::string> g_logger_queue;
static std::thread g_logger_thread;
// The logger thread tells the item apart from a message by the value of the
// pointer, so the pointer has to keep it until the thread has seen it and ended.
static std::string *g_quit_item = nullptr;
static bool g_is_logger_stopping = false;
static std::mutex g_quit_mutex;
// Everything the outside world can ask of the file the logger thread owns is
// asked through these, so that no other thread ever touches the stream. A
// clear is a number rather than a flag: the thread compares it with the one it
// has already carried out, so two clears in a row are two clears.
static std::atomic<Ui64> g_log_size_limit = ATOMIC_VAR_INIT(0);
static std::atomic<Ui64> g_log_clear_requests = ATOMIC_VAR_INIT(0);
static std::mutex g_log_path_mutex;
static std::string g_log_path;

#ifndef ARCTIC_PLATFORM_WEB
static const char *kLogFileName = "log.txt";
static const char *kPreviousLogFileName = "log_prev.txt";
#endif  // ARCTIC_PLATFORM_WEB

#ifdef ARCTIC_PLATFORM_WEB
  void LoggerThreadFunction() {
    while (true) {
      std::string *message = g_logger_queue.TryDequeue();
      if (!message) {
        message = g_logger_queue.SyncDequeue();
      }
      if (message == g_quit_item) {
        delete message;
        return;
      }
      std::cout << *message << std::endl;
      delete message;
    }
  }
#else  // ARCTIC_PLATFORM_WEB
  // Opens the log and answers how much of it is already there, so that the
  // thread can keep the size without asking the stream after every write.
  static Ui64 OpenLogFile(std::ofstream *out, bool is_truncated) {
    std::ios_base::openmode mode = std::ios_base::binary | std::ios_base::out;
    mode |= is_truncated ? std::ios_base::trunc : std::ios_base::app;
    out->open(kLogFileName, mode);
    Check(!(out->rdstate() & std::ios_base::failbit),
      "Error in LoggerThreadFunction. Can't create/open the file, file_name: ",
      kLogFileName);
    out->exceptions(std::ios_base::goodbit);
    out->seekp(0, std::ios_base::end);
    std::streamoff size = out->tellp();
    if (size <= 0) {
      return 0;
    }
    return static_cast<Ui64>(size);
  }

  static void ReopenLogFile(std::ofstream *out, Ui64 *size, bool is_truncated) {
    out->flush();
    out->close();
    *size = OpenLogFile(out, is_truncated);
  }

  // The current log becomes the previous one and a new one is started. Two
  // files is the whole scheme: an older one to look back at and a newer one
  // short enough that the end of the last run is near its end.
  static void RotateLogFile(std::ofstream *out, Ui64 *size) {
    out->flush();
    out->close();
    std::remove(kPreviousLogFileName);
    std::rename(kLogFileName, kPreviousLogFileName);
    *size = OpenLogFile(out, true);
  }

  void LoggerThreadFunction() {
    const char *newline = "\r\n";
    std::ofstream out;
    Ui64 size = OpenLogFile(&out, false);
    Ui64 carried_out_clears = g_log_clear_requests.load();
    bool is_flush_needed = false;
    while (true) {
      std::string *message = g_logger_queue.TryDequeue();
      if (!message) {
        if (is_flush_needed) {
          out.flush();
          is_flush_needed = false;
        }
        message = g_logger_queue.SyncDequeue();
      }
      if (message == g_quit_item) {
        if (is_flush_needed) {
          out.flush();
        }
        out.close();
        Check(!(out.rdstate() & std::ios_base::failbit),
          "Error in LoggerThreadFunction. Can't close the file, file_name: ",
          kLogFileName);
        delete message;
        return;
      }
      const Ui64 asked_clears = g_log_clear_requests.load();
      if (asked_clears != carried_out_clears) {
        carried_out_clears = asked_clears;
        ReopenLogFile(&out, &size, true);
        is_flush_needed = false;
      }
      const Ui64 limit = g_log_size_limit.load();
      if (limit != 0 && size >= limit) {
        RotateLogFile(&out, &size);
        is_flush_needed = false;
      }
      is_flush_needed = true;
  #ifdef ARCTIC_PLATFORM_MACOSX
      os_log_info(OS_LOG_DEFAULT, "%{public}s", message->c_str());
  #endif
      out.write(message->data(), static_cast<std::streamsize>(message->size()));
      Check(!(out.rdstate() & std::ios_base::badbit),
        "Error in LoggerThreadFunction. Can't write the file, file_name: ",
        kLogFileName);
      out.write(newline, 2);
      Check(!(out.rdstate() & std::ios_base::badbit),
        "Error in LoggerThreadFunction. Can't write the file, file_name: ",
        kLogFileName);
      size += static_cast<Ui64>(message->size()) + 2;
      delete message;
    }
  }
#endif  // ARCTIC_PLATFORM_WEB


  void Log(const char *text) {
    if (g_is_log_enabled.load()) {
      std::string *str = new std::string(text);
      g_logger_queue.Enqueue(str);
    }
  }

  void Log(const char *text1, const char *text2) {
    if (g_is_log_enabled.load()) {
      std::string *str = new std::string(text1);
      str->append(text2);
      g_logger_queue.Enqueue(str);
    }
  }

  void Log(const char *text1, const char *text2, const char *text3) {
    if (g_is_log_enabled.load()) {
      std::string *str = new std::string(text1);
      str->append(text2);
      str->append(text3);
      g_logger_queue.Enqueue(str);
    }
  }

  void LogAndDelete(std::ostringstream *str) {
    Check(str, "Unexpected nullptr in LogAndDelete call");
    if (g_is_log_enabled.load()) {
      std::string *p = new std::string(str->str());
      g_logger_queue.Enqueue(p);
    }
    delete str;
  }

  std::unique_ptr<std::ostringstream, void(*)(std::ostringstream *str)> Log() {
    return std::unique_ptr<std::ostringstream, void(*)(std::ostringstream *str)>
      (new std::ostringstream, LogAndDelete);
  }

  std::string LogFilePath() {
#ifdef ARCTIC_PLATFORM_WEB
    // The log goes to the console of the browser, there is no file to name.
    return std::string();
#else
    {
      std::lock_guard<std::mutex> lock(g_log_path_mutex);
      if (!g_log_path.empty()) {
        return g_log_path;
      }
    }
    // Asked before the logger opened anything, so the answer is where it would
    // open it with the current directory as it is now.
    return CanonicalizePath(kLogFileName);
#endif
  }

  void SetLogSizeLimit(Ui64 max_bytes) {
    g_log_size_limit.store(max_bytes);
  }

  Ui64 LogSizeLimit() {
    return g_log_size_limit.load();
  }

  void ClearLog() {
    g_log_clear_requests.fetch_add(1);
    // The thread acts on the request when it takes the next message out of the
    // queue, and a request with nothing following it would sit there unseen.
    Log("Log cleared");
  }

  void LogRunHeader() {
    std::time_t now = std::time(nullptr);
    std::tm broken_down;
#ifdef ARCTIC_PLATFORM_WINDOWS
    localtime_s(&broken_down, &now);
#else
    localtime_r(&now, &broken_down);
#endif
    char stamp[64];
    if (std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S",
        &broken_down) == 0) {
      stamp[0] = '\0';
    }
    std::ostringstream header;
    header << "=== run started " << stamp << " ===";
    Log(header.str().c_str());
    std::ostringstream where;
    where << "run: log=" << LogFilePath();
    std::string current_path;
    if (GetCurrentPath(&current_path)) {
      where << " cwd=" << current_path;
    }
    where << " exe=" << GetExecutablePath();
    Log(where.str().c_str());
    std::ostringstream args;
    args << "run: args=";
    const Engine *engine = GetEngine();
    Si32 argc = engine->GetArgc();
    const char *const *argv = engine->GetArgv();
    for (Si32 i = 0; i < argc; ++i) {
      if (i != 0) {
        args << " ";
      }
      args << (argv[i] != nullptr ? argv[i] : "");
    }
    Log(args.str().c_str());
  }

  void StartLogger() {
    std::lock_guard<std::mutex> lock(g_quit_mutex);
    Check(g_quit_item == nullptr,
        "StartLogger called with g_quit_item already initialized");
    g_is_log_enabled.store(true);
    g_quit_item = new std::string("g_quit_item");
#ifndef ARCTIC_PLATFORM_WEB
    {
      // Resolved here rather than in the logger thread: the name is relative to
      // the current directory, and the current directory can be changed by the
      // application at any time afterwards, so the answer is only certain now.
      std::string path = CanonicalizePath(kLogFileName);
      std::lock_guard<std::mutex> path_lock(g_log_path_mutex);
      g_log_path = path;
    }
#endif
    g_logger_thread = std::thread(arctic::LoggerThreadFunction);
    static bool is_exit_handler_registered = false;
    if (!is_exit_handler_registered) {
      // A fatal error ends the process with exit() from wherever it was noticed,
      // and the destructor of a still joinable std::thread calls std::terminate,
      // which replaces the diagnostic the user needs with "libc++abi:
      // terminating" and an abort. Stopping the logger from an exit handler both
      // flushes what was logged on the way out and leaves nothing joinable to
      // destroy.
      std::atexit([]() { StopLogger(); });
      is_exit_handler_registered = true;
    }
    // A log is opened for appending, so without a line like this the runs of a
    // week run together and the end of the last one has to be guessed at.
    LogRunHeader();
  }

  void StopLogger() {
    std::thread logger_thread;
    {
      std::lock_guard<std::mutex> lock(g_quit_mutex);
      if (g_quit_item == nullptr || g_is_logger_stopping) {
        return;
      }
      g_is_logger_stopping = true;
      g_is_log_enabled.store(false);
      g_logger_queue.Enqueue(g_quit_item);
      logger_thread = std::move(g_logger_thread);
    }
    // The waiting happens with the mutex released. The logger thread ends the
    // process itself when it cannot write the file, its exit handler enters this
    // function again, and a mutex held across the join would leave the two
    // threads waiting for each other forever. The second call sees the stopping
    // flag and returns at once instead.
    if (logger_thread.get_id() == std::this_thread::get_id()) {
      // Asked to stop from inside the logger thread, and no thread can wait for
      // itself, so the thread is let go instead of joined.
      logger_thread.detach();
    } else {
      logger_thread.join();
    }
    std::lock_guard<std::mutex> lock(g_quit_mutex);
    // The thread deleted the item on its way out, and the next StartLogger makes
    // one of its own.
    g_quit_item = nullptr;
    g_is_logger_stopping = false;
  }
}  // namespace arctic

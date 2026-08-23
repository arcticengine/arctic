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
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <mutex>  // NOLINT
#include <sstream>
#include <string>
#include <thread>  // NOLINT

#include "engine/mtq_mpsc_vinfarr.h"
#include "engine/arctic_platform.h"
#include "engine/arctic_platform_def.h"

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
  void LoggerThreadFunction() {
    const char *file_name = "log.txt";
    const char *newline = "\r\n";
    std::ofstream out(file_name,
      std::ios_base::binary | std::ios_base::out | std::ios_base::app);
    Check(!(out.rdstate() & std::ios_base::failbit),
      "Error in LoggerThreadFunction. Can't create/open the file, file_name: ",
      file_name);
    out.exceptions(std::ios_base::goodbit);
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
          file_name);
        delete message;
        return;
      }
      is_flush_needed = true;
  #ifdef ARCTIC_PLATFORM_MACOSX
      os_log_info(OS_LOG_DEFAULT, "%{public}s", message->c_str());
  #endif
      out.write(message->data(), static_cast<std::streamsize>(message->size()));
      Check(!(out.rdstate() & std::ios_base::badbit),
        "Error in LoggerThreadFunction. Can't write the file, file_name: ",
        file_name);
      out.write(newline, 2);
      Check(!(out.rdstate() & std::ios_base::badbit),
        "Error in LoggerThreadFunction. Can't write the file, file_name: ",
        file_name);
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

  void StartLogger() {
    std::lock_guard<std::mutex> lock(g_quit_mutex);
    Check(g_quit_item == nullptr,
        "StartLogger called with g_quit_item already initialized");
    g_is_log_enabled.store(true);
    g_quit_item = new std::string("g_quit_item");
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

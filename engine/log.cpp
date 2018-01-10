// The MIT License(MIT)
//
// Copyright 2018 Huldra
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
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

#include <condition_variable>
#include <deque>
#include <iostream>
#include <mutex>  // NOLINT
#include <string>
#include <thread>  // NOLINT

#include "engine/arctic_platform.h"

namespace arctic {
  static std::mutex g_logger_mutex;
  static std::deque<std::string> g_logger_queue;
  static std::thread g_logger_thread;
  static std::atomic<bool> g_logger_do_quit = ATOMIC_VAR_INIT(true);
  static std::condition_variable g_logger_condition_variable;

  void LoggerThreadFunction() {
    bool is_present = false;
    while (true) {
      if (g_logger_do_quit) {
        // TODO(Huldra): sync file and close it
        return;
      }
      std::string message;
      {
        std::lock_guard<std::mutex> lock(g_logger_mutex);
        if (is_present) {
          g_logger_queue.pop_front();
        }
        if (!g_logger_queue.empty()) {
          is_present = true;
          message = g_logger_queue.front();
        } else {
          is_present = false;
        }
      }
      if (is_present) {
        std::cout << message << std::endl;
      } else {
        std::unique_lock<std::mutex> lock(g_logger_mutex);
        if (g_logger_queue.empty()) {
          g_logger_condition_variable.wait(lock);
        }
      }
    }
  }

  void Log(const char *text) {
    std::string string(text);
    std::lock_guard<std::mutex> lock(g_logger_mutex);
    g_logger_queue.push_back(string);
    g_logger_condition_variable.notify_one();
  }

  void StartLogger() {
    Check(g_logger_do_quit == true, "StartLogger called while g_logger_do_quit is false");
    g_logger_do_quit = false;
    g_logger_thread = std::thread(arctic::LoggerThreadFunction);
  }

  void StopLogger() {
    Check(g_logger_do_quit == false, "StopLogger called while g_logger_do_quit is true");
    g_logger_do_quit = true;
  }
}  // namespace arctic

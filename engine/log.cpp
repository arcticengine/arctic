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
#include <fstream>
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
    const char *file_name = "log.txt";
    const char *newline = "\r\n";
    std::ofstream out(file_name,
      std::ios_base::binary | std::ios_base::out | std::ios_base::app);
    Check(out.rdstate() != std::ios_base::failbit,
      "Error in LoggerThreadFunction. Can't create/open the file, file_name: ",
      file_name);
    out.exceptions(std::ios_base::goodbit);
    bool is_present = false;
    while (true) {
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
        out.write(message.data(), message.size());
        Check(!(out.rdstate() & std::ios_base::badbit),
          "Error in LoggerThreadFunction. Can't write the file, file_name: ",
          file_name);
        out.write(newline, 2);
        Check(!(out.rdstate() & std::ios_base::badbit),
          "Error in LoggerThreadFunction. Can't write the file, file_name: ",
          file_name);
      } else {
        if (g_logger_do_quit) {
          out.close();
          Check(!(out.rdstate() & std::ios_base::failbit),
            "Error in LoggerThreadFunction. Can't close the file, file_name: ",
            file_name);
          return;
        }
        out.flush();
        std::unique_lock<std::mutex> lock(g_logger_mutex);
        if (g_logger_queue.empty()) {
          g_logger_condition_variable.wait(lock);
        }
      }
    }
  }

  void PushLog(std::string &str) {
    std::lock_guard<std::mutex> lock(g_logger_mutex);
    g_logger_queue.push_back(str);
    g_logger_condition_variable.notify_one();
  }

  void Log(const char *text) {
    std::string str(text);
    PushLog(str);
  }

  void Log(const char *text1, const char *text2) {
    std::string str(text1);
    str.append(text2);
    PushLog(str);
  }

  void Log(const char *text1, const char *text2, const char *text3) {
    std::string str(text1);
    str.append(text2);
    str.append(text3);
    PushLog(str);
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

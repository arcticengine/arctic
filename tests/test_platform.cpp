// The platform layer: files and paths, sockets, the startup mode, the log, the
// window title and the close handler.
#define TEST_NO_MAIN
#include "test_helpers.h"

void test_file_operations() {
  std::vector<DirectoryEntry> list;
  std::string canonic = arctic::CanonicalizePath("./..");
  
  std::string arctic_engine_dir = "../engine";
  bool is_engine_dir_found = false;
  for (Si32 i = 0; i < 10; ++i) {
    if (arctic::DoesDirectoryExist(arctic_engine_dir.c_str())
        == kTrivalentTrue) {
      is_engine_dir_found = true;
      break;
    }
    arctic_engine_dir = std::string("../") + arctic_engine_dir;
  }
  if (!is_engine_dir_found) {
    TEST_MSG("skipped: no engine directory above the binary");
    return;
  }

  std::string arctic_root_dir = arctic_engine_dir + std::string("/..");

  std::string relative = RelativePathFromTo(arctic_root_dir.c_str(),
      arctic_engine_dir.c_str());
  TEST_CHECK_(relative == std::string("./engine"), "relative: %s", relative.c_str());

  std::string relative2 = RelativePathFromTo(arctic_engine_dir.c_str(),
                                             arctic_root_dir.c_str());
  TEST_CHECK_(relative2 == std::string("../"), "relative2: %s", relative2.c_str());
  
  bool isok = GetDirectoryEntries(arctic_engine_dir.c_str(), &list);
  TEST_CHECK(isok);
  TEST_CHECK(list.size() > 0);
}

// A path from argv means "relative to where the user was standing", and the
// current directory is not that place: on macOS the engine makes the resources
// folder of the bundle current before EasyMain. So the helper must resolve
// against the startup directory and must keep doing that after the current
// directory moves again.
void test_canonicalize_argv_path() {
  const std::string startup = arctic::GetStartupDirectory();
  TEST_CHECK_(!startup.empty(),
      "the startup directory was not remembered");
  TEST_CHECK_(startup == arctic::CanonicalizePath(startup.c_str()),
      "the startup directory is not canonical: '%s'", startup.c_str());

  const std::string relative = arctic::CanonicalizeArgvPath("snaps/world.dcs");
  const std::string expected = arctic::CanonicalizePath(
      arctic::GluePath(startup.c_str(), "snaps/world.dcs").c_str());
  TEST_CHECK_(relative == expected,
      "relative argv path resolved to '%s', expected '%s'",
      relative.c_str(), expected.c_str());

  // An absolute path is nobody's business but its own.
  const std::string absolute = arctic::CanonicalizeArgvPath(
      arctic::GluePath(startup.c_str(), "snaps/world.dcs").c_str());
  TEST_CHECK_(absolute == expected,
      "absolute argv path changed to '%s', expected '%s'",
      absolute.c_str(), expected.c_str());

  // Empty in, empty out: the caller can tell "no path given" from a path.
  TEST_CHECK(arctic::CanonicalizeArgvPath("").empty());
  TEST_CHECK(arctic::CanonicalizeArgvPath(nullptr).empty());

  // The whole point: the answer does not follow the current directory. This is
  // the situation a bundled application is in from the very first line of
  // EasyMain, and the reason CanonicalizePath alone is not enough.
  std::string current_before;
  TEST_CHECK(arctic::GetCurrentPath(&current_before));
  const std::string parent = arctic::CanonicalizePath(
      arctic::GluePath(current_before.c_str(), "..").c_str());
  if (parent != current_before && parent != startup) {
    TEST_CHECK(arctic::ChangeCurrentDirectory(parent.c_str()));
    const std::string moved = arctic::CanonicalizeArgvPath("snaps/world.dcs");
    const std::string plain = arctic::CanonicalizePath("snaps/world.dcs");
    TEST_CHECK(arctic::ChangeCurrentDirectory(current_before.c_str()));
    TEST_CHECK_(moved == expected,
        "after a chdir the same argv path resolved to '%s', expected '%s'",
        moved.c_str(), expected.c_str());
    TEST_CHECK_(plain != expected,
        "CanonicalizePath happened to give the same answer, so this test "
        "proves nothing: '%s'", plain.c_str());
  }
}

// "The file does not exist" is not a diagnosis. Whatever else the description
// says, it has to name the path as given and the absolute path behind it, and it
// has to point at the startup directory when the file is sitting right there.
void test_describe_file_path() {
  const std::string missing =
      arctic::DescribeFilePath("data/no_such_file_41287.tga");
  TEST_CHECK_(missing.find("data/no_such_file_41287.tga") != std::string::npos,
      "the description lost the path as given: '%s'", missing.c_str());
  const std::string absolute =
      arctic::CanonicalizePath("data/no_such_file_41287.tga");
  TEST_CHECK_(missing.find(absolute) != std::string::npos,
      "the description lost the absolute path '%s': '%s'",
      absolute.c_str(), missing.c_str());

  std::string current;
  TEST_CHECK(arctic::GetCurrentPath(&current));
  TEST_CHECK_(missing.find(current) != std::string::npos,
      "the description does not say what a relative path was resolved "
      "against: '%s'", missing.c_str());

  // An absolute path needs no directory to be resolved against, so the current
  // directory would only be noise there.
  const std::string absolute_missing = arctic::DescribeFilePath(
      arctic::GluePath(current.c_str(), "no_such_file_41287.tga").c_str());
  TEST_CHECK_(absolute_missing.find("current directory") == std::string::npos,
      "an absolute path was described through the current directory: '%s'",
      absolute_missing.c_str());

  const std::string deep =
      arctic::DescribeFilePath("no_such_dir_41287/file.txt");
  TEST_CHECK_(deep.find("does not exist") != std::string::npos,
      "a missing parent directory was not reported: '%s'", deep.c_str());

  // The trap the description exists for: the file is where the user typed it,
  // and the engine looked for it next to the assets.
  const std::string startup = arctic::GetStartupDirectory();
  if (!startup.empty() && startup != current) {
    const char *name = "test_startup_hint_41287.txt";
    const std::string in_startup =
        arctic::GluePath(startup.c_str(), name);
    {
      std::ofstream ofs(in_startup.c_str());
      TEST_CHECK_(ofs.good(), "failed to create '%s'", in_startup.c_str());
      ofs << "test";
    }
    const std::string hinted = arctic::DescribeFilePath(name);
    std::remove(in_startup.c_str());
    TEST_CHECK_(hinted.find(in_startup) != std::string::npos,
        "the description does not mention the file in the startup directory: "
        "'%s'", hinted.c_str());
    TEST_CHECK_(hinted.find("CanonicalizeArgvPath") != std::string::npos,
        "the description does not name the way out: '%s'", hinted.c_str());
  }
}

// DoesFileExist has to tell a file from a directory and from nothing at all,
// because "it exists" and "it is a file I can read" are different answers.
// ChangeCurrentDirectory is checked together with it: a test that moves the
// current directory has to be able to move it back.
void test_file_existence_and_current_directory() {
  std::string current;
  TEST_CHECK(arctic::GetCurrentPath(&current));

  TEST_CHECK(arctic::DoesFileExist("data/no_such_file_41287.tga")
      == kTrivalentFalse);
  // A directory is not a file, and saying "false" here would hide the reason a
  // read of such a path fails.
  TEST_CHECK(arctic::DoesFileExist(current.c_str()) == kTrivalentUnknown);

  const std::string temp = arctic::GluePath(current.c_str(),
      "test_file_exists_41287.txt");
  {
    std::ofstream ofs(temp.c_str());
    TEST_CHECK_(ofs.good(), "failed to create '%s'", temp.c_str());
    ofs << "test";
  }
  const Trivalent created = arctic::DoesFileExist(temp.c_str());
  std::remove(temp.c_str());
  TEST_CHECK_(created == kTrivalentTrue,
      "a file that was just created is not seen at '%s'", temp.c_str());
  TEST_CHECK(arctic::DoesFileExist(temp.c_str()) == kTrivalentFalse);

  TEST_CHECK(!arctic::ChangeCurrentDirectory(""));
  TEST_CHECK(!arctic::ChangeCurrentDirectory(nullptr));
  TEST_CHECK(!arctic::ChangeCurrentDirectory("no_such_dir_41287"));

  const std::string parent = arctic::CanonicalizePath(
      arctic::GluePath(current.c_str(), "..").c_str());
  if (parent != current) {
    TEST_CHECK(arctic::ChangeCurrentDirectory(parent.c_str()));
    std::string moved;
    TEST_CHECK(arctic::GetCurrentPath(&moved));
    TEST_CHECK(arctic::ChangeCurrentDirectory(current.c_str()));
    TEST_CHECK_(moved == parent,
        "the current directory moved to '%s', expected '%s'",
        moved.c_str(), parent.c_str());
    std::string back;
    TEST_CHECK(arctic::GetCurrentPath(&back));
    TEST_CHECK_(back == current,
        "the current directory was left at '%s', expected '%s'",
        back.c_str(), current.c_str());
  }
}

// Two copies of one program are told apart by the title of the window and by
// nothing else, so the default is the name of the program rather than the name
// of the engine, and an application can say something of its own there. Whether
// the platform showed it cannot be read back from the system, so what is
// checked here is the value the engine keeps and hands to the window.
void test_window_title_names_the_program() {
  const std::string default_title = WindowTitle();
  TEST_CHECK_(!default_title.empty(), "the default window title is empty");

  const std::string executable = GetExecutablePath();
  if (!executable.empty()) {
    const size_t slash = executable.find_last_of("/\\");
    const std::string file_name = (slash == std::string::npos)
      ? executable : executable.substr(slash + 1);
    TEST_CHECK_(default_title == file_name,
        "the default title is '%s' while the program is '%s'",
        default_title.c_str(), file_name.c_str());
    TEST_CHECK_(default_title != "Arctic Engine",
        "the default title still names the engine instead of the program");
    TEST_CHECK_(default_title.find('/') == std::string::npos,
        "the default title is a whole path, '%s'", default_title.c_str());
  }

  const std::string mine = "Hover Racer \xe2\x80\x94 player 2";
  SetWindowTitle(mine.c_str());
  TEST_CHECK_(WindowTitle() == mine,
      "the title set to '%s' reads back as '%s'", mine.c_str(),
      WindowTitle().c_str());

  // An empty title, and a missing one, mean the default rather than a nameless
  // window; a program that has nothing to say stays recognisable.
  SetWindowTitle("");
  TEST_CHECK_(WindowTitle() == default_title,
      "an empty title gave '%s' instead of the default '%s'",
      WindowTitle().c_str(), default_title.c_str());
  SetWindowTitle(mine.c_str());
  SetWindowTitle(nullptr);
  TEST_CHECK_(WindowTitle() == default_title,
      "a null title gave '%s' instead of the default '%s'",
      WindowTitle().c_str(), default_title.c_str());
}

namespace {

// A port nothing else is using, so a socket test does not fail because some
// other program on the machine got to the port first.
bool BindLoopbackListener(ListenerSocket *listener, uint16_t *out_port) {
  for (uint16_t port = 34567; port < 34667; ++port) {
    ListenerSocket candidate(AddressFamily::kIpV4, SocketProtocol::kTcp);
    if (!candidate.IsValid()) {
      return false;
    }
    if (candidate.Bind("127.0.0.1", port) == SocketResult::kSocketOk) {
      *listener = std::move(candidate);
      *out_port = port;
      return true;
    }
  }
  return false;
}

// Waits a while for a connection instead of blocking for good: the listener is
// non-blocking, so an Accept with nobody waiting comes back invalid.
ConnectionSocket AcceptWithin(const ListenerSocket &listener, double seconds) {
  const double deadline = Time() + seconds;
  while (Time() < deadline) {
    ConnectionSocket accepted = listener.Accept();
    if (accepted.IsValid()) {
      return accepted;
    }
    Sleep(0.002);
  }
  return ConnectionSocket();
}

// Reads until the bytes expected have arrived, the connection says something
// other than "ok", or the time runs out.
SocketResult ReadWithin(ConnectionSocket *socket, size_t expected_size,
    double seconds, std::string *out_text) {
  out_text->clear();
  const double deadline = Time() + seconds;
  while (out_text->size() < expected_size && Time() < deadline) {
    char buffer[64];
    size_t read_size = 0;
    const SocketResult result = socket->Read(buffer, sizeof(buffer),
        &read_size);
    if (result != SocketResult::kSocketOk) {
      return result;
    }
    if (read_size == 0) {
      Sleep(0.002);
      continue;
    }
    out_text->append(buffer, read_size);
  }
  return SocketResult::kSocketOk;
}

// A short message still goes out in pieces if the socket feels like it.
bool WriteAll(ConnectionSocket *socket, const std::string &text) {
  size_t total = 0;
  const double deadline = Time() + 1.0;
  while (total < text.size() && Time() < deadline) {
    size_t written = 0;
    if (socket->Write(text.data() + total, text.size() - total, &written)
        != SocketResult::kSocketOk) {
      return false;
    }
    total += written;
  }
  return total == text.size();
}

}  // namespace

// The whole life of a connection over the loopback interface, because a server
// example is worth nothing if these do not hold: a port is bound once and the
// second attempt is refused, a connection is accepted, both ends can write and
// read, closing one end is visible at the other, and a connect to a port
// nobody listens on fails instead of hanging.
void test_loopback_sockets_connect_talk_and_close() {
  ListenerSocket listener;
  uint16_t port = 0;
  TEST_CHECK_(BindLoopbackListener(&listener, &port),
      "no port in the range could be bound on 127.0.0.1: %s",
      listener.GetLastError().c_str());
  if (!listener.IsValid()) {
    return;
  }
  TEST_CHECK(listener.SetSoNonblocking(true) == SocketResult::kSocketOk);

  // The port is taken now, and the engine has to say so rather than quietly
  // producing a second listener that receives nothing.
  ListenerSocket intruder(AddressFamily::kIpV4, SocketProtocol::kTcp);
  TEST_CHECK_(intruder.Bind("127.0.0.1", port) == SocketResult::kSocketError,
      "binding port %d a second time was allowed", static_cast<int>(port));
  TEST_CHECK_(!intruder.GetLastError().empty(),
      "the refused bind left no explanation in GetLastError");

  ConnectionSocket client(AddressFamily::kIpV4, SocketProtocol::kTcp);
  TEST_CHECK_(client.IsValid(), "no socket: %s", client.GetLastError().c_str());
  const SocketConnectResult connected = client.Connect("127.0.0.1", port);
  TEST_CHECK_(connected == SocketConnectResult::kSocketOk,
      "connect to the listening port answered %d: %s",
      static_cast<int>(connected), client.GetLastError().c_str());
  TEST_CHECK_(client.GetState() == SocketState::kConnected,
      "a connected socket does not report kConnected");
  TEST_CHECK(client.SetSoNonblocking(true) == SocketResult::kSocketOk);

  ConnectionSocket served = AcceptWithin(listener, 2.0);
  TEST_CHECK_(served.IsValid(),
      "the listener never handed over the connection: %s",
      listener.GetLastError().c_str());
  if (!served.IsValid()) {
    return;
  }
  TEST_CHECK_(served.GetState() == SocketState::kConnected,
      "an accepted connection does not report kConnected");
  TEST_CHECK(served.SetSoNonblocking(true) == SocketResult::kSocketOk);

  TEST_CHECK_(WriteAll(&client, "ping"), "the client could not write: %s",
      client.GetLastError().c_str());
  std::string question;
  const SocketResult got_question = ReadWithin(&served, 4, 2.0, &question);
  TEST_CHECK_(got_question == SocketResult::kSocketOk,
      "reading the request answered %d: %s", static_cast<int>(got_question),
      served.GetLastError().c_str());
  TEST_CHECK_(question == "ping", "the server got '%s' instead of 'ping'",
      question.c_str());

  TEST_CHECK_(WriteAll(&served, "pong-42"), "the server could not write: %s",
      served.GetLastError().c_str());
  std::string answer;
  const SocketResult got_answer = ReadWithin(&client, 7, 2.0, &answer);
  TEST_CHECK_(got_answer == SocketResult::kSocketOk,
      "reading the answer answered %d: %s", static_cast<int>(got_answer),
      client.GetLastError().c_str());
  TEST_CHECK_(answer == "pong-42", "the client got '%s' instead of 'pong-42'",
      answer.c_str());

  // The other end goes away. This has to be told apart from "no data yet",
  // which is what a server loop hangs on when it is not.
  {
    ConnectionSocket closing = std::move(client);
  }
  std::string nothing;
  const SocketResult after_close = ReadWithin(&served, 1, 2.0, &nothing);
  TEST_CHECK_(after_close == SocketResult::kSocketConnectionReset,
      "reading a connection closed by the other end answered %d, and "
      "kSocketConnectionReset (%d) was expected", static_cast<int>(after_close),
      static_cast<int>(SocketResult::kSocketConnectionReset));
  TEST_CHECK_(nothing.empty(), "the closed connection produced '%s'",
      nothing.c_str());
  TEST_CHECK_(!served.IsValid(),
      "a connection the other end closed is still valid");
  TEST_CHECK_(served.GetState() == SocketState::kDisconnected,
      "a connection the other end closed does not report kDisconnected");

  // And with the listener gone, the same port refuses a connection.
  {
    ListenerSocket closing = std::move(listener);
  }
  ConnectionSocket hopeful(AddressFamily::kIpV4, SocketProtocol::kTcp);
  const SocketConnectResult refused = hopeful.Connect("127.0.0.1", port);
  TEST_CHECK_(refused == SocketConnectResult::kSocketError,
      "connecting to port %d with nothing listening answered %d",
      static_cast<int>(port), static_cast<int>(refused));
  TEST_CHECK_(!hopeful.GetLastError().empty(),
      "the refused connect left no explanation in GetLastError");
  TEST_CHECK_(hopeful.GetState() == SocketState::kDisconnected,
      "a socket that failed to connect does not report kDisconnected");
}

// How a run starts is decided before main gets going, from a function the
// application registers. The registration is the only part of it a test can
// reach; the startup code that asks the question runs once, before this.
void test_startup_mode_decider() {
  const bool env_hides = std::getenv("ARCTIC_HEADLESS") != nullptr;
  const bool env_asks_no_window =
      env_hides && std::getenv("ARCTIC_DISABLE_HW") != nullptr;
  if (!env_hides) {
    // This binary registers TestsStartupMode in main.cpp, and that is what has to
    // be heard while the environment is silent.
    TEST_CHECK_(arctic::RequestedStartupMode()
            == arctic::StartupMode::kHiddenWindow,
        "the suite's own decider asks for a hidden window and a clean "
        "environment must hear it, got %d",
        static_cast<int>(arctic::RequestedStartupMode()));
  }

  // The environment wins over the decider, so a run can always be hidden from
  // the outside: that is how the self-tests of a game are run.
  TEST_CHECK(arctic::SetStartupModeDecider(
      []() { return arctic::StartupMode::kNoWindow; }));
  if (!env_hides) {
    TEST_CHECK(arctic::IsHeadlessStartupRequested());
    TEST_CHECK(arctic::RequestedStartupMode()
        == arctic::StartupMode::kNoWindow);
  }

  // A hidden window is its own answer, and must not be mistaken for the mode
  // that has no window and no GL context at all.
  arctic::SetStartupModeDecider(
      []() { return arctic::StartupMode::kHiddenWindow; });
  if (!env_hides) {
    TEST_CHECK_(arctic::RequestedStartupMode()
            == arctic::StartupMode::kHiddenWindow,
        "a decider asking for a hidden window must be heard, got %d",
        static_cast<int>(arctic::RequestedStartupMode()));
    TEST_CHECK_(!arctic::IsHeadlessStartupRequested(),
        "a hidden window still has a window and a GL context, so it is not a "
        "headless start");
  }

  // With no decider at all a clean environment means an ordinary window.
  arctic::SetStartupModeDecider(nullptr);
  if (!env_hides) {
    TEST_CHECK_(arctic::RequestedStartupMode()
            == arctic::StartupMode::kWindowed,
        "a binary with no decider and a clean environment must start "
        "windowed, got %d", static_cast<int>(arctic::RequestedStartupMode()));
  } else if (env_asks_no_window) {
    TEST_CHECK(arctic::RequestedStartupMode()
        == arctic::StartupMode::kNoWindow);
  } else {
    TEST_CHECK_(arctic::RequestedStartupMode()
            == arctic::StartupMode::kHiddenWindow,
        "ARCTIC_HEADLESS on its own means a hidden window, got %d",
        static_cast<int>(arctic::RequestedStartupMode()));
  }

  // Leave the suite's own decider in place for whatever asks next.
  arctic::SetStartupModeDecider(TestsStartupMode);
}

namespace {

// The logger writes from a thread of its own and flushes once its queue runs
// dry, so a test that wants to read what it wrote has to give it a moment.
// Waiting for a condition rather than for a fixed time keeps the test both
// quick when the machine is idle and sound when it is not.
bool WaitForLogFile(const std::string &path,
    const std::function<bool(const std::string &)> &is_as_wanted,
    std::string *out_text) {
  for (Si32 attempt = 0; attempt < 200; ++attempt) {
    std::ifstream file(path, std::ios_base::binary);
    std::ostringstream text;
    if (file.is_open()) {
      text << file.rdbuf();
    }
    *out_text = text.str();
    if (is_as_wanted(*out_text)) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return false;
}

bool LogFileHolds(const std::string &text, const std::string &part) {
  return text.find(part) != std::string::npos;
}

}  // namespace

// Two runs of a game land in the same log, and a long session grows it past the
// point where the end of the last run can be found by eye. Both are answered
// here: a run says in the log where and when it started, a run can throw away
// what came before it, and a size limit keeps the newer half short.
void test_log_file_is_findable_clearable_and_rotated() {
  const std::string path = arctic::LogFilePath();
  TEST_CHECK_(!path.empty(), "the log file has no path");
  TEST_CHECK_(path.size() > 8
          && path.compare(path.size() - 8, 8, "/log.txt") == 0,
      "the log path '%s' does not name log.txt", path.c_str());
  TEST_CHECK_(path[0] == '/', "the log path '%s' is not absolute",
      path.c_str());

  const std::string previous_path =
      path.substr(0, path.size() - 8) + "/log_prev.txt";
  const Ui64 limit_before = arctic::LogSizeLimit();
  std::remove(previous_path.c_str());

  // A clear has to be a clear: a marker logged before it must be gone, and one
  // logged after it must be there.
  arctic::SetLogSizeLimit(0);
  *arctic::Log() << "test_log before the clear 0123456789";
  std::string text;
  TEST_CHECK_(WaitForLogFile(path, [](const std::string &t) {
        return LogFileHolds(t, "test_log before the clear");
      }, &text), "the line logged before the clear never reached the file");
  arctic::ClearLog();
  *arctic::Log() << "test_log after the clear 0123456789";
  TEST_CHECK_(WaitForLogFile(path, [](const std::string &t) {
        return LogFileHolds(t, "test_log after the clear");
      }, &text), "the line logged after the clear never reached the file");
  TEST_CHECK_(!LogFileHolds(text, "test_log before the clear"),
      "the clear left the older lines in a %d byte log",
      static_cast<Si32>(text.size()));

  // Rotation: with a small limit, enough lines push the older ones out into
  // log_prev.txt, and what is left is short and ends with the newest line.
  arctic::LogRunHeader();
  arctic::SetLogSizeLimit(4096);
  for (Si32 i = 0; i < 400; ++i) {
    *arctic::Log() << "test_log filler line " << i
      << " 0123456789 0123456789 0123456789 0123456789";
  }
  *arctic::Log() << "test_log last line after rotation";
  TEST_CHECK_(WaitForLogFile(path, [](const std::string &t) {
        return LogFileHolds(t, "test_log last line after rotation");
      }, &text), "the last line never reached the file");
  TEST_CHECK_(text.size() < 4096 + 4096,
      "the log grew to %d bytes with a 4096 byte limit",
      static_cast<Si32>(text.size()));
  TEST_CHECK_(!LogFileHolds(text, "test_log filler line 0 "),
      "the first filler line is still in a log that was rotated");
  // Rotation keeps two files, so the filler that no longer fits is in the
  // previous one -- not necessarily the very first line of it, because that
  // many lines rotate the log more than once and only the last two files are
  // kept, which is the whole point of the limit.
  std::string previous_text;
  TEST_CHECK_(WaitForLogFile(previous_path, [](const std::string &t) {
        return LogFileHolds(t, "test_log filler line ");
      }, &previous_text),
      "the lines pushed out of the log did not end up in log_prev.txt");
  TEST_CHECK_(!LogFileHolds(previous_text, "test_log last line after rotation"),
      "the newest line ended up in log_prev.txt as well");

  arctic::SetLogSizeLimit(limit_before);
  arctic::ClearLog();
  arctic::LogRunHeader();
  TEST_CHECK_(WaitForLogFile(path, [](const std::string &t) {
        return LogFileHolds(t, "=== run started ");
      }, &text), "the run header is not written to a fresh log");
  TEST_CHECK_(LogFileHolds(text, "run: log=" + path),
      "the run header does not tell where the log is: %s", text.c_str());
}

namespace {

Si32 g_close_handler_calls = 0;
bool g_close_handler_answer = false;

bool CountingCloseHandler() {
  ++g_close_handler_calls;
  return g_close_handler_answer;
}

}  // namespace

// The window close arrives from the system, so a test stands in for it by calling
// what the platform code calls. Without a handler the answer is "close now",
// which is what every application had before the handler existed; with one, the
// application decides, and the flag stays raised either way so that a main loop
// can end when it is ready.
void test_main_window_close_handler() {
  TEST_CHECK_(!arctic::IsMainWindowCloseRequested(),
      "the close flag was raised before anybody asked to close");

  arctic::SetMainWindowCloseHandler(nullptr);
  TEST_CHECK_(arctic::OnMainWindowCloseRequested(),
      "a run with no handler refused to close");
  TEST_CHECK_(arctic::IsMainWindowCloseRequested(),
      "the request did not raise the flag");

  g_close_handler_calls = 0;
  g_close_handler_answer = false;
  arctic::SetMainWindowCloseHandler(CountingCloseHandler);
  TEST_CHECK_(!arctic::OnMainWindowCloseRequested(),
      "the engine closed although the handler took the closing over");
  TEST_CHECK_(g_close_handler_calls == 1, "the handler was called %d times",
      (int)g_close_handler_calls);
  TEST_CHECK_(arctic::IsMainWindowCloseRequested(),
      "the flag went down after a refused close");

  g_close_handler_answer = true;
  TEST_CHECK_(arctic::OnMainWindowCloseRequested(),
      "the handler said yes and the engine did not close");
  TEST_CHECK_(g_close_handler_calls == 2,
      "the second request called the handler %d times in total",
      (int)g_close_handler_calls);

  arctic::SetMainWindowCloseHandler(nullptr);
}

void test_canonicalize_nonexistent_path() {
  std::string via_nonexistent =
      arctic::CanonicalizePath("./nonexistent_dir_82736/../..");
  std::string via_existing =
      arctic::CanonicalizePath("./..");

  TEST_CHECK_(!via_nonexistent.empty(),
      "CanonicalizePath must not return empty for non-existent path");
  TEST_CHECK_(!via_existing.empty(),
      "CanonicalizePath must not return empty for existing path");
  TEST_CHECK_(via_nonexistent == via_existing,
      "Paths must match: '%s' vs '%s'",
      via_nonexistent.c_str(), via_existing.c_str());

  std::string with_dot = arctic::CanonicalizePath("./.");
  std::string without_dot = arctic::CanonicalizePath(".");
  TEST_CHECK_(with_dot == without_dot,
      "Single dot must collapse: '%s' vs '%s'",
      with_dot.c_str(), without_dot.c_str());

  std::string with_dots = arctic::CanonicalizePath("././.");
  TEST_CHECK_(with_dots == without_dot,
      "Multiple dots must collapse: '%s' vs '%s'",
      with_dots.c_str(), without_dot.c_str());

  std::string into_and_back =
      arctic::CanonicalizePath("./nonexistent_abc_99/..");
  std::string just_here = arctic::CanonicalizePath(".");
  TEST_CHECK_(into_and_back == just_here,
      "Enter and exit dir must equal current: '%s' vs '%s'",
      into_and_back.c_str(), just_here.c_str());

  std::string deep_into_and_back =
      arctic::CanonicalizePath("./aaa_fake/bbb_fake/../..");
  TEST_CHECK_(deep_into_and_back == just_here,
      "Enter two dirs and exit both must equal current: '%s' vs '%s'",
      deep_into_and_back.c_str(), just_here.c_str());
}

void test_canonicalize_before_and_after_create() {
  const char *filename = "./test_canon_tmpfile_93721";

  std::remove(filename);

  std::string before = arctic::CanonicalizePath(filename);
  TEST_CHECK_(!before.empty(),
      "CanonicalizePath must not return empty before file exists");

  {
    std::ofstream ofs(filename);
    TEST_CHECK_(ofs.good(), "Failed to create temp file '%s'", filename);
    ofs << "test";
  }

  std::string after = arctic::CanonicalizePath(filename);
  TEST_CHECK_(!after.empty(),
      "CanonicalizePath must not return empty after file exists");

  TEST_CHECK_(before == after,
      "Canonical path before ('%s') and after ('%s') file creation must match",
      before.c_str(), after.c_str());

  std::remove(filename);
}

// RelativePathFromTo writes the path of every engine file into the project
// files the wizard makes, so an answer that is one ".." off gives a project
// that builds nowhere. A directory can be named with a trailing slash or
// without one; both name the same place, and the two spellings have to give the
// same way out of it.
void test_relative_path_from_to() {
#ifndef ARCTIC_PLATFORM_WINDOWS
  struct Case {
    const char *from;
    const char *to;
    const char *expected;
  };
  static const Case kCases[] = {
    // A sibling of the directory the path starts from: one step up, one down.
    {"/pet/arctic/tests", "/pet/arctic/engine/font.cpp", "../engine/font.cpp"},
    {"/pet/arctic/tests/", "/pet/arctic/engine/font.cpp", "../engine/font.cpp"},
    // Three directories deep, and the trailing slash still counts for nothing.
    {"/a/b/c/d", "/a/x/y.txt", "../../../x/y.txt"},
    {"/a/b/c/d/", "/a/x/y.txt", "../../../x/y.txt"},
    // Nothing in common but the root.
    {"/a/b", "/x/y", "../../x/y"},
    {"/a/b/", "/x/y", "../../x/y"},
    // A file right inside the directory needs no climb at all.
    {"/a/b", "/a/b/main.cpp", "./main.cpp"},
    {"/a/b/", "/a/b/main.cpp", "main.cpp"},
    {"/a/b/", "/a/b/sub/main.cpp", "sub/main.cpp"},
    // The parent is one step up, in all four spellings of the pair.
    {"/a/b/c", "/a/b", "../"},
    {"/a/b/c/", "/a/b/", "../"},
    {"/a/b/c/", "/a/b", "../"},
    {"/a/b/c", "/a/b/", "../"},
    // The same place is "./", again in all four spellings.
    {"/a/b", "/a/b", "./"},
    {"/a/b/", "/a/b/", "./"},
    {"/a/b/", "/a/b", "./"},
    {"/a/b", "/a/b/", "./"},
    // Names that share a prefix of letters are still different names, and the
    // common part has to be cut back to the last whole directory.
    {"/a/bc", "/a/bcd/f", "../bcd/f"},
    {"/a/bcd", "/a/bc", "../bc"},
    // A "." or a ".." inside the path is resolved before anything else.
    {"/a/./b", "/a/c/../c/f", "../c/f"},
  };
  const size_t case_count = sizeof(kCases) / sizeof(kCases[0]);
  for (size_t idx = 0; idx < case_count; ++idx) {
    const Case &one = kCases[idx];
    const std::string actual = RelativePathFromTo(one.from, one.to);
    TEST_CHECK_(actual == std::string(one.expected),
        "RelativePathFromTo(\"%s\", \"%s\") gave \"%s\", expected \"%s\"",
        one.from, one.to, actual.c_str(), one.expected);
  }
#endif

  // The same rule on paths this machine spells itself, so the check holds on
  // Windows too, where the separator and the whole implementation differ: from
  // a directory to a file in a sibling of it there is exactly one step up, and
  // naming that directory with a trailing separator must not add a second one.
  const std::string base = arctic::CanonicalizePath(".");
  TEST_CHECK_(!base.empty(), "the current directory has no canonical form");
  const char separator =
      (base.find('\\') != std::string::npos) ? '\\' : '/';
  const std::string from_plain = arctic::GluePath(base.c_str(), "one_dir");
  const std::string from_slashed = from_plain + separator;
  const std::string target = arctic::GluePath(
      arctic::GluePath(base.c_str(), "two_dir").c_str(), "file.txt");

  const std::string plain = RelativePathFromTo(from_plain.c_str(),
      target.c_str());
  const std::string slashed = RelativePathFromTo(from_slashed.c_str(),
      target.c_str());
  TEST_CHECK_(plain == slashed,
      "a trailing separator changed the answer: \"%s\" became \"%s\"",
      plain.c_str(), slashed.c_str());

  for (Si32 variant = 0; variant < 2; ++variant) {
    const std::string &answer = (variant == 0) ? plain : slashed;
    size_t up_count = 0;
    for (size_t i = 0; i + 1 < answer.size(); ++i) {
      if (answer[i] == '.' && answer[i + 1] == '.') {
        ++up_count;
        ++i;
      }
    }
    TEST_CHECK_(up_count == 1,
        "\"%s\" climbs %d directories, a sibling is exactly one up",
        answer.c_str(), static_cast<int>(up_count));
    TEST_CHECK_(answer.find("two_dir") != std::string::npos,
        "\"%s\" does not lead to the directory that was asked for",
        answer.c_str());
    TEST_CHECK_(answer.find("one_dir") == std::string::npos,
        "\"%s\" still holds the directory it was supposed to leave",
        answer.c_str());
  }
}

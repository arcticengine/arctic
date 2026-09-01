// A dedicated server, and the client that talks to it, in one executable.
//
// This is the smallest complete answer to "how do I run Arctic without a
// window": a startup mode decider that asks for kNoWindow, a ListenerSocket on
// a port, a loop paced by Time() and Sleep() rather than by a display, and an
// end condition that does not involve a keyboard. Run it in two terminals:
//
//   headless_server.app/Contents/MacOS/headless_server --port 21112
//   headless_server.app/Contents/MacOS/headless_server --client --port 21112
//
// The server answers `ping`, reports its tick count, ends on `stop`, and gives
// up on its own after a budget of ticks so that a forgotten process does not
// live forever. The client sends all of that and reports whether the answers
// were the expected ones, which makes the pair runnable as a smoke test.

#include "engine/easy.h"
// The sockets are not part of easy.h, they come from their own header.
#include "engine/arctic_platform_tcpip.h"

#include <cstdio>
#include <cstdlib>
#include <deque>
#include <string>
#include <vector>

using namespace arctic;  // NOLINT

namespace {

const uint16_t kDefaultPort = 21112;
const double kTickDuration = 1.0 / 60.0;
const Si32 kDefaultTickBudget = 3600;  // a minute at sixty ticks a second

struct Options {
  bool is_client = false;
  uint16_t port = kDefaultPort;
  Si32 tick_budget = kDefaultTickBudget;
};

// Xcode appends `-NSDocumentRevisionsDebugMode YES` to a Debug Run and Finder
// used to append `-psn_0_...`; neither is an argument of this program.
bool IsInjectedArgument(const std::string &arg) {
  if (arg == "-NSDocumentRevisionsDebugMode" || arg == "YES") {
    return true;
  }
  return arg.size() >= 5 && arg.compare(0, 5, "-psn_") == 0;
}

Options ParseOptions(Si32 argc, const char * const *argv) {
  Options options;
  for (Si32 i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (IsInjectedArgument(arg)) {
      continue;
    }
    if (arg == "--client") {
      options.is_client = true;
      continue;
    }
    if (arg == "--port" && i + 1 < argc) {
      options.port = static_cast<uint16_t>(std::atoi(argv[i + 1]));
      ++i;
      continue;
    }
    if (arg == "--ticks" && i + 1 < argc) {
      options.tick_budget = static_cast<Si32>(std::atoi(argv[i + 1]));
      ++i;
      continue;
    }
    std::printf("Unknown argument \"%s\".\n"
        "Usage: headless_server [--client] [--port N] [--ticks N]\n",
        arg.c_str());
  }
  return options;
}

// The decision is needed before EasyMain runs, which is why the registration
// below happens before main. This program never draws, in either mode, so the
// answer is the same every time; a game with console subcommands would look at
// argv here and return kWindowed for an ordinary run.
StartupMode DecideStartupMode() {
  return StartupMode::kNoWindow;
}

// Writes the whole message, however many pieces the socket takes it in.
bool WriteAll(ConnectionSocket *socket, const std::string &text) {
  size_t total = 0;
  const double deadline = Time() + 1.0;
  while (total < text.size() && Time() < deadline) {
    size_t written = 0;
    const SocketResult result = socket->Write(text.data() + total,
        text.size() - total, &written);
    if (result != SocketResult::kSocketOk) {
      return false;
    }
    total += written;
  }
  return total == text.size();
}

// A newline ends a command, so what arrives has to be collected until one shows
// up: a socket knows nothing about message boundaries and a single read may
// bring half a command or three of them.
bool TakeLine(std::string *in_out_inbox, std::string *out_line) {
  const size_t end = in_out_inbox->find('\n');
  if (end == std::string::npos) {
    return false;
  }
  *out_line = in_out_inbox->substr(0, end);
  in_out_inbox->erase(0, end + 1);
  if (!out_line->empty() && out_line->back() == '\r') {
    out_line->pop_back();
  }
  return true;
}

struct Client {
  ConnectionSocket socket;
  std::string inbox;
};

Si32 RunServer(const Options &options) {
  ListenerSocket listener(AddressFamily::kIpV4, SocketProtocol::kTcp);
  if (!listener.IsValid()) {
    std::printf("No socket: %s\n", listener.GetLastError().c_str());
    return 1;
  }
  // Without this the port stays unusable for a couple of minutes after a
  // restart, which is the first thing anyone hits while writing a server.
  if (listener.SetSoReuseAddress(true) != SocketResult::kSocketOk) {
    std::printf("Warning: %s\n", listener.GetLastError().c_str());
  }
  if (listener.Bind("0.0.0.0", options.port) != SocketResult::kSocketOk) {
    std::printf("Port %d could not be bound: %s\n",
        static_cast<int>(options.port), listener.GetLastError().c_str());
    return 1;
  }
  // The tick has to keep running whether or not anyone is knocking, so nothing
  // in the loop is allowed to block: neither the accept nor the reads.
  if (listener.SetSoNonblocking(true) != SocketResult::kSocketOk) {
    std::printf("Non-blocking mode failed: %s\n",
        listener.GetLastError().c_str());
    return 1;
  }
  std::printf("Listening on port %d, budget %d ticks.\n",
      static_cast<int>(options.port), static_cast<int>(options.tick_budget));
  Log("headless_server: listening");

  std::vector<Client> clients;
  Si32 tick = 0;
  bool is_running = true;
  // The pace is kept here and nowhere else. In kNoWindow there is no buffer
  // swap inside ShowFrame, so nothing waits for a display and a loop without a
  // Sleep of its own would spin on a full core for no reason at all.
  double next_tick = Time();
  while (is_running && tick < options.tick_budget) {
    ++tick;

    ConnectionSocket accepted = listener.Accept();
    while (accepted.IsValid()) {
      if (accepted.SetSoNonblocking(true) != SocketResult::kSocketOk) {
        std::printf("A connection could not be made non-blocking: %s\n",
            accepted.GetLastError().c_str());
      }
      std::printf("Tick %d: a client connected.\n", static_cast<int>(tick));
      Client client;
      client.socket = std::move(accepted);
      clients.push_back(std::move(client));
      accepted = listener.Accept();
    }

    for (size_t i = 0; i < clients.size();) {
      Client &client = clients[i];
      bool is_gone = false;
      char buffer[512];
      size_t read_size = 0;
      const SocketResult result = client.socket.Read(buffer, sizeof(buffer),
          &read_size);
      if (result != SocketResult::kSocketOk) {
        std::printf("Tick %d: a client left (%s).\n", static_cast<int>(tick),
            client.socket.GetLastError().c_str());
        is_gone = true;
      } else {
        client.inbox.append(buffer, read_size);
      }

      std::string line;
      while (!is_gone && TakeLine(&client.inbox, &line)) {
        std::string answer;
        if (line == "ping") {
          answer = "pong\n";
        } else if (line == "ticks") {
          char text[64];
          std::snprintf(text, sizeof(text), "ticks %d\n",
              static_cast<int>(tick));
          answer = text;
        } else if (line == "stop") {
          answer = "stopping\n";
          is_running = false;
        } else {
          answer = "unknown command\n";
        }
        if (!WriteAll(&client.socket, answer)) {
          std::printf("Tick %d: an answer could not be sent (%s).\n",
              static_cast<int>(tick), client.socket.GetLastError().c_str());
          is_gone = true;
        }
      }

      if (is_gone) {
        clients.erase(clients.begin() + static_cast<Si64>(i));
      } else {
        ++i;
      }
    }

    next_tick += kTickDuration;
    const double idle = next_tick - Time();
    if (idle > 0.0) {
      Sleep(idle);
    } else {
      next_tick = Time();  // fell behind, and catching the time up is worse
    }
  }

  std::printf("%s after %d ticks.\n",
      is_running ? "Out of tick budget" : "Stopped on request",
      static_cast<int>(tick));
  Log("headless_server: done");
  return 0;
}

// Reads one line, giving up after a while rather than waiting for good: a
// client that hangs on a silent server is no better than a server that hangs.
bool ReadLineWithin(ConnectionSocket *socket, std::string *in_out_inbox,
    double seconds, std::string *out_line) {
  const double deadline = Time() + seconds;
  while (Time() < deadline) {
    if (TakeLine(in_out_inbox, out_line)) {
      return true;
    }
    char buffer[512];
    size_t read_size = 0;
    if (socket->Read(buffer, sizeof(buffer), &read_size)
        != SocketResult::kSocketOk) {
      return false;
    }
    if (read_size == 0) {
      Sleep(0.002);
      continue;
    }
    in_out_inbox->append(buffer, read_size);
  }
  return false;
}

Si32 RunClient(const Options &options) {
  ConnectionSocket socket(AddressFamily::kIpV4, SocketProtocol::kTcp);
  if (!socket.IsValid()) {
    std::printf("No socket: %s\n", socket.GetLastError().c_str());
    return 1;
  }
  if (socket.Connect("127.0.0.1", options.port) != SocketConnectResult::kSocketOk) {
    std::printf("Port %d refused the connection: %s\n",
        static_cast<int>(options.port), socket.GetLastError().c_str());
    return 1;
  }
  if (socket.SetSoNonblocking(true) != SocketResult::kSocketOk) {
    std::printf("Non-blocking mode failed: %s\n",
        socket.GetLastError().c_str());
    return 1;
  }

  const std::vector<std::string> commands = {"ping", "ticks", "stop"};
  std::string inbox;
  for (size_t i = 0; i < commands.size(); ++i) {
    if (!WriteAll(&socket, commands[i] + "\n")) {
      std::printf("Sending \"%s\" failed: %s\n", commands[i].c_str(),
          socket.GetLastError().c_str());
      return 1;
    }
    std::string answer;
    if (!ReadLineWithin(&socket, &inbox, 2.0, &answer)) {
      std::printf("No answer to \"%s\": %s\n", commands[i].c_str(),
          socket.GetLastError().c_str());
      return 1;
    }
    std::printf("%s -> %s\n", commands[i].c_str(), answer.c_str());
    if (commands[i] == "ping" && answer != "pong") {
      std::printf("The server answered \"%s\" instead of \"pong\".\n",
          answer.c_str());
      return 1;
    }
  }
  return 0;
}

}  // namespace

ARCTIC_STARTUP_MODE_DECIDER(DecideStartupMode)

void EasyMain() {
  Engine *engine = GetEngine();
  const Options options = ParseOptions(engine->GetArgc(), engine->GetArgv());
  // Nothing here draws, nothing waits for a key, and there is no window to
  // close: the mode is decided above, and ExitProgram is the way out, with the
  // code the shell will see.
  ExitProgram(options.is_client ? RunClient(options) : RunServer(options));
}

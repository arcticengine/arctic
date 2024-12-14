/// @brief A simulator that simulates client-server communication with packet transmission
/// This simulation shows how network packets are transmitted between clients and a server
/// through a router, including packet loss, latency, and bandwidth limitations.

/// This simulation visualizes how multiplayer games work over the internet, showing:
/// - How clients connect to a server through a router
/// - How game data (level, player positions, etc.) is sent in packets
/// - Network issues like packet loss and latency
/// - Server and router buffer management
/// - Network congestion and its effects
/// 
/// Key concepts demonstrated:
/// - White dots: Regular data packets
/// - Yellow dots: Acknowledgment packets (confirming data receipt) and retransmitted packets
/// - Red dots: Invalid packets (packets of a timed out session)
/// - Blue dots: World state updates (player positions, etc.)
/// - Cyan dots: Player control inputs
///
/// You can experiment by:
/// - Adding/removing clients to see how network load changes
/// - Adjusting simulation speed to observe details
/// - Rebooting server/router to see reconnection behavior
/// - Enabling server buffer limits to simulate resource constraints

#include "engine/easy.h"
#include <map>

using namespace arctic;  //NOLINT

/// Maximum number of packets for level data transmission
const Si32 kLevelSizePackets = 1000000/576;

/// Simulation time step
double g_dt = 0.0001;
/// Current simulation time
double g_t = 0.0;
/// Time multiplier for simulation speed control
double g_t_mult = 1.0/128.0;
/// Next available IP address for new clients
Si32 g_next_ip_address = 2;
/// Next available session ID
Ui64 g_next_session_id = 1;

// GUI elements
std::shared_ptr<GuiTheme> g_theme;
std::shared_ptr<Panel> g_gui;
std::shared_ptr<Checkbox> g_checkbox_limit_server_buffer;

/// @brief Creates a glowing ball sprite with specified color and size
/// @param rm Red component multiplier
/// @param gm Green component multiplier
/// @param bm Blue component multiplier
/// @param min Minimum brightness
/// @param size Size of the sprite in pixels
Sprite CreateLightball(Ui8 rm, Ui8 gm, Ui8 bm, float min, Si32 size) {
  Sprite sprite;
  sprite.Create(size, size);
  Ui32 *pd = (Ui32*)sprite.RawData();
  Si32 stride = sprite.StridePixels();
  float max_bright=min / 256.0f * ((size / 2) * (size / 2) + 1.0f) ;
  for(Si32 y=0; y<size; y++) {
    for(Si32 x=0; x<size; x++) {
      float dx = (float)(x - size / 2);
      float dy = (float)(y - size / 2);
      float bright = max_bright / (dx * dx + dy * dy + 1.0f);
      Ui32 r = (Ui32)std::min(bright * (float)rm, 255.f);
      Ui32 g = (Ui32)std::min(bright * (float)gm, 255.f);
      Ui32 b = (Ui32)std::min(bright * (float)bm, 255.f);
      pd[x + y * stride] = (r) | (g << 8) | (b << 16) | (255ull << 24);
    }
  }
  sprite.SetPivot(sprite.Size() / 2);
  return sprite;
}

/// @brief Gets the next available session ID
/// @return Unique session identifier
Ui64 GetNextSessionId() {
  return g_next_session_id++;
}

/// @brief Represents a network packet in the simulation
struct Packet {
  /// Different types of packets in the simulation
  enum class Kind {
    kConnectionRequest,  ///< Initial connection request from client
    kData,              ///< Level data from server to client
    kDataAck,           ///< Acknowledgment of received data
    kDataRetransmitt,   ///< Retransmitted data after packet loss
    kPlayerControls,    ///< Player input sent to server
    kWorldState,        ///< Game state update from server
  };

  // all types
  Ui64 session_id; ///< Session identifier
  Si32 source_ip;  ///< Source IP address
  Si32 destination_ip; ///< Destination IP address
  Si32 size_bytes; ///< Size of the packet in bytes
  Kind kind; ///< Type of the packet

  // kData, kDataRetransmitt, kDataAck
  double server_t0; // also, filled in kWorldState
  Si32 part_idx; ///< Index of the packet part

  // simulation
  bool is_lost; ///< Whether the packet is lost
  double t_in; ///< Time when the packet was received
  double t_out; ///< Time when the packet was sent
};

/// @brief Represents a network node (client, server, or router)
struct Node {
  Vec2Si32 screen_pos = Vec2Si32(0, 0);  ///< Position for visualization
  Si32 ip_address;                        ///< Node's IP address
  std::deque<Packet> in_buffer;           ///< Incoming packet buffer
  bool is_active;                         ///< Whether node is active
  Si64 received_bytes = 0;                ///< Total bytes received
};

/// @brief Represents a network channel between two nodes
struct Channel {
  Node* in_node;                          ///< Source node
  Node* out_node;                         ///< Destination node
  double speed_bit_per_second;            ///< Channel bandwidth
  double delay;                           ///< Base latency
  double t_busy_up_to;                    ///< Channel busy until this time
  double loss_probability;                ///< Probability of packet loss
  bool is_active;                         ///< Whether channel is active
  std::multimap<double, Packet> packets;  ///< In-flight packets

  /// @brief Updates packet delivery in the channel
  void Update() {
    while (!packets.empty()) {
      auto it = packets.begin();
      Packet &packet = it->second;
      if (packet.t_out <= g_t) {
        if (!packet.is_lost) {
          if (out_node && out_node->is_active) {
            packet.t_in = g_t;
            out_node->in_buffer.push_back(packet);
            out_node->received_bytes += packet.size_bytes;
            packets.erase(it);
          } else {
            packets.erase(it);
          }
        } else {
          packets.erase(it);
        }
      } else {
        break;
      }
    }
  }

  /// @brief Attempts to send a packet through the channel
  /// @param packet Packet to send
  /// @return true if packet was accepted, false if channel is busy
  bool TrySendPacket(Packet& packet) {
    if (t_busy_up_to > g_t + g_dt) {
      return false;
    }
    double busy_duration = packet.size_bytes * 8.0 / speed_bit_per_second;
    t_busy_up_to = std::max(g_t, t_busy_up_to) + busy_duration;
    double transfer_duration = delay * (1.0 + 0.01 * Random(0,100)) + busy_duration;
    packet.t_in = g_t;
    packet.t_out = g_t + transfer_duration;
    if (loss_probability * 10000.0 > Random(0, 10000)) {
      packet.is_lost = true;
    }
    packets.emplace(packet.t_out, packet);
    return true;
  }
};

/// @brief Manages buffered output for a node
struct BufferedOutput {
  Si64 used_memory = 0;                   ///< Current buffer memory usage
  Si64 sent_bytes = 0;                    ///< Total bytes sent
  Si64 drpooed_bytes = 0;                 ///< Total bytes dropped
  Channel* out_channel = nullptr;         ///< Output channel
  std::deque<Packet> out_buffer;          ///< Output packet buffer

  void ApplyLimit(Si32 limit) {
    while (used_memory > limit) {
      Packet &packet = out_buffer.back();
      used_memory -= packet.size_bytes;
      drpooed_bytes += packet.size_bytes;
      out_buffer.pop_back();
    }
  }

  void Update() {
    while (!out_buffer.empty()) {
      Packet &packet = out_buffer.front();
      if (!out_channel) {
        return;
      }
      if (out_channel->TrySendPacket(packet)) {
        used_memory -= packet.size_bytes;
        sent_bytes += packet.size_bytes;
        out_buffer.pop_front();
      } else {
        return;
      }
    }
  }

  void Push(Packet &packet) {
    used_memory += packet.size_bytes;
    out_buffer.push_back(packet);
  }

  void Clear() {
    used_memory = 0;
    sent_bytes = 0;
    out_buffer.clear();
  }
};

/// @brief Tracks state of a client session on the server
struct SessionState {
  Si32 ip_address;                        ///< Client's IP address
  Si64 session_id;                        ///< Session identifier
  double t_last_received;                 ///< Time of last received packet
  double ping_window[5] = {0.5, 0.5, 0.5, 0.5, 0.5};
  Si32 next_ping_idx = 0;                 ///< Next ping measurement index
  std::map<Si32, double> unconfirmed_send_t_by_part_idx;

  double GetAvgPing() {
    double s = ping_window[0];
    for (Si32 i = 1; i < 5; ++i) {
      s = std::max(ping_window[i], s);
    }
    return s;
  }
};

/// @brief Server node implementation
struct Server : Node {
  BufferedOutput output;                  ///< Server's output buffer
  std::map<Si32, SessionState> session_state_by_ip_address;  ///< Active sessions
  double next_world_state_send_t = 0.0;   ///< Next world state broadcast time
  Si32 buffer_limit = -1;                 ///< Server buffer size limit

  /// @brief Handles server logic including:
  /// - Processing incoming packets
  /// - Managing client sessions
  /// - Sending level data
  /// - Broadcasting world state
  /// - Handling packet retransmission
  void Reboot() {
    received_bytes = 0;
    output.Clear();
    session_state_by_ip_address.clear();
    in_buffer.clear();
  }

  void Update() {
    // process incoming packets
    while (!in_buffer.empty()) {
      Packet &packet = in_buffer.front();
      auto it = session_state_by_ip_address.find(packet.source_ip);
      if (it == session_state_by_ip_address.end()) {
        if (packet.kind == Packet::Kind::kConnectionRequest) {
          // establish a new session
          SessionState &state = session_state_by_ip_address[packet.source_ip];
          state.ip_address = packet.source_ip;
          state.session_id = packet.session_id;
          state.t_last_received = g_t;
          state.next_ping_idx = 0;
          for (Si32 i = 0; i < 5; ++i) {
            state.ping_window[i] = 0.5;
          }

          // send the level data
          for (Si32 i = 0; i < kLevelSizePackets; ++i) {
            Packet packet;
            packet.kind = Packet::Kind::kData;
            packet.destination_ip = state.ip_address;
            packet.is_lost = false;
            packet.part_idx = i;
            packet.server_t0 = g_t;
            packet.session_id = state.session_id;
            packet.size_bytes = 576;
            packet.source_ip = ip_address;
            output.Push(packet);

            state.unconfirmed_send_t_by_part_idx[i] = g_t;
          }
        } else {
          // just ignore the packet
        }
      } else {
        if (packet.session_id != it->second.session_id) {
          // there is a different session present, ignore
        } else {
          // session found
          SessionState &state = it->second;
          state.t_last_received = g_t;
          if (packet.kind == Packet::Kind::kDataAck) {
            state.ping_window[state.next_ping_idx] = g_t - packet.server_t0;
            state.next_ping_idx = (state.next_ping_idx + 1) % 5;

            state.unconfirmed_send_t_by_part_idx.erase(packet.part_idx);
          }
        }
      }
      in_buffer.pop_front();
    }
    // update all sessions
    for (auto it = session_state_by_ip_address.begin(); it != session_state_by_ip_address.end(); ) {
      SessionState &state = it->second;
      if (state.t_last_received + 3.0 < g_t) {
        // timeout reached, drop session
        auto it2 = it;
        ++it;
        session_state_by_ip_address.erase(it2);
      } else {
        if (!state.unconfirmed_send_t_by_part_idx.empty()) {
          // retransfer after timeout
          double deadline_t = g_t - state.GetAvgPing();
          for (auto unconfirmed_it = state.unconfirmed_send_t_by_part_idx.begin();
               unconfirmed_it != state.unconfirmed_send_t_by_part_idx.end(); ++unconfirmed_it) {
            if (unconfirmed_it->second <= deadline_t) {
              unconfirmed_it->second = g_t;
              Packet packet;
              packet.kind = Packet::Kind::kDataRetransmitt;
              packet.destination_ip = state.ip_address;
              packet.is_lost = false;
              packet.part_idx = unconfirmed_it->first;
              packet.server_t0 = g_t;
              packet.session_id = state.session_id;
              packet.size_bytes = 576;
              packet.source_ip = ip_address;
              output.Push(packet);
            }
          }
        } else {
          // send the world state periodically
          if (next_world_state_send_t <= g_t) {
            Packet packet;
            packet.kind = Packet::Kind::kWorldState;
            packet.destination_ip = state.ip_address;
            packet.is_lost = false;
            packet.session_id = state.session_id;
            packet.server_t0 = g_t;
            packet.size_bytes = 576;
            packet.source_ip = ip_address;
            output.Push(packet);
          }
        }
        ++it;
      }
    }
    if (next_world_state_send_t <= g_t) {
      next_world_state_send_t = g_t + 0.05;
    }

    if (buffer_limit >= 0) {
      output.ApplyLimit(buffer_limit);
    }
    output.Update();
  }
};

/// @brief Client node implementation
struct Client : Node {
  BufferedOutput output;                  ///< Client's output buffer
  bool is_world_state_obtained = false;   ///< Has received world state
  double next_controls_send_t = 0.0;      ///< Next control update time
  double last_receive_t = -3.0;           ///< Time of last received packet
  Ui64 session_id;                        ///< Current session ID
  double last_delay = 0.0;                ///< Last measured delay
  bool level_obtained[kLevelSizePackets] = {0}; ///< Tracking received level chunks

  /// @brief Handles client logic including:
  /// - Processing incoming packets
  /// - Managing connection state
  /// - Sending control updates
  /// - Acknowledging received data
  void Update() {
    if (!is_active) {
      return;
    }
    // process incoming packets
    while (!in_buffer.empty()) {
      Packet &packet = in_buffer.front();
      if (packet.session_id == session_id) {
        last_receive_t = g_t;
        last_delay = g_t - packet.server_t0;
        if (packet.kind == Packet::Kind::kData || packet.kind == Packet::Kind::kDataRetransmitt) {
          level_obtained[packet.part_idx] = true;
          // send ack
          Packet ack_packet;
          ack_packet.kind = Packet::Kind::kDataAck;
          ack_packet.destination_ip = 1;
          ack_packet.is_lost = false;
          ack_packet.session_id = session_id;
          ack_packet.size_bytes = 100;
          ack_packet.source_ip = ip_address;
          ack_packet.part_idx = packet.part_idx;
          ack_packet.server_t0 = packet.server_t0;
          output.Push(ack_packet);
        } else if (packet.kind == Packet::Kind::kWorldState) {
          is_world_state_obtained = true;
        }
      }
      in_buffer.pop_front();
    }

    // disconnect on timeout, reconnect
    if (last_receive_t + 3.0 <= g_t) {
      is_world_state_obtained = false;
      for (Si32 i = 0; i < kLevelSizePackets; ++i) {
       level_obtained[i] = false;
      }
      session_id = GetNextSessionId();
      last_receive_t = g_t;

      Packet packet;
      packet.kind = Packet::Kind::kConnectionRequest;
      packet.destination_ip = 1;
      packet.is_lost = false;
      packet.session_id = session_id;
      packet.size_bytes = 100;
      packet.source_ip = ip_address;
      output.Push(packet);
    }

    // send controls periodically
    if (is_world_state_obtained) {
      if (next_controls_send_t <= g_t) {
        next_controls_send_t = g_t + 0.05;

        Packet packet;
        packet.kind = Packet::Kind::kPlayerControls;
        packet.destination_ip = 1;
        packet.is_lost = false;
        packet.session_id = session_id;
        packet.size_bytes = 200;
        packet.source_ip = ip_address;
        output.Push(packet);
      }
    }

    output.Update();
  }
};

/// @brief Router node implementation
struct Router : Node {
  Si32 total_memory = 1000000;            ///< Total buffer memory
  Si32 used_memory = 0;                   ///< Current memory usage
  Si64 dropped_bytes = 0;                 ///< Total bytes dropped
  std::map<Si32, BufferedOutput> output_by_ip_address;  ///< Output buffers by destination

  /// @brief Handles router logic including:
  /// - Buffer management
  /// - Packet forwarding
  /// - Congestion handling
  void RegisterChannel(Channel* channel) {
    if (channel && channel->in_node == this && channel->out_node) {
      output_by_ip_address[channel->out_node->ip_address].out_channel = channel;
    }
  }

  /// @brief Drops a packet from the router's buffer
  /// @param packet The packet to drop
  void Drop(Packet &packet) {
    dropped_bytes += packet.size_bytes;
  }

  /// @brief Updates the router's state
  void Update() {
    Si32 in_buffer_size = 0;
    Si32 in_buffer_items = (Si32)in_buffer.size();
    for (auto it = in_buffer.begin(); it != in_buffer.end(); ++it) {
      in_buffer_size += it->size_bytes;
    }
    if (in_buffer_size > 0) {
      Si32 bytes_to_discard = in_buffer_size;
      Si32 available_bytes = total_memory - used_memory;
      if (available_bytes > 0) {
        if (in_buffer_size > available_bytes) {
          bytes_to_discard = in_buffer_size - available_bytes;
        } else {
          bytes_to_discard = 0;
        }
      }
      Si64 items_to_discard = (Si64)in_buffer_items * (Si64)bytes_to_discard / (Si64)in_buffer_size;
      double discard_probability = (double)items_to_discard / (double)in_buffer_items;

      while (!in_buffer.empty()) {
        Packet &packet = in_buffer.front();
        if (used_memory >= total_memory) {
          Drop(packet);
        } else if (discard_probability * 1000000.0 > Random(0, 1000000)) {
          Drop(packet);
        } else {
          auto it = output_by_ip_address.find(packet.destination_ip);
          if (it != output_by_ip_address.end()) {
            output_by_ip_address[packet.destination_ip].Push(packet);
          } else {
            Drop(packet);
          }
        }
        in_buffer.pop_front();
      }
    }

    used_memory = 0;
    for (auto it = output_by_ip_address.begin(); it != output_by_ip_address.end(); ++it) {
      it->second.Update();
      used_memory += it->second.used_memory;
    }
  }

  /// @brief Resets router state
  void Reboot() {
    for (auto it = output_by_ip_address.begin(); it != output_by_ip_address.end(); ++it) {
      dropped_bytes += it->second.used_memory;
      used_memory -= it->second.used_memory;
      it->second.Clear();
    }
  }
};

// Global simulation state
const Si32 kMaxClients = 100000;
std::vector<Channel> g_channels;
std::vector<Client> g_clients;
Server g_server;
Router g_router;

// Visual elements
Font g_font;
Sprite white_ball;   ///< Connection request packets
Sprite red_ball;     ///< Lost/invalid packets
Sprite yellow_ball;  ///< Acknowledgment packets
Sprite cyan_ball;    ///< Control packets
Sprite blue_ball;    ///< World state packets

/// @brief Creates bidirectional channels between two nodes
/// @param a First node
/// @param b Second node
/// @param delay Base latency
/// @param loss_probability Packet loss probability
/// @param speed_bit_per_second Channel bandwidth
/// @return Index of the channel created
Si32 AddSymmetricChannels(Node *a, Node *b, double delay, double loss_probability, double speed_bit_per_second) {
  Si32 idx = (Si32)g_channels.size();
  g_channels.resize(idx + 2);
  for (Si32 i = 0; i < 2; ++i) {
    g_channels[idx + i].delay = delay;
    g_channels[idx + i].in_node = (i == 0 ? a : b);
    g_channels[idx + i].is_active = true;
    g_channels[idx + i].loss_probability = loss_probability;
    g_channels[idx + i].out_node = (i == 0 ? b : a);
    g_channels[idx + i].speed_bit_per_second = speed_bit_per_second;
    g_channels[idx + i].t_busy_up_to = g_t;
  }
  return idx;
}

/// @brief Positions clients evenly on screen
void PositionClients() {
  Si32 client_count = (Si32)g_clients.size();
  for (Si32 i = 0; i < client_count; ++i) {
    Client &client = g_clients[i];
    client.screen_pos = Vec2Si32(40+1900*(i)/(client_count), 200);
  }
  Si32 center_x = (g_clients[0].screen_pos.x + g_clients[client_count-1].screen_pos.x)/2;
  g_server.screen_pos.x = center_x;
  g_router.screen_pos.x = center_x;
}

/// @brief Adds a new client to the simulation
void AddClient() {
  for (Si32 i = 0; i < g_clients.size(); ++i) {
    if (!g_clients[i].is_active) {
      g_clients[i].is_active = true;
      g_clients[i].session_id = GetNextSessionId();
      g_clients[i].last_receive_t = -3.0;
      g_clients[i].last_delay = 0;
      g_clients[i].is_world_state_obtained = false;
      for (Si32 i = 0; i < kLevelSizePackets; ++i) {
        g_clients[i].level_obtained[i] = false;
      }
      PositionClients();
      return;
    }
  }
  g_clients.emplace_back();
  Client &client = g_clients.back();
  client.ip_address = g_next_ip_address++;
  client.is_active = true;
  client.session_id = GetNextSessionId();

  Si32 c2r_idx = AddSymmetricChannels(&client, &g_router, 0.010 + Random(0, 35)*0.001, 0.01 + Random(0, 1)*0.01, 2000000.0);
  g_router.RegisterChannel(&g_channels[c2r_idx]);
  g_router.RegisterChannel(&g_channels[c2r_idx + 1]);
  client.output.out_channel = &g_channels[c2r_idx];
  PositionClients();
}

/// @brief Removes an active client from the simulation
void RemoveClient() {
  for (Si32 i = 0; i < g_clients.size(); ++i) {
    Client &client = g_clients[i];
    if (client.is_active) {
      client.is_active = false;
      client.in_buffer.clear();
      client.output.Clear();
      client.received_bytes = 0;
      PositionClients();
      return;
    }
  }
}

/// @brief Creates initial simulation state with server, router, and initial clients
void CreateInitialState() {
  g_router.ip_address = 0;
  g_router.is_active = true;
  g_router.screen_pos = Vec2Si32(1920/2, 1080/2);

  g_server.ip_address = 1;
  g_server.is_active = true;
  g_server.screen_pos = Vec2Si32(1920/2, 1080-200);

  Si32 r2s_idx = AddSymmetricChannels(&g_router, &g_server, 0.010, 0.0001, 20000000.0);
  g_router.RegisterChannel(&g_channels[r2s_idx]);
  g_router.RegisterChannel(&g_channels[r2s_idx + 1]);
  g_server.output.out_channel = &g_channels[r2s_idx + 1];

  int g_client_count = 5;
  for (Si32 i = 0; i < g_client_count; ++i) {
    AddClient();
  }
}

/// @brief Updates simulation state for one time step
void UpdateModel() {
  g_t += g_dt;
  Si32 channel_count = (Si32)g_channels.size();
  for (Si32 i = 0; i < channel_count; ++i) {
    g_channels[i].Update();
  }
  Si32 client_count = (Si32)g_clients.size();
  for (Si32 i = 0; i < client_count; ++i) {
    g_clients[i].Update();
  }
  g_server.Update();
  g_router.Update();
}

/// @brief Draws current simulation state including:
/// - Network topology
/// - Node status
/// - In-flight packets
/// - Performance statistics
void DrawModel() {
  char text[128];
  // chennel lines
  Si32 channel_count = (Si32)g_channels.size();
  for (Si32 i = 0; i < channel_count; ++i) {
    Channel &channel = g_channels[i];
    DrawLine(channel.in_node->screen_pos, channel.out_node->screen_pos, Rgba(128, 128, 128));
  }

  // node circles
  Si32 client_count = (Si32)g_clients.size();
  for (Si32 i = 0; i < client_count; ++i) {
    Rgba color = Rgba(128, 128, 128);
    if (!g_clients[i].is_active) {
      color = Rgba(255, 0, 0);
    }
    DrawCircle(g_clients[i].screen_pos, 10, color);

    // node stats
    Si32 level_obtained = 0;
    for (Si32 n = 0; n < kLevelSizePackets; ++n) {
      if (g_clients[i].level_obtained[n]) {
        ++level_obtained;
      }
    }
    snprintf(text, sizeof(text), u8"Client %d\nLevel: %d/%d\nRcv: %.2f MiB\nDelay: %.4f s",
             i,
             level_obtained, kLevelSizePackets,
             g_clients[i].received_bytes * (1.0 / 1024.0 / 1024.0),
             g_clients[i].last_delay);
    g_font.Draw(text, g_clients[i].screen_pos.x - 20, g_clients[i].screen_pos.y - 20, kTextOriginTop);
  }
  DrawCircle(g_server.screen_pos, 10, Rgba(128, 128, 128));
  DrawCircle(g_router.screen_pos, 10, Rgba(128, 128, 128));

  // node stats
  snprintf(text, sizeof(text), u8"Server\nBuffered: %f MiB\nSent: %f MiB\nDropped: %f MiB",
           g_server.output.used_memory * (1.0 / 1024.0 / 1024.0),
           g_server.output.sent_bytes * (1.0 / 1024.0 / 1024.0),
           g_server.output.drpooed_bytes * (1.0 / 1024.0 / 1024.0));
  g_font.Draw(text, g_server.screen_pos.x - 20, g_server.screen_pos.y + 20, kTextOriginBottom);

  snprintf(text, sizeof(text), u8"Router\nBuffered: %f MiB\nDropped: %f MiB",
           g_router.used_memory * (1.0 / 1024.0 / 1024.0),
           g_router.dropped_bytes * (1.0 / 1024.0 / 1024.0));
  g_font.Draw(text, g_router.screen_pos.x + 20, g_router.screen_pos.y + 20, kTextOriginBottom);

  // channel packets
  for (Si32 i = 0; i < channel_count; ++i) {
    Channel &channel = g_channels[i];
    for (auto it = channel.packets.begin(); it != channel.packets.end(); ++it) {
      double part = 0.0;
      Packet &packet = it->second;
      if (packet.t_out - packet.t_in > 0.0) {
        part = Clamp((g_t - packet.t_in)/(packet.t_out - packet.t_in), 0.0, 1.0);
      }
      Vec2F p0 = Vec2F(channel.in_node->screen_pos);
      Vec2F p1 = Vec2F(channel.out_node->screen_pos);
      if (p1.y > p0.y) {
        p0.x += 3;
        p1.x += 3;
      } else {
        p0.x -= 3;
        p1.x -= 3;
      }
      Vec2F pos(Lerp(p0.x, p1.x, part), Lerp(p0.y, p1.y, part));

      bool is_valid = true;
      switch (packet.kind) {
        case Packet::Kind::kData:
        case Packet::Kind::kDataRetransmitt:
        case Packet::Kind::kWorldState:
        {
          auto it = g_router.output_by_ip_address.find(packet.destination_ip);
          if (it == g_router.output_by_ip_address.end() || !it->second.out_channel) {
            is_valid = false;
          } else {
            Client *client = (Client*)it->second.out_channel->out_node;
            if (!client || !client->is_active || client->session_id != packet.session_id) {
              is_valid = false;
            }
          }
        }
          break;
        case Packet::Kind::kDataAck:
        case Packet::Kind::kPlayerControls:
        {
          if (!g_server.is_active) {
            is_valid = false;
          } else {
            auto it = g_server.session_state_by_ip_address.find(packet.source_ip);
            if (it == g_server.session_state_by_ip_address.end() || it->second.session_id != packet.session_id) {
              is_valid = false;
            }
          }
        }
          break;
        default:
          break;
      }

      Sprite *ball = &white_ball;
      switch (packet.kind) {
        case Packet::Kind::kDataAck:
        case Packet::Kind::kDataRetransmitt:
          ball = is_valid ? &yellow_ball : &red_ball;
          break;
        case Packet::Kind::kPlayerControls:
          ball = is_valid ? &cyan_ball : &red_ball;
          break;
        case Packet::Kind::kWorldState:
          ball = is_valid ? &blue_ball : &red_ball;
          break;
        default:
          ball = is_valid ? &white_ball : &red_ball;
          break;
      }

      ball->Draw((Si32)pos.x, (Si32)pos.y, kDrawBlendingModeAdd);
    }
  }
}

/// @brief Increases simulation speed
void SpeedUp() {
  g_t_mult = std::min(32.0, g_t_mult * 2.0);
}

/// @brief Decreases simulation speed
void SlowDown() {
  g_t_mult = std::max(1.0 / 128.0, g_t_mult / 2.0);
}

/// @brief Toggles server buffer limit
void LimitServerBuffer() {
  if (g_checkbox_limit_server_buffer->IsChecked()) {
    g_server.buffer_limit = 1000000;
  } else {
    g_server.buffer_limit = -1;
  }
}

/// @brief Main entry point for the simulation
void EasyMain() {
  // Load GUI theme and font
  g_theme = std::make_shared<GuiTheme>();
  g_theme->Load("data/gui_theme.xml");
  g_font.Load("data/arctic_one_bmf.fnt");

  // Create lightballs for visualizing packets
  white_ball = CreateLightball(255, 255, 255, 6.0, 32);
  red_ball = CreateLightball(255, 8, 8, 6, 32);
  yellow_ball = CreateLightball(255, 255, 84, 6, 32);
  cyan_ball = CreateLightball(30, 191, 255, 6, 32);
  blue_ball = CreateLightball(8, 8, 255, 12, 32);

  // Resize screen to match the desired resolution
  ResizeScreen(1920, 1080);

  // Create GUI elements
  GuiFactory gf;
  gf.theme_ = g_theme;
  g_gui = gf.MakeTransparentPanel();
  std::shared_ptr<Button> button;

  button = gf.MakeButton();
  button->SetText("Slow Down");
  button->SetPos(Vec2Si32(16, 935));
  button->SetWidth(200);
  button->OnButtonClick = SlowDown;
  g_gui->AddChild(button);

  button = gf.MakeButton();
  button->SetText("Speed Up");
  button->SetPos(Vec2Si32(16+208, 935));
  button->SetWidth(200);
  button->OnButtonClick = SpeedUp;
  g_gui->AddChild(button);

  button = gf.MakeButton();
  button->SetText("Reboot Server");
  button->SetPos(Vec2Si32(16, 935-80*1));
  button->SetWidth(200);
  button->OnButtonClick = std::bind(&Server::Reboot, &g_server);
  g_gui->AddChild(button);

  g_checkbox_limit_server_buffer = gf.MakeCheckbox();
  g_checkbox_limit_server_buffer->SetPos(Vec2Si32(16+208, 935-80*1));
  g_checkbox_limit_server_buffer->SetText(" Limit server buffer");
  g_checkbox_limit_server_buffer->OnButtonClick = LimitServerBuffer;
  g_gui->AddChild(g_checkbox_limit_server_buffer);

  button = gf.MakeButton();
  button->SetText("Reboot Router");
  button->SetPos(Vec2Si32(16, 935-80*2));
  button->SetWidth(200);
  button->OnButtonClick = std::bind(&Router::Reboot, &g_router);
  g_gui->AddChild(button);

  button = gf.MakeButton();
  button->SetText("Remove Client");
  button->SetPos(Vec2Si32(16, 935-80*3));
  button->SetWidth(200);
  button->OnButtonClick = RemoveClient;
  g_gui->AddChild(button);

  button = gf.MakeButton();
  button->SetText("Add Client");
  button->SetPos(Vec2Si32(16+208, 935-80*3));
  button->SetWidth(200);
  button->OnButtonClick = AddClient;
  g_gui->AddChild(button);

  // Reserve memory for channels and clients
  g_channels.reserve(kMaxClients*2+2);
  g_clients.reserve(kMaxClients);

  // Create initial simulation state
  CreateInitialState();

  // Main simulation loop
  double rt0 = Time();
  double rt1 = Time();
  double t_target = 0.0;

  while (!IsKeyDownward(kKeyEscape)) {
    // Update simulation time
    rt0 = rt1;
    rt1 = Time();
    double rdt = rt1 - rt0;
    t_target = g_t + rdt * g_t_mult;

    // Clear screen
    Clear();

    // Update simulation state until target time is reached or timeout
    while (g_t < t_target && Time() - rt1 < 0.1) {
      UpdateModel();
    }

    // Apply GUI input
    for (Si32 i = 0; i < InputMessageCount(); ++i) {
      g_gui->ApplyInput(GetInputMessage(i), nullptr);
    }

    // Draw simulation state
    DrawModel();
    g_gui->Draw(Vec2Si32(0, 0));

    // Draw time info
    char text[128];
    snprintf(text, sizeof(text), u8"Model time: %f s\nTarget multiplier: %f", g_t, g_t_mult);
    g_font.Draw(text, 20, ScreenSize().y - 20, kTextOriginTop);

    // Show frame
    ShowFrame();
  }
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Jettison Project Team

#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

struct lws_context;
struct lws;

namespace jettison
{

/**
 * @brief WebSocket client for receiving binary state messages
 *
 * This client connects to a WebSocket endpoint over WSS (TLS),
 * ignoring certificate validation errors for local/development use.
 */
class WebSocketClient
{
public:
  using MessageCallback = std::function<void (const uint8_t *data, size_t len)>;
  using ConnectionCallback = std::function<void (bool connected)>;
  using ErrorCallback = std::function<void (const std::string &error)>;

  /**
   * @brief Construct a WebSocket client
   * @param host Hostname or IP address (e.g., "sych.local")
   * @param port Port number (default 443 for HTTPS/WSS)
   * @param path WebSocket path (e.g., "/ws/ws_state")
   */
  WebSocketClient (const std::string &host, int port, const std::string &path);
  ~WebSocketClient ();

  // Non-copyable, non-movable
  WebSocketClient (const WebSocketClient &) = delete;
  WebSocketClient &operator= (const WebSocketClient &) = delete;
  WebSocketClient (WebSocketClient &&) = delete;
  WebSocketClient &operator= (WebSocketClient &&) = delete;

  /**
   * @brief Set callback for received messages
   * @param callback Function called when binary message is received
   */
  void set_message_callback (MessageCallback callback);

  /**
   * @brief Set callback for connection status changes
   * @param callback Function called when connection state changes
   */
  void set_connection_callback (ConnectionCallback callback);

  /**
   * @brief Set callback for errors
   * @param callback Function called on errors
   */
  void set_error_callback (ErrorCallback callback);

  /**
   * @brief Connect to the WebSocket server
   * @return true if connection initiated successfully
   */
  bool connect ();

  /**
   * @brief Run the event loop (blocking)
   *
   * This will process WebSocket events until disconnect or error.
   * Call this after connect().
   */
  void run ();

  /**
   * @brief Request disconnection
   *
   * Can be called from callbacks to stop the event loop.
   */
  void disconnect ();

  /**
   * @brief Check if currently connected
   * @return true if connected
   */
  bool is_connected () const;

private:
  class Impl;
  std::unique_ptr<Impl> pimpl_;
};

} // namespace jettison

#endif // WEBSOCKET_CLIENT_H

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2024 Jettison State RX Contributors

#include "websocket_client.h"
#include <cstring>
#include <iostream>
#include <libwebsockets.h>
#include <memory>
#include <vector>

namespace jettison
{

class WebSocketClient::Impl
{
public:
  Impl (const std::string &host, int port, const std::string &path)
      : host_ (host), port_ (port), path_ (path), context_ (nullptr),
        wsi_ (nullptr), connected_ (false), should_disconnect_ (false)
  {
  }

  ~Impl ()
  {
    disconnect ();
    if (context_ != nullptr)
      {
        lws_context_destroy (context_);
      }
  }

  void
  set_message_callback (MessageCallback callback)
  {
    message_callback_ = std::move (callback);
  }

  void
  set_connection_callback (ConnectionCallback callback)
  {
    connection_callback_ = std::move (callback);
  }

  void
  set_error_callback (ErrorCallback callback)
  {
    error_callback_ = std::move (callback);
  }

  bool
  connect ()
  {
    // Create context
    struct lws_context_creation_info info;
    std::memset (&info, 0, sizeof (info));

    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols_;
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.user = this;

    // SSL options - accept self-signed certificates
    info.client_ssl_cert_filepath = nullptr;
    info.ssl_cert_filepath = nullptr;
    info.ssl_private_key_filepath = nullptr;

    context_ = lws_create_context (&info);
    if (context_ == nullptr)
      {
        if (error_callback_)
          {
            error_callback_ ("Failed to create libwebsockets context");
          }
        return false;
      }

    // Setup connection info
    struct lws_client_connect_info ccinfo;
    std::memset (&ccinfo, 0, sizeof (ccinfo));

    ccinfo.context = context_;
    ccinfo.address = host_.c_str ();
    ccinfo.port = port_;
    ccinfo.path = path_.c_str ();
    ccinfo.host = host_.c_str ();
    ccinfo.origin = host_.c_str ();
    ccinfo.protocol = "binary"; // Use binary protocol
    ccinfo.ssl_connection = LCCSCF_USE_SSL
                            | LCCSCF_ALLOW_SELFSIGNED
                            | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK
                            | LCCSCF_ALLOW_EXPIRED;
    ccinfo.userdata = this;

    wsi_ = lws_client_connect_via_info (&ccinfo);
    if (wsi_ == nullptr)
      {
        if (error_callback_)
          {
            error_callback_ ("Failed to initiate connection");
          }
        return false;
      }

    return true;
  }

  void
  run ()
  {
    while (!should_disconnect_ && context_ != nullptr)
      {
        int n = lws_service (context_, 50); // 50ms timeout
        if (n < 0)
          {
            break;
          }
      }
  }

  void
  disconnect ()
  {
    should_disconnect_ = true;
    if (wsi_ != nullptr && connected_)
      {
        lws_callback_on_writable (wsi_);
      }
  }

  bool
  is_connected () const
  {
    return connected_;
  }

  // Static callback for libwebsockets
  static int
  callback_function (struct lws *wsi, enum lws_callback_reasons reason,
                     void * /*user*/, void *in, size_t len)
  {
    Impl *impl = static_cast<Impl *> (lws_context_user (lws_get_context (wsi)));
    if (impl == nullptr)
      {
        return 0;
      }

    switch (reason)
      {
      case LWS_CALLBACK_CLIENT_ESTABLISHED:
        impl->connected_ = true;
        impl->wsi_ = wsi;
        if (impl->connection_callback_)
          {
            impl->connection_callback_ (true);
          }
        break;

      case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        {
          const char *error_msg = in != nullptr ? static_cast<const char *> (in)
                                                 : "Unknown error";
          if (impl->error_callback_)
            {
              impl->error_callback_ (std::string ("Connection error: ")
                                     + error_msg);
            }
          impl->connected_ = false;
          impl->should_disconnect_ = true;
        }
        break;

      case LWS_CALLBACK_CLIENT_RECEIVE:
        {
          const uint8_t *data = static_cast<const uint8_t *> (in);
          if (impl->message_callback_ && data != nullptr && len > 0)
            {
              // Check if this is the final fragment
              int is_final = lws_is_final_fragment (wsi);

              if (is_final != 0)
                {
                  // For simplicity, we handle single-frame messages
                  // Multi-frame would require buffering
                  impl->message_callback_ (data, len);
                }
              else
                {
                  // Handle fragmented messages by buffering
                  impl->rx_buffer_.insert (impl->rx_buffer_.end (), data,
                                           data + len);

                  // This check is redundant but safe
                  if (is_final != 0 && !impl->rx_buffer_.empty ())
                    {
                      impl->message_callback_ (impl->rx_buffer_.data (),
                                               impl->rx_buffer_.size ());
                      impl->rx_buffer_.clear ();
                    }
                }
            }
        }
        break;

      case LWS_CALLBACK_CLIENT_CLOSED:
        impl->connected_ = false;
        if (impl->connection_callback_)
          {
            impl->connection_callback_ (false);
          }
        impl->should_disconnect_ = true;
        break;

      case LWS_CALLBACK_WSI_DESTROY:
        impl->wsi_ = nullptr;
        break;

      default:
        // Ignore all other callback reasons
        break;
      }

    return 0;
  }

private:
  std::string host_;
  int port_;
  std::string path_;

  struct lws_context *context_;
  struct lws *wsi_;

  bool connected_;
  bool should_disconnect_;

  MessageCallback message_callback_;
  ConnectionCallback connection_callback_;
  ErrorCallback error_callback_;

  std::vector<uint8_t> rx_buffer_; // Buffer for fragmented messages

  static constexpr struct lws_protocols protocols_[]
      = { { "binary", callback_function, 0, 4096, 0, nullptr, 0 },
          { nullptr, nullptr, 0, 0, 0, nullptr, 0 } };
};

// Static member initialization
constexpr struct lws_protocols WebSocketClient::Impl::protocols_[];

// WebSocketClient implementation (forwarding to Impl)

WebSocketClient::WebSocketClient (const std::string &host, int port,
                                  const std::string &path)
    : pimpl_ (std::make_unique<Impl> (host, port, path))
{
}

WebSocketClient::~WebSocketClient () = default;

void
WebSocketClient::set_message_callback (MessageCallback callback)
{
  pimpl_->set_message_callback (std::move (callback));
}

void
WebSocketClient::set_connection_callback (ConnectionCallback callback)
{
  pimpl_->set_connection_callback (std::move (callback));
}

void
WebSocketClient::set_error_callback (ErrorCallback callback)
{
  pimpl_->set_error_callback (std::move (callback));
}

bool
WebSocketClient::connect ()
{
  return pimpl_->connect ();
}

void
WebSocketClient::run ()
{
  pimpl_->run ();
}

void
WebSocketClient::disconnect ()
{
  pimpl_->disconnect ();
}

bool
WebSocketClient::is_connected () const
{
  return pimpl_->is_connected ();
}

} // namespace jettison

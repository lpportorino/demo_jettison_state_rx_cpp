// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2024 Jettison State RX Contributors

#include "dump_manager.h"
#include "json_converter.h"
#include "proto_validator.h"
#include "websocket_client.h"
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <atomic>

using namespace jettison;

static std::atomic<bool> g_running{ true };
static WebSocketClient *g_client = nullptr;

static void
signal_handler (int /*signal*/)
{
  g_running = false;
  if (g_client != nullptr)
    {
      g_client->disconnect ();
    }
}

static void
print_help (const char *program_name)
{
  std::cout << "Jettison State Receiver - Read and validate Jettison state "
               "messages\n\n";
  std::cout << "Usage:\n";
  std::cout << "  " << program_name << " <host>              "
            << "Connect and stream state from host\n";
  std::cout << "  " << program_name << " <host> --dump N    "
            << "Dump N payloads to dumps/ directory\n";
  std::cout << "  " << program_name
            << " --read-dump <file>  Read, validate and print dump file\n\n";
  std::cout << "Arguments:\n";
  std::cout << "  <host>         Hostname or IP address (e.g., sych.local)\n";
  std::cout << "  --dump N       Dump N payloads and exit\n";
  std::cout << "  --read-dump    Read and validate a dump file\n\n";
  std::cout << "Examples:\n";
  std::cout << "  " << program_name << " sych.local\n";
  std::cout << "  " << program_name << " sych.local --dump 10\n";
  std::cout << "  " << program_name << " --read-dump dumps/state_0001.bin\n\n";
  std::cout << "Notes:\n";
  std::cout << "  - SSL certificate errors are ignored for local connections\n";
  std::cout << "  - Dumps may contain sensitive data - handle with care\n";
  std::cout << "  - Press Ctrl+C to stop streaming\n";
}

static int
stream_mode (const std::string &host, int dump_count)
{
  const int port = 443;
  const std::string path = "/ws/ws_state";

  std::cout << "Connecting to wss://" << host << ":" << port << path << "\n";

  WebSocketClient client (host, port, path);
  g_client = &client;

  ProtoValidator validator;
  JsonConverter json_converter;
  DumpManager dump_manager;

  int message_count = 0;
  int saved_count = 0;

  // Setup callbacks
  client.set_connection_callback ([&] (bool connected) {
    if (connected)
      {
        std::cout << "Connected successfully\n";
      }
    else
      {
        std::cout << "Disconnected\n";
        g_running = false;
      }
  });

  client.set_error_callback (
      [&] (const std::string &error) { std::cerr << "Error: " << error << "\n"; });

  client.set_message_callback ([&] (const uint8_t *data, size_t len) {
    message_count++;
    std::cout << "\n=== Message #" << message_count << " (size: " << len
              << " bytes) ===\n";

    // Save dump if requested
    if (dump_count > 0 && saved_count < dump_count)
      {
        dump_manager.ensure_dump_dir_exists ();
        if (dump_manager.save_dump (data, len, saved_count + 1))
          {
            saved_count++;
            std::cout << "Saved dump " << saved_count << "/" << dump_count
                      << "\n";

            if (saved_count >= dump_count)
              {
                std::cout << "Dump complete. Exiting.\n";
                g_running = false;
                client.disconnect ();
                return;
              }
          }
        else
          {
            std::cerr << "Failed to save dump\n";
          }
      }

    // Parse and validate
    auto state_opt = validator.parse_and_validate (data, len);
    const auto &result = validator.get_last_result ();

    if (!state_opt)
      {
        std::cerr << "INVALID MESSAGE\n";
        std::cerr << "Parse errors:\n";
        for (const auto &error : result.errors)
          {
            std::cerr << "  - " << error << "\n";
          }
        return;
      }

    // Print validation status
    if (result.is_valid)
      {
        std::cout << "Validation: PASSED\n";
      }
    else
      {
        std::cout << "Validation: FAILED\n";
        for (const auto &error : result.errors)
          {
            std::cout << "  Error: " << error << "\n";
          }
      }

    if (!result.warnings.empty ())
      {
        std::cout << "Warnings:\n";
        for (const auto &warning : result.warnings)
          {
            std::cout << "  - " << warning << "\n";
          }
      }

    // Convert to JSON
    if (dump_count == 0) // Only print JSON in non-dump mode
      {
        std::string json = json_converter.to_json (*state_opt, true);
        std::cout << "\nJSON Output:\n" << json << "\n";
      }
  });

  // Setup signal handlers
  std::signal (SIGINT, signal_handler);
  std::signal (SIGTERM, signal_handler);

  // Connect and run
  if (!client.connect ())
    {
      std::cerr << "Failed to initiate connection\n";
      g_client = nullptr;
      return EXIT_FAILURE;
    }

  client.run ();

  g_client = nullptr;
  std::cout << "Total messages received: " << message_count << "\n";

  return EXIT_SUCCESS;
}

static int
read_dump_mode (const std::string &filename)
{
  std::cout << "Reading dump file: " << filename << "\n";

  DumpManager dump_manager;
  auto data = dump_manager.read_dump (filename);

  if (data.empty ())
    {
      std::cerr << "Failed to read dump file or file is empty\n";
      return EXIT_FAILURE;
    }

  std::cout << "Read " << data.size () << " bytes\n";

  ProtoValidator validator;
  JsonConverter json_converter;

  auto state_opt = validator.parse_and_validate (data.data (), data.size ());
  const auto &result = validator.get_last_result ();

  if (!state_opt)
    {
      std::cerr << "INVALID MESSAGE\n";
      std::cerr << "Parse errors:\n";
      for (const auto &error : result.errors)
        {
          std::cerr << "  - " << error << "\n";
        }
      return EXIT_FAILURE;
    }

  // Print validation status
  if (result.is_valid)
    {
      std::cout << "Validation: PASSED\n";
    }
  else
    {
      std::cout << "Validation: FAILED\n";
      for (const auto &error : result.errors)
        {
          std::cout << "  Error: " << error << "\n";
        }
    }

  if (!result.warnings.empty ())
    {
      std::cout << "Warnings:\n";
      for (const auto &warning : result.warnings)
        {
          std::cout << "  - " << warning << "\n";
        }
    }

  // Convert to JSON
  std::string json = json_converter.to_json (*state_opt, true);
  std::cout << "\nJSON Output:\n" << json << "\n";

  return EXIT_SUCCESS;
}

int
main (int argc, char *argv[])
{
  if (argc < 2)
    {
      print_help (argv[0]);
      return EXIT_SUCCESS;
    }

  std::string arg1 = argv[1];

  // Read dump mode
  if (arg1 == "--read-dump")
    {
      if (argc < 3)
        {
          std::cerr << "Error: --read-dump requires a filename argument\n\n";
          print_help (argv[0]);
          return EXIT_FAILURE;
        }
      return read_dump_mode (argv[2]);
    }

  // Stream mode (with optional --dump)
  std::string host = arg1;
  int dump_count = 0;

  if (argc >= 4)
    {
      std::string arg2 = argv[2];
      if (arg2 == "--dump")
        {
          try
            {
              dump_count = std::stoi (argv[3]);
              if (dump_count <= 0)
                {
                  std::cerr << "Error: dump count must be positive\n";
                  return EXIT_FAILURE;
                }
            }
          catch (...)
            {
              std::cerr << "Error: invalid dump count\n";
              return EXIT_FAILURE;
            }
        }
      else
        {
          std::cerr << "Error: unknown argument '" << arg2 << "'\n\n";
          print_help (argv[0]);
          return EXIT_FAILURE;
        }
    }

  return stream_mode (host, dump_count);
}

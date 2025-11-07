// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Jettison Project Team

#ifndef DUMP_MANAGER_H
#define DUMP_MANAGER_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace jettison
{

/**
 * @brief Manager for dumping and reading protobuf message payloads
 *
 * Saves raw binary protobuf messages to files for later inspection
 * and validation.
 */
class DumpManager
{
public:
  /**
   * @brief Construct a dump manager
   * @param dump_dir Directory to save dumps (default: "dumps")
   */
  explicit DumpManager (const std::string &dump_dir = "dumps");

  /**
   * @brief Save a binary payload to a numbered dump file
   * @param data Pointer to binary data
   * @param len Length of data in bytes
   * @param sequence_number Sequence number for filename
   * @return true if saved successfully
   */
  bool save_dump (const uint8_t *data, size_t len, int sequence_number);

  /**
   * @brief Read a dump file
   * @param filename Path to dump file
   * @return Binary data if successful, empty vector otherwise
   */
  std::vector<uint8_t> read_dump (const std::string &filename);

  /**
   * @brief Ensure dump directory exists
   * @return true if directory exists or was created successfully
   */
  bool ensure_dump_dir_exists ();

  /**
   * @brief Get the dump directory path
   * @return Dump directory path
   */
  const std::string &get_dump_dir () const { return dump_dir_; }

private:
  std::string dump_dir_;
};

} // namespace jettison

#endif // DUMP_MANAGER_H

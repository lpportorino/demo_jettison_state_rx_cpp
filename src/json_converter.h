// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2024 Jettison State RX Contributors

#ifndef JSON_CONVERTER_H
#define JSON_CONVERTER_H

#include <string>

#include "jon_shared_data.pb.h"

namespace jettison
{

/**
 * @brief Convert protobuf messages to JSON format
 *
 * Uses Google Protocol Buffers' JSON serialization to convert
 * binary protobuf messages to human-readable JSON.
 */
class JsonConverter
{
public:
  JsonConverter () = default;

  /**
   * @brief Convert a JonGUIState message to JSON string
   * @param state The protobuf state message
   * @param pretty If true, format with indentation
   * @return JSON string representation
   */
  std::string to_json (const ser::JonGUIState &state, bool pretty = true);
};

} // namespace jettison

#endif // JSON_CONVERTER_H

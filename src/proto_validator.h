// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2024 Jettison State RX Contributors

#ifndef PROTO_VALIDATOR_H
#define PROTO_VALIDATOR_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "jon_shared_data.pb.h"

namespace jettison
{

/**
 * @brief Validation result for a protobuf message
 */
struct ValidationResult
{
  bool is_valid;
  std::vector<std::string> errors;
  std::vector<std::string> warnings;
};

/**
 * @brief Validator for Jettison state protobuf messages
 *
 * Parses binary protobuf messages and validates them according
 * to buf.validate constraints embedded in the proto definitions.
 */
class ProtoValidator
{
public:
  ProtoValidator () = default;

  /**
   * @brief Parse and validate a binary protobuf message
   * @param data Pointer to binary data
   * @param len Length of data in bytes
   * @return Parsed message if successful, nullopt otherwise
   */
  std::optional<ser::JonGUIState>
  parse_and_validate (const uint8_t *data, size_t len);

  /**
   * @brief Get the last validation result
   * @return Validation result from last parse attempt
   */
  const ValidationResult &get_last_result () const { return last_result_; }

private:
  /**
   * @brief Validate a parsed message
   * @param state The parsed state message
   * @return Validation result
   */
  ValidationResult validate (const ser::JonGUIState &state);

  ValidationResult last_result_;
};

} // namespace jettison

#endif // PROTO_VALIDATOR_H

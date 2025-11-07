// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2024 Jettison State RX Contributors

#include "proto_validator.h"
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

namespace jettison
{

std::optional<ser::JonGUIState>
ProtoValidator::parse_and_validate (const uint8_t *data, size_t len)
{
  last_result_ = ValidationResult{ false, {}, {} };

  ser::JonGUIState state;

  // Parse the protobuf message
  if (!state.ParseFromArray (data, static_cast<int> (len)))
    {
      last_result_.errors.push_back ("Failed to parse protobuf message");
      return std::nullopt;
    }

  // Validate the parsed message
  last_result_ = validate (state);

  if (!last_result_.is_valid)
    {
      return std::nullopt;
    }

  return state;
}

ValidationResult
ProtoValidator::validate (const ser::JonGUIState &state)
{
  ValidationResult result;
  result.is_valid = true;

  // Basic validation checks based on buf.validate constraints

  // Check protocol_version (must be > 0 and <= 2147483647)
  if (state.protocol_version () == 0)
    {
      result.errors.push_back ("protocol_version must be greater than 0");
      result.is_valid = false;
    }

  // Check system_monotonic_time_us (must be >= 0, which is automatic for uint64)
  // No explicit check needed

  // Check required fields
  if (!state.has_system ())
    {
      result.errors.push_back ("Missing required field: system");
      result.is_valid = false;
    }

  if (!state.has_meteo_internal ())
    {
      result.errors.push_back ("Missing required field: meteo_internal");
      result.is_valid = false;
    }

  if (!state.has_lrf ())
    {
      result.errors.push_back ("Missing required field: lrf");
      result.is_valid = false;
    }

  if (!state.has_time ())
    {
      result.errors.push_back ("Missing required field: time");
      result.is_valid = false;
    }

  if (!state.has_gps ())
    {
      result.errors.push_back ("Missing required field: gps");
      result.is_valid = false;
    }

  if (!state.has_compass ())
    {
      result.errors.push_back ("Missing required field: compass");
      result.is_valid = false;
    }

  if (!state.has_rotary ())
    {
      result.errors.push_back ("Missing required field: rotary");
      result.is_valid = false;
    }

  if (!state.has_camera_day ())
    {
      result.errors.push_back ("Missing required field: camera_day");
      result.is_valid = false;
    }

  if (!state.has_camera_heat ())
    {
      result.errors.push_back ("Missing required field: camera_heat");
      result.is_valid = false;
    }

  if (!state.has_compass_calibration ())
    {
      result.errors.push_back ("Missing required field: compass_calibration");
      result.is_valid = false;
    }

  if (!state.has_rec_osd ())
    {
      result.errors.push_back ("Missing required field: rec_osd");
      result.is_valid = false;
    }

  if (!state.has_day_cam_glass_heater ())
    {
      result.errors.push_back (
          "Missing required field: day_cam_glass_heater");
      result.is_valid = false;
    }

  if (!state.has_actual_space_time ())
    {
      result.errors.push_back ("Missing required field: actual_space_time");
      result.is_valid = false;
    }

  // Additional validation for nested fields could be added here
  // For now, we rely on protobuf's built-in validation

  // Note: Full buf.validate constraint validation would require
  // the protovalidate-cc library, which validates CEL expressions.
  // This is a simplified version checking basic required fields.

  return result;
}

} // namespace jettison

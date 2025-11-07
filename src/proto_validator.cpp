// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Jettison Project Team

#include "proto_validator.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <iostream>

namespace jettison
{

ProtoValidator::ProtoValidator ()
{
  // Initialize the validator factory
  auto factory_or = buf::validate::ValidatorFactory::New ();
  if (!factory_or.ok ())
    {
      std::cerr << "Failed to create ValidatorFactory: "
                << factory_or.status ().message () << "\n";
      // Continue anyway - we'll fall back to basic validation
    }
  else
    {
      validator_factory_ = std::move (*factory_or);
    }
}

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

  // If protovalidate is available, use it for full validation
  if (validator_factory_)
    {
      // Create a validator for this validation
      auto validator = validator_factory_->NewValidator (&arena_, false);

      // Validate the message
      auto validation_result = validator.Validate (state);

      if (!validation_result.ok ())
        {
          result.errors.push_back ("Validation error: "
                                   + std::string (validation_result.status ().message ()));
          result.is_valid = false;
          return result;
        }

      // Check for violations
      if (!validation_result->success ())
        {
          result.is_valid = false;
          for (int i = 0; i < validation_result->violations_size (); ++i)
            {
              const auto &violation = validation_result->violations (i);
              const auto &proto = violation.proto ();
              std::string error_msg = "Field '";

              // Build field path from protobuf Violation
              if (proto.has_field ())
                {
                  // Convert FieldPath to string
                  const auto &field_path = proto.field ();
                  std::string path_str;
                  for (const auto &element : field_path.elements ())
                    {
                      if (!path_str.empty ())
                        path_str += ".";
                      path_str += element.field_name ();
                    }
                  error_msg += path_str;
                }
              else
                {
                  error_msg += "<root>";
                }

              error_msg += "': " + proto.message ();

              if (!proto.rule_id ().empty ())
                {
                  error_msg += " (rule: " + proto.rule_id () + ")";
                }

              result.errors.push_back (error_msg);
            }
        }

      return result;
    }

  // Fallback: Basic validation if protovalidate is not available
  // Check protocol_version (must be > 0 and <= 2147483647)
  if (state.protocol_version () == 0)
    {
      result.errors.push_back ("protocol_version must be greater than 0");
      result.is_valid = false;
    }

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

  return result;
}

} // namespace jettison

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2024 Jettison State RX Contributors

#include "json_converter.h"
#include <google/protobuf/util/json_util.h>

namespace jettison
{

std::string
JsonConverter::to_json (const ser::JonGUIState &state, bool pretty)
{
  google::protobuf::util::JsonPrintOptions options;
  options.add_whitespace = pretty;
  options.always_print_primitive_fields = true;
  options.preserve_proto_field_names = true;

  std::string json_string;
  auto status
      = google::protobuf::util::MessageToJsonString (state, &json_string, options);

  if (!status.ok ())
    {
      return "{\"error\": \"Failed to convert to JSON: "
             + std::string (status.message ()) + "\"}";
    }

  return json_string;
}

} // namespace jettison

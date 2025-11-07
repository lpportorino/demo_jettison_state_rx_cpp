// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Jettison Project Team

#include "dump_manager.h"
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

namespace jettison
{

DumpManager::DumpManager (const std::string &dump_dir) : dump_dir_ (dump_dir)
{
}

bool
DumpManager::ensure_dump_dir_exists ()
{
  struct stat st;
  if (stat (dump_dir_.c_str (), &st) == 0)
    {
      return S_ISDIR (st.st_mode);
    }

  // Try to create directory
  if (mkdir (dump_dir_.c_str (), 0755) == 0)
    {
      return true;
    }

  return false;
}

bool
DumpManager::save_dump (const uint8_t *data, size_t len, int sequence_number)
{
  if (!ensure_dump_dir_exists ())
    {
      std::cerr << "Failed to create dump directory: " << dump_dir_ << "\n";
      return false;
    }

  // Generate filename: dumps/state_0001.bin
  std::ostringstream filename;
  filename << dump_dir_ << "/state_" << std::setfill ('0') << std::setw (4)
           << sequence_number << ".bin";

  std::ofstream file (filename.str (), std::ios::binary);
  if (!file.is_open ())
    {
      std::cerr << "Failed to open file for writing: " << filename.str ()
                << "\n";
      return false;
    }

  file.write (reinterpret_cast<const char *> (data),
              static_cast<std::streamsize> (len));
  file.close ();

  if (!file.good ())
    {
      std::cerr << "Error writing to file: " << filename.str () << "\n";
      return false;
    }

  std::cout << "Saved dump to: " << filename.str () << "\n";
  return true;
}

std::vector<uint8_t>
DumpManager::read_dump (const std::string &filename)
{
  std::ifstream file (filename, std::ios::binary | std::ios::ate);
  if (!file.is_open ())
    {
      std::cerr << "Failed to open file: " << filename << "\n";
      return {};
    }

  auto size = file.tellg ();
  if (size <= 0)
    {
      std::cerr << "Invalid file size: " << filename << "\n";
      return {};
    }

  std::vector<uint8_t> data (static_cast<size_t> (size));

  file.seekg (0);
  file.read (reinterpret_cast<char *> (data.data ()),
             static_cast<std::streamsize> (size));

  if (!file.good ())
    {
      std::cerr << "Error reading file: " << filename << "\n";
      return {};
    }

  return data;
}

} // namespace jettison

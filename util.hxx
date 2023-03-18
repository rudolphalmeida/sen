#pragma once

#include <filesystem>
#include <vector>

#include "constants.hxx"

std::vector<byte> read_binary_file(const std::filesystem::path& path);

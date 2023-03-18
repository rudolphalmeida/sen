#pragma once

#include <filesystem>
#include <vector>

#include "sen.hxx"

std::vector<byte> read_binary_file(const std::filesystem::path& path);

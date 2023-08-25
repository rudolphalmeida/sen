#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <vector>

#include <spdlog/spdlog.h>

#include "util.hxx"

std::vector<byte> ReadBinaryFile(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        spdlog::error("File with path {} does not exist", path.string());
        std::exit(-1);
    }

    std::ifstream input_file(path.c_str(), std::ios::binary);
    input_file.unsetf(std::ios::skipws);

    // Determine the size
    auto size = input_file.tellg();
    input_file.seekg(0);

    std::vector<byte> buffer;
    buffer.reserve(size);

    buffer.insert(buffer.begin(), std::istream_iterator<byte>(input_file),
                  std::istream_iterator<byte>());

    return buffer;
}

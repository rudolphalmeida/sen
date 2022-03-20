//
// Created by rudolph on 20/3/22.
//

#include "utils.h"

std::vector<byte> readBinaryFile(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        spdlog::info("File {} does not exist. Exiting", path.string());
        std::exit(-1);
    }

    std::ifstream file(path, std::ios::binary);
    // Don't eat newlines in binary mode
    file.unsetf(std::ios::skipws);

    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<byte> data;
    data.reserve(fileSize);

    // Read the actual data
    data.insert(data.begin(), std::istream_iterator<byte>(file),
                std::istream_iterator<byte>());

    return data;
}

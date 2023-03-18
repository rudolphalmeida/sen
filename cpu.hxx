#pragma once

#include <memory>

#include <spdlog/spdlog.h>

#include "bus.hxx"
#include "constants.hxx"

class Cpu {
   private:
    std::shared_ptr<Bus> bus{};

   public:
    Cpu() = default;

    Cpu(std::shared_ptr<Bus> bus) : bus{std::move(bus)} { spdlog::debug("Initialized CPU"); }
};

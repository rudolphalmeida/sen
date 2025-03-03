//
// Created by rudolph on 1/03/25.
//

#pragma once

#include <imgui.h>
#include <spdlog/common.h>
#include <spdlog/details/log_msg.h>
#include <spdlog/details/synchronous_factory.h>
#include <spdlog/sinks/base_sink.h>

#include <algorithm>
#include <boost/circular_buffer.hpp>
#include <boost/circular_buffer/base.hpp>
#include <cstddef>
#include <memory>
#include <mutex>
#include <ranges>
#include <string>

struct formatted_log {
    std::string message;
    spdlog::level::level_enum level;
    decltype(spdlog::details::log_msg::time) time_point;
};

template<typename Mutex = std::mutex>
class imgui_sink final: public spdlog::sinks::base_sink<Mutex> {
  public:
    explicit imgui_sink(const size_t init_size) : msg_buf(init_size) {
        spdlog::set_level(level_options[selected_level_idx].second);
        spdlog::set_level(spdlog::level::trace);
    }

    void render_window(bool* open, const ImGuiWindowFlags flags) {
        if (ImGui::Begin("Log", open, flags)) {
            if (ImGui::Button("Clear")) {
                clear();
            }
            ImGui::SameLine();
            const auto* const selected_level_name = level_options[selected_level_idx].first;
            const auto selected_level = level_options[selected_level_idx].second;
            if (ImGui::BeginCombo("Log Levels", selected_level_name)) {
                for (size_t i = 0; i < IM_ARRAYSIZE(level_options); i++) {
                    const auto* const label = level_options[i].first;
                    const auto level = level_options[i].second;
                    const bool selected = level >= selected_level;
                    if (ImGui::Selectable(label, selected)) {
                        selected_level_idx = i;
                        spdlog::set_level(level);
                    }

                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }

            if (ImGui::InputInt("Max Log output", &max_log_output, 1, 10)) {
                max_log_output = std::max(max_log_output, 1);
            }
            ImGui::Separator();

            for (const formatted_log& log_msg : msg_buf | std::ranges::views::reverse
                     | std::ranges::views::filter([selected_level](
                                                      const formatted_log& msg
                                                  ) { return msg.level >= selected_level; })
                     | std::ranges::views::take(max_log_output) | std::ranges::views::reverse) {
                const char* level = nullptr;
                auto level_color = ImVec4(1.0F, 1.0F, 1.0F, 1.0F);
                switch (log_msg.level) {
                    case spdlog::level::trace:
                        level = "trace";
                        level_color = ImVec4(1.0F, 1.0F, 1.0F, 1.0f);
                        break;
                    case spdlog::level::debug:
                        level = "debug";
                        level_color = ImVec4(0.4F, 1.0F, 1.0F, 1.0f);
                        break;
                    case spdlog::level::info:
                        level = "info";
                        level_color = ImVec4(0.4F, 1.0F, 0.4F, 1.0f);
                        break;
                    case spdlog::level::err:
                        level = "error";
                        level_color = ImVec4(1.0F, 0.2F, 0.2F, 1.0f);
                        break;
                    default:
                        break;
                }
                if (level != nullptr) {
                    ImGui::Text("[");
                    ImGui::SameLine();
                    ImGui::TextColored(level_color, "%s", level);
                    ImGui::SameLine();
                    ImGui::Text("] ");
                    ImGui::SameLine();
                }

                ImGui::Text(log_msg.message.c_str());
            }

            ImGui::Separator();
            ImGui::Text("Log size: %ld", msg_buf.size());
        }

        ImGui::End();
    }

    void resize(const size_t new_size) {
        msg_buf.resize(new_size);
    }

    void clear() {
        msg_buf.clear();
    }

  private:
    boost::circular_buffer<formatted_log> msg_buf;
    int max_log_output{10};

    std::pair<const char*, spdlog::level::level_enum> level_options[4] = {
        {"TRACE", spdlog::level::trace},
        {"DEBUG", spdlog::level::debug},
        {"INFO", spdlog::level::info},
        {"ERROR", spdlog::level::err}
    };
    size_t selected_level_idx = 1;

  protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        const auto level = msg.level;
        const auto time_point = msg.time;
        const auto payload = std::string(msg.payload.begin(), msg.payload.end());
        msg_buf.push_back({
            .message = payload,
            .level = level,
            .time_point = time_point,
        });
    }

    void flush_() override {
        // Do nothing
    }
};

template<typename Factory = spdlog::synchronous_factory>
std::shared_ptr<spdlog::logger>
imgui_sink_mt(const std::string& logger_name, const size_t init_size) {
    return Factory::template create<imgui_sink<>>(logger_name, init_size);
}

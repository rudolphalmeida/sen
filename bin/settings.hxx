#pragma once

#include <spdlog/spdlog.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <libconfig.h++>
#include <ranges>
#include <vector>

#include "constants.hxx"

constexpr int DEFAULT_SCALE_FACTOR = 4;

enum class UiStyle : uint8_t {
    Classic = 0,
    Light = 1,
    Dark = 2,
    SuperDark = 3,
};

constexpr int NUM_PANELS = 9;
enum class UiPanel : uint8_t {
    Registers = 0,
    PatternTables = 1,
    PpuMemory = 2,
    CpuMemory = 3,
    Sprites = 4,
    Disassembly = 5,
    Debugger = 6,
    Logs = 7,
    VolumeControl = 8,
};

enum class FilterType : uint8_t {
    NoFilter = 0,
    Ntsc = 1,
};

struct SenSettings {
    libconfig::Config cfg;
    std::array<bool, NUM_PANELS> open_panels{};

    SenSettings() {
        const auto settings_file_path = settings_file_path_for_platform();
        try {
            cfg.readFile(settings_file_path.string());
        } catch (const libconfig::FileIOException&) {
            spdlog::info("Failed to find settings. Using default");
        } catch (const libconfig::ParseException& e) {
            spdlog::error(
                "Failed to parse settings in file {} with {}. Using default",
                e.getFile(),
                e.getLine()
            );
        }

        libconfig::Setting& root = cfg.getRoot();
        if (!root.exists("ui")) {
            root.add("ui", libconfig::Setting::TypeGroup);
        }
        libconfig::Setting& ui_settings = root["ui"];
        if (!ui_settings.exists("scale")) {
            ui_settings.add("scale", libconfig::Setting::TypeInt) = DEFAULT_SCALE_FACTOR;
        }
        if (!ui_settings.exists("recents")) {
            ui_settings.add("recents", libconfig::Setting::TypeArray);
        }
        if (!ui_settings.exists("style")) {
            ui_settings.add("style", libconfig::Setting::TypeInt) = static_cast<int>(UiStyle::Dark);
        }
        if (!ui_settings.exists("filter")) {
            ui_settings.add("filter", libconfig::Setting::TypeInt) =
                static_cast<int>(FilterType::NoFilter);
        }
        if (!ui_settings.exists("open_panels")) {
            ui_settings.add("open_panels", libconfig::Setting::TypeInt) = 0;
        } else {
            const auto saved_open_panels = static_cast<unsigned>(ui_settings["open_panels"]);
            for (int i = 0; i < NUM_PANELS; i++) {
                open_panels.at(i) = (saved_open_panels & (1U << static_cast<unsigned>(i))) != 0;
            }
        }

        if (!ui_settings.exists("width")) {
            ui_settings.add("width", libconfig::Setting::TypeInt) =
                (NES_WIDTH * DEFAULT_SCALE_FACTOR) + 15;
        }
        if (!ui_settings.exists("height")) {
            ui_settings.add("height", libconfig::Setting::TypeInt) =
                (NES_HEIGHT * DEFAULT_SCALE_FACTOR) + 55;
        }
    }

    static std::filesystem::path settings_file_path_for_platform() {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
        return std::filesystem::path(std::getenv("LOCALAPPDATA")) / "sen" / "config.cfg";
#elif defined(__linux__)
        return std::filesystem::path(std::getenv("HOME")) / ".sen" / "config.cfg";
#else
        return std::filesystem::path("config.cfg");
#endif
    }

    [[nodiscard]] int Width() const {
        return cfg.getRoot()["ui"]["width"];
    }

    void Width(const int width) const {
        cfg.getRoot()["ui"]["width"] = width;
    }

    [[nodiscard]] int Height() const {
        return cfg.getRoot()["ui"]["height"];
    }

    void Height(const int height) const {
        cfg.getRoot()["ui"]["height"] = height;
    }

    [[nodiscard]] int ScaleFactor() const {
        return cfg.getRoot()["ui"]["scale"];
    }

    void SetScale(const int scale) const {
        cfg.getRoot()["ui"]["scale"] = scale;
    }

    [[nodiscard]] FilterType GetFilterType() const {
        return static_cast<enum FilterType>(static_cast<int>(cfg.getRoot()["ui"]["filter"]));
    }

    void SetFilterType(enum FilterType filter) const {
        cfg.getRoot()["ui"]["filter"] = static_cast<int>(filter);
    }

    [[nodiscard]] UiStyle GetUiStyle() const {
        return static_cast<enum UiStyle>(static_cast<int>(cfg.getRoot()["ui"]["style"]));
    }

    void SetUiStyle(enum UiStyle style) const {
        cfg.getRoot()["ui"]["style"] = static_cast<int>(style);
    }

    [[nodiscard]] std::array<bool, NUM_PANELS>& GetOpenPanels() {
        return open_panels;
    }

    void TogglePanel(UiPanel panel) {
        const auto panel_id = static_cast<unsigned>(panel);
        const auto open_panels_config = static_cast<unsigned>(cfg.getRoot()["ui"]["open_panels"]);
        cfg.getRoot()["ui"]["open_panels"] =
            static_cast<int>(open_panels_config ^ (1U << panel_id));
        open_panels.at(panel_id) = !open_panels.at(panel_id);
    }

    void SyncPanelStates() const {
        unsigned int panel_config = 0;
        for (size_t i = 0; i < NUM_PANELS; i++) {
            panel_config |= (open_panels.at(i) ? 0b1U : 0b0U) << i;
        }
        cfg.getRoot()["ui"]["open_panels"] = static_cast<int>(panel_config);
    }

    void RecentRoms(std::vector<const char*>& paths) const {
        const libconfig::Setting& recents = cfg.getRoot()["ui"]["recents"];
        paths.resize(recents.getLength());
        std::copy(recents.begin(), recents.end(), paths.begin());
    }

    void PushRecentPath(const char* path) const {
        libconfig::Setting& recents = cfg.getRoot()["ui"]["recents"];
        for (int i = 0; i < recents.getLength(); i++) {
            // TODO: Use something better
            if (strcmp(recents[i], path) == 0) {
                return;
            }
        }
        recents.add(libconfig::Setting::TypeString) = path;
    }

    void write_to_disk(bool create_file = true) {
        const auto settings_file_path = settings_file_path_for_platform();
        try {
            SyncPanelStates();
            cfg.writeFile(settings_file_path.string().c_str());
        } catch (const libconfig::FileIOException& e) {
            if (create_file) {
                std::filesystem::create_directories(settings_file_path.parent_path());
                std::ofstream ofs(settings_file_path.string());
                ofs.close();
                spdlog::info("Created configuration file at {}", settings_file_path.string());
                write_to_disk(false);
                return;
            }
            spdlog::error("Failed to save settings: {}", e.what());
        }
    }
};

#pragma once

#include <spdlog/spdlog.h>

#include <cstring>
#include <libconfig.h++>
#include <vector>

#include "constants.hxx"

constexpr int DEFAULT_SCALE_FACTOR = 4;

enum class UiStyle {
    Classic = 0,
    Light = 1,
    Dark = 2,
    SuperDark = 3,
};

constexpr int NUM_PANELS = 9;
enum class UiPanel {
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

enum class FilterType {
    NoFilter = 0,
    Ntsc = 1,
};

struct SenSettings {
    libconfig::Config cfg{};
    std::array<bool, NUM_PANELS> open_panels{};

    SenSettings() {
        try {
            // TODO: Change this path to be the standard config directory for each OS
            cfg.readFile("test.cfg");
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
        int scale_factor = DEFAULT_SCALE_FACTOR;
        if (!ui_settings.exists("scale")) {
            ui_settings.add("scale", libconfig::Setting::TypeInt) = DEFAULT_SCALE_FACTOR;
        } else {
            scale_factor = ScaleFactor();
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
            const int saved_open_panels = ui_settings["open_panels"];
            for (int i = 0; i < NUM_PANELS; i++) {
                open_panels[i] = (saved_open_panels & (1U << i)) != 0;
            }
        }
        int width = NES_WIDTH * DEFAULT_SCALE_FACTOR + 15;
        int height = NES_HEIGHT * DEFAULT_SCALE_FACTOR + 55;
        if (!ui_settings.exists("width")) {
            ui_settings.add("width", libconfig::Setting::TypeInt) = width;
        } else {
            width = Width();
        }
        if (!ui_settings.exists("height")) {
            ui_settings.add("height", libconfig::Setting::TypeInt) = height;
        } else {
            height = Height();
        }
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
        const auto panel_id = static_cast<int>(panel);
        const auto open_panels_config = static_cast<int>(cfg.getRoot()["ui"]["open_panels"]);
        cfg.getRoot()["ui"]["open_panels"] = open_panels_config ^ (1 << panel_id);
        open_panels[panel_id] = !open_panels[panel_id];
    }

    void SyncPanelStates() const {
        int panel_config = 0;
        for (int i = 0; i < NUM_PANELS; i++) {
            panel_config |= static_cast<int>(open_panels[i]) << i;
        }
        cfg.getRoot()["ui"]["open_panels"] = panel_config;
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
};

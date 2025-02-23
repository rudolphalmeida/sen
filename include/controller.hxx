#pragma once

#include <spdlog/spdlog.h>

#include "constants.hxx"

enum class ControllerKey : byte {
    A = 0x01,
    B = 0x02,
    Select = 0x04,
    Start = 0x08,
    Up = 0x10,
    Down = 0x20,
    Left = 0x40,
    Right = 0x80,
};

enum class ControllerPort {
    Port1,
    Port2,
};

class Controller {
  public:
    byte CpuRead(word address) {
        // TODO: Open bus values
        switch (address) {
            case 0x4016:
                if (strobe) {
                    return key_state_1 & 0b1;
                } else {
                    byte value = key_shift_reg_1 & 0b1;
                    key_shift_reg_1 = (key_shift_reg_1 >> 1) | 0x80; // Ensure all 1s after 8 reads
                    return value;
                }
            case 0x4017:
                if (strobe) {
                    return key_state_2 & 0b1;
                } else {
                    byte value = key_shift_reg_2 & 0b1;
                    key_shift_reg_2 = (key_shift_reg_2 >> 1) | 0x80; // Ensure all 1s after 8 reads
                    return value;
                }
            default:
                spdlog::error("Read from invalid controller address 0x{:#04X}");
                return 0x00;
        }
    }

    void CpuWrite(word address, byte data) {
        if (address == 0x4016) {
            auto old_strobe = strobe;
            strobe = (data & 0b1) == 0b1;

            if (old_strobe && !strobe) { // Stop polling
                key_shift_reg_1 = key_state_1;
                key_shift_reg_2 = key_state_2;
            }
       } else {
            spdlog::error(
                "Write to invalid controller address 0x{:#04X} with {:08b}",
                address,
                data
            );
        }
    }

    void set_pressed_keys(ControllerPort port, byte keys) {
        switch (port) {
            case ControllerPort::Port1:
                key_state_1 = keys;
                break;
            case ControllerPort::Port2:
                key_state_2 = keys;
                break;
        }
    }

  private:
    bool strobe{false};

    byte key_state_1{0x00};
    byte key_shift_reg_1{0x00};

    byte key_state_2{0x00};
    byte key_shift_reg_2{0x00};
};

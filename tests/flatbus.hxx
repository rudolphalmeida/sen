#include <array>

#include "../constants.hxx"

class FlatBus {
   private:
    std::array<byte, 65536> ram{};

   public:
    unsigned int cycles{};

    FlatBus() {}

    void Tick() { cycles++; }

    byte UntickedCpuRead(word address) { return ram[address]; }

    void UntickedCpuWrite(word address, byte data) { ram[address] = data; }

    byte CpuRead(word address) {
        Tick();
        return UntickedCpuRead(address);
    }

    void CpuWrite(word address, byte data) {
        Tick();
        UntickedCpuWrite(address, data);
    }
};
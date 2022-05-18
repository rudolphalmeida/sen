//
// Created by rudolph on 8/5/22.
//

#include "nes.hxx"

void Nes::Start() {
    cpu.Start();
}

void Nes::RunFrame() {
    // TODO: Proper frame loop
    cpu.Tick();
}

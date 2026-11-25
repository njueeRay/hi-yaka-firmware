#pragma once

#include <driver/i2c_master.h>

// One-shot, laboratory-only I2S experiments. The speaker amplifier stays off.
void HiYaKaRunAudioAbDiagnostics(i2c_master_bus_handle_t i2c_bus);

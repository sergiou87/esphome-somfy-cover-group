#pragma once

#include "esphome/core/component.h"
#include "esphome/components/cover/cover.h"

#include <SomfyRemote.h>

class EEPROMRollingCodeStorage;

namespace esphome {
namespace somfy_cover_group {

class SomfySwitch;

class SomfyCover : public cover::Cover {
private:
  uint32_t remoteCode;
  SomfyRemote *remote;
  EEPROMRollingCodeStorage *storage;
  SomfySwitch *progSwitch;
  SomfySwitch *favoriteSwitch;

  void program();
  void favorite();

public:
  SomfyCover(const std::string &name, byte emitterPin, int eepromAddress, uint32_t remoteCode);

  cover::CoverTraits get_traits() override;

  void sendCC1101Command(Command command, int repeat = 1);

  void control(const cover::CoverCall &call) override;

  void register_to_app();
};

}  // namespace somfy_cover_group
}  // namespace esphome

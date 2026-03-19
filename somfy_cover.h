#pragma once

#include "esphome/core/component.h"
#include "esphome/components/cover/cover.h"

#include <SomfyRemote.h>

#include <cstdint>

class EEPROMRollingCodeStorage;

namespace esphome {
namespace somfy_cover_group {

class SomfySwitch;

class SomfyCover : public cover::Cover, public Component {
private:
  uint32_t remote_code_;
  uint32_t calibration_ms_;
  uint32_t movement_start_ms_{0};
  float movement_start_position_{cover::COVER_CLOSED};
  float target_position_{cover::COVER_CLOSED};
  SomfyRemote *remote_;
  EEPROMRollingCodeStorage *storage_;
  SomfySwitch *prog_switch_;
  SomfySwitch *favorite_switch_;

  void start_move_to_position_(float target_position);
  void begin_motion_(float target_position, bool send_command);
  void finish_motion_();
  void stop_motion_(bool send_stop_command, bool save_state);
  void sync_position_();
  bool is_endpoint_position_(float position) const;

  void program();
  void favorite();

public:
  SomfyCover(const std::string &name, byte emitterPin, int eepromAddress, uint32_t remote_code,
             uint32_t calibration_ms);

  void setup() override;
  void dump_config() override;

  cover::CoverTraits get_traits() override;

  void send_cc1101_command(Command command, int repeat = 1);

  void control(const cover::CoverCall &call) override;

  void register_to_app();
};

}  // namespace somfy_cover_group
}  // namespace esphome

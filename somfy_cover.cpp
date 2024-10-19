#include "somfy_cover.h"

#include "esphome/core/application.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/switch/switch.h"

#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <EEPROMRollingCodeStorage.h>
#include <SomfyRemote.h>

#define COVER_OPEN 1.0f
#define COVER_CLOSED 0.0f

#define TILT_OPEN 1.0f
#define TILT_CLOSED 0.0f

namespace esphome {
namespace somfy_cover_group {

class SomfySwitch : public switch_::Switch {
private:
  // action as lambda
  std::function<void()> action;
public:
  // constructor with name and action
  SomfySwitch(const std::string &name, std::function<void()> action) : switch_::Switch() {
    this->action = action;

    this->set_object_id(strdup(name.c_str()));
    this->set_name(strdup(name.c_str()));
  }

  void write_state(bool state) override {
    if (state) {
      action();
    }
    publish_state(false);
  }
};

SomfyCover::SomfyCover(const std::string &name, byte emitterPin, int eepromAddress, uint32_t remoteCode)
    : Cover() {
  this->remoteCode = remoteCode;
  storage = new EEPROMRollingCodeStorage(eepromAddress);
  remote = new SomfyRemote(emitterPin, remoteCode, storage);

  favoriteSwitch = new SomfySwitch(name + " favorite", [this]() {
    this->favorite();
  });
  progSwitch = new SomfySwitch(name + " prog", [this]() {
    this->program();
  });

  this->set_object_id(strdup(name.c_str()));
  this->set_name(strdup(name.c_str()));
}

void SomfyCover::register_to_app() {
  App.register_switch(favoriteSwitch);
  App.register_switch(progSwitch);
  App.register_cover(this);
}

cover::CoverTraits SomfyCover::get_traits() {
  auto traits = cover::CoverTraits();
  traits.set_is_assumed_state(true);
  traits.set_supports_position(false);
  traits.set_supports_stop(true);
  traits.set_supports_tilt(false);
  return traits;
}

void SomfyCover::sendCC1101Command(Command command, int repeat) {
  ELECHOUSE_cc1101.SetTx();
  remote->sendCommand(command, repeat);
  ELECHOUSE_cc1101.setSidle();
}

void SomfyCover::control(const cover::CoverCall &call) {
  if (call.get_position().has_value()) {
    float pos = *call.get_position();

    if (pos == COVER_OPEN) {
      ESP_LOGI("somfy", "OPEN");
      sendCC1101Command(Command::Up);
    } else if (pos == COVER_CLOSED) {
      ESP_LOGI("somfy", "CLOSE");
      sendCC1101Command(Command::Down);
    } else {
      ESP_LOGI("somfy", "WAT");
    }

    this->position = pos;
    this->publish_state();
  }

  if (call.get_stop()) {
    ESP_LOGI("somfy", "STOP");
    sendCC1101Command(Command::My);
  }

  if(call.get_tilt().has_value()) {
    float tilt = *call.get_tilt();      
    ESP_LOGI("somfy", "tilt: %.1f",tilt );
    if(tilt == TILT_OPEN) {
      sendCC1101Command(Command::Up, 4);
    } else {
      sendCC1101Command(Command::Down, 4);
    }
  }
}

void SomfyCover::program() {
  ESP_LOGI("somfy", "PROG");
  sendCC1101Command(Command::Prog);
}

void SomfyCover::favorite() {
  ESP_LOGI("somfy", "FAVORITE (MY)");
  sendCC1101Command(Command::My);
}

}  // namespace somfy_cover_group
}  // namespace esphome

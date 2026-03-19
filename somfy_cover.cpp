#include "somfy_cover.h"

#include "esphome/core/application.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esphome/components/switch/switch.h"

#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <EEPROMRollingCodeStorage.h>
#include <SomfyRemote.h>

#include <algorithm>
#include <cmath>

namespace esphome {
namespace somfy_cover_group {

static const char *const TAG = "somfy.cover";
static const uint32_t POSITION_UPDATE_INTERVAL_MS = 250;
static const uint32_t DIRECTION_CHANGE_DELAY_MS = 400;
static const float POSITION_EPSILON = 0.01f;

class SomfySwitch : public switch_::Switch {
private:
  std::function<void()> action_;
public:
  SomfySwitch(const std::string &name, std::function<void()> action) : switch_::Switch() {
    this->action_ = action;

    this->set_object_id(strdup(name.c_str()));
    this->set_name(strdup(name.c_str()));
  }

  void write_state(bool state) override {
    if (state) {
      this->action_();
    }
    publish_state(false);
  }
};

SomfyCover::SomfyCover(const std::string &name, byte emitterPin, int eepromAddress, uint32_t remote_code,
                       uint32_t calibration_ms)
    : Cover(), Component() {
  this->remote_code_ = remote_code;
  this->calibration_ms_ = calibration_ms;
  this->storage_ = new EEPROMRollingCodeStorage(eepromAddress);
  this->remote_ = new SomfyRemote(emitterPin, remote_code, this->storage_);

  this->favorite_switch_ = new SomfySwitch(name + " favorite", [this]() {
    this->favorite();
  });
  this->prog_switch_ = new SomfySwitch(name + " prog", [this]() {
    this->program();
  });

  this->set_object_id(strdup(name.c_str()));
  this->set_name(strdup(name.c_str()));
  this->position = cover::COVER_CLOSED;
  this->tilt = cover::COVER_OPEN;
}

void SomfyCover::setup() {
  auto restored = this->restore_state_();
  if (restored.has_value()) {
    this->position = clamp(restored->position, cover::COVER_CLOSED, cover::COVER_OPEN);
  } else {
    this->position = cover::COVER_CLOSED;
  }

  this->tilt = cover::COVER_OPEN;
  this->current_operation = cover::COVER_OPERATION_IDLE;

  this->set_interval("position_update", POSITION_UPDATE_INTERVAL_MS, [this]() {
    if (this->current_operation == cover::COVER_OPERATION_IDLE) {
      return;
    }

    this->sync_position_();
    this->publish_state(false);
  });

  this->publish_state(false);
}

void SomfyCover::dump_config() {
  ESP_LOGCONFIG(TAG, "Somfy Cover '%s'", this->get_name().c_str());
  ESP_LOGCONFIG(TAG, "  Remote Code: 0x%06" PRIx32, this->remote_code_);
  ESP_LOGCONFIG(TAG, "  Calibration: %" PRIu32 " ms", this->calibration_ms_);
}

void SomfyCover::register_to_app() {
  App.register_component(this);
  App.register_switch(this->favorite_switch_);
  App.register_switch(this->prog_switch_);
  App.register_cover(this);
}

cover::CoverTraits SomfyCover::get_traits() {
  auto traits = cover::CoverTraits();
  traits.set_is_assumed_state(true);
  traits.set_supports_position(true);
  traits.set_supports_stop(true);
  traits.set_supports_tilt(false);
  return traits;
}

void SomfyCover::send_cc1101_command(Command command, int repeat) {
  ELECHOUSE_cc1101.SetTx();
  this->remote_->sendCommand(command, repeat);
  ELECHOUSE_cc1101.setSidle();
}

bool SomfyCover::is_endpoint_position_(float position) const {
  return std::fabs(position - cover::COVER_OPEN) <= POSITION_EPSILON ||
         std::fabs(position - cover::COVER_CLOSED) <= POSITION_EPSILON;
}

void SomfyCover::sync_position_() {
  if (this->current_operation == cover::COVER_OPERATION_IDLE) {
    return;
  }

  const uint32_t now = millis();
  const float elapsed_fraction = std::min(1.0f, static_cast<float>(now - this->movement_start_ms_) /
                                                    static_cast<float>(this->calibration_ms_));
  const float delta = elapsed_fraction;

  if (this->current_operation == cover::COVER_OPERATION_OPENING) {
    this->position = std::min(this->target_position_, this->movement_start_position_ + delta);
  } else if (this->current_operation == cover::COVER_OPERATION_CLOSING) {
    this->position = std::max(this->target_position_, this->movement_start_position_ - delta);
  }
}

void SomfyCover::finish_motion_() {
  if (this->current_operation == cover::COVER_OPERATION_IDLE) {
    return;
  }

  if (!this->is_endpoint_position_(this->target_position_)) {
    ESP_LOGI(TAG, "'%s' stopping at %.0f%% open", this->get_name().c_str(), this->target_position_ * 100.0f);
    this->send_cc1101_command(Command::My);
  }

  this->position = this->target_position_;
  this->current_operation = cover::COVER_OPERATION_IDLE;
  this->publish_state();
}

void SomfyCover::stop_motion_(bool send_stop_command, bool save_state) {
  this->cancel_timeout("direction_change");
  this->cancel_timeout("movement_stop");
  this->sync_position_();

  if (send_stop_command && this->current_operation != cover::COVER_OPERATION_IDLE) {
    this->send_cc1101_command(Command::My);
  }

  this->current_operation = cover::COVER_OPERATION_IDLE;
  this->target_position_ = this->position;
  this->publish_state(save_state);
}

void SomfyCover::begin_motion_(float target_position, bool send_command) {
  const auto next_operation = target_position > this->position ? cover::COVER_OPERATION_OPENING
                                                               : cover::COVER_OPERATION_CLOSING;
  const uint32_t travel_time = std::max<uint32_t>(1, static_cast<uint32_t>(
      std::round(std::fabs(target_position - this->position) * static_cast<float>(this->calibration_ms_))));

  if (send_command) {
    ESP_LOGI(TAG, "'%s' moving %s to %.0f%% open", this->get_name().c_str(),
             next_operation == cover::COVER_OPERATION_OPENING ? "up" : "down", target_position * 100.0f);
    this->send_cc1101_command(next_operation == cover::COVER_OPERATION_OPENING ? Command::Up : Command::Down);
  }

  this->movement_start_ms_ = millis();
  this->movement_start_position_ = this->position;
  this->target_position_ = target_position;
  this->current_operation = next_operation;

  this->cancel_timeout("movement_stop");
  this->set_timeout("movement_stop", travel_time, [this]() { this->finish_motion_(); });
  this->publish_state(false);
}

void SomfyCover::start_move_to_position_(float target_position) {
  this->cancel_timeout("direction_change");
  this->sync_position_();

  if (std::fabs(target_position - this->position) <= POSITION_EPSILON) {
    this->position = target_position;
    this->current_operation = cover::COVER_OPERATION_IDLE;
    this->publish_state();
    return;
  }

  const auto next_operation = target_position > this->position ? cover::COVER_OPERATION_OPENING
                                                               : cover::COVER_OPERATION_CLOSING;

  if (this->current_operation != cover::COVER_OPERATION_IDLE && next_operation != this->current_operation) {
    ESP_LOGI(TAG, "'%s' reversing direction", this->get_name().c_str());
    this->stop_motion_(true, false);
    this->set_timeout("direction_change", DIRECTION_CHANGE_DELAY_MS,
                      [this, target_position]() { this->start_move_to_position_(target_position); });
    return;
  }

  this->begin_motion_(target_position, this->current_operation == cover::COVER_OPERATION_IDLE);
}

void SomfyCover::control(const cover::CoverCall &call) {
  this->cancel_timeout("direction_change");

  if (call.get_stop()) {
    ESP_LOGI(TAG, "'%s' stop requested", this->get_name().c_str());
    this->stop_motion_(this->current_operation != cover::COVER_OPERATION_IDLE, true);
    return;
  }

  if (call.get_position().has_value()) {
    const float target_position = clamp(*call.get_position(), cover::COVER_CLOSED, cover::COVER_OPEN);
    this->start_move_to_position_(target_position);
  }
}

void SomfyCover::program() {
  ESP_LOGI(TAG, "'%s' prog", this->get_name().c_str());
  this->send_cc1101_command(Command::Prog);
}

void SomfyCover::favorite() {
  ESP_LOGI(TAG, "'%s' favorite", this->get_name().c_str());
  this->send_cc1101_command(Command::My);
}

}  // namespace somfy_cover_group
}  // namespace esphome

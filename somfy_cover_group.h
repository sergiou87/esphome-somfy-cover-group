#pragma once

#include "esphome/core/component.h"

namespace esphome {
namespace somfy_cover_group {

class SomfyCover;

class SomfyCoverGroup : public Component {
 private:
  std::vector<SomfyCover *> covers;

 public:
  void setup() override;
  void dump_config() override;

  void add_cover(const std::string &name, uint32_t remoteCode);
};

}  // namespace somfy_cover_group
}  // namespace esphome

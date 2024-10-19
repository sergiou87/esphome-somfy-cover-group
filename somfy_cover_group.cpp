#include "somfy_cover_group.h"

#include "esphome/core/application.h"
#include "esphome/core/log.h"
#include "somfy_cover.h"

#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <EEPROMRollingCodeStorage.h>

// Pin 5 is D1 on the Wemos D1 Mini, which is wired to GDO0 on the CC1101
#define EMITTER_GPIO 5
#define CC1101_FREQUENCY 433.42

namespace esphome {
namespace somfy_cover_group {

static const char *TAG = "somfy.cover_group";

void SomfyCoverGroup::setup() {
    ESP_LOGI("somfy", "setting up Somfy ESP Remote with %d covers...", covers.size());
    Serial.begin(115200);

    if (ELECHOUSE_cc1101.getCC1101()){       // Check the CC1101 Spi connection.
        ESP_LOGI("somfy", "Connection OK");
    }else{
        ESP_LOGI("somfy", "Connection Error");
    }

    // need to set GPIO PIN 5 (D1) as OUTPUT, otherwise no commands will be sent
    pinMode(EMITTER_GPIO, OUTPUT);
    digitalWrite(EMITTER_GPIO, LOW);

    // We need 2 bytes for each cover to store the rolling code
    EEPROM.begin(covers.size() * 2);

    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.setMHZ(CC1101_FREQUENCY);
    ESP_LOGI("somfy", "Somfy ESP Remote is set up!");
}

void SomfyCoverGroup::dump_config() {
    ESP_LOGCONFIG(TAG, "Somfy cover group");
}

void SomfyCoverGroup::add_cover(const std::string &name, uint32_t remoteCode) {
    // log eeprom address and remote code in hex
    ESP_LOGI("somfy", "adding cover with remote code 0x%x", remoteCode);
    auto cover = new SomfyCover(name, EMITTER_GPIO, covers.size() * 2, remoteCode);
    covers.push_back(cover);

    cover->register_to_app();
}

}  // namespace somfy_cover_group
}  // namespace esphome

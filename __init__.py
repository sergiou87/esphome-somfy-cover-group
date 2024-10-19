import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import cover
from esphome.const import CONF_ID, CONF_NAME, UNIT_EMPTY, ICON_EMPTY

somfy_cover_group_ns = cg.esphome_ns.namespace("somfy_cover_group")
SomfyCoverGroup = somfy_cover_group_ns.class_("SomfyCoverGroup", cg.Component)
SomfyCover = somfy_cover_group_ns.class_("SomfyCover", cover.Cover, cg.Component)

AUTO_LOAD = ["cover", "switch"]

CONF_COVERS = "covers"
CONF_REMOTE_CODE = "remote_code"

CONFIG_SOMFY_COVER_SCHEMA = cover.COVER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(SomfyCover),
        cv.Required(CONF_NAME): cv.string_strict,
        cv.Required(CONF_REMOTE_CODE): cv.uint32_t,
    }
).extend(cv.COMPONENT_SCHEMA)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(SomfyCoverGroup),
        cv.Required(CONF_COVERS): cv.ensure_list(CONFIG_SOMFY_COVER_SCHEMA),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    for cc in config[CONF_COVERS]:
      cg.add(var.add_cover(cc[CONF_NAME], cc[CONF_REMOTE_CODE]))

    cg.add_library("EEPROM", None)
    cg.add_library("SPI", None)
    cg.add_library(
        "SmartRC-CC1101-Driver-Lib",
        "2.5.7",
    )
    cg.add_library(
        "Somfy_Remote_Lib",
        "0.4.1"
    )

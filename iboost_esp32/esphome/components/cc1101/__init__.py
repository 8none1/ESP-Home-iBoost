import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome import pins

CONF_CS_PIN = "cs_pin"
CONF_MISO_PIN = "miso_pin"

# Define the namespace for the CC1101 component
cc1101_ns = cg.esphome_ns.namespace("cc1101")
CC1101 = cc1101_ns.class_("CC1101", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(CC1101),
        cv.Optional(CONF_CS_PIN, default=5): cv.int_range(min=0, max=39),
        cv.Optional(CONF_MISO_PIN, default=4): cv.int_range(min=0, max=39),
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    if CONF_CS_PIN in config:
        cg.add(var.set_cs_pin(config[CONF_CS_PIN]))
    if CONF_MISO_PIN in config:
        cg.add(var.set_miso_pin(config[CONF_MISO_PIN]))

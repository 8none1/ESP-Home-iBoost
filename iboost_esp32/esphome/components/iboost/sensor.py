import esphome.codegen as cg
import esphome.components.sensor as sensor
import esphome.components.text_sensor as text_sensor
import esphome.components.binary_sensor as binary_sensor
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.const import (
    UNIT_WATT, UNIT_WATT_HOURS, UNIT_MINUTE,
    DEVICE_CLASS_POWER, DEVICE_CLASS_ENERGY, DEVICE_CLASS_DURATION,
    DEVICE_CLASS_HEAT, DEVICE_CLASS_BATTERY,
    STATE_CLASS_TOTAL_INCREASING, ICON_FLASH
)

DEPENDENCIES = ["cc1101"]
AUTO_LOAD = ["sensor", "text_sensor", "binary_sensor"]

# Define iBoost component namespace
iboost_ns = cg.esphome_ns.namespace("iboost")
iBoost = iboost_ns.class_("iBoost", cg.PollingComponent)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(iBoost),

        # Numeric Sensors
        cv.Optional("heating_import"): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT, accuracy_decimals=0, device_class=DEVICE_CLASS_POWER
        ),
        cv.Optional("heating_power"): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT, accuracy_decimals=0, device_class=DEVICE_CLASS_POWER
        ),
        cv.Optional("heating_today"): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT_HOURS, accuracy_decimals=0, device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING
        ),
        cv.Optional("heating_yesterday"): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT_HOURS, accuracy_decimals=0, device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING
        ),
        cv.Optional("heating_last_7"): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT_HOURS, accuracy_decimals=0, device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING
        ),
        cv.Optional("heating_last_28"): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT_HOURS, accuracy_decimals=0, device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING
        ),
        cv.Optional("heating_last_gt"): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT_HOURS, accuracy_decimals=0, device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING
        ),
        cv.Optional("heating_boost_time"): sensor.sensor_schema(
            unit_of_measurement=UNIT_MINUTE, accuracy_decimals=0, device_class=DEVICE_CLASS_DURATION
        ),

        # Text Sensors
        cv.Optional("heating_mode"): text_sensor.text_sensor_schema(),
        cv.Optional("heating_warn"): text_sensor.text_sensor_schema(),

        # Binary Sensors
        cv.Optional("tank_hot"): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_HEAT
        ),
        cv.Optional("sender_battery_low"): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_BATTERY
        ),
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])  # Ensure correct ID usage
    await cg.register_component(var, config)

    # Register numeric sensors
    for key in ["heating_import", "heating_power", "heating_today", "heating_yesterday", "heating_last_7", "heating_last_28", "heating_last_gt", "heating_boost_time"]:
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(getattr(var, f"set_{key}")(sens))

    # Register text sensors
    if "heating_mode" in config:
        mode_sensor = await text_sensor.new_text_sensor(config["heating_mode"])
        cg.add(var.set_heating_mode(mode_sensor))  

    if "heating_warn" in config:
        warn_sensor = await text_sensor.new_text_sensor(config["heating_warn"])
        cg.add(var.set_heating_warn(warn_sensor))

    # Register binary sensors
    if "tank_hot" in config:
        tank_hot_sensor = await binary_sensor.new_binary_sensor(config["tank_hot"])
        cg.add(var.set_tank_hot(tank_hot_sensor))

    if "sender_battery_low" in config:
        battery_low_sensor = await binary_sensor.new_binary_sensor(config["sender_battery_low"])
        cg.add(var.set_sender_battery_low(battery_low_sensor))  

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import time
from esphome import automation
from esphome.const import CONF_ID

CODEOWNERS = ["@10der"]

CONF_CLOCK_TIME = "clock_time"
CONF_ON_PLAY_SOUND = "on_play_sound"

display_tools_ns = cg.esphome_ns.namespace("display_tools")
DisplayTools = display_tools_ns.class_("DisplayTools", cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(DisplayTools),
    cv.Optional(CONF_CLOCK_TIME): cv.use_id(time.RealTimeClock),
    cv.Optional(CONF_ON_PLAY_SOUND): automation.validate_automation(single=True),
})

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if CONF_CLOCK_TIME in config:
        clk = await cg.get_variable(config[CONF_CLOCK_TIME])
        cg.add(var.set_clock_time(clk))

    if CONF_ON_PLAY_SOUND in config:
        await automation.build_automation(
            var.get_on_play_trigger(), [(cg.int_, "x")], config[CONF_ON_PLAY_SOUND]
        )

    #cg.add_library("ArduinoJson", None)
    #cg.add_library("SPI", None)
    #cg.add_library("Wire", None)
    #cg.add_library("Adafruit BusIO", None)
    #cg.add_library("adafruit/Adafruit GFX Library", None)
    #cg.add_library("adafruit/Adafruit Protomatter", None)
    

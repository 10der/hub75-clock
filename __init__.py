import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import time
from esphome.const import CONF_ID

CODEOWNERS = ["@10der"]

CONF_CLOCK_TIME = "clock_time"

display_tools_ns = cg.esphome_ns.namespace("display_tools")
DisplayTools = display_tools_ns.class_("DisplayTools", cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(DisplayTools),
    cv.Optional(CONF_CLOCK_TIME): cv.use_id(time.RealTimeClock),
})

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if CONF_CLOCK_TIME in config:
        clk = await cg.get_variable(config[CONF_CLOCK_TIME])
        cg.add(var.set_clock_time(clk))

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_UART_ID
from esphome.components import uart

# Переконайтесь, що залежності правильно налаштовані
DEPENDENCIES = ["uart"]
CODEOWNERS = ["@10der"]

# Створення простору імен і класу
dfplayer_pro_ns = cg.esphome_ns.namespace("dfplayer_pro")
DFPlayerPro = dfplayer_pro_ns.class_("DFPlayerPro", cg.Component, uart.UARTDevice)

# Схема конфігурації. Вона тепер розширюється схемою UART-пристрою, яка додає CONF_UART_ID
# Це дозволяє вам використовувати uart_id: в esphome.yaml
CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(DFPlayerPro),
    }
).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA)

# Функція, яка генерує код C++
async def to_code(config):
    # Отримати змінну UART-компонента з конфігурації
    # Це створює посилання на той UART-компонент, який ви визначили в YAML
    uart_component = await cg.get_variable(config[CONF_UART_ID])

    # Створити новий екземпляр класу DFPlayerPro, передаючи uart_component до конструктора.
    # Це відповідає конструктору DFPlayerPro(uart::UARTComponent *parent) з вашого .h-файлу.
    var = cg.new_Pvariable(config[CONF_ID], uart_component)
    
    # Зареєструвати компонент.
    await cg.register_component(var, config)


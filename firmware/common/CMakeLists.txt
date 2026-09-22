# 让 firmware/common 作为 ESP-IDF 组件被复用（STM32/Keil 侧忽略此文件）
idf_component_register(
    SRCS
        "radar_proto.c"
        "fall_algo.c"
        "alarm_rule.c"
        "onenet_json.c"
        "modbus_rtu.c"
        "guardian_app.c"
        "motion_link.c"
    INCLUDE_DIRS "."
)

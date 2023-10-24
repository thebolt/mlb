add_library(mqtt_lite_broker_lib lib.c subtree.c)

target_include_directories(mqtt_lite_broker_lib PUBLIC ../include)

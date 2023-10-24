cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(
  mqtt_lite_broker
  VERSION 0.1
  DESCRIPTION "A small, light weight MQTT broker for embedded systems"
  LANGUAGES C)

set(CMAKE_C_STANDARD 11)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

include(nalatest)
include(cmake/CPM.cmake)
cpmaddpackage("gh:TheLartians/Format.cmake@1.7.3")

add_subdirectory(src)
add_subdirectory(tests)

cmake_minimum_required(VERSION 3.24)

# ESP-IDF component
idf_component_register(
    SRCS "./src/source/ttgo_power.c"
    INCLUDE_DIRS "." "./src"
    REQUIRES driver freertos
)

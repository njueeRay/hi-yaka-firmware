idf_component_register(SRCS "main.c" INCLUDE_DIRS "."
                       REQUIRES esp_driver_gpio esp_driver_ledc esp_driver_parlio esp_timer)
set(WS_PARLIO_HZ "1000000" CACHE STRING "Waveshare PARLIO output clock in Hz")
target_compile_definitions(${COMPONENT_LIB} PRIVATE WS_PARLIO_HZ=${WS_PARLIO_HZ})

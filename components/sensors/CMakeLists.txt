idf_component_register(
    SRCS "src/adc_frontend.c" "src/gp2y1014.c" "src/ms1100.c" "src/sensor_service.c"
    INCLUDE_DIRS "include"
    REQUIRES p169h002 esp_adc esp_driver_gpio esp_timer
)

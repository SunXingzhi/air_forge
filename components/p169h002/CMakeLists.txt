idf_component_register(
    SRCS "src/lcd_init.c" "src/lcd.c" "src/cst816.c" "src/demo.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_spi esp_driver_gpio esp_driver_i2c
)

# 厂商 lcdfont.h 用 2 字节字符字面量初始化 GB2312 索引，新编译器会报
# unterminated-string-initialization；该点阵表是固定数据，按警告处理即可。
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unterminated-string-initialization)

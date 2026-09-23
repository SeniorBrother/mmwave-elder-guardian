idf_component_register(
    SRCS
        "app_main.c"
        "bsp_esp32s3.c"
    INCLUDE_DIRS "."
    REQUIRES common driver esp_wifi esp_netif mqtt nvs_flash esp_http_client
             app_update esp_https_ota lvgl
)

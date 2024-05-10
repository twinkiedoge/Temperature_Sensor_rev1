// Minimal example of using ESP-MQTT to publish messages to a broker
// Complete documentation is here:
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/api-reference/protocols/mqtt.html
//
// See also the example code, which demonstrates how to receive messages:
// https://github.com/espressif/esp-idf/blob/5f4249357372f209fdd57288265741aaba21a2b1/examples/protocols/mqtt/tcp/main/app_main.c

#include "freertos/FreeRTOS.h"
#include "freertos/task.h" // Used for timer delay
#include "nvs_flash.h"
#include "esp_netif.h"
#include "mqtt_client.h"
#include "minimal_wifi.h"
#include "thermistor.h"
#include "keepalive.h"
#include "MCP9808.h"
#include <stdio.h> 

#define WIFI_SSID      "Tufts_Wireless"
#define WIFI_PASS      ""

#define BROKER_URI "mqtt://en1-pi.eecs.tufts.edu"

void app_main() {
    // Enable Flash (aka non-volatile storage, NVS)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Normally we'd need to initialize the event loop and the network stack,
    // but both of these will be done when we connect to WiFi.
    printf("Connecting to WiFi...");
    wifi_connect(WIFI_SSID, WIFI_PASS);

    // Initialize the MQTT client
    // Read the documentation for more information on what you can configure:
    // https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/api-reference/protocols/mqtt.html
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = BROKER_URI,
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
   
    if (esp_mqtt_client_start(client) != ESP_OK){
        printf("error initializing client\n");
    }
    
    TaskHandle_t keepalive_task = NULL;
    xTaskCreate(keep_battery_alive, // Function pointer (i.e., name of function to run)
                "KEEPALIVE", // A text name for this task
                4096, // Stack size in words (4 bytes per word)
                NULL, // void* pointer to parameters for task function
                2, // Task priority (higher numbers -> higher priority)
                &keepalive_task);
    printf("Launched keepalive task\n");

    TaskHandle_t report_temp_ic_task = NULL;
    xTaskCreate(report_temp_ic, // Function pointer (i.e., name of function to run)
                "TEMPIC", // A text name for this task
                4096, // Stack size in words (4 bytes per word)
                (void*)&client, // void* pointer to parameters for task function
                1, // Task priority (higher numbers -> higher priority)
                &report_temp_ic_task);
    printf("launched temp IC task\n");

    TaskHandle_t report_temp_thermistor_task = NULL;
    xTaskCreate(report_temp_therm, // Function pointer (i.e., name of function to run)
                "TEMPTHERM", // A text name for this task
                4096, // Stack size in words (4 bytes per word)
                (void*)&client, // void* pointer to parameters for task function
                1, // Task priority (higher numbers -> higher priority)
                &report_temp_thermistor_task);
    printf("launched temp thermistor task\n");

    while(1){
        vTaskDelay(100);
    }
}
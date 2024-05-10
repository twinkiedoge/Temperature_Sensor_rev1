// Minimal code to initialize and read a value from the ADC
// Based on Espressif example: 
// https://github.com/espressif/esp-idf/blob/f68c131e5603feca21659e92ad85f0c3369692fe/examples/peripherals/adc/oneshot_read/main/oneshot_read_main.c
// Steven Bell <sbell@ece.tufts.edu>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h" // Used for timer delay
#include "esp_adc/adc_oneshot.h"
#include "sdkconfig.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "mqtt_client.h"
#include "minimal_wifi.h"

#include <stdio.h>
#include <math.h>

#define ADC_CHANNEL ADC_CHANNEL_0
#define MAX_12_BIT (4095)
#define VIN (3.3)
#define THERMSAMPLES (50)
#define R2 (10000)
#define RREF (7920)
#define B (3380) // b const of spec sheet
#define BTEST (900) // empirically calibrated b const
#define REFTEMP (295.4) // Calibrated by just looking at 550 thermostat

#define MINS_DELAY 1
#define TOPIC "abobro01/hw5/thermistor_temp"


adc_oneshot_unit_handle_t init_adc(void) {
    // Configure the ADC
    adc_oneshot_unit_init_cfg_t adc1_init_cfg = {
        .unit_id = ADC_UNIT_1
    };
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_new_unit(&adc1_init_cfg, &adc1_handle);


    // Configure the channel within the ADC
    adc_oneshot_chan_cfg_t adc1_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT, // Default is 12 bits (max)
        .atten = ADC_ATTEN_DB_11
    };

    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &adc1_cfg);
    
    return adc1_handle;
}

// this is certainly wildly innacurate
float read_therm(adc_oneshot_unit_handle_t adc1_handle){
    float avg = 0;
    int adc_raw;

    for (int i = 0; i < THERMSAMPLES; i++){
        vTaskDelay(100 / portTICK_PERIOD_MS);
        adc_oneshot_read(adc1_handle, ADC_CHANNEL, &adc_raw);
        avg += adc_raw;
    }
    avg /= THERMSAMPLES;
    float vout = avg * (VIN / MAX_12_BIT);
    float resistance = ((VIN*R2)/vout) - R2;
    float temp = (1/REFTEMP) + log(resistance/RREF)/BTEST;
    temp = 1/temp;
    temp = temp - 273.15;

    return temp;
}


void report_temp_therm(void* params)
{
    esp_mqtt_client_handle_t client = *(esp_mqtt_client_handle_t*)params;
    char data[20];

    adc_oneshot_unit_handle_t handle = init_adc();
    float temp;
    
    while(1){
        temp = read_therm(handle);
        vTaskDelay(pdMS_TO_TICKS(60000 * MINS_DELAY));
        sprintf(data, "%0.4f", temp); 
        printf("publishing thermistor temp %s\n", data);
        esp_mqtt_client_publish(client, TOPIC, data, 0, 0, 1);
    }
}
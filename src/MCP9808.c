// thanks chatgpt :)
#include "driver/i2c.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "mqtt_client.h"
#include "minimal_wifi.h"
#include <stdio.h>

#define MINS_DELAY 1
#define TOPIC "abobro01/hw5/ic_temp"

#define I2C_MASTER_SCL_IO    5   /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO    4    /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM       0     /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ   100000 /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0   /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0   /*!< I2C master doesn't need buffer */

#define MCP9808_SENSOR_ADDR  0x18    /*!< slave address for MCP9808 sensor */
#define MCP9808_REG_TEMP     0x05    /*!< Temp register */
#define WRITE_BIT            I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT             I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN         0x1     /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS        0x0     /*!< I2C master will not check ack from slave */
#define ACK_VAL              0x0         /*!< I2C ack value */
#define NACK_VAL             0x1         /*!< I2C nack value */

const char *TAG = "mcp9808";

esp_err_t i2c_master_init(void) 
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode,
                              I2C_MASTER_RX_BUF_DISABLE,
                              I2C_MASTER_TX_BUF_DISABLE, 0);
}


float read_temperature() 
{
    uint8_t write_buf[1] = {MCP9808_REG_TEMP};
    uint8_t read_buf[2];
    float temperature = 0.0;

    // Start I2C write/read transaction
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MCP9808_SENSOR_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write(cmd, write_buf, sizeof(write_buf), ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MCP9808_SENSOR_ADDR << 1) | READ_BIT, ACK_CHECK_EN);
    i2c_master_read(cmd, read_buf, sizeof(read_buf), ACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        // Parse the temperature
        int temp = (read_buf[0] << 8) | read_buf[1];
        temperature = temp & 0x0FFF;
        temperature /= 16.0;
        if (temp & 0x1000) temperature -= 256;
    } else {
        ESP_LOGE(TAG, "Failed to read temperature");
    }

    return temperature;
}

void report_temp_ic(void* params)
{
    esp_mqtt_client_handle_t client = *(esp_mqtt_client_handle_t*)params;
    char data[20];
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI("mcp9808", "I2C initialized successfully");

    while(1){
        float temp = read_temperature();
        vTaskDelay(pdMS_TO_TICKS(60000 * MINS_DELAY));
        sprintf(data, "%0.4f", temp); 
        printf("publishing IC temp %s\n", data);
        if (esp_mqtt_client_publish(client, TOPIC, data, 0, 0, 1) < 0){
            printf("error publishing IC temp\n");
        }
    }
}


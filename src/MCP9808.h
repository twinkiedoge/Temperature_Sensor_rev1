#include "esp_log.h"

#ifndef MCP9808_H
#define MCP9808_H

esp_err_t i2c_master_init(void);
float read_temperature();
void report_temp_ic(void* params);

#endif //MCP9808_H
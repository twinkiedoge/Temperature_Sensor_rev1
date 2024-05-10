#include "esp_adc/adc_oneshot.h"

#ifndef ADC_H
#define ADC_H

adc_oneshot_unit_handle_t init_adc(void);
float read_therm(adc_oneshot_unit_handle_t adc1_handle);
void report_temp_therm(void* params);

#endif //ADC_H
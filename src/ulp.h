
#ifndef _ULP_H_
#define _ULP_H_

#define HALL_CHAN_N ADC_CHANNEL_0
#define HALL_CHAN_P ADC_CHANNEL_3
#define PWR_3V3_CHAN ADC_CHANNEL_6
#define PWR_BAT_CHAN ADC_CHANNEL_5

void init_ulp_program(void);
// uint32_t update_pulse_count(void);
void start_ulp_program(void);

bool hall_detected(void);
uint16_t get_detection_cnt(void);
uint16_t get_hallval(void);
void reset_silentcount(void);
void reset_detection(void);
void print_hallraw(void);

float get_5v(void);
float get_3v(void);

#endif // _ULP_H_

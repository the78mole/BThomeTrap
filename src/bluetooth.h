
#ifndef _BT_H_
#define _BT_H_

void ble_init(void);
void ble_deinit(void);

void ble_advert(uint8_t btn, uint16_t btncnt, float supply, float vbat);

#endif // _BT_H_

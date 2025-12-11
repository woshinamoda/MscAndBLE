#ifndef _BLE_NUS_ORDER_H_
#define _BLE_NUS_ORDER_H_

#include "main.h"

const static uint8_t order_pwdn[4] = {0x50, 0x57, 0x44, 0x4e};
const static uint8_t order_status[6] = {0x53, 0x54, 0x41, 0x54, 0x55, 0x53};


uint8_t che_sync_time_order(const uint8_t *const rxbuf, uint16_t len);
uint8_t che_collect_data_order(const uint8_t *const rxbuf, uint16_t len);
uint8_t che_open_storage_order(const uint8_t *const rxbuf, uint16_t len);
uint8_t che_close_storage_order(const uint8_t *const rxbuf, uint16_t len);
uint8_t che_get_storage_data_order(const uint8_t *const rxbuf, uint16_t len);
uint8_t che_systemoff_order(const uint8_t *const rxbuf, uint16_t len);
uint8_t che_DevStatus_order(const uint8_t *const rxbuf, uint16_t len);
uint8_t che_SetAram_threshold_order(const uint8_t *const rxbuf, uint16_t len);
uint8_t che_clearStorage(const uint8_t *const rxbuf, uint16_t len);










#endif
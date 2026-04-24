/* App/gatt_db.h */
#ifndef GATT_DB_H
#define GATT_DB_H

#include "bluenrg_def.h"
#include <stdint.h>

/* 保留原有的型別定義 */
typedef struct {
  int32_t AXIS_X;
  int32_t AXIS_Y;
  int32_t AXIS_Z;
} AxesRaw_t;

typedef union {
  uint16_t Service_UUID_16;
  uint8_t  Service_UUID_128[16];
} Service_UUID_t;

typedef union {
  uint16_t Char_UUID_16;
  uint8_t  Char_UUID_128[16];
} Char_UUID_t;

/* Characteristic handles（外部存取用）*/
extern uint16_t AccServiceHandle;
extern uint16_t AccDataCharHandle;   /* Char A: NOTIFY */
extern uint16_t AccFreqCharHandle;   /* Char B: WRITE  */

/* API */
tBleStatus Add_AccService(void);
tBleStatus AccData_Update(int16_t x, int16_t y, int16_t z);
void       Read_Request_CB(uint16_t handle);

#endif /* GATT_DB_H */

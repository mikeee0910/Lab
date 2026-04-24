#include "gatt_db.h"
#include "bluenrg_conf.h"
#include "bluenrg_gatt_aci.h"
#include "bluenrg_def.h"
#include <string.h>

#define HOST_TO_LE_16(buf, val) \
  ( ((buf)[0] = (uint8_t)(val)), ((buf)[1] = (uint8_t)((val)>>8)) )

#define COPY_UUID_128(s, b15,b14,b13,b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1,b0) \
do { \
  s[0]=b0; s[1]=b1; s[2]=b2;  s[3]=b3;  s[4]=b4;  s[5]=b5;  s[6]=b6;  s[7]=b7; \
  s[8]=b8; s[9]=b9; s[10]=b10; s[11]=b11; s[12]=b12; s[13]=b13; s[14]=b14; s[15]=b15; \
} while(0)

#define COPY_ACC_SERVICE_UUID(s) \
  COPY_UUID_128(s,0x00,0x00,0x00,0x00,0x00,0x01,0x11,0xe1,0x9a,0xb4,0x00,0x02,0xa5,0xd5,0xc5,0x1b)
#define COPY_ACC_DATA_UUID(s) \
  COPY_UUID_128(s,0x00,0xE0,0x00,0x00,0x00,0x01,0x11,0xe1,0xac,0x36,0x00,0x02,0xa5,0xd5,0xc5,0x1b)
#define COPY_ACC_FREQ_UUID(s) \
  COPY_UUID_128(s,0x00,0xE0,0x00,0x01,0x00,0x01,0x11,0xe1,0xac,0x36,0x00,0x02,0xa5,0xd5,0xc5,0x1b)

uint16_t AccServiceHandle  = 0;
uint16_t AccDataCharHandle = 0;
uint16_t AccFreqCharHandle = 0;

static Service_UUID_t service_uuid;
static Char_UUID_t    char_uuid;

tBleStatus Add_AccService(void)
{
  tBleStatus ret;
  uint8_t uuid[16];

  /* Service */
  COPY_ACC_SERVICE_UUID(uuid);
  BLUENRG_memcpy(&service_uuid.Service_UUID_128, uuid, 16);
  ret = aci_gatt_add_serv(UUID_TYPE_128, service_uuid.Service_UUID_128,
                           PRIMARY_SERVICE, 7, &AccServiceHandle);
  if (ret != BLE_STATUS_SUCCESS) return ret;

  /* Char A: AccData, NOTIFY, 6 bytes */
  COPY_ACC_DATA_UUID(uuid);
  BLUENRG_memcpy(&char_uuid.Char_UUID_128, uuid, 16);
  ret = aci_gatt_add_char(AccServiceHandle,
                           UUID_TYPE_128, char_uuid.Char_UUID_128,
                           6,
                           CHAR_PROP_NOTIFY,
                           ATTR_PERMISSION_NONE,
                           GATT_DONT_NOTIFY_EVENTS,
                           16, 0, &AccDataCharHandle);
  if (ret != BLE_STATUS_SUCCESS) return ret;

  /* Char B: AccFreq, WRITE, 2 bytes */
  COPY_ACC_FREQ_UUID(uuid);
  BLUENRG_memcpy(&char_uuid.Char_UUID_128, uuid, 16);
  ret = aci_gatt_add_char(AccServiceHandle,
                           UUID_TYPE_128, char_uuid.Char_UUID_128,
                           2,
                           CHAR_PROP_WRITE | CHAR_PROP_WRITE_WITHOUT_RESP,
                           ATTR_PERMISSION_NONE,
                           GATT_NOTIFY_ATTRIBUTE_WRITE,
                           16, 0, &AccFreqCharHandle);
  return ret;
}

tBleStatus AccData_Update(int16_t x, int16_t y, int16_t z)
{
  uint8_t buff[6];
  HOST_TO_LE_16(buff,   x);
  HOST_TO_LE_16(buff+2, y);
  HOST_TO_LE_16(buff+4, z);
  return aci_gatt_update_char_value(AccServiceHandle, AccDataCharHandle,
                                    0, 6, buff);
}

void Read_Request_CB(uint16_t handle)
{
  (void)handle;
}

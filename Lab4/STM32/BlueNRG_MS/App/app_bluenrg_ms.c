/* App/app_bluenrg_ms.c */
#include "app_bluenrg_ms.h"
#include "hci.h"
#include "hci_le.h"
#include "hci_tl.h"
#include "bluenrg_gap.h"
#include "bluenrg_gap_aci.h"
#include "bluenrg_gatt_aci.h"
#include "bluenrg_hal_aci.h"
#include "sensor.h"
#include "gatt_db.h"
#include "b_l475e_iot01a2.h"
#include <string.h>
#include <stdio.h>
#include "bluenrg_utils.h"
extern volatile uint8_t set_connectable;

uint8_t bnrg_expansion_board = IDB04A1;
uint8_t bdaddr[BDADDR_SIZE];

void MX_BlueNRG_MS_Init(void)
{
  uint16_t service_handle, dev_name_char_handle, appearance_char_handle;
  uint8_t  bdaddr_len_out;
  uint8_t  hwVersion;
  uint16_t fwVersion;
  int ret;

  const char *name = "STM32-ACC";

  hci_init(user_notify, NULL);

  getBlueNRGVersion(&hwVersion, &fwVersion);

  hci_reset();
  HAL_Delay(100);

  if (hwVersion > 0x30) {
    bnrg_expansion_board = IDB05A1;
  }

  /* 讀取隨機 MAC */
  ret = aci_hal_read_config_data(CONFIG_DATA_RANDOM_ADDRESS,
                                  BDADDR_SIZE, &bdaddr_len_out, bdaddr);
  if (ret || (bdaddr[5] & 0xC0) != 0xC0) {
    /* 讀取失敗時使用固定地址 */
    uint8_t fixed_addr[] = {0xAA,0xBB,0xCC,0xDD,0xEE,0xFF};
    memcpy(bdaddr, fixed_addr, BDADDR_SIZE);
  }

  /* GATT init */
  ret = aci_gatt_init();
  if (ret) { PRINTF("GATT_Init failed\n"); return; }

  /* GAP init */
  if (bnrg_expansion_board == IDB05A1) {
    ret = aci_gap_init_IDB05A1(GAP_PERIPHERAL_ROLE_IDB05A1, 0, 0x07,
                                &service_handle, &dev_name_char_handle,
                                &appearance_char_handle);
  } else {
    ret = aci_gap_init_IDB04A1(GAP_PERIPHERAL_ROLE_IDB04A1,
                                &service_handle, &dev_name_char_handle,
                                &appearance_char_handle);
  }
  if (ret != BLE_STATUS_SUCCESS) { PRINTF("GAP_Init failed\n"); return; }

  /* 設備名稱 */
  aci_gatt_update_char_value(service_handle, dev_name_char_handle,
                              0, strlen(name), (uint8_t*)name);

  /* 建立 Accelerometer GATT Service */
  ret = Add_AccService();
  if (ret == BLE_STATUS_SUCCESS) {
    PRINTF("AccService added OK. "
           "SvcH=0x%04X DataH=0x%04X FreqH=0x%04X\r\n",
           AccServiceHandle, AccDataCharHandle, AccFreqCharHandle);
  } else {
    PRINTF("Add_AccService failed: 0x%02X\r\n", ret);
    return;
  }

  aci_hal_set_tx_power_level(1, 4);

  PRINTF("BLE Init OK, advertising as \"%s\"\r\n", name);
}

void MX_BlueNRG_MS_Process(void)
{
  if (set_connectable) {
    Set_DeviceConnectable();
    set_connectable = FALSE;
  }
  hci_user_evt_proc();
}

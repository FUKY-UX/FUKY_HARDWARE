/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "esp_hidd_prf_api.h"
#include "hidd_le_prf_int.h"
#include "hid_dev.h"
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"

// HID mouse input report length
#define HID_MOUSE_IN_RPT_LEN        5

// HID mouse input report length
#define HID_IMU_IN_RPT_LEN          16

esp_err_t esp_hidd_register_callbacks(esp_hidd_event_cb_t callbacks)
{
    esp_err_t hidd_status;

    if(callbacks != NULL) {
   	    hidd_le_env.hidd_cb = callbacks;
    } else {
        return ESP_FAIL;
    }

    if((hidd_status = hidd_register_cb()) != ESP_OK) {
        return hidd_status;
    }
    //esp_ble_gatts_app_register(HID_BAS_APP_ID);
    if((hidd_status = esp_ble_gatts_app_register(HIDD_APP_ID)) != ESP_OK) {
        return hidd_status;
    }
    //esp_ble_gatts_app_register(DIS_APP_ID);

    return hidd_status;
}

esp_err_t esp_hidd_profile_init(void)
{
     if (hidd_le_env.enabled) {
        ESP_LOGE(HID_LE_PRF_TAG, "HID device profile already initialized");
        return ESP_FAIL;
    }
    // Reset the hid device target environment
    memset(&hidd_le_env, 0, sizeof(hidd_le_env_t));
    hidd_le_env.enabled = true;
    return ESP_OK;
}

esp_err_t esp_hidd_profile_deinit(void)
{
    uint16_t hidd_svc_hdl = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_SVC];
    if (!hidd_le_env.enabled) {
        ESP_LOGE(HID_LE_PRF_TAG, "HID device profile already initialized");
        return ESP_OK;
    }

    if(hidd_svc_hdl != 0) {
	esp_ble_gatts_stop_service(hidd_svc_hdl);
	esp_ble_gatts_delete_service(hidd_svc_hdl);
    } else {
	return ESP_FAIL;
   }

    /* register the HID device profile to the BTA_GATTS module*/
    esp_ble_gatts_app_unregister(hidd_le_env.gatt_if);

    return ESP_OK;
}

uint16_t esp_hidd_get_version(void)
{
	return HIDD_VERSION;
}

void esp_hidd_send_mouse_value(uint16_t conn_id, uint8_t mouse_button, int8_t mickeys_x, int8_t mickeys_y)
{
    uint8_t buffer[HID_MOUSE_IN_RPT_LEN];

    buffer[0] = mouse_button;   // Buttons
    buffer[1] = mickeys_x;           // X
    buffer[2] = mickeys_y;           // Y
    buffer[3] = 0;           // Wheel
    buffer[4] = 0;           // AC Pan

    hid_dev_send_report(hidd_le_env.gatt_if, conn_id,
                        HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT, HID_MOUSE_IN_RPT_LEN, buffer);
    return;
}

void esp_hidd_send_hid_report(uint16_t conn_id,uint8_t mouse_button, int8_t mickeys_x,int8_t mickeys_y,IMUReport_t *report) {

    // ESP_LOGI("HID_COMBINED", "Sending HID Report: Btn: %d, X: %d, Y: %d, Ax: %.2f, Ay: %.2f, Az: %.2f",
    //          report->mouse_button, report->mickeys_x, report->mickeys_y,
    //          report->lin_accel_x, report->lin_accel_y, report->lin_accel_z);
    uint8_t bufferM[HID_MOUSE_IN_RPT_LEN];
    bufferM[0] = mouse_button;   // Buttons
    bufferM[1] = mickeys_x;           // X
    bufferM[2] = mickeys_y;           // Y
    bufferM[3] = 0;           // Wheel
    bufferM[4] = 0;           // AC Pan

    uint8_t bufferIMU[sizeof(report)];
    memcpy(bufferIMU, report, sizeof(report));

    // 通过 HID 发送鼠标数据（Report ID = 1）
    hid_dev_send_report(hidd_le_env.gatt_if, conn_id, 1, HID_REPORT_TYPE_INPUT, HID_MOUSE_IN_RPT_LEN, bufferM);
    // 通过 HID 发送IMU数据（Report ID = 5）
    //hid_dev_send_report(hidd_le_env.gatt_if, conn_id, 5, HID_REPORT_TYPE_INPUT, sizeof(report), bufferIMU);
}

void esp_hidd_send_imu_value(uint16_t conn_id, int16_t acc_x, int16_t acc_y, int16_t acc_z,
    int16_t quat_w, int16_t quat_x, int16_t quat_y, int16_t quat_z)
{
uint8_t buffer[HID_IMU_IN_RPT_LEN];  // 确保这个长度足够容纳数据

buffer[0] = HID_RPT_ID_IMU_IN;   // Report ID (假设为 2)
buffer[1] = (uint8_t)(acc_x & 0xFF);
buffer[2] = (uint8_t)((acc_x >> 8) & 0xFF);
buffer[3] = (uint8_t)(acc_y & 0xFF);
buffer[4] = (uint8_t)((acc_y >> 8) & 0xFF);
buffer[5] = (uint8_t)(acc_z & 0xFF);
buffer[6] = (uint8_t)((acc_z >> 8) & 0xFF);

buffer[7]  = (uint8_t)(quat_w & 0xFF);
buffer[8]  = (uint8_t)((quat_w >> 8) & 0xFF);
buffer[9]  = (uint8_t)(quat_x & 0xFF);
buffer[10] = (uint8_t)((quat_x >> 8) & 0xFF);
buffer[11] = (uint8_t)(quat_y & 0xFF);
buffer[12] = (uint8_t)((quat_y >> 8) & 0xFF);
buffer[13] = (uint8_t)(quat_z & 0xFF);
buffer[14] = (uint8_t)((quat_z >> 8) & 0xFF);

hid_dev_send_report(hidd_le_env.gatt_if, conn_id,
HID_RPT_ID_IMU_IN, HID_REPORT_TYPE_INPUT, HID_IMU_IN_RPT_LEN, buffer);
}
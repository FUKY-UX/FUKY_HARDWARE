/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

 #include "hidd_le_prf_int.h"
 #include <string.h>
 #include "esp_log.h"
 
 
 
 /// characteristic presentation information
 struct prf_char_pres_fmt
 {
     /// Unit (The Unit is a UUID)
     uint16_t unit;
     /// Description
     uint16_t description;
     /// Format
     uint8_t format;
     /// Exponent
     uint8_t exponent;
     /// Name space
     uint8_t name_space;
 };
 
 // HID report mapping table
 static hid_report_map_t hid_rpt_map[HID_NUM_REPORTS];
 
 
 static const uint8_t hidReportMap[] = {
     0x05, 0x01,  // Usage Page (Generic Desktop)
     0x09, 0x02,  // Usage (Mouse)
     0xA1, 0x01,  // Collection (Application)
     0x85, 0x01,  // Report Id (1)
     0x09, 0x01,  //   Usage (Pointer)
     0xA1, 0x00,  //   Collection (Physical)
     0x05, 0x09,  //     Usage Page (Buttons)
     0x19, 0x01,  //     Usage Minimum (01) - Button 1
     0x29, 0x03,  //     Usage Maximum (03) - Button 3
     0x15, 0x00,  //     Logical Minimum (0)
     0x25, 0x01,  //     Logical Maximum (1)
     0x75, 0x01,  //     Report Size (1)
     0x95, 0x03,  //     Report Count (3)
     0x81, 0x02,  //     Input (Data, Variable, Absolute) - Button states
     0x75, 0x05,  //     Report Size (5)
     0x95, 0x01,  //     Report Count (1)
     0x81, 0x01,  //     Input (Constant) - Padding or Reserved bits
     0x05, 0x01,  //     Usage Page (Generic Desktop)
     0x09, 0x30,  //     Usage (X)
     0x09, 0x31,  //     Usage (Y)
     0x09, 0x38,  //     Usage (Wheel)
     0x15, 0x81,  //     Logical Minimum (-127)
     0x25, 0x7F,  //     Logical Maximum (127)
     0x75, 0x08,  //     Report Size (8)
     0x95, 0x03,  //     Report Count (3)
     0x81, 0x06,  //     Input (Data, Variable, Relative) - X & Y coordinate
     0xC0,        //   End Collection
     0xC0,        // End Collection
 
 };
 
 #define HI_UINT16(a) (((a) >> 8) & 0xFF)
 #define LO_UINT16(a) ((a) & 0xFF)
 #define PROFILE_NUM            1
 #define PROFILE_APP_IDX        0
 
 struct gatts_profile_inst {
     esp_gatts_cb_t gatts_cb;
     uint16_t gatts_if;
     uint16_t app_id;
     uint16_t conn_id;
 };
 
 hidd_le_env_t hidd_le_env;
 
 // HID report map length
 uint8_t hidReportMapLen = sizeof(hidReportMap);
 uint8_t hidProtocolMode = HID_PROTOCOL_MODE_REPORT;
 
 // HID report mapping table
 //static hidRptMap_t  hidRptMap[HID_NUM_REPORTS];
 
 // HID Information characteristic value
 static const uint8_t hidInfo[HID_INFORMATION_LEN] = {
     LO_UINT16(0x0111), HI_UINT16(0x0111),             // bcdHID (USB HID version)
     0x00,                                             // bCountryCode
     HID_KBD_FLAGS                                     // Flags
 };
 
 
 // HID Report Reference characteristic descriptor, mouse input
 static uint8_t hidReportRefMouseIn[HID_REPORT_REF_LEN] =
              { HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT };
 
 // // HID Report Reference characteristic descriptor, IMU
 // static uint8_t hidReportRefIMUin[HID_REPORT_REF_LEN] =
 //              { HID_RPT_ID_IMU_IN, HID_REPORT_TYPE_INPUT };
 
 // HID Report Reference characteristic descriptor, Feature
 static uint8_t hidReportRefFeature[HID_REPORT_REF_LEN] =
              { HID_RPT_ID_FEATURE, HID_REPORT_TYPE_FEATURE };
 
 /*
  *  Heart Rate PROFILE ATTRIBUTES
  ****************************************************************************************
  */
 
 /// hid Service uuid
 static uint16_t hid_le_svc = ATT_SVC_HID;
 uint16_t            hid_count = 0;
 //esp_gatts_incl_svc_desc_t incl_svc = {0};
 //esp_gatts_incl_svc_desc_t incl_dis_svc = {0};
 
 #define CHAR_DECLARATION_SIZE   (sizeof(uint8_t))
 ///the uuid definition
 static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
 //static const uint16_t include_service_uuid = ESP_GATT_UUID_INCLUDE_SERVICE;
 static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
 static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
 static const uint16_t hid_info_char_uuid = ESP_GATT_UUID_HID_INFORMATION;
 static const uint16_t hid_report_map_uuid    = ESP_GATT_UUID_HID_REPORT_MAP;
 static const uint16_t hid_control_point_uuid = ESP_GATT_UUID_HID_CONTROL_POINT;
 static const uint16_t hid_report_uuid = ESP_GATT_UUID_HID_REPORT;
 static const uint16_t hid_proto_mode_uuid = ESP_GATT_UUID_HID_PROTO_MODE;
 static const uint16_t hid_mouse_input_uuid = ESP_GATT_UUID_HID_BT_MOUSE_INPUT;
 //static const uint16_t hid_repot_map_ext_desc_uuid = ESP_GATT_UUID_EXT_RPT_REF_DESCR;
 static const uint16_t hid_report_ref_descr_uuid = ESP_GATT_UUID_RPT_REF_DESCR;
 
 ///the propoty definition
 //static const uint8_t char_prop_notify = ESP_GATT_CHAR_PROP_BIT_NOTIFY;
 static const uint8_t char_prop_read = ESP_GATT_CHAR_PROP_BIT_READ;
 static const uint8_t char_prop_write_nr = ESP_GATT_CHAR_PROP_BIT_WRITE_NR;
 static const uint8_t char_prop_read_write = ESP_GATT_CHAR_PROP_BIT_WRITE|ESP_GATT_CHAR_PROP_BIT_READ;
 static const uint8_t char_prop_read_notify = ESP_GATT_CHAR_PROP_BIT_READ|ESP_GATT_CHAR_PROP_BIT_NOTIFY;
 //static const uint8_t char_prop_read_write_notify = ESP_GATT_CHAR_PROP_BIT_READ|ESP_GATT_CHAR_PROP_BIT_WRITE|ESP_GATT_CHAR_PROP_BIT_NOTIFY;
 //static const uint8_t char_prop_read_write_write_nr = ESP_GATT_CHAR_PROP_BIT_READ|ESP_GATT_CHAR_PROP_BIT_WRITE|ESP_GATT_CHAR_PROP_BIT_WRITE_NR;
 
 /// Full Hid device Database Description - Used to add attributes into the database
 static esp_gatts_attr_db_t hidd_le_gatt_db[HIDD_LE_IDX_NB] =
 {
     // HID Service Declaration
     [HIDD_LE_IDX_SVC]                       = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid,
                                                              ESP_GATT_PERM_READ_ENCRYPTED, sizeof(uint16_t), sizeof(hid_le_svc),
                                                             (uint8_t *)&hid_le_svc}},
                                                             
     // HID Information Characteristic Declaration
     [HIDD_LE_IDX_HID_INFO_CHAR]     = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid,
                                                             ESP_GATT_PERM_READ,
                                                             CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE,
                                                             (uint8_t *)&char_prop_read}},
     // HID Information Characteristic Value
     [HIDD_LE_IDX_HID_INFO_VAL]       = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_info_char_uuid,
                                                             ESP_GATT_PERM_READ,
                                                             sizeof(hids_hid_info_t), sizeof(hidInfo),
                                                             (uint8_t *)&hidInfo}},
 
     // HID Control Point Characteristic Declaration
     [HIDD_LE_IDX_HID_CTNL_PT_CHAR]  = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid,
                                                               ESP_GATT_PERM_READ,
                                                               CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE,
                                                               (uint8_t *)&char_prop_write_nr}},
     // HID Control Point Characteristic Value
     [HIDD_LE_IDX_HID_CTNL_PT_VAL]    = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_control_point_uuid,
                                                              ESP_GATT_PERM_WRITE,
                                                              sizeof(uint8_t), 0,
                                                              NULL}},
 
     // Report Map Characteristic Declaration
     [HIDD_LE_IDX_REPORT_MAP_CHAR]   = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid,
                                                               ESP_GATT_PERM_READ,
                                                               CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE,
                                                               (uint8_t *)&char_prop_read}},
     // Report Map Characteristic Value
     [HIDD_LE_IDX_REPORT_MAP_VAL]     = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_report_map_uuid,
                                                               ESP_GATT_PERM_READ,
                                                               HIDD_LE_REPORT_MAP_MAX_LEN, sizeof(hidReportMap),
                                                               (uint8_t *)&hidReportMap}},
 
 
     // Protocol Mode Characteristic Declaration
     [HIDD_LE_IDX_PROTO_MODE_CHAR]            = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid,
                                                                         ESP_GATT_PERM_READ,
                                                                         CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE,
                                                                         (uint8_t *)&char_prop_read_write}},
     // Protocol Mode Characteristic Value
     [HIDD_LE_IDX_PROTO_MODE_VAL]               = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_proto_mode_uuid,
                                                                         (ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE),
                                                                         sizeof(uint8_t), sizeof(hidProtocolMode),
                                                                         (uint8_t *)&hidProtocolMode}},
     // MOUSE Characteristic Value
     [HIDD_LE_IDX_REPORT_MOUSE_IN_CHAR]       = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid,
                                                                          ESP_GATT_PERM_READ,
                                                                          CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE,
                                                                          (uint8_t *)&char_prop_read_notify}},
 
     [HIDD_LE_IDX_REPORT_MOUSE_IN_VAL]        = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_report_uuid,
                                                                        ESP_GATT_PERM_READ,
                                                                        HIDD_LE_REPORT_MAX_LEN, 0,
                                                                        NULL}},
 
     [HIDD_LE_IDX_REPORT_MOUSE_IN_CCC]        = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid,
                                                                       (ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE),
                                                                       sizeof(uint16_t), 0,
                                                                       NULL}},
 
     [HIDD_LE_IDX_REPORT_MOUSE_REP_REF]       = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_report_ref_descr_uuid,
                                                                        ESP_GATT_PERM_READ,
                                                                        sizeof(hidReportRefMouseIn), sizeof(hidReportRefMouseIn),
                                                                        hidReportRefMouseIn}},
 
     // // IMU Characteristic Value
     // [HIDD_LE_IDX_REPORT_IMU_IN_CHAR]       = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid,
     //                                                                 ESP_GATT_PERM_READ,
     //                                                                 CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE,
     //                                                                 (uint8_t *)&char_prop_read_notify}},
 
     // [HIDD_LE_IDX_REPORT_IMU_IN_VAL]        = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_report_uuid,
     //                                                                 ESP_GATT_PERM_READ,
     //                                                                 HIDD_LE_REPORT_MAX_LEN, 0,
     //                                                                 NULL}},
 
     // [HIDD_LE_IDX_REPORT_IMU_IN_CCC]        = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, 
     //                                                                 (uint8_t *)&character_client_config_uuid,
     //                                                                 (ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE),
     //                                                                 sizeof(uint16_t), 0,
     //                                                                 NULL}},
 
     // [HIDD_LE_IDX_REPORT_IMU_REP_REF]       = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_report_ref_descr_uuid,
     //                                                                 ESP_GATT_PERM_READ,
     //                                                                 sizeof(hidReportRefIMUin), sizeof(hidReportRefIMUin),
     //                                                                 hidReportRefIMUin}},
 
     // Boot Mouse Input Report Characteristic Declaration
     [HIDD_LE_IDX_BOOT_MOUSE_IN_REPORT_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid,
                                                                               ESP_GATT_PERM_READ,
                                                                               CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE,
                                                                               (uint8_t *)&char_prop_read_notify}},
     // Boot Mouse Input Report Characteristic Value
     [HIDD_LE_IDX_BOOT_MOUSE_IN_REPORT_VAL]   = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_mouse_input_uuid,
                                                                               ESP_GATT_PERM_READ,
                                                                               HIDD_LE_BOOT_REPORT_MAX_LEN, 0,
                                                                               NULL}},
     // Boot Mouse Input Report Characteristic - Client Characteristic Configuration Descriptor
     [HIDD_LE_IDX_BOOT_MOUSE_IN_REPORT_NTF_CFG]    = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid,
                                                                                       (ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE),
                                                                                       sizeof(uint16_t), 0,
                                                                                       NULL}},
 
     // Report Characteristic Declaration
     [HIDD_LE_IDX_REPORT_CHAR]                    = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid,
                                                                          ESP_GATT_PERM_READ,
                                                                          CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE,
                                                                          (uint8_t *)&char_prop_read_write}},
     // Report Characteristic Value
     [HIDD_LE_IDX_REPORT_VAL]                      = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_report_uuid,
                                                                        ESP_GATT_PERM_READ,
                                                                        HIDD_LE_REPORT_MAX_LEN, 0,
                                                                        NULL}},
     // Report Characteristic - Report Reference Descriptor
     [HIDD_LE_IDX_REPORT_REP_REF]               = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_report_ref_descr_uuid,
                                                                        ESP_GATT_PERM_READ,
                                                                        sizeof(hidReportRefFeature), sizeof(hidReportRefFeature),
                                                                        hidReportRefFeature}},
 };
 
 static void hid_add_id_tbl(void);
 
 void esp_hidd_prf_cb_hdl(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                     esp_ble_gatts_cb_param_t *param)
 {
     switch(event) {
         case ESP_GATTS_REG_EVT: {
             esp_ble_gap_config_local_icon (ESP_BLE_APPEARANCE_GENERIC_HID);
             esp_hidd_cb_param_t hidd_param;
             hidd_param.init_finish.state = param->reg.status;
             if(param->reg.app_id == HIDD_APP_ID) {
                 hidd_le_env.gatt_if = gatts_if;
                 if(hidd_le_env.hidd_cb != NULL) {
                     (hidd_le_env.hidd_cb)(ESP_HIDD_EVENT_REG_FINISH, &hidd_param);
                     esp_ble_gatts_create_attr_tab(hidd_le_gatt_db, gatts_if, HIDD_LE_IDX_NB, 0);
                 }
             }
             
             break;
         }
         case ESP_GATTS_CONF_EVT: {
             break;
         }
         case ESP_GATTS_CREATE_EVT:
             break;
         case ESP_GATTS_CONNECT_EVT: {
             esp_hidd_cb_param_t cb_param = {0};
             ESP_LOGI(HID_LE_PRF_TAG, "HID connection establish, conn_id = %x",param->connect.conn_id);
             memcpy(cb_param.connect.remote_bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
             cb_param.connect.conn_id = param->connect.conn_id;
             hidd_clcb_alloc(param->connect.conn_id, param->connect.remote_bda);
             esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_NO_MITM);
             if(hidd_le_env.hidd_cb != NULL) {
                 (hidd_le_env.hidd_cb)(ESP_HIDD_EVENT_BLE_CONNECT, &cb_param);
             }
             break;
         }
         case ESP_GATTS_DISCONNECT_EVT: {
              if(hidd_le_env.hidd_cb != NULL) {
                     (hidd_le_env.hidd_cb)(ESP_HIDD_EVENT_BLE_DISCONNECT, NULL);
              }
             hidd_clcb_dealloc(param->disconnect.conn_id);
             break;
         }
         case ESP_GATTS_CLOSE_EVT:
             break;
         case ESP_GATTS_WRITE_EVT: {
             //esp_hidd_cb_param_t cb_param = {0};
 
             break;
         }
         case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
             if(param->add_attr_tab.status != ESP_GATT_OK)
             {
                 ESP_LOGE("报错", "表没有创建,服务号为%d",param->add_attr_tab.svc_inst_id);
 
             }
             if (param->add_attr_tab.num_handle == HIDD_LE_IDX_NB &&
                 param->add_attr_tab.status == ESP_GATT_OK)
             {
                 memcpy(hidd_le_env.hidd_inst.att_tbl, param->add_attr_tab.handles,HIDD_LE_IDX_NB*sizeof(uint16_t));
                 ESP_LOGI(HID_LE_PRF_TAG, "hid svc handle = %x",hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_SVC]);
                 ESP_LOGI("GATT", "attribute table created, %d handles:", param->add_attr_tab.num_handle);
                 for (int i = 0; i < param->add_attr_tab.num_handle; i++) {
                     ESP_LOGI("GATT", "Att_idx %d -> handle 0x%04x[%04x]", i, param->add_attr_tab.handles[i],param->add_attr_tab.svc_uuid.uuid.uuid16);
                 }
                 hid_add_id_tbl();
                 esp_ble_gatts_start_service(hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_SVC]);
             }
             else 
             {
                 esp_ble_gatts_start_service(param->add_attr_tab.handles[0]);
             }
 
                 
 
             break;
         }
 
         default:
             break;
     }
 }
 
 
 
 void hidd_le_init(void)
 {
     // Reset the hid device target environment
     memset(&hidd_le_env, 0, sizeof(hidd_le_env_t));
 }
 
 void hidd_clcb_alloc (uint16_t conn_id, esp_bd_addr_t bda)
 {
     uint8_t                   i_clcb = 0;
     hidd_clcb_t      *p_clcb = NULL;
 
     for (i_clcb = 0, p_clcb= hidd_le_env.hidd_clcb; i_clcb < HID_MAX_APPS; i_clcb++, p_clcb++) {
         if (!p_clcb->in_use) {
             p_clcb->in_use      = true;
             p_clcb->conn_id     = conn_id;
             p_clcb->connected   = true;
             memcpy (p_clcb->remote_bda, bda, ESP_BD_ADDR_LEN);
             break;
         }
     }
     return;
 }
 
 bool hidd_clcb_dealloc (uint16_t conn_id)
 {
     uint8_t              i_clcb = 0;
     hidd_clcb_t      *p_clcb = NULL;
 
     for (i_clcb = 0, p_clcb= hidd_le_env.hidd_clcb; i_clcb < HID_MAX_APPS; i_clcb++, p_clcb++) {
             memset(p_clcb, 0, sizeof(hidd_clcb_t));
             return true;
     }
 
     return false;
 }
 
 static struct gatts_profile_inst heart_rate_profile_tab[PROFILE_NUM] = {
     [PROFILE_APP_IDX] = {
         .gatts_cb = esp_hidd_prf_cb_hdl,
         .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
     },
 
 };
 
 static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                 esp_ble_gatts_cb_param_t *param)
 {
     /* If event is register event, store the gatts_if for each profile */
     if (event == ESP_GATTS_REG_EVT) 
     {
         if (param->reg.status == ESP_GATT_OK) {
             heart_rate_profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
         } else {
             ESP_LOGI(HID_LE_PRF_TAG, "Reg app failed, app_id %04x, status %d",
                     param->reg.app_id,
                     param->reg.status);
             return;
         }
     }
 
     do {
         int idx;
         for (idx = 0; idx < PROFILE_NUM; idx++) {
             if (gatts_if == ESP_GATT_IF_NONE || gatts_if == heart_rate_profile_tab[idx].gatts_if) {
                 if (heart_rate_profile_tab[idx].gatts_cb) {
                     heart_rate_profile_tab[idx].gatts_cb(event, gatts_if, param);
                 }
             }
         }
     } while (0);
 }
 
 esp_err_t hidd_register_cb(void)
 {
     esp_err_t status;
     status = esp_ble_gatts_register_callback(gatts_event_handler);
     return status;
 }
 
 void hidd_set_attr_value(uint16_t handle, uint16_t val_len, const uint8_t *value)
 {
     hidd_inst_t *hidd_inst = &hidd_le_env.hidd_inst;
     if(hidd_inst->att_tbl[HIDD_LE_IDX_HID_INFO_VAL] <= handle &&
         hidd_inst->att_tbl[HIDD_LE_IDX_REPORT_REP_REF] >= handle) {
         esp_ble_gatts_set_attr_value(handle, val_len, value);
     } else {
         ESP_LOGE(HID_LE_PRF_TAG, "%s error:Invalid handle value.",__func__);
     }
     return;
 }
 
 void hidd_get_attr_value(uint16_t handle, uint16_t *length, uint8_t **value)
 {
     hidd_inst_t *hidd_inst = &hidd_le_env.hidd_inst;
     if(hidd_inst->att_tbl[HIDD_LE_IDX_HID_INFO_VAL] <= handle &&
         hidd_inst->att_tbl[HIDD_LE_IDX_REPORT_REP_REF] >= handle){
         esp_ble_gatts_get_attr_value(handle, length, (const uint8_t **)value);
     } else {
         ESP_LOGE(HID_LE_PRF_TAG, "%s error:Invalid handle value.", __func__);
     }
 
     return;
 }
 
 static void hid_add_id_tbl(void)
 {
     // Mouse input report
     hid_rpt_map[0].id = hidReportRefMouseIn[0];
     hid_rpt_map[0].type = hidReportRefMouseIn[1];
     hid_rpt_map[0].handle = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_REPORT_MOUSE_IN_VAL];
     hid_rpt_map[0].cccdHandle = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_REPORT_MOUSE_IN_CCC];
     hid_rpt_map[0].mode = HID_PROTOCOL_MODE_REPORT;
 
     // // IMU input report
     // hid_rpt_map[1].id = hidReportRefIMUin[0];
     // hid_rpt_map[1].type = hidReportRefIMUin[1];
     // hid_rpt_map[1].handle = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_REPORT_IMU_IN_VAL];
     // hid_rpt_map[1].cccdHandle = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_REPORT_IMU_IN_CCC];
     // hid_rpt_map[1].mode = HID_PROTOCOL_MODE_REPORT;
 
     // report Boot mouse input
     // Use same ID and type as mouse input report
     hid_rpt_map[1].id = hidReportRefMouseIn[0];
     hid_rpt_map[1].type = hidReportRefMouseIn[1];
     hid_rpt_map[1].handle = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_BOOT_MOUSE_IN_REPORT_VAL];
     hid_rpt_map[1].cccdHandle = 0;
     hid_rpt_map[1].mode = HID_PROTOCOL_MODE_BOOT;
 
     // Feature report
     hid_rpt_map[2].id = hidReportRefFeature[0];
     hid_rpt_map[2].type = hidReportRefFeature[1];
     hid_rpt_map[2].handle = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_REPORT_VAL];
     hid_rpt_map[2].cccdHandle = 0;
     hid_rpt_map[2].mode = HID_PROTOCOL_MODE_REPORT;
 
 
   // Setup report ID map
   hid_dev_register_reports(HID_NUM_REPORTS, hid_rpt_map);
 }
 
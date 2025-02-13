#ifndef BLE_HIDD_H
#define BLE_HIDD_H

#ifdef __cplusplus
extern "C" {
#endif

void BLE_HID_Init(void);

void SendData(uint8_t mouse_button, int8_t mickeys_x, int8_t mickeys_y,
    float AccX,float AccY,float AccZ,
    float QuatI,float QuatJ,float QuatK,float QuatW,float Interval);


#ifdef __cplusplus
}
#endif

#endif // BLE_HIDD_H

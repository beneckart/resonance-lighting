#pragma once
#include <stdint.h>
bool usbServiceActive();
bool usbServiceReady();
void usbServicePrepare();
void usbServiceCancel();
void usbServiceTick();
void powerRecoveryTest(uint16_t tier);

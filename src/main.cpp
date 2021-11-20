/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include <Arduino.h>

#include <ESP8266WiFi.h>

#include "sensor.h"
#include "control.h"
#include "rtcmem_map.h"

/* enable ADC and configure for external circuitry */
ADC_MODE(ADC_TOUT);

#if 0
/* From
 * ESP8266WiFi/examples/EarlyDisableWiFi/EarlyDisableWiFi.ino
 */
void preinit() {
    /*
     * Global WiFi constructors are not called yet
     * (global class instances like WiFi, Serial... are not yet initialized)..
     * No global object methods or C++ exceptions can be called in here!
     * The below is a static class method, which is similar to a function, so it's ok.
     */
    ESP8266WiFiClass::preinitWiFiOff();
}
#endif

FirmwareControl ctrl;

void setup() {
    Serial.begin(115200);
    Serial.println();
    ctrl.setup();
}

void loop() {
    ctrl.loop();
    delay(100);
}

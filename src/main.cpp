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

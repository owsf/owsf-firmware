/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include "control.h"


FirmwareControl ctrl;

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.printf("ESP8266 Firmware Version %s (%s)\n", VERSION, DATE);
    ctrl.setup();
}

void loop() {
    ctrl.loop();
    delay(500);
}

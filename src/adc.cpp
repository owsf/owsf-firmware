/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "adc.h"

void Sensor_ADC::publish(Point &p) {
    p.addField("voltage", current_value);
}

Sensor_State Sensor_ADC::sample() {
    if (state != SENSOR_INIT)
        return state;

    Serial.printf("  Sampling sensor ADC\n");

    current_value = factor * (1. * analogRead(0)) * (1 + r1 / r2) / 1024 + offset;

    state = SENSOR_DONE_UPDATE;

    return state;
}

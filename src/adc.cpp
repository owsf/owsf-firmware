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
    state = SENSOR_DONE_NOUPDATE;

    Serial.printf("  Sampling sensor ADC\n");

    current_value = factor * (1. * analogRead(0)) * (1 + r1 / r2) / 1024 + offset;

    if (mem < 0) {
        state = SENSOR_DONE_UPDATE;
        return state;
    }

    ESP.rtcUserMemoryRead(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));
    if (threshold_helper_float(current_value, &rtc_data.current_value, threshold_voltage))
        state = SENSOR_DONE_UPDATE;
    ESP.rtcUserMemoryWrite(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));

    return state;
}

const char *Sensor_ADC::get_sensor_type() {
    return sensor_type;
}

String &Sensor_ADC::get_tags() {
    return tags;
}
